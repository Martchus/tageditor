#include "./musicbrainz.h"

#include "../misc/utility.h"
#include "../misc/networkaccessmanager.h"
#include "../application/settings.h"

#include <QStringBuilder>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QUrlQuery>
#include <QXmlStreamReader>

#include <functional>

using namespace std;
using namespace std::placeholders;
using namespace Utility;

namespace QtGui {

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
                auto *reply = queryCoverArtArchive(desc.albumId);
                addReply(reply, bind(&MusicBrainzResultsModel::handleCoverReplyFinished, this, reply, desc.albumId, index.row()));
                setFetchingCover(true);
                return false;
            }
        } else {
            m_errorList << tr("Unable to fetch cover: Album ID unknown");
            emit resultsAvailable();
        }
    }
    return true;
}

void MusicBrainzResultsModel::parseInitialResults(const QByteArray &data)
{
    // prepare parsing MusicBrainz meta data
    beginResetModel();
    m_results.clear();
    QXmlStreamReader xmlReader(data);

    // parse XML tree
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

QueryResultsModel *queryMusicBrainz(SongDescription &&songDescription)
{
    static const QString defaultMusicBrainzUrl(QStringLiteral("https://musicbrainz.org/ws/2/recording/"));

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
    const QString &musicBrainzUrl = Settings::values().dbQuery.musicBrainzUrl;
    QUrl url(musicBrainzUrl.isEmpty() ? defaultMusicBrainzUrl : (musicBrainzUrl + QStringLiteral("/recording/")));
    QUrlQuery query;
    query.addQueryItem(QStringLiteral("query"), parts.join(QStringLiteral(" AND ")));
    url.setQuery(query);

    // make request
    return new MusicBrainzResultsModel(move(songDescription), Utility::networkAccessManager().get(QNetworkRequest(url)));
}

QNetworkReply *queryCoverArtArchive(const QString &albumId)
{
    static const QString defaultArchiveUrl(QStringLiteral("https://coverartarchive.org"));
    const QString &coverArtArchiveUrl = Settings::values().dbQuery.coverArtArchiveUrl;
    return networkAccessManager().get(QNetworkRequest(QUrl((coverArtArchiveUrl.isEmpty() ? defaultArchiveUrl : coverArtArchiveUrl) % QStringLiteral("/release/") % albumId % QStringLiteral("/front"))));
}

} // namespace QtGui
