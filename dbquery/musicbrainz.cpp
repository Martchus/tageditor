#include "./musicbrainz.h"

#include "../application/settings.h"
#include "../misc/networkaccessmanager.h"
#include "../misc/utility.h"

#include <QNetworkReply>
#include <QNetworkRequest>
#include <QStringBuilder>
#include <QUrlQuery>
#include <QXmlStreamReader>

#include <functional>

using namespace std;
using namespace std::placeholders;
using namespace Utility;

namespace QtGui {

MusicBrainzResultsModel::MusicBrainzResultsModel(SongDescription &&initialSongDescription, QNetworkReply *reply)
    : HttpResultsModel(move(initialSongDescription), reply)
{
}

bool MusicBrainzResultsModel::fetchCover(const QModelIndex &index)
{
    // FIXME: avoid code duplication with lyricswikia.cpp

    // find song description
    if (index.parent().isValid() || index.row() >= m_results.size()) {
        return true;
    }
    SongDescription &desc = m_results[index.row()];

    // skip if cover is already available
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
    const auto coverData = m_coverData.find(desc.albumId);
    if (coverData != m_coverData.end()) {
        desc.cover = coverData->second;
        return true;
    }

    // request the cover art
    auto *const reply = queryCoverArtArchive(desc.albumId);
    addReply(reply, bind(&MusicBrainzResultsModel::handleCoverReplyFinished, this, reply, desc.albumId, index.row()));
    setFetchingCover(true);
    return false;
}

QUrl MusicBrainzResultsModel::webUrl(const QModelIndex &index)
{
    if (index.parent().isValid() || index.row() >= results().size()) {
        return QUrl();
    }
    return QUrl(QStringLiteral("https://musicbrainz.org/recording/") + results().at(index.row()).songId);
}

void MusicBrainzResultsModel::parseInitialResults(const QByteArray &data)
{
    // prepare parsing MusicBrainz meta data
    beginResetModel();
    m_results.clear();
    QXmlStreamReader xmlReader(data);

    // parse XML tree
    // clang-format off
#include <qtutilities/misc/xmlparsermacros.h>
    children {
        iftag("metadata") {
            children {
                iftag("recording-list") {
                    children {
                        iftag("recording") {
                            SongDescription currentDescription(attribute("id").toString());
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
                                                        }
                                                        else_skip
                                                    }
                                                }
                                                else_skip
                                            }
                                        }
                                        else_skip
                                    }
                                } eliftag("release-list") {
                                    children {
                                        iftag("release") {
                                            if (currentDescription.albumId.isEmpty()) {
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
                                                                    currentDescription.disk = text.toInt();
                                                                } eliftag("track-list") {
                                                                    currentDescription.totalTracks = attribute("count").toInt();
                                                                    children {
                                                                        iftag("track") {
                                                                            children {
                                                                                iftag("number") {
                                                                                    currentDescription.track = text.toInt();
                                                                                } else_skip
                                                                            }
                                                                        }
                                                                        else_skip
                                                                    }
                                                                }
                                                                else_skip
                                                            }
                                                        }
                                                        else_skip
                                                    }
                                                }
                                                else_skip
                                            }
                                        }
                                        else_skip
                                    }
                                } eliftag("tag-list") {
                                    children {
                                        iftag("tag") {
                                            children {
                                                iftag("name") {
                                                    if (!currentDescription.genre.isEmpty()) {
                                                        currentDescription.genre.append(QLatin1Char(' '));
                                                    }
                                                    currentDescription.genre.append(text);
                                                }
                                                else_skip
                                            }
                                        }
                                        else_skip
                                    }
                                }
                                else_skip
                            }
                            m_results << currentDescription;
                        }
                        else_skip
                    }
                }
                else_skip
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

// promote changes
endResetModel();
}

QueryResultsModel *queryMusicBrainz(SongDescription &&songDescription)
{
    static const auto defaultMusicBrainzUrl(QStringLiteral("https://musicbrainz.org/ws/2/recording/"));

    // compose parts
    QStringList parts;
    parts.reserve(4);
    if (!songDescription.title.isEmpty()) {
        parts << QChar('\"') % songDescription.title % QChar('\"');
    }
    if (!songDescription.artist.isEmpty()) {
        parts << QStringLiteral("artist:\"") % songDescription.artist % QChar('\"');
    }
    if (!songDescription.album.isEmpty()) {
        parts << QStringLiteral("release:\"") % songDescription.album % QChar('\"');
    }
    if (songDescription.track) {
        parts << QStringLiteral("number:") + QString::number(songDescription.track);
    }

    // compose URL
    const auto &musicBrainzUrl = Settings::values().dbQuery.musicBrainzUrl;
    QUrl url(musicBrainzUrl.isEmpty() ? defaultMusicBrainzUrl : (musicBrainzUrl + QStringLiteral("/recording/")));
    QUrlQuery query;
    query.addQueryItem(QStringLiteral("query"), parts.join(QStringLiteral(" AND ")));
    url.setQuery(query);

    // make request
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::UserAgentHeader, QStringLiteral("Mozilla/5.0 (X11; Linux x86_64; rv:54.0) Gecko/20100101 Firefox/54.0"));
    return new MusicBrainzResultsModel(move(songDescription), Utility::networkAccessManager().get(request));
}

QNetworkReply *queryCoverArtArchive(const QString &albumId)
{
    static const auto defaultArchiveUrl(QStringLiteral("https://coverartarchive.org"));
    const auto &coverArtArchiveUrl = Settings::values().dbQuery.coverArtArchiveUrl;
    return networkAccessManager().get(QNetworkRequest(QUrl(
        (coverArtArchiveUrl.isEmpty() ? defaultArchiveUrl : coverArtArchiveUrl) % QStringLiteral("/release/") % albumId % QStringLiteral("/front"))));
}

} // namespace QtGui
