#ifndef DBQUERY_H
#define DBQUERY_H

#include <c++utilities/application/global.h>
#include <c++utilities/conversion/types.h>

#include <QAbstractTableModel>
#include <QNetworkReply>

#ifdef DEBUG_BUILD
#include <iostream>
#endif

QT_FORWARD_DECLARE_CLASS(QNetworkReply)

namespace TagParser {
class TagValue;
DECLARE_ENUM_CLASS(KnownField, unsigned int);
} // namespace TagParser

namespace QtGui {

struct SongDescription {
    SongDescription(const QString &songId = QString());

    QString songId;
    QString title;
    QString album;
    QString albumId;
    QString artist;
    QString year;
    QString genre;
    int32 track;
    int32 totalTracks;
    int32 disk;
    QByteArray cover;
    QString lyrics;
    QString coverUrl;
};

class QueryResultsModel : public QAbstractTableModel {
    Q_OBJECT

public:
    enum Column {
        TitleCol,
        AlbumCol,
        ArtistCol,
        GenreCol,
        YearCol,
        TrackCol,
        TotalTracksCol,
    };

    const QList<SongDescription> &results() const;
    const QStringList &errorList() const;
    bool areResultsAvailable() const;
    bool isFetchingCover() const;
    TagParser::TagValue fieldValue(int row, TagParser::KnownField knownField) const;

    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;
    Qt::ItemFlags flags(const QModelIndex &index) const;
    QVariant headerData(int section, Qt::Orientation orientation, int role) const;
    int rowCount(const QModelIndex &parent = QModelIndex()) const;
    int columnCount(const QModelIndex &parent = QModelIndex()) const;
    const QByteArray *cover(const QModelIndex &index) const;
    virtual bool fetchCover(const QModelIndex &index);
    const QString *lyrics(const QModelIndex &index) const;
    virtual bool fetchLyrics(const QModelIndex &index);
    virtual void abort();
    virtual QUrl webUrl(const QModelIndex &index);

signals:
    void resultsAvailable();
    void coverAvailable(const QModelIndex &index);
    void lyricsAvailable(const QModelIndex &index);

protected:
    QueryResultsModel(QObject *parent = nullptr);
    void setResultsAvailable(bool resultsAvailable);
    void setFetchingCover(bool fetchingCover);

    QList<SongDescription> m_results;
    QStringList m_errorList;
    bool m_resultsAvailable;
    bool m_fetchingCover;
    static std::map<QString, QByteArray> m_coverData;
};

inline const QList<SongDescription> &QueryResultsModel::results() const
{
    return m_results;
}

inline const QStringList &QueryResultsModel::errorList() const
{
    return m_errorList;
}

inline bool QueryResultsModel::areResultsAvailable() const
{
    return m_resultsAvailable;
}

inline bool QueryResultsModel::isFetchingCover() const
{
    return m_fetchingCover;
}

class HttpResultsModel : public QueryResultsModel {
    Q_OBJECT
public:
    ~HttpResultsModel() override;
    void abort() override;

protected:
    HttpResultsModel(SongDescription &&initialSongDescription, QNetworkReply *reply);
    template <class Object, class Function> void addReply(QNetworkReply *reply, Object object, Function handler);
    template <class Function> void addReply(QNetworkReply *reply, Function handler);
    virtual void parseInitialResults(const QByteArray &data) = 0;
    QNetworkReply *evaluateReplyResults(QNetworkReply *reply, QByteArray &data, bool alwaysFollowRedirection = false);

    void handleCoverReplyFinished(QNetworkReply *reply, const QString &albumId, int row);
    void parseCoverResults(const QString &albumId, int row, const QByteArray &data);

private slots:
    void handleInitialReplyFinished();

protected:
    QList<QNetworkReply *> m_replies;
    const SongDescription m_initialDescription;
};

template <class Object, class Function> inline void HttpResultsModel::addReply(QNetworkReply *reply, Object object, Function handler)
{
    (m_replies << reply), connect(reply, &QNetworkReply::finished, object, handler);
#ifdef DEBUG_BUILD
    std::cerr << "HTTP query: " << reply->url().toString().toUtf8().data() << std::endl;
#endif
}

/*!
 * \brief Adds a reply.
 * \remarks Called within c'tor and handleReplyFinished() in case of redirection. Might be called when subclassing to do further requests.
 */
template <class Function> inline void HttpResultsModel::addReply(QNetworkReply *reply, Function handler)
{
    (m_replies << reply), connect(reply, &QNetworkReply::finished, handler);
#ifdef DEBUG_BUILD
    std::cerr << "HTTP query: " << reply->url().toString().toUtf8().data() << std::endl;
#endif
}

QueryResultsModel *queryMusicBrainz(SongDescription &&songDescription);
QueryResultsModel *queryLyricsWikia(SongDescription &&songDescription);
QNetworkReply *queryCoverArtArchive(const QString &albumId);

} // namespace QtGui

#endif // DBQUERY_H
