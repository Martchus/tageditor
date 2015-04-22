#ifndef CONTAINERINFOWIDGET_H
#define CONTAINERINFOWIDGET_H

#include "infowidgetbase.h"

QT_BEGIN_NAMESPACE
class QLabel;
class QLineEdit;
QT_END_NAMESPACE

namespace Media {
class MediaFileInfo;
class AbstractContainer;
}

namespace QtGui {

class ContainerInfoWidget : public InfoWidgetBase
{
    Q_OBJECT
public:
    explicit ContainerInfoWidget(QWidget *parent = nullptr);
    ~ContainerInfoWidget();

    void update(const Media::MediaFileInfo &fileInfo, const Media::AbstractContainer &container);

private:
    QLabel *m_nameLabel;
    QLabel *m_abbreviationLabel;
    QLabel *m_versionLabel;
    QLabel *m_readVersionLabel;
    QLabel *m_docTypeLabel;
    QLabel *m_docVersionLabel;
    QLabel *m_docReadVersionLabel;
    QLineEdit *m_titleLabel;

};

}

#endif // CONTAINERINFOWIDGET_H
