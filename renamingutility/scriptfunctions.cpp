#include "./scriptfunctions.h"

#include "../misc/utility.h"

#include <tagparser/mediafileinfo.h>
#include <tagparser/tag.h>
#include <tagparser/tagvalue.h>
#include <tagparser/exceptions.h>

#include <c++utilities/conversion/conversionexception.h>

#include <QDir>
#include <QScriptEngine>
#include <QScriptContext>
#include <QScriptValue>

#include <iostream>

using namespace ConversionUtilities;
using namespace Utility;
using namespace Media;
using namespace std;

namespace RenamingUtility {

/*
    TRANSLATOR RenamingUtility::ScriptFunctions
    Necessary for lupdate.
*/

QScriptValue &operator <<(QScriptValue &notificationsObject, const StatusProvider &statusProvider)
{
    quint32 counter = 0;
    for(const auto &notification : statusProvider.notifications()) {
        QScriptValue val;
        val.setProperty("msg", QString::fromLocal8Bit(notification.message().data()), QScriptValue::ReadOnly);
        val.setProperty("critical", notification.type() == NotificationType::Critical, QScriptValue::ReadOnly);
        notificationsObject.setProperty(counter, val);
        ++counter;
    }
    return notificationsObject;
}

QScriptValue &operator <<(QScriptValue &tagObject, const Tag &tag)
{
    // text fields
    tagObject.setProperty("title", tagValueToQString(tag.value(KnownField::Title)), QScriptValue::ReadOnly);
    tagObject.setProperty("artist", tagValueToQString(tag.value(KnownField::Artist)), QScriptValue::ReadOnly);
    tagObject.setProperty("album", tagValueToQString(tag.value(KnownField::Album)), QScriptValue::ReadOnly);
    tagObject.setProperty("year", tagValueToQString(tag.value(KnownField::Year)), QScriptValue::ReadOnly);
    tagObject.setProperty("comment", tagValueToQString(tag.value(KnownField::Comment)), QScriptValue::ReadOnly);
    tagObject.setProperty("genre", tagValueToQString(tag.value(KnownField::Genre)), QScriptValue::ReadOnly);
    tagObject.setProperty("encoder", tagValueToQString(tag.value(KnownField::Encoder)), QScriptValue::ReadOnly);
    tagObject.setProperty("language", tagValueToQString(tag.value(KnownField::Language)), QScriptValue::ReadOnly);
    tagObject.setProperty("descriptions", tagValueToQString(tag.value(KnownField::Description)), QScriptValue::ReadOnly);
    // numeric fields
    try {
        tagObject.setProperty("partNumber", tag.value(KnownField::PartNumber).toInteger(), QScriptValue::ReadOnly);
    } catch(ConversionException &) {}
    try {
        tagObject.setProperty("totalParts", tag.value(KnownField::TotalParts).toInteger(), QScriptValue::ReadOnly);
    } catch(ConversionException &) {}
    PositionInSet pos;
    try {
        pos = tag.value(KnownField::TrackPosition).toPositionIntSet();
    } catch(ConversionException &) {}
    tagObject.setProperty("trackPos", pos.position(), QScriptValue::ReadOnly);
    tagObject.setProperty("trackTotal", pos.total(), QScriptValue::ReadOnly);
    pos = PositionInSet();
    try {
        pos = tag.value(KnownField::DiskPosition).toPositionIntSet();
    } catch(ConversionException &) {}
    tagObject.setProperty("diskPos", pos.position(), QScriptValue::ReadOnly);
    tagObject.setProperty("diskTotal", pos.total(), QScriptValue::ReadOnly);
    // notifications
    tagObject.setProperty("hasCriticalNotifications", tag.hasCriticalNotifications(), QScriptValue::ReadOnly);
    return tagObject;
}

QScriptValue ScriptFunctions::parseFileInfo(QScriptContext *context, QScriptEngine *engine)
{
    if(context->argumentCount() != 1 && !context->argument(0).isString()) {
        return QScriptValue();
    }
    auto fileName = context->argument(0).toString();
    MediaFileInfo fileInfo(fileName.toLocal8Bit().data());

    QScriptValue fileInfoObject = engine->newObject();
    fileInfoObject.setProperty("currentName", QString::fromLocal8Bit(fileInfo.fileName(false).data()));
    fileInfoObject.setProperty("currentBaseName", QString::fromLocal8Bit(fileInfo.fileName(true).data()));
    QString suffix = QString::fromLocal8Bit(fileInfo.extension().data());
    if(suffix.startsWith('.')) {
        suffix.remove(0, 1);
    }
    fileInfoObject.setProperty("currentSuffix", suffix, QScriptValue::ReadOnly);
    bool critical = false;
    try {
        fileInfo.parseEverything();
    } catch(Failure &) {
        // parsing notifications will be addded anyways
        critical = true;
    } catch(ios_base::failure &) {
        critical = true;
    }

    QScriptValue mainNotificationObject = engine->newArray(fileInfo.notifications().size());
    mainNotificationObject << fileInfo;
    critical |= fileInfo.hasCriticalNotifications();
    fileInfoObject.setProperty("hasCriticalNotifications", critical);
    fileInfoObject.setProperty("notifications", mainNotificationObject);

    fileInfoObject.setProperty("mimeType", QString::fromLocal8Bit(fileInfo.mimeType()), QScriptValue::ReadOnly);
    fileInfoObject.setProperty("suitableSuffix", QString::fromLocal8Bit(fileInfo.containerFormatAbbreviation()), QScriptValue::ReadOnly);

    vector<Tag *> tags;
    fileInfo.tags(tags);
    QScriptValue combinedTagObject = engine->newObject();
    QScriptValue combinedTagNotifications = engine->newArray();
    QScriptValue tagsObject = engine->newArray(tags.size());
    uint32 tagIndex = 0;

    for(auto tagIterator = tags.cbegin(), end = tags.cend(); tagIterator != end; ++tagIterator, ++tagIndex) {
        const Tag &tag = **tagIterator;
        QScriptValue tagObject = engine->newObject();
        combinedTagObject << tag;
        combinedTagNotifications << tag;
        tagObject << tag;
        QScriptValue tagNotificationsObject = engine->newArray(tag.notifications().size());
        tagNotificationsObject << tag;
        tagObject.setProperty("notifications", tagNotificationsObject, QScriptValue::ReadOnly);
        tagsObject.setProperty(tagIndex, tagObject, QScriptValue::ReadOnly);
    }
    combinedTagObject.setProperty("notifications", combinedTagNotifications, QScriptValue::ReadOnly);
    fileInfoObject.setProperty("tag", combinedTagObject, QScriptValue::ReadOnly);
    fileInfoObject.setProperty("tags", tagsObject, QScriptValue::ReadOnly);
    return fileInfoObject;
}

QScriptValue ScriptFunctions::parseFileName(QScriptContext *context, QScriptEngine *engine)
{
    if(context->argumentCount() != 1 && !context->argument(0).isString()) {
        return QScriptValue();
    }
    QString fileName = context->argument(0).toString();
    QString title;
    int trackNumber = 0;
    Utility::parseFileName(fileName, title, trackNumber);
    QScriptValue result = engine->newObject();
    result.setProperty("title", QScriptValue(title), QScriptValue::ReadOnly);
    result.setProperty("trackPos", QScriptValue(trackNumber), QScriptValue::ReadOnly);
    return result;
}

QScriptValue ScriptFunctions::allFiles(QScriptContext *context, QScriptEngine *engine)
{
    if(context->argumentCount() != 1 && !context->argument(0).isString()) {
        return QScriptValue();
    }
    QString dirName = context->argument(0).toString();
    QDir dir(dirName);
    if(dir.exists()) {
        QStringList files = dir.entryList(QDir::Files);
        QScriptValue entriesObj = engine->newArray(files.length());
        quint32 counter = 0;
        foreach(const QString &file, files) {
            entriesObj.setProperty(counter, file, QScriptValue::ReadOnly);
            ++counter;
        }
        return entriesObj;
    } else {
        return QScriptValue();
    }
}

QScriptValue ScriptFunctions::firstFile(QScriptContext *context, QScriptEngine *engine)
{
    if(context->argumentCount() != 1 && !context->argument(0).isString()) {
        return QScriptValue();
    }
    QString dirName = context->argument(0).toString();
    QDir dir(dirName);
    if(dir.exists()) {
        QStringList files = dir.entryList(QDir::Files);
        if(files.length() > 0) {
            return engine->newVariant(files.first());
        }
    }
    return QScriptValue();
}

QScriptValue ScriptFunctions::writeLog(QScriptContext *context, QScriptEngine *)
{
    if(context->argumentCount() != 1 && !context->argument(0).isString()) {
        return QScriptValue();
    }
    cout << context->argument(0).toString().toStdString() << endl;
    return QScriptValue();
}

} // namespace RenamingUtility
