#ifndef SETTINGS_DIALOG_H
#define SETTINGS_DIALOG_H

#include <qtutilities/settingsdialog/optionpage.h>
#include <qtutilities/settingsdialog/qtsettings.h>
#include <qtutilities/settingsdialog/settingsdialog.h>

#include <QWidget>

namespace Settings {
class KnownFieldModel;
class TargetLevelModel;
} // namespace Settings

namespace QtGui {

DECLARE_UI_FILE_BASED_OPTION_PAGE(FileBrowserGeneralOptionPage)

DECLARE_UI_FILE_BASED_OPTION_PAGE(EditorGeneralOptionPage)

DECLARE_UI_FILE_BASED_OPTION_PAGE_CUSTOM_SETUP(EditorTempOptionPage)

BEGIN_DECLARE_UI_FILE_BASED_OPTION_PAGE(EditorFieldsOptionPage)
DECLARE_SETUP_WIDGETS
Settings::KnownFieldModel *m_model;
END_DECLARE_OPTION_PAGE

BEGIN_DECLARE_UI_FILE_BASED_OPTION_PAGE(EditorAutoCorrectionOptionPage)
DECLARE_SETUP_WIDGETS
Settings::KnownFieldModel *m_model;
END_DECLARE_OPTION_PAGE

DECLARE_UI_FILE_BASED_OPTION_PAGE(EditorDbQueryOptionsPage)

DECLARE_UI_FILE_BASED_OPTION_PAGE(InfoOptionPage)

DECLARE_UI_FILE_BASED_OPTION_PAGE(TagProcessingGeneralOptionPage)

DECLARE_UI_FILE_BASED_OPTION_PAGE(Id3v1OptionPage)

DECLARE_UI_FILE_BASED_OPTION_PAGE(Id3v2OptionPage)

BEGIN_DECLARE_UI_FILE_BASED_OPTION_PAGE(TagProcessingTargetsOptionPage)
DECLARE_SETUP_WIDGETS
Settings::TargetLevelModel *m_model;
END_DECLARE_OPTION_PAGE

DECLARE_UI_FILE_BASED_OPTION_PAGE_CUSTOM_SETUP(FileLayoutPage)

class SettingsDialog : public Dialogs::SettingsDialog {
    Q_OBJECT
public:
    explicit SettingsDialog(QWidget *parent = nullptr);
    ~SettingsDialog() override;
};

} // namespace QtGui

DECLARE_EXTERN_UI_FILE_BASED_OPTION_PAGE_NS(QtGui, FileBrowserGeneralOptionPage)
DECLARE_EXTERN_UI_FILE_BASED_OPTION_PAGE_NS(QtGui, EditorGeneralOptionPage)
DECLARE_EXTERN_UI_FILE_BASED_OPTION_PAGE_NS(QtGui, EditorTempOptionPage)
DECLARE_EXTERN_UI_FILE_BASED_OPTION_PAGE_NS(QtGui, EditorFieldsOptionPage)
DECLARE_EXTERN_UI_FILE_BASED_OPTION_PAGE_NS(QtGui, EditorAutoCorrectionOptionPage)
DECLARE_EXTERN_UI_FILE_BASED_OPTION_PAGE_NS(QtGui, EditorDbQueryOptionsPage)
DECLARE_EXTERN_UI_FILE_BASED_OPTION_PAGE_NS(QtGui, InfoOptionPage)
DECLARE_EXTERN_UI_FILE_BASED_OPTION_PAGE_NS(QtGui, TagProcessingGeneralOptionPage)
DECLARE_EXTERN_UI_FILE_BASED_OPTION_PAGE_NS(QtGui, Id3v1OptionPage)
DECLARE_EXTERN_UI_FILE_BASED_OPTION_PAGE_NS(QtGui, Id3v2OptionPage)
DECLARE_EXTERN_UI_FILE_BASED_OPTION_PAGE_NS(QtGui, TagProcessingTargetsOptionPage)
DECLARE_EXTERN_UI_FILE_BASED_OPTION_PAGE_NS(QtGui, FileLayoutPage)

#endif // SETTINGS_DIALOG_H
