#ifndef ID3V2OPTIONPAGE_H
#define ID3V2OPTIONPAGE_H

#include "gui/ui_filebrowsergeneraloptionpage.h"
#include "gui/ui_editorgeneraloptionpage.h"
#include "gui/ui_editortempoptionpage.h"
#include "gui/ui_editorfieldsoptionpage.h"
#include "gui/ui_editorautocorrectionoptionpage.h"
#include "gui/ui_infooptionpage.h"
#include "gui/ui_tagprocessinggeneraloptionpage.h"
#include "gui/ui_id3v1optionpage.h"
#include "gui/ui_id3v2optionpage.h"

#include <qtutilities/settingsdialog/settingsdialog.h>
#include <qtutilities/settingsdialog/optionpage.h>

#include <QWidget>

namespace Settings {
class KnownFieldModel;
}

namespace QtGui {

// FileBrowserGeneralOptionPage
namespace Ui {
class FileBrowserGeneralOptionPage;
}

class FileBrowserGeneralOptionPage : public Dialogs::UiFileBasedOptionPage<Ui::FileBrowserGeneralOptionPage>
{
public:
    explicit FileBrowserGeneralOptionPage();
    ~FileBrowserGeneralOptionPage();

    QString displayName() const;
    bool apply();
    void reset();
};

// EditorGeneralOptionPage
namespace Ui {
class EditorGeneralOptionPage;
}

class EditorGeneralOptionPage : public Dialogs::UiFileBasedOptionPage<Ui::EditorGeneralOptionPage>
{
public:
    explicit EditorGeneralOptionPage();
    ~EditorGeneralOptionPage();

    QString displayName() const;
    bool apply();
    void reset();
};

// EditorTempOptionPage
namespace Ui {
class EditorTempOptionPage;
}

class EditorTempOptionPage : public Dialogs::UiFileBasedOptionPage<Ui::EditorTempOptionPage>
{
public:
    explicit EditorTempOptionPage(QWidget *parentWindow);
    ~EditorTempOptionPage();

    QString displayName() const;
    bool apply();
    void reset();

protected:
    QWidget *setupWidget();

private:
    void showDirectorySelection();
};

// EditorFieldsOptionPage
namespace Ui {
class EditorFieldsOptionPage;
}

class EditorFieldsOptionPage : public Dialogs::UiFileBasedOptionPage<Ui::EditorFieldsOptionPage>
{
public:
    explicit EditorFieldsOptionPage();
    ~EditorFieldsOptionPage();

    QString displayName() const;
    bool apply();
    void reset();
private:
    QWidget *setupWidget();
    Settings::KnownFieldModel *m_model;
};

// EditorAutoCorrectionOptionPage
namespace Ui {
class EditorAutoCorrectionOptionPage;
}

class EditorAutoCorrectionOptionPage : public Dialogs::UiFileBasedOptionPage<Ui::EditorAutoCorrectionOptionPage>
{
public:
    explicit EditorAutoCorrectionOptionPage();
    ~EditorAutoCorrectionOptionPage();

    QString displayName() const;
    bool apply();
    void reset();
private:
    QWidget *setupWidget();
    Settings::KnownFieldModel *m_model;
};

// EditorInfoOptionPage
namespace Ui {
class InfoOptionPage;
}

class InfoOptionPage : public Dialogs::UiFileBasedOptionPage<Ui::InfoOptionPage>
{
public:
    explicit InfoOptionPage();
    ~InfoOptionPage();

    QString displayName() const;
    bool apply();
    void reset();
};


// TagProcessingGeneralOptionPage
namespace Ui {
class TagProcessingGeneralOptionPage;
}

class TagProcessingGeneralOptionPage : public Dialogs::UiFileBasedOptionPage<Ui::TagProcessingGeneralOptionPage>
{
public:
    explicit TagProcessingGeneralOptionPage();
    ~TagProcessingGeneralOptionPage();

    QString displayName() const;
    bool apply();
    void reset();
};

// Id3v1OptionPage
namespace Ui {
class Id3v1OptionPage;
}

class Id3v1OptionPage : public Dialogs::UiFileBasedOptionPage<Ui::Id3v1OptionPage>
{
public:
    explicit Id3v1OptionPage();
    ~Id3v1OptionPage();

    QString displayName() const;
    bool apply();
    void reset();
};

// Id3v2OptionPage
namespace Ui {
class Id3v2OptionPage;
}

class Id3v2OptionPage : public Dialogs::UiFileBasedOptionPage<Ui::Id3v2OptionPage>
{    
public:
    explicit Id3v2OptionPage();
    ~Id3v2OptionPage();

    QString displayName() const;
    bool apply();
    void reset();
};

class SettingsDialog : public Dialogs::SettingsDialog
{
    Q_OBJECT
public:
    explicit SettingsDialog(QWidget *parent = nullptr);
    ~SettingsDialog();

};

}

#endif // ID3V2OPTIONPAGE_H
