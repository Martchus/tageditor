#include "./initiate.h"
#include "./mainwindow.h"
#include "./renamefilesdialog.h"

#include "../application/settings.h"

#include "resources/config.h"
#include "resources/qtconfig.h"

#include <qtutilities/resources/importplugin.h>
#include <qtutilities/resources/qtconfigarguments.h>
#include <qtutilities/resources/resources.h>
#include <qtutilities/settingsdialog/qtsettings.h>

#include <QApplication>

ENABLE_QT_RESOURCES_OF_STATIC_DEPENDENCIES

using namespace CppUtilities;

namespace QtGui {

int runWidgetsGui(int argc, char *argv[], const QtConfigArguments &qtConfigArgs, const QString &path, bool launchRenamingUtility)
{
    SET_QT_APPLICATION_INFO;
    QApplication a(argc, argv);
    Settings::restore();
    // apply settings specified via command line args after the settings chosen in the GUI to give the CLI options precedence
    Settings::values().qt.apply();
    qtConfigArgs.applySettings(Settings::values().qt.hasCustomFont());
    LOAD_QT_TRANSLATIONS;
    int res;
    if (launchRenamingUtility) {
        RenameFilesDialog w;
        w.show();
        res = a.exec();
    } else {
        MainWindow w;
        w.show();
        if (!path.isEmpty()) {
            w.startParsing(path);
        }
        res = a.exec();
    }
    Settings::save();
    return res;
}

} // namespace QtGui
