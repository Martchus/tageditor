#ifndef TAGEDITOR_NO_JSENGINE

#include "./tageditorobject.h"
#include "./filesystemitem.h"
#include "./jsincludes.h"

#include "../misc/utility.h"

#include <tagparser/mediafileinfo.h>
#include <tagparser/tag.h>
#include <tagparser/tagvalue.h>
#include <tagparser/abstracttrack.h>
#include <tagparser/exceptions.h>

#include <qtutilities/misc/conversion.h>

#include <c++utilities/conversion/conversionexception.h>
#include <c++utilities/io/catchiofailure.h>

#include <QDir>

#include <iostream>

using namespace ConversionUtilities;
using namespace Utility;
using namespace Media;
using namespace std;

namespace RenamingUtility {

/// \brief Adds notifications from \a statusProvider to \a notificationsObject.
TAGEDITOR_JS_VALUE &operator <<(TAGEDITOR_JS_VALUE &notificationsObject, const StatusProvider &statusProvider)
{
    quint32 counter = 0;
    for(const auto &notification : statusProvider.notifications()) {
        TAGEDITOR_JS_VALUE val;
        val.setProperty("msg", QString::fromUtf8(notification.message().data()) TAGEDITOR_JS_READONLY);
        val.setProperty("critical", notification.type() == NotificationType::Critical TAGEDITOR_JS_READONLY);
        notificationsObject.setProperty(counter, val);
        ++counter;
    }
    return notificationsObject;
}

/// \brief Add fields and notifications from \a tag to \a tagObject.
TAGEDITOR_JS_VALUE &operator <<(TAGEDITOR_JS_VALUE &tagObject, const Tag &tag)
{
    // add text fields
    static const char *fieldNames[] = {"title", "artist", "album", "year", "comment", "genre", "encoder", "language",
                                       "description", nullptr};
    static const KnownField fields[] = {KnownField::Title, KnownField::Artist, KnownField::Album, KnownField::Year,
                                        KnownField::Comment, KnownField::Genre, KnownField::Encoder, KnownField::Language,
                                        KnownField::Description};
    const char **fieldName = fieldNames;
    const KnownField *field = fields;
    for(; *fieldName; ++fieldName, ++field) {
        try {
            tagObject.setProperty(*fieldName, tagValueToQString(tag.value(*field)) TAGEDITOR_JS_READONLY);
        } catch(const ConversionException &) {}
    }

    // add numeric fields
    try {
        tagObject.setProperty("partNumber", tag.value(KnownField::PartNumber).toInteger() TAGEDITOR_JS_READONLY);
    } catch(const ConversionException &) {}
    try {
        tagObject.setProperty("totalParts", tag.value(KnownField::TotalParts).toInteger() TAGEDITOR_JS_READONLY);
    } catch(const ConversionException &) {}
    PositionInSet pos;
    try {
        pos = tag.value(KnownField::TrackPosition).toPositionInSet();
    } catch(const ConversionException &) {}
    tagObject.setProperty("trackPos", pos.position() TAGEDITOR_JS_READONLY);
    tagObject.setProperty("trackTotal", pos.total() TAGEDITOR_JS_READONLY);
    pos = PositionInSet();
    try {
        pos = tag.value(KnownField::DiskPosition).toPositionInSet();
    } catch(const ConversionException &) {}
    tagObject.setProperty("diskPos", pos.position() TAGEDITOR_JS_READONLY);
    tagObject.setProperty("diskTotal", pos.total() TAGEDITOR_JS_READONLY);

    // add notifications
    tagObject.setProperty("hasCriticalNotifications", tag.hasCriticalNotifications() TAGEDITOR_JS_READONLY);
    return tagObject;
}

TagEditorObject::TagEditorObject(TAGEDITOR_JS_ENGINE *engine) :
    m_engine(engine),
    m_currentType(ItemType::Dir),
    m_action(ActionType::None)
{}

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
    MediaFileInfo fileInfo(toNativeFileName(fileName).data());

    // add basic file information
    auto fileInfoObject = m_engine->newObject();
    fileInfoObject.setProperty(QStringLiteral("currentName"), QString::fromUtf8(fileInfo.fileName(false).data()));
    fileInfoObject.setProperty(QStringLiteral("currentBaseName"), QString::fromUtf8(fileInfo.fileName(true).data()));
    QString suffix = fromNativeFileName(fileInfo.extension().data());
    if(suffix.startsWith('.')) {
        suffix.remove(0, 1);
    }
    fileInfoObject.setProperty(QStringLiteral("currentSuffix"), suffix TAGEDITOR_JS_READONLY);

    // parse further file information
    bool criticalParseingErrorOccured = false;
    try {
        fileInfo.parseEverything();
    } catch(const Failure &) {
        // parsing notifications will be addded anyways
        criticalParseingErrorOccured = true;
    } catch(...) {
        ::IoUtilities::catchIoFailure();
        criticalParseingErrorOccured = true;
    }

    // gather notifications
    auto mainNotificationObject = m_engine->newArray(static_cast<uint>(fileInfo.notifications().size()));
    mainNotificationObject << fileInfo;
    criticalParseingErrorOccured |= fileInfo.hasCriticalNotifications();
    fileInfoObject.setProperty(QStringLiteral("hasCriticalNotifications"), criticalParseingErrorOccured);
    fileInfoObject.setProperty(QStringLiteral("notifications"), mainNotificationObject);

    // add MIME-type, suitable suffix and technical summary
    fileInfoObject.setProperty(QStringLiteral("mimeType"), QString::fromUtf8(fileInfo.mimeType()) TAGEDITOR_JS_READONLY);
    fileInfoObject.setProperty(QStringLiteral("suitableSuffix"), QString::fromUtf8(fileInfo.containerFormatAbbreviation()) TAGEDITOR_JS_READONLY);
    fileInfoObject.setProperty(QStringLiteral("technicalSummary"), QString::fromUtf8(fileInfo.technicalSummary().data()) TAGEDITOR_JS_READONLY);

    // gather tag information
    const vector<Tag *> tags = fileInfo.tags();
    auto combinedTagObject = m_engine->newObject();
    auto combinedTagNotifications = m_engine->newArray();
    auto tagsObject = m_engine->newArray(static_cast<uint>(tags.size()));
    uint32 tagIndex = 0;
    for(auto tagIterator = tags.cbegin(), end = tags.cend(); tagIterator != end; ++tagIterator, ++tagIndex) {
        const Tag &tag = **tagIterator;
        auto tagObject = m_engine->newObject();
        combinedTagObject << tag;
        combinedTagNotifications << tag;
        tagObject << tag;
        auto tagNotificationsObject = m_engine->newArray(static_cast<uint>(tag.notifications().size()));
        tagNotificationsObject << tag;
        tagObject.setProperty(QStringLiteral("notifications"), tagNotificationsObject TAGEDITOR_JS_READONLY);
        tagsObject.setProperty(tagIndex, tagObject TAGEDITOR_JS_READONLY);
    }
    combinedTagObject.setProperty(QStringLiteral("notifications"), combinedTagNotifications TAGEDITOR_JS_READONLY);
    fileInfoObject.setProperty(QStringLiteral("tag"), combinedTagObject TAGEDITOR_JS_READONLY);
    fileInfoObject.setProperty(QStringLiteral("tags"), tagsObject TAGEDITOR_JS_READONLY);

    // gather track information
    const vector<AbstractTrack *> tracks = fileInfo.tracks();
    auto tracksObject = m_engine->newArray(static_cast<uint>(tracks.size()));
    uint32 trackIndex = 0;
    for(auto trackIterator = tracks.cbegin(), end = tracks.cend(); trackIterator != end; ++trackIterator, ++trackIndex) {
        const AbstractTrack &track = **trackIterator;
        auto trackObject = m_engine->newObject();
        trackObject.setProperty(QStringLiteral("mediaType"), QString::fromUtf8(track.mediaTypeName()));
        trackObject.setProperty(QStringLiteral("format"), QString::fromUtf8(track.formatName()));
        trackObject.setProperty(QStringLiteral("formatAbbreviation"), QString::fromUtf8(track.formatAbbreviation()));
        trackObject.setProperty(QStringLiteral("description"), QString::fromUtf8(track.description().data()));
        auto trackNotificationsObject = m_engine->newArray(static_cast<uint>(track.notifications().size()));
        trackNotificationsObject << track;
        trackObject.setProperty(QStringLiteral("notifications"), trackNotificationsObject TAGEDITOR_JS_READONLY);
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
    QDir dir(dirName);
    if(dir.exists()) {
        QStringList files = dir.entryList(QDir::Files);
        auto entriesObj = m_engine->newArray(files.length());
        quint32 counter = 0;
        for(const QString &file : files) {
            entriesObj.setProperty(counter, file TAGEDITOR_JS_READONLY);
            ++counter;
        }
        return entriesObj;
    } else {
        return TAGEDITOR_JS_VALUE();
    }
}

TAGEDITOR_JS_VALUE TagEditorObject::firstFile(const QString &dirName)
{
    QDir dir(dirName);
    if(dir.exists()) {
        QStringList files = dir.entryList(QDir::Files);
        if(!files.empty()) {
            return TAGEDITOR_JS_VALUE(files.first());
        }
    }
    return TAGEDITOR_JS_VALUE();
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

void TagEditorObject::skip()
{
    m_action = ActionType::Skip;
}

} // namespace RenamingUtility

#endif
