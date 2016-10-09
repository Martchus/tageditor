#include "./lyricswikia.h"

#include "../misc/networkaccessmanager.h"
#include "../application/settings.h"

#include <QUrlQuery>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QXmlStreamReader>
#include <QTextDocument>

#include <functional>

using namespace std;
using namespace std::placeholders;
using namespace Utility;

namespace QtGui {

static const QString defaultLyricsWikiaUrl(QStringLiteral("https://lyrics.wikia.com"));

LyricsWikiaResultsModel::LyricsWikiaResultsModel(SongDescription &&initialSongDescription, QNetworkReply *reply) :
    HttpResultsModel(move(initialSongDescription), reply)
{}

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
    // prepare parsing MusicBrainz meta data
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
                                            songs.back().track = static_cast<unsigned int>(songs.size());
                                        } else_skip
                                    }
                                } else_skip
                            }
                            // need to filter results manually because the filtering provided by Lyrica Wiki API doesn't work
                            if((m_initialDescription.album.isEmpty() || m_initialDescription.album == album)
                               && (m_initialDescription.year.isEmpty() || m_initialDescription.year == year)
                               && (!m_initialDescription.totalTracks || m_initialDescription.totalTracks == static_cast<unsigned int>(songs.size()))) {
                                for(SongDescription &song : songs) {
                                    if((m_initialDescription.title.isEmpty() || m_initialDescription.title == song.title)
                                            && (!m_initialDescription.track || m_initialDescription.track == song.track)) {
                                        song.album = album;
                                        song.year = year;
                                        song.totalTracks = static_cast<unsigned int>(songs.size());
                                        m_results << move(song);
                                    }
                                }
                            }
                        } else_skip
                    }
                } else_skip
            }
            for(SongDescription &song : m_results) {
                song.artist = artist;
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
    QUrl url((Settings::lyricsWikiaUrl().isEmpty() ? defaultLyricsWikiaUrl : Settings::lyricsWikiaUrl()) + QStringLiteral("/api.php"));
    QUrlQuery query;
    query.addQueryItem(QStringLiteral("func"), QStringLiteral("getSong"));
    query.addQueryItem(QStringLiteral("action"), QStringLiteral("lyrics"));
    query.addQueryItem(QStringLiteral("fmt"), QStringLiteral("xml"));
    query.addQueryItem(QStringLiteral("fixXML"), QString());
    query.addQueryItem(QStringLiteral("artist"), songDescription.artist);
    query.addQueryItem(QStringLiteral("title"), songDescription.title);
    if(!songDescription.album.isEmpty()) {
        // specifying album seems to have no effect but also don't hurt
        query.addQueryItem(QStringLiteral("album"), songDescription.album);
    }
    url.setQuery(query);

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
    QUrl requestUrl(Settings::lyricsWikiaUrl().isEmpty() ? defaultLyricsWikiaUrl : Settings::lyricsWikiaUrl());
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

    // parse lyrics from HTML
    QString html(data);
    int lyricsStart = html.indexOf("<div class='lyricbox'>");
    if(!lyricsStart) {
        m_errorList << tr("Song details requested for %1/%2 do not contain lyrics").arg(assocDesc.artist, assocDesc.title);
        setResultsAvailable(true);
        return;
    }
    int lyricsEnd = html.indexOf("<div class='lyricsbreak'></div>", lyricsStart);
    QTextDocument textDoc;
    textDoc.setHtml(html.mid(lyricsStart, (lyricsEnd > lyricsStart) ? (lyricsEnd - lyricsStart) : -1));

    assocDesc.lyrics = textDoc.toPlainText();
    emit lyricsAvailable(index(row, 0));
}

QueryResultsModel *queryLyricsWikia(SongDescription &&songDescription)
{
    // compose URL
    QUrl url((Settings::lyricsWikiaUrl().isEmpty() ? defaultLyricsWikiaUrl : Settings::lyricsWikiaUrl()) + QStringLiteral("/api.php"));
    QUrlQuery query;
    query.addQueryItem(QStringLiteral("func"), QStringLiteral("getArtist"));
    query.addQueryItem(QStringLiteral("fmt"), QStringLiteral("xml"));
    query.addQueryItem(QStringLiteral("fixXML"), QString());
    query.addQueryItem(QStringLiteral("artist"), songDescription.artist);
    url.setQuery(query);

    // NOTE: Only getArtist seems to work, so artist must be specified and filtering must
    // be done manually when parsing results.

    // make request
    return new LyricsWikiaResultsModel(move(songDescription), Utility::networkAccessManager().get(QNetworkRequest(url)));
}

} // namespace QtGui
