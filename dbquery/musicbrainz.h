#ifndef QTGUI_MUSICBRAINZ_H
#define QTGUI_MUSICBRAINZ_H

#include "./dbquery.h"

#include <map>

QT_FORWARD_DECLARE_CLASS(QNetworkRequest)

namespace QtGui {

class MusicBrainzResultsModel : public HttpResultsModel {
    Q_OBJECT
private:
    enum What { MusicBrainzMetaData, CoverArt };

public:
    MusicBrainzResultsModel(SongDescription &&initialSongDescription, QNetworkReply *reply);
    bool fetchCover(const QModelIndex &index) override;
    QUrl webUrl(const QModelIndex &index) override;

protected:
    void parseInitialResults(const QByteArray &data) override;

private:
    What m_what;
};

} // namespace QtGui

#endif // QTGUI_MUSICBRAINZ_H
