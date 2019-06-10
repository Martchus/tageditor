#ifndef ENTERTARGETLEVELDIALOG_H
#define ENTERTARGETLEVELDIALOG_H

#include <tagparser/tagtarget.h>

#include <QDialog>

#include <memory>

namespace QtUtilities {
class ChecklistModel;
}

namespace TagParser {
class MediaFileInfo;
enum class ContainerFormat : unsigned int;
} // namespace TagParser

namespace QtGui {

namespace Ui {
class EnterTargetDialog;
}

class EnterTargetDialog : public QDialog {
    Q_OBJECT
    Q_PROPERTY(TagParser::TagTarget target READ target WRITE setTarget)

public:
    explicit EnterTargetDialog(QWidget *parent = nullptr);
    ~EnterTargetDialog() override;

    TagParser::TagTarget target() const;
    void setTarget(const TagParser::TagTarget &target, const TagParser::MediaFileInfo *file = nullptr);

private slots:
    void updateLevelNamePlaceholderText(int i);

private:
    std::unique_ptr<Ui::EnterTargetDialog> m_ui;
    QtUtilities::ChecklistModel *m_tracksModel;
    QtUtilities::ChecklistModel *m_chaptersModel;
    QtUtilities::ChecklistModel *m_editionsModel;
    QtUtilities::ChecklistModel *m_attachmentsModel;
    TagParser::ContainerFormat m_currentContainerFormat;
};

} // namespace QtGui

#endif // ENTERTARGETLEVELDIALOG_H
