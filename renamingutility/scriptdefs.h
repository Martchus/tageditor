#ifndef SCRIPTDEFS_H
#define SCRIPTDEFS_H

#include <QtGlobal>

#if defined(TAGEDITOR_USE_JSENGINE)
# define TAGEDITOR_JS_ENGINE QJSEngine
# define TAGEDITOR_JS_VALUE QJSValue
# define TAGEDITOR_JS_READONLY
# define TAGEDITOR_JS_UNDELETABLE
# define TAGEDITOR_JS_QOBJECT(engine, obj) engine.newQObject(obj)
# define TAGEDITOR_JS_INT(value) value.toInt()
# define TAGEDITOR_JS_IS_VALID_PROG(program) (!program.isError() && program.isCallable())
QT_FORWARD_DECLARE_CLASS(QJSValue)
QT_FORWARD_DECLARE_CLASS(QJSEngine)
#elif defined(TAGEDITOR_USE_SCRIPT)
# define TAGEDITOR_JS_ENGINE QScriptEngine
# define TAGEDITOR_JS_VALUE QScriptValue
# define TAGEDITOR_JS_READONLY ,QScriptValue::ReadOnly
# define TAGEDITOR_JS_UNDELETABLE ,QScriptValue::Undeletable
# define TAGEDITOR_JS_QOBJECT(engine, obj) engine.newQObject(obj, QScriptEngine::ScriptOwnership)
# define TAGEDITOR_JS_INT(value) value.toInt32()
# define TAGEDITOR_JS_IS_VALID_PROG(program) (!program.isError() && program.isFunction())
QT_FORWARD_DECLARE_CLASS(QScriptValue)
QT_FORWARD_DECLARE_CLASS(QScriptEngine)
#endif

#endif // SCRIPTDEFS_H
