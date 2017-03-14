// Created via CMake from template webviewdefs.h.in
// WARNING! Any changes to this file will be overwritten by the next CMake run!

#ifndef TAGEDITOR_JAVA_SCRIPT_INCLUDES
#define TAGEDITOR_JAVA_SCRIPT_INCLUDES

#include <QtGlobal>

#if defined(TAGEDITOR_USE_JSENGINE)
# include <QJSEngine>
# include <QJSValue>
#elif defined(TAGEDITOR_USE_SCRIPT)
# include <QScriptEngine>
# include <QScriptValue>
#elif !defined(TAGEDITOR_NO_WEBVIEW)
# error "No definition for JavaScript provider present."
#endif

#endif // TAGEDITOR_JAVA_SCRIPT_INCLUDES
