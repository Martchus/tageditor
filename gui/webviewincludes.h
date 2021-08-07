// Created via CMake from template webviewincludes.h.in
// WARNING! Any changes to this file will be overwritten by the next CMake run!

#ifndef TAGEDITOR_WEB_VIEW_INCLUDES
#define TAGEDITOR_WEB_VIEW_INCLUDES

#include <QtGlobal>

#if defined(TAGEDITOR_USE_WEBENGINE)
# include <QWebEngineView>
# include <QWebEnginePage>
# include <QtWebEngineWidgetsVersion>
#elif defined(TAGEDITOR_USE_WEBKIT)
# include <QWebView>
# include <QWebPage>
# include <QWebFrame>
#elif !defined(TAGEDITOR_NO_WEBVIEW)
# error "No definition for web view provider present."
#endif

#endif // TAGEDITOR_WEB_VIEW_INCLUDES
