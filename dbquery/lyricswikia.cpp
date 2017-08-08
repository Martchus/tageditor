#include "./lyricswikia.h"

#include "../misc/networkaccessmanager.h"
#include "../application/settings.h"

#include <QUrlQuery>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QXmlStreamReader>
#include <QTextDocument>
#include <QStringBuilder>

#include <functional>

using namespace std;
using namespace std::placeholders;
using namespace Utility;

namespace QtGui {

static const QString defaultLyricsWikiaUrl(QStringLiteral("https://lyrics.wikia.com"));

QUrl lyricsWikiaApiUrl()
{
    const QString &lyricsWikiaUrl = Settings::values().dbQuery.lyricsWikiaUrl;
    return QUrl((lyricsWikiaUrl.isEmpty() ? defaultLyricsWikiaUrl : lyricsWikiaUrl) + QStringLiteral("/api.php"));
}

LyricsWikiaResultsModel::LyricsWikiaResultsModel(SongDescription &&initialSongDescription, QNetworkReply *reply) :
    HttpResultsModel(move(initialSongDescription), reply)
{}

bool LyricsWikiaResultsModel::fetchCover(const QModelIndex &index)
{
    if(!index.parent().isValid() && index.row() < m_results.size()) {
        SongDescription &desc = m_results[index.row()];
        if(!desc.cover.isEmpty()) {
            // cover is already available -> nothing to do
        } else if(!desc.albumId.isEmpty()) {
            try {
                // the item belongs to an album which cover has already been fetched
                desc.cover = m_coverData.at(desc.albumId);
            } catch(const out_of_range &) {
                if(desc.coverUrl.isEmpty()) {
                    // request the cover URL
                    auto *reply = requestAlbumDetails(desc);
                    addReply(reply, bind(&LyricsWikiaResultsModel::handleAlbumDetailsReplyFinished, this, reply, index.row()));
                    setFetchingCover(true);
                    return false;
                } else {
                    // request the cover art
                    auto *reply = networkAccessManager().get(QNetworkRequest(QUrl(desc.coverUrl)));
                    addReply(reply, bind(&LyricsWikiaResultsModel::handleCoverReplyFinished, this, reply, desc.albumId, index.row()));
                    setFetchingCover(true);
                    return false;
                }
            }
        } else {
            m_errorList << tr("Unable to fetch cover: Album ID unknown");
            emit resultsAvailable();
        }
    }
    return true;
}

bool LyricsWikiaResultsModel::fetchLyrics(const QModelIndex &index)
{
    if(!index.parent().isValid() && index.row() < m_results.size()) {
        SongDescription &desc = m_results[index.row()];
        if(!desc.lyrics.isEmpty()) {
            // lyrics already available -> nothing to do
        } else if(!desc.artist.isEmpty() && !desc.title.isEmpty()) {
            auto *reply = requestSongDetails(desc);
            addReply(reply, bind(&LyricsWikiaResultsModel::handleSongDetailsFinished, this, reply, index.row()));
            return false;
        } else {
            m_errorList << tr("Unable to fetch lyrics: Artist or title is unknown.");
            emit resultsAvailable();
        }
    }
    return true;
}

void LyricsWikiaResultsModel::parseInitialResults(const QByteArray &data)
{
    // prepare parsing LyricsWikia meta data
    beginResetModel();
    m_results.clear();
    QXmlStreamReader xmlReader(data);

    // parse XML tree
#include <qtutilities/misc/xmlparsermacros.h>
    children {
        iftag("getArtistResponse") {
            QString artist;
            children {
                iftag("artist") {
                    artist = text;
                } eliftag("albums") {
                    children {
                        iftag("albumResult") {
                            QString album, year;
                            QList<SongDescription> songs;
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
                                            songs.back().track = songs.size();
                                        } else_skip
                                    }
                                } else_skip
                            }
                            // need to filter results manually because the filtering provided by Lyrica Wiki API doesn't work
                            if((m_initialDescription.album.isEmpty() || m_initialDescription.album == album)
                               && (m_initialDescription.year.isEmpty() || m_initialDescription.year == year)
                               && (!m_initialDescription.totalTracks || m_initialDescription.totalTracks == songs.size())) {
                                for(SongDescription &song : songs) {
                                    if((m_initialDescription.title.isEmpty() || m_initialDescription.title == song.title)
                                            && (!m_initialDescription.track || m_initialDescription.track == song.track)) {
                                        song.album = album;
                                        song.year = year;
                                        song.totalTracks = songs.size();
                                        m_results << move(song);
                                    }
                                }
                            }
                        } else_skip
                    }
                } else_skip
            }
            for(SongDescription &song : m_results) {
                // set the arist which is the same for all results
                song.artist = artist;
                // set the album ID (album is identified by its artist, year and name)
                song.albumId = artist % QChar(':') % song.album % QChar('_') % QChar('(') % song.year % QChar(')');
                song.albumId.replace(QChar(' '), QChar('_'));
            }
        } else_skip
    }
#include <qtutilities/misc/undefxmlparsermacros.h>

    // check for parsing errors
    switch(xmlReader.error()) {
    case QXmlStreamReader::NoError:
    case QXmlStreamReader::PrematureEndOfDocumentError:
        break;
    default:
        m_errorList << xmlReader.errorString();
    }

    // promote changes
    endResetModel();
}

QNetworkReply *LyricsWikiaResultsModel::requestSongDetails(const SongDescription &songDescription)
{
    // compose URL
    QUrlQuery query;
    query.addQueryItem(QStringLiteral("func"), QStringLiteral("getSong"));
    query.addQueryItem(QStringLiteral("action"), QStringLiteral("lyrics"));
    query.addQueryItem(QStringLiteral("fmt"), QStringLiteral("xml"));
    query.addQueryItem(QStringLiteral("fixXML"), QString());
    query.addQueryItem(QStringLiteral("artist"), songDescription.artist);
    query.addQueryItem(QStringLiteral("title"), songDescription.title);
    if(!songDescription.album.isEmpty()) {
        // specifying album seems to have no effect but also doesn't hurt
        query.addQueryItem(QStringLiteral("album"), songDescription.album);
    }
    QUrl url(lyricsWikiaApiUrl());
    url.setQuery(query);

    return Utility::networkAccessManager().get(QNetworkRequest(url));
}

QNetworkReply *LyricsWikiaResultsModel::requestAlbumDetails(const SongDescription &songDescription)
{
    QUrl url(lyricsWikiaApiUrl());
    url.setPath(QStringLiteral("/wiki/") + songDescription.albumId);
    return Utility::networkAccessManager().get(QNetworkRequest(url));
}

void LyricsWikiaResultsModel::handleSongDetailsFinished(QNetworkReply *reply, int row)
{
    QByteArray data;
    if(auto *newReply = evaluateReplyResults(reply, data, true)) {
        addReply(newReply, bind(&LyricsWikiaResultsModel::handleSongDetailsFinished, this, newReply, row));
    } else if(!data.isEmpty()) {
        parseSongDetails(row, data);
    }
}

void LyricsWikiaResultsModel::parseSongDetails(int row, const QByteArray &data)
{
    // find associated result/desc
    if(row >= m_results.size()) {
        m_errorList << tr("Internal error: context for song details reply invalid");
        setResultsAvailable(true);
        return;
    }
    const SongDescription &assocDesc = m_results.at(row);

    QUrl parsedUrl;

    // parse XML tree
    QXmlStreamReader xmlReader(data);
#include <qtutilities/misc/xmlparsermacros.h>
    children {
        iftag("LyricsResult") {
            SongDescription parsedDesc;
            children {
                iftag("artist") {
                    parsedDesc.artist = text;
                } eliftag("song") {
                    parsedDesc.title = text;
                } eliftag("url") {
                    parsedUrl = text;
                } else_skip
            }

            // verify whether parsed results match what was requested
            if(!parsedUrl.isEmpty() && assocDesc.title == parsedDesc.title && assocDesc.artist == parsedDesc.artist) {
                break;
            } else {
                parsedUrl.clear();
            }
        } else_skip
    }
#include <qtutilities/misc/undefxmlparsermacros.h>

    // check for parsing errors
    switch(xmlReader.error()) {
    case QXmlStreamReader::NoError:
    case QXmlStreamReader::PrematureEndOfDocumentError:
        break;
    default:
        m_errorList << tr("Unable to parse song details: ") + xmlReader.errorString();
        setResultsAvailable(true);
        if(parsedUrl.isEmpty()) {
            return; // don't complain about missing URL when the XML isn't even valid
        }
    }

    // requets lyrics (seem to be incomplete in XML response, so just get the regular Wiki page)
    if(parsedUrl.isEmpty()) {
        m_errorList << tr("Song details requested for %1/%2 do not contain URL for Wiki page").arg(assocDesc.artist, assocDesc.title);
        setResultsAvailable(true);
        return;
    }
    // do not use parsed URL directly to avoid unintended requests
    QUrl requestUrl(lyricsWikiaApiUrl());
    requestUrl.setPath(parsedUrl.path());
    auto *reply = Utility::networkAccessManager().get(QNetworkRequest(parsedUrl));
    addReply(reply, bind(&LyricsWikiaResultsModel::handleLyricsReplyFinished, this, reply, row));
}

void LyricsWikiaResultsModel::handleLyricsReplyFinished(QNetworkReply *reply, int row)
{
    QByteArray data;
    if(auto *newReply = evaluateReplyResults(reply, data, true)) {
        addReply(newReply, bind(&LyricsWikiaResultsModel::handleLyricsReplyFinished, this, newReply, row));
    } else {
        if(!data.isEmpty()) {
            parseLyricsResults(row, data);
        }
        setResultsAvailable(true);
    }
}

void LyricsWikiaResultsModel::parseLyricsResults(int row, const QByteArray &data)
{
    // find associated result/desc
    if(row >= m_results.size()) {
        m_errorList << tr("Internal error: context for LyricsWikia page reply invalid");
        setResultsAvailable(true);
        return;
    }
    SongDescription &assocDesc = m_results[row];

    // convert data to QString
    const QString html(data);

    // parse lyrics from HTML
    const int lyricsStart = html.indexOf(QLatin1String("<div class='lyricbox'>"));
    if(lyricsStart < 0) {
        m_errorList << tr("Song details requested for %1/%2 do not contain lyrics").arg(assocDesc.artist, assocDesc.title);
        setResultsAvailable(true);
        return;
    }
    const int lyricsEnd = html.indexOf(QLatin1String("<div class='lyricsbreak'></div>"), lyricsStart);
    QTextDocument textDoc;
    textDoc.setHtml(html.mid(lyricsStart, (lyricsEnd > lyricsStart) ? (lyricsEnd - lyricsStart) : -1));
    assocDesc.lyrics = textDoc.toPlainText();

    emit lyricsAvailable(index(row, 0));
}

void LyricsWikiaResultsModel::handleAlbumDetailsReplyFinished(QNetworkReply *reply, int row)
{
    QByteArray data;
    if(auto *newReply = evaluateReplyResults(reply, data, true)) {
        addReply(newReply, bind(&LyricsWikiaResultsModel::handleAlbumDetailsReplyFinished, this, newReply, row));
    } else {
        parseAlbumDetailsAndFetchCover(row, data);
    }
}

void LyricsWikiaResultsModel::parseAlbumDetailsAndFetchCover(int row, const QByteArray &data)
{
    // check whether the request failed (sufficient error message already emitted in this case)
    if(data.isEmpty()) {
        setFetchingCover(false);
        setResultsAvailable(true);
        return;
    }

    // find associated result/desc
    if(row >= m_results.size()) {
        m_errorList << tr("Internal error: context for LyricsWikia page reply invalid");
        setFetchingCover(false);
        setResultsAvailable(true);
        return;
    }
    SongDescription &assocDesc = m_results[row];

    // convert data to QString
    const QString html(data);

    // parse cover URL from HTML
    const int coverDivStart = html.indexOf(QLatin1String("<div class=\"plainlinks\" style=\"clear:right; float:right;")) + 56;
    if(coverDivStart > 56) {
        const int coverHrefStart = html.indexOf(QLatin1String("href=\""), coverDivStart) + 6;
        if(coverHrefStart > coverDivStart + 6) {
            const int coverHrefEnd = html.indexOf(QLatin1String("\""), coverHrefStart);
            if(coverHrefEnd > 0) {
                assocDesc.coverUrl = html.mid(coverHrefStart, coverHrefEnd - coverHrefStart);
            }
        }
    }

    // handle error case (cover URL not found)
    if(assocDesc.coverUrl.isEmpty()) {
        m_errorList << tr("Album details requested for %1/%2 do not contain cover").arg(assocDesc.artist, assocDesc.album);
        setFetchingCover(false);
        setResultsAvailable(true);
        return;
    }

    // request the cover art
    auto *reply = networkAccessManager().get(QNetworkRequest(QUrl(assocDesc.coverUrl)));
    addReply(reply, bind(&LyricsWikiaResultsModel::handleCoverReplyFinished, this, reply, assocDesc.albumId, row));
}

QueryResultsModel *queryLyricsWikia(SongDescription &&songDescription)
{
    // compose URL
    QUrlQuery query;
    query.addQueryItem(QStringLiteral("func"), QStringLiteral("getArtist"));
    query.addQueryItem(QStringLiteral("fmt"), QStringLiteral("xml"));
    query.addQueryItem(QStringLiteral("fixXML"), QString());
    query.addQueryItem(QStringLiteral("artist"), songDescription.artist);
    QUrl url(lyricsWikiaApiUrl());
    url.setQuery(query);

    // NOTE: Only getArtist seems to work, so artist must be specified and filtering must
    // be done manually when parsing results.

    // make request
    return new LyricsWikiaResultsModel(move(songDescription), Utility::networkAccessManager().get(QNetworkRequest(url)));
}

} // namespace QtGui
