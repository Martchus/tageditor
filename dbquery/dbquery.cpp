#include "./dbquery.h"

#include "../misc/utility.h"
#include "../misc/networkaccessmanager.h"

#include <tagparser/tagvalue.h>
#include <tagparser/tag.h>
#include <tagparser/signature.h>

#include <QNetworkReply>
#include <QMessageBox>

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
    if((m_resultsAvailable = resultsAvailable)) {
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
                TagValue tagValue(res.cover.data(), res.cover.size(), TagDataType::Picture);
                tagValue.setMimeType(containerMimeType(parseSignature(res.cover.data(), res.cover.size())));
                return tagValue;
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

/*!
 * \brief Constructs a new HttpResultsModel for the specified \a reply.
 * \remarks The model takes ownership over the specified \a reply.
 */
HttpResultsModel::HttpResultsModel(SongDescription &&initialSongDescription, QNetworkReply *reply) :
    m_initialDescription(initialSongDescription)
{
    addReply(reply);
}

/*!
 * \brief Deletes all associated replies.
 */
HttpResultsModel::~HttpResultsModel()
{
    qDeleteAll(m_replies);
}

/*!
 * \brief Helper to copy a property from one QObject to another.
 * \remarks Used to transfer reply properties to new reply in case a second reply is required.
 */
void copyProperty(const char *property, const QObject *from, QObject *to)
{
    to->setProperty(property, from->property(property));
}

/*!
 * \brief Evaluates request results.
 * \remarks Calls parseResults() if the requested data is available. Handles errors/redirections otherwise.
 */
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

/*!
 * \brief Adds a reply.
 * \remarks Called within c'tor and handleReplyFinished() in case of redirection. Might be called when subclassing to do further requests.
 */
void HttpResultsModel::addReply(QNetworkReply *reply)
{
    m_replies << reply;
#ifdef DEBUG_BUILD
    std::cerr << "HTTP query: " << reply->url().toString().toLocal8Bit().data() << std::endl;
#endif
    connect(reply, &QNetworkReply::finished, this, &HttpResultsModel::handleReplyFinished);
}

/*!
 * \brief Aborts all ongoing requests and causes error "Aborted by user" if requests where ongoing.
 */
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

}

#include "dbquery.moc"
