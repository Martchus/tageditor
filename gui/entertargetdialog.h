#ifndef ENTERTARGETLEVELDIALOG_H
#define ENTERTARGETLEVELDIALOG_H

#include <tagparser/tagtarget.h>

#include <QDialog>

#include <memory>

namespace Models {
class ChecklistModel;
}

namespace Media {
class MediaFileInfo;
}

namespace QtGui {

namespace Ui {
class EnterTargetDialog;
}

class EnterTargetDialog : public QDialog
{
    Q_OBJECT
    Q_PROPERTY(Media::TagTarget target READ target WRITE setTarget)

public:
    explicit EnterTargetDialog(QWidget *parent = nullptr);
    ~EnterTargetDialog();

    Media::TagTarget target() const;
    void setTarget(const Media::TagTarget &target, const Media::MediaFileInfo *file = nullptr);

private slots:
    void updateLevelNamePlaceholderText(int i);

private:
    std::unique_ptr<Ui::EnterTargetDialog> m_ui;
    Models::ChecklistModel *m_tracksModel;
    Models::ChecklistModel *m_chaptersModel;
    Models::ChecklistModel *m_editionsModel;
    Models::ChecklistModel *m_attachmentsModel;
};

}

#endif // ENTERTARGETLEVELDIALOG_H
