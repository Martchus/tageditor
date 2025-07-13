#ifndef SETTINGS_H
#define SETTINGS_H

#include "./knownfieldmodel.h"
#include "./targetlevelmodel.h"

#include <tagparser/abstractcontainer.h>
#include <tagparser/settings.h>
#include <tagparser/tag.h>
#include <tagparser/tagvalue.h>

#include <qtutilities/settingsdialog/qtsettings.h>

#include <QByteArray>
#include <QRegularExpression>
#include <QString>

namespace Settings {

enum class ActionEnabled { Ask, Yes, No };

enum class AdoptFields { Never, WithinDirectory, Always };

enum class MultipleTagHandling { SingleEditorPerTarget, SeparateEditors };

struct CustomSubstitution {
    QRegularExpression regex;
    QString replacement;
    bool enabled = false;
};

struct AutoCompletition {
    AutoCompletition();
    bool insertTitleFromFilename = false;
    bool trimWhitespaces = true;
    bool formatNames = false;
    bool fixUmlauts = false;
    CustomSubstitution customSubstitution;
    KnownFieldModel fields;
};

struct Editor {
    Editor();
    std::string backupDirectory;
    AdoptFields adoptFields = AdoptFields::Never;
    bool saveAndShowNextOnEnter = false;
    bool askBeforeDeleting = true;
    MultipleTagHandling multipleTagHandling = MultipleTagHandling::SingleEditorPerTarget;
    bool hideTagSelectionComboBox = false;
    bool forceFullParse = false;
#ifndef TAGEDITOR_NO_WEBVIEW
    bool noWebView = false;
#endif
    bool hideCoverButtons = false;
    AutoCompletition autoCompletition;
    KnownFieldModel fields;
    TargetLevelModel defaultTargets;
};

enum class UnsupportedFieldHandling { Ignore, Discard };

struct FileBrowser {
    bool hideBackupFiles = true;
    bool readOnly = true;
};

struct FileLayout {
    bool forceRewrite = true;
    TagParser::ElementPosition preferredTagPosition = TagParser::ElementPosition::BeforeData;
    bool forceTagPosition = true;
    TagParser::ElementPosition preferredIndexPosition = TagParser::ElementPosition::BeforeData;
    bool forceIndexPosition = true;
    std::size_t minPadding = 0;
    std::size_t maxPadding = 0;
    std::size_t preferredPadding = 0;
};

struct TagProcessing {
    TagParser::TagTextEncoding preferredEncoding = TagParser::TagTextEncoding::Utf8;
    UnsupportedFieldHandling unsupportedFieldHandling = UnsupportedFieldHandling::Ignore;
    bool autoTagManagement = true;
    bool preserveModificationTime = false;
    bool preserveMuxingApp = false;
    bool preserveWritingApp = false;
    bool convertTotalFields = true;
    TagParser::TagCreationSettings creationSettings;
    FileLayout fileLayout;
};

struct MainWindow {
    QByteArray geometry;
    QByteArray state;
    QString currentFileBrowserDirectory;
    bool layoutLocked = false;
};

struct DbQuery {
    DbQuery();
    bool widgetShown = false;
    bool override = false;
    KnownFieldModel fields;
    QString musicBrainzUrl;
    QString coverArtArchiveUrl;
    QString lyricsWikiaUrl;
    QString makeItPersonalUrl;
    QString tekstowoUrl;
};

struct RenamingUtility {
    int scriptSource = 0;
    QString externalScript;
    QString editorScript;
};

struct Settings {
    QString error;
    Editor editor;
    FileBrowser fileBrowser;
    TagProcessing tagPocessing;
    MainWindow mainWindow;
    DbQuery dbQuery;
    RenamingUtility renamingUtility;
    QtUtilities::QtSettings qt;
};

Settings &values();
void restore(QSettings &settings);
void save();

} // namespace Settings

#endif // SETTINGS_H
