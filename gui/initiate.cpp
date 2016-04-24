#include "./initiate.h"
#include "./mainwindow.h"
#include "./renamefilesdialog.h"

#include "../application/settings.h"

#include "resources/config.h"

#include <qtutilities/resources/qtconfigarguments.h>
#include <qtutilities/resources/resources.h>

#include <QApplication>

using namespace ApplicationUtilities;

namespace QtGui {

int runWidgetsGui(int argc, char *argv[], const QtConfigArguments &qtConfigArgs, const QString &path, bool launchRenamingUtility)
{
    SET_QT_APPLICATION_INFO;
    QApplication a(argc, argv);
    // load resources needed by classes of qtutilities
    QtUtilitiesResources::init();
    // apply settings specified via command line args
    qtConfigArgs.applySettings();
    LOAD_QT_TRANSLATIONS;
    Settings::restore();
    int res;
    if(launchRenamingUtility) {
        RenameFilesDialog w;
        w.show();
        res = a.exec();
    } else {
        MainWindow w;
        w.show();
        if(!path.isEmpty()) {
            w.startParsing(path);
        }
        res = a.exec();
    }
    Settings::save();
    // cleanup resources
    QtUtilitiesResources::cleanup();
    return res;
}

}
