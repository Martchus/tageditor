#ifndef ID3V2OPTIONPAGE_H
#define ID3V2OPTIONPAGE_H

#include "ui_filebrowsergeneraloptionpage.h"
#include "ui_editorgeneraloptionpage.h"
#include "ui_editortempoptionpage.h"
#include "ui_editorfieldsoptionpage.h"
#include "ui_editorautocorrectionoptionpage.h"
#include "ui_editordbqueryoptionpage.h"
#include "ui_infooptionpage.h"
#include "ui_tagprocessinggeneraloptionpage.h"
#include "ui_id3v1optionpage.h"
#include "ui_id3v2optionpage.h"
#include "ui_tagprocessingtargetsoptionpage.h"
#include "ui_filelayout.h"

#include <qtutilities/settingsdialog/settingsdialog.h>
#include <qtutilities/settingsdialog/optionpage.h>

#include <QWidget>

namespace Settings {
class KnownFieldModel;
class TargetLevelModel;
}

DECLARE_EXTERN_UI_FILE_BASED_OPTION_PAGE(QtAppearanceOptionPage)
DECLARE_EXTERN_UI_FILE_BASED_OPTION_PAGE(QtLanguageOptionPage)
DECLARE_EXTERN_UI_FILE_BASED_OPTION_PAGE(QtEnvOptionPage)

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

class SettingsDialog : public Dialogs::SettingsDialog
{
    Q_OBJECT
public:
    explicit SettingsDialog(QWidget *parent = nullptr);
    ~SettingsDialog();
};

}

#endif // ID3V2OPTIONPAGE_H
