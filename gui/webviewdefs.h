// Created via CMake from template webviewdefs.h.in
// WARNING! Any changes to this file will be overwritten by the next CMake run!

#ifndef TAGEDITOR_WEB_VIEW_DEFINES
#define TAGEDITOR_WEB_VIEW_DEFINES

#include <QtGlobal>

#if defined(TAGEDITOR_USE_WEBENGINE)
# define TAGEDITOR_WEB_VIEW QWebEngineView
# define TAGEDITOR_WEB_PAGE QWebEnginePage
#elif defined(TAGEDITOR_USE_WEBKIT)
# define TAGEDITOR_WEB_VIEW QWebView
# define TAGEDITOR_WEB_PAGE QWebPage
# define TAGEDITOR_WEB_FRAME QWebFrame
#elif !defined(TAGEDITOR_NO_WEBVIEW)
# error "No definition for web view provider present."
#endif

#ifdef TAGEDITOR_WEB_VIEW
QT_FORWARD_DECLARE_CLASS(TAGEDITOR_WEB_VIEW)
#endif
#ifdef TAGEDITOR_WEB_PAGE
QT_FORWARD_DECLARE_CLASS(TAGEDITOR_WEB_PAGE)
#endif
#ifdef TAGEDITOR_WEB_FRAME
QT_FORWARD_DECLARE_CLASS(TAGEDITOR_WEB_FRAME)
#endif

#endif // TAGEDITOR_WEB_VIEW_DEFINES
