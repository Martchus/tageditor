#ifndef QTGUI_MAKE_IT_PERSONAL_H
#define QTGUI_MAKE_IT_PERSONAL_H

#include "./dbquery.h"

#include <map>

namespace QtGui {

class MakeItPersonalResultsModel : public HttpResultsModel {
    Q_OBJECT

public:
    MakeItPersonalResultsModel(SongDescription &&initialSongDescription, QNetworkReply *reply);
    bool fetchLyrics(const QModelIndex &index) override;

protected:
    void parseInitialResults(const QByteArray &data) override;
};

} // namespace QtGui

#endif // QTGUI_MAKE_IT_PERSONAL_H
