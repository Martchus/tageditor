#include "./settingsdialog.h"
#include "./notificationlabel.h"

#include "../application/knownfieldmodel.h"
#include "../application/settings.h"
#include "../application/targetlevelmodel.h"

#include "ui_editorautocorrectionoptionpage.h"
#include "ui_editordbqueryoptionpage.h"
#include "ui_editorfieldsoptionpage.h"
#include "ui_editorgeneraloptionpage.h"
#include "ui_editortempoptionpage.h"
#include "ui_filebrowsergeneraloptionpage.h"
#include "ui_filelayout.h"
#include "ui_id3v1optionpage.h"
#include "ui_id3v2optionpage.h"
#include "ui_infooptionpage.h"
#include "ui_tagprocessinggeneraloptionpage.h"
#include "ui_tagprocessingtargetsoptionpage.h"

#include <tagparser/backuphelper.h>
#include <tagparser/mediafileinfo.h>

#include <qtutilities/settingsdialog/optioncategory.h>
#include <qtutilities/settingsdialog/optioncategorymodel.h>
#include <qtutilities/settingsdialog/qtsettings.h>

#include <QFileDialog>

#include <functional>

using namespace std;
using namespace std::placeholders;
using namespace Settings;
using namespace TagParser;
using namespace QtUtilities;

namespace QtGui {

// FileBrowserGeneralOptionPage
FileBrowserGeneralOptionPage::FileBrowserGeneralOptionPage(QWidget *parentWidget)
    : FileBrowserGeneralOptionPageBase(parentWidget)
{
}

FileBrowserGeneralOptionPage::~FileBrowserGeneralOptionPage()
{
}

bool FileBrowserGeneralOptionPage::apply()
{
    auto &settings = values().fileBrowser;
    if (hasBeenShown()) {
        settings.hideBackupFiles = ui()->hideBackupFilesCheckBox->isChecked();
        settings.readOnly = ui()->readOnlyCheckBox->isChecked();
    }
    return true;
}

void FileBrowserGeneralOptionPage::reset()
{
    const auto &settings = values().fileBrowser;
    if (hasBeenShown()) {
        ui()->hideBackupFilesCheckBox->setChecked(settings.hideBackupFiles);
        ui()->readOnlyCheckBox->setChecked(settings.readOnly);
    }
}

// EditorGeneralOptionPage
EditorGeneralOptionPage::EditorGeneralOptionPage(QWidget *parentWidget)
    : EditorGeneralOptionPageBase(parentWidget)
{
}

EditorGeneralOptionPage::~EditorGeneralOptionPage()
{
}

bool EditorGeneralOptionPage::apply()
{
    if (hasBeenShown()) {
        auto &settings = values().editor;
        if (ui()->disableAdoptRadioButton->isChecked()) {
            settings.adoptFields = AdoptFields::Never;
        } else if (ui()->enableWithinDirRadioButton->isChecked()) {
            settings.adoptFields = AdoptFields::WithinDirectory;
        } else if (ui()->enableForAllRadioButton->isChecked()) {
            settings.adoptFields = AdoptFields::Always;
        }
        if (ui()->multipleTagsOneEditorRadioButton->isChecked()) {
            settings.multipleTagHandling = MultipleTagHandling::SingleEditorPerTarget;
        } else if (ui()->multipleTagsSeparateEditorsRadioButton->isChecked()) {
            settings.multipleTagHandling = MultipleTagHandling::SeparateEditors;
        }
        settings.saveAndShowNextOnEnter = ui()->nextWhenPressingEnterCheckBox->isChecked();
        settings.askBeforeDeleting = ui()->askBeforeDeletingCheckBox->isChecked();
        settings.hideTagSelectionComboBox = ui()->hideTagSelectionComboBoxCheckBox->isChecked();
        settings.hideCoverButtons = ui()->hideCoverButtonsCheckBox->isChecked();
    }
    return true;
}

void EditorGeneralOptionPage::reset()
{
    if (hasBeenShown()) {
        const auto &settings = values().editor;
        switch (settings.adoptFields) {
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
        switch (settings.multipleTagHandling) {
        case MultipleTagHandling::SingleEditorPerTarget:
            ui()->multipleTagsOneEditorRadioButton->setChecked(true);
            break;
        case MultipleTagHandling::SeparateEditors:
            ui()->multipleTagsSeparateEditorsRadioButton->setChecked(true);
            break;
        }
        ui()->nextWhenPressingEnterCheckBox->setChecked(settings.saveAndShowNextOnEnter);
        ui()->askBeforeDeletingCheckBox->setChecked(settings.askBeforeDeleting);
        ui()->hideTagSelectionComboBoxCheckBox->setChecked(settings.hideTagSelectionComboBox);
        ui()->hideCoverButtonsCheckBox->setChecked(settings.hideCoverButtons);
    }
}

// EditorTempOptionPage
EditorTempOptionPage::EditorTempOptionPage(QWidget *parentWindow)
    : EditorTempOptionPageBase(parentWindow)
{
}

EditorTempOptionPage::~EditorTempOptionPage()
{
}

bool EditorTempOptionPage::apply()
{
    if (hasBeenShown()) {
        values().editor.backupDirectory = ui()->directoryWidget->lineEdit()->text().toStdString();
    }
    return true;
}

void EditorTempOptionPage::reset()
{
    if (hasBeenShown()) {
        ui()->directoryWidget->lineEdit()->setText(QString::fromStdString(values().editor.backupDirectory));
    }
}

QWidget *EditorTempOptionPage::setupWidget()
{
    auto *widget = EditorTempOptionPageBase::setupWidget();
    ui()->notificationLabel->setText(QCoreApplication::translate(
        "QtGui::EditorTempOptionPage", "To avoid unnecessary copying this directory should be on the same partition as the files you want to edit."));
    ui()->notificationLabel->setNotificationType(NotificationType::Information);
    return widget;
}

// EditorFieldsOptionPage
EditorFieldsOptionPage::EditorFieldsOptionPage(QWidget *parentWidget)
    : EditorFieldsOptionPageBase(parentWidget)
    , m_model(nullptr)
{
}

EditorFieldsOptionPage::~EditorFieldsOptionPage()
{
}

bool EditorFieldsOptionPage::apply()
{
    if (hasBeenShown() && m_model) {
        values().editor.fields.setItems(m_model->items());
    }
    return true;
}

void EditorFieldsOptionPage::reset()
{
    if (hasBeenShown() && m_model) {
        m_model->setItems(values().editor.fields.items());
    }
}

QWidget *EditorFieldsOptionPage::setupWidget()
{
    auto *w = EditorFieldsOptionPageBase::setupWidget();
    if (!m_model) {
        m_model = new KnownFieldModel(w);
    }
    ui()->fieldsListView->setModel(m_model);
    return w;
}

// EditorAutoCorrectionOptionPage
EditorAutoCorrectionOptionPage::EditorAutoCorrectionOptionPage(QWidget *parentWidget)
    : EditorAutoCorrectionOptionPageBase(parentWidget)
    , m_model(nullptr)
{
}

EditorAutoCorrectionOptionPage::~EditorAutoCorrectionOptionPage()
{
}

bool EditorAutoCorrectionOptionPage::apply()
{
    if (hasBeenShown()) {
        auto &settings = values().editor.autoCompletition;
        settings.insertTitleFromFilename = ui()->insertTitleFromFilenameCheckBox->isChecked();
        settings.trimWhitespaces = ui()->trimWhitespacesCheckBox->isChecked();
        settings.formatNames = ui()->formatNamesCheckBox->isChecked();
        settings.fixUmlauts = ui()->fixUmlautsCheckBox->isChecked();
        auto &subst = settings.customSubstitution;
        subst.enabled = ui()->customSubstitutionGroupBox->isChecked();
        subst.regex = QRegularExpression(ui()->regularExpressionLineEdit->text());
        subst.replacement = ui()->replacementLineEdit->text();
        if (m_model) {
            settings.fields.setItems(m_model->items());
        }
    }
    return true;
}

void EditorAutoCorrectionOptionPage::reset()
{
    if (hasBeenShown()) {
        const auto &settings = values().editor.autoCompletition;
        ui()->insertTitleFromFilenameCheckBox->setChecked(settings.insertTitleFromFilename);
        ui()->trimWhitespacesCheckBox->setChecked(settings.trimWhitespaces);
        ui()->formatNamesCheckBox->setChecked(settings.formatNames);
        ui()->fixUmlautsCheckBox->setChecked(settings.fixUmlauts);
        const auto &subst = settings.customSubstitution;
        ui()->customSubstitutionGroupBox->setChecked(subst.enabled);
        ui()->regularExpressionLineEdit->setText(subst.regex.pattern());
        ui()->replacementLineEdit->setText(subst.replacement);
        if (m_model) {
            m_model->setItems(settings.fields.items());
        }
    }
}

QWidget *EditorAutoCorrectionOptionPage::setupWidget()
{
    auto *w = EditorAutoCorrectionOptionPageBase::setupWidget();
    if (!m_model) {
        m_model = new KnownFieldModel(w);
    }
    ui()->fieldsListView->setModel(m_model);
    return w;
}

// EditorDbQueryOptionsPage
EditorDbQueryOptionsPage::EditorDbQueryOptionsPage(QWidget *parentWidget)
    : EditorDbQueryOptionsPageBase(parentWidget)
{
}

EditorDbQueryOptionsPage::~EditorDbQueryOptionsPage()
{
}

bool EditorDbQueryOptionsPage::apply()
{
    if (hasBeenShown()) {
        auto &settings = values().dbQuery;
        settings.musicBrainzUrl = ui()->musicBrainzUrlLineEdit->text();
        settings.lyricsWikiaUrl = ui()->lyricWikiUrlLineEdit->text();
        settings.coverArtArchiveUrl = ui()->coverArtArchiveUrlLineEdit->text();
    }
    return true;
}

void EditorDbQueryOptionsPage::reset()
{
    if (hasBeenShown()) {
        const auto &settings = values().dbQuery;
        ui()->musicBrainzUrlLineEdit->setText(settings.musicBrainzUrl);
        ui()->lyricWikiUrlLineEdit->setText(settings.lyricsWikiaUrl);
        ui()->coverArtArchiveUrlLineEdit->setText(settings.coverArtArchiveUrl);
    }
}

// InfoOptionPage
InfoOptionPage::InfoOptionPage(QWidget *parentWidget)
    : InfoOptionPageBase(parentWidget)
{
}

InfoOptionPage::~InfoOptionPage()
{
}

bool InfoOptionPage::apply()
{
    if (hasBeenShown()) {
        auto &settings = values().editor;
        settings.forceFullParse = ui()->forceFullParseCheckBox->isChecked();
#ifndef TAGEDITOR_NO_WEBVIEW
        settings.noWebView = ui()->noWebViewCheckBox->isChecked();
#endif
    }
    return true;
}

void InfoOptionPage::reset()
{
    if (hasBeenShown()) {
        const auto &settings = values().editor;
        ui()->forceFullParseCheckBox->setChecked(settings.forceFullParse);
#ifdef TAGEDITOR_NO_WEBVIEW
        ui()->noWebViewCheckBox->setChecked(true);
        ui()->noWebViewCheckBox->setEnabled(false);
#else
        ui()->noWebViewCheckBox->setChecked(settings.noWebView);
#endif
    }
}

// TagProcessingGeneralOptionPage
TagProcessingGeneralOptionPage::TagProcessingGeneralOptionPage(QWidget *parentWidget)
    : TagProcessingGeneralOptionPageBase(parentWidget)
{
}

TagProcessingGeneralOptionPage::~TagProcessingGeneralOptionPage()
{
}

bool TagProcessingGeneralOptionPage::apply()
{
    if (hasBeenShown()) {
        auto &settings = values().tagPocessing;
        if (ui()->latin1RadioButton->isChecked()) {
            settings.preferredEncoding = TagTextEncoding::Latin1;
        } else if (ui()->utf8RadioButton->isChecked()) {
            settings.preferredEncoding = TagTextEncoding::Utf8;
        } else if (ui()->utf16leRadioButton->isChecked()) {
            settings.preferredEncoding = TagTextEncoding::Utf16LittleEndian;
        } else if (ui()->utf16beRadioButton->isChecked()) {
            settings.preferredEncoding = TagTextEncoding::Utf16BigEndian;
        } else if (ui()->autoRadioButton->isChecked()) {
            settings.preferredEncoding = TagTextEncoding::Unspecified;
        }

        if (ui()->ignoreUnsupportedRadioButton->isChecked()) {
            settings.unsupportedFieldHandling = UnsupportedFieldHandling::Ignore;
        } else if (ui()->discardUnsupportedRadioButton->isChecked()) {
            settings.unsupportedFieldHandling = UnsupportedFieldHandling::Discard;
        }
        settings.autoTagManagement = ui()->autoTagManagementCheckBox->isChecked();
        settings.preserveModificationTime = ui()->preserveModificationTimeCheckBox->isChecked();
        settings.preserveMuxingApp = ui()->preserveMuxingAppCheckBox->isChecked();
        settings.preserveWritingApp = ui()->preserveWritingAppCheckBox->isChecked();
    }
    return true;
}

void TagProcessingGeneralOptionPage::reset()
{
    if (hasBeenShown()) {
        const auto &settings = values().tagPocessing;
        switch (settings.preferredEncoding) {
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
        switch (settings.unsupportedFieldHandling) {
        case UnsupportedFieldHandling::Ignore:
            ui()->ignoreUnsupportedRadioButton->setChecked(true);
            break;
        case UnsupportedFieldHandling::Discard:
            ui()->discardUnsupportedRadioButton->setChecked(true);
            break;
        }
        ui()->autoTagManagementCheckBox->setChecked(settings.autoTagManagement);
        ui()->preserveModificationTimeCheckBox->setChecked(settings.preserveModificationTime);
        ui()->preserveMuxingAppCheckBox->setChecked(settings.preserveMuxingApp);
        ui()->preserveWritingAppCheckBox->setChecked(settings.preserveWritingApp);
    }
}

// Id3v1OptionPage
Id3v1OptionPage::Id3v1OptionPage(QWidget *parentWidget)
    : Id3v1OptionPageBase(parentWidget)
{
}

Id3v1OptionPage::~Id3v1OptionPage()
{
}

bool Id3v1OptionPage::apply()
{
    if (hasBeenShown()) {
        auto &settings = values().tagPocessing.creationSettings;
        if (ui()->alwaysCreateRadioButton->isChecked()) {
            settings.id3v1usage = TagUsage::Always;
        } else if (ui()->keepExistingRadioButton->isChecked()) {
            settings.id3v1usage = TagUsage::KeepExisting;
        } else if (ui()->removeExistingRadioButton->isChecked()) {
            settings.id3v1usage = TagUsage::Never;
        }
    }
    return true;
}

void Id3v1OptionPage::reset()
{
    if (hasBeenShown()) {
        const auto &settings = values().tagPocessing.creationSettings;
        switch (settings.id3v1usage) {
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
Id3v2OptionPage::Id3v2OptionPage(QWidget *parentWidget)
    : Id3v2OptionPageBase(parentWidget)
{
}

Id3v2OptionPage::~Id3v2OptionPage()
{
}

bool Id3v2OptionPage::apply()
{
    if (hasBeenShown()) {
        auto &settings = values().tagPocessing.creationSettings;
        if (ui()->alwaysCreateRadioButton->isChecked()) {
            settings.id3v2usage = TagUsage::Always;
        } else if (ui()->keepExistingRadioButton->isChecked()) {
            settings.id3v2usage = TagUsage::KeepExisting;
        } else if (ui()->removeExistingRadioButton->isChecked()) {
            settings.id3v2usage = TagUsage::Never;
        }
        if (ui()->version230radioButton->isChecked()) {
            settings.id3v2MajorVersion = 3;
        } else if (ui()->version240radioButton->isChecked()) {
            settings.id3v2MajorVersion = 4;
        } else if (ui()->version220radioButton->isChecked()) {
            settings.id3v2MajorVersion = 2;
        }
        settings.setFlag(TagCreationFlags::KeepExistingId3v2Version, ui()->keepExistingVersionCheckBox->isChecked());
        settings.setFlag(TagCreationFlags::MergeMultipleSuccessiveId3v2Tags, ui()->mergeRadioButton->isChecked());
    }
    return true;
}

void Id3v2OptionPage::reset()
{
    if (hasBeenShown()) {
        const auto &settings = values().tagPocessing.creationSettings;
        switch (settings.id3v2usage) {
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
        switch (settings.id3v2MajorVersion) {
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
        ui()->keepExistingVersionCheckBox->setChecked(settings.flags & TagCreationFlags::KeepExistingId3v2Version);
        if (settings.flags & TagCreationFlags::MergeMultipleSuccessiveId3v2Tags) {
            ui()->mergeRadioButton->setChecked(true);
        } else {
            ui()->keepSeparateRadioButton->setChecked(true);
        }
    }
}

// TagProcessingTargetsOptionPage
TagProcessingTargetsOptionPage::TagProcessingTargetsOptionPage(QWidget *parentWidget)
    : TagProcessingTargetsOptionPageBase(parentWidget)
    , m_model(nullptr)
{
}

TagProcessingTargetsOptionPage::~TagProcessingTargetsOptionPage()
{
}

bool TagProcessingTargetsOptionPage::apply()
{
    if (hasBeenShown() && m_model) {
        values().editor.defaultTargets.setItems(m_model->items());
    }
    return true;
}

void TagProcessingTargetsOptionPage::reset()
{
    if (hasBeenShown() && m_model) {
        m_model->setItems(values().editor.defaultTargets.items());
    }
}

QWidget *TagProcessingTargetsOptionPage::setupWidget()
{
    auto *w = TagProcessingTargetsOptionPageBase::setupWidget();
    if (!m_model) {
        m_model = new TargetLevelModel(w);
    }
    ui()->targetsToBeAddedListView->setModel(m_model);
    return w;
}

// FileLayoutPage
FileLayoutPage::FileLayoutPage(QWidget *parentWidget)
    : FileLayoutPageBase(parentWidget)
{
}

FileLayoutPage::~FileLayoutPage()
{
}

bool FileLayoutPage::apply()
{
    // error handling
    errors().clear();
    bool ok = true;

    if (hasBeenShown()) {
        auto &settings = values().tagPocessing.fileLayout;
        settings.forceRewrite = ui()->forceRewriteCheckBox->isChecked();
        if (ui()->minPaddingSpinBox->value() > ui()->maxPaddingSpinBox->value()) {
            errors() << QCoreApplication::translate("QtGui::FileLayoutPage", "Minimum padding must be less or equal than maximum padding.");
            ok = false;
        } else {
            settings.maxPadding = static_cast<size_t>(ui()->maxPaddingSpinBox->value());
            settings.minPadding = static_cast<size_t>(ui()->minPaddingSpinBox->value());
        }
        settings.preferredPadding = static_cast<size_t>(ui()->preferredPaddingSpinBox->value());
        if (ui()->tagPosBeforeDataRadioButton->isChecked()) {
            settings.preferredTagPosition = ElementPosition::BeforeData;
        } else if (ui()->tagPosAfterDataRadioButton->isChecked()) {
            settings.preferredTagPosition = ElementPosition::AfterData;
        } else if (ui()->tagPosKeepRadioButton->isChecked()) {
            settings.preferredTagPosition = ElementPosition::Keep;
        }
        settings.forceTagPosition = ui()->tagPosForceCheckBox->isChecked();
        if (ui()->indexPosBeforeDataRadioButton->isChecked()) {
            settings.preferredIndexPosition = ElementPosition::BeforeData;
        } else if (ui()->indexPosAfterDataRadioButton->isChecked()) {
            settings.preferredIndexPosition = ElementPosition::AfterData;
        } else if (ui()->indexPosKeepRadioButton->isChecked()) {
            settings.preferredIndexPosition = ElementPosition::Keep;
        }
        settings.forceIndexPosition = ui()->indexPosForceCheckBox->isChecked();
    }
    return ok;
}

void FileLayoutPage::reset()
{
    if (hasBeenShown()) {
        const auto &settings = values().tagPocessing.fileLayout;
        ui()->forceRewriteCheckBox->setChecked(settings.forceRewrite);
        ui()->maxPaddingSpinBox->setValue(static_cast<int>(settings.maxPadding));
        ui()->minPaddingSpinBox->setValue(static_cast<int>(settings.minPadding));
        ui()->preferredPaddingSpinBox->setValue(static_cast<int>(settings.preferredPadding));
        switch (settings.preferredTagPosition) {
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
        ui()->tagPosForceCheckBox->setChecked(settings.forceTagPosition);
        switch (settings.preferredIndexPosition) {
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
        ui()->indexPosForceCheckBox->setChecked(settings.forceIndexPosition);
    }
}

QWidget *FileLayoutPage::setupWidget()
{
    auto *widget = FileLayoutPageBase::setupWidget();
    ui()->preferredTagPosLabel->setNotificationType(NotificationType::Warning);
    ui()->preferredTagPosLabel->setText(QApplication::translate(
        "QtGui::FileLayoutPage", "These options might be ignored if not supported by either the format or the implementation."));
    QObject::connect(
        ui()->minPaddingSpinBox, static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged), ui()->maxPaddingSpinBox, &QSpinBox::setMinimum);
    QObject::connect(
        ui()->minPaddingSpinBox, static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged), ui()->preferredPaddingSpinBox, &QSpinBox::setMinimum);
    QObject::connect(
        ui()->maxPaddingSpinBox, static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged), ui()->minPaddingSpinBox, &QSpinBox::setMaximum);
    QObject::connect(
        ui()->maxPaddingSpinBox, static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged), ui()->preferredPaddingSpinBox, &QSpinBox::setMaximum);
    return widget;
}

SettingsDialog::SettingsDialog(QWidget *parent)
    : QtUtilities::SettingsDialog(parent)
{
    // setup categories
    QList<OptionCategory *> categories;
    OptionCategory *category;

    category = new OptionCategory(this);
    category->setDisplayName(tr("Tag processing"));
    category->assignPages(
        { new TagProcessingGeneralOptionPage, new Id3v1OptionPage, new Id3v2OptionPage, new TagProcessingTargetsOptionPage, new FileLayoutPage });
    category->setIcon(QIcon::fromTheme(QStringLiteral("tag"), QIcon(QStringLiteral(":/tageditor/icons/hicolor/32x32/settingscategories/tag.svg"))));
    categories << category;

    category = new OptionCategory(this);
    category->setDisplayName(tr("Editor"));
    category->setIcon(QIcon::fromTheme(
        QStringLiteral("document-edit"), QIcon(QStringLiteral(":/tageditor/icons/hicolor/32x32/settingscategories/document-edit.svg"))));
    category->assignPages({ new EditorGeneralOptionPage, new EditorTempOptionPage(this), new EditorFieldsOptionPage, new InfoOptionPage,
        new EditorAutoCorrectionOptionPage, new EditorDbQueryOptionsPage });
    categories << category;

    category = new OptionCategory(this);
    category->setDisplayName(tr("File browser"));
    category->setIcon(QIcon::fromTheme(
        QStringLiteral("view-list-tree"), QIcon(QStringLiteral(":/tageditor/icons/hicolor/32x32/settingscategories/view-list-tree.svg"))));
    category->assignPages({ new FileBrowserGeneralOptionPage });
    categories << category;

    categories << values().qt.category();

    categoryModel()->setCategories(categories);

    setMinimumSize(800, 450);
    setWindowIcon(QIcon::fromTheme(
        QStringLiteral("preferences-other"), QIcon(QStringLiteral(":/tageditor/icons/hicolor/32x32/settingscategories/preferences-other.svg"))));

    connect(this, &SettingsDialog::applied, std::bind(&QtSettings::apply, &::Settings::values().qt));
}

SettingsDialog::~SettingsDialog()
{
}

} // namespace QtGui

INSTANTIATE_UI_FILE_BASED_OPTION_PAGE_NS(QtGui, FileBrowserGeneralOptionPage)
INSTANTIATE_UI_FILE_BASED_OPTION_PAGE_NS(QtGui, EditorGeneralOptionPage)
INSTANTIATE_UI_FILE_BASED_OPTION_PAGE_NS(QtGui, EditorTempOptionPage)
INSTANTIATE_UI_FILE_BASED_OPTION_PAGE_NS(QtGui, EditorFieldsOptionPage)
INSTANTIATE_UI_FILE_BASED_OPTION_PAGE_NS(QtGui, EditorAutoCorrectionOptionPage)
INSTANTIATE_UI_FILE_BASED_OPTION_PAGE_NS(QtGui, EditorDbQueryOptionsPage)
INSTANTIATE_UI_FILE_BASED_OPTION_PAGE_NS(QtGui, InfoOptionPage)
INSTANTIATE_UI_FILE_BASED_OPTION_PAGE_NS(QtGui, TagProcessingGeneralOptionPage)
INSTANTIATE_UI_FILE_BASED_OPTION_PAGE_NS(QtGui, Id3v1OptionPage)
INSTANTIATE_UI_FILE_BASED_OPTION_PAGE_NS(QtGui, Id3v2OptionPage)
INSTANTIATE_UI_FILE_BASED_OPTION_PAGE_NS(QtGui, TagProcessingTargetsOptionPage)
INSTANTIATE_UI_FILE_BASED_OPTION_PAGE_NS(QtGui, FileLayoutPage)
