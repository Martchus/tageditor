#ifndef DBQUERY_H
#define DBQUERY_H

#include <c++utilities/application/global.h>

#include <QAbstractTableModel>

namespace Media {
class TagValue;
DECLARE_ENUM(KnownField, unsigned int)
}

namespace QtGui {

struct SongDescription
{
    SongDescription();

    QString title;
    QString album;
    QString artist;
    QString year;
    QString genre;
    unsigned int track;
    unsigned int totalTracks;
    unsigned int disk;
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
    Media::TagValue fieldValue(int row, Media::KnownField knownField) const;

    QVariant data(const QModelIndex &index, int role) const;
    Qt::ItemFlags flags(const QModelIndex &index) const;
    QVariant headerData(int section, Qt::Orientation orientation, int role) const;
    int rowCount(const QModelIndex &parent) const;
    int columnCount(const QModelIndex &parent) const;

signals:
    void resultsAvailable();

protected:
    QueryResultsModel(QObject *parent = nullptr);
    void setResultsAvailable(bool resultsAvailable);

    QList<SongDescription> m_results;
    QStringList m_errorList;
    bool m_resultsAvailable;
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

QueryResultsModel *queryMusicBrainz(const SongDescription &songDescription);

}

#endif // DBQUERY_H
