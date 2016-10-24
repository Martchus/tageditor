#ifndef DBQUERY_H
#define DBQUERY_H

#include <c++utilities/application/global.h>

#include <QAbstractTableModel>

#ifdef DEBUG_BUILD
# include <QNetworkReply>
# include <iostream>
#endif

QT_FORWARD_DECLARE_CLASS(QNetworkReply)

namespace Media {
class TagValue;
DECLARE_ENUM_CLASS(KnownField, unsigned int);
}

namespace QtGui {

struct SongDescription
{
    SongDescription();

    QString title;
    QString album;
    QString albumId;
    QString artist;
    QString year;
    QString genre;
    unsigned int track;
    unsigned int totalTracks;
    unsigned int disk;
    QByteArray cover;
    QString lyrics;
};

class QueryResultsModel : public QAbstractTableModel
{
    Q_OBJECT

public:
    enum Column {
        TitleCol,
        AlbumCol,
        ArtistCol,
        GenreCol,
        YearCol,
        TrackCol,
        TotalTracksCol
    };

    const QList<SongDescription> &results() const;
    const QStringList &errorList() const;
    bool areResultsAvailable() const;
    bool isFetchingCover() const;
    Media::TagValue fieldValue(int row, Media::KnownField knownField) const;

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

class HttpResultsModel : public QueryResultsModel
{
    Q_OBJECT
public:
    ~HttpResultsModel();
    void abort();

protected:
    HttpResultsModel(SongDescription &&initialSongDescription, QNetworkReply *reply);
    template<class Object, class Function> void addReply(QNetworkReply *reply, Object object, Function handler);
    template<class Function> void addReply(QNetworkReply *reply, Function handler);
    virtual void parseInitialResults(const QByteArray &data) = 0;
    QNetworkReply *evaluateReplyResults(QNetworkReply *reply, QByteArray &data, bool alwaysFollowRedirection = false);

private slots:
    void handleInitialReplyFinished();

protected:
    QList<QNetworkReply *> m_replies;
    const SongDescription m_initialDescription;
};

template<class Object, class Function>
inline void HttpResultsModel::addReply(QNetworkReply *reply, Object object, Function handler)
{
    (m_replies << reply), connect(reply, &QNetworkReply::finished, object, handler);
#ifdef DEBUG_BUILD
    std::cerr << "HTTP query: " << reply->url().toString().toLocal8Bit().data() << std::endl;
#endif
}

/*!
 * \brief Adds a reply.
 * \remarks Called within c'tor and handleReplyFinished() in case of redirection. Might be called when subclassing to do further requests.
 */
template<class Function>
inline void HttpResultsModel::addReply(QNetworkReply *reply, Function handler)
{
    (m_replies << reply), connect(reply, &QNetworkReply::finished, handler);
#ifdef DEBUG_BUILD
    std::cerr << "HTTP query: " << reply->url().toString().toLocal8Bit().data() << std::endl;
#endif
}

QueryResultsModel *queryMusicBrainz(SongDescription &&songDescription);
QueryResultsModel *queryLyricsWikia(SongDescription &&songDescription);
QNetworkReply *queryCoverArtArchive(const QString &albumId);

}

#endif // DBQUERY_H
