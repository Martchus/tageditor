#include "./mainwindow.h"
#include "./settingsdialog.h"
#include "./notificationlabel.h"
#include "./renamefilesdialog.h"
#include "./tagedit.h"
#include "./attachmentsedit.h"
#include "./entertargetdialog.h"

#include "../application/settings.h"
#include "../misc/htmlinfo.h"
#include "../misc/utility.h"

#include "ui_mainwindow.h"

#include <tagparser/exceptions.h>
#include <tagparser/signature.h>
#include <tagparser/abstractcontainer.h>
#include <tagparser/tag.h>
#include <tagparser/id3/id3v1tag.h>
#include <tagparser/id3/id3v2tag.h>
#include <tagparser/mp4/mp4container.h>
#include <tagparser/mp4/mp4tag.h>
#include <tagparser/matroska/matroskatag.h>

#include <c++utilities/conversion/stringconversion.h>
#include <c++utilities/io/path.h>

#include <qtutilities/aboutdialog/aboutdialog.h>
#include <qtutilities/misc/dialogutils.h>

#include <QMessageBox>
#include <QGraphicsScene>
#include <QGraphicsPixmapItem>
#include <QFileDialog>
#include <QInputDialog>
#include <QClipboard>
#include <QKeyEvent>
#include <QFileSystemModel>
#include <QCompleter>
#include <QPlainTextEdit>
#include <QMimeData>
#include <QTextStream>
#include <QFileSystemWatcher>
#include <QDesktopServices>
#ifdef TAGEDITOR_USE_WEBENGINE
#include <QWebEngineView>
#else
#include <QWebView>
#endif

#include <list>
#include <unordered_set>
#include <thread>
#include <sstream>
#include <iomanip>
#include <functional>
#include <algorithm>

using namespace std;
using namespace Utility;
using namespace Media;
using namespace Dialogs;
using namespace Widgets;

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
 * \brief The MainWindow class provides the main window of the Tag Editor's Qt gui.
 */

/*!
 * \brief Constructs a new main window.
 */
MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent, Qt::Window),
    m_ui(new Ui::MainWindow()),
    m_aboutDlg(nullptr),
    m_settingsDlg(nullptr),
    m_makingResultsAvailable(false),
    m_abortClicked(false)
{
    // setup UI
    m_ui->setupUi(this);
    makeHeading(m_ui->fileNameLabel);
#ifdef TAGEDITOR_USE_WEBENGINE
    m_infoWebView = new QWebEngineView(m_ui->tagSplitter);
#else
    m_infoWebView = new QWebView(m_ui->tagSplitter);
#endif
    m_infoWebView->setObjectName(QStringLiteral("infoWebView"));
    m_infoWebView->setAcceptDrops(false);
    m_infoWebView->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(m_infoWebView, &QWidget::customContextMenuRequested, this, &MainWindow::showInfoWebViewContextMenu);
    m_ui->tagSplitter->addWidget(m_infoWebView);
#ifdef Q_OS_WIN32
    setStyleSheet(dialogStyle() + QStringLiteral("#rightWidget, #rightWidget QSplitter::handle { color: palette(text); background-color: palette(base); }"));
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
    m_fileFilterModel->setExtensionsToBeFiltered(QStringList() << "bak" << "tmp");
    m_fileFilterModel->setSourceModel(m_fileModel);
    m_fileFilterModel->setFilterEnabled(Settings::hideBackupFiles());
    m_ui->filesTreeView->sortByColumn(0, Qt::AscendingOrder);
    m_ui->filesTreeView->setModel(m_fileFilterModel);
    m_ui->filesTreeView->setColumnWidth(0, 300);
    // setup path line edit
    m_ui->pathLineEdit->setCompletionModel(m_fileModel);
    // setup file watcher
    m_fileWatcher = new QFileSystemWatcher(this);
    m_fileChangedOnDisk = false;
    // setup command link button icons
    m_ui->saveButton->setIcon(style()->standardIcon(QStyle::SP_DialogSaveButton, nullptr, m_ui->saveButton));
    m_ui->deleteTagsButton->setIcon(style()->standardIcon(QStyle::SP_DialogResetButton, nullptr, m_ui->deleteTagsButton));
    m_ui->closeButton->setIcon(style()->standardIcon(QStyle::SP_DialogDiscardButton, nullptr, m_ui->closeButton));
    // setup m_keepPreviousValuesMenu
    m_keepPreviousValuesMenu = new QMenu(this);
    QActionGroup *group = new QActionGroup(this);
    group->addAction(m_ui->actionKeep_previous_values_never);
    group->addAction(m_ui->actionKeep_previous_values_within_same_dir);
    group->addAction(m_ui->actionKeep_previous_values_always);
    connect(group, &QActionGroup::triggered, this, &MainWindow::handleKeepPreviousValuesActionTriggered);
    m_keepPreviousValuesMenu->addActions(group->actions());
    m_ui->keepPreviousValuesPushButton->setMenu(m_keepPreviousValuesMenu);
    // setup m_tagOptionsMenu, m_addTagMenu, m_removeTagMenu, m_changeTargetMenu
    m_tagOptionsMenu = new QMenu(this);
    m_tagOptionsMenu->addAction(m_ui->actionManage_tags_automatically_when_loading_file);
    connect(m_ui->actionManage_tags_automatically_when_loading_file, &QAction::triggered, [] (bool checked) { Settings::autoTagManagement() = checked; });
    m_tagOptionsMenu->addSeparator();
    m_addTagMenu = new QMenu(tr("Add tag"), m_tagOptionsMenu);
    m_addTagMenu->setEnabled(false);
    m_addTagMenu->setIcon(QIcon::fromTheme(QStringLiteral("tag-add")));
    m_tagOptionsMenu->addMenu(m_addTagMenu);
    m_removeTagMenu = new QMenu(tr("Remove tag"), m_tagOptionsMenu);
    m_removeTagMenu->setEnabled(false);
    m_removeTagMenu->setIcon(QIcon::fromTheme(QStringLiteral("tag-delete")));
    m_tagOptionsMenu->addMenu(m_removeTagMenu);
    m_changeTargetMenu = new QMenu(tr("Change target"), m_tagOptionsMenu);
    m_changeTargetMenu->setEnabled(false);
    m_changeTargetMenu->setIcon(QIcon::fromTheme(QStringLiteral("tag-properties")));
    m_tagOptionsMenu->addMenu(m_changeTargetMenu);
    m_ui->tagOptionsPushButton->setMenu(m_tagOptionsMenu);
    // other widgets
    updateUiStatus();
    m_ui->abortButton->setVisible(false);
    // connect signals and slots, install event filter
    //  menu: application
    connect(m_ui->actionSettings, &QAction::triggered, this, &MainWindow::showSettingsDlg);
    connect(m_ui->actionQuit, &QAction::triggered, this, &MainWindow::close);
    //  menu: file
    connect(m_ui->actionOpen, &QAction::triggered, this, &MainWindow::showOpenFileDlg);
    connect(m_ui->actionSave, &QAction::triggered, this, &MainWindow::applyEntriesAndSaveChangings);
    connect(m_ui->actionDelete_all_tags, &QAction::triggered, this, &MainWindow::deleteAllTagsAndSave);
    connect(m_ui->actionSave_file_information, &QAction::triggered, this, &MainWindow::saveFileInformation);
    connect(m_ui->actionClose, &QAction::triggered, this, &MainWindow::closeFile);
    connect(m_ui->actionReload, &QAction::triggered, this, &MainWindow::reparseFile);
    connect(m_ui->actionExternalPlayer, &QAction::triggered, this, &MainWindow::spawnExternalPlayer);
    //  menu: directory
    connect(m_ui->actionSelect_next_file, &QAction::triggered, this, &MainWindow::selectNextFile);
    connect(m_ui->actionSelect_next_file_and_save_current, &QAction::triggered, this, &MainWindow::saveAndShowNextFile);
    connect(m_ui->actionRename_files, &QAction::triggered, this, &MainWindow::showRenameFilesDlg);
    //  menu: help
    connect(m_ui->actionAbout, &QAction::triggered, this, &MainWindow::showAboutDlg);
    //  buttons: save, delete, next, close
    connect(m_ui->saveButton, &QPushButton::clicked, this, &MainWindow::applyEntriesAndSaveChangings);
    connect(m_ui->deleteTagsButton, &QPushButton::clicked, this, &MainWindow::deleteAllTagsAndSave);
    connect(m_ui->nextButton, &QPushButton::clicked, this, &MainWindow::saveAndShowNextFile);
    connect(m_ui->closeButton, &QPushButton::clicked, this, &MainWindow::closeFile);
    //  misc
    connect(m_ui->pathLineEdit, &QLineEdit::textEdited, this, &MainWindow::pathEntered);
    connect(m_ui->filesTreeView->selectionModel(), &QItemSelectionModel::selectionChanged, this, &MainWindow::fileSelected);
    connect(m_ui->selectNextCommandLinkButton, &QCommandLinkButton::clicked, this, &MainWindow::selectNextFile);
    connect(m_ui->abortButton, &QPushButton::clicked, [this] {m_abortClicked = true; m_ui->abortButton->setEnabled(false); });
    connect(m_ui->tagSelectionComboBox, static_cast<void (QComboBox::*)(int)>(&QComboBox::activated), m_ui->stackedWidget, &QStackedWidget::setCurrentIndex);
    connect(m_fileWatcher, &QFileSystemWatcher::fileChanged, this, &MainWindow::fileChangedOnDisk);
    //  event filter
    m_ui->rightWidget->installEventFilter(this);
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
 * \brief
 *  - Saves the applications settings relating the state of the main window.
 *  - Updates the info webview when the palette changed.
 */
bool MainWindow::event(QEvent *event)
{
    switch(event->type()) {
    case QEvent::PaletteChange:
        updateInfoWebView();
        break;
    case QEvent::Close:
        // save settings
        Settings::mainWindowGeometry() = saveGeometry();
        Settings::mainWindowState() = saveState();
        Settings::mainWindowCurrentFileBrowserDirectory() = currentDirectory();
        break;
    default:
        ;
    }
    return QMainWindow::event(event);
}

bool MainWindow::eventFilter(QObject *obj, QEvent *event)
{
    if(obj == m_ui->rightWidget) {
        switch(event->type()) {
        case QEvent::DragEnter:
        case QEvent::Drop:
            if(QDropEvent *dropEvent = static_cast<QDropEvent *>(event)) {
                for(const auto &url : dropEvent->mimeData()->urls()) {
                    if(url.scheme() == QLatin1String("file")) {
                        event->accept();
                        if(event->type() == QEvent::Drop) {
#ifdef Q_OS_WIN32
                            // remove leading slash
                            QString path = url.path();
                            int index = 0;
                            for(const auto &c : path) {
                               if(c == QChar('/')) {
                                   ++index;
                               } else {
                                   break;
                               }
                            }
                            if(index) {
                                path = path.mid(index);
                            }
                            startParsing(path, true);
#else
                            startParsing(url.path(), true);
#endif
                        }
                        return true;
                    }
                }
            }
        default:
            ;
        }
    }
    return QMainWindow::eventFilter(obj, event);
}

/*!
 * \brief This private slot is called when the entered text of m_ui->pathLineEdit which represents the current directory changes.
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
            m_ui->pathLineEdit->setStyleSheet(QString());
        } else {
            m_ui->pathLineEdit->setStyleSheet(QStringLiteral("color: red;"));
        }
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
        m_ui->pathLineEdit->setStyleSheet(QString());
    }
}

/*!
 * \brief Sets the file to be opened next and invokes saving the current file.
 */
void MainWindow::saveAndShowNextFile()
{
    if(!m_fileOperationMutex.try_lock()) {
        m_ui->statusBar->showMessage(tr("Unable to save the selected file and load the next file after saving because the current process hasn't finished yet."));
        return;
    }
    // clear previous value
    m_nextFilePath.clear();
    // fetch next file from the file model
    QModelIndexList selectedIndexes = m_ui->filesTreeView->selectionModel()->selectedRows();
    if(selectedIndexes.count() == 1) {
        const QModelIndex &selected = selectedIndexes.at(0);
        QModelIndex next = selected.sibling(selected.row() + 1, selected.column());
        if(next.isValid()) {
            QModelIndex nextInSource = m_fileFilterModel->mapToSource(next);
            // set it to m_nextFilePath
            m_nextFilePath = m_fileModel->filePath(nextInSource);
            m_fileOperationMutex.unlock();
            applyEntriesAndSaveChangings();
            // next file set to m_nextFilePath will be opened when changings have been applied in showSavingResult(...)
            return;
        }
    }
    m_fileOperationMutex.unlock();
    static const QString msg = tr("Unable to find the next file. The chanings of the currently opened file will be saved regardless.");
    QMessageBox::warning(this, QApplication::applicationName(), msg);
    m_ui->statusBar->showMessage(msg);
    applyEntriesAndSaveChangings();
}

/*!
 * \brief Updates the line edits for the document title(s).
 */
void MainWindow::updateDocumentTitleEdits()
{
    // get container, segment count and present titles
    AbstractContainer *container = m_fileInfo.container();
    int segmentCount = container ? static_cast<int>(container->segmentCount()) : 0;
    const vector<string> &titles = container ? container->titles() : vector<string>();

    // get layout
    QLayout *docTitleLayout = m_ui->docTitleWidget->layout();
    int lineEditCount = docTitleLayout->count() - 1;

    // update existing line edits, remove unneeded line edits
    int i = 0;
    for(; i < lineEditCount; ++i) {
        if(i < segmentCount) {
            // update existing line edit
            static_cast<ClearLineEdit *>(docTitleLayout->itemAt(i + 1)->widget())
                    ->setText(static_cast<size_t>(i) < titles.size() ? QString::fromUtf8(titles[i].data()) : QString());
        } else {
            // remove unneeded line edit
            docTitleLayout->removeItem(docTitleLayout->itemAt(i + 1));
        }
    }

    // add missing line edits
    while(i < segmentCount) {
        auto *lineEdit = new Widgets::ClearLineEdit;
        if(static_cast<size_t>(i) < titles.size()) {
            lineEdit->setText(QString::fromUtf8(titles[i].data()));
        }
        lineEdit->setPlaceholderText(tr("Segment %1").arg(++i));
        docTitleLayout->addWidget(lineEdit);
    }
}

/*!
 * \brief Update the tag edits and the tag selection to show the specified \a tags.
 * \param tags Specifies the tags to be shown.
 * \param updateUi Specifies whether the UI of the tag edits should be updated.
 * \remarks The tag selection combo box should be updated after calling this method to
 *          ensure the updated edits can be selected properly.
 */
void MainWindow::updateTagEditsAndAttachmentEdits(bool updateUi, PreviousValueHandling previousValueHandling)
{
    // determine to previous value handling according to the settings if auto is specified
    switch(previousValueHandling) {
    case PreviousValueHandling::Auto:
        switch(Settings::adoptFields()) {
        case Settings::AdoptFields::WithinDirectory:
            if(m_lastDir != m_currentDir) {
                previousValueHandling = PreviousValueHandling::Clear;
            }
            break;
        case Settings::AdoptFields::Never:
            previousValueHandling = PreviousValueHandling::Clear;
            break;
        default:
            ;
        }
        break;
    default:
        ;
    }
    // define helper function to fetch next edit
    TagEdit *edit; // holds current edit
    int widgetIndex = 0; // holds index of current edit in the stacked widget
    auto fetchNextEdit = [this, &edit, &widgetIndex, &previousValueHandling] {
        // reuse existing edit (assigned in if-condition!) or ...
        if(!((widgetIndex < m_ui->stackedWidget->count())
                && (edit = qobject_cast<TagEdit *>(m_ui->stackedWidget->widget(widgetIndex))))) {
            // ... create and add a new edit
            edit = new TagEdit;
            connect(m_ui->clearEntriesPushButton, &QPushButton::clicked, edit, &TagEdit::clear);
            connect(m_ui->restoreEntriesPushButton, &QPushButton::clicked, edit, &TagEdit::restore);
            connect(edit, &TagEdit::returnPressed, this, &MainWindow::handleReturnPressed);
            m_ui->stackedWidget->insertWidget(widgetIndex, edit);
        }
        // apply settings
        edit->setPreviousValueHandling(previousValueHandling);
    };
    // add/update TagEdit widgets
    if(m_tags.size()) {
        // create a lists of the targets and tags
        QList<TagTarget> targets;
        QList<QList<Tag *> > tagsByTarget;
        for(Tag *tag : m_tags) {
            const TagTarget &target = tag->target();
            int index = targets.indexOf(target);
            if(index < 0) {
                targets << target;
                tagsByTarget << (QList<Tag *>() << tag);
            } else {
                tagsByTarget[index] << tag;
            }
        }
        // create a singe editor per target or seperate editors for each tag depending on the settings
        switch(Settings::multipleTagHandling()) {
        case Settings::MultipleTagHandling::SingleEditorPerTarget:
            // iterate through all targets in both cases
            for(int targetIndex = 0, targetCount = targets.size(); targetIndex < targetCount; ++targetIndex) {
                fetchNextEdit();
                edit->setTags(tagsByTarget.at(targetIndex), updateUi); // set all tags with the same target to a single edit
                ++widgetIndex;                
            }
            break;
        case Settings::MultipleTagHandling::SeparateEditors:
            // iterate through all targets in both cases
            for(int targetIndex = 0, targetCount = targets.size(); targetIndex < targetCount; ++targetIndex) {
                for(Tag *tag : tagsByTarget.at(targetIndex)) {
                    fetchNextEdit();
                    edit->setTag(tag, updateUi); // use a separate edit for each tag
                    ++widgetIndex;
                }
            }
            break;
        }
    } else {
        // there are no tags -> leave one edit existend but ensure no tags are assigned
        fetchNextEdit();
        edit->setTag(nullptr, false);
        ++widgetIndex;
    }
    // add/update AttachmentsEdit widget
    if(m_fileInfo.areAttachmentsSupported()) {
        AttachmentsEdit *edit;
        // reuse existing edit (assigned in if-condition!) or ...
        if((widgetIndex < m_ui->stackedWidget->count())
             && (edit = qobject_cast<AttachmentsEdit *>(m_ui->stackedWidget->widget(widgetIndex)))) {
            edit->setFileInfo(&m_fileInfo, true);
        } else {
            // ... create and add a new edit
            edit = new AttachmentsEdit(&m_fileInfo, this);
            connect(m_ui->clearEntriesPushButton, &QPushButton::clicked, edit, &AttachmentsEdit::clear);
            connect(m_ui->restoreEntriesPushButton, &QPushButton::clicked, edit, &AttachmentsEdit::restore);
            //connect(edit, &AttachmentsEdit::returnPressed, this, &MainWindow::handleReturnPressed);
            m_ui->stackedWidget->insertWidget(widgetIndex, edit);
        }
        ++widgetIndex;
    }
    // remove surplus edits
    while(widgetIndex < m_ui->stackedWidget->count()) {
        QWidget *toRemove = m_ui->stackedWidget->widget(widgetIndex);
        m_ui->stackedWidget->removeWidget(toRemove);
        delete toRemove;
    }
}

/*!
 * \brief Updates the items and the visibility of the tag selection combo box.
 * \remarks Tag edits should have been updated before since this method uses the
 *          tag edits to generate the labels.
 */
void MainWindow::updateTagSelectionComboBox()
{
    if(m_fileInfo.isOpen()) {
        // memorize the index of the previously selected edit
        int previouslySelectedEditIndex = m_ui->tagSelectionComboBox->currentIndex();
        // clear old entries and create new labels
        m_ui->tagSelectionComboBox->clear();
        bool haveTargetInfo = false;
        for(int index = 0, count = m_ui->stackedWidget->count(); index < count; ++index) {
            QString label;
            if(TagEdit *edit = qobject_cast<TagEdit *>(m_ui->stackedWidget->widget(index))) {
                if(!edit->tags().isEmpty()) {
                    label = edit->generateLabel();
                    haveTargetInfo |= !edit->tags().at(0)->target().isEmpty();
                }
            } else if(qobject_cast<AttachmentsEdit *>(m_ui->stackedWidget->widget(index))) {
                static const QString attachmentsLabel = tr("Attachments");
                label = attachmentsLabel;
            }
            m_ui->tagSelectionComboBox->addItem(label);
        }
        // set visibility
        m_ui->tagSelectionComboBox->setHidden(Settings::hideTagSelectionComboBox() && m_ui->tagSelectionComboBox->count() <= 1 && !haveTargetInfo);
        // restore selected index
        if(previouslySelectedEditIndex >= 0 && previouslySelectedEditIndex < m_ui->tagSelectionComboBox->count()) {
            m_ui->tagSelectionComboBox->setCurrentIndex(previouslySelectedEditIndex);
        }
    }
}

/*!
 * \brief Updates the status of the relevant widgets (enabled/disabled, visible/hidden) according to the
 *        current "file status" (opened/closed, has tags/no tags).
 */
void MainWindow::updateUiStatus()
{
    bool opened = m_fileInfo.isOpen();
    bool hasTag = opened && m_tags.size();
    // notification widgets
    m_ui->parsingNotificationWidget->setVisible(opened);
    m_ui->makingNotificationWidget->setVisible(opened && (m_makingResultsAvailable));
    // document title widget
    m_ui->docTitleWidget->setVisible(opened && m_fileInfo.container() && m_fileInfo.container()->supportsTitle());
    // buttons and actions to save, delete, close
    m_ui->saveButton->setEnabled(opened);
    m_ui->deleteTagsButton->setEnabled(hasTag);
    m_ui->actionSave->setEnabled(opened);
    m_ui->actionDelete_all_tags->setEnabled(hasTag);
    m_ui->actionSave_file_information->setEnabled(opened);
    m_ui->actionClose->setEnabled(opened);
    m_ui->actionReload->setEnabled(opened);
    m_ui->actionExternalPlayer->setEnabled(opened);
    m_ui->buttonsWidget->setEnabled(opened);
    // clear and restore buttons
    m_ui->clearEntriesPushButton->setEnabled(hasTag);
    m_ui->restoreEntriesPushButton->setEnabled(hasTag);
    m_ui->clearEntriesPushButton->setEnabled(hasTag);
    m_ui->restoreEntriesPushButton->setEnabled(hasTag);
    // tag management button
    m_ui->tagOptionsPushButton->setEnabled(opened);
    // stacked widget containering edits and selection
    m_ui->tagSelectionComboBox->setEnabled(hasTag);
    // visibility of m_ui->tagSelectionComboBox is set within updateTagEdits
    m_ui->stackedWidget->setEnabled(hasTag);
    // webview
    m_infoWebView->setEnabled(opened);
    // next button
    bool nextFileAvailable = false;
    if(opened) {
        QModelIndexList selectedIndexes = m_ui->filesTreeView->selectionModel()->selectedRows();
        if(selectedIndexes.count() == 1) {
            const QModelIndex &selected = selectedIndexes.at(0);
            nextFileAvailable = selected.sibling(selected.row() + 1, selected.column()).isValid();
        }
    }
    m_ui->nextButton->setEnabled(nextFileAvailable);
    // window title
    setWindowTitle(Dialogs::generateWindowTitle(m_fileInfo.isOpen() ? DocumentStatus::Saved : DocumentStatus::NoDocument, m_currentPath));
}

/*!
 * \brief Updates the "tag management menu".
 */
void MainWindow::updateTagManagementMenu()
{
    m_addTagMenu->clear();
    m_removeTagMenu->clear();
    m_changeTargetMenu->clear();
    if(m_fileInfo.isOpen()) {
        // add "Add tag" actions
        if(m_fileInfo.container()) {
            // there is a container object which might be able to create tags
            QString label;
            if(m_fileInfo.containerFormat() == ContainerFormat::Matroska) {
                // tag format supports targets (Matroska tags are currently the only tag format supporting targets.)
                label = tr("Matroska tag");
                connect(m_addTagMenu->addAction(label), &QAction::triggered, std::bind(&MainWindow::addTag, this, [this] (MediaFileInfo &file) -> Media::Tag * {
                            if(file.container()) {
                                EnterTargetDialog targetDlg(this);
                                targetDlg.setTarget(TagTarget(50), &this->m_fileInfo);
                                if(targetDlg.exec() == QDialog::Accepted) {
                                    return file.container()->createTag(targetDlg.target());
                                }
                            }
                            return nullptr;
                        }));                
            } else {
                // tag format does not support targets
                if(!m_fileInfo.container()->tagCount()) {
                    switch(m_fileInfo.containerFormat()) {
                    case ContainerFormat::Mp4:
                        label = tr("MP4/iTunes tag");
                        break;
                    case ContainerFormat::Ogg:
                        label = tr("Vorbis comment");
                        break;
                    default:
                        label = tr("Tag");
                    }
                    connect(m_addTagMenu->addAction(label), &QAction::triggered, std::bind(&MainWindow::addTag, this, [] (MediaFileInfo &file) {
                                return file.container() ? file.container()->createTag() : nullptr;
                            }));
                }
            }
        } else {
            // there is no container object; creation of ID3 tags is possible
            if(!m_fileInfo.hasId3v1Tag()) {
                connect(m_addTagMenu->addAction(tr("ID3v1 tag")), &QAction::triggered, std::bind(&MainWindow::addTag, this, [] (MediaFileInfo &file) {
                            return file.createId3v1Tag();
                        }));
            }
            if(!m_fileInfo.hasId3v2Tag()) {
                connect(m_addTagMenu->addAction(tr("ID3v2 tag")), &QAction::triggered, std::bind(&MainWindow::addTag, this, [] (MediaFileInfo &file) {
                            return file.createId3v2Tag();
                        }));
            }
        }
        // add "Remove tag" and "Change target" actions
        for(Tag *tag : m_tags) {
            connect(m_removeTagMenu->addAction(QString::fromLocal8Bit(tag->toString().c_str())), &QAction::triggered, std::bind(&MainWindow::removeTag, this, tag));
            if(tag->supportsTarget()) {
                connect(m_changeTargetMenu->addAction(QString::fromLocal8Bit(tag->toString().c_str())), &QAction::triggered, std::bind(&MainWindow::changeTarget, this, tag));
            }
        }
    }
    m_addTagMenu->setEnabled(!m_addTagMenu->actions().empty());
    m_removeTagMenu->setEnabled(!m_removeTagMenu->actions().empty());
    m_changeTargetMenu->setEnabled(!m_changeTargetMenu->actions().empty());
}

/*!
 * \brief Inserts the title from the filename keeping a possibly available title from the tags.
 * \remarks Does nothing if there are no tags assigned and if this feature is not enabled in the settings.
 */
void MainWindow::insertTitleFromFilename()
{
    if(!m_tags.empty() && Settings::insertTitleFromFilename()) {
        QString title;
        int trackNum;
        parseFileName(QString::fromLocal8Bit(m_fileInfo.fileName().c_str()), title, trackNum);
        TagValue titleValue = qstringToTagValue(title, TagTextEncoding::Utf16LittleEndian);
        foreachTagEdit([&titleValue] (TagEdit *edit) {
            edit->setValue(KnownField::Title, titleValue, PreviousValueHandling::Keep);
        });
    }
}

/*!
 * \brief Updates the info web view to show information about the
 *        currently opened file.
 */
void MainWindow::updateInfoWebView()
{
    if(m_fileInfo.isOpen()) {
        m_fileInfoHtml = HtmlInfo::generateInfo(m_fileInfo, m_originalNotifications);
        m_infoWebView->setContent(m_fileInfoHtml, QStringLiteral("application/xhtml+xml"));
    } else {
        m_infoWebView->setUrl(QStringLiteral("about:blank"));
    }
}

/*!
 * \brief Shows the context menu for the info web view.
 */
void MainWindow::showInfoWebViewContextMenu(const QPoint &)
{
    QAction copyAction(QIcon::fromTheme(QStringLiteral("edit-copy")), tr("Copy"), nullptr);
    copyAction.setDisabled(m_infoWebView->selectedText().isEmpty());
    connect(&copyAction, &QAction::triggered, this, &MainWindow::copyInfoWebViewSelection);
    QMenu menu;
    menu.addAction(&copyAction);
    menu.exec(QCursor::pos());
}

/*!
 * \brief Copies the current selection of the info web view.
 */
void MainWindow::copyInfoWebViewSelection()
{
    QApplication::clipboard()->setText(m_infoWebView->selectedText());
}

/*!
 * \brief Spawns an external player for the current file.
 */
void MainWindow::spawnExternalPlayer()
{
    if(!m_currentPath.isEmpty()) {
        QDesktopServices::openUrl(QStringLiteral("file://") + m_currentPath);
    } else {
        m_ui->statusBar->showMessage(tr("No file opened."));
    }
}

/*!
 * \brief Calls the specified \a function for each of the currently present tag edits.
 */
void MainWindow::foreachTagEdit(const std::function<void (TagEdit *)> &function)
{
    for(int i = 0, count = m_ui->stackedWidget->count(); i < count; ++i) {
        if(auto *edit = qobject_cast<TagEdit *>(m_ui->stackedWidget->widget(i))) {
            function(edit);
        }
    }
}

/*!
 * \brief Opens and parses a file using another thread.
 *
 * Shows its tags and general information using the showFile() method.
 *
 * \param path Specifies the \a path of the file.
 * \param forceRefresh Specifies whether the file should be reparsed if it is already opened.
 */
bool MainWindow::startParsing(const QString &path, bool forceRefresh)
{
    // check if file is current file
    bool sameFile = m_currentPath == path;
    if(!forceRefresh && sameFile) {
        return true;
    }
    if(!m_fileOperationMutex.try_lock()) {
        m_ui->statusBar->showMessage(tr("Unable to load the selected file \"%1\" because the current process hasn't finished yet.").arg(path));
        return false;
    }
    lock_guard<mutex> guard(m_fileOperationMutex, adopt_lock);
    // clear previous results and status
    m_tags.clear();
    m_fileInfo.clearParsingResults();
    m_fileInfo.invalidateStatus();
    m_fileInfo.invalidateNotifications();
    if(!sameFile) {
        // close last file if possibly open
        m_fileInfo.close();
        // set path of file info
        m_currentPath = path;
        m_fileInfo.setPath(path.toLocal8Bit().data());
        // update directory
        m_lastDir = m_currentDir;
        m_currentDir = QString::fromLocal8Bit(m_fileInfo.containingDirectory().c_str());
    }
    // update availability of making results
    m_makingResultsAvailable &= sameFile;
    if(!m_makingResultsAvailable) {
        m_originalNotifications.clear();
    }
    // show filename
    m_ui->fileNameLabel->setText(QString::fromLocal8Bit(m_fileInfo.fileName().c_str()));
    // define function to parse the file
    auto startThread = [this, sameFile] {
        m_fileOperationMutex.lock();
        char result;
        try {
            if(sameFile) {
                m_fileInfo.reopen();
            }
            m_fileInfo.setForceFullParse(Settings::forceFullParse());
            m_fileInfo.parseEverything();
            result = ParsingSuccessful;
        } catch(Failure &) {
            // the file has been opened; parsing notifications will be shown in the info box
            result = FatalParsingError;
        } catch(ios_base::failure &) {
            // the file could not be opened because an IO error occured
            m_fileInfo.close(); // ensure file is closed
            result = IoError;
        }
        m_fileInfo.unregisterAllCallbacks();
        QMetaObject::invokeMethod(this, "showFile", Qt::QueuedConnection, Q_ARG(char, result));
        // showFile() will unlock the mutex!
    };
    m_fileInfo.unregisterAllCallbacks();
    //m_fileInfo.registerCallback(showProgress); can't show progress yet
    // use another thread to perform the operation
    std::thread thr(startThread);
    thr.detach();
    // inform user
    static const QString statusMsg(tr("The file is beeing parsed ..."));
    m_ui->parsingNotificationWidget->setNotificationType(NotificationType::Progress);
    m_ui->parsingNotificationWidget->setText(statusMsg);
    m_ui->parsingNotificationWidget->setVisible(true); // ensure widget is visible!
    m_ui->statusBar->showMessage(statusMsg);
    return true;
}

/*!
 * \brief Reparses the current file.
 */
bool MainWindow::reparseFile()
{
    if(!m_fileOperationMutex.try_lock()) {
        m_ui->statusBar->showMessage(tr("Unable to reload the file because the current process hasn't finished yet."));
        return false;
    }
    {
        lock_guard<mutex> guard(m_fileOperationMutex, adopt_lock);
        if(!m_fileInfo.isOpen() || m_currentPath.isEmpty()) {
            QMessageBox::warning(this, windowTitle(), tr("Currently is not file opened."));
            return false;
        }
    }
    return startParsing(m_currentPath, true);
}

/*!
 * \brief Shows the current file info (technical info, tags, ...).
 * This private slot is invoked from the thread which performed the
 * parsing operation using Qt::QueuedConnection.
 * \param result Specifies whether the file could be load sucessfully.
 * \remarks Expects m_fileOperationMutex to be locked!
 */
void MainWindow::showFile(char result)
{    
    lock_guard<mutex> guard(m_fileOperationMutex, adopt_lock);
    if(result == IoError) {
        // update status
        updateUiStatus();
        static const QString statusMsg(tr("The file could not be opened because an IO error occurred."));
        QMessageBox::critical(this, windowTitle(), statusMsg);
        m_ui->statusBar->showMessage(statusMsg);
    } else {
        // update webview
        updateInfoWebView();
        // show parsing status/result using parsing notification widget
        auto worstNotificationType = m_fileInfo.worstNotificationTypeIncludingRelatedObjects();
        if(worstNotificationType >= Media::NotificationType::Critical) {
            // we catched no exception, but there are critical notifications
            // -> treat critical notifications as fatal parsing errors
            result = LoadingResult::FatalParsingError;
        }
        switch(result) {
        case ParsingSuccessful:
            m_ui->parsingNotificationWidget->setNotificationType(NotificationType::TaskComplete);
            m_ui->parsingNotificationWidget->setText(tr("File could be parsed correctly."));
            break;
        default:
            m_ui->parsingNotificationWidget->setNotificationType(NotificationType::Critical);
            m_ui->parsingNotificationWidget->setText(tr("File couldn't be parsed correctly."));
        }
        switch(worstNotificationType) {
        case Media::NotificationType::Critical:
            m_ui->parsingNotificationWidget->setNotificationType(NotificationType::Critical);
            m_ui->parsingNotificationWidget->appendLine(tr("There are critical parsing notifications."));
            break;
        case Media::NotificationType::Warning:
            m_ui->parsingNotificationWidget->setNotificationType(NotificationType::Warning);
            m_ui->parsingNotificationWidget->appendLine(tr("There are warnings."));
            break;
        default:
            ;
        }
        // load existing tags
        m_tags.clear();
        m_fileInfo.tags(m_tags);
        // show notification if there is currently no existing tag(s) could be found
        if(!m_tags.size()) {
            m_ui->parsingNotificationWidget->appendLine(tr("There is no (supported) tag assigned."));
            if(!m_fileInfo.areTagsSupported()) {
                m_ui->parsingNotificationWidget->setNotificationType(NotificationType::Warning);
                m_ui->parsingNotificationWidget->appendLine(tr("File format is not supported (an ID3 tag can be added anyways)."));
            }
        }
        // create appropriate tags according to file type and user preferences when automatic tag management is enabled
        if(Settings::autoTagManagement()) {
            if(!m_fileInfo.createAppropriateTags(false, Settings::id3v1usage(), Settings::id3v2usage(), Settings::mergeMultipleSuccessiveId3v2Tags(),
                                             Settings::keepVersionOfExistingId3v2Tag(), Settings::id3v2versionToBeUsed())) {
                if(confirmCreationOfId3TagForUnsupportedFile()) {
                    m_fileInfo.createAppropriateTags(true, Settings::id3v1usage(), Settings::id3v2usage(), Settings::mergeMultipleSuccessiveId3v2Tags(),
                                                             Settings::keepVersionOfExistingId3v2Tag(), Settings::id3v2versionToBeUsed());
                }
            }
        }
        // reload tags
        m_tags.clear();
        m_fileInfo.tags(m_tags);
        // update file watcher
        m_fileWatcher->addPath(m_currentPath);
        m_fileChangedOnDisk = false;
        // update related widgets
        updateDocumentTitleEdits();
        updateTagEditsAndAttachmentEdits();
        updateTagSelectionComboBox();
        updateTagManagementMenu();
        insertTitleFromFilename();
        // update status
        m_ui->statusBar->showMessage(tr("The file %1 has been opened.").arg(QString::fromLocal8Bit(m_fileInfo.fileName().c_str())));
        updateUiStatus();
    }
}

/*!
 * \brief Applies all entries and starts saving.
 * \sa startSaving()
 */
bool MainWindow::applyEntriesAndSaveChangings()
{
    {
        if(!m_fileOperationMutex.try_lock()) {
            static const QString statusMsg(tr("Unable to apply the entered tags to the file because the current process hasn't finished yet."));
            m_ui->makingNotificationWidget->setText(statusMsg);
            m_ui->statusBar->showMessage(statusMsg);
            return false;
        }
        lock_guard<mutex> guard(m_fileOperationMutex, adopt_lock);
        m_ui->makingNotificationWidget->setNotificationType(NotificationType::Information);
        m_ui->makingNotificationWidget->setNotificationSubject(NotificationSubject::Saving);
        m_ui->makingNotificationWidget->setHidden(false);
        m_makingResultsAvailable = true;
        if(m_fileInfo.isOpen()) {
            // apply titles
            if(AbstractContainer *container = m_fileInfo.container()) {
                if(container->supportsTitle()) {
                    QLayout *docTitleLayout = m_ui->docTitleWidget->layout();
                    for(int i = 0, count = min<int>(docTitleLayout->count() - 1, container->segmentCount()); i < count; ++i) {
                        container->setTitle(static_cast<ClearLineEdit *>(docTitleLayout->itemAt(i + 1)->widget())->text().toUtf8().data(), i);
                    }
                }
            }
            // apply all tags
            foreachTagEdit([] (TagEdit *edit) {edit->apply();});
            static const QString statusMsg(tr("Saving tags ..."));
            m_ui->makingNotificationWidget->setNotificationSubject(NotificationSubject::None);
            m_ui->makingNotificationWidget->setNotificationType(NotificationType::Progress);
            m_ui->makingNotificationWidget->setText(statusMsg);
            m_ui->statusBar->showMessage(statusMsg);
        } else {
            QString statusMsg = tr("No file has been opened.");
            m_ui->makingNotificationWidget->setText(statusMsg);
            QMessageBox::warning(this, QApplication::applicationName(), statusMsg);
            return false;
        }
    }
    return startSaving();
}

/*!
 * \brief Deletes all tags and starts saving.
 * \sa startSaving()
 */
bool MainWindow::deleteAllTagsAndSave()
{
    {
        if(!m_fileOperationMutex.try_lock()) {
            static const QString statusMsg(tr("Unable to delete all tags from the file because the current process hasn't been finished yet."));
            m_ui->makingNotificationWidget->setText(statusMsg);
            m_ui->statusBar->showMessage(statusMsg);
            return false;
        }
        lock_guard<mutex> guard(m_fileOperationMutex, adopt_lock);
        if(Settings::askBeforeDeleting()) {
            QMessageBox msgBox(this);
            msgBox.setText(tr("Do you really want to delete all tags from the file?"));
            msgBox.setIcon(QMessageBox::Warning);
            msgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
            msgBox.setDefaultButton(QMessageBox::No);
#if QT_VERSION >= 0x050200
            QCheckBox *checkBox = new QCheckBox(&msgBox);
            checkBox->setText(tr("don't show this message again"));
            msgBox.setCheckBox(checkBox);
#endif
            int res = msgBox.exec();
#if QT_VERSION >= 0x050200
            if(checkBox->isChecked()) {
                Settings::askBeforeDeleting() = false;
            }
#endif
            if(res != QMessageBox::Yes) {
                return false;
            }
        }
        m_ui->makingNotificationWidget->setNotificationSubject(NotificationSubject::Saving);
        m_ui->makingNotificationWidget->setNotificationType(NotificationType::Information);
        m_ui->makingNotificationWidget->setHidden(false);
        m_makingResultsAvailable = true;
        if(m_fileInfo.isOpen()) {
            if(m_fileInfo.hasAnyTag()) {
                foreachTagEdit([] (TagEdit *edit) {edit->clear();});
                m_fileInfo.removeAllTags();
                m_ui->makingNotificationWidget->setNotificationSubject(NotificationSubject::None);
                m_ui->makingNotificationWidget->setNotificationType(NotificationType::Progress);
                static const QString statusMsg(tr("Deleting all tags ..."));
                m_ui->makingNotificationWidget->setText(statusMsg);
                m_ui->statusBar->showMessage(statusMsg);
            } else {
                m_ui->makingNotificationWidget->setText(tr("The selected file stores no tag (at least no supported), so there is nothing to delete."));
                return false;
            }
        } else {
            m_ui->makingNotificationWidget->setText(tr("No file has been opened, so no tags can be deleted."));
            return false;
        }
    }
    return startSaving();
}

/*!
 * \brief Starts saving. This method is called by applyEntriesAndSaveChangings() and deleteAllTagsAndSave().
 * The actual process is performed in another thread.
 * \remarks Will start a new thread to perform the operation. Then showSavingResult() is called
 *          using Qt::QueuedConnection in the main thread. m_fileOperationMutex will remain locked when saving is
 *          finished and will be unlocked in showSavingResult(). This way any method which might be called after
 *          the operation thread ends and before the invokation of showSavingResult() will see a locked mutex and
 *          hence not mutate the current file.
 */
bool MainWindow::startSaving()
{
    if(!m_fileOperationMutex.try_lock()) {
        static const QString errorMsg(tr("Unable to start saving process because there an other process hasn't finished yet."));
        m_ui->statusBar->showMessage(errorMsg);
        QMessageBox::warning(this, QApplication::applicationName(), errorMsg);
        return false;
    }
    lock_guard<mutex> guard(m_fileOperationMutex, adopt_lock);
    // tags might get invalidated
    m_tags.clear();
    foreachTagEdit([] (TagEdit *edit) { edit->setTag(nullptr, false); });
    // show abort button
    m_ui->abortButton->setHidden(false);
    m_ui->abortButton->setEnabled(true);
    m_abortClicked = false;
    // remove current path from file watcher
    m_fileWatcher->removePath(m_currentPath);
    // use current configuration
    m_fileInfo.setForceRewrite(Settings::forceRewrite());
    m_fileInfo.setTagPosition(Settings::preferredTagPosition());
    m_fileInfo.setForceTagPosition(Settings::forceTagPosition());
    m_fileInfo.setIndexPosition(Settings::preferredIndexPosition());
    m_fileInfo.setForceIndexPosition(Settings::forceIndexPosition());
    m_fileInfo.setMinPadding(Settings::minPadding());
    m_fileInfo.setMaxPadding(Settings::maxPadding());
    m_fileInfo.setPreferredPadding(Settings::preferredPadding());
    // define functions to show the saving progress and to actually applying the changes
    auto showProgress = [this] (StatusProvider &sender) -> void {
        QMetaObject::invokeMethod(m_ui->makingNotificationWidget, "setPercentage", Qt::QueuedConnection, Q_ARG(int, static_cast<int>(sender.currentPercentage() * 100.0)));
        if(m_abortClicked) {
            QMetaObject::invokeMethod(m_ui->makingNotificationWidget, "setText", Qt::QueuedConnection, Q_ARG(QString, tr("Cancelling ...")));
            m_fileInfo.tryToAbort();
        } else {
            QMetaObject::invokeMethod(m_ui->makingNotificationWidget, "setText", Qt::QueuedConnection, Q_ARG(QString, QString::fromStdString(sender.currentStatus())));
        }
    };
    auto startThread = [this] {
        m_fileOperationMutex.lock();
        bool processingError = false, ioError = false;
        try {
            m_fileInfo.applyChanges();
        } catch(Failure &) {
            processingError = true;
        } catch(ios_base::failure &) {
            ioError = true;
        }
        m_fileInfo.unregisterAllCallbacks();
        QMetaObject::invokeMethod(this, "showSavingResult", Qt::QueuedConnection, Q_ARG(bool, processingError), Q_ARG(bool, ioError));
        // showSavingResult() will unlock the mutex!
    };
    m_fileInfo.unregisterAllCallbacks();
    m_fileInfo.registerCallback(showProgress);
    // use another thread to perform the operation
    std::thread thr(startThread);
    thr.detach();
    return true;
}

/*!
 * \brief Shows the saving results.
 * This private slot is invoked from the thread which performed the
 * saving operation using Qt::QueuedConnection.
 * \param sucess Specifies whether the file could be saved sucessfully.
 * \remarks Expects m_fileOperationMutex to be locked!
 */
void MainWindow::showSavingResult(bool processingError, bool ioError)
{
    m_ui->abortButton->setHidden(true);
    m_ui->makingNotificationWidget->setNotificationType(NotificationType::TaskComplete);
    m_ui->makingNotificationWidget->setNotificationSubject(NotificationSubject::Saving);
    m_ui->makingNotificationWidget->setPercentage(-1);
    m_ui->makingNotificationWidget->setHidden(false);
    m_makingResultsAvailable = true;
    m_originalNotifications = m_fileInfo.gatherRelatedNotifications();
    if(!processingError && !ioError) {
        // display status messages
        QString statusMsg;
        size_t critical = 0;
        size_t warnings = 0;
        for(const Notification &notification : m_originalNotifications) {
            switch(notification.type()) {
            case Media::NotificationType::Critical:
                ++critical;
                break;
            case Media::NotificationType::Warning:
                ++warnings;
                break;
            default:
                ;
            }
        }
        if(warnings || critical) {
            if(critical) {
                statusMsg = tr("The tags have been saved, but there is/are %1 warning(s) ", 0, warnings).arg(warnings);
                statusMsg.append(tr("and %1 error(s).", 0, critical).arg(critical));
            } else {
                statusMsg = tr("The tags have been saved, but there is/are %1 warning(s).", 0, warnings).arg(warnings);
            }
            m_ui->makingNotificationWidget->setNotificationType(critical > 0 ? NotificationType::Critical : NotificationType::Warning);

        } else {
            statusMsg = tr("The tags have been saved.");
        }
        m_ui->makingNotificationWidget->setText(statusMsg);
        m_ui->statusBar->showMessage(statusMsg);
        m_fileOperationMutex.unlock();
        // ensure the current file is still selected
        QModelIndex index = m_fileFilterModel->mapFromSource(m_fileModel->index(QString::fromStdString(m_fileInfo.path())));
        if(index.isValid()) {
            m_ui->filesTreeView->selectionModel()->setCurrentIndex(index, QItemSelectionModel::Rows | QItemSelectionModel::ClearAndSelect);
        }
        activateWindow(); // ensure this is the active window
        // update info model/show next file
        bool showNextFile = false;
        if(critical < 1 && !m_nextFilePath.isEmpty()) { // do not show next file if there are critical notifications
            QModelIndex next = m_fileModel->index(m_nextFilePath);
            m_nextFilePath.clear();
            if(next.isValid()) {
                QItemSelectionModel *selectionModel = m_ui->filesTreeView->selectionModel();
                selectionModel->setCurrentIndex(m_fileFilterModel->mapFromSource(next), QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Rows);
                showNextFile = true;
            } else {
                static const QString errormsg(tr("Unable to show the next file because it can't be found anymore."));
                QMessageBox::warning(this, QApplication::applicationName(), errormsg);
                m_ui->statusBar->showMessage(errormsg);
            }
        }
        if(!showNextFile) {
            startParsing(m_currentPath, true);
        }
    } else {
        static const QString processingErrorMsg(tr("The tags couldn't be saved. See the info box for detail."));
        static const QString ioErrorMsg(tr("The tags couldn't be saved because an IO error occured."));
        const auto &errorMsg = ioError ? ioErrorMsg : processingErrorMsg;
        QMessageBox::warning(this, QApplication::applicationName(), errorMsg);
        m_ui->statusBar->showMessage(errorMsg);
        m_ui->makingNotificationWidget->setText(errorMsg);
        m_ui->makingNotificationWidget->setNotificationType(NotificationType::Critical);
        m_fileOperationMutex.unlock();
        startParsing(m_currentPath, true);
    }
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
        //connect(m_settingsDlg, &SettingsDialog::accept, this, &MainWindow::applySettingsFromDialog);
    }
    if(m_settingsDlg->exec() == QDialog::Accepted) {
        applySettingsFromDialog();
    }
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
 * \brief Asks the user whether an ID3 tag should be add to a not supported container format and returns the result.
 */
bool MainWindow::confirmCreationOfId3TagForUnsupportedFile()
{
    QMessageBox msgBox(this);
    msgBox.setWindowTitle(tr("Automatic tag management"));
    msgBox.setText(tr("The container format of the selected file is not supported. The file can be treated as MP3 file (an ID3 tag according to the settings will be created). This might break the file. Do you want to continue?"));
    msgBox.setIcon(QMessageBox::Warning);
    msgBox.addButton(tr("Treat file as MP3 file"), QMessageBox::AcceptRole);
    msgBox.addButton(tr("Abort"), QMessageBox::RejectRole);
    return msgBox.exec() == 0;
}

/*!
 * \brief Selects the next file.
 */
void MainWindow::selectNextFile()
{
    QItemSelectionModel *selectionModel = m_ui->filesTreeView->selectionModel();
    QModelIndexList selectedIndexes = selectionModel->selectedIndexes();
    if(selectedIndexes.count() >= 1) {
        const QModelIndex &selectedIndex = selectedIndexes.at(0);
        QModelIndex next;
        if(selectionModel->model()->hasChildren(selectedIndex)) { // returns true for directories
            if(m_fileFilterModel->canFetchMore(selectedIndex)) { // files and subdirectories have to be fetched
                // fetchMore will return immediatly because QFileSystemModel seems to use an
                // extra thread to fetch files and directories. That's why I select the next file
                // in this case when the rowsInserted signal is emitted.
                auto conn = std::make_shared<QMetaObject::Connection>();
                *conn = connect(m_fileFilterModel, &QAbstractItemModel::rowsInserted, [this, selectedIndex, conn] (const QModelIndex &parent, int, int) {
                    disconnect(*conn);
                    if(parent == selectedIndex) {
                        QModelIndex next = m_fileFilterModel->index(0, 0, parent);
                        if(next.isValid()) {
                            m_ui->filesTreeView->selectionModel()->setCurrentIndex(next, QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Rows);
                        }
                    }
                });
                m_fileModel->fetchMore(m_fileFilterModel->mapToSource(selectedIndex));
                return;
            } else { // files and subdirectories have been fetched already
                next = selectedIndex.child(0, selectedIndex.column());
            }
        }
        if(!next.isValid()) {
            next = selectedIndex.sibling(selectedIndex.row() + 1, selectedIndex.column());
        }
        if(!next.isValid()) {
            QModelIndex parent = selectedIndex.parent();
            if(parent.isValid()) {
                next = parent.sibling(parent.row() + 1, parent.column());
            }
        }
        if(next.isValid()) {
            selectionModel->setCurrentIndex(next, QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Rows);
        }
    }
}

/*!
 * \brief Shows an open file dialog and opens the selected file.
 */
void MainWindow::showOpenFileDlg()
{
    QString path = QFileDialog::getOpenFileName(this, QApplication::applicationName());
    if(!path.isEmpty()) {
        startParsing(path);
    }
}

/*!
 * \brief This slot is connected to the fileChanged() signal of the file info watcher.
 */
void MainWindow::fileChangedOnDisk(const QString &path)
{
    if(!m_fileChangedOnDisk && m_fileInfo.isOpen() && path == m_currentPath) {
        auto &notifyWidget = *m_ui->parsingNotificationWidget;
        notifyWidget.appendLine(tr("The currently opened file changed on the disk."));
        notifyWidget.setNotificationType(notifyWidget.notificationType() == NotificationType::Critical ? NotificationType::Critical : NotificationType::Warning);
        m_fileChangedOnDisk = true;
    }
}

/*!
 * \brief Closes the currently opened file and disables all related widgets.
 */
void MainWindow::closeFile()
{
    if(!m_fileOperationMutex.try_lock()) {
        m_ui->statusBar->showMessage("Unable to close the file because the current process hasn't been finished yet.");
        return;
    }
    lock_guard<mutex> guard(m_fileOperationMutex, adopt_lock);
    // close file
    m_fileInfo.close();
    // remove current path from file watcher
    m_fileWatcher->removePath(m_currentPath);
    // update ui
    m_ui->statusBar->showMessage("The file has been closed.");
    updateUiStatus();
}

/*!
 * \brief Saves the file information generated to be displayed in the info web view in a file.
 */
void MainWindow::saveFileInformation()
{
    if(!m_fileOperationMutex.try_lock()) {
        m_ui->statusBar->showMessage("Unable to save file information because the current process hasn't been finished yet.");
        return;
    }
    lock_guard<mutex> guard(m_fileOperationMutex, adopt_lock);
    if(m_fileInfo.isOpen() && m_fileInfoHtml.size()) {
        QString path = QFileDialog::getSaveFileName(this, windowTitle());
        if(!path.isEmpty()) {
            QFile file(path);
            if(file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
                QTextStream stream(&file);
                stream << m_fileInfoHtml;
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
}

/*!
 * \brief This private slot is invoked the the return key has been pressed in a tag edit.
 *
 * The file will be saved and then the next opened if the user selected that option.
 */
void MainWindow::handleReturnPressed()
{
    if(Settings::saveAndShowNextOnEnter() && m_fileInfo.isOpen()) {
        saveAndShowNextFile();
    }
}

void MainWindow::handleKeepPreviousValuesActionTriggered(QAction *action)
{
    if(action == m_ui->actionKeep_previous_values_never) {
        Settings::adoptFields() = Settings::AdoptFields::Never;
    } else if(action == m_ui->actionKeep_previous_values_within_same_dir) {
        Settings::adoptFields() = Settings::AdoptFields::WithinDirectory;
    } else if(action == m_ui->actionKeep_previous_values_always) {
        Settings::adoptFields() = Settings::AdoptFields::Always;
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
        QModelIndex index = m_fileFilterModel->mapFromSource(m_fileModel->index(m_ui->pathLineEdit->text()));
        if(index.isValid()) {
            m_ui->filesTreeView->selectionModel()->setCurrentIndex(index, QItemSelectionModel::Select);
        }
    }
    if(m_fileModel->isReadOnly() != Settings::fileBrowserReadOnly()) {
        m_fileModel->setReadOnly(Settings::fileBrowserReadOnly());
    }
    switch(Settings::adoptFields()) {
    case Settings::AdoptFields::Never:
        m_ui->actionKeep_previous_values_never->setChecked(true);
        break;
    case Settings::AdoptFields::WithinDirectory:
        m_ui->actionKeep_previous_values_within_same_dir->setChecked(true);
        break;
    case Settings::AdoptFields::Always:
        m_ui->actionKeep_previous_values_always->setChecked(true);
        break;
    }
    m_ui->actionManage_tags_automatically_when_loading_file->setChecked(Settings::autoTagManagement());
}

/*!
 * \brief Adds a tag (using the specified \a createTag function) to the currently opened file.
 *
 * Shows an error message if no file is opened. Tag edits, tag management menu und UI status will be updated.
 */
void MainWindow::addTag(const function<Media::Tag *(Media::MediaFileInfo &)> &createTag)
{
    if(!m_fileOperationMutex.try_lock()) {
        m_ui->statusBar->showMessage("Unable to add a tag because the current process hasn't been finished yet.");
        return;
    }
    lock_guard<mutex> guard(m_fileOperationMutex, adopt_lock);
    if(!m_fileInfo.isOpen()) {
        m_ui->statusBar->showMessage("Unable to add a tag because no file is opened.");
        return;
    }
    if(Tag *tag = createTag(m_fileInfo)) {
        if(std::find(m_tags.cbegin(), m_tags.cend(), tag) == m_tags.cend()) {
            m_tags.push_back(tag);
            updateTagEditsAndAttachmentEdits(true, m_tags.size() > 1 ? PreviousValueHandling::Keep : PreviousValueHandling::Auto);
            updateTagSelectionComboBox();
            updateTagManagementMenu();
            updateUiStatus();
            insertTitleFromFilename();
        } else {
            QMessageBox::warning(this, windowTitle(), tr("A tag (with the selected target) already exists."));
        }

    } else {
        QMessageBox::warning(this, windowTitle(), tr("The tag can not be created."));
    }
}

/*!
 * \brief Removes the specified \a tag from the currently opened file.
 *
 * Shows an error message if the removal is (currently) not possible. Tag edits, tag management menu und UI status will be updated.
 */
void MainWindow::removeTag(Tag *tag)
{
    if(tag) {
        if(!m_fileOperationMutex.try_lock()) {
            m_ui->statusBar->showMessage(tr("Unable to remove the tag because the current process hasn't been finished yet."));
            return;
        }
        lock_guard<mutex> guard(m_fileOperationMutex, adopt_lock);
        if(!m_fileInfo.isOpen()) {
            m_ui->statusBar->showMessage(tr("Unable to remove the tag because no file is opened."));
            return;
        }
        if(m_fileInfo.isOpen()) {
            m_fileInfo.removeTag(tag);
            // remove tag from m_tags
            m_tags.erase(remove(m_tags.begin(), m_tags.end(), tag), m_tags.end());
            // remove tag from all TagEdit widgets
            vector<TagEdit *> toRemove;
            for(int index = 0, count = m_ui->stackedWidget->count(); index < count; ++index) {
                TagEdit *edit = qobject_cast<TagEdit *>(m_ui->stackedWidget->widget(index));
                if(edit && edit->tags().contains(tag)) {
                    QList<Tag *> tagsOfEdit = edit->tags();
                    tagsOfEdit.removeAll(tag);
                    if(tagsOfEdit.empty()) {
                        // no tags left in the edit
                        if(m_tags.empty()) {
                            // there are no other tag edits -> just disable the edit
                            edit->setTag(nullptr, false);
                        } else {
                            // there are still other tag edits -> remove the edit
                            toRemove.push_back(edit);
                        }
                    } else {
                        // there are still tags left, reassign remaining tags (keeping the previous values)
                        edit->setPreviousValueHandling(PreviousValueHandling::Keep);
                        edit->setTags(tagsOfEdit, true);
                    }
                }
            }
            // remove TagEdit widgets
            for(TagEdit *edit : toRemove) {
                m_ui->tagSelectionComboBox->removeItem(m_ui->stackedWidget->indexOf(edit));
                m_ui->stackedWidget->removeWidget(edit);
                delete edit;
            }
            // update affected widgets
            updateTagSelectionComboBox();
            updateTagManagementMenu();
            updateUiStatus();
        }
    }
}

/*!
 * \brief Changes the target of the specified \a tag; prompts the user to enter tag target information.
 *
 * Shows an error message if the change is (currently) not possible. Tag management menu and tag selection combo box will be updated.
 */
void MainWindow::changeTarget(Tag *tag)
{
    if(tag) {
        if(!m_fileOperationMutex.try_lock()) {
            m_ui->statusBar->showMessage(tr("Unable to change the target because the current process hasn't been finished yet."));
            return;
        }
        lock_guard<mutex> guard(m_fileOperationMutex, adopt_lock);
        if(!m_fileInfo.isOpen()) {
            m_ui->statusBar->showMessage(tr("Unable to change the target because no file is opened."));
            return;
        }
        if(m_fileInfo.isOpen()) {
            if(tag->supportsTarget()) {
                EnterTargetDialog targetDlg(this);
                targetDlg.setTarget(tag->target(), &m_fileInfo);
                if(targetDlg.exec() == QDialog::Accepted) {
                    tag->setTarget(targetDlg.target());
                    updateTagSelectionComboBox();
                    updateTagManagementMenu();
                }
            } else {
                QMessageBox::warning(this, windowTitle(), tr("Can not change the target of the selected tag because the tag does not support targets."));
            }
        }
    }
}

}
