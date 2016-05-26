#include "./settingsdialog.h"
#include "./notificationlabel.h"

#include "../application/settings.h"
#include "../application/knownfieldmodel.h"
#include "../application/targetlevelmodel.h"

#include <tagparser/mediafileinfo.h>
#include <tagparser/backuphelper.h>

#include <qtutilities/settingsdialog/optioncategory.h>
#include <qtutilities/settingsdialog/optioncategorymodel.h>
//#include <qtutilities/settingsdialog/qtsettings.h>

#include <QFileDialog>

#include <functional>

using namespace std;
using namespace std::placeholders;
using namespace Settings;
using namespace Media;

namespace QtGui {

// FileBrowserGeneralOptionPage
FileBrowserGeneralOptionPage::FileBrowserGeneralOptionPage(QWidget *parentWidget) :
    FileBrowserGeneralOptionPageBase(parentWidget)
{}

FileBrowserGeneralOptionPage::~FileBrowserGeneralOptionPage()
{}

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
EditorGeneralOptionPage::EditorGeneralOptionPage(QWidget *parentWidget) :
    EditorGeneralOptionPageBase(parentWidget)
{}

EditorGeneralOptionPage::~EditorGeneralOptionPage()
{}

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
        hideCoverButtons() = ui()->hideCoverButtonsCheckBox->isChecked();
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
        ui()->hideCoverButtonsCheckBox->setChecked(hideCoverButtons());
    }
}

// EditorTempOptionPage
EditorTempOptionPage::EditorTempOptionPage(QWidget *parentWindow) :
    EditorTempOptionPageBase(parentWindow)
{}

EditorTempOptionPage::~EditorTempOptionPage()
{}

bool EditorTempOptionPage::apply()
{
    if(hasBeenShown()) {
        BackupHelper::backupDirectory() = ui()->directoryWidget->lineEdit()->text().toStdString();
    }
    return true;
}

void EditorTempOptionPage::reset()
{
    if(hasBeenShown()) {
        ui()->directoryWidget->lineEdit()->setText(QString::fromStdString(BackupHelper::backupDirectory()));
    }
}

QWidget *EditorTempOptionPage::setupWidget()
{
    auto *widget = EditorTempOptionPageBase::setupWidget();
    ui()->notificationLabel->setText(QCoreApplication::translate("QtGui::EditorTempOptionPage", "To avoid unnecessary copying this directory should be on the same partition as the files you want to edit."));
    ui()->notificationLabel->setNotificationType(NotificationType::Information);
    return widget;
}

// EditorFieldsOptionPage
EditorFieldsOptionPage::EditorFieldsOptionPage(QWidget *parentWidget) :
    EditorFieldsOptionPageBase(parentWidget),
    m_model(nullptr)
{}

EditorFieldsOptionPage::~EditorFieldsOptionPage()
{}

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
    auto *w = EditorFieldsOptionPageBase::setupWidget();
    if(!m_model) {
        m_model = new KnownFieldModel(w);
    }
    ui()->fieldsListView->setModel(m_model);
    return w;
}

// EditorAutoCorrectionOptionPage
EditorAutoCorrectionOptionPage::EditorAutoCorrectionOptionPage(QWidget *parentWidget) :
    EditorAutoCorrectionOptionPageBase(parentWidget),
    m_model(nullptr)
{}

EditorAutoCorrectionOptionPage::~EditorAutoCorrectionOptionPage()
{}

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
    auto *w = EditorAutoCorrectionOptionPageBase::setupWidget();
    if(!m_model) {
        m_model = new KnownFieldModel(w);
    }
    ui()->fieldsListView->setModel(m_model);
    return w;
}

// EditorDbQueryOptionsPage
EditorDbQueryOptionsPage::EditorDbQueryOptionsPage(QWidget *parentWidget) :
    EditorDbQueryOptionsPageBase(parentWidget)
{}

EditorDbQueryOptionsPage::~EditorDbQueryOptionsPage()
{}

bool EditorDbQueryOptionsPage::apply()
{
    if(hasBeenShown()) {
        Settings::musicBrainzUrl() = ui()->musicBrainzUrlLineEdit->text();
        Settings::coverArtArchiveUrl() = ui()->coverArtArchiveUrlLineEdit->text();
    }
    return true;
}

void EditorDbQueryOptionsPage::reset()
{
    if(hasBeenShown()) {
        ui()->musicBrainzUrlLineEdit->setText(Settings::musicBrainzUrl());
        ui()->coverArtArchiveUrlLineEdit->setText(Settings::coverArtArchiveUrl());
    }
}

// InfoOptionPage
InfoOptionPage::InfoOptionPage(QWidget *parentWidget) :
    InfoOptionPageBase(parentWidget)
{}

InfoOptionPage::~InfoOptionPage()
{}

bool InfoOptionPage::apply()
{
    if(hasBeenShown()) {
        Settings::forceFullParse() = ui()->forceFullParseCheckBox->isChecked();
#ifndef TAGEDITOR_NO_WEBVIEW
        Settings::noWebView() = ui()->noWebViewCheckBox->isChecked();
#endif
    }
    return true;
}

void InfoOptionPage::reset()
{
    if(hasBeenShown()) {
        ui()->forceFullParseCheckBox->setChecked(Settings::forceFullParse());
#ifdef TAGEDITOR_NO_WEBVIEW
        ui()->noWebViewCheckBox->setChecked(true);
        ui()->noWebViewCheckBox->setEnabled(false);
#else
        ui()->noWebViewCheckBox->setChecked(Settings::noWebView());
#endif
    }
}

// TagProcessingGeneralOptionPage
TagProcessingGeneralOptionPage::TagProcessingGeneralOptionPage(QWidget *parentWidget) :
    TagProcessingGeneralOptionPageBase(parentWidget)
{}

TagProcessingGeneralOptionPage::~TagProcessingGeneralOptionPage()
{}

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
Id3v1OptionPage::Id3v1OptionPage(QWidget *parentWidget) :
    Id3v1OptionPageBase(parentWidget)
{}

Id3v1OptionPage::~Id3v1OptionPage()
{}

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
Id3v2OptionPage::Id3v2OptionPage(QWidget *parentWidget) :
    Id3v2OptionPageBase(parentWidget)
{}

Id3v2OptionPage::~Id3v2OptionPage()
{}

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

// TagProcessingTargetsOptionPage
TagProcessingTargetsOptionPage::TagProcessingTargetsOptionPage(QWidget *parentWidget) :
    TagProcessingTargetsOptionPageBase(parentWidget),
    m_model(nullptr)
{}

TagProcessingTargetsOptionPage::~TagProcessingTargetsOptionPage()
{}

bool TagProcessingTargetsOptionPage::apply()
{
    if(hasBeenShown() && m_model) {
        Settings::defaultTargetsModel().setItems(m_model->items());
    }
    return true;
}

void TagProcessingTargetsOptionPage::reset()
{
    if(hasBeenShown() && m_model) {
        m_model->setItems(Settings::defaultTargetsModel().items());
    }
}

QWidget *TagProcessingTargetsOptionPage::setupWidget()
{
    auto *w = TagProcessingTargetsOptionPageBase::setupWidget();
    if(!m_model) {
        m_model = new TargetLevelModel(w);
    }
    ui()->targetsToBeAddedListView->setModel(m_model);
    return w;
}

// FileLayoutPage
FileLayoutPage::FileLayoutPage(QWidget *parentWidget) :
    FileLayoutPageBase(parentWidget)
{}

FileLayoutPage::~FileLayoutPage()
{}

bool FileLayoutPage::apply()
{
    // error handling
    errors().clear();
    bool ok = true;

    if(hasBeenShown()) {
        Settings::forceRewrite() = ui()->forceRewriteCheckBox->isChecked();
        if(ui()->minPaddingSpinBox->value() > ui()->maxPaddingSpinBox->value()) {
            errors() << QCoreApplication::translate("QtGui::FileLayoutPage", "Minimum padding must be less or equal than maximum padding.");
            ok = false;
        } else {
            Settings::maxPadding() = static_cast<size_t>(ui()->maxPaddingSpinBox->value());
            Settings::minPadding() = static_cast<size_t>(ui()->minPaddingSpinBox->value());
        }
        Settings::preferredPadding() = static_cast<size_t>(ui()->preferredPaddingSpinBox->value());
        if(ui()->tagPosBeforeDataRadioButton->isChecked()) {
            preferredTagPosition() = ElementPosition::BeforeData;
        } else if(ui()->tagPosAfterDataRadioButton->isChecked()) {
            preferredTagPosition() = ElementPosition::AfterData;
        } else if(ui()->tagPosKeepRadioButton->isChecked()) {
            preferredTagPosition() = ElementPosition::Keep;
        }
        forceTagPosition() = ui()->tagPosForceCheckBox->isChecked();
        if(ui()->indexPosBeforeDataRadioButton->isChecked()) {
            preferredIndexPosition() = ElementPosition::BeforeData;
        } else if(ui()->indexPosAfterDataRadioButton->isChecked()) {
            preferredIndexPosition() = ElementPosition::AfterData;
        } else if(ui()->indexPosKeepRadioButton->isChecked()) {
            preferredIndexPosition() = ElementPosition::Keep;
        }
        forceIndexPosition() = ui()->indexPosForceCheckBox->isChecked();
    }
    return ok;
}

void FileLayoutPage::reset()
{
    if(hasBeenShown()) {
        ui()->forceRewriteCheckBox->setChecked(Settings::forceRewrite());
        ui()->maxPaddingSpinBox->setValue(static_cast<int>(Settings::maxPadding()));
        ui()->minPaddingSpinBox->setValue(static_cast<int>(Settings::minPadding()));
        ui()->preferredPaddingSpinBox->setValue(static_cast<int>(Settings::preferredPadding()));
        switch(preferredTagPosition()) {
        case ElementPosition::BeforeData:
            ui()->tagPosBeforeDataRadioButton->setChecked(true);
            break;
        case ElementPosition::AfterData:
            ui()->tagPosAfterDataRadioButton->setChecked(true);
            break;
        case ElementPosition::Keep:
            ui()->tagPosKeepRadioButton->setChecked(true);
            break;
        }
        ui()->tagPosForceCheckBox->setChecked(forceTagPosition());
        switch(preferredIndexPosition()) {
        case ElementPosition::BeforeData:
            ui()->indexPosBeforeDataRadioButton->setChecked(true);
            break;
        case ElementPosition::AfterData:
            ui()->indexPosAfterDataRadioButton->setChecked(true);
            break;
        case ElementPosition::Keep:
            ui()->indexPosKeepRadioButton->setChecked(true);
            break;
        }
        ui()->indexPosForceCheckBox->setChecked(forceIndexPosition());
    }
}

QWidget *FileLayoutPage::setupWidget()
{
    auto *widget = FileLayoutPageBase::setupWidget();
    ui()->preferredTagPosLabel->setNotificationType(NotificationType::Warning);
    ui()->preferredTagPosLabel->setText(QApplication::translate("QtGui::FileLayoutPage", "These options might be ignored if not supported by either the format or the implementation."));
    QObject::connect(ui()->minPaddingSpinBox, static_cast<void(QSpinBox::*)(int)>(&QSpinBox::valueChanged), ui()->maxPaddingSpinBox, &QSpinBox::setMinimum);
    QObject::connect(ui()->minPaddingSpinBox, static_cast<void(QSpinBox::*)(int)>(&QSpinBox::valueChanged), ui()->preferredPaddingSpinBox, &QSpinBox::setMinimum);
    QObject::connect(ui()->maxPaddingSpinBox, static_cast<void(QSpinBox::*)(int)>(&QSpinBox::valueChanged), ui()->minPaddingSpinBox, &QSpinBox::setMaximum);
    QObject::connect(ui()->maxPaddingSpinBox, static_cast<void(QSpinBox::*)(int)>(&QSpinBox::valueChanged), ui()->preferredPaddingSpinBox, &QSpinBox::setMaximum);
    return widget;
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
                          << new TagProcessingGeneralOptionPage << new Id3v1OptionPage
                          << new Id3v2OptionPage << new TagProcessingTargetsOptionPage << new FileLayoutPage);
    category->setIcon(QIcon::fromTheme(QStringLiteral("tag"), QIcon(QStringLiteral(":/tageditor/icons/hicolor/32x32/settingscategories/tag.png"))));
    categories << category;

    category = new Dialogs::OptionCategory(this);
    category->setDisplayName(tr("Editor"));
    category->setIcon(QIcon::fromTheme(QStringLiteral("document-edit"), QIcon(QStringLiteral(":/tageditor/icons/hicolor/32x32/settingscategories/key-enter.png"))));
    category->assignPages(QList<Dialogs::OptionPage *>()
                          << new EditorGeneralOptionPage << new EditorTempOptionPage(this) << new EditorFieldsOptionPage
                          << new InfoOptionPage << new EditorAutoCorrectionOptionPage << new EditorDbQueryOptionsPage);
    categories << category;

    category = new Dialogs::OptionCategory(this);
    category->setDisplayName(tr("File browser"));
    category->setIcon(QIcon::fromTheme(QStringLiteral("view-list-tree"), QIcon(QStringLiteral(":/tageditor/icons/hicolor/32x32/settingscategories/system-file-manager.png"))));
    category->assignPages(QList<Dialogs::OptionPage *>() << new FileBrowserGeneralOptionPage);
    categories << category;

    //categories <<  Dialogs::qtOptionCategory(this);

    categoryModel()->setCategories(categories);

    setMinimumSize(800, 450);
    setWindowIcon(QIcon::fromTheme(QStringLiteral("preferences-other"), QIcon(QStringLiteral(":/tageditor/icons/hicolor/32x32/settingscategories/preferences-other.png"))));
}

SettingsDialog::~SettingsDialog()
{}

}
