#ifndef GENERALFILEINFOWIDGET_H
#define GENERALFILEINFOWIDGET_H

#include "infowidgetbase.h"

#include <string>

QT_BEGIN_NAMESPACE
class QLabel;
QT_END_NAMESPACE

namespace Media {
class MediaFileInfo;
}

namespace QtGui {

class GeneralFileInfoWidget : public InfoWidgetBase
{
    Q_OBJECT
public:
    explicit GeneralFileInfoWidget(QWidget *parent = nullptr);
    ~GeneralFileInfoWidget();

    void update(const Media::MediaFileInfo &fileInfo);

private:
    QLabel *m_pathLabel;
    QLabel *m_sizeLabel;
    QLabel *m_durationLabel;
    QLabel *m_bitrateLabel;
    QLabel *m_containerLabel;
    QLabel *m_mimeTypeLabel;
};

}

#endif // GENERALFILEINFOWIDGET_H
