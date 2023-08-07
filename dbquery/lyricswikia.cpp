#include "./lyricswikia.h"

#include "../application/settings.h"
#include "../misc/networkaccessmanager.h"
#include "../misc/utility.h"

#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QStringBuilder>
#include <QTextDocument>
#include <QUrlQuery>
#include <QXmlStreamReader>

#include <functional>

using namespace std;
using namespace std::placeholders;
using namespace Utility;

namespace QtGui {

static const auto defaultLyricsWikiaUrl = QStringLiteral("https://lyrics.fandom.com");

static QUrl lyricsWikiaApiUrl()
{
    const auto &lyricsWikiaUrl = Settings::values().dbQuery.lyricsWikiaUrl;
    return QUrl((lyricsWikiaUrl.isEmpty() ? defaultLyricsWikiaUrl : lyricsWikiaUrl) + QStringLiteral("/api.php"));
}

static void lazyInitializeLyricsWikiaSongId(SongDescription &desc)
{
    if (!desc.songId.isEmpty()) {
        return;
    }
    desc.songId = desc.artist % QChar(':') % desc.title;
    desc.songId.replace(QChar(' '), QChar('_'));
}

LyricsWikiaResultsModel::LyricsWikiaResultsModel(SongDescription &&initialSongDescription, QNetworkReply *reply)
    : HttpResultsModel(std::move(initialSongDescription), reply)
{
}

bool LyricsWikiaResultsModel::fetchCover(const QModelIndex &index)
{
    if (index.parent().isValid() || !index.isValid() || index.row() >= m_results.size()) {
        return true;
    }

    // skip if cover is already available
    auto &desc = m_results[index.row()];
    if (!desc.cover.isEmpty()) {
        return true;
    }

    // fail if album ID is unknown
    if (desc.albumId.isEmpty()) {
        m_errorList << tr("Unable to fetch cover: Album ID unknown");
        emit resultsAvailable();
        return true;
    }

    // skip if the item belongs to an album which cover has already been fetched
    const auto coverData = s_coverData.find(desc.albumId);
    if (coverData != s_coverData.end()) {
        desc.cover = coverData->second;
        return true;
    }

    // start http request
    if (desc.coverUrl.isEmpty()) {
        // request the cover URL
        auto *const reply = requestAlbumDetails(desc);
        addReply(reply, bind(&LyricsWikiaResultsModel::handleAlbumDetailsReplyFinished, this, reply, index.row()));
        setFetchingCover(true);
    } else {
        // request the cover art
        auto *const reply = networkAccessManager().get(QNetworkRequest(QUrl(desc.coverUrl)));
        addReply(reply, bind(&LyricsWikiaResultsModel::handleCoverReplyFinished, this, reply, desc.albumId, index.row()));
        setFetchingCover(true);
    }
    return false;
}

bool LyricsWikiaResultsModel::fetchLyrics(const QModelIndex &index)
{
    if (index.parent().isValid() || !index.isValid() || index.row() >= m_results.size()) {
        return true;
    }

    // skip if lyrics already present
    auto &desc = m_results[index.row()];
    if (!desc.lyrics.isEmpty()) {
        return true;
    }

    // fail if artist or title unknown
    if (desc.artist.isEmpty() || desc.title.isEmpty()) {
        m_errorList << tr("Unable to fetch lyrics: Artist or title is unknown.");
        emit resultsAvailable();
        return true;
    }

    // request lyrics
    auto *reply = requestSongDetails(desc);
    addReply(reply, bind(&LyricsWikiaResultsModel::handleSongDetailsFinished, this, reply, index.row()));
    return false;
}

void LyricsWikiaResultsModel::parseInitialResults(const QByteArray &data)
{
    beginResetModel();
    m_results.clear();

    // parse XML tree
    auto xmlReader = QXmlStreamReader(data);
    // clang-format off
#include <qtutilities/misc/xmlparsermacros.h>
    children {
        iftag("getArtistResponse") {
            auto artist = QString();
            children {
                iftag("artist") {
                    artist = text;
                } eliftag("albums") {
                    children {
                        iftag("albumResult") {
                            auto album = QString(), year = QString();
                            auto songs = QList<SongDescription>();
                            children {
                                iftag("album") {
                                    album = text;
                                } eliftag("year") {
                                    year = text;
                                } eliftag("songs") {
                                    children {
                                        iftag("item") {
                                            songs << SongDescription();
                                            songs.back().title = text;
                                            songs.back().track = Utility::containerSizeToInt(songs.size());
                                        }
                                        else_skip
                                    }
                                }
                                else_skip
                            }
                            // need to filter results manually because the filtering provided by Lyrica Wiki API doesn't work
                            if ((m_initialDescription.album.isEmpty() || m_initialDescription.album == album)
                                && (m_initialDescription.year.isEmpty() || m_initialDescription.year == year)
                                && (!m_initialDescription.totalTracks || m_initialDescription.totalTracks == songs.size())) {
                                for (auto &song : songs) {
                                    if ((m_initialDescription.title.isEmpty() || m_initialDescription.title == song.title)
                                        && (!m_initialDescription.track || m_initialDescription.track == song.track)) {
                                        song.album = album;
                                        song.year = year;
                                        song.totalTracks = Utility::containerSizeToInt(songs.size());
                                        m_results << std::move(song);
                                    }
                                }
                            }
                        }
                        else_skip
                    }
                }
                else_skip
            }
            for (auto &song : m_results) {
                // set the arist which is the same for all results
                song.artist = artist;
                // set the album ID (album is identified by its artist, year and name)
                song.albumId = artist % QChar(':') % song.album % QChar('_') % QChar('(') % song.year % QChar(')');
                song.albumId.replace(QChar(' '), QChar('_'));
            }
        }
        else_skip
    }
#include <qtutilities/misc/undefxmlparsermacros.h>
    // clang-format on

    // check for parsing errors
    switch (xmlReader.error()) {
    case QXmlStreamReader::NoError:
    case QXmlStreamReader::PrematureEndOfDocumentError:
        break;
    default:
        m_errorList << xmlReader.errorString();
    }

    endResetModel();
}

QNetworkReply *LyricsWikiaResultsModel::requestSongDetails(const SongDescription &songDescription)
{
    // compose URL
    auto query = QUrlQuery();
    query.addQueryItem(QStringLiteral("func"), QStringLiteral("getSong"));
    query.addQueryItem(QStringLiteral("action"), QStringLiteral("lyrics"));
    query.addQueryItem(QStringLiteral("fmt"), QStringLiteral("xml"));
    query.addQueryItem(QStringLiteral("fixXML"), QString());
    query.addQueryItem(QStringLiteral("artist"), songDescription.artist);
    query.addQueryItem(QStringLiteral("title"), songDescription.title);
    if (!songDescription.album.isEmpty()) {
        // specifying album seems to have no effect but also doesn't hurt
        query.addQueryItem(QStringLiteral("album"), songDescription.album);
    }
    auto url = lyricsWikiaApiUrl();
    url.setQuery(query);
    return Utility::networkAccessManager().get(QNetworkRequest(url));
}

QNetworkReply *LyricsWikiaResultsModel::requestAlbumDetails(const SongDescription &songDescription)
{
    auto url = lyricsWikiaApiUrl();
    url.setPath(QStringLiteral("/wiki/") + songDescription.albumId);
    return Utility::networkAccessManager().get(QNetworkRequest(url));
}

void LyricsWikiaResultsModel::handleSongDetailsFinished(QNetworkReply *reply, int row)
{
    auto data = QByteArray();
    if (auto *newReply = evaluateReplyResults(reply, data, true)) {
        addReply(newReply, bind(&LyricsWikiaResultsModel::handleSongDetailsFinished, this, newReply, row));
    } else if (!data.isEmpty()) {
        parseSongDetails(row, data);
    }
}

void LyricsWikiaResultsModel::parseSongDetails(int row, const QByteArray &data)
{
    if (row >= m_results.size()) {
        m_errorList << tr("Internal error: context for song details reply invalid");
        setResultsAvailable(true);
        return;
    }

    // parse XML tree
    auto &assocDesc = m_results[row];
    auto parsedUrl = QUrl();
    auto xmlReader = QXmlStreamReader(data);
    // clang-format off
#include <qtutilities/misc/xmlparsermacros.h>
    children {
        iftag("LyricsResult") {
            auto parsedDesc = SongDescription();
            children {
                iftag("artist") {
                    parsedDesc.artist = text;
                } eliftag("song") {
                    parsedDesc.title = text;
                } eliftag("url") {
                    parsedUrl = text;
                }
                else_skip
            }

            // verify whether parsed results match what was requested
            if (!parsedUrl.isEmpty() && assocDesc.title == parsedDesc.title && assocDesc.artist == parsedDesc.artist) {
                break;
            } else {
                parsedUrl.clear();
            }
        }
        else_skip
    }
#include <qtutilities/misc/undefxmlparsermacros.h>
    // clang-format on

    // check for parsing errors
    switch (xmlReader.error()) {
    case QXmlStreamReader::NoError:
    case QXmlStreamReader::PrematureEndOfDocumentError:
        break;
    default:
        m_errorList << tr("Unable to parse song details: ") + xmlReader.errorString();
        setResultsAvailable(true);
        if (parsedUrl.isEmpty()) {
            return; // don't complain about missing URL when the XML isn't even valid
        }
    }

    // requets lyrics (lyrics are truncated in XML response so just get them via the regular Wiki page)
    // -> fall back to self-crafted URL if it was not possible to parse it from XML
    if (parsedUrl.isEmpty()) {
        lazyInitializeLyricsWikiaSongId(assocDesc);
        parsedUrl.setPath(QStringLiteral("/wiki/") + assocDesc.songId);
        m_errorList << tr(
            "Song details requested for %1/%2 do not contain URL for Wiki page or do not match requested song; using self-crafted link instead")
                           .arg(assocDesc.artist, assocDesc.title);
    }
    // -> do not use parsed URL "as-is" in any case to avoid unintended requests
    auto requestUrl = lyricsWikiaApiUrl();
    requestUrl.setPath(parsedUrl.path());
    // -> initialize the actual request
    auto *const reply = Utility::networkAccessManager().get(QNetworkRequest(requestUrl));
    addReply(reply, bind(&LyricsWikiaResultsModel::handleLyricsReplyFinished, this, reply, row));
}

void LyricsWikiaResultsModel::handleLyricsReplyFinished(QNetworkReply *reply, int row)
{
    auto data = QByteArray();
    if (auto *newReply = evaluateReplyResults(reply, data, true)) {
        addReply(newReply, bind(&LyricsWikiaResultsModel::handleLyricsReplyFinished, this, newReply, row));
        return;
    }
    if (!data.isEmpty()) {
        parseLyricsResults(row, data);
    }
    if (!m_resultsAvailable) {
        setResultsAvailable(true);
    }
}

void LyricsWikiaResultsModel::parseLyricsResults(int row, const QByteArray &data)
{
    if (row >= m_results.size()) {
        m_errorList << tr("Internal error: context for LyricsWikia page reply invalid");
        setResultsAvailable(true);
        return;
    }

    // parse lyrics from HTML
    auto &assocDesc = m_results[row];
    const auto html = QString(data);
    const auto lyricsStart = html.indexOf(QLatin1String("<div class='lyricbox'>"));
    if (lyricsStart < 0) {
        m_errorList << tr("Song details requested for %1/%2 do not contain lyrics").arg(assocDesc.artist, assocDesc.title);
        setResultsAvailable(true);
        return;
    }
    const auto lyricsEnd = html.indexOf(QLatin1String("<div class='lyricsbreak'></div>"), lyricsStart);
    auto textDoc = QTextDocument();
    textDoc.setHtml(html.mid(lyricsStart, (lyricsEnd > lyricsStart) ? (lyricsEnd - lyricsStart) : -1));
    assocDesc.lyrics = textDoc.toPlainText();

    setResultsAvailable(true);
    emit lyricsAvailable(index(row, 0));
}

void LyricsWikiaResultsModel::handleAlbumDetailsReplyFinished(QNetworkReply *reply, int row)
{
    auto data = QByteArray();
    if (auto *newReply = evaluateReplyResults(reply, data, true)) {
        addReply(newReply, bind(&LyricsWikiaResultsModel::handleAlbumDetailsReplyFinished, this, newReply, row));
    } else {
        parseAlbumDetailsAndFetchCover(row, data);
    }
}

void LyricsWikiaResultsModel::parseAlbumDetailsAndFetchCover(int row, const QByteArray &data)
{
    // check whether the request failed (sufficient error message already emitted in this case)
    if (data.isEmpty()) {
        setFetchingCover(false);
        setResultsAvailable(true);
        return;
    }

    if (row >= m_results.size()) {
        m_errorList << tr("Internal error: context for LyricsWikia page reply invalid");
        setFetchingCover(false);
        setResultsAvailable(true);
        return;
    }

    // parse cover URL from HTML
    auto &assocDesc = m_results[row];
    const auto html = QString(data);
    const auto coverDivStart = html.indexOf(QLatin1String("<div class=\"plainlinks\" style=\"clear:right; float:right;")) + 56;
    if (coverDivStart > 56) {
        const auto coverHrefStart = html.indexOf(QLatin1String("href=\""), coverDivStart) + 6;
        if (coverHrefStart > coverDivStart + 6) {
            const auto coverHrefEnd = html.indexOf(QLatin1String("\""), coverHrefStart);
            if (coverHrefEnd > 0) {
                assocDesc.coverUrl = html.mid(coverHrefStart, coverHrefEnd - coverHrefStart);
            }
        }
    }

    // handle error case (cover URL not found)
    if (assocDesc.coverUrl.isEmpty()) {
        m_errorList << tr("Album details requested for %1/%2 do not contain cover").arg(assocDesc.artist, assocDesc.album);
        setFetchingCover(false);
        setResultsAvailable(true);
        return;
    }

    // request the cover art
    auto *const reply = networkAccessManager().get(QNetworkRequest(QUrl(assocDesc.coverUrl)));
    addReply(reply, bind(&LyricsWikiaResultsModel::handleCoverReplyFinished, this, reply, assocDesc.albumId, row));
}

QUrl LyricsWikiaResultsModel::webUrl(const QModelIndex &index)
{
    if (index.parent().isValid() || !index.isValid() || index.row() >= m_results.size()) {
        return QUrl();
    }

    auto &desc = m_results[index.row()];
    lazyInitializeLyricsWikiaSongId(desc);

    // return URL
    auto url = lyricsWikiaApiUrl();
    url.setPath(QStringLiteral("/wiki/") + desc.songId);
    return url;
}

QueryResultsModel *queryLyricsWikia(SongDescription &&songDescription)
{
    auto query = QUrlQuery();
    query.addQueryItem(QStringLiteral("func"), QStringLiteral("getArtist"));
    query.addQueryItem(QStringLiteral("fmt"), QStringLiteral("xml"));
    query.addQueryItem(QStringLiteral("fixXML"), QString());
    query.addQueryItem(QStringLiteral("artist"), songDescription.artist);
    auto url = lyricsWikiaApiUrl();
    url.setQuery(query);
    return new LyricsWikiaResultsModel(std::move(songDescription), Utility::networkAccessManager().get(QNetworkRequest(url)));

    // NOTE: Only getArtist seems to work, so artist must be specified and filtering must
    // be done manually when parsing results.
}

} // namespace QtGui
