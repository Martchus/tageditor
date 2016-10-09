#ifndef QTGUI_LYRICSWIKIA_H
#define QTGUI_LYRICSWIKIA_H

#include "./dbquery.h"

#include <map>

namespace QtGui {

class LyricsWikiaResultsModel : public HttpResultsModel
{
    Q_OBJECT

public:
    LyricsWikiaResultsModel(SongDescription &&initialSongDescription, QNetworkReply *reply);
    bool fetchLyrics(const QModelIndex &index);

protected:
    void parseInitialResults(const QByteArray &data);

private:
    QNetworkReply *requestSongDetails(const SongDescription &songDescription);
    void handleSongDetailsFinished(QNetworkReply *reply, int row);
    void parseSongDetails(int row, const QByteArray &data);
    void handleLyricsReplyFinished(QNetworkReply *reply, int row);
    void parseLyricsResults(int row, const QByteArray &data);

};

} // namespace QtGui

#endif // QTGUI_LYRICSWIKIA_H
