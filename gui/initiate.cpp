#include "initiate.h"
#include "mainwindow.h"
#include "application/settings.h"

#include <qtutilities/resources/resources.h>

#include <QApplication>

namespace QtGui {

int runWidgetsGui(int argc, char *argv[], const QString &path)
{
    SET_QT_APPLICATION_INFO;
    QApplication a(argc, argv);
    LOAD_QT_TRANSLATIONS;
    // load resources needed by classes of qtutilities
    QtUtilitiesResources::init();
    Theme::setup();
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
