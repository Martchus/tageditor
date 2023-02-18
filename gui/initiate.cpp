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
#include <QMessageBox>

ENABLE_QT_RESOURCES_OF_STATIC_DEPENDENCIES

using namespace CppUtilities;

namespace QtGui {

static void showSettingsError(const QString &settingsError)
{
    if (!settingsError.isEmpty()) {
        QMessageBox::critical(nullptr, QCoreApplication::applicationName(), settingsError);
    }
}

static void saveSettings()
{
    Settings::save();
    showSettingsError(Settings::values().error);
}

int runWidgetsGui(int argc, char *argv[], const QtConfigArguments &qtConfigArgs, const QString &path, bool launchRenamingUtility)
{
    SET_QT_APPLICATION_INFO;
    QApplication application(argc, argv);
    Settings::restore();

    // apply settings specified via command line args after the settings chosen in the GUI to give the CLI options precedence
    auto &settings = Settings::values();
    settings.qt.apply();
    qtConfigArgs.applySettings(settings.qt.hasCustomFont());

    LOAD_QT_TRANSLATIONS;

    showSettingsError(settings.error);
    QObject::connect(&application, &QCoreApplication::aboutToQuit, &saveSettings);

    if (launchRenamingUtility) {
        RenameFilesDialog window;
        window.setAttribute(Qt::WA_QuitOnClose);
        window.show();
        return application.exec();
    }

    MainWindow window;
    window.show();
    if (!path.isEmpty()) {
        window.startParsing(path);
    }
    return application.exec();
}

} // namespace QtGui
