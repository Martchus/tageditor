#ifndef QTGUI_MUSICBRAINZ_H
#define QTGUI_MUSICBRAINZ_H

#include "./dbquery.h"

#include <QXmlStreamReader>

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
    static QNetworkRequest coverRequest(const QString &albumId);

protected:
    void parseResults(QNetworkReply *reply, const QByteArray &data);

private:
    static std::map<QString, QByteArray> m_coverData;
    QXmlStreamReader m_reader;
    What m_what;
};

} // namespace QtGui

#endif // QTGUI_MUSICBRAINZ_H
