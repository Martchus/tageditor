#include "./settingsdialog.h"
#include "./notificationlabel.h"

#include "../application/settings.h"
#include "../application/knownfieldmodel.h"

#include <tagparser/mediafileinfo.h>
#include <tagparser/backuphelper.h>

#include <qtutilities/settingsdialog/optioncategory.h>
#include <qtutilities/settingsdialog/optioncategorymodel.h>

#include <QFileDialog>

using namespace Settings;
using namespace Media;

namespace QtGui {

// FileBrowserGeneralOptionPage
FileBrowserGeneralOptionPage::FileBrowserGeneralOptionPage()
{}

FileBrowserGeneralOptionPage::~FileBrowserGeneralOptionPage()
{}

QString FileBrowserGeneralOptionPage::displayName() const
{
    return QApplication::translate("QtGui::FileBrowserGeneralOptionPage", "General");
}

bool FileBrowserGeneralOptionPage::apply()
{
    if(hasBeenShown()) {
        hideBackupFiles() = ui()->hideBackupFilesCheckBox->isChecked();
        fileBrowserReadOnly() = ui()->readOnlyCheckBox->isChecked();
    }
    return true;
}

void FileBrowserGeneralOptionPage::reset()
{
    if(hasBeenShown()) {
        ui()->hideBackupFilesCheckBox->setChecked(hideBackupFiles());
        ui()->readOnlyCheckBox->setChecked(fileBrowserReadOnly());
    }
}

// EditorGeneralOptionPage
EditorGeneralOptionPage::EditorGeneralOptionPage()
{}

EditorGeneralOptionPage::~EditorGeneralOptionPage()
{}

QString EditorGeneralOptionPage::displayName() const
{
    return QApplication::translate("QtGui::EditorGeneralOptionPage", "General");
}

bool EditorGeneralOptionPage::apply()
{
    if(hasBeenShown()) {
        if(ui()->disableAdoptRadioButton->isChecked()) {
            adoptFields() = AdoptFields::Never;
        } else if(ui()->enableWithinDirRadioButton->isChecked()) {
            adoptFields() = AdoptFields::WithinDirectory;
        } else if(ui()->enableForAllRadioButton->isChecked()) {
            adoptFields() = AdoptFields::Always;
        }
        if(ui()->multipleTagsOneEditorRadioButton->isChecked()) {
            multipleTagHandling() = MultipleTagHandling::SingleEditorPerTarget;
        } else if(ui()->multipleTagsSeparateEditorsRadioButton->isChecked()) {
            multipleTagHandling() = MultipleTagHandling::SeparateEditors;
        }
        saveAndShowNextOnEnter() = ui()->nextWhenPressingEnterCheckBox->isChecked();
        askBeforeDeleting() = ui()->askBeforeDeletingCheckBox->isChecked();
        hideTagSelectionComboBox() = ui()->hideTagSelectionComboBoxCheckBox->isChecked();
    }
    return true;
}

void EditorGeneralOptionPage::reset()
{
    if(hasBeenShown()) {
        switch(adoptFields()) {
        case AdoptFields::Never:
            ui()->disableAdoptRadioButton->setChecked(true);
            break;
        case AdoptFields::WithinDirectory:
            ui()->enableWithinDirRadioButton->setChecked(true);
            break;
        case AdoptFields::Always:
            ui()->enableForAllRadioButton->setChecked(true);
            break;
        }
        switch(multipleTagHandling()) {
        case MultipleTagHandling::SingleEditorPerTarget:
            ui()->multipleTagsOneEditorRadioButton->setChecked(true);
            break;
        case MultipleTagHandling::SeparateEditors:
            ui()->multipleTagsSeparateEditorsRadioButton->setChecked(true);
            break;
        }
        ui()->nextWhenPressingEnterCheckBox->setChecked(saveAndShowNextOnEnter());
        ui()->askBeforeDeletingCheckBox->setChecked(askBeforeDeleting());
        ui()->hideTagSelectionComboBoxCheckBox->setChecked(hideTagSelectionComboBox());
    }
}

// EditorTempOptionPage
EditorTempOptionPage::EditorTempOptionPage(QWidget *parentWindow) :
    UiFileBasedOptionPage<Ui::EditorTempOptionPage>(parentWindow)
{}

EditorTempOptionPage::~EditorTempOptionPage()
{}

QString EditorTempOptionPage::displayName() const
{
    return QApplication::translate("QtGui::EditorTempOptionPage", "Temp files");
}

bool EditorTempOptionPage::apply()
{
    if(hasBeenShown()) {
        BackupHelper::backupDirectory() = ui()->pathLineEdit->text().toStdString();
    }
    return true;
}

void EditorTempOptionPage::reset()
{
    if(hasBeenShown()) {
        ui()->pathLineEdit->setText(QString::fromStdString(BackupHelper::backupDirectory()));
    }
}

QWidget *EditorTempOptionPage::setupWidget()
{
    QWidget *widget = UiFileBasedOptionPage<Ui::EditorTempOptionPage>::setupWidget();
    QObject::connect(ui()->selectPushButton, &QPushButton::clicked, std::bind(&EditorTempOptionPage::showDirectorySelection, this));
    return widget;
}

void EditorTempOptionPage::showDirectorySelection()
{
    QFileDialog dlg(parentWindow());
    dlg.setModal(true);
    dlg.setFileMode(QFileDialog::DirectoryOnly);
    dlg.setWindowTitle(QApplication::translate("QtGui::EditorTempOptionPage", "Select directory to store temporary files"));
    dlg.setDirectory(ui()->pathLineEdit->text());
    if(dlg.exec() == QFileDialog::Accepted && dlg.selectedFiles().size() == 1) {
        ui()->pathLineEdit->setText(dlg.selectedFiles().front());
    }
}

// EditorFieldsOptionPage
EditorFieldsOptionPage::EditorFieldsOptionPage() :
    m_model(nullptr)
{}

EditorFieldsOptionPage::~EditorFieldsOptionPage()
{}

QString EditorFieldsOptionPage::displayName() const
{
    return QApplication::translate("QtGui::EditorFieldsOptionPage", "Fields");
}

bool EditorFieldsOptionPage::apply()
{
    if(hasBeenShown() && m_model) {
        Settings::selectedFieldsModel().setItems(m_model->items());
    }
    return true;
}

void EditorFieldsOptionPage::reset()
{
    if(hasBeenShown() && m_model) {
        m_model->setItems(Settings::selectedFieldsModel().items());
    }
}

QWidget *EditorFieldsOptionPage::setupWidget()
{
    QWidget *w = UiFileBasedOptionPage<Ui::EditorFieldsOptionPage>::setupWidget();
    if(!m_model) {
        m_model = new KnownFieldModel(w);
    }
    ui()->fieldsListView->setModel(m_model);
    return w;
}

// EditorAutoCorrectionOptionPage
EditorAutoCorrectionOptionPage::EditorAutoCorrectionOptionPage() :
    m_model(nullptr)
{}

EditorAutoCorrectionOptionPage::~EditorAutoCorrectionOptionPage()
{}

QString EditorAutoCorrectionOptionPage::displayName() const
{
    return QApplication::translate("QtGui::EditorAutoCorrectionOptionPage", "Auto correction/completition");
}

bool EditorAutoCorrectionOptionPage::apply()
{
    if(hasBeenShown()) {
        Settings::insertTitleFromFilename() = ui()->insertTitleFromFilenameCheckBox->isChecked();
        Settings::trimWhitespaces() = ui()->trimWhitespacesCheckBox->isChecked();
        Settings::formatNames() = ui()->formatNamesCheckBox->isChecked();
        Settings::fixUmlauts() = ui()->fixUmlautsCheckBox->isChecked();
        if(m_model) {
            Settings::autoCorrectionFields().setItems(m_model->items());
        }
    }
    return true;
}

void EditorAutoCorrectionOptionPage::reset()
{
    if(hasBeenShown()) {
        ui()->insertTitleFromFilenameCheckBox->setChecked(Settings::insertTitleFromFilename());
        ui()->trimWhitespacesCheckBox->setChecked(Settings::trimWhitespaces());
        ui()->formatNamesCheckBox->setChecked(Settings::formatNames());
        ui()->fixUmlautsCheckBox->setChecked(Settings::fixUmlauts());
        if(m_model) {
            m_model->setItems(Settings::autoCorrectionFields().items());
        }
    }
}

QWidget *EditorAutoCorrectionOptionPage::setupWidget()
{
    QWidget *w = UiFileBasedOptionPage<Ui::EditorAutoCorrectionOptionPage>::setupWidget();
    if(!m_model) {
        m_model = new KnownFieldModel(w);
    }
    ui()->fieldsListView->setModel(m_model);
    return w;
}

// InfoOptionPage
InfoOptionPage::InfoOptionPage()
{}

InfoOptionPage::~InfoOptionPage()
{}

QString InfoOptionPage::displayName() const
{
    return QApplication::translate("QtGui::InfoOptionPage", "Info");
}

bool InfoOptionPage::apply()
{
    if(hasBeenShown()) {
        Settings::forceFullParse() = ui()->forceFullParseCheckBox->isChecked();
    }
    return true;
}

void InfoOptionPage::reset()
{
    if(hasBeenShown()) {
        ui()->forceFullParseCheckBox->setChecked(Settings::forceFullParse());
    }
}

// TagProcessingGeneralOptionPage
TagProcessingGeneralOptionPage::TagProcessingGeneralOptionPage()
{}

TagProcessingGeneralOptionPage::~TagProcessingGeneralOptionPage()
{}

QString TagProcessingGeneralOptionPage::displayName() const
{
    return QApplication::translate("QtGui::TagProcessingGeneralOptionPage", "General");
}

bool TagProcessingGeneralOptionPage::apply()
{
    if(hasBeenShown()) {
        if(ui()->latin1RadioButton->isChecked()) {
            preferredEncoding() = TagTextEncoding::Latin1;
        } else if(ui()->utf8RadioButton->isChecked()) {
            preferredEncoding() = TagTextEncoding::Utf8;
        } else if(ui()->utf16leRadioButton->isChecked()) {
            preferredEncoding() = TagTextEncoding::Utf16LittleEndian;
        } else if(ui()->utf16beRadioButton->isChecked()) {
            preferredEncoding() = TagTextEncoding::Utf16BigEndian;
        } else if(ui()->autoRadioButton->isChecked()) {
            preferredEncoding() = TagTextEncoding::Unspecified;
        }

        if(ui()->ignoreUnsupportedRadioButton->isChecked()) {
            unsupportedFieldHandling() = UnsupportedFieldHandling::Ignore;
        } else if(ui()->discardUnsupportedRadioButton->isChecked()) {
            unsupportedFieldHandling() = UnsupportedFieldHandling::Discard;
        }
        autoTagManagement() = ui()->autoTagManagementCheckBox->isChecked();
    }
    return true;
}

void TagProcessingGeneralOptionPage::reset()
{
    if(hasBeenShown()) {
        switch(preferredEncoding()) {
        case TagTextEncoding::Latin1:
            ui()->latin1RadioButton->setChecked(true);
            break;
        case TagTextEncoding::Utf8:
            ui()->utf8RadioButton->setChecked(true);
            break;
        case TagTextEncoding::Utf16LittleEndian:
            ui()->utf16leRadioButton->setChecked(true);
            break;
        case TagTextEncoding::Utf16BigEndian:
            ui()->utf16beRadioButton->setChecked(true);
            break;
        case TagTextEncoding::Unspecified:
            ui()->autoRadioButton->setChecked(true);
            break;
        }
        switch(unsupportedFieldHandling()) {
        case UnsupportedFieldHandling::Ignore:
            ui()->ignoreUnsupportedRadioButton->setChecked(true);
            break;
        case UnsupportedFieldHandling::Discard:
            ui()->discardUnsupportedRadioButton->setChecked(true);
            break;
        }
        ui()->autoTagManagementCheckBox->setChecked(autoTagManagement());
    }
}

// Id3v1OptionPage
Id3v1OptionPage::Id3v1OptionPage()
{}

Id3v1OptionPage::~Id3v1OptionPage()
{}

QString Id3v1OptionPage::displayName() const
{
    return QApplication::translate("QtGui::Id3v1OptionPage", "ID3v1");
}

bool Id3v1OptionPage::apply()
{
    if(hasBeenShown()) {
        if(ui()->alwaysCreateRadioButton->isChecked()) {
            id3v1usage() = TagUsage::Always;
        } else if(ui()->keepExistingRadioButton->isChecked()) {
            id3v1usage() = TagUsage::KeepExisting;
        } else if(ui()->removeExistingRadioButton->isChecked()) {
            id3v1usage() = TagUsage::Never;
        }
    }
    return true;
}

void Id3v1OptionPage::reset()
{
    if(hasBeenShown()) {
        switch(id3v1usage()) {
        case TagUsage::Always:
            ui()->alwaysCreateRadioButton->setChecked(true);
            break;
        case TagUsage::KeepExisting:
            ui()->keepExistingRadioButton->setChecked(true);
            break;
        case TagUsage::Never:
            ui()->removeExistingRadioButton->setChecked(true);
            break;
        }
    }
}

// Id3v2OptionPage
Id3v2OptionPage::Id3v2OptionPage()
{}

Id3v2OptionPage::~Id3v2OptionPage()
{}

QString Id3v2OptionPage::displayName() const
{
    return QApplication::translate("QtGui::Id3v2OptionPage", "ID3v2");
}

bool Id3v2OptionPage::apply()
{
    if(hasBeenShown()) {
        if(ui()->alwaysCreateRadioButton->isChecked()) {
            id3v2usage() = TagUsage::Always;
        } else if(ui()->keepExistingRadioButton->isChecked()) {
            id3v2usage() = TagUsage::KeepExisting;
        } else if(ui()->removeExistingRadioButton->isChecked()) {
            id3v2usage() = TagUsage::Never;
        }
        if(ui()->version230radioButton->isChecked()) {
            id3v2versionToBeUsed() = 3;
        } else if(ui()->version240radioButton->isChecked()) {
            id3v2versionToBeUsed() = 4;
        } else if(ui()->version220radioButton->isChecked()) {
            id3v2versionToBeUsed() = 2;
        }
        keepVersionOfExistingId3v2Tag() = ui()->keepExistingVersionCheckBox->isChecked();
        mergeMultipleSuccessiveId3v2Tags() = ui()->mergeRadioButton->isChecked();
    }
    return true;
}

void Id3v2OptionPage::reset()
{
    if(hasBeenShown()) {
        switch(id3v2usage()) {
        case TagUsage::Always:
            ui()->alwaysCreateRadioButton->setChecked(true);
            break;
        case TagUsage::KeepExisting:
            ui()->keepExistingRadioButton->setChecked(true);
            break;
        case TagUsage::Never:
            ui()->removeExistingRadioButton->setChecked(true);
            break;
        }
        switch(id3v2versionToBeUsed()) {
        case 3:
            ui()->version230radioButton->setChecked(true);
            break;
        case 4:
            ui()->version240radioButton->setChecked(true);
            break;
        case 2:
            ui()->version220radioButton->setChecked(true);
            break;
        }
        ui()->keepExistingVersionCheckBox->setChecked(keepVersionOfExistingId3v2Tag());
        if(mergeMultipleSuccessiveId3v2Tags()) {
            ui()->mergeRadioButton->setChecked(true);
        } else {
            ui()->keepSeparateRadioButton->setChecked(true);
        }
    }
}

/*
    TRANSLATOR QtGui::SettingsDialog
    Necessary for lupdate.
*/

SettingsDialog::SettingsDialog(QWidget *parent) :
    Dialogs::SettingsDialog(parent)
{
    // setup categories
    QList<Dialogs::OptionCategory *> categories;
    Dialogs::OptionCategory *category;

    category = new Dialogs::OptionCategory(this);
    category->setDisplayName(tr("Tag processing"));
    category->assignPages(QList<Dialogs::OptionPage *>()
                          << new TagProcessingGeneralOptionPage <<
                          new Id3v1OptionPage << new Id3v2OptionPage);
    category->setIcon(QIcon(QStringLiteral(":/tageditor/icons/hicolor/32x32/settingscategories/tag.png")));
    categories << category;

    category = new Dialogs::OptionCategory(this);
    category->setDisplayName(tr("Editor"));
    category->setIcon(QIcon(QStringLiteral(":/tageditor/icons/hicolor/32x32/settingscategories/key-enter.png")));
    category->assignPages(QList<Dialogs::OptionPage *>()
                          << new EditorGeneralOptionPage << new EditorTempOptionPage(this) << new EditorFieldsOptionPage
                          << new InfoOptionPage << new EditorAutoCorrectionOptionPage);
    categories << category;

    category = new Dialogs::OptionCategory(this);
    category->setDisplayName(tr("File browser"));
    category->setIcon(QIcon(QStringLiteral(":/tageditor/icons/hicolor/32x32/settingscategories/system-file-manager.png")));
    category->assignPages(QList<Dialogs::OptionPage *>() << new FileBrowserGeneralOptionPage);
    categories << category;
    categoryModel()->setCategories(categories);

    setMinimumSize(800, 450);
    setWindowIcon(QIcon::fromTheme(QStringLiteral("preferences-other"), QIcon(QStringLiteral(":/tageditor/icons/hicolor/32x32/settingscategories/preferences-other.png"))));
}

SettingsDialog::~SettingsDialog()
{}


}
