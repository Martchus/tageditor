#include "./initiate.h"
#include "./mainwindow.h"
#include "./renamefilesdialog.h"

#include "../application/settings.h"

#include "resources/config.h"
#include "resources/qtconfig.h"

#include <QApplication> // ensure QGuiApplication is defined before resources.h for desktop file name

#include <qtutilities/resources/importplugin.h>
#include <qtutilities/resources/qtconfigarguments.h>
#include <qtutilities/resources/resources.h>
#include <qtutilities/settingsdialog/qtsettings.h>

#ifdef TAGEDITOR_SETUP_TOOLS_ENABLED
#include <qtutilities/setup/updater.h>

#include <c++utilities/misc/verification.h>
#endif

#include <QMessageBox>
#include <QSettings>

#ifdef TAGEDITOR_SETUP_TOOLS_ENABLED
#include <QNetworkAccessManager>
#endif

ENABLE_QT_RESOURCES_OF_STATIC_DEPENDENCIES

using namespace CppUtilities;

namespace QtGui {

// define public key and signature extension depending on verification backend
#ifdef TAGEDITOR_SETUP_TOOLS_ENABLED
// clang-format off
#define SYNCTHINGTRAY_SIGNATURE_EXTENSION ".openssl.sig"
constexpr auto signingKeyOpenSSL = std::string_view(
    R"(-----BEGIN EC PUBLIC KEY-----
MIGbMBAGByqGSM49AgEGBSuBBAAjA4GGAAQAWJAn1E7ZE5Q6H69oaV5sqCIppJdg
4bXDan9dJv6GOg70/t7q2CvwcwUXhV4FvCZxCHo25+rWYINfqKU2Utul8koAx8tK
59ohfOzI63I+CC76GfX41uRGU0P5i6hS7o/hgBLiVXqT0FgS2BMfmnLMUvUjqnI2
YQM7C55/5BM5Vrblkow=
-----END EC PUBLIC KEY-----
)");
// clang-format on
#endif

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
    auto application = QApplication(argc, argv);
    auto s = QtUtilities::getSettings(QStringLiteral(PROJECT_NAME));
    Settings::restore(*s);

    // apply settings specified via command line args after the settings chosen in the GUI to give the CLI options precedence
    auto &settings = Settings::values();
    settings.qt.disableNotices();
    settings.qt.apply();
    qtConfigArgs.applySettings(settings.qt.hasCustomFont());

    // initialize updater
#ifdef TAGEDITOR_SETUP_TOOLS_ENABLED
    auto *const nam = new QNetworkAccessManager;
    auto updateHandler = QtUtilities::UpdateHandler(s.get(), nam);
    nam->setParent(&updateHandler);
    auto verificationErrorMsgBox = QtUtilities::VerificationErrorMessageBox();
    updateHandler.updater()->setVerifier([&verificationErrorMsgBox](const QtUtilities::Updater::Update &update) {
        auto error = QString();
        if (update.signature.empty()) {
            error = QStringLiteral("empty/non-existent signature");
        } else {
            const auto res = CppUtilities::verifySignature(signingKeyOpenSSL, update.signature, update.data);
            error = QString::fromUtf8(res.data(), static_cast<QString::size_type>(res.size()));
        }
        if (!error.isEmpty()) {
            verificationErrorMsgBox.execForError(error);
        }
        return error;
    });
    updateHandler.applySettings();
    QtUtilities::UpdateHandler::setMainInstance(&updateHandler);
#endif

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
    const auto res = application.exec();
#ifdef TAGEDITOR_SETUP_TOOLS_ENABLED
    window.respawnIfRestartRequested();
#endif
    return res;
}

} // namespace QtGui
