#include "./mainwindow.h"
#include "./settingsdialog.h"
#include "./renamefilesdialog.h"
#include "./dbquerywidget.h"
#include "./tageditorwidget.h"

#include "../application/settings.h"
#include "../misc/utility.h"
#include "../misc/htmlinfo.h"

#include "ui_mainwindow.h"

#include <tagparser/mediafileinfo.h>

#include <qtutilities/aboutdialog/aboutdialog.h>
#include <qtutilities/misc/dialogutils.h>
#include <qtutilities/misc/desktoputils.h>
#include <qtutilities/misc/trylocker.h>

#include <c++utilities/conversion/stringconversion.h>
#include <c++utilities/io/path.h>

#include <QMessageBox>
#include <QFileDialog>
#include <QFileSystemModel>
#include <QTextStream>
#include <QTextCodec>

#include <iomanip>

using namespace std;
using namespace Utility;
using namespace Media;
using namespace Dialogs;
using namespace Widgets;
using namespace ThreadingUtils;

namespace QtGui {

/*!
 * \brief The LoadingResult enum specifies whether the file could be parsed.
 */
enum LoadingResult : char
{
    ParsingSuccessful,
    FatalParsingError,
    IoError
};

/*!
 * \class QtGui::MainWindow
 * \brief The MainWindow class provides the main window of the Tag Editor's Qt GUI.
 */

/*!
 * \brief Shortcut to access file operation mutex of TagEditorWidget.
 */
QMutex &MainWindow::fileOperationMutex()
{
    return m_ui->tagEditorWidget->fileOperationMutex();
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
MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent, Qt::Window),
    m_ui(new Ui::MainWindow()),
    m_aboutDlg(nullptr),
    m_settingsDlg(nullptr),
    m_dbQueryWidget(nullptr)
{
    // setup UI
    m_ui->setupUi(this);
#ifdef Q_OS_WIN32
    setStyleSheet(dialogStyle() + QStringLiteral("#tagEditorWidget { color: palette(text); background-color: palette(base); }"));
#else
    setStyleSheet(dialogStyle());
#endif

    // restore geometry and state
    restoreGeometry(Settings::mainWindowGeometry());
    restoreState(Settings::mainWindowState());

    // setup file model and file tree view
    m_fileModel = new QFileSystemModel(this);
    m_fileModel->setRootPath(QString());
    m_fileFilterModel = new FileFilterProxyModel(this);
    m_fileFilterModel->setExtensionsToBeFiltered(QStringList() << QStringLiteral("bak") << QStringLiteral("tmp"));
    m_fileFilterModel->setSourceModel(m_fileModel);
    m_fileFilterModel->setFilterEnabled(Settings::hideBackupFiles());
    m_ui->filesTreeView->sortByColumn(0, Qt::AscendingOrder);
    m_ui->filesTreeView->setModel(m_fileFilterModel);
    m_ui->filesTreeView->setColumnWidth(0, 300);

    // setup path line edit
    m_ui->pathLineEdit->setCompletionModel(m_fileModel);

    // apply initial file status
    handleFileStatusChange(false, false);

    // dbquery dock widget
    if(Settings::dbQueryWidgetShown()) {
        showDbQueryWidget();
    } else {
        // ensure the dock widget is invisible
        m_ui->dbQueryDockWidget->setVisible(false);
    }

    // connect signals and slots, install event filter
    //  menu: application
    connect(m_ui->actionSettings, &QAction::triggered, this, &MainWindow::showSettingsDlg);
    connect(m_ui->actionOpen_MusicBrainz_search, &QAction::triggered, this, &MainWindow::showDbQueryWidget);
    connect(m_ui->actionQuit, &QAction::triggered, this, &MainWindow::close);
    //  menu: file
    connect(m_ui->actionOpen, &QAction::triggered, this, &MainWindow::showOpenFileDlg);
    connect(m_ui->actionSave, &QAction::triggered, m_ui->tagEditorWidget, &TagEditorWidget::applyEntriesAndSaveChangings);
    connect(m_ui->actionSave_as, &QAction::triggered, this, &MainWindow::showSaveAsDlg);
    connect(m_ui->actionDelete_all_tags, &QAction::triggered, m_ui->tagEditorWidget, &TagEditorWidget::deleteAllTagsAndSave);
    connect(m_ui->actionSave_file_information, &QAction::triggered, this, &MainWindow::saveFileInformation);
    connect(m_ui->actionClose, &QAction::triggered, m_ui->tagEditorWidget, &TagEditorWidget::closeFile);
    connect(m_ui->actionReload, &QAction::triggered, m_ui->tagEditorWidget, &TagEditorWidget::reparseFile);
    connect(m_ui->actionExternalPlayer, &QAction::triggered, this, &MainWindow::spawnExternalPlayer);
    //  menu: directory
    connect(m_ui->actionSelect_next_file, &QAction::triggered, this, static_cast<void(MainWindow::*)(void)>(&MainWindow::selectNextFile));
    connect(m_ui->actionSelect_next_file_and_save_current, &QAction::triggered, m_ui->tagEditorWidget, &TagEditorWidget::saveAndShowNextFile);
    connect(m_ui->actionRename_files, &QAction::triggered, this, &MainWindow::showRenameFilesDlg);
    //  menu: help
    connect(m_ui->actionAbout, &QAction::triggered, this, &MainWindow::showAboutDlg);
    // tag editor widget
    connect(m_ui->tagEditorWidget, &TagEditorWidget::nextFileSelected, this, static_cast<void(MainWindow::*)(void)>(&MainWindow::selectNextFile));
    connect(m_ui->tagEditorWidget, &TagEditorWidget::fileStatusChange, this, &MainWindow::handleFileStatusChange);
    connect(m_ui->tagEditorWidget, &TagEditorWidget::statusMessage, m_ui->statusBar, &QStatusBar::showMessage);
    connect(m_ui->tagEditorWidget, &TagEditorWidget::fileSaved, this, &MainWindow::handleFileSaved);
    //  misc
    connect(m_ui->pathLineEdit, &QLineEdit::textEdited, this, &MainWindow::pathEntered);
    connect(m_ui->filesTreeView->selectionModel(), &QItemSelectionModel::selectionChanged, this, &MainWindow::fileSelected);
    connect(m_ui->selectNextCommandLinkButton, &QCommandLinkButton::clicked, this, static_cast<void(MainWindow::*)(void)>(&MainWindow::selectNextFile));
    // apply settings
    setCurrentDirectory(Settings::mainWindowCurrentFileBrowserDirectory());
    applySettingsFromDialog();
}

/*!
 * \brief Destroys the main window.
 */
MainWindow::~MainWindow()
{}

/*!
 * \brief Returns directory the file browser is currently showning.
 */
QString MainWindow::currentDirectory()
{
    return m_ui->pathLineEdit->text();
}

/*!
 * \brief Sets the directory the file browser is showing.
 * If a file is specified the file will be opended.
 */
void MainWindow::setCurrentDirectory(const QString &path)
{
    m_ui->pathLineEdit->editText(path);
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
    switch(event->type()) {
    case QEvent::Close:
        // save settings
        Settings::mainWindowGeometry() = saveGeometry();
        Settings::mainWindowState() = saveState();
        Settings::mainWindowCurrentFileBrowserDirectory() = currentDirectory();
        Settings::dbQueryWidgetShown() = m_ui->dbQueryDockWidget->isVisible();
        break;
    default:
        ;
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
    QString path = m_ui->pathLineEdit->text();
    if(!path.isEmpty()) {
        QModelIndex index = m_fileFilterModel->mapFromSource(m_fileModel->index(path));
        if(index.isValid()) {
            m_ui->filesTreeView->selectionModel()->setCurrentIndex(index, QItemSelectionModel::Rows | QItemSelectionModel::ClearAndSelect);
            m_ui->pathLineEdit->setProperty("classNames", QStringList());
        } else {
            m_ui->pathLineEdit->setProperty("classNames", QStringList() << QStringLiteral("input-invalid"));
        }
        updateStyle(m_ui->pathLineEdit);
    }
}

/*!
 * \brief This private slot is called when the selected file or directory changes.
 * If a directory is selected the m_ui->pathLineEdit will be updated.
 * If a file is selected it will be opened.
 */
void MainWindow::fileSelected()
{
    QModelIndexList selectedIndexes = m_ui->filesTreeView->selectionModel()->selectedRows();
    if(selectedIndexes.count() == 1) {
        QString path(m_fileModel->filePath(m_fileFilterModel->mapToSource(selectedIndexes.at(0))));
        QFileInfo fileInfo(path);
        if(fileInfo.isFile()) {
            startParsing(path);
            m_ui->pathLineEdit->setText(fileInfo.dir().path());
        } else if(fileInfo.isDir()) {
            m_ui->pathLineEdit->setText(path);
        }
        m_ui->pathLineEdit->setProperty("classNames", QStringList());
        updateStyle(m_ui->pathLineEdit);
    }
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
    setWindowTitle(Dialogs::generateWindowTitle(opened ? DocumentStatus::Saved : DocumentStatus::NoDocument, m_ui->tagEditorWidget->currentPath()));
}

void MainWindow::handleFileSaved(const QString &currentPath)
{
    // ensure the current file is still selected
    const QModelIndex index = m_fileFilterModel->mapFromSource(m_fileModel->index(currentPath));
    if(index.isValid()) {
        m_ui->filesTreeView->selectionModel()->setCurrentIndex(index, QItemSelectionModel::Rows | QItemSelectionModel::ClearAndSelect);
    }
    // ensure this is the active window
    activateWindow();
}

/*!
 * \brief Spawns an external player for the current file.
 */
void MainWindow::spawnExternalPlayer()
{
    const QString &currentPath = m_ui->tagEditorWidget->currentPath();
    if(!currentPath.isEmpty()) {
        DesktopUtils::openLocalFileOrDir(currentPath);
    } else {
        m_ui->statusBar->showMessage(tr("No file opened."));
    }
}

/*!
 * \brief Shows the database query widget.
 */
void MainWindow::showDbQueryWidget()
{
    if(!m_dbQueryWidget) {
        m_ui->dbQueryDockWidget->setWidget(m_dbQueryWidget = new DbQueryWidget(m_ui->tagEditorWidget, this));
    }
    m_ui->dbQueryDockWidget->setVisible(true);
}

/*!
 * \brief Shows the about dialog.
 */
void MainWindow::showAboutDlg()
{
    if(!m_aboutDlg) {
        m_aboutDlg = new Dialogs::AboutDialog(this, tr("A tag editing utility supporting ID3, MP4 (iTunes style), Vorbis and Matroska tags."), QImage(QStringLiteral(":/tageditor/icons/hicolor/128x128/apps/tageditor.png")));
    }
    if(m_aboutDlg->isHidden()) {
        m_aboutDlg->show();
    } else {
        m_aboutDlg->activateWindow();
    }
}

/*!
 * \brief Shows the settings dialog (modal).
 */
void MainWindow::showSettingsDlg()
{
    if(!m_settingsDlg) {
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
    if(!m_renameFilesDlg) {
        m_renameFilesDlg.reset(new RenameFilesDialog);
    }
    m_renameFilesDlg->setDirectory(currentDirectory());
    if(m_renameFilesDlg->isHidden()) {
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
    if(!selectedIndexes.isEmpty()) {
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
    if(!notDeeper && selectionModel->model()->hasChildren(currentIndex)) {
        // a directory is selected -> go deeper
        if(m_fileFilterModel->canFetchMore(currentIndex)) {
            // files and subdirectories have to be fetched
            // -> QFileSystemModel seems to fetch files and directories async
            // -> hence fetchMore will return immediatly
            // -> select next file when rowsInserted is emitted
            auto conn = make_shared<QMetaObject::Connection>();
            *conn = connect(m_fileFilterModel, &QAbstractItemModel::rowsInserted, [this, selectionModel, currentIndex, conn] (const QModelIndex &parent, int, int) {
                disconnect(*conn);
                if(parent == currentIndex) {
                    const QModelIndex next = m_fileFilterModel->index(0, 0, parent);
                    if(next.isValid()) {
                        if(m_ui->filesTreeView->model()->hasChildren(next)) {
                            // next item is a directory -> keep on searching
                            selectNextFile(selectionModel, next, false);
                        } else {
                            m_ui->filesTreeView->selectionModel()->setCurrentIndex(next, QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Rows);
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
            next = currentIndex.child(0, currentIndex.column());
        }
    }
    if(!next.isValid()) {
        // not possible to go deeper -> choose next sibling
        next = currentIndex.sibling(currentIndex.row() + 1, currentIndex.column());
    }
    if(!next.isValid()) {
        // not possible to choose next sibling -> go higher
        const QModelIndex parent = currentIndex.parent();
        if(parent.isValid()) {
            selectNextFile(selectionModel, parent, true);
            return;
        }
    }
    if(next.isValid()) {
        if(selectionModel->model()->hasChildren(next)) {
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
    QMessageBox::warning(this, QApplication::applicationName(), errormsg);
    m_ui->statusBar->showMessage(errormsg);
}

/*!
 * \brief Shows an open file dialog and opens the selected file.
 */
void MainWindow::showOpenFileDlg()
{
    const QString path = QFileDialog::getOpenFileName(this, tr("Open file - ") + QCoreApplication::applicationName());
    if(!path.isEmpty()) {
        startParsing(path);
    }
}

/*!
 * \brief Shows an save file dialog and triggers saving the changes under the selected location.
 */
void MainWindow::showSaveAsDlg()
{
    const QString path = QFileDialog::getSaveFileName(this, tr("Save changes as - ") + QCoreApplication::applicationName());
    if(!path.isEmpty()) {
        m_ui->tagEditorWidget->fileInfo().setSaveFilePath(path.toLocal8Bit().data());
        m_ui->tagEditorWidget->applyEntriesAndSaveChangings();
    }
}

/*!
 * \brief Saves the file information generated to be displayed in the info web view in a file.
 */
void MainWindow::saveFileInformation()
{
    TryLocker<> locker(fileOperationMutex());
    if(locker) {
        if(fileInfo().isOpen()) {
            const QByteArray htmlData =
        #ifndef TAGEDITOR_NO_WEBVIEW
                   !m_ui->tagEditorWidget->fileInfoHtml().isEmpty() ?
                        m_ui->tagEditorWidget->fileInfoHtml() :
            #endif
                        HtmlInfo::generateInfo(fileInfo(), m_ui->tagEditorWidget->originalNotifications());
            if(!htmlData.isEmpty()) {
                const QString path = QFileDialog::getSaveFileName(this, tr("Save file information - ") + QCoreApplication::applicationName());
                if(!path.isEmpty()) {
                    QFile file(path);
                    if(file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
                        QTextStream stream(&file);
                        stream.setCodec(QTextCodec::codecForName("UTF-8"));
                        stream << htmlData;
                        file.close();
                        if(file.error() != QFileDevice::NoError) {
                            QMessageBox::critical(this, QApplication::applicationName(), tr("Unable to write to file.\n%1").arg(file.errorString()));
                        }
                    } else {
                        QMessageBox::critical(this, QApplication::applicationName(), tr("Unable to open file."));
                    }
                }
            } else {
                QMessageBox::information(this, QApplication::applicationName(), tr("No file information available."));
            }
        } else {
            QMessageBox::information(this, QApplication::applicationName(), tr("No file is opened."));
        }
    } else {
        m_ui->statusBar->showMessage(tr("Unable to save file information because the current process hasn't been finished yet."));
    }
}

/*!
 * \brief Applies settings from Settings namespace. Only settings configurable through the SettingsDialog
 *        will be applied and not settings like the main window's geometry and state.
 */
void MainWindow::applySettingsFromDialog()
{
    if(m_fileFilterModel->isFilterEnabled() != Settings::hideBackupFiles()) {
        // check this condition to avoid unnecessary model reset
        m_fileFilterModel->setFilterEnabled(Settings::hideBackupFiles());
        const QModelIndex index = m_fileFilterModel->mapFromSource(m_fileModel->index(m_ui->pathLineEdit->text()));
        if(index.isValid()) {
            m_ui->filesTreeView->selectionModel()->setCurrentIndex(index, QItemSelectionModel::Select);
        }
    }
    if(m_fileModel->isReadOnly() != Settings::fileBrowserReadOnly()) {
        m_fileModel->setReadOnly(Settings::fileBrowserReadOnly());
    }
}

}
