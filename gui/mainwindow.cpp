#include "./mainwindow.h"
#include "./dbquerywidget.h"
#include "./renamefilesdialog.h"
#include "./settingsdialog.h"
#include "./tageditorwidget.h"

#include "../application/settings.h"
#include "../misc/htmlinfo.h"
#include "../misc/utility.h"

#include "ui_mainwindow.h"

#include "resources/config.h"

#include <tagparser/mediafileinfo.h>

#include <qtutilities/aboutdialog/aboutdialog.h>
#include <qtutilities/misc/conversion.h>
#include <qtutilities/misc/desktoputils.h>
#include <qtutilities/misc/dialogutils.h>
#include <qtutilities/misc/trylocker.h>
#ifdef TAGEDITOR_SETUP_TOOLS_ENABLED
#include <qtutilities/settingsdialog/optioncategory.h>
#endif

#include <c++utilities/conversion/stringconversion.h>
#include <c++utilities/io/path.h>

#include <QCoreApplication>
#include <QFileDialog>
#include <QFileSystemModel>
#include <QMessageBox>

#include <iomanip>

using namespace std;
using namespace Utility;
using namespace QtUtilities;
using namespace TagParser;

namespace QtGui {

/*!
 * \brief The LoadingResult enum specifies whether the file could be parsed.
 */
enum LoadingResult : char { ParsingSuccessful, FatalParsingError, IoError };

/*!
 * \class QtGui::MainWindow
 * \brief The MainWindow class provides the main window of the Tag Editor's Qt GUI.
 */

/*!
 * \brief Shortcut to access file operation mutex of TagEditorWidget.
 */
bool MainWindow::fileOperationOngoing() const
{
    return m_ui->tagEditorWidget->isFileOperationOngoing();
}

/*!
 * \brief Shortcut to access MediaFileInfo of TagEditorWidget.
 */
MediaFileInfo &MainWindow::fileInfo()
{
    return m_ui->tagEditorWidget->fileInfo();
}

/*!
 * \brief Constructs a new main window.
 */
MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent, Qt::Window)
    , m_ui(new Ui::MainWindow())
    , m_internalFileSelection(false)
    , m_aboutDlg(nullptr)
    , m_settingsDlg(nullptr)
    , m_dbQueryWidget(nullptr)
#ifdef TAGEDITOR_SETUP_TOOLS_ENABLED
    , m_updateSettingsDlg(nullptr)
#endif
{
    // setup UI
    m_ui->setupUi(this);
    updateStyleSheet();

    // restore geometry and state
    const auto &settings = Settings::values();
    restoreGeometry(settings.mainWindow.geometry);
    restoreState(settings.mainWindow.state);

    // setup file model and file tree view
    m_fileModel = new QFileSystemModel(this);
    m_fileModel->setRootPath(QString());
    m_fileFilterModel = new FileFilterProxyModel(this);
    m_fileFilterModel->setExtensionsToBeFiltered(QStringList() << QStringLiteral("bak") << QStringLiteral("tmp"));
    m_fileFilterModel->setSourceModel(m_fileModel);
    m_fileFilterModel->setFilterEnabled(settings.fileBrowser.hideBackupFiles);
    m_ui->filesTreeView->sortByColumn(0, Qt::AscendingOrder);
    m_ui->filesTreeView->setModel(m_fileFilterModel);
    m_ui->filesTreeView->setColumnWidth(0, 300);

    // setup path line edit
    m_ui->pathLineEdit->setCompletionModel(m_fileModel);

    // apply initial file status
    handleFileStatusChange(false, false);

    // dbquery dock widget
    if (settings.dbQuery.widgetShown) {
        initDbQueryWidget();
    }

    // restore locked
    setLayoutLocked(settings.mainWindow.layoutLocked);

    // add action for updater if setup tools are enabled
#ifdef TAGEDITOR_SETUP_TOOLS_ENABLED
    auto *const updaterAction = m_ui->menu->addAction(QIcon::fromTheme(QStringLiteral("install")), tr("Check for updates"));
#endif

    // connect signals and slots, install event filter
    //  menu: application
    connect(m_ui->actionNew_window, &QAction::triggered, this, &MainWindow::showNewWindow);
    connect(m_ui->actionSettings, &QAction::triggered, this, &MainWindow::showSettingsDlg);
    connect(m_ui->actionOpen_MusicBrainz_search, &QAction::triggered, this, &MainWindow::toggleDbQueryWidget);
    connect(m_ui->lockLayout, &QAction::triggered, this, &MainWindow::toggleLayoutLocked);
    connect(m_ui->actionQuit, &QAction::triggered, this, &MainWindow::close);
    //  menu: file
    connect(m_ui->actionOpen, &QAction::triggered, this, &MainWindow::showOpenFileDlg);
    connect(m_ui->actionSave, &QAction::triggered, m_ui->tagEditorWidget, &TagEditorWidget::applyEntriesAndSaveChangings);
    connect(m_ui->actionSave_as, &QAction::triggered, this, &MainWindow::showSaveAsDlg);
    connect(m_ui->actionDelete_all_tags, &QAction::triggered, m_ui->tagEditorWidget, &TagEditorWidget::deleteAllTagsAndSave);
    connect(m_ui->actionSave_file_information, &QAction::triggered, m_ui->tagEditorWidget, &TagEditorWidget::saveFileInfo);
    connect(m_ui->actionClose, &QAction::triggered, m_ui->tagEditorWidget, &TagEditorWidget::closeFile);
    connect(m_ui->actionReload, &QAction::triggered, m_ui->tagEditorWidget, &TagEditorWidget::reparseFile);
    connect(m_ui->actionExternalPlayer, &QAction::triggered, this, &MainWindow::spawnExternalPlayer);
    //  menu: directory
    connect(m_ui->actionSelect_next_file, &QAction::triggered, this, static_cast<void (MainWindow::*)(void)>(&MainWindow::selectNextFile));
    connect(m_ui->actionSelect_next_file_and_save_current, &QAction::triggered, m_ui->tagEditorWidget, &TagEditorWidget::saveAndShowNextFile);
    connect(m_ui->actionRename_files, &QAction::triggered, this, &MainWindow::showRenameFilesDlg);
    //  menu: help
    connect(m_ui->actionAbout, &QAction::triggered, this, &MainWindow::showAboutDlg);
#ifdef TAGEDITOR_SETUP_TOOLS_ENABLED
    connect(updaterAction, &QAction::triggered, this, &MainWindow::showUpdaterDialog);
#endif
    // tag editor widget
    connect(m_ui->tagEditorWidget, &TagEditorWidget::nextFileSelected, this, static_cast<void (MainWindow::*)(void)>(&MainWindow::selectNextFile));
    connect(m_ui->tagEditorWidget, &TagEditorWidget::fileStatusChanged, this, &MainWindow::handleFileStatusChange);
    connect(m_ui->tagEditorWidget, &TagEditorWidget::statusMessage, m_ui->statusBar, &QStatusBar::showMessage);
    connect(m_ui->tagEditorWidget, &TagEditorWidget::currentPathChanged, this, &MainWindow::handleCurrentPathChanged);
    //  misc
    connect(m_ui->pathLineEdit, &QLineEdit::textEdited, this, &MainWindow::pathEntered);
    connect(m_ui->filesTreeView, &QTreeView::expanded, this, &MainWindow::handleFileModelIndexExpanded);
    connect(m_ui->filesTreeView->selectionModel(), &QItemSelectionModel::selectionChanged, this, &MainWindow::fileSelected);
    connect(
        m_ui->selectNextCommandLinkButton, &QCommandLinkButton::clicked, this, static_cast<void (MainWindow::*)(void)>(&MainWindow::selectNextFile));
    // apply settings
    setCurrentDirectory(settings.mainWindow.currentFileBrowserDirectory);
    applySettingsFromDialog();
}

/*!
 * \brief Destroys the main window.
 */
MainWindow::~MainWindow()
{
}

/*!
 * \brief Returns directory the file browser is currently showning.
 */
QString MainWindow::currentDirectory() const
{
    return m_ui->pathLineEdit->text();
}

/*!
 * \brief Sets the directory the file browser is showing.
 * If a file is specified the file will be opened.
 */
void MainWindow::setCurrentDirectory(const QString &path)
{
    m_ui->pathLineEdit->editText(path);
}

/*!
 * \brief Returns whether the layout is \a locked.
 */
bool MainWindow::isLayoutLocked() const
{
    return m_ui->fileSelectionDockWidget->features() == QDockWidget::NoDockWidgetFeatures;
}

/*!
 * \brief Sets whether the layout is \a locked.
 */
void MainWindow::setLayoutLocked(bool locked)
{
    // lock layout if not already locked
    const auto change = locked != isLayoutLocked();
    if (locked) {
        if (!change) {
            return;
        }
        m_ui->fileSelectionDockWidget->setFeatures(QDockWidget::NoDockWidgetFeatures);
        m_ui->dbQueryDockWidget->setFeatures(QDockWidget::NoDockWidgetFeatures);
        m_ui->lockLayout->setText(tr("Unlock layout"));
        m_ui->lockLayout->setIcon(QIcon::fromTheme(QStringLiteral("unlock")));
        return;
    }

    // unlock layout using different features depending on the platform
#if defined(Q_OS_WINDOWS) || defined(Q_OS_MAC)
    constexpr auto features = QDockWidget::DockWidgetMovable | QDockWidget::DockWidgetFloatable;
#else
    auto features = static_cast<QDockWidget::DockWidgetFeatures>(QDockWidget::DockWidgetMovable);
    if (QGuiApplication::platformName().compare(QLatin1String("wayland"), Qt::CaseInsensitive)) {
        // enable floating windows only on non-Wayland platforms as one can never put a floating window back under Wayland
        features |= QDockWidget::DockWidgetFloatable;
    } else {
        // ensure currently floating windows (e.g. from the last X11 session) aren't floating anymore under Wayland
        m_ui->fileSelectionDockWidget->setFloating(false);
        m_ui->dbQueryDockWidget->setFloating(false);
    }
#endif
    m_ui->fileSelectionDockWidget->setFeatures(features);
    m_ui->dbQueryDockWidget->setFeatures(features | QDockWidget::DockWidgetClosable);
    if (change) {
        m_ui->lockLayout->setText(tr("Lock layout"));
        m_ui->lockLayout->setIcon(QIcon::fromTheme(QStringLiteral("lock")));
    }
}

void MainWindow::toggleLayoutLocked()
{
    setLayoutLocked(!isLayoutLocked());
}

/*!
 * \brief Starts parsing the specified file.
 */
void MainWindow::startParsing(const QString &path)
{
    m_ui->tagEditorWidget->startParsing(path, true);
}

/*!
 * \brief
 *  - Saves the applications settings relating the state of the main window.
 *  - Updates the info webview when the palette changed.
 */
bool MainWindow::event(QEvent *event)
{
    auto &settings = Settings::values();
    switch (event->type()) {
    case QEvent::PaletteChange:
        settings.qt.reevaluatePaletteAndDefaultIconTheme();
        updateStyleSheet();
        break;
    case QEvent::Close:
        if (m_ui->tagEditorWidget->isFileOperationOngoing()) {
            event->ignore();
            static const auto warning(tr("Unable to close while the file operation is still ongoing."));
            QMessageBox::warning(this, QCoreApplication::applicationName(), warning);
            return true;
        }

        // save settings
        settings.mainWindow.geometry = saveGeometry();
        settings.mainWindow.state = saveState();
        settings.mainWindow.currentFileBrowserDirectory = currentDirectory();
        settings.mainWindow.layoutLocked = isLayoutLocked();
        settings.dbQuery.widgetShown = m_ui->dbQueryDockWidget->isVisible();
        break;
    default:;
    }
    return QMainWindow::event(event);
}

/*!
 * \brief This private slot is called when the entered text of m_ui->pathLineEdit which represents the current directory changes.
 *
 * If the text is a valid path the current index of the m_ui->filesTreeView is updated to show the path. This invokes the file
 * selected slot.
 */
void MainWindow::pathEntered()
{
    const QString path = m_ui->pathLineEdit->text();
    if (!path.isEmpty()) {
        const QModelIndex index = m_fileFilterModel->mapFromSource(m_fileModel->index(path));
        if (index.isValid()) {
            m_ui->filesTreeView->selectionModel()->setCurrentIndex(index, QItemSelectionModel::Rows | QItemSelectionModel::ClearAndSelect);
            m_ui->pathLineEdit->setProperty("classNames", QStringList());
        } else {
            m_ui->pathLineEdit->setProperty("classNames", QStringList({ QStringLiteral("input-invalid") }));
        }
        updateStyle(m_ui->pathLineEdit);
    }
}

/*!
 * \brief This private slot is called when the selected file or directory changes.
 *
 * If a directory is selected the m_ui->pathLineEdit will be updated.
 * If a file is selected it will be opened.
 */
void MainWindow::fileSelected()
{
    if (m_internalFileSelection) {
        return;
    }
    const QModelIndexList selectedIndexes = m_ui->filesTreeView->selectionModel()->selectedRows();
    if (selectedIndexes.count() != 1) {
        return;
    }
    const QString path(m_fileModel->filePath(m_fileFilterModel->mapToSource(selectedIndexes.at(0))));
    const QFileInfo fileInfo(path);
    if (fileInfo.isFile()) {
        startParsing(path);
        m_ui->pathLineEdit->setText(fileInfo.dir().path());
    } else if (fileInfo.isDir()) {
        m_ui->pathLineEdit->setText(path);
    }
    m_ui->pathLineEdit->setProperty("classNames", QStringList());
    updateStyle(m_ui->pathLineEdit);
}

/*!
 * \brief Updates the status of the relevant widgets (enabled/disabled, visible/hidden) according to the
 *        current "file status" (opened/closed, has tags/no tags).
 */
void MainWindow::handleFileStatusChange(bool opened, bool hasTag)
{
    // actions to save, delete, close
    m_ui->actionSave->setEnabled(opened);
    m_ui->actionDelete_all_tags->setEnabled(hasTag);
    m_ui->actionSave_file_information->setEnabled(opened);
    m_ui->actionSave_as->setEnabled(opened);
    m_ui->actionClose->setEnabled(opened);
    m_ui->actionReload->setEnabled(opened);
    m_ui->actionExternalPlayer->setEnabled(opened);
    // window title
    setWindowTitle(generateWindowTitle(opened ? DocumentStatus::Saved : DocumentStatus::NoDocument, m_ui->tagEditorWidget->currentPath()));
}

/*!
 * \brief Handles that the current path has changed by the tag editor widget itself.
 */
void MainWindow::handleCurrentPathChanged(const QString &newPath)
{
    // ensure the current file is still selected
    m_internalFileSelection = true;
    if (const auto index = m_fileFilterModel->mapFromSource(m_fileModel->index(newPath)); index.isValid()) {
        if (auto *const selectionModel = m_ui->filesTreeView->selectionModel(); !selectionModel->isSelected(index)) {
            m_selectedIndex = index;
            selectionModel->setCurrentIndex(index, QItemSelectionModel::Rows | QItemSelectionModel::ClearAndSelect);
            m_ui->filesTreeView->scrollTo(index, QAbstractItemView::PositionAtCenter);
            m_ui->pathLineEdit->setText(QFileInfo(newPath).dir().path());
            m_ui->pathLineEdit->setProperty("classNames", QStringList());
            updateStyle(m_ui->pathLineEdit);
        }
    } else {
        m_selectedIndex = QModelIndex();
    }
    m_internalFileSelection = false;
    // ensure this is the active window
    activateWindow();
}

/*!
 * \brief Scrolls to the selected index after its parent \a index has been expanded.
 */
void MainWindow::handleFileModelIndexExpanded(const QModelIndex &index)
{
    if (m_selectedIndex.isValid() && index == m_selectedIndex.parent()) {
        m_ui->filesTreeView->scrollTo(m_selectedIndex, QAbstractItemView::PositionAtCenter);
        m_selectedIndex = QModelIndex();
    }
}

/*!
 * \brief Updates the style sheet.
 */
void MainWindow::updateStyleSheet()
{
#ifdef Q_OS_WINDOWS
    setStyleSheet(dialogStyleForPalette(palette()) + QStringLiteral("#tagEditorWidget { color: palette(text); background-color: palette(base); }"));
#else
    setStyleSheet(dialogStyleForPalette(palette()));
#endif
}

/*!
 * \brief Spawns an external player for the current file.
 */
void MainWindow::spawnExternalPlayer()
{
    const QString &currentPath = m_ui->tagEditorWidget->currentPath();
    if (!currentPath.isEmpty()) {
        openLocalFileOrDir(currentPath);
    } else {
        m_ui->statusBar->showMessage(tr("No file opened."));
    }
}

/*!
 * \brief Initializes m_dbQueryWidget is not already initialized.
 */
void MainWindow::initDbQueryWidget()
{
    if (!m_dbQueryWidget) {
        m_ui->dbQueryDockWidget->setWidget(m_dbQueryWidget = new DbQueryWidget(m_ui->tagEditorWidget, this));
        connect(m_ui->tagEditorWidget, &TagEditorWidget::tagValuesLoaded, m_dbQueryWidget,
            static_cast<void (DbQueryWidget::*)(void)>(&DbQueryWidget::autoInsertMatchingResults), Qt::DirectConnection);
    }
}

/*!
 * \brief Toggles visibility of the database query widget.
 */
void MainWindow::toggleDbQueryWidget()
{
    initDbQueryWidget();
    m_ui->dbQueryDockWidget->setVisible(m_ui->dbQueryDockWidget->isHidden());
}

/*!
 * \brief Shows the about dialog.
 */
void MainWindow::showAboutDlg()
{
    if (!m_aboutDlg) {
        m_aboutDlg = new AboutDialog(this, QString(),
            QStringLiteral("<p>Developed by " APP_AUTHOR
                           "<br>Program icon based on icons from <a href=\"https://invent.kde.org/frameworks/breeze-icons\">KDE/Breeze</a> "
                           "project (copyright © 2014 Uri Herrera <uri_herrera@nitrux.in> and others, see the according "
                           "<a href=\"" APP_URL "/blob/master/LICENSE.LESSER\">LGPL-3.0 license</a>)"
                           "<br>Fallback icons from KDE/Breeze as well</p>"),
            QString(), QString(), tr("A tag editing utility supporting ID3, MP4 (iTunes style), Vorbis and Matroska tags."),
            QImage(QStringLiteral(":/tageditor/icons/hicolor/128x128/apps/tageditor.png")));
    }
    if (m_aboutDlg->isHidden()) {
        m_aboutDlg->show();
    } else {
        m_aboutDlg->activateWindow();
    }
}

#ifdef TAGEDITOR_SETUP_TOOLS_ENABLED
/*!
 * \brief Shows the updater dialog.
 */
void MainWindow::showUpdaterDialog()
{
    if (!m_updateSettingsDlg) {
        m_updateSettingsDlg = new UpdateDialog(this);
        m_updateSettingsDlg->page()->setRestartHandler(m_restartHandler.requester());
    }
    if (m_updateSettingsDlg->isHidden()) {
        m_updateSettingsDlg->showNormal();
    } else {
        m_updateSettingsDlg->activateWindow();
    }
}
#endif

/*!
 * \brief MainWindow::showNewWindow
 */
void MainWindow::showNewWindow()
{
    auto *const newWindow = new MainWindow;
    newWindow->setAttribute(Qt::WA_DeleteOnClose);
    newWindow->show();
}

/*!
 * \brief Shows the settings dialog (modal).
 */
void MainWindow::showSettingsDlg()
{
    if (!m_settingsDlg) {
        m_settingsDlg = new SettingsDialog(this);
        connect(m_settingsDlg, &SettingsDialog::applied, this, &MainWindow::applySettingsFromDialog);
        connect(m_settingsDlg, &SettingsDialog::applied, m_ui->tagEditorWidget, &TagEditorWidget::applySettingsFromDialog);
    }
    m_settingsDlg->exec();
}

/*!
 * \brief Shows the "Rename files" dialog (instance of RenameFilesDialog).
 */
void MainWindow::showRenameFilesDlg()
{
    if (!m_renameFilesDlg) {
        m_renameFilesDlg.reset(new RenameFilesDialog);
    }
    m_renameFilesDlg->setDirectory(currentDirectory());
    if (m_renameFilesDlg->isHidden()) {
        m_renameFilesDlg->show();
    } else {
        m_renameFilesDlg->activateWindow();
    }
}

/*!
 * \brief Selects the next file.
 * \remarks Does nothing if there is currently no file selected.
 */
void MainWindow::selectNextFile()
{
    QItemSelectionModel *selectionModel = m_ui->filesTreeView->selectionModel();
    const QModelIndexList selectedIndexes = selectionModel->selectedIndexes();
    if (!selectedIndexes.isEmpty()) {
        selectNextFile(selectionModel, selectedIndexes.at(0), false);
    }
}

/*!
 * \brief Selects the file next to the file with the specified index.
 *
 * If \a notDeeper is false, this method will not try to go deeper into
 * the file system tree.
 */
void MainWindow::selectNextFile(QItemSelectionModel *selectionModel, const QModelIndex &currentIndex, bool notDeeper)
{
    QModelIndex next;
    if (!notDeeper && selectionModel->model()->hasChildren(currentIndex)) {
        // a directory is selected -> go deeper
        if (m_fileFilterModel->canFetchMore(currentIndex)) {
            // files and subdirectories have to be fetched
            // -> QFileSystemModel seems to fetch files and directories async
            // -> hence fetchMore will return immediately
            // -> select next file when rowsInserted is emitted
            auto conn = make_shared<QMetaObject::Connection>();
            *conn = connect(m_fileFilterModel, &QAbstractItemModel::rowsInserted,
                [this, selectionModel, currentIndex, conn](const QModelIndex &parent, int, int) {
                    disconnect(*conn);
                    if (parent == currentIndex) {
                        const QModelIndex nextFetched = m_fileFilterModel->index(0, 0, parent);
                        if (nextFetched.isValid()) {
                            if (m_ui->filesTreeView->model()->hasChildren(nextFetched)) {
                                // next item is a directory -> keep on searching
                                selectNextFile(selectionModel, nextFetched, false);
                            } else {
                                m_ui->filesTreeView->selectionModel()->setCurrentIndex(
                                    nextFetched, QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Rows);
                            }
                        } else {
                            selectNextFile(selectionModel, currentIndex, true);
                        }
                    }
                });
            m_fileModel->fetchMore(m_fileFilterModel->mapToSource(currentIndex));
            return;
        } else {
            // files and subdirectories have been fetched already
            next = currentIndex.model()->index(0, currentIndex.column(), currentIndex);
        }
    }
    if (!next.isValid()) {
        // not possible to go deeper -> choose next sibling
        next = currentIndex.sibling(currentIndex.row() + 1, currentIndex.column());
    }
    if (!next.isValid()) {
        // not possible to choose next sibling -> go higher
        const QModelIndex parent = currentIndex.parent();
        if (parent.isValid()) {
            selectNextFile(selectionModel, parent, true);
            return;
        }
    }
    if (next.isValid()) {
        if (selectionModel->model()->hasChildren(next)) {
            // next item is a directory -> keep on searching
            selectNextFile(selectionModel, next, false);
        } else {
            selectionModel->setCurrentIndex(next, QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Rows);
        }
    } else {
        showNextFileNotFound();
    }
}

void MainWindow::showNextFileNotFound()
{
    static const QString errormsg(tr("Unable to show the next file because it can't be found anymore."));
    QMessageBox::warning(this, QCoreApplication::applicationName(), errormsg);
    m_ui->statusBar->showMessage(errormsg);
}

/*!
 * \brief Shows an open file dialog and opens the selected file.
 */
void MainWindow::showOpenFileDlg()
{
    const QString path = QFileDialog::getOpenFileName(this, tr("Open file - ") + QCoreApplication::applicationName());
    if (!path.isEmpty()) {
        startParsing(path);
    }
}

/*!
 * \brief Shows an save file dialog and triggers saving the changes under the selected location.
 */
void MainWindow::showSaveAsDlg()
{
    const QString path
        = QFileDialog::getSaveFileName(this, tr("Save changes as - ") + QCoreApplication::applicationName(), m_ui->tagEditorWidget->currentDir());
    if (!path.isEmpty()) {
        m_ui->tagEditorWidget->fileInfo().setSaveFilePath(std::string(toNativeFileName(path).data()));
        m_ui->tagEditorWidget->applyEntriesAndSaveChangings();
    }
}

/*!
 * \brief Applies settings from Settings namespace. Only settings configurable through the SettingsDialog
 *        will be applied and not settings like the main window's geometry and state.
 */
void MainWindow::applySettingsFromDialog()
{
    auto &settings = Settings::values();
    if (m_fileFilterModel->isFilterEnabled() != settings.fileBrowser.hideBackupFiles) {
        // check this condition to avoid unnecessary model reset
        m_fileFilterModel->setFilterEnabled(settings.fileBrowser.hideBackupFiles);
        const QModelIndex index = m_fileFilterModel->mapFromSource(m_fileModel->index(m_ui->pathLineEdit->text()));
        if (index.isValid()) {
            m_ui->filesTreeView->selectionModel()->setCurrentIndex(index, QItemSelectionModel::Select);
        }
    }
    if (m_fileModel->isReadOnly() != settings.fileBrowser.readOnly) {
        m_fileModel->setReadOnly(settings.fileBrowser.readOnly);
    }
}

} // namespace QtGui
