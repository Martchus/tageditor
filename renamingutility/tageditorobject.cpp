#ifndef TAGEDITOR_NO_JSENGINE

#include "./tageditorobject.h"
#include "./filesystemitem.h"
#include "./jsincludes.h"

#include "../misc/utility.h"

#include <tagparser/abstracttrack.h>
#include <tagparser/exceptions.h>
#include <tagparser/mediafileinfo.h>
#include <tagparser/tag.h>
#include <tagparser/tagvalue.h>

#include <qtutilities/misc/conversion.h>

#include <c++utilities/conversion/conversionexception.h>
#include <c++utilities/io/catchiofailure.h>

#include <QDir>

#include <iostream>

using namespace ConversionUtilities;
using namespace Utility;
using namespace TagParser;
using namespace std;

namespace RenamingUtility {

/// \brief Adds notifications from \a statusProvider to \a notificationsObject.
TAGEDITOR_JS_VALUE &operator<<(TAGEDITOR_JS_VALUE &diagObject, const Diagnostics &diag)
{
    quint32 counter = 0;
    for (const auto &msg : diag) {
        TAGEDITOR_JS_VALUE val;
        val.setProperty("msg", QString::fromUtf8(msg.message().data()) TAGEDITOR_JS_READONLY);
        val.setProperty("critical", msg.level() >= DiagLevel::Critical TAGEDITOR_JS_READONLY);
        diagObject.setProperty(counter, val);
        ++counter;
    }
    return diagObject;
}

/// \brief Add fields and notifications from \a tag to \a tagObject.
TAGEDITOR_JS_VALUE &operator<<(TAGEDITOR_JS_VALUE &tagObject, const Tag &tag)
{
    // add text fields
    static const char *const fieldNames[]
        = { "title", "artist", "albumartist", "album", "year", "comment", "genre", "encoder", "language", "description", nullptr };
    static const KnownField fields[] = { KnownField::Title, KnownField::Artist, KnownField::AlbumArtist, KnownField::Album, KnownField::Year,
        KnownField::Comment, KnownField::Genre, KnownField::Encoder, KnownField::Language, KnownField::Description };
    const char *const *fieldName = fieldNames;
    const KnownField *field = fields;
    for (; *fieldName; ++fieldName, ++field) {
        try {
            tagObject.setProperty(*fieldName, tagValueToQString(tag.value(*field)) TAGEDITOR_JS_READONLY);
        } catch (const ConversionException &) {
        }
    }

    // add numeric fields
    try {
        tagObject.setProperty("partNumber", tag.value(KnownField::PartNumber).toInteger() TAGEDITOR_JS_READONLY);
    } catch (const ConversionException &) {
    }
    try {
        tagObject.setProperty("totalParts", tag.value(KnownField::TotalParts).toInteger() TAGEDITOR_JS_READONLY);
    } catch (const ConversionException &) {
    }
    PositionInSet pos;
    try {
        pos = tag.value(KnownField::TrackPosition).toPositionInSet();
    } catch (const ConversionException &) {
    }
    tagObject.setProperty("trackPos", pos.position() TAGEDITOR_JS_READONLY);
    tagObject.setProperty("trackTotal", pos.total() TAGEDITOR_JS_READONLY);
    pos = PositionInSet();
    try {
        pos = tag.value(KnownField::DiskPosition).toPositionInSet();
    } catch (const ConversionException &) {
    }
    tagObject.setProperty("diskPos", pos.position() TAGEDITOR_JS_READONLY);
    tagObject.setProperty("diskTotal", pos.total() TAGEDITOR_JS_READONLY);

    return tagObject;
}

TagEditorObject::TagEditorObject(TAGEDITOR_JS_ENGINE *engine)
    : m_engine(engine)
    , m_currentType(ItemType::Dir)
    , m_action(ActionType::None)
{
}

void TagEditorObject::setFileInfo(const QFileInfo &file, FileSystemItem *item)
{
    m_currentPath = file.absoluteFilePath();
    m_currentName = file.fileName();
    m_currentRelativeDirectory = item->relativeDir();
    m_currentType = item->type();
    m_action = ActionType::None;
    m_newName.clear();
    m_newRelativeDirectory.clear();
}

const QString &TagEditorObject::currentPath() const
{
    return m_currentPath;
}

const QString &TagEditorObject::currentName() const
{
    return m_currentName;
}

const QString &TagEditorObject::currentRelativeDirectory() const
{
    return m_currentRelativeDirectory;
}

bool TagEditorObject::isDir() const
{
    return m_currentType == ItemType::Dir;
}

bool TagEditorObject::isFile() const
{
    return m_currentType == ItemType::File;
}

const QString &TagEditorObject::newName() const
{
    return m_newName;
}

const QString &TagEditorObject::newRelativeDirectory() const
{
    return m_newRelativeDirectory;
}

TAGEDITOR_JS_VALUE TagEditorObject::parseFileInfo(const QString &fileName)
{
    Diagnostics diag;
    MediaFileInfo fileInfo(toNativeFileName(fileName).data());

    // add basic file information
    auto fileInfoObject = m_engine->newObject();
    fileInfoObject.setProperty(QStringLiteral("currentPath"), QString::fromUtf8(fileInfo.path().data()));
    fileInfoObject.setProperty(QStringLiteral("currentPathWithoutExtension"), QString::fromUtf8(fileInfo.pathWithoutExtension().data()));
    fileInfoObject.setProperty(QStringLiteral("currentName"), QString::fromUtf8(fileInfo.fileName(false).data()));
    fileInfoObject.setProperty(QStringLiteral("currentBaseName"), QString::fromUtf8(fileInfo.fileName(true).data()));
    QString suffix = fromNativeFileName(fileInfo.extension().data());
    if (suffix.startsWith('.')) {
        suffix.remove(0, 1);
    }
    fileInfoObject.setProperty(QStringLiteral("currentSuffix"), suffix TAGEDITOR_JS_READONLY);

    // parse further file information
    bool criticalParseingErrorOccured = false, ioErrorOccured = false;
    try {
        fileInfo.parseEverything(diag);
    } catch (const Failure &) {
        // parsing notifications will be addded anyways
        criticalParseingErrorOccured = true;
    } catch (...) {
        ::IoUtilities::catchIoFailure();
        criticalParseingErrorOccured = true;
        ioErrorOccured = true;
    }

    // gather notifications
    auto diagObj = m_engine->newArray(static_cast<uint>(diag.size()));
    diagObj << diag;
    criticalParseingErrorOccured |= diag.level() >= DiagLevel::Critical;
    fileInfoObject.setProperty(QStringLiteral("hasCriticalMessages"), criticalParseingErrorOccured);
    fileInfoObject.setProperty(QStringLiteral("ioErrorOccured"), ioErrorOccured);
    fileInfoObject.setProperty(QStringLiteral("diagMessages"), diagObj);

    // add MIME-type, suitable suffix and technical summary
    fileInfoObject.setProperty(QStringLiteral("mimeType"), QString::fromUtf8(fileInfo.mimeType()) TAGEDITOR_JS_READONLY);
    fileInfoObject.setProperty(QStringLiteral("suitableSuffix"), QString::fromUtf8(fileInfo.containerFormatAbbreviation()) TAGEDITOR_JS_READONLY);
    fileInfoObject.setProperty(QStringLiteral("technicalSummary"), QString::fromUtf8(fileInfo.technicalSummary().data()) TAGEDITOR_JS_READONLY);
    fileInfoObject.setProperty(QStringLiteral("hasAudioTracks"), fileInfo.hasTracksOfType(MediaType::Audio) TAGEDITOR_JS_READONLY);
    fileInfoObject.setProperty(QStringLiteral("hasVideoTracks"), fileInfo.hasTracksOfType(MediaType::Video) TAGEDITOR_JS_READONLY);

    // gather tag information
    const vector<Tag *> tags = fileInfo.tags();
    auto combinedTagObject = m_engine->newObject();
    auto combinedTagNotifications = m_engine->newArray();
    auto tagsObject = m_engine->newArray(static_cast<uint>(tags.size()));
    uint32 tagIndex = 0;
    for (auto tagIterator = tags.cbegin(), end = tags.cend(); tagIterator != end; ++tagIterator, ++tagIndex) {
        const Tag &tag = **tagIterator;
        auto tagObject = m_engine->newObject();
        combinedTagObject << tag;
        combinedTagNotifications << tag;
        tagObject << tag;
        tagsObject.setProperty(tagIndex, tagObject TAGEDITOR_JS_READONLY);
    }
    combinedTagObject.setProperty(QStringLiteral("notifications"), combinedTagNotifications TAGEDITOR_JS_READONLY);
    fileInfoObject.setProperty(QStringLiteral("tag"), combinedTagObject TAGEDITOR_JS_READONLY);
    fileInfoObject.setProperty(QStringLiteral("tags"), tagsObject TAGEDITOR_JS_READONLY);

    // gather track information
    const vector<AbstractTrack *> tracks = fileInfo.tracks();
    auto tracksObject = m_engine->newArray(static_cast<uint>(tracks.size()));
    uint32 trackIndex = 0;
    for (auto trackIterator = tracks.cbegin(), end = tracks.cend(); trackIterator != end; ++trackIterator, ++trackIndex) {
        const AbstractTrack &track = **trackIterator;
        auto trackObject = m_engine->newObject();
        trackObject.setProperty(QStringLiteral("mediaType"), QString::fromUtf8(track.mediaTypeName()));
        trackObject.setProperty(QStringLiteral("format"), QString::fromUtf8(track.formatName()));
        trackObject.setProperty(QStringLiteral("formatAbbreviation"), QString::fromUtf8(track.formatAbbreviation()));
        trackObject.setProperty(QStringLiteral("description"), QString::fromUtf8(track.description().data()));
        tracksObject.setProperty(trackIndex, trackObject TAGEDITOR_JS_READONLY);
    }

    return fileInfoObject;
}

TAGEDITOR_JS_VALUE TagEditorObject::parseFileName(const QString &fileName)
{
    QString title;
    int trackNumber = 0;
    Utility::parseFileName(fileName, title, trackNumber);
    auto result = m_engine->newObject();
    result.setProperty(QStringLiteral("title"), TAGEDITOR_JS_VALUE(title) TAGEDITOR_JS_READONLY);
    result.setProperty(QStringLiteral("trackPos"), TAGEDITOR_JS_VALUE(trackNumber) TAGEDITOR_JS_READONLY);
    return result;
}

TAGEDITOR_JS_VALUE TagEditorObject::allFiles(const QString &dirName)
{
    const QDir dir(dirName);
    if (!dir.exists()) {
        return TAGEDITOR_JS_VALUE();
    }
    const auto files(dir.entryList(QDir::Files));
    auto entriesObj = m_engine->newArray(static_cast<uint>(files.size()));
    quint32 counter = 0;
    for (const auto &file : files) {
        entriesObj.setProperty(counter++, file TAGEDITOR_JS_READONLY);
    }
    return entriesObj;
}

TAGEDITOR_JS_VALUE TagEditorObject::firstFile(const QString &dirName)
{
    const QDir dir(dirName);
    if (!dir.exists()) {
        return TAGEDITOR_JS_VALUE();
    }
    const auto files(dir.entryList(QDir::Files));
    if (files.empty()) {
        return TAGEDITOR_JS_VALUE();
    }
    return TAGEDITOR_JS_VALUE(files.first());
}

void TagEditorObject::writeLog(const QString &message)
{
    cout << message.toStdString() << endl;
}

void TagEditorObject::rename(const QString &newName)
{
    m_newName = newName;
    m_action = ActionType::Rename;
}

void TagEditorObject::move(const QString &newRelativeDirectory)
{
    m_newRelativeDirectory = newRelativeDirectory;
    m_action = ActionType::Rename;
}

void TagEditorObject::skip(const QString &note)
{
    m_action = ActionType::Skip;
    m_note = note;
}

} // namespace RenamingUtility

#endif
