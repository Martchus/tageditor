#ifndef SETTINGS_H
#define SETTINGS_H

#include <c++utilities/conversion/types.h>

#include <tagparser/tagvalue.h>

#include <QtGlobal>

QT_BEGIN_NAMESPACE
class QByteArray;
class QString;
QT_END_NAMESPACE

namespace Media {
enum class TagUsage;
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
uint32 &id3v2versionToBeUsed();
bool &keepVersionOfExistingId3v2Tag();
bool &mergeMultipleSuccessiveId3v2Tags();

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

void restore();
void save();

}

#endif // SETTINGS_H
