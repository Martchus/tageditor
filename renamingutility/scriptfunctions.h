#ifndef RENAMINGUTILITY_SCRIPTFUNCTIONS_H
#define RENAMINGUTILITY_SCRIPTFUNCTIONS_H

#include <QtGlobal>

QT_BEGIN_NAMESPACE
class QScriptValue;
class QScriptContext;
class QScriptEngine;
QT_END_NAMESPACE

namespace RenamingUtility {

class ScriptFunctions
{
public:
    static QScriptValue parseFileInfo(QScriptContext *context, QScriptEngine *engine);
    static QScriptValue parseFileName(QScriptContext *context, QScriptEngine *engine);
    static QScriptValue allFiles(QScriptContext *context, QScriptEngine *engine);
    static QScriptValue firstFile(QScriptContext *context, QScriptEngine *engine);
    static QScriptValue writeLog(QScriptContext *context, QScriptEngine *);
};

} // namespace RenamingUtility

#endif // RENAMINGUTILITY_SCRIPTFUNCTIONS_H
