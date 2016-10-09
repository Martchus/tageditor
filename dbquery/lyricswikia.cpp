#include "./lyricswikia.h"

#include "../misc/networkaccessmanager.h"
#include "../application/settings.h"

#include <QUrlQuery>
#include <QNetworkAccessManager>
#include <QNetworkRequest>

using namespace std;
using namespace Utility;

namespace QtGui {

LyricsWikiaResultsModel::LyricsWikiaResultsModel(SongDescription &&initialSongDescription, QNetworkReply *reply) :
    HttpResultsModel(move(initialSongDescription), reply)
{}

void LyricsWikiaResultsModel::parseResults(QNetworkReply *reply, const QByteArray &data)
{
    Q_UNUSED(reply)

    // prepare parsing MusicBrainz meta data
    beginResetModel();
    m_results.clear();
    m_reader.addData(data);

    // parse XML tree
#define xmlReader m_reader
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
                                // need to filter results manually because the filtering provided by Lyrica Wiki API doesn't work
                                if((m_initialDescription.album.isEmpty() || m_initialDescription.album == album)
                                   && (m_initialDescription.year.isEmpty() || m_initialDescription.year == year)
                                   && (!m_initialDescription.totalTracks || m_initialDescription.totalTracks == static_cast<unsigned int>(songs.size()))) {
                                    for(SongDescription &song : songs) {
                                        if((m_initialDescription.title.isEmpty() || m_initialDescription.title == song.title)
                                                && (!m_initialDescription.track || m_initialDescription.track == static_cast<unsigned int>(songs.size()))) {
                                            song.album = album;
                                            song.year = year;
                                            song.totalTracks = static_cast<unsigned int>(songs.size());
                                            m_results << move(song);
                                        }
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
    switch(m_reader.error()) {
    case QXmlStreamReader::NoError:
    case QXmlStreamReader::PrematureEndOfDocumentError:
        break;
    default:
        m_errorList << m_reader.errorString();
    }

    // promote changes
    endResetModel();
}

QueryResultsModel *queryLyricsWikia(SongDescription &&songDescription)
{
    static QString defaultUrl(QStringLiteral("https://lyrics.wikia.com/api.php"));

    // compose URL
    QUrl url(Settings::lyricsWikiaUrl().isEmpty() ? defaultUrl : Settings::lyricsWikiaUrl());
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
