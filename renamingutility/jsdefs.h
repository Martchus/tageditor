// Created via CMake from template webviewdefs.h.in
// WARNING! Any changes to this file will be overwritten by the next CMake run!

#ifndef TAGEDITOR_JAVA_SCRIPT_DEFINES
#define TAGEDITOR_JAVA_SCRIPT_DEFINES

#include <QtGlobal>

#if defined(TAGEDITOR_USE_JSENGINE)
# define TAGEDITOR_JS_ENGINE QJSEngine
# define TAGEDITOR_JS_VALUE QJSValue
# define TAGEDITOR_JS_READONLY
# define TAGEDITOR_JS_UNDELETABLE
# define TAGEDITOR_JS_QOBJECT(engine, obj) engine.newQObject(obj)
# define TAGEDITOR_JS_INT(value) value.toInt()
# define TAGEDITOR_JS_IS_VALID_PROG(program) (!program.isError() && program.isCallable())
#elif defined(TAGEDITOR_USE_SCRIPT)
# define TAGEDITOR_JS_ENGINE QScriptEngine
# define TAGEDITOR_JS_VALUE QScriptValue
# define TAGEDITOR_JS_READONLY ,QScriptValue::ReadOnly
# define TAGEDITOR_JS_UNDELETABLE ,QScriptValue::Undeletable
# define TAGEDITOR_JS_QOBJECT(engine, obj) engine.newQObject(obj, QScriptEngine::ScriptOwnership)
# define TAGEDITOR_JS_INT(value) value.toInt32()
# define TAGEDITOR_JS_IS_VALID_PROG(program) (!program.isError() && program.isFunction())
#elif !defined(TAGEDITOR_NO_WEBVIEW)
# error "No definition for JavaScript provider present."
#endif

#ifdef TAGEDITOR_JS_ENGINE
QT_FORWARD_DECLARE_CLASS(TAGEDITOR_JS_ENGINE)
#endif
#ifdef TAGEDITOR_JS_VALUE
QT_FORWARD_DECLARE_CLASS(TAGEDITOR_JS_VALUE)
#endif

#endif // TAGEDITOR_JAVA_SCRIPT_DEFINES
