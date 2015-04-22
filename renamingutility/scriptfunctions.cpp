#include "scriptfunctions.h"
#include "misc/utility.h"

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

QScriptValue ScriptFunctions::parseFileInfo(QScriptContext *context, QScriptEngine *engine)
{
    if(context->argumentCount() != 1 && !context->argument(0).isString()) {
        return QScriptValue();
    }
    QString fileName = context->argument(0).toString();

    auto addNotifications = [] (QScriptValue &notificationObject, const StatusProvider &statusProvider) {
        quint32 counter = 0;
        for(const Notification &notification : statusProvider.notifications()) {
            QScriptValue val;
            val.setProperty("msg", QString::fromLocal8Bit(notification.message().c_str()), QScriptValue::ReadOnly);
            val.setProperty("critical", notification.type() == NotificationType::Critical, QScriptValue::ReadOnly);
            notificationObject.setProperty(counter, val);
            ++counter;
        }
    };

    QScriptValue fileInfoObject = engine->newObject();
    QScriptValue tagObject = engine->newObject();
    QScriptValue tagNotificationObject = engine->newArray();

    MediaFileInfo fileInfo(fileName.toLocal8Bit().data());

    fileInfoObject.setProperty("currentName", QString::fromLocal8Bit(fileInfo.fileName(false).c_str()));
    fileInfoObject.setProperty("currentBaseName", QString::fromLocal8Bit(fileInfo.fileName(true).c_str()));
    QString suffix = QString::fromLocal8Bit(fileInfo.extension().c_str());
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
    addNotifications(mainNotificationObject, fileInfo);
    critical |= fileInfo.hasCriticalNotifications();
    fileInfoObject.setProperty("hasCriticalNotifications", critical);
    fileInfoObject.setProperty("notifications", mainNotificationObject);

    fileInfoObject.setProperty("mimeType", QString::fromLocal8Bit(fileInfo.mimeType()), QScriptValue::ReadOnly);
    fileInfoObject.setProperty("suitableSuffix", QString::fromLocal8Bit(fileInfo.containerFormatAbbreviation()), QScriptValue::ReadOnly);

    vector<Tag *> tags;
    fileInfo.tags(tags);
    for(vector<Tag *>::const_iterator tagIterator = tags.cbegin(), end = tags.cend();
        tagIterator != end; ++tagIterator) {
        const Tag &tag = **tagIterator;
        tagObject.setProperty("title", tagValueToQString(tag.value(KnownField::Title)), QScriptValue::ReadOnly);
        tagObject.setProperty("artist", tagValueToQString(tag.value(KnownField::Artist)), QScriptValue::ReadOnly);
        tagObject.setProperty("album", tagValueToQString(tag.value(KnownField::Album)), QScriptValue::ReadOnly);
        tagObject.setProperty("year", tagValueToQString(tag.value(KnownField::Year)), QScriptValue::ReadOnly);
        tagObject.setProperty("comment", tagValueToQString(tag.value(KnownField::Comment)), QScriptValue::ReadOnly);
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

        addNotifications(tagNotificationObject, tag);
        tagObject.setProperty("hasCriticalNotifications", tag.hasCriticalNotifications(), QScriptValue::ReadOnly);
    }
    fileInfoObject.setProperty("notifications", tagNotificationObject, QScriptValue::ReadOnly);
    fileInfoObject.setProperty("tag", tagObject, QScriptValue::ReadOnly);
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
