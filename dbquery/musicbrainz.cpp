#include "./musicbrainz.h"

#include "../application/settings.h"
#include "../misc/networkaccessmanager.h"
#include "../misc/utility.h"

#include <QNetworkReply>
#include <QNetworkRequest>
#include <QStringBuilder>
#include <QUrlQuery>
#include <QXmlStreamReader>

#include <algorithm>
#include <functional>
#include <map>
#include <unordered_map>
#include <vector>

using namespace std;
using namespace std::placeholders;
using namespace Utility;

#if (QT_VERSION < QT_VERSION_CHECK(5, 14, 0))
namespace std {
template <> struct hash<QString> {
    std::size_t operator()(const QString &s) const
    {
        return qHash(s);
    }
};
} // namespace std
#endif

namespace QtGui {

MusicBrainzResultsModel::MusicBrainzResultsModel(SongDescription &&initialSongDescription, QNetworkReply *reply)
    : HttpResultsModel(std::move(initialSongDescription), reply)
{
}

bool MusicBrainzResultsModel::fetchCover(const QModelIndex &index)
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

    // request the cover art
    auto *const reply = queryCoverArtArchive(desc.albumId);
    addReply(reply, bind(&MusicBrainzResultsModel::handleCoverReplyFinished, this, reply, desc.albumId, index.row()));
    setFetchingCover(true);
    return false;
}

QUrl MusicBrainzResultsModel::webUrl(const QModelIndex &index)
{
    if (index.parent().isValid() || !index.isValid() || index.row() >= m_results.size()) {
        return QUrl();
    }
    return QUrl(QStringLiteral("https://musicbrainz.org/recording/") + m_results.at(index.row()).songId);
}

void MusicBrainzResultsModel::parseInitialResults(const QByteArray &data)
{
    beginResetModel();
    m_results.clear();

    // store all song information (called recordings by MusicBrainz)
    auto recordings = std::vector<SongDescription>();
    // store all albums/collections (called releases by MusicBrainz) for a song
    auto releasesByRecording = std::unordered_map<QString, std::vector<SongDescription>>();

    // parse XML tree
    auto xmlReader = QXmlStreamReader(data);
    // clang-format off
    #include <qtutilities/misc/xmlparsermacros.h>
    children {
        iftag("metadata") {
            children {
                iftag("recording-list") {
                    children {
                        iftag("recording") {
                            auto currentDescription = SongDescription(attribute("id").toString());
                            children {
                                iftag("title") {
                                    currentDescription.title = text;
                                } eliftag("artist-credit") {
                                    children {
                                        iftag("name-credit") {
                                            children {
                                                iftag("artist") {
                                                    if (currentDescription.artistId.isEmpty()) {
                                                        currentDescription.artistId = attribute("id").toString();
                                                    }
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
                                            auto releaseInfo = SongDescription();
                                            releaseInfo.albumId = attribute("id").toString();
                                            children {
                                                iftag("title") {
                                                    releaseInfo.album = text;
                                                } eliftag("artist-credit") {
                                                    children {
                                                       iftag("name-credit") {
                                                           children {
                                                               iftag("artist") {
                                                                   if (currentDescription.artistId.isEmpty()) {
                                                                       currentDescription.artistId = attribute("id").toString();
                                                                   }
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
                                                } eliftag("date") {
                                                    releaseInfo.year = text;
                                                } eliftag("medium-list") {
                                                    children {
                                                        iftag("medium") {
                                                            children {
                                                                iftag("position") {
                                                                    releaseInfo.disk = text.toInt();
                                                                } eliftag("track-list") {
                                                                    releaseInfo.totalTracks = attribute("count").toInt();
                                                                    children {
                                                                        iftag("track") {
                                                                            children {
                                                                                iftag("number") {
                                                                                    releaseInfo.track = text.toInt();
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
                                            releasesByRecording[currentDescription.songId].emplace_back(std::move(releaseInfo));
                                        }
                                        else_skip
                                    }
                                } eliftag("tag-list") {
                                    children {
                                        iftag("tag") {
                                            children {
                                                iftag("name") {
                                                    if (!currentDescription.genre.isEmpty()) {
                                                        currentDescription.genre.append(QStringLiteral(", "));
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
                            recordings.emplace_back(std::move(currentDescription));
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

    // populate results
    // -> create a song for each recording/release combination and group those songs by their releases sorted ascendingly from oldest to latest
    auto recordingsByRelease = std::map<QString, std::vector<SongDescription>>();
    for (const auto &recording : recordings) {
        const auto &releases = releasesByRecording[recording.songId];
        for (const auto &release : releases) {
            // make a copy of the recording/song information and add release/album specific information to it
            SongDescription releaseSpecificRecording = recording;
            if (!release.album.isEmpty()) {
                releaseSpecificRecording.album = release.album;
                releaseSpecificRecording.albumId = release.albumId;
            }
            if (!release.artist.isEmpty()) {
                releaseSpecificRecording.artist = release.artist;
                releaseSpecificRecording.artistId = release.artistId;
            }
            if (release.track) {
                releaseSpecificRecording.track = release.track;
            }
            if (release.totalTracks) {
                releaseSpecificRecording.totalTracks = release.totalTracks;
            }
            if (release.disk) {
                releaseSpecificRecording.disk = release.disk;
            }
            if (!release.year.isEmpty()) {
                releaseSpecificRecording.year = release.year;
            }
            recordingsByRelease[release.year % QChar('-') % release.albumId].emplace_back(std::move(releaseSpecificRecording));
        }
    }
    // -> sort recordings within each release by track number and add recordings to results
    for (auto &releaseAndRecordings : recordingsByRelease) {
        auto &recordingsOfRelease = releaseAndRecordings.second;
        std::sort(recordingsOfRelease.begin(), recordingsOfRelease.end(), [](const auto &recording1, const auto &recording2) {
            if (recording1.disk != recording2.disk) {
                return recording1.disk < recording2.disk;
            }
            return recording1.track < recording2.track;
        });
        for (auto &recording : recordingsOfRelease) {
            m_results << std::move(recording);
        }
    }

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
// clang-format on

QueryResultsModel *queryMusicBrainz(SongDescription &&songDescription)
{
    static const auto defaultMusicBrainzUrl(QStringLiteral("https://musicbrainz.org/ws/2/recording/"));

    auto parts = QStringList();
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

    const auto &musicBrainzUrl = Settings::values().dbQuery.musicBrainzUrl;
    auto url = QUrl(musicBrainzUrl.isEmpty() ? defaultMusicBrainzUrl : (musicBrainzUrl + QStringLiteral("/recording/")));
    auto query = QUrlQuery();
    query.addQueryItem(QStringLiteral("query"), parts.join(QStringLiteral(" AND ")));
    url.setQuery(query);
    auto request = QNetworkRequest(url);
    request.setHeader(QNetworkRequest::UserAgentHeader, QStringLiteral("Mozilla/5.0 (X11; Linux x86_64; rv:54.0) Gecko/20100101 Firefox/54.0"));
    return new MusicBrainzResultsModel(std::move(songDescription), Utility::networkAccessManager().get(request));
}

QNetworkReply *queryCoverArtArchive(const QString &albumId)
{
    static const auto defaultArchiveUrl(QStringLiteral("https://coverartarchive.org"));
    const auto &coverArtArchiveUrl = Settings::values().dbQuery.coverArtArchiveUrl;
    return networkAccessManager().get(QNetworkRequest(QUrl(
        (coverArtArchiveUrl.isEmpty() ? defaultArchiveUrl : coverArtArchiveUrl) % QStringLiteral("/release/") % albumId % QStringLiteral("/front"))));
}

} // namespace QtGui
