#include "./tageditorwidget.h"
#include "./notificationlabel.h"
#include "./tagedit.h"
#include "./attachmentsedit.h"
#include "./entertargetdialog.h"

#include "../application/settings.h"
#include "../misc/htmlinfo.h"
#include "../misc/utility.h"

#include "ui_tageditorwidget.h"

#include <tagparser/exceptions.h>
#include <tagparser/signature.h>
#include <tagparser/abstractcontainer.h>
#include <tagparser/tag.h>
#include <tagparser/id3/id3v1tag.h>
#include <tagparser/id3/id3v2tag.h>
#include <tagparser/mp4/mp4container.h>
#include <tagparser/mp4/mp4tag.h>
#include <tagparser/matroska/matroskatag.h>

#include <qtutilities/misc/dialogutils.h>
#include <qtutilities/misc/trylocker.h>
#include <qtutilities/misc/adoptlocker.h>
#include <qtutilities/widgets/clearlineedit.h>

#include <c++utilities/conversion/stringconversion.h>
#include <c++utilities/io/path.h>

#include <QMessageBox>
#include <QKeyEvent>
#include <QClipboard>
#include <QFileSystemModel>
#include <QPlainTextEdit>
#include <QMimeData>
#include <QFileSystemWatcher>
#include <QMenu>
#include <QCheckBox>
#include <QtConcurrent>
#if defined(TAGEDITOR_NO_WEBVIEW)
# error "not supported (yet)."
#elif defined(TAGEDITOR_USE_WEBENGINE)
# include <QWebEngineView>
#else
# include <QWebView>
#endif

#include <functional>
#include <algorithm>

using namespace std;
using namespace Utility;
using namespace Dialogs;
using namespace Widgets;
using namespace ThreadingUtils;
using namespace Media;

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
 * \class QtGui::TagEditorWidget
 * \brief The TagEditorWidget class provides a widget for tag editing.
 */

/*!
 * \brief Constructs a new tag editor widget.
 */
TagEditorWidget::TagEditorWidget(QWidget *parent) :
    QWidget(parent),
    m_ui(new Ui::TagEditorWidget()),
    m_nextFileAfterSaving(false),
    m_makingResultsAvailable(false),
    m_abortClicked(false)
{
    // setup UI
    m_ui->setupUi(this);
    makeHeading(m_ui->fileNameLabel);
    // setup web view
    m_infoWebView = new WEB_VIEW_PROVIDER(m_ui->tagSplitter);
    m_infoWebView->setAcceptDrops(false);
    m_infoWebView->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(m_infoWebView, &QWidget::customContextMenuRequested, this, &TagEditorWidget::showInfoWebViewContextMenu);
    m_ui->tagSplitter->addWidget(m_infoWebView);
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
    connect(group, &QActionGroup::triggered, this, &TagEditorWidget::handleKeepPreviousValuesActionTriggered);
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
    updateFileStatusStatus();
    m_ui->abortButton->setVisible(false);
    // connect signals and slots, install event filter
    //  buttons: save, delete, next, close
    connect(m_ui->saveButton, &QPushButton::clicked, this, &TagEditorWidget::applyEntriesAndSaveChangings);
    connect(m_ui->deleteTagsButton, &QPushButton::clicked, this, &TagEditorWidget::deleteAllTagsAndSave);
    connect(m_ui->nextButton, &QPushButton::clicked, this, &TagEditorWidget::saveAndShowNextFile);
    connect(m_ui->closeButton, &QPushButton::clicked, this, &TagEditorWidget::closeFile);
    //  misc
    connect(m_ui->abortButton, &QPushButton::clicked, [this] {m_abortClicked = true; m_ui->abortButton->setEnabled(false); });
    connect(m_ui->tagSelectionComboBox, static_cast<void (QComboBox::*)(int)>(&QComboBox::activated), m_ui->stackedWidget, &QStackedWidget::setCurrentIndex);
    connect(m_fileWatcher, &QFileSystemWatcher::fileChanged, this, &TagEditorWidget::fileChangedOnDisk);
    // apply settings
    applySettingsFromDialog();
}

/*!
 * \brief Destroys the tag editor widget.
 */
TagEditorWidget::~TagEditorWidget()
{}

/*!
 * \brief Returns whether the file name is visible at the top of the widget.
 */
bool TagEditorWidget::isFileNameVisible() const
{
    return m_ui->fileNameLabel->isVisible();
}

/*!
 * \brief Sets whether the file name is visible at the top of the widget.
 */
void TagEditorWidget::setFileNameVisible(bool visible)
{
    m_ui->fileNameLabel->setVisible(visible);
}

/*!
 * \brief Returns whether the buttons default save, next, delete and close buttons are visible.
 */
bool TagEditorWidget::areButtonsVisible() const
{
    return m_ui->buttonsWidget->isVisible();
}

/*!
 * \brief Sets whether the buttons default save, next, delete and close buttons are visible.
 */
void TagEditorWidget::setButtonVisible(bool visible)
{
    m_ui->buttonsWidget->setVisible(visible);
    m_ui->line->setVisible(visible);
}

/*!
 * \brief
 *  - Saves the applications settings relating the state of the main window.
 *  - Updates the info webview when the palette changed.
 */
bool TagEditorWidget::event(QEvent *event)
{
    switch(event->type()) {
    case QEvent::PaletteChange:
        updateInfoWebView();
        break;
    case QEvent::DragEnter:
    case QEvent::Drop: {
        auto *dropEvent = static_cast<QDropEvent *>(event);
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
    } default:
        ;
    }
    return QWidget::event(event);
}

/*!
 * \brief Updates the line edits for the document title(s).
 */
void TagEditorWidget::updateDocumentTitleEdits()
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
void TagEditorWidget::updateTagEditsAndAttachmentEdits(bool updateUi, PreviousValueHandling previousValueHandling)
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
            connect(edit, &TagEdit::returnPressed, this, &TagEditorWidget::handleReturnPressed);
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
            //connect(edit, &AttachmentsEdit::returnPressed, this, &TagEditorWidget::handleReturnPressed);
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
void TagEditorWidget::updateTagSelectionComboBox()
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
void TagEditorWidget::updateFileStatusStatus()
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
    m_ui->nextButton->setEnabled(opened);
    m_ui->deleteTagsButton->setEnabled(hasTag);
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
    // inform the main window about the file status change as well
    emit fileStatusChange(opened, hasTag);
}

/*!
 * \brief Updates the "tag management menu".
 */
void TagEditorWidget::updateTagManagementMenu()
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
                connect(m_addTagMenu->addAction(label), &QAction::triggered, std::bind(&TagEditorWidget::addTag, this, [this] (MediaFileInfo &file) -> Media::Tag * {
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
                    connect(m_addTagMenu->addAction(label), &QAction::triggered, std::bind(&TagEditorWidget::addTag, this, [] (MediaFileInfo &file) {
                                return file.container() ? file.container()->createTag() : nullptr;
                            }));
                }
            }
        } else {
            // there is no container object; creation of ID3 tags is possible
            if(!m_fileInfo.hasId3v1Tag()) {
                connect(m_addTagMenu->addAction(tr("ID3v1 tag")), &QAction::triggered, std::bind(&TagEditorWidget::addTag, this, [] (MediaFileInfo &file) {
                            return file.createId3v1Tag();
                        }));
            }
            if(!m_fileInfo.hasId3v2Tag()) {
                connect(m_addTagMenu->addAction(tr("ID3v2 tag")), &QAction::triggered, std::bind(&TagEditorWidget::addTag, this, [] (MediaFileInfo &file) {
                            return file.createId3v2Tag();
                        }));
            }
        }
        // add "Remove tag" and "Change target" actions
        for(Tag *tag : m_tags) {
            connect(m_removeTagMenu->addAction(QString::fromLocal8Bit(tag->toString().c_str())), &QAction::triggered, std::bind(&TagEditorWidget::removeTag, this, tag));
            if(tag->supportsTarget()) {
                connect(m_changeTargetMenu->addAction(QString::fromLocal8Bit(tag->toString().c_str())), &QAction::triggered, std::bind(&TagEditorWidget::changeTarget, this, tag));
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
void TagEditorWidget::insertTitleFromFilename()
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
void TagEditorWidget::updateInfoWebView()
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
void TagEditorWidget::showInfoWebViewContextMenu(const QPoint &)
{
    QAction copyAction(QIcon::fromTheme(QStringLiteral("edit-copy")), tr("Copy"), nullptr);
    copyAction.setDisabled(m_infoWebView->selectedText().isEmpty());
    connect(&copyAction, &QAction::triggered, this, &TagEditorWidget::copyInfoWebViewSelection);
    QMenu menu;
    menu.addAction(&copyAction);
    menu.exec(QCursor::pos());
}

/*!
 * \brief Copies the current selection of the info web view.
 */
void TagEditorWidget::copyInfoWebViewSelection()
{
    QApplication::clipboard()->setText(m_infoWebView->selectedText());
}

/*!
 * \brief Calls the specified \a function for each of the currently present tag edits.
 */
void TagEditorWidget::foreachTagEdit(const std::function<void (TagEdit *)> &function)
{
    for(int i = 0, count = m_ui->stackedWidget->count(); i < count; ++i) {
        if(auto *edit = qobject_cast<TagEdit *>(m_ui->stackedWidget->widget(i))) {
            function(edit);
        }
    }
}

/*!
 * \brief Returns the active tag edit or nullptr if there is currently no acitve tag edit.
 */
TagEdit *TagEditorWidget::activeTagEdit()
{
    return m_fileInfo.isOpen() ? qobject_cast<TagEdit *>(m_ui->stackedWidget->currentWidget()) : nullptr;
}

/*!
 * \brief Opens and parses a file using another thread.
 *
 * Shows its tags and general information using the showFile() method.
 *
 * \param path Specifies the \a path of the file.
 * \param forceRefresh Specifies whether the file should be reparsed if it is already opened.
 */
bool TagEditorWidget::startParsing(const QString &path, bool forceRefresh)
{
    // check if file is current file
    bool sameFile = m_currentPath == path;
    if(!forceRefresh && sameFile) {
        return true;
    }
    TryLocker<> locker(fileOperationMutex());
    if(locker) {
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
        auto startThread = [this] {
            m_fileOperationMutex.lock();
            char result;
            try {
                try {
                    // try to open with write access
                    m_fileInfo.reopen(false);
                } catch(const ios_base::failure &) {
                    // try to open read-only if opening with write access failed
                    m_fileInfo.reopen(true);
                }
                m_fileInfo.setForceFullParse(Settings::forceFullParse());
                m_fileInfo.parseEverything();
                result = ParsingSuccessful;
            } catch(const Failure &) {
                // the file has been opened; parsing notifications will be shown in the info box
                result = FatalParsingError;
            } catch(const ios_base::failure &) {
                // the file could not be opened because an IO error occured
                m_fileInfo.close(); // ensure file is closed
                result = IoError;
            }
            m_fileInfo.unregisterAllCallbacks();
            QMetaObject::invokeMethod(this, "showFile", Qt::QueuedConnection, Q_ARG(char, result));
            // showFile() will unlock the mutex!
        };
        m_fileInfo.unregisterAllCallbacks();
        // perform the operation concurrently
        QtConcurrent::run(startThread);
        // inform user
        static const QString statusMsg(tr("The file is beeing parsed ..."));
        m_ui->parsingNotificationWidget->setNotificationType(NotificationType::Progress);
        m_ui->parsingNotificationWidget->setText(statusMsg);
        m_ui->parsingNotificationWidget->setVisible(true); // ensure widget is visible!
        emit statusMessage(statusMsg);
        return true;
    } else {
        emit statusMessage(tr("Unable to load the selected file \"%1\" because the current process hasn't finished yet.").arg(path));
    }
    return false;
}

/*!
 * \brief Reparses the current file.
 */
bool TagEditorWidget::reparseFile()
{
    {
        TryLocker<> locker(m_fileOperationMutex);
        if(locker) {
            if(!m_fileInfo.isOpen() || m_currentPath.isEmpty()) {
                QMessageBox::warning(this, windowTitle(), tr("Currently is not file opened."));
                return false;
            }
        } else {
            emit statusMessage(tr("Unable to reload the file because the current process hasn't finished yet."));
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
void TagEditorWidget::showFile(char result)
{    
    AdoptLocker<> locker(m_fileOperationMutex);
    if(result == IoError) {
        // update status
        updateFileStatusStatus();
        static const QString statusMsg(tr("The file could not be opened because an IO error occurred."));
        QMessageBox::critical(this, windowTitle(), statusMsg);
        emit statusMessage(statusMsg);
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
        if(worstNotificationType == Media::NotificationType::Critical) {
            m_ui->parsingNotificationWidget->setNotificationType(NotificationType::Critical);
            m_ui->parsingNotificationWidget->appendLine(tr("There are critical parsing notifications."));
        } else if(worstNotificationType == Media::NotificationType::Warning || m_fileInfo.isReadOnly()) {
            m_ui->parsingNotificationWidget->setNotificationType(NotificationType::Warning);
            if(worstNotificationType == Media::NotificationType::Warning) {
                m_ui->parsingNotificationWidget->appendLine(tr("There are warnings."));
            }
            if(m_fileInfo.isReadOnly()) {
                m_ui->parsingNotificationWidget->appendLine(tr("No write access; the file has been opened in read-only mode."));
            }
        }
        // load existing tags
        m_tags.clear();
        m_fileInfo.tags(m_tags);
        // show notification if no existing tag(s) could be found
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
            // tags might have been adjusted -> reload tags
            m_tags.clear();
            m_fileInfo.tags(m_tags);
        }
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
        emit statusMessage(tr("The file %1 has been opened.").arg(m_currentPath));
        updateFileStatusStatus();
    }
}

/*!
 * \brief Invokes saving the current file and - if saving was successful loading the next file.
 */
void TagEditorWidget::saveAndShowNextFile()
{
    m_nextFileAfterSaving = true;
    applyEntriesAndSaveChangings();
}

/*!
 * \brief Applies all entries and starts saving.
 * \sa startSaving()
 */
bool TagEditorWidget::applyEntriesAndSaveChangings()
{
    {
        TryLocker<> locker(m_fileOperationMutex);
        if(locker) {
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
                emit statusMessage(statusMsg);
            } else {
                QString statusMsg(tr("No file has been opened."));
                m_ui->makingNotificationWidget->setText(statusMsg);
                QMessageBox::warning(this, QApplication::applicationName(), statusMsg);
                return false;
            }
        } else {
            static const QString statusMsg(tr("Unable to apply the entered tags to the file because the current process hasn't finished yet."));
            m_ui->makingNotificationWidget->setText(statusMsg);
            emit statusMessage(statusMsg);
            return false;
        }
    }
    return startSaving();
}

/*!
 * \brief Deletes all tags and starts saving.
 * \sa startSaving()
 */
bool TagEditorWidget::deleteAllTagsAndSave()
{
    {
        TryLocker<> locker(m_fileOperationMutex);
        if(locker) {
            if(Settings::askBeforeDeleting()) {
                QMessageBox msgBox(this);
                msgBox.setText(tr("Do you really want to delete all tags from the file?"));
                msgBox.setIcon(QMessageBox::Warning);
                msgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
                msgBox.setDefaultButton(QMessageBox::No);
#if QT_VERSION >= 0x050200
                auto *checkBox = new QCheckBox(&msgBox);
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
                    emit statusMessage(statusMsg);
                } else {
                    m_ui->makingNotificationWidget->setText(tr("The selected file stores no tag (at least no supported), so there is nothing to delete."));
                    return false;
                }
            } else {
                m_ui->makingNotificationWidget->setText(tr("No file has been opened, so no tags can be deleted."));
                return false;
            }
        } else {
            static const QString statusMsg(tr("Unable to delete all tags from the file because the current process hasn't been finished yet."));
            m_ui->makingNotificationWidget->setText(statusMsg);
            emit statusMessage(statusMsg);
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
bool TagEditorWidget::startSaving()
{
    TryLocker<> locker(m_fileOperationMutex);
    if(locker) {
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
            } catch(const Failure &) {
                processingError = true;
            } catch(const ios_base::failure &) {
                ioError = true;
            }
            m_fileInfo.unregisterAllCallbacks();
            QMetaObject::invokeMethod(this, "showSavingResult", Qt::QueuedConnection, Q_ARG(bool, processingError), Q_ARG(bool, ioError));
            // showSavingResult() will unlock the mutex!
        };
        m_fileInfo.unregisterAllCallbacks();
        m_fileInfo.registerCallback(showProgress);
        // use another thread to perform the operation
        QtConcurrent::run(startThread);
        return true;
    } else {
        static const QString errorMsg(tr("Unable to start saving process because there an other process hasn't finished yet."));
        emit statusMessage(errorMsg);
        QMessageBox::warning(this, QApplication::applicationName(), errorMsg);
        return false;
    }
}

/*!
 * \brief Shows the saving results.
 * This private slot is invoked from the thread which performed the
 * saving operation using Qt::QueuedConnection.
 * \param sucess Specifies whether the file could be saved sucessfully.
 * \remarks Expects m_fileOperationMutex to be locked!
 */
void TagEditorWidget::showSavingResult(bool processingError, bool ioError)
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
        size_t critical = 0, warnings = 0;
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
        emit statusMessage(statusMsg);
        m_fileOperationMutex.unlock();
        // let the main window know that the current file has been saved
        // -> the main window will ensure the current file is still selected
        emit fileSaved(m_currentPath);
        // show next file (only if there are critical notifications)
        if(critical < 1 && m_nextFileAfterSaving) {
            emit nextFileSelected();
        } else {
            startParsing(m_currentPath, true);
        }
        m_nextFileAfterSaving = false;
    } else {
        static const QString processingErrorMsg(tr("The tags couldn't be saved. See the info box for detail."));
        static const QString ioErrorMsg(tr("The tags couldn't be saved because an IO error occured."));
        const auto &errorMsg = ioError ? ioErrorMsg : processingErrorMsg;
        QMessageBox::warning(this, QApplication::applicationName(), errorMsg);
        emit statusMessage(errorMsg);
        m_ui->makingNotificationWidget->setText(errorMsg);
        m_ui->makingNotificationWidget->setNotificationType(NotificationType::Critical);
        m_fileOperationMutex.unlock();
        startParsing(m_currentPath, true);
    }
}

/*!
 * \brief Asks the user whether an ID3 tag should be add to a not supported container format and returns the result.
 */
bool TagEditorWidget::confirmCreationOfId3TagForUnsupportedFile()
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
 * \brief This slot is connected to the fileChanged() signal of the file info watcher.
 */
void TagEditorWidget::fileChangedOnDisk(const QString &path)
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
void TagEditorWidget::closeFile()
{
    TryLocker<> locker(m_fileOperationMutex);
    if(locker) {
        // close file
        m_fileInfo.close();
        // remove current path from file watcher
        m_fileWatcher->removePath(m_currentPath);
        // update ui
        emit statusMessage("The file has been closed.");
        updateFileStatusStatus();
    } else {
        emit statusMessage("Unable to close the file because the current process hasn't been finished yet.");
    }
}

/*!
 * \brief This private slot is invoked the the return key has been pressed in a tag edit.
 *
 * The file will be saved and then the next opened if the user selected that option.
 */
void TagEditorWidget::handleReturnPressed()
{
    if(Settings::saveAndShowNextOnEnter() && m_fileInfo.isOpen()) {
        saveAndShowNextFile();
    }
}

void TagEditorWidget::handleKeepPreviousValuesActionTriggered(QAction *action)
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
void TagEditorWidget::applySettingsFromDialog()
{
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
void TagEditorWidget::addTag(const function<Media::Tag *(Media::MediaFileInfo &)> &createTag)
{
    TryLocker<> locker(m_fileOperationMutex);
    if(locker) {
        if(!m_fileInfo.isOpen()) {
            emit statusMessage("Unable to add a tag because no file is opened.");
            return;
        }
        if(Tag *tag = createTag(m_fileInfo)) {
            if(std::find(m_tags.cbegin(), m_tags.cend(), tag) == m_tags.cend()) {
                m_tags.push_back(tag);
                updateTagEditsAndAttachmentEdits(true, m_tags.size() > 1 ? PreviousValueHandling::Keep : PreviousValueHandling::Auto);
                updateTagSelectionComboBox();
                updateTagManagementMenu();
                updateFileStatusStatus();
                insertTitleFromFilename();
            } else {
                QMessageBox::warning(this, windowTitle(), tr("A tag (with the selected target) already exists."));
            }
        } else {
            QMessageBox::warning(this, windowTitle(), tr("The tag can not be created."));
        }
    } else {
        emit statusMessage("Unable to add a tag because the current process hasn't been finished yet.");
    }
}

/*!
 * \brief Removes the specified \a tag from the currently opened file.
 *
 * Shows an error message if the removal is (currently) not possible. Tag edits, tag management menu und UI status will be updated.
 */
void TagEditorWidget::removeTag(Tag *tag)
{
    if(tag) {
        TryLocker<> locker(m_fileOperationMutex);
        if(locker) {
            if(!m_fileInfo.isOpen()) {
                emit statusMessage(tr("Unable to remove the tag because no file is opened."));
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
                updateFileStatusStatus();
            }
        } else {
            emit statusMessage(tr("Unable to remove the tag because the current process hasn't been finished yet."));
        }
    }
}

/*!
 * \brief Changes the target of the specified \a tag; prompts the user to enter tag target information.
 *
 * Shows an error message if the change is (currently) not possible. Tag management menu and tag selection combo box will be updated.
 */
void TagEditorWidget::changeTarget(Tag *tag)
{
    if(tag) {
        TryLocker<> locker(m_fileOperationMutex);
        if(locker) {
            if(!m_fileInfo.isOpen()) {
                emit statusMessage(tr("Unable to change the target because no file is opened."));
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
        } else {
            emit statusMessage(tr("Unable to change the target because the current process hasn't been finished yet."));
        }
    }
}

}
