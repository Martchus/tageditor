#include "./settings.h"
#include "./knownfieldmodel.h"

#include <tagparser/mediafileinfo.h>
#include <tagparser/tag.h>
#include <tagparser/backuphelper.h>

#include <QString>
#include <QByteArray>
#include <QApplication>
#include <QSettings>

using namespace Media;

namespace Settings {

// editor
AdoptFields &adoptFields()
{
    static AdoptFields v = AdoptFields::Never;
    return v;
}
bool &saveAndShowNextOnEnter()
{
    static bool v = false;
    return v;
}
bool &askBeforeDeleting()
{
    static bool v = true;
    return v;
}
MultipleTagHandling &multipleTagHandling()
{
    static MultipleTagHandling v = MultipleTagHandling::SingleEditorPerTarget;
    return v;
}
bool &hideTagSelectionComboBox()
{
    static bool v = true;
    return v;
}
bool &forceFullParse()
{
    static bool v = false;
    return v;
}

// file browser
bool &hideBackupFiles()
{
    static bool v = true;
    return v;
}
bool &fileBrowserReadOnly()
{
    static bool v = true;
    return v;
}

// general tag processing
Media::TagTextEncoding &preferredEncoding()
{
    static Media::TagTextEncoding v = Media::TagTextEncoding::Utf8;
    return v;
}

UnsupportedFieldHandling &unsupportedFieldHandling()
{
    static UnsupportedFieldHandling v = UnsupportedFieldHandling::Ignore;
    return v;
}

bool &autoTagManagement()
{
    static bool v = true;
    return v;
}

// ID3 tag processing
TagUsage &id3v1usage()
{
    static TagUsage v = TagUsage::Always;
    return v;
}

TagUsage &id3v2usage()
{
    static TagUsage v = TagUsage::Always;
    return v;
}

uint32 &id3v2versionToBeUsed()
{
    static uint32 v = 3;
    return v;
}

bool &keepVersionOfExistingId3v2Tag()
{
    static bool v = true;
    return v;
}

bool &mergeMultipleSuccessiveId3v2Tags()
{
    static bool v = true;
    return v;
}

// file layout
bool &forceRewrite()
{
    static bool v = true;
    return v;
}

ElementPosition &preferredTagPosition()
{
    static ElementPosition v = ElementPosition::BeforeData;
    return v;
}

bool &forceTagPosition()
{
    static bool v = true;
    return v;
}

ElementPosition &preferredIndexPosition()
{
    static ElementPosition v = ElementPosition::BeforeData;
    return v;
}

bool &forceIndexPosition()
{
    static bool v = true;
    return v;
}

size_t &minPadding()
{
    static size_t v = 0;
    return v;
}

size_t &maxPadding()
{
    static size_t v = 0;
    return v;
}

size_t &preferredPadding()
{
    static size_t v = 0;
    return v;
}

// fields
KnownFieldModel &selectedFieldsModel()
{
    static KnownFieldModel model(nullptr, KnownFieldModel::DefaultSelection::CommonFields);
    return model;
}

// auto correction/completition
bool &insertTitleFromFilename()
{
    static bool v = false;
    return v;
}

bool &trimWhitespaces()
{
    static bool v = true;
    return v;
}

bool &formatNames()
{
    static bool v = false;
    return v;
}

bool &fixUmlauts()
{
    static bool v = false;
    return v;
}

KnownFieldModel &autoCorrectionFields()
{
    static KnownFieldModel model(nullptr, KnownFieldModel::DefaultSelection::None);
    return model;
}

// main window
QByteArray &mainWindowGeometry()
{
    static QByteArray v;
    return v;
}

QByteArray &mainWindowState()
{
    static QByteArray v;
    return v;
}

QString &mainWindowCurrentFileBrowserDirectory()
{
    static QString v;
    return v;
}

// db query
bool &dbQueryWidgetShown()
{
    static bool v;
    return v;
}

bool &dbQueryOverride()
{
    static bool v;
    return v;
}

KnownFieldModel &dbQueryFields()
{
    static KnownFieldModel v(QList<Models::ChecklistItem>()
                             << KnownFieldModel::mkItem(KnownField::Title)
                             << KnownFieldModel::mkItem(KnownField::TrackPosition)
                             << KnownFieldModel::mkItem(KnownField::DiskPosition)
                             << KnownFieldModel::mkItem(KnownField::Album)
                             << KnownFieldModel::mkItem(KnownField::Album)
                             << KnownFieldModel::mkItem(KnownField::Year)
                             << KnownFieldModel::mkItem(KnownField::Genre));
    return v;
}

QString &musicBrainzUrl()
{
    static QString v;
    return v;
}

// renaming files dialog
int &scriptSource()
{
    static int v = 0;
    return v;
}

QString &externalScript()
{
    static QString v;
    return v;
}

void restore()
{
    QSettings settings(QSettings::IniFormat, QSettings::UserScope,  QApplication::organizationName(), QApplication::applicationName());

    settings.beginGroup(QStringLiteral("editor"));
    switch(settings.value(QStringLiteral("adoptfields"), 0).toInt()) {
    case 1:
        adoptFields() = AdoptFields::WithinDirectory;
        break;
    case 2:
        adoptFields() = AdoptFields::Always;
        break;
    default:
        adoptFields() = AdoptFields::Never;
        break;
    };
    saveAndShowNextOnEnter() = settings.value(QStringLiteral("saveandshownextonenter"), false).toBool();
    askBeforeDeleting() = settings.value(QStringLiteral("askbeforedeleting"), true).toBool();
    switch(settings.value(QStringLiteral("multipletaghandling"), 0).toInt()) {
    case 0:
        multipleTagHandling() = MultipleTagHandling::SingleEditorPerTarget;
        break;
    case 1:
        multipleTagHandling() = MultipleTagHandling::SeparateEditors;
        break;
    }
    hideTagSelectionComboBox() = settings.value(QStringLiteral("hidetagselectioncombobox"), true).toBool();
    settings.beginGroup(QStringLiteral("autocorrection"));
    insertTitleFromFilename() = settings.value(QStringLiteral("inserttitlefromfilename"), false).toBool();
    trimWhitespaces() = settings.value(QStringLiteral("trimwhitespaces"), true).toBool();
    formatNames() = settings.value(QStringLiteral("formatnames"), false).toBool();
    fixUmlauts() = settings.value(QStringLiteral("fixumlauts"), false).toBool();
    settings.endGroup();
    BackupHelper::backupDirectory() = settings.value(QStringLiteral("tempdir")).toString().toStdString();
    settings.endGroup();

    selectedFieldsModel().restore(settings, QStringLiteral("selectedfields"));

    autoCorrectionFields().restore(settings, QStringLiteral("autocorrectionfields"));

    settings.beginGroup(QStringLiteral("info"));
    Settings::forceFullParse() = settings.value(QStringLiteral("forcefullparse"), false).toBool();
    settings.endGroup();

    settings.beginGroup(QStringLiteral("filebrowser"));
    hideBackupFiles() = settings.value(QStringLiteral("hidebackupfiles"), true).toBool();
    fileBrowserReadOnly() = settings.value(QStringLiteral("readonly"), true).toBool();
    settings.endGroup();

    settings.beginGroup(QStringLiteral("tagprocessing"));
    switch(settings.value(QStringLiteral("preferredencoding"), 1).toInt()) {
    case 0:
        preferredEncoding() = Media::TagTextEncoding::Latin1;
        break;
    case 2:
        preferredEncoding() = Media::TagTextEncoding::Utf16BigEndian;
        break;
    case 3:
        preferredEncoding() = Media::TagTextEncoding::Utf16LittleEndian;
        break;
    default:
        preferredEncoding() = Media::TagTextEncoding::Utf8;
    };
    switch(settings.value(QStringLiteral("unsupportedfieldhandling"), 0).toInt()) {
    case 1:
        unsupportedFieldHandling() = UnsupportedFieldHandling::Discard;
        break;
    default:
        unsupportedFieldHandling() = UnsupportedFieldHandling::Ignore;
    };
    autoTagManagement() = settings.value(QStringLiteral("autotagmanagement"), true).toBool();
    settings.beginGroup(QStringLiteral("id3v1"));
    switch(settings.value(QStringLiteral("usage"), 0).toInt()) {
    case 1:
        id3v1usage() = TagUsage::KeepExisting;
        break;
    case 2:
        id3v1usage() = TagUsage::Never;
        break;
    default:
        id3v1usage() = TagUsage::Always;
        break;
    };
    settings.endGroup();
    settings.beginGroup(QStringLiteral("id3v2"));
    switch(settings.value(QStringLiteral("usage"), 0).toInt()) {
    case 1:
        id3v2usage() = TagUsage::KeepExisting;
        break;
    case 2:
        id3v2usage() = TagUsage::Never;
        break;
    default:
        id3v2usage() = TagUsage::Always;
    };
    id3v2versionToBeUsed() = settings.value(QStringLiteral("versiontobeused"), 3).toUInt();
    keepVersionOfExistingId3v2Tag() = settings.value(QStringLiteral("keepversionofexistingtag"), true).toBool();
    mergeMultipleSuccessiveId3v2Tags() = settings.value(QStringLiteral("mergemultiplesuccessivetags"), true).toBool();
    settings.endGroup();
    settings.beginGroup(QStringLiteral("filelayout"));
    forceRewrite() = settings.value(QStringLiteral("forcerewrite"), true).toBool();
    switch(settings.value(QStringLiteral("tagpos")).toInt()) {
    case 0:
        preferredTagPosition() = ElementPosition::BeforeData;
        break;
    case 1:
        preferredTagPosition() = ElementPosition::AfterData;
        break;
    }
    forceTagPosition() = settings.value(QStringLiteral("forcetagpos"), true).toBool();
    switch(settings.value(QStringLiteral("indexpos")).toInt()) {
    case 0:
        preferredIndexPosition() = ElementPosition::BeforeData;
        break;
    case 1:
        preferredIndexPosition() = ElementPosition::AfterData;
        break;
    }
    forceIndexPosition() = settings.value(QStringLiteral("forceindexpos"), true).toBool();
    minPadding() = settings.value(QStringLiteral("minpad"), 0).toInt();
    maxPadding() = settings.value(QStringLiteral("maxpad"), 0).toInt();
    preferredPadding() = settings.value(QStringLiteral("prefpad"), 0).toInt();
    settings.endGroup();
    settings.endGroup();

    settings.beginGroup(QStringLiteral("mainwindow"));
    mainWindowGeometry() = settings.value(QStringLiteral("geometry")).toByteArray();
    mainWindowState() = settings.value(QStringLiteral("windowstate")).toByteArray();
    mainWindowCurrentFileBrowserDirectory() = settings.value(QStringLiteral("currentfilebrowserdirectory")).toString();
    settings.endGroup();

    settings.beginGroup(QStringLiteral("dbquery"));
    dbQueryWidgetShown() = settings.value(QStringLiteral("visible"), false).toBool();
    dbQueryOverride() = settings.value(QStringLiteral("override"), true).toBool();
    dbQueryFields().restore(settings, QStringLiteral("fields"));
    musicBrainzUrl() = settings.value(QStringLiteral("musicbrainzurl")).toString();
    settings.endGroup();

    settings.beginGroup(QStringLiteral("renamedlg"));
    scriptSource() = settings.value(QStringLiteral("src")).toInt();
    externalScript() = settings.value(QStringLiteral("file")).toString();
    settings.endGroup();
}

void save()
{
    QSettings settings(QSettings::IniFormat, QSettings::UserScope,  QApplication::organizationName(), QApplication::applicationName());

    settings.beginGroup(QStringLiteral("editor"));
    settings.setValue(QStringLiteral("adoptfields"), static_cast<int>(adoptFields()));
    settings.setValue(QStringLiteral("saveandshownextonenter"), saveAndShowNextOnEnter());
    settings.setValue(QStringLiteral("askbeforedeleting"), askBeforeDeleting());
    settings.setValue(QStringLiteral("multipletaghandling"), static_cast<int>(multipleTagHandling()));
    settings.setValue(QStringLiteral("hidetagselectioncombobox"), hideTagSelectionComboBox());
    settings.beginGroup(QStringLiteral("autocorrection"));
    settings.setValue(QStringLiteral("inserttitlefromfilename"), Settings::insertTitleFromFilename());
    settings.setValue(QStringLiteral("trimwhitespaces"), Settings::trimWhitespaces());
    settings.setValue(QStringLiteral("formatnames"), Settings::formatNames());
    settings.setValue(QStringLiteral("fixumlauts"), Settings::fixUmlauts());
    settings.endGroup();
    settings.setValue(QStringLiteral("tempdir"), QString::fromStdString(BackupHelper::backupDirectory()));
    settings.endGroup();

    selectedFieldsModel().save(settings, QStringLiteral("selectedfields"));

    autoCorrectionFields().save(settings, QStringLiteral("autocorrectionfields"));

    settings.beginGroup(QStringLiteral("info"));
    settings.setValue(QStringLiteral("forcefullparse"), Settings::forceFullParse());
    settings.endGroup();

    settings.beginGroup(QStringLiteral("filebrowser"));
    settings.setValue(QStringLiteral("hidebackupfiles"), hideBackupFiles());
    settings.setValue(QStringLiteral("readonly"), fileBrowserReadOnly());
    settings.endGroup();

    settings.beginGroup(QStringLiteral("tagprocessing"));
    settings.setValue(QStringLiteral("preferredencoding"), static_cast<int>(preferredEncoding()));
    settings.setValue(QStringLiteral("unsupportedfieldhandling"), static_cast<int>(unsupportedFieldHandling()));
    settings.setValue(QStringLiteral("autotagmanagement"), autoTagManagement());
    settings.beginGroup(QStringLiteral("id3v1"));
    settings.setValue(QStringLiteral("usage"), static_cast<int>(id3v1usage()));
    settings.endGroup();
    settings.beginGroup(QStringLiteral("id3v2"));
    settings.setValue(QStringLiteral("usage"), static_cast<int>(id3v2usage()));
    settings.setValue(QStringLiteral("versiontobeused"), id3v2versionToBeUsed());
    settings.setValue(QStringLiteral("keepversionofexistingtag"), keepVersionOfExistingId3v2Tag());
    settings.setValue(QStringLiteral("mergemultiplesuccessivetags"), mergeMultipleSuccessiveId3v2Tags());
    settings.endGroup();
    settings.beginGroup(QStringLiteral("filelayout"));
    settings.setValue(QStringLiteral("forcerewrite"), forceRewrite());
    settings.setValue(QStringLiteral("tagpos"), static_cast<int>(preferredTagPosition()));
    settings.setValue(QStringLiteral("forcetagpos"), forceTagPosition());
    settings.setValue(QStringLiteral("indexpos"), static_cast<int>(preferredIndexPosition()));
    settings.setValue(QStringLiteral("forceindexpos"), forceIndexPosition());
    settings.setValue(QStringLiteral("minpad"), QVariant::fromValue(minPadding()));
    settings.setValue(QStringLiteral("maxpad"), QVariant::fromValue(maxPadding()));
    settings.setValue(QStringLiteral("prefpad"), QVariant::fromValue(preferredPadding()));
    settings.endGroup();
    settings.endGroup();

    settings.beginGroup(QStringLiteral("mainwindow"));
    settings.setValue(QStringLiteral("geometry"), mainWindowGeometry());
    settings.setValue(QStringLiteral("windowstate"), mainWindowState());
    settings.setValue(QStringLiteral("currentfilebrowserdirectory"), mainWindowCurrentFileBrowserDirectory());
    settings.endGroup();

    settings.beginGroup(QStringLiteral("dbquery"));
    settings.setValue(QStringLiteral("visible"), dbQueryWidgetShown());
    settings.setValue(QStringLiteral("override"), dbQueryOverride());
    dbQueryFields().save(settings, QStringLiteral("fields"));
    settings.setValue(QStringLiteral("musicbrainzurl"), musicBrainzUrl());
    settings.endGroup();

    settings.beginGroup(QStringLiteral("renamedlg"));
    settings.setValue(QStringLiteral("src"), Settings::scriptSource());
    settings.setValue(QStringLiteral("file"), Settings::externalScript());
    settings.endGroup();
}

}
