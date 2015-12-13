#include "./initiate.h"
#include "./mainwindow.h"

#include "../application/settings.h"

// include configuration from separate header file when building with CMake
#ifndef APP_METADATA_AVAIL
#include "resources/config.h"
#endif

#include <qtutilities/resources/qtconfigarguments.h>
#include <qtutilities/resources/resources.h>

#include <QApplication>

using namespace ApplicationUtilities;

namespace QtGui {

int runWidgetsGui(int argc, char *argv[], const QtConfigArguments &qtConfigArgs, const QString &path)
{
    SET_QT_APPLICATION_INFO;
    QApplication a(argc, argv);
    // load resources needed by classes of qtutilities
    QtUtilitiesResources::init();
    // apply settings specified via command line args
    qtConfigArgs.applySettings();
    LOAD_QT_TRANSLATIONS;
    Settings::restore();
    MainWindow w;
    w.show();
    if(!path.isEmpty()) {
        w.startParsing(path, true);
    }
    int res = a.exec();
    Settings::save();
    // cleanup resources
    QtUtilitiesResources::cleanup();
    return res;
}

}
