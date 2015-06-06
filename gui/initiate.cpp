#include "initiate.h"
#include "mainwindow.h"
#include "application/settings.h"

#include <qtutilities/resources/resources.h>

#include <QApplication>

namespace QtGui {

int runWidgetsGui(int argc, char *argv[])
{
    // set application information
    QApplication a(argc, argv);
    QApplication::setOrganizationName(QStringLiteral("Martchus"));
    QApplication::setOrganizationDomain(QStringLiteral("http://martchus.netai.net/"));
    QApplication::setApplicationName(QStringLiteral("Tageditor"));
    QApplication::setApplicationVersion(QStringLiteral("1.1.5"));
    // load translation files
    TranslationFiles::loadQtTranslationFile();
    TranslationFiles::loadApplicationTranslationFile(QStringLiteral("tageditor"));
    // load resources needed by classes of qtutilities
    QtUtilitiesResources::init();
    Theme::setup();
    Settings::restore();
    MainWindow w;
    w.show();
    int res = a.exec();
    Settings::save();
    // cleanup resources
    QtUtilitiesResources::cleanup();
    return res;
}

}
