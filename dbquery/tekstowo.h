#ifndef QTGUI_TEKSTOWO_H
#define QTGUI_TEKSTOWO_H

#include "./dbquery.h"

#include <map>

namespace QtGui {

class TekstowoResultsModel : public HttpResultsModel {
    Q_OBJECT

public:
    explicit TekstowoResultsModel(SongDescription &&initialSongDescription, QNetworkReply *reply);
    bool fetchLyrics(const QModelIndex &index) override;
    QUrl webUrl(const QModelIndex &index) override;

protected:
    void parseInitialResults(const QByteArray &data) override;

private:
    void handleLyricsReplyFinished(QNetworkReply *reply, int row);
    void parseLyricsResults(int row, const QByteArray &data);
};

} // namespace QtGui

#endif // QTGUI_TEKSTOWO_H
