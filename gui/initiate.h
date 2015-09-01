#ifndef INITIATE_H
#define INITIATE_H

#include <QtGlobal>

QT_FORWARD_DECLARE_CLASS(QString)

namespace ApplicationUtilities {
class QtConfigArguments;
}

namespace QtGui {

int runWidgetsGui(int argc, char *argv[], const ApplicationUtilities::QtConfigArguments &qtConfigArgs, const QString &path);

}

#endif // INITIATE_H
