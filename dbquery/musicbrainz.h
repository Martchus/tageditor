#ifndef QTGUI_MUSICBRAINZ_H
#define QTGUI_MUSICBRAINZ_H

#include "./dbquery.h"

#include <map>

QT_FORWARD_DECLARE_CLASS(QNetworkRequest)

namespace QtGui {

class MusicBrainzResultsModel : public HttpResultsModel
{
    Q_OBJECT
private:
    enum What {
        MusicBrainzMetaData,
        CoverArt
    };

public:
    MusicBrainzResultsModel(SongDescription &&initialSongDescription, QNetworkReply *reply);
    bool fetchCover(const QModelIndex &index);

protected:
    void parseInitialResults(const QByteArray &data);

private:
    void handleCoverReplyFinished(QNetworkReply *reply, const QString &albumId, int row);
    void parseCoverResults(const QString &albumId, int row, const QByteArray &data);

private:
    static std::map<QString, QByteArray> m_coverData;
    What m_what;
};

} // namespace QtGui

#endif // QTGUI_MUSICBRAINZ_H
