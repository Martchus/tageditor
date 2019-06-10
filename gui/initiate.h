#ifndef INITIATE_H
#define INITIATE_H

#include <QtGlobal>

QT_FORWARD_DECLARE_CLASS(QString)

namespace CppUtilities {
class QtConfigArguments;
}

namespace QtGui {

int runWidgetsGui(
    int argc, char *argv[], const CppUtilities::QtConfigArguments &qtConfigArgs, const QString &path, bool launchRenamingUtility = false);
}

#endif // INITIATE_H
