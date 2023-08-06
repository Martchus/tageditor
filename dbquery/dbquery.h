#ifndef DBQUERY_H
#define DBQUERY_H

#include <c++utilities/application/global.h>

#include <QAbstractTableModel>
#include <QNetworkReply>

#ifdef CPP_UTILITIES_DEBUG_BUILD
#include <iostream>
#endif

QT_FORWARD_DECLARE_CLASS(QNetworkReply)

#define TAGEDITOR_ENUM_CLASS enum class
namespace TagParser {
class TagValue;
TAGEDITOR_ENUM_CLASS KnownField : unsigned int;
} // namespace TagParser
#undef TAGEDITOR_ENUM_CLASS

namespace QtGui {

struct SongDescription {
    explicit SongDescription(const QString &songId = QString());

    QString songId;
    QString title;
    QString album;
    QString albumId;
    QString artist;
    QString artistId;
    QString year;
    QString genre;
    std::int32_t track;
    std::int32_t totalTracks;
    std::int32_t disk;
    QByteArray cover;
    QString lyrics;
    QString coverUrl;
};

class QueryResultsModel : public QAbstractTableModel {
    Q_OBJECT
    Q_PROPERTY(QStringList errorList READ errorList)
    Q_PROPERTY(bool areResultsAvailable READ areResultsAvailable)
    Q_PROPERTY(bool isFetchingCover READ isFetchingCover)

public:
    enum Column {
        TitleCol,
        AlbumCol,
        ArtistCol,
        GenreCol,
        YearCol,
        TrackCol,
        TotalTracksCol,
        DiskCol,
        EndCol,
    };

    const QList<SongDescription> &results() const;
    const QStringList &errorList() const;
    bool areResultsAvailable() const;
    bool isFetchingCover() const;
    TagParser::TagValue fieldValue(int row, TagParser::KnownField knownField) const;

    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    Qt::ItemFlags flags(const QModelIndex &index) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role) const override;
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;
    const QByteArray *cover(const QModelIndex &index) const;
    Q_INVOKABLE QByteArray coverValue(const QModelIndex &index) const;
    Q_INVOKABLE virtual bool fetchCover(const QModelIndex &index);
    const QString *lyrics(const QModelIndex &index) const;
    Q_INVOKABLE QString lyricsValue(const QModelIndex &index) const;
    Q_INVOKABLE virtual bool fetchLyrics(const QModelIndex &index);
    Q_INVOKABLE virtual void abort();
    Q_INVOKABLE virtual QUrl webUrl(const QModelIndex &index);

Q_SIGNALS:
    void resultsAvailable();
    void coverAvailable(const QModelIndex &index);
    void lyricsAvailable(const QModelIndex &index);

protected:
    explicit QueryResultsModel(QObject *parent = nullptr);
    void setResultsAvailable(bool resultsAvailable);
    void setFetchingCover(bool fetchingCover);

    QList<SongDescription> m_results;
    QStringList m_errorList;
    bool m_resultsAvailable;
    bool m_fetchingCover;
    static std::list<QString> s_coverNames;
    static std::map<QString, QByteArray> s_coverData;
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
    explicit HttpResultsModel(SongDescription &&initialSongDescription, QNetworkReply *reply);
    template <class Object, class Function> void addReply(QNetworkReply *reply, Object object, Function handler);
    template <class Function> void addReply(QNetworkReply *reply, Function handler);
    virtual void parseInitialResults(const QByteArray &data) = 0;
    QNetworkReply *evaluateReplyResults(QNetworkReply *reply, QByteArray &data, bool alwaysFollowRedirection = false);

    void handleCoverReplyFinished(QNetworkReply *reply, const QString &albumId, int row);
    void parseCoverResults(const QString &albumId, int row, const QByteArray &data);

private Q_SLOTS:
    void handleInitialReplyFinished();
#ifdef CPP_UTILITIES_DEBUG_BUILD
    void logReply(QNetworkReply *reply);
#endif

protected:
    QList<QNetworkReply *> m_replies;
    SongDescription m_initialDescription;
};

template <class Object, class Function> inline void HttpResultsModel::addReply(QNetworkReply *reply, Object object, Function handler)
{
    (m_replies << reply), connect(reply, &QNetworkReply::finished, object, handler);
#ifdef CPP_UTILITIES_DEBUG_BUILD
    logReply(reply);
#endif
}

/*!
 * \brief Adds a reply.
 * \remarks Called within c'tor and handleReplyFinished() in case of redirection. Might be called when subclassing to do further requests.
 */
template <class Function> inline void HttpResultsModel::addReply(QNetworkReply *reply, Function handler)
{
    (m_replies << reply), connect(reply, &QNetworkReply::finished, handler);
#ifdef CPP_UTILITIES_DEBUG_BUILD
    logReply(reply);
#endif
}

QueryResultsModel *queryMusicBrainz(SongDescription &&songDescription);
QueryResultsModel *queryLyricsWikia(SongDescription &&songDescription);
QNetworkReply *queryCoverArtArchive(const QString &albumId);
QueryResultsModel *queryMakeItPersonal(SongDescription &&songDescription);
QueryResultsModel *queryTekstowo(SongDescription &&songDescription);

} // namespace QtGui

#endif // DBQUERY_H
