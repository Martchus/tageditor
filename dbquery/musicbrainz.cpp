#include "./musicbrainz.h"

#include "../misc/utility.h"
#include "../misc/networkaccessmanager.h"
#include "../application/settings.h"

#include <QStringBuilder>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QUrlQuery>

using namespace std;
using namespace Utility;

namespace QtGui {

map<QString, QByteArray> MusicBrainzResultsModel::m_coverData = map<QString, QByteArray>();

MusicBrainzResultsModel::MusicBrainzResultsModel(SongDescription &&initialSongDescription, QNetworkReply *reply) :
    HttpResultsModel(move(initialSongDescription), reply)
{}

bool MusicBrainzResultsModel::fetchCover(const QModelIndex &index)
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
                // request the cover art
                QNetworkReply *reply = queryCoverArtArchive(desc.albumId);
                addReply(reply);
                reply->setProperty("coverArt", true);
                reply->setProperty("albumId", desc.albumId);
                reply->setProperty("row", index.row());
                setFetchingCover(true);
                return false;
            }
        } else {
            m_errorList << tr("Unable to fetch cover: Album ID is unknown.");
            emit resultsAvailable();
        }
    }
    return true;
}

void MusicBrainzResultsModel::parseResults(QNetworkReply *reply, const QByteArray &data)
{
    if(reply->property("coverArt").toBool()) {
        // add cover -> determine album ID and row
        bool ok;
        QString albumId = reply->property("albumId").toString();
        int row = reply->property("row").toInt(&ok);
        if(!albumId.isEmpty() && ok && row < m_results.size()) {
            m_coverData[albumId] = data;
            m_results[row].cover = data;
            emit coverAvailable(index(row, 0));
        } else {
            m_errorList << tr("Cover reply is invalid (internal error).");
        }
        setFetchingCover(false);
    } else {
        // prepare parsing MusicBrainz meta data
        beginResetModel();
        m_results.clear();
        m_reader.addData(data);

        // parse XML tree
#define xmlReader m_reader
#include <qtutilities/misc/xmlparsermacros.h>
        children {
            iftag("metadata") {
                children {
                    iftag("recording-list") {
                        children {
                            iftag("recording") {
                                SongDescription currentDescription;
                                children {
                                    iftag("title") {
                                        currentDescription.title = text;
                                    } eliftag("artist-credit") {
                                        children {
                                            iftag("name-credit") {
                                                children {
                                                    iftag("artist") {
                                                        children {
                                                            iftag("name") {
                                                                currentDescription.artist = text;
                                                            } else_skip
                                                        }
                                                    } else_skip
                                                }
                                            } else_skip
                                        }
                                    } eliftag("release-list") {
                                        children {
                                            iftag("release") {
                                                if(currentDescription.albumId.isEmpty()) {
                                                    currentDescription.albumId = attribute("id").toString();
                                                }
                                                children {
                                                    iftag("title") {
                                                        currentDescription.album = text;
                                                    } eliftag("date") {
                                                        currentDescription.year = text;
                                                    } eliftag("medium-list") {
                                                        children {
                                                            iftag("medium") {
                                                                children {
                                                                    iftag("position") {
                                                                        currentDescription.disk = text.toUInt();
                                                                    } eliftag("track-list") {
                                                                        currentDescription.totalTracks = attribute("count").toUInt();
                                                                        children {
                                                                            iftag("track") {
                                                                                children {
                                                                                    iftag("number") {
                                                                                        currentDescription.track = text.toUInt();
                                                                                    } else_skip
                                                                                }
                                                                            } else_skip
                                                                        }
                                                                    } else_skip
                                                                }
                                                            } else_skip
                                                        }
                                                    } else_skip
                                                }
                                            } else_skip
                                        }
                                    } eliftag("tag-list") {
                                        children {
                                            iftag("tag") {
                                                children {
                                                    iftag("name") {
                                                        if(!currentDescription.genre.isEmpty()) {
                                                            currentDescription.genre.append(QLatin1Char(' '));
                                                        }
                                                        currentDescription.genre.append(text);
                                                    } else_skip
                                                }
                                            } else_skip
                                        }
                                    } else_skip
                                }
                                m_results << currentDescription;
                            } else_skip
                        }
                    } else_skip
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
}

QueryResultsModel *queryMusicBrainz(SongDescription &&songDescription)
{
    static QString defaultMusicBrainzUrl(QStringLiteral("https://musicbrainz.org/ws/2/recording/"));

    // compose parts
    QStringList parts;
    parts.reserve(4);
    if(!songDescription.title.isEmpty()) {
        parts << QChar('\"') % songDescription.title % QChar('\"');
    }
    if(!songDescription.artist.isEmpty()) {
        parts << QStringLiteral("artist:\"") % songDescription.artist % QChar('\"');
    }
    if(!songDescription.album.isEmpty()) {
        parts << QStringLiteral("release:\"") % songDescription.album % QChar('\"');
    }
    if(songDescription.track) {
        parts << QStringLiteral("number:") + QString::number(songDescription.track);
    }

    // compose URL
    QUrl url(Settings::musicBrainzUrl().isEmpty() ? defaultMusicBrainzUrl : (Settings::musicBrainzUrl() + QStringLiteral("/recording/")));
    QUrlQuery query;
    query.addQueryItem(QStringLiteral("query"), parts.join(QStringLiteral(" AND ")));
    url.setQuery(query);

    // make request
    return new MusicBrainzResultsModel(move(songDescription), Utility::networkAccessManager().get(QNetworkRequest(url)));
}

QNetworkReply *queryCoverArtArchive(const QString &albumId)
{
    static QString defaultArchiveUrl(QStringLiteral("https://coverartarchive.org"));
    return networkAccessManager().get(QNetworkRequest(QUrl((Settings::coverArtArchiveUrl().isEmpty() ? defaultArchiveUrl : Settings::coverArtArchiveUrl()) % QStringLiteral("/release/") % albumId % QStringLiteral("/front"))));
}

} // namespace QtGui
