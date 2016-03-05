#include "./dbquery.h"

#include "../misc/utility.h"
#include "../misc/networkaccessmanager.h"
#include "../application/settings.h"

#include <tagparser/tagvalue.h>
#include <tagparser/tag.h>

#include <QNetworkReply>
#include <QNetworkRequest>
#include <QUrlQuery>
#include <QStringBuilder>

#define xmlReader m_reader
#include <qtutilities/misc/xmlparsermacros.h>

using namespace Utility;
using namespace Media;

namespace QtGui {

SongDescription::SongDescription() :
    track(0),
    totalTracks(0),
    disk(0)
{}

QueryResultsModel::QueryResultsModel(QObject *parent) :
    QAbstractTableModel(parent),
    m_resultsAvailable(false)
{}

void QueryResultsModel::setResultsAvailable(bool resultsAvailable)
{
    if(resultsAvailable != m_resultsAvailable) {
        if(m_resultsAvailable = resultsAvailable) {
            emit this->resultsAvailable();
        }
    }
}

#define returnValue(field) return qstringToTagValue(res.field, TagTextEncoding::Utf16LittleEndian)

TagValue QueryResultsModel::fieldValue(int row, KnownField knownField) const
{
    if(row < m_results.size()) {
        const SongDescription &res = m_results.at(row);
        switch(knownField) {
        case KnownField::Title:
            returnValue(title);
        case KnownField::Album:
            returnValue(album);
        case KnownField::Artist:
            returnValue(artist);
        case KnownField::Genre:
            returnValue(genre);
        case KnownField::Year:
            returnValue(year);
        case KnownField::TrackPosition:
            return TagValue(PositionInSet(res.track, res.totalTracks));
        case KnownField::PartNumber:
            return TagValue(res.track);
        case KnownField::TotalParts:
            return TagValue(res.totalTracks);
        default:
            ;
        }
    }
    return TagValue();
}

#undef returnValue

QVariant QueryResultsModel::data(const QModelIndex &index, int role) const
{
    if(index.isValid() && index.row() < m_results.size()) {
        const SongDescription &res = m_results.at(index.row());
        switch(role) {
        case Qt::DisplayRole:
            switch(index.column()) {
            case TitleCol:
                return res.title;
            case AlbumCol:
                return res.album;
            case ArtistCol:
                return res.artist;
            case GenreCol:
                return res.genre;
            case YearCol:
                return res.year;
            case TrackCol:
                if(res.track) {
                    return res.track;
                } else {
                    return QString();
                }
            case TotalTracksCol:
                if(res.totalTracks) {
                    return res.totalTracks;
                } else {
                    return QString();
                }
            default:
                ;
            }
            break;
        default:
            ;
        }

    }
    return QVariant();
}

Qt::ItemFlags QueryResultsModel::flags(const QModelIndex &index) const
{
    Qt::ItemFlags flags = Qt::ItemNeverHasChildren | Qt::ItemIsSelectable | Qt::ItemIsEnabled;
    if(index.isValid()) {
        flags |= Qt::ItemIsUserCheckable;
    }
    return flags;
}

QVariant QueryResultsModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    switch(orientation) {
    case Qt::Horizontal:
        switch(role) {
        case Qt::DisplayRole:
            switch(section) {
            case TitleCol:
                return tr("Song title");
            case AlbumCol:
                return tr("Album");
            case ArtistCol:
                return tr("Artist");
            case YearCol:
                return tr("Year");
            case TrackCol:
                return tr("Track");
            case TotalTracksCol:
                return tr("Total tracks");
            case GenreCol:
                return tr("Genre");
            default:
                ;
            }
            break;
        default:
            ;
        }
        break;
    default:
        ;
    }
    return QVariant();
}

int QueryResultsModel::rowCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : m_results.size();
}

int QueryResultsModel::columnCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : (TotalTracksCol + 1);
}

class MusicBrainzResultsModel : public QueryResultsModel
{
    Q_OBJECT
public:
    MusicBrainzResultsModel(QNetworkReply *reply);
    ~MusicBrainzResultsModel();

private slots:
    void parseResults();

private:
    QNetworkReply *m_reply;
    QXmlStreamReader m_reader;
};

MusicBrainzResultsModel::MusicBrainzResultsModel(QNetworkReply *reply) :
    m_reply(reply)
{
    connect(reply, &QNetworkReply::finished, this, &MusicBrainzResultsModel::parseResults);
    m_reader.setDevice(m_reply);
}

MusicBrainzResultsModel::~MusicBrainzResultsModel()
{
    if(m_reply) {
        m_reply->abort();
        delete m_reply;
    }
}

void MusicBrainzResultsModel::parseResults()
{
    beginResetModel();

    // check for network error
    switch(m_reply->error()) {
    case QNetworkReply::NoError:
        // parse XML tree
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

        // check for parsing errors
        switch(m_reader.error()) {
        case QXmlStreamReader::NoError:
        case QXmlStreamReader::PrematureEndOfDocumentError:
            break;
        default:
            m_errorList << m_reader.errorString();
        }
        break;

    default:
        m_errorList << m_reply->errorString();
    }

    // delete reply
    m_reply->deleteLater();
    m_reader.setDevice(m_reply = nullptr);

    // promote changes
    endResetModel();
    setResultsAvailable(true);
}

QueryResultsModel *queryMusicBrainz(const SongDescription &songDescription)
{
    static QString defaultMusicBrainzUrl(QStringLiteral("https://musicbrainz.org/ws/2/recording"));
    // TODO: make this configurable

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
    QUrl url(Settings::musicBrainzUrl().isEmpty() ? defaultMusicBrainzUrl : (Settings::musicBrainzUrl() + QStringLiteral("/recording")));
    QUrlQuery query;
    query.addQueryItem(QStringLiteral("query"), parts.join(QStringLiteral(" AND ")));
    url.setQuery(query);

    // make request
    return new MusicBrainzResultsModel(Utility::networkAccessManager().get(QNetworkRequest(url)));
}

}

#include "dbquery.moc"
