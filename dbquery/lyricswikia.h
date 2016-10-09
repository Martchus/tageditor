#ifndef QTGUI_LYRICSWIKIA_H
#define QTGUI_LYRICSWIKIA_H

#include "./dbquery.h"

#include <QXmlStreamReader>

namespace QtGui {

class LyricsWikiaResultsModel : public HttpResultsModel
{
    Q_OBJECT

public:
    LyricsWikiaResultsModel(SongDescription &&initialSongDescription, QNetworkReply *reply);

protected:
    void parseResults(QNetworkReply *reply, const QByteArray &data);

private:
    QXmlStreamReader m_reader;

};

} // namespace QtGui

#endif // QTGUI_LYRICSWIKIA_H
