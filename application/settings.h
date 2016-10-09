#ifndef SETTINGS_H
#define SETTINGS_H

#include <c++utilities/conversion/types.h>

#include <tagparser/tagvalue.h>

#include <QtGlobal>

QT_FORWARD_DECLARE_CLASS(QByteArray)
QT_FORWARD_DECLARE_CLASS(QString)

namespace Media {
enum class TagUsage;
enum class ElementPosition;
}

namespace Dialogs {
class QtSettings;
}

namespace Settings {

// general
enum class ActionEnabled
{
    Ask,
    Yes,
    No
};

// editor
enum class AdoptFields
{
    Never,
    WithinDirectory,
    Always
};
AdoptFields &adoptFields();
bool &saveAndShowNextOnEnter();
bool &askBeforeDeleting();
enum class MultipleTagHandling
{
    SingleEditorPerTarget,
    SeparateEditors
};
MultipleTagHandling &multipleTagHandling();
bool &hideTagSelectionComboBox();
bool &forceFullParse();
#ifndef TAGEDITOR_NO_WEBVIEW
bool &noWebView();
#endif
bool &hideCoverButtons();

// file browser
bool &hideBackupFiles();
bool &fileBrowserReadOnly();

// general tag processing
Media::TagTextEncoding &preferredEncoding();
enum class UnsupportedFieldHandling
{
    Ignore,
    Discard
};
UnsupportedFieldHandling &unsupportedFieldHandling();
bool &autoTagManagement();

// ID3 tag processing
Media::TagUsage &id3v1usage();
Media::TagUsage &id3v2usage();
byte &id3v2versionToBeUsed();
bool &keepVersionOfExistingId3v2Tag();
bool &mergeMultipleSuccessiveId3v2Tags();

// file layout
bool &forceRewrite();
Media::ElementPosition &preferredTagPosition();
bool &forceTagPosition();
Media::ElementPosition &preferredIndexPosition();
bool &forceIndexPosition();
size_t &minPadding();
size_t &maxPadding();
size_t &preferredPadding();

// targets
class TargetLevelModel;
TargetLevelModel &defaultTargetsModel();

// fields
class KnownFieldModel;
KnownFieldModel &selectedFieldsModel();

// auto correction/completition
bool &insertTitleFromFilename();
bool &trimWhitespaces();
bool &formatNames();
bool &fixUmlauts();
KnownFieldModel &autoCorrectionFields();

// main window
QByteArray &mainWindowGeometry();
QByteArray &mainWindowState();
QString &mainWindowCurrentFileBrowserDirectory();
bool &mainWindowLayoutLocked();

// db query
bool &dbQueryWidgetShown();
bool &dbQueryOverride();
KnownFieldModel &dbQueryFields();
QString &musicBrainzUrl();
QString &coverArtArchiveUrl();
QString &lyricsWikiaUrl();

// rename files dialog
int &scriptSource();
QString &externalScript();
QString &editorScript();

// Qt settings
Dialogs::QtSettings &qtSettings();

void restore();
void save();

}

#endif // SETTINGS_H
