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
#include <QXmlStreamReader>
#include <QMessageBox>

#include <map>
#ifdef DEBUG_BUILD
# include <iostream>
#endif

using namespace std;
using namespace Utility;
using namespace Media;

namespace QtGui {

SongDescription::SongDescription() :
    track(0),
    totalTracks(0),
    disk(0),
    cover(nullptr)
{}

QueryResultsModel::QueryResultsModel(QObject *parent) :
    QAbstractTableModel(parent),
    m_resultsAvailable(false),
    m_fetchingCover(false)
{}

void QueryResultsModel::setResultsAvailable(bool resultsAvailable)
{
    if(m_resultsAvailable = resultsAvailable) {
        emit this->resultsAvailable();
    }
}

void QueryResultsModel::setFetchingCover(bool fetchingCover)
{
    m_fetchingCover = fetchingCover;
}

#define returnValue(field) return qstringToTagValue(res.field, TagTextEncoding::Utf16LittleEndian)

void QueryResultsModel::abort()
{}

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
        case KnownField::Cover:
            if(!res.cover.isEmpty()) {
                return TagValue(res.cover.data(), res.cover.size(), TagDataType::Picture);
            }
            break;
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

const QByteArray *QueryResultsModel::cover(const QModelIndex &index) const
{
    if(!index.parent().isValid() && index.row() < m_results.size()) {
        const QByteArray &cover = m_results.at(index.row()).cover;
        if(!cover.isEmpty()) {
            return &cover;
        }
    }
    return nullptr;
}

/*!
 * \brief Fetches the cover the specified \a index.
 * \returns True if the cover is immidiately available; false is the cover is fetched asynchronously.
 *
 * If the cover is fetched asynchronously the coverAvailable() signal is emitted, when the cover
 * is available.
 *
 * The resultsAvailable() signal is emitted if errors have been added to errorList().
 */
bool QueryResultsModel::fetchCover(const QModelIndex &)
{
    m_errorList << tr("Fetching the cover is not implemented for the selected provider.");
    emit resultsAvailable();
    return false;
}

class HttpResultsModel : public QueryResultsModel
{
    Q_OBJECT
public:
    ~HttpResultsModel();
    void addReply(QNetworkReply *reply);
    void abort();

protected:
    HttpResultsModel(QNetworkReply *reply);
    virtual void parseResults(QNetworkReply *reply, const QByteArray &data) = 0;

private slots:
    void handleReplyFinished();

protected:
    QList<QNetworkReply *> m_replies;
};

HttpResultsModel::HttpResultsModel(QNetworkReply *reply)
{
    addReply(reply);
}

HttpResultsModel::~HttpResultsModel()
{
    qDeleteAll(m_replies);
}

void copyProperty(const char *property, const QObject *from, QObject *to)
{
    to->setProperty(property, from->property(property));
}

void HttpResultsModel::handleReplyFinished()
{
    auto *reply = static_cast<QNetworkReply *>(sender());
    if(reply->error() == QNetworkReply::NoError) {
        QVariant redirectionTarget = reply->attribute(QNetworkRequest::RedirectionTargetAttribute);
        if(!redirectionTarget.isNull()) {
            // there's a redirection available
            // -> resolve new URL
            const QUrl newUrl = reply->url().resolved(redirectionTarget.toUrl());
            // -> always follow when retrieving cover art, otherwise ask user
            bool follow = reply->property("coverArt").toBool();
            if(!follow) {
                const QString message = tr("<p>Do you want to redirect form <i>%1</i> to <i>%2</i>?</p>").arg(
                            reply->url().toString(), newUrl.toString());
                follow = QMessageBox::question(nullptr, tr("Search"), message, QMessageBox::Yes | QMessageBox::No) == QMessageBox::Yes;
            }
            if(follow) {
                QNetworkReply *newReply = networkAccessManager().get(QNetworkRequest(newUrl));
                // retain possible assigned dynamic properties (TODO: implement it in a better way)
                copyProperty("coverArt", reply, newReply);
                copyProperty("albumId", reply, newReply);
                copyProperty("row", reply, newReply);
                addReply(newReply);
                return;
            } else {
                m_errorList << tr("Redirection to: ") + newUrl.toString();
            }
        } else {
            QByteArray data = reply->readAll();
#ifdef DEBUG_BUILD
            std::cerr << "Results from HTTP query:" << std::endl;
            std::cerr << data.data() << std::endl;
#endif
            parseResults(reply, data);
        }
    } else {
        m_errorList << reply->errorString();
    }

    // delete reply
    reply->deleteLater();
    m_replies.removeAll(reply);

    // update status, emit resultsAvailable()
    setResultsAvailable(true);
}

void HttpResultsModel::addReply(QNetworkReply *reply)
{
    m_replies << reply;
#ifdef DEBUG_BUILD
    std::cerr << "HTTP query: " << reply->url().toString().toLocal8Bit().data() << std::endl;
#endif
    connect(reply, &QNetworkReply::finished, this, &HttpResultsModel::handleReplyFinished);
}

void HttpResultsModel::abort()
{
    if(!m_replies.isEmpty()) {
        qDeleteAll(m_replies);
        m_replies.clear();
        // must update status manually because handleReplyFinished() won't be called anymore
        m_errorList << tr("Aborted by user.");
        setResultsAvailable(true);
    }
}


class MusicBrainzResultsModel : public HttpResultsModel
{
    Q_OBJECT
private:
    enum What {
        MusicBrainzMetaData,
        CoverArt
    };

public:
    MusicBrainzResultsModel(QNetworkReply *reply);
    bool fetchCover(const QModelIndex &index);
    static QNetworkRequest coverRequest(const QString &albumId);

protected:
    void parseResults(QNetworkReply *reply, const QByteArray &data);

private:
    static map<QString, QByteArray> m_coverData;
    QXmlStreamReader m_reader;
    What m_what;
};

map<QString, QByteArray> MusicBrainzResultsModel::m_coverData = map<QString, QByteArray>();

MusicBrainzResultsModel::MusicBrainzResultsModel(QNetworkReply *reply) :
    HttpResultsModel(reply)
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

#define xmlReader m_reader
#include <qtutilities/misc/xmlparsermacros.h>

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

QueryResultsModel *queryMusicBrainz(const SongDescription &songDescription)
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
    return new MusicBrainzResultsModel(Utility::networkAccessManager().get(QNetworkRequest(url)));
}

QNetworkReply *queryCoverArtArchive(const QString &albumId)
{
    static QString defaultArchiveUrl(QStringLiteral("https://coverartarchive.org"));
    return networkAccessManager().get(QNetworkRequest(QUrl((Settings::coverArtArchiveUrl().isEmpty() ? defaultArchiveUrl : Settings::coverArtArchiveUrl()) % QStringLiteral("/release/") % albumId % QStringLiteral("/front"))));
}

}

#include "dbquery.moc"
