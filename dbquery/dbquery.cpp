#include "./dbquery.h"

#include "../misc/networkaccessmanager.h"
#include "../misc/utility.h"

#include "resources/config.h"

#include <tagparser/signature.h>
#include <tagparser/tag.h>
#include <tagparser/tagvalue.h>

#include <QMessageBox>

#ifdef CPP_UTILITIES_DEBUG_BUILD
#include <iostream>
#endif

using namespace std;
using namespace Utility;
using namespace TagParser;

namespace QtGui {

SongDescription::SongDescription(const QString &songId)
    : songId(songId)
    , track(0)
    , totalTracks(0)
    , disk(0)
    , cover(nullptr)
{
}

std::list<QString> QueryResultsModel::s_coverNames = std::list<QString>();
std::map<QString, QByteArray> QueryResultsModel::s_coverData = std::map<QString, QByteArray>();

QueryResultsModel::QueryResultsModel(QObject *parent)
    : QAbstractTableModel(parent)
    , m_resultsAvailable(false)
    , m_fetchingCover(false)
{
}

void QueryResultsModel::setResultsAvailable(bool resultsAvailable)
{
    if ((m_resultsAvailable = resultsAvailable)) {
        emit this->resultsAvailable();
    }
}

void QueryResultsModel::setFetchingCover(bool fetchingCover)
{
    m_fetchingCover = fetchingCover;
}

void QueryResultsModel::abort()
{
}

QUrl QueryResultsModel::webUrl(const QModelIndex &index)
{
    Q_UNUSED(index)
    return QUrl();
}

#define returnValue(field) return qstringToTagValue(res.field, TagTextEncoding::Utf16LittleEndian)

TagValue QueryResultsModel::fieldValue(int row, KnownField knownField) const
{
    if (row >= m_results.size()) {
        return TagValue();
    }
    const SongDescription &res = m_results.at(row);
    switch (knownField) {
    case KnownField::Title:
        returnValue(title);
    case KnownField::Album:
        returnValue(album);
    case KnownField::Artist:
        returnValue(artist);
    case KnownField::Genre:
        returnValue(genre);
    case KnownField::RecordDate:
        returnValue(year);
    case KnownField::TrackPosition:
        return TagValue(PositionInSet(res.track, res.totalTracks));
    case KnownField::PartNumber:
        return TagValue(res.track);
    case KnownField::TotalParts:
        return TagValue(res.totalTracks);
    case KnownField::DiskPosition:
        return TagValue(PositionInSet(res.disk));
    case KnownField::Cover:
        if (!res.cover.isEmpty()) {
            TagValue tagValue(res.cover.data(), static_cast<size_t>(res.cover.size()), TagDataType::Picture);
            tagValue.setMimeType(containerMimeType(parseSignature(res.cover.data(), static_cast<std::size_t>(res.cover.size()))));
            return tagValue;
        }
        break;
    case KnownField::Lyrics:
        returnValue(lyrics);
    default:;
    }
    return TagValue();
}

#undef returnValue

QVariant QueryResultsModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() >= m_results.size()) {
        return QVariant();
    }
    const auto &res = m_results.at(index.row());
    switch (role) {
    case Qt::DisplayRole:
        switch (index.column()) {
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
            if (res.track) {
                return res.track;
            } else {
                return QVariant();
            }
        case TotalTracksCol:
            if (res.totalTracks) {
                return res.totalTracks;
            } else {
                return QVariant();
            }
        case DiskCol:
            if (res.disk) {
                return res.disk;
            } else {
                return QVariant();
            }
        default:;
        }
        break;
    default:;
    }
    return QVariant();
}

Qt::ItemFlags QueryResultsModel::flags(const QModelIndex &index) const
{
    auto flags = Qt::ItemFlags(Qt::ItemNeverHasChildren | Qt::ItemIsSelectable | Qt::ItemIsEnabled);
    if (index.isValid()) {
        flags |= Qt::ItemIsUserCheckable;
    }
    return flags;
}

QVariant QueryResultsModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    switch (orientation) {
    case Qt::Horizontal:
        switch (role) {
        case Qt::DisplayRole:
            switch (section) {
            case TitleCol:
                return tr("Song title");
            case AlbumCol:
                return tr("Album");
            case ArtistCol:
                return tr("Artist");
            case GenreCol:
                return tr("Genre");
            case YearCol:
                return tr("Year");
            case TrackCol:
                return tr("Track");
            case TotalTracksCol:
                return tr("Total tracks");
            case DiskCol:
                return tr("Disk");
            default:;
            }
            break;
        default:;
        }
        break;
    default:;
    }
    return QVariant();
}

int QueryResultsModel::rowCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : Utility::containerSizeToInt(m_results.size());
}

int QueryResultsModel::columnCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : EndCol;
}

const QByteArray *QueryResultsModel::cover(const QModelIndex &index) const
{
    if (!index.isValid() || index.row() >= m_results.size()) {
        return nullptr;
    }
    const auto &cover = m_results.at(index.row()).cover;
    if (!cover.isEmpty()) {
        return &cover;
    }
    return nullptr;
}

QByteArray QueryResultsModel::coverValue(const QModelIndex &index) const
{
    if (const auto *c = cover(index)) {
        return *c;
    }
    return QByteArray();
}

/*!
 * \brief Fetches the cover the specified \a index.
 * \returns
 *  - true if the cover is immediately available or an error occurs immediately
 *  - and false if the cover will be fetched asynchronously.
 *
 * If the cover is fetched asynchronously the coverAvailable() signal is emitted, when the cover
 * becomes available.
 *
 * The resultsAvailable() signal is emitted if errors have been added to errorList().
 */
bool QueryResultsModel::fetchCover(const QModelIndex &index)
{
    Q_UNUSED(index)
    m_errorList << tr("Fetching cover is not implemented for this provider");
    emit resultsAvailable();
    return true;
}

const QString *QueryResultsModel::lyrics(const QModelIndex &index) const
{
    if (!index.isValid() || index.row() >= m_results.size()) {
        return nullptr;
    }
    const auto &lyrics = m_results.at(index.row()).lyrics;
    if (!lyrics.isEmpty()) {
        return &lyrics;
    }
    return nullptr;
}

QString QueryResultsModel::lyricsValue(const QModelIndex &index) const
{
    if (const auto *l = lyrics(index)) {
        return *l;
    }
    return QString();
}

/*!
 * \brief Fetches the lyrics the specified \a index.
 * \returns
 *  - true if the lyrics are immediately available or an error occurs immediately
 *  - and false if the lyrics will be fetched asynchronously.
 *
 * If the lyrics are fetched asynchronously the lyricsAvailable() signal is emitted, when the lyrics
 * become available.
 *
 * The resultsAvailable() signal is emitted if errors have been added to errorList().
 */
bool QueryResultsModel::fetchLyrics(const QModelIndex &index)
{
    Q_UNUSED(index)
    m_errorList << tr("Fetching lyrics is not implemented for this provider");
    emit resultsAvailable();
    return true;
}

/*!
 * \brief Constructs a new HttpResultsModel for the specified \a reply.
 * \remarks The model takes ownership over the specified \a reply.
 */
HttpResultsModel::HttpResultsModel(SongDescription &&initialSongDescription, QNetworkReply *reply)
    : m_initialDescription(initialSongDescription)
{
    addReply(reply, this, &HttpResultsModel::handleInitialReplyFinished);
}

/*!
 * \brief Deletes all associated replies.
 */
HttpResultsModel::~HttpResultsModel()
{
    qDeleteAll(m_replies);
}

/*!
 * \brief Evaluates request results.
 * \remarks Calls parseResults() if the requested data is available. Handles errors/redirections otherwise.
 */
void HttpResultsModel::handleInitialReplyFinished()
{
    auto *const reply = static_cast<QNetworkReply *>(sender());
    QByteArray data;
    if (auto *const newReply = evaluateReplyResults(reply, data, false)) {
        addReply(newReply, this, &HttpResultsModel::handleInitialReplyFinished);
        return;
    }
    if (!data.isEmpty()) {
        parseInitialResults(data);
    }
    setResultsAvailable(true); // update status, emit resultsAvailable()
}

#ifdef CPP_UTILITIES_DEBUG_BUILD
void HttpResultsModel::logReply(QNetworkReply *reply)
{
    static const auto enableQueryLogging = qEnvironmentVariableIntValue(PROJECT_VARNAME_UPPER "_ENABLE_QUERY_LOGGING");
    if (enableQueryLogging) {
        std::cerr << "HTTP query: " << reply->url().toString().toUtf8().data() << std::endl;
    }
}
#endif

QNetworkReply *HttpResultsModel::evaluateReplyResults(QNetworkReply *reply, QByteArray &data, bool alwaysFollowRedirection)
{
    // delete reply (later)
    reply->deleteLater();
    m_replies.removeAll(reply);

    if (reply->error() != QNetworkReply::NoError) {
        m_errorList << reply->errorString();
        return nullptr;
    }
    const auto redirectionTarget = reply->attribute(QNetworkRequest::RedirectionTargetAttribute);
    if (redirectionTarget.isNull()) {
        // read all data if it is not redirection
        if ((data = reply->readAll()).isEmpty()) {
            m_errorList << tr("Server replied no data.");
        }
#ifdef CPP_UTILITIES_DEBUG_BUILD
        static const auto enableQueryLogging = qEnvironmentVariableIntValue(PROJECT_VARNAME_UPPER "_ENABLE_QUERY_LOGGING");
        if (enableQueryLogging) {
            std::cerr << "Results from HTTP query:\n" << data.data() << '\n';
        }
#endif
        return nullptr;
    }

    // there's a redirection available
    // -> resolve new URL
    const auto newUrl = reply->url().resolved(redirectionTarget.toUrl());
    // -> ask user whether to follow redirection unless alwaysFollowRedirection is true
    if (!alwaysFollowRedirection) {
        const auto message = tr("<p>Do you want to redirect form <i>%1</i> to <i>%2</i>?</p>").arg(reply->url().toString(), newUrl.toString());
        alwaysFollowRedirection = QMessageBox::question(nullptr, tr("Search"), message, QMessageBox::Yes | QMessageBox::No) == QMessageBox::Yes;
    }
    if (alwaysFollowRedirection) {
        return networkAccessManager().get(QNetworkRequest(newUrl));
    }
    m_errorList << tr("Redirection to: ") + newUrl.toString();
    return nullptr;
}

/*!
 * \brief Aborts all ongoing requests and causes error "Aborted by user" if requests where ongoing.
 */
void HttpResultsModel::abort()
{
    if (m_replies.isEmpty()) {
        return;
    }
    qDeleteAll(m_replies);
    m_replies.clear();
    // must update status manually because handleReplyFinished() won't be called anymore
    m_errorList << tr("Aborted by user.");
    setResultsAvailable(true);
}

void HttpResultsModel::handleCoverReplyFinished(QNetworkReply *reply, const QString &albumId, int row)
{
    auto data = QByteArray();
    if (auto *const newReply = evaluateReplyResults(reply, data, true)) {
        addReply(newReply, bind(&HttpResultsModel::handleCoverReplyFinished, this, newReply, albumId, row));
        return;
    }
    if (!data.isEmpty()) {
        parseCoverResults(albumId, row, data);
    }
    if (!m_resultsAvailable) {
        setResultsAvailable(true);
    }
}

void HttpResultsModel::parseCoverResults(const QString &albumId, int row, const QByteArray &data)
{
    setFetchingCover(false);

    // add cover -> determine album ID and row
    if (albumId.isEmpty() || row >= m_results.size()) {
        m_errorList << tr("Internal error: context for cover reply invalid");
        setResultsAvailable(true);
        return;
    }

    if (data.isEmpty()) {
        return;
    }

    // cache the fetched cover
    const auto currentCachedCoverCount = s_coverData.size();
    s_coverData[albumId] = data;
    if (s_coverData.size() > currentCachedCoverCount) {
        s_coverNames.emplace_back(albumId);

        // keep only the last 20 cover images around
        while (s_coverNames.size() > 20) {
            s_coverData.erase(s_coverNames.front());
            s_coverNames.pop_front();
        }
    } else if (s_coverNames.back() != albumId) {
        s_coverNames.remove(albumId);
        s_coverNames.emplace_back(albumId);
    }

    // add the cover to the results
    m_results[row].cover = data;
    setResultsAvailable(true);
    emit coverAvailable(index(row, 0));
}

} // namespace QtGui

#include "dbquery.moc"
