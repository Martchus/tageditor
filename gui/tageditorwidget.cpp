#include "./tageditorwidget.h"
#include "./attachmentsedit.h"
#include "./entertargetdialog.h"
#include "./fileinfomodel.h"
#include "./notificationlabel.h"
#include "./tagedit.h"
#include "./webviewincludes.h"

#include "../application/settings.h"
#include "../application/targetlevelmodel.h"
#include "../misc/htmlinfo.h"
#include "../misc/utility.h"

#include "resources/config.h"
#include "ui_tageditorwidget.h"

#include <tagparser/exceptions.h>
#include <tagparser/id3/id3v1tag.h>
#include <tagparser/id3/id3v2tag.h>
#include <tagparser/matroska/matroskatag.h>
#include <tagparser/mp4/mp4container.h>
#include <tagparser/mp4/mp4tag.h>
#include <tagparser/ogg/oggcontainer.h>
#include <tagparser/signature.h>

#include <qtutilities/misc/conversion.h>
#include <qtutilities/misc/dialogutils.h>
#include <qtutilities/widgets/clearlineedit.h>

#include <c++utilities/conversion/stringconversion.h>
#include <c++utilities/io/ansiescapecodes.h>
#include <c++utilities/io/path.h>

#include <QActionGroup>
#include <QCheckBox>
#include <QClipboard>
#include <QDesktopServices>
#include <QDir>
#include <QFileDialog>
#include <QFileInfo>
#include <QFileSystemModel>
#include <QFileSystemWatcher>
#include <QGuiApplication>
#include <QHeaderView>
#include <QKeyEvent>
#include <QMenu>
#include <QMessageBox>
#include <QMimeData>
#include <QPlainTextEdit>
#include <QStyle>
#include <QTemporaryFile>
#include <QTreeView>
#include <QtConcurrent>

#include <algorithm>
#include <functional>
#include <iostream>

using namespace std;
using namespace std::placeholders;
using namespace CppUtilities;
using namespace CppUtilities::EscapeCodes;
using namespace QtUtilities;
using namespace TagParser;
using namespace Utility;

namespace QtGui {

enum LoadingResult : char { ParsingSuccessful, FatalParsingError, IoError };

/*!
 * \class QtGui::TagEditorWidget
 * \brief The TagEditorWidget class provides a widget for tag editing.
 */

/*!
 * \brief Constructs a new tag editor widget.
 */
TagEditorWidget::TagEditorWidget(QWidget *parent)
    : QWidget(parent)
    , m_ui(new Ui::TagEditorWidget)
    ,
#ifndef TAGEDITOR_NO_WEBVIEW
    m_infoWebView(nullptr)
    ,
#endif
    m_infoModel(nullptr)
    , m_infoTreeView(nullptr)
    , m_nextFileAfterSaving(false)
    , m_makingResultsAvailable(false)
    , m_abortClicked(false)
{
    // setup UI
    m_ui->setupUi(this);
    makeHeading(m_ui->fileNameLabel);

    // setup (web) view
    initInfoView();

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
    connect(m_ui->actionManage_tags_automatically_when_loading_file, &QAction::triggered,
        [](bool checked) { Settings::values().tagPocessing.autoTagManagement = checked; });
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
    m_ui->parsingNotificationWidget->setContext(tr("Parsing notifications"));
    m_ui->makingNotificationWidget->setContext(tr("Applying notifications"));
    // connect signals and slots, install event filter
    //  buttons: save, delete, next, close
    connect(m_ui->saveButton, &QPushButton::clicked, this, &TagEditorWidget::applyEntriesAndSaveChangings);
    connect(m_ui->deleteTagsButton, &QPushButton::clicked, this, &TagEditorWidget::deleteAllTagsAndSave);
    connect(m_ui->nextButton, &QPushButton::clicked, this, &TagEditorWidget::saveAndShowNextFile);
    connect(m_ui->closeButton, &QPushButton::clicked, this, &TagEditorWidget::closeFile);

    //  misc
    connect(m_ui->abortButton, &QPushButton::clicked, [this] {
        m_abortClicked = true;
        m_ui->makingNotificationWidget->setText(tr("Cancelling ..."));
        m_ui->abortButton->setEnabled(false);
    });
    connect(m_ui->tagSelectionComboBox, static_cast<void (QComboBox::*)(int)>(&QComboBox::activated), m_ui->stackedWidget,
        &QStackedWidget::setCurrentIndex);
    connect(m_fileWatcher, &QFileSystemWatcher::fileChanged, this, &TagEditorWidget::fileChangedOnDisk);
    m_fileInfo.setWritingApplication(APP_NAME " v" APP_VERSION);

    // apply settings
    applySettingsFromDialog();
}

/*!
 * \brief Destroys the tag editor widget.
 */
TagEditorWidget::~TagEditorWidget()
{
    if (isFileOperationOngoing()) {
        cout << Phrases::Warning << "Waiting for the ongoing file operation to finish ..." << Phrases::EndFlush;
        m_ongoingFileOperation.waitForFinished();
    }
}

/*!
 * \brief Returns the HTML source of the info website.
 * \remarks In contrast to fileInfoHtml(), this method will generate file info if not available yet.
 */
const QByteArray &TagEditorWidget::generateFileInfoHtml()
{
    if (m_fileInfoHtml.isEmpty()) {
        m_fileInfoHtml = HtmlInfo::generateInfo(m_fileInfo, m_diag, m_diagReparsing);
    }
    return m_fileInfoHtml;
}

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
    switch (event->type()) {
    case QEvent::PaletteChange:
        updateInfoView();
        break;
    case QEvent::DragEnter:
    case QEvent::Drop: {
        auto *dropEvent = static_cast<QDropEvent *>(event);
        for (const auto &url : dropEvent->mimeData()->urls()) {
            if (url.scheme() == QLatin1String("file")) {
                event->accept();
                if (event->type() == QEvent::Drop) {
#ifdef Q_OS_WIN32
                    // remove leading slash
                    QString path = url.path();
                    int index = 0;
                    for (const auto &c : path) {
                        if (c == QChar('/')) {
                            ++index;
                        } else {
                            break;
                        }
                    }
                    if (index) {
                        path = path.mid(index);
                    }
                    startParsing(path, true);
#else
                    emit currentPathChanged(url.path());
                    startParsing(url.path(), true);
#endif
                }
                return true;
            }
        }
        break;
    }
    default:;
    }
    return QWidget::event(event);
}

/*!
 * \brief Updates the line edits for the document title(s).
 */
void TagEditorWidget::updateDocumentTitleEdits()
{
    // get container, segment count and present titles
    const auto *const container = m_fileInfo.container();
    const auto segmentCount = [&] {
        constexpr auto segmentLimit = 10;
        const auto count = container ? container->segmentCount() : static_cast<size_t>(0);
        if (count <= segmentLimit) {
            return static_cast<int>(count);
        }
        m_ui->parsingNotificationWidget->appendLine(tr("The file contains more segments than the GUI can handle."));
        return segmentLimit;
    }();
    const vector<string> &titles = container ? container->titles() : vector<string>();

    // get layout
    QLayout *const docTitleLayout = m_ui->docTitleWidget->layout();
    int lineEditCount = docTitleLayout->count();

    // update existing line edits, remove unneeded line edits
    int i = 0;
    for (; i < lineEditCount; ++i) {
        if (i < segmentCount) {
            // update existing line edit
            static_cast<ClearLineEdit *>(docTitleLayout->itemAt(i)->widget())
                ->setText(static_cast<size_t>(i) < titles.size() ? QString::fromUtf8(titles[static_cast<size_t>(i)].data()) : QString());
        } else {
            // remove unneeded line edit
            docTitleLayout->removeItem(docTitleLayout->itemAt(i + 1));
        }
    }

    // add missing line edits
    while (i < segmentCount) {
        auto *const lineEdit = new ClearLineEdit;
        if (static_cast<size_t>(i) < titles.size()) {
            lineEdit->setText(QString::fromUtf8(titles[static_cast<size_t>(i)].data()));
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
    switch (previousValueHandling) {
    case PreviousValueHandling::Auto:
        switch (Settings::values().editor.adoptFields) {
        case Settings::AdoptFields::WithinDirectory:
            if (m_lastDir != m_currentDir) {
                previousValueHandling = PreviousValueHandling::Clear;
            }
            break;
        case Settings::AdoptFields::Never:
            previousValueHandling = PreviousValueHandling::Clear;
            break;
        default:;
        }
        break;
    default:;
    }

    // define helper function to fetch next edit
    TagEdit *edit; // holds current edit
    int widgetIndex = 0; // holds index of current edit in the stacked widget
    auto fetchNextEdit = [this, &edit, &widgetIndex, &previousValueHandling] {
        // reuse existing edit (assigned in if-condition!) or ...
        if (!((widgetIndex < m_ui->stackedWidget->count()) && (edit = qobject_cast<TagEdit *>(m_ui->stackedWidget->widget(widgetIndex))))) {
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
    if (m_tags.size()) {
        // create a lists of the targets and tags
        QList<TagTarget> targets;
        QList<QList<Tag *>> tagsByTarget;
        for (Tag *tag : m_tags) {
            const TagTarget &target = tag->target();
            int index = targets.indexOf(target);
            if (index < 0) {
                targets << target;
                tagsByTarget << (QList<Tag *>() << tag);
            } else {
                tagsByTarget[index] << tag;
            }
        }
        // create a singe editor per target or seperate editors for each tag depending on the settings
        bool hasAutoCorrectionBeenApplied = false;
        switch (Settings::values().editor.multipleTagHandling) {
        case Settings::MultipleTagHandling::SingleEditorPerTarget:
            // iterate through all targets in both cases
            for (int targetIndex = 0, targetCount = targets.size(); targetIndex < targetCount; ++targetIndex) {
                fetchNextEdit();
                edit->setTags(tagsByTarget.at(targetIndex), updateUi); // set all tags with the same target to a single edit
                if (!hasAutoCorrectionBeenApplied) {
                    hasAutoCorrectionBeenApplied = edit->hasAutoCorrectionBeenApplied();
                }
                ++widgetIndex;
            }
            break;
        case Settings::MultipleTagHandling::SeparateEditors:
            // iterate through all targets in both cases
            for (int targetIndex = 0, targetCount = targets.size(); targetIndex < targetCount; ++targetIndex) {
                for (Tag *tag : tagsByTarget.at(targetIndex)) {
                    fetchNextEdit();
                    edit->setTag(tag, updateUi); // use a separate edit for each tag
                    if (!hasAutoCorrectionBeenApplied) {
                        hasAutoCorrectionBeenApplied = edit->hasAutoCorrectionBeenApplied();
                    }
                    ++widgetIndex;
                }
            }
            break;
        }
        if (hasAutoCorrectionBeenApplied) {
            addParsingNotificationLine(tr("Some values have been changed by the auto-correction."));
        }
    } else {
        // there are no tags -> leave one edit existend but ensure no tags are assigned
        fetchNextEdit();
        edit->setTag(nullptr, false);
        ++widgetIndex;
    }

    // add/update AttachmentsEdit widget
    if (m_fileInfo.areAttachmentsSupported()) {
        AttachmentsEdit *attachmentsEdit;
        // reuse existing edit (assigned in if-condition!) or ...
        if ((widgetIndex < m_ui->stackedWidget->count())
            && (attachmentsEdit = qobject_cast<AttachmentsEdit *>(m_ui->stackedWidget->widget(widgetIndex)))) {
            attachmentsEdit->setFileInfo(&m_fileInfo, true);
        } else {
            // ... create and add a new edit
            attachmentsEdit = new AttachmentsEdit(&m_fileInfo, this);
            connect(m_ui->clearEntriesPushButton, &QPushButton::clicked, attachmentsEdit, &AttachmentsEdit::clear);
            connect(m_ui->restoreEntriesPushButton, &QPushButton::clicked, attachmentsEdit, &AttachmentsEdit::restore);
            //connect(edit, &AttachmentsEdit::returnPressed, this, &TagEditorWidget::handleReturnPressed);
            m_ui->stackedWidget->insertWidget(widgetIndex, attachmentsEdit);
        }
        ++widgetIndex;
    }

    // remove surplus edits
    while (widgetIndex < m_ui->stackedWidget->count()) {
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
    if (m_fileInfo.isOpen()) {
        // memorize the index of the previously selected edit
        int previouslySelectedEditIndex = m_ui->tagSelectionComboBox->currentIndex();
        // clear old entries and create new labels
        m_ui->tagSelectionComboBox->clear();
        bool haveTargetInfo = false;
        for (int index = 0, count = m_ui->stackedWidget->count(); index < count; ++index) {
            QString label;
            if (TagEdit *edit = qobject_cast<TagEdit *>(m_ui->stackedWidget->widget(index))) {
                if (!edit->tags().isEmpty()) {
                    label = edit->generateLabel();
                    haveTargetInfo |= !edit->tags().at(0)->target().isEmpty();
                }
            } else if (qobject_cast<AttachmentsEdit *>(m_ui->stackedWidget->widget(index))) {
                static const QString attachmentsLabel = tr("Attachments");
                label = attachmentsLabel;
            }
            m_ui->tagSelectionComboBox->addItem(label);
        }
        // set visibility
        m_ui->tagSelectionComboBox->setHidden(
            Settings::values().editor.hideTagSelectionComboBox && m_ui->tagSelectionComboBox->count() <= 1 && !haveTargetInfo);
        // restore selected index
        if (previouslySelectedEditIndex >= 0 && previouslySelectedEditIndex < m_ui->tagSelectionComboBox->count()) {
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
    const bool opened = m_fileInfo.isOpen();
    const bool hasTag = opened && m_tags.size();
    // notification widgets
    m_ui->parsingNotificationWidget->setVisible(opened);
    m_ui->makingNotificationWidget->setVisible(opened && (m_makingResultsAvailable));
    // document title widget
    const bool showDocumentTitle = opened && m_fileInfo.container() && m_fileInfo.container()->supportsTitle();
    m_ui->docTitleLabel->setVisible(showDocumentTitle);
    m_ui->docTitleWidget->setVisible(showDocumentTitle);
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
#ifndef TAGEDITOR_NO_WEBVIEW
    if (m_infoWebView) {
        m_infoWebView->setVisible(opened);
    }
#endif
    if (m_infoTreeView) {
        m_infoTreeView->setVisible(opened);
    }
    // inform the main window about the file status change as well
    emit fileStatusChanged(opened, hasTag);
}

/*!
 * \brief Updates the "tag management menu".
 */
void TagEditorWidget::updateTagManagementMenu()
{
    m_addTagMenu->clear();
    m_removeTagMenu->clear();
    m_changeTargetMenu->clear();
    if (m_fileInfo.isOpen()) {
        // add "Add tag" actions
        if (m_fileInfo.areTagsSupported() && m_fileInfo.container()) {
            // there is a container object which is able to create tags
            QString label;
            switch (m_fileInfo.containerFormat()) {
            case ContainerFormat::Matroska:
            case ContainerFormat::Webm:
                // tag format supports targets (Matroska tags are currently the only tag format supporting targets.)
                label = tr("Matroska tag");
                connect(m_addTagMenu->addAction(label), &QAction::triggered,
                    std::bind(&TagEditorWidget::addTag, this, [this](MediaFileInfo &file) -> TagParser::Tag * {
                        if (file.container()) {
                            EnterTargetDialog targetDlg(this);
                            targetDlg.setTarget(TagTarget(50), &this->m_fileInfo);
                            if (targetDlg.exec() == QDialog::Accepted) {
                                return file.container()->createTag(targetDlg.target());
                            }
                        }
                        return nullptr;
                    }));
                break;

            default:
                // tag format does not support targets
                if (!m_fileInfo.container()->tagCount()) {
                    switch (m_fileInfo.containerFormat()) {
                    case ContainerFormat::Mp4:
                        label = tr("MP4/iTunes tag");
                        break;
                    case ContainerFormat::Ogg:
                        label = tr("Vorbis comment");
                        break;
                    default:
                        label = tr("Tag");
                    }
                    connect(m_addTagMenu->addAction(label), &QAction::triggered, std::bind(&TagEditorWidget::addTag, this, [](MediaFileInfo &file) {
                        return file.container() ? file.container()->createTag() : nullptr;
                    }));
                }
            }
        } else {
            // there is no container object which is able to create tags
            switch (m_fileInfo.containerFormat()) {
            case ContainerFormat::Flac:
                if (!m_fileInfo.vorbisComment()) {
                    connect(m_addTagMenu->addAction(tr("Vorbis comment")), &QAction::triggered,
                        std::bind(&TagEditorWidget::addTag, this, [](MediaFileInfo &file) { return file.createVorbisComment(); }));
                }
                break;

            default:
                // creation of ID3 tags is always possible
                if (!m_fileInfo.hasId3v1Tag()) {
                    connect(m_addTagMenu->addAction(tr("ID3v1 tag")), &QAction::triggered,
                        std::bind(&TagEditorWidget::addTag, this, [](MediaFileInfo &file) { return file.createId3v1Tag(); }));
                }
                if (!m_fileInfo.hasId3v2Tag()) {
                    connect(m_addTagMenu->addAction(tr("ID3v2 tag")), &QAction::triggered,
                        std::bind(&TagEditorWidget::addTag, this, [](MediaFileInfo &file) { return file.createId3v2Tag(); }));
                }
            }
        }
        // add "Remove tag" and "Change target" actions
        for (Tag *tag : m_tags) {
            // don't propose removal for Vorbis comments from Voribs or FLAC streams (removing from Opus streams should be ok)
            if (tag->type() == TagType::OggVorbisComment) {
                switch (static_cast<OggVorbisComment *>(tag)->oggParams().streamFormat) {
                case GeneralMediaFormat::Vorbis:
                case GeneralMediaFormat::Flac:
                    continue;
                default:;
                }
            }

            connect(m_removeTagMenu->addAction(QString::fromUtf8(tag->toString().c_str())), &QAction::triggered,
                std::bind(&TagEditorWidget::removeTag, this, tag));
            if (tag->supportsTarget()) {
                connect(m_changeTargetMenu->addAction(QString::fromUtf8(tag->toString().c_str())), &QAction::triggered,
                    std::bind(&TagEditorWidget::changeTarget, this, tag));
            }
        }
    }
    m_addTagMenu->setEnabled(!m_addTagMenu->actions().empty());
    m_removeTagMenu->setEnabled(!m_removeTagMenu->actions().empty());
    m_changeTargetMenu->setEnabled(!m_changeTargetMenu->actions().empty());
}

void TagEditorWidget::updateKeepPreviousValuesButton()
{
    switch (Settings::values().editor.adoptFields) {
    case Settings::AdoptFields::Never:
        m_ui->keepPreviousValuesPushButton->setText(tr("Clear previous values"));
        break;
    case Settings::AdoptFields::WithinDirectory:
        m_ui->keepPreviousValuesPushButton->setText(tr("Keep previous values in same dir"));
        break;
    case Settings::AdoptFields::Always:
        m_ui->keepPreviousValuesPushButton->setText(tr("Keep previous values"));
        break;
    }
}

/*!
 * \brief Inserts the title from the filename if no title is available from the tags.
 * \remarks Does nothing if there are no tags assigned or if this feature is not enabled.
 */
void TagEditorWidget::insertTitleFromFilename()
{
    if (m_tags.empty() || !Settings::values().editor.autoCompletition.insertTitleFromFilename) {
        return;
    }
    QString title;
    int trackNum;
    parseFileName(m_fileName, title, trackNum);
    const auto titleValue = qstringToTagValue(title, TagTextEncoding::Utf16LittleEndian);
    auto updated = false;
    foreachTagEdit([&titleValue, &updated](TagEdit *const edit) {
        for (const auto *const tag : edit->tags()) {
            if (tag->supportsTarget() && tag->isTargetingLevel(TagTargetLevel::Part)) {
                return;
            }
        }
        if (edit->setValue(KnownField::Title, titleValue, PreviousValueHandling::Keep)) {
            updated = true;
        }
    });
    if (updated) {
        addParsingNotificationLine(tr("Inserted title from filename."));
    }
}

void TagEditorWidget::initInfoView()
{
#ifndef TAGEDITOR_NO_WEBVIEW
    if (!Settings::values().editor.noWebView && !m_infoWebView) {
        if (m_infoTreeView) {
            m_infoTreeView->deleteLater();
            m_infoTreeView = nullptr;
        }
        if (m_infoModel) {
            m_infoModel->deleteLater();
            m_infoModel = nullptr;
        }
        m_infoWebView = new TAGEDITOR_WEB_VIEW(m_ui->tagSplitter);
        m_infoWebView->setAcceptDrops(false);
        m_infoWebView->setContextMenuPolicy(Qt::CustomContextMenu);
        connect(m_infoWebView, &QWidget::customContextMenuRequested, this, &TagEditorWidget::showInfoWebViewContextMenu);
        m_ui->tagSplitter->addWidget(m_infoWebView);
    } else if (Settings::values().editor.noWebView && !m_infoTreeView) {
        if (m_infoWebView) {
            m_infoWebView->deleteLater();
            m_infoWebView = nullptr;
        }
#endif
        if (!m_infoTreeView) {
            m_infoTreeView = new QTreeView(this);
            m_infoTreeView->setContextMenuPolicy(Qt::CustomContextMenu);
            m_infoTreeView->setSelectionBehavior(QAbstractItemView::SelectItems);
            m_infoTreeView->setSelectionMode(QAbstractItemView::ExtendedSelection);
            connect(m_infoTreeView, &QWidget::customContextMenuRequested, this, &TagEditorWidget::showInfoTreeViewContextMenu);
            m_ui->tagSplitter->addWidget(m_infoTreeView);
        }
        if (!m_infoModel) {
            m_infoModel = new FileInfoModel(this);
            m_infoModel->setFileInfo(m_fileInfo, m_diag, m_makingResultsAvailable ? &m_diagReparsing : nullptr);
            m_infoTreeView->setModel(m_infoModel);
            m_infoTreeView->setHeaderHidden(true);
            m_infoTreeView->header()->setSectionResizeMode(QHeaderView::ResizeToContents);
        }
#ifndef TAGEDITOR_NO_WEBVIEW
    }
#endif
}

/*!
 * \brief Updates the info web view to show information about the currently opened file.
 */
void TagEditorWidget::updateInfoView()
{
    // ensure previous file info HTML is cleared in any case
    m_fileInfoHtml.clear();

    // update webview if present
#ifndef TAGEDITOR_NO_WEBVIEW
    if (m_infoWebView) {
        if (m_fileInfo.isOpen()) {
            m_infoWebView->setContent(generateFileInfoHtml(), QStringLiteral("application/xhtml+xml"));
        } else {
            m_infoWebView->setUrl(QStringLiteral("about:blank"));
        }
    }
#endif

    // update info model if present
    if (m_infoModel) {
        m_infoModel->setFileInfo(m_fileInfo, m_diag, m_makingResultsAvailable ? &m_diagReparsing : nullptr); // resets the model
    }
}

void TagEditorWidget::showInfoTreeViewContextMenu(const QPoint &position)
{
    QAction copyAction(QIcon::fromTheme(QStringLiteral("edit-copy")), tr("Copy"), nullptr);
    copyAction.setDisabled(m_infoTreeView->selectionModel()->selectedIndexes().isEmpty());
    connect(&copyAction, &QAction::triggered, [this] {
        const auto selection = m_infoTreeView->selectionModel()->selectedIndexes();
        if (!selection.isEmpty()) {
            QStringList text;
            text.reserve(selection.size());
            for (const QModelIndex &index : selection) {
                text << index.data().toString();
            }
            QGuiApplication::clipboard()->setText(text.join(QChar(' ')));
            // TODO: improve copied text
        }
    });
    QAction expandAllAction(QIcon::fromTheme(QStringLiteral("expand-menu-hover")), tr("Expand all"), nullptr);
    connect(&expandAllAction, &QAction::triggered, m_infoTreeView, &QTreeView::expandAll);
    QAction collapseAllAction(QIcon::fromTheme(QStringLiteral("collapse-menu-hover")), tr("Collapse all"), nullptr);
    connect(&collapseAllAction, &QAction::triggered, m_infoTreeView, &QTreeView::collapseAll);
    QMenu menu;
    menu.addAction(&copyAction);
    menu.addSeparator();
    menu.addAction(&expandAllAction);
    menu.addAction(&collapseAllAction);
    menu.exec(m_infoTreeView->viewport()->mapToGlobal(position));
}

#ifndef TAGEDITOR_NO_WEBVIEW
/*!
 * \brief Shows the context menu for the info web view.
 */
void TagEditorWidget::showInfoWebViewContextMenu(const QPoint &position)
{
    QAction copyAction(QIcon::fromTheme(QStringLiteral("edit-copy")), tr("Copy"), nullptr);
    copyAction.setDisabled(m_infoWebView->selectedText().isEmpty());
    connect(&copyAction, &QAction::triggered, [this] { QGuiApplication::clipboard()->setText(m_infoWebView->selectedText()); });
    QAction saveAction(QIcon::fromTheme(QStringLiteral("document-save")), tr("Save ..."), nullptr);
    saveAction.setDisabled(m_fileInfoHtml.isEmpty());
    connect(&saveAction, &QAction::triggered, this, &TagEditorWidget::saveFileInfo);
    QAction openAction(QIcon::fromTheme(QStringLiteral("internet-web-browser")), tr("Open in browser"), nullptr);
    openAction.setDisabled(m_fileInfoHtml.isEmpty());
    connect(&openAction, &QAction::triggered, this, &TagEditorWidget::openFileInfoInBrowser);
    QMenu menu;
    menu.addAction(&copyAction);
    menu.addAction(&saveAction);
    menu.addAction(&openAction);
    menu.exec(m_infoWebView->mapToGlobal(position));
}
#endif

/*!
 * \brief Calls the specified \a function for each of the currently present tag edits.
 */
void TagEditorWidget::foreachTagEdit(const std::function<void(TagEdit *)> &function)
{
    for (int i = 0, count = m_ui->stackedWidget->count(); i < count; ++i) {
        if (auto *edit = qobject_cast<TagEdit *>(m_ui->stackedWidget->widget(i))) {
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
    const bool sameFile = m_currentPath == path;
    if (!forceRefresh && sameFile) {
        return true;
    }
    if (isFileOperationOngoing()) {
        emit statusMessage(tr("Unable to load the selected file \"%1\" because the current process hasn't finished yet.").arg(path));
        return false;
    }

    // clear previous results and status
    m_tags.clear();
    m_fileInfo.clearParsingResults();
    if (!sameFile) {
        // close last file if possibly open
        m_fileInfo.close();
        // set path of file info
        m_currentPath = path;
        m_fileInfo.setSaveFilePath(string());
        m_fileInfo.setPath(std::string(toNativeFileName(path).data()));
        // update file name and directory
        const QFileInfo fileInfo(path);
        m_lastDir = m_currentDir;
        m_currentDir = fileInfo.absolutePath();
        m_fileName = fileInfo.fileName();
    }
    // write diagnostics to m_diagReparsing if making results are avalable
    m_makingResultsAvailable &= sameFile;
    Diagnostics &diag = m_makingResultsAvailable ? m_diagReparsing : m_diag;
    // clear diagnostics
    diag.clear();
    m_diagReparsing.clear();
    // show filename
    m_ui->fileNameLabel->setText(m_fileName);
    // define function to parse the file
    const auto startThread = [this, &diag] {
        auto result = char();
        auto ioError = QString();
        try {
            // try to open with write access
            try {
                m_fileInfo.reopen(false);
            } catch (const std::ios_base::failure &) {
                // try to open read-only if opening with write access failed
                m_fileInfo.reopen(true);
            }
            AbortableProgressFeedback progress; // FIXME: actually use the progress object
            m_fileInfo.setForceFullParse(Settings::values().editor.forceFullParse);
            m_fileInfo.parseEverything(diag, progress);
            result = ParsingSuccessful;
        } catch (const Failure &) {
            // the file has been opened; parsing notifications will be shown in the info box
            result = FatalParsingError;
        } catch (const std::ios_base::failure &e) {
            // the file could not be opened because an IO error occured
            m_fileInfo.close(); // ensure file is closed
            result = IoError;
            if ((ioError = QString::fromLocal8Bit(e.what())).isEmpty()) {
                ioError = tr("unknown error");
            }
        } catch (const std::exception &e) {
            diag.emplace_back(TagParser::DiagLevel::Critical, argsToString("Something completely unexpected happened: ", +e.what()), "parsing");
            result = FatalParsingError;
        }
        QMetaObject::invokeMethod(this, "showFile", Qt::QueuedConnection, Q_ARG(char, result), Q_ARG(QString, ioError));
    };
    // perform the operation concurrently
    m_ongoingFileOperation = QtConcurrent::run(startThread);
    // inform user
    static const auto statusMsg(tr("The file is beeing parsed ..."));
    m_ui->parsingNotificationWidget->setNotificationType(NotificationType::Progress);
    m_ui->parsingNotificationWidget->setText(statusMsg);
    m_ui->parsingNotificationWidget->setVisible(true); // ensure widget is visible!
    emit statusMessage(statusMsg);
    return true;
}

/*!
 * \brief Reparses the current file.
 */
bool TagEditorWidget::reparseFile()
{
    if (isFileOperationOngoing()) {
        emit statusMessage(tr("Unable to reload the file because the current process hasn't finished yet."));
        return false;
    }
    if (!m_fileInfo.isOpen() || m_currentPath.isEmpty()) {
        QMessageBox::warning(this, windowTitle(), tr("Currently is not file opened."));
        return false;
    }
    return startParsing(m_currentPath, true);
}

/*!
 * \brief Shows the current file info (technical info, tags, ...).
 * This private slot is invoked from the thread which performed the
 * parsing operation using Qt::QueuedConnection.
 * \param result Specifies whether the file could be load sucessfully.
 */
void TagEditorWidget::showFile(char result, const QString &ioError)
{
    // handle IO errors
    if (result == IoError) {
        // update status
        updateFileStatusStatus();
        static const QString statusMsg(tr("The file could not be opened because an IO error occurred: %1"));
        auto msgBox = new QMessageBox(this);
        msgBox->setIcon(QMessageBox::Critical);
        msgBox->setAttribute(Qt::WA_DeleteOnClose, true);
        msgBox->setWindowTitle(tr("Opening file - ") + QCoreApplication::applicationName());
        msgBox->setText(statusMsg.arg(ioError));
        msgBox->setInformativeText(tr("Tried to open file: ") + m_currentPath);
        msgBox->show();
        emit statusMessage(statusMsg);
        return;
    }

    // load existing tags
    m_tags.clear();
    m_fileInfo.tags(m_tags);
    // show notification if no existing tag(s) could be found
    if (!m_tags.size()) {
        m_ui->parsingNotificationWidget->appendLine(tr("There is no (supported) tag assigned."));
    }

    // create appropriate tags according to file type and user preferences when automatic tag management is enabled
    auto &settings = Settings::values().tagPocessing;
    if (settings.autoTagManagement) {
        settings.creationSettings.requiredTargets.clear();
        settings.creationSettings.requiredTargets.reserve(2);
        for (const ChecklistItem &targetItem : Settings::values().editor.defaultTargets.items()) {
            if (targetItem.isChecked()) {
                settings.creationSettings.requiredTargets.emplace_back(
                    containerTargetLevelValue(m_fileInfo.containerFormat(), static_cast<TagTargetLevel>(targetItem.id().toInt())));
            }
        }
        // TODO: allow initialization of new ID3 tag with values from already present ID3 tag
        // TODO: allow not to transfer values from removed ID3 tag to remaining ID3 tags
        // TODO: still show the version as on disk in the info view
        settings.creationSettings.flags -= TagCreationFlags::KeepExistingId3v2Version;
        if (!m_fileInfo.createAppropriateTags(settings.creationSettings)) {
            if (confirmCreationOfId3TagForUnsupportedFile()) {
                settings.creationSettings.flags += TagCreationFlags::KeepExistingId3v2Version;
                m_fileInfo.createAppropriateTags(settings.creationSettings);
            }
        }
        // tags might have been adjusted -> reload tags
        m_tags.clear();
        m_fileInfo.tags(m_tags);
    }

    // show parsing status/result using parsing notification widget
    auto diagLevel = m_diag.level();
    if (diagLevel < worstDiagLevel) {
        diagLevel |= m_diagReparsing.level();
    }
    if (diagLevel >= DiagLevel::Critical) {
        // we catched no exception, but there are critical diag messages
        // -> treat those as fatal parsing errors
        result = LoadingResult::FatalParsingError;
    }
    switch (result) {
    case ParsingSuccessful:
        m_ui->parsingNotificationWidget->setNotificationType(NotificationType::TaskComplete);
        m_ui->parsingNotificationWidget->setText(tr("File could be parsed correctly."));
        break;
    default:
        m_ui->parsingNotificationWidget->setNotificationType(NotificationType::Critical);
        m_ui->parsingNotificationWidget->setText(tr("File couldn't be parsed correctly."));
    }
    bool multipleSegmentsNotTested = m_fileInfo.containerFormat() == ContainerFormat::Matroska && m_fileInfo.container()->segmentCount() > 1;
    if (diagLevel >= TagParser::DiagLevel::Critical) {
        m_ui->parsingNotificationWidget->setNotificationType(NotificationType::Critical);
        m_ui->parsingNotificationWidget->appendLine(tr("Errors occured."));
    } else if (diagLevel == TagParser::DiagLevel::Warning || m_fileInfo.isReadOnly() || !m_fileInfo.areTagsSupported() || multipleSegmentsNotTested) {
        m_ui->parsingNotificationWidget->setNotificationType(NotificationType::Warning);
        if (diagLevel == TagParser::DiagLevel::Warning) {
            m_ui->parsingNotificationWidget->appendLine(tr("There are warnings."));
        }
    }
    if (m_fileInfo.isReadOnly()) {
        m_ui->parsingNotificationWidget->appendLine(tr("No write access; the file has been opened in read-only mode."));
    }
    if (!m_fileInfo.areTagsSupported()) {
        m_ui->parsingNotificationWidget->appendLine(tr("File format is not supported (an ID3 tag can be added anyways)."));
    }
    if (multipleSegmentsNotTested) {
        m_ui->parsingNotificationWidget->appendLine(
            tr("The file is composed of multiple segments. Dealing with such files has not been tested yet and might be broken."));
    }

    // update relevant (UI) components
    m_fileWatcher->addPath(m_currentPath);
    m_fileChangedOnDisk = false;
    updateInfoView();
    updateDocumentTitleEdits();
    updateTagEditsAndAttachmentEdits();
    updateTagSelectionComboBox();
    updateTagManagementMenu();
    emit tagValuesLoaded();
    insertTitleFromFilename();
    updateFileStatusStatus();
    emit statusMessage(tr("The file %1 has been opened.").arg(m_currentPath));
    emit fileShown();
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
    if (isFileOperationOngoing()) {
        static const QString statusMsg(tr("Unable to apply the entered tags to the file because the current process hasn't finished yet."));
        emit statusMessage(statusMsg);
        return false;
    }

    m_ui->makingNotificationWidget->setNotificationType(NotificationType::Information);
    m_ui->makingNotificationWidget->setNotificationSubject(NotificationSubject::Saving);
    m_ui->makingNotificationWidget->setHidden(false);

    if (!m_fileInfo.isOpen()) {
        m_ui->makingNotificationWidget->setText(tr("No file has been opened, so tags can not be saved."));
        return false;
    }

    m_makingResultsAvailable = true;

    // apply titles
    AbstractContainer *const container = m_fileInfo.container();
    if (container && container->supportsTitle()) {
        QLayout *const docTitleLayout = m_ui->docTitleWidget->layout();
        for (std::size_t i = 0, count = min<std::size_t>(static_cast<std::size_t>(docTitleLayout->count()), container->segmentCount()); i < count;
             ++i) {
            container->setTitle(static_cast<ClearLineEdit *>(docTitleLayout->itemAt(static_cast<int>(i))->widget())->text().toUtf8().data(), i);
        }
    }
    // apply all tags
    foreachTagEdit([](TagEdit *edit) { edit->apply(); });
    static const QString statusMsg(tr("Saving tags ..."));
    m_ui->makingNotificationWidget->setNotificationSubject(NotificationSubject::None);
    m_ui->makingNotificationWidget->setNotificationType(NotificationType::Progress);
    m_ui->makingNotificationWidget->setText(statusMsg);
    emit statusMessage(statusMsg);
    return startSaving();
}

/*!
 * \brief Deletes all tags and starts saving.
 * \sa startSaving()
 */
bool TagEditorWidget::deleteAllTagsAndSave()
{
    if (isFileOperationOngoing()) {
        static const QString statusMsg(tr("Unable to delete all tags from the file because the current process hasn't been finished yet."));
        emit statusMessage(statusMsg);
        return false;
    }

    m_ui->makingNotificationWidget->setNotificationSubject(NotificationSubject::Saving);
    m_ui->makingNotificationWidget->setNotificationType(NotificationType::Information);
    m_ui->makingNotificationWidget->setHidden(false);

    if (!m_fileInfo.isOpen()) {
        m_ui->makingNotificationWidget->setText(tr("No file has been opened, so no tags can be deleted."));
        return false;
    }
    if (!m_fileInfo.hasAnyTag()) {
        m_ui->makingNotificationWidget->setText(tr("The selected file has no tag (at least no supported), so there is nothing to delete."));
        return false;
    }

    if (Settings::values().editor.askBeforeDeleting) {
        m_ui->makingNotificationWidget->setText(tr("Do you really want to delete all tags from the file?"));

        QMessageBox msgBox(this);
        msgBox.setText(m_ui->makingNotificationWidget->text());
        msgBox.setIcon(QMessageBox::Warning);
        msgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
        msgBox.setDefaultButton(QMessageBox::No);
#if QT_VERSION >= 0x050200
        auto *checkBox = new QCheckBox(&msgBox);
        checkBox->setText(tr("Don't show this message again"));
        msgBox.setCheckBox(checkBox);
#endif
        int res = msgBox.exec();
#if QT_VERSION >= 0x050200
        if (checkBox->isChecked()) {
            Settings::values().editor.askBeforeDeleting = false;
        }
#endif
        if (res != QMessageBox::Yes) {
            m_ui->makingNotificationWidget->setText(tr("Deletion aborted."));
            return false;
        }
    }

    m_makingResultsAvailable = true;

    foreachTagEdit([](TagEdit *edit) { edit->clear(); });
    m_fileInfo.removeAllTags();
    m_ui->makingNotificationWidget->setNotificationSubject(NotificationSubject::None);
    m_ui->makingNotificationWidget->setNotificationType(NotificationType::Progress);
    static const QString statusMsg(tr("Deleting all tags ..."));
    m_ui->makingNotificationWidget->setText(statusMsg);
    emit statusMessage(statusMsg);
    return startSaving();
}

/*!
 * \brief Starts saving. This method is called by applyEntriesAndSaveChangings() and deleteAllTagsAndSave().
 *
 * The actual process is performed in another thread.
 *
 * \remarks Will start a new thread to perform the operation. Then showSavingResult() is called
 *          using Qt::QueuedConnection in the main thread.
 */
bool TagEditorWidget::startSaving()
{
    if (isFileOperationOngoing()) {
        static const QString errorMsg(tr("Unable to start saving process because there an other process hasn't finished yet."));
        emit statusMessage(errorMsg);
        QMessageBox::warning(this, QCoreApplication::applicationName(), errorMsg);
        return false;
    }

    // tags might get invalidated
    m_tags.clear();
    foreachTagEdit([](TagEdit *edit) { edit->setTag(nullptr, false); });
    // show abort button
    m_ui->abortButton->setHidden(false);
    m_ui->abortButton->setEnabled(true);
    m_abortClicked = false;
    // remove current path from file watcher
    m_fileWatcher->removePath(m_currentPath);
    // use current configuration
    const auto &settings = Settings::values();
    const auto &fileLayoutSettings = settings.tagPocessing.fileLayout;
    m_fileInfo.setForceRewrite(fileLayoutSettings.forceRewrite);
    m_fileInfo.setTagPosition(fileLayoutSettings.preferredTagPosition);
    m_fileInfo.setForceTagPosition(fileLayoutSettings.forceTagPosition);
    m_fileInfo.setIndexPosition(fileLayoutSettings.preferredIndexPosition);
    m_fileInfo.setForceIndexPosition(fileLayoutSettings.forceIndexPosition);
    m_fileInfo.setMinPadding(fileLayoutSettings.minPadding);
    m_fileInfo.setMaxPadding(fileLayoutSettings.maxPadding);
    m_fileInfo.setPreferredPadding(fileLayoutSettings.preferredPadding);
    m_fileInfo.setBackupDirectory(settings.editor.backupDirectory);
    const auto startThread = [this] {
        // define functions to show the saving progress and to actually applying the changes
        auto showPercentage([this](AbortableProgressFeedback &progress) {
            if (m_abortClicked) {
                progress.tryToAbort();
            }
            QMetaObject::invokeMethod(m_ui->makingNotificationWidget, "setPercentage", Qt::QueuedConnection, Q_ARG(int, progress.stepPercentage()));
        });
        auto showStep([this](AbortableProgressFeedback &progress) {
            if (m_abortClicked) {
                progress.tryToAbort();
            } else {
                QMetaObject::invokeMethod(
                    m_ui->makingNotificationWidget, "setText", Qt::QueuedConnection, Q_ARG(QString, QString::fromStdString(progress.step())));
            }
            QMetaObject::invokeMethod(m_ui->makingNotificationWidget, "setPercentage", Qt::QueuedConnection, Q_ARG(int, progress.stepPercentage()));
        });
        AbortableProgressFeedback progress(std::move(showStep), std::move(showPercentage));

        auto ioError = QString();
        auto processingError = false, canceled = false;
        try {
            try {
                m_fileInfo.applyChanges(m_diag, progress);
            } catch (const OperationAbortedException &) {
                canceled = true;
            } catch (const Failure &) {
                processingError = true;
            } catch (const std::ios_base::failure &e) {
                if ((ioError = QString::fromLocal8Bit(e.what())).isEmpty()) {
                    ioError = tr("unknown error");
                }
            }
        } catch (const exception &e) {
            m_diag.emplace_back(TagParser::DiagLevel::Critical, argsToString("Something completely unexpected happened: ", e.what()), "making");
            processingError = true;
        }
        QMetaObject::invokeMethod(
            this, "showSavingResult", Qt::QueuedConnection, Q_ARG(QString, ioError), Q_ARG(bool, processingError), Q_ARG(bool, canceled));
    };
    // use another thread to perform the operation
    m_ongoingFileOperation = QtConcurrent::run(startThread);
    return true;
}

/*!
 * \brief Shows the saving results.
 *
 * This private slot is invoked from the thread which performed the
 * saving operation using Qt::QueuedConnection.
 *
 * \param sucess Specifies whether the file could be saved sucessfully.
 */
void TagEditorWidget::showSavingResult(QString ioError, bool processingError, bool canceled)
{
    m_ui->abortButton->setHidden(true);
    m_ui->makingNotificationWidget->setNotificationType(NotificationType::TaskComplete);
    m_ui->makingNotificationWidget->setNotificationSubject(NotificationSubject::Saving);
    m_ui->makingNotificationWidget->setPercentage(-1);
    m_ui->makingNotificationWidget->setHidden(false);
    m_makingResultsAvailable = true;
    if (!processingError && ioError.isEmpty()) {
        // display status messages
        QString statusMsg;
        size_t critical = 0, warnings = 0;
        for (const auto &msg : m_diag) {
            switch (msg.level()) {
            case TagParser::DiagLevel::Fatal:
            case TagParser::DiagLevel::Critical:
                ++critical;
                break;
            case TagParser::DiagLevel::Warning:
                ++warnings;
                break;
            default:;
            }
        }
        if (canceled) {
            if (critical) {
                statusMsg = tr("Saving has been canceled and there is/are %1 warning(s) ", nullptr, trQuandity(warnings)).arg(warnings);
                statusMsg.append(tr("and %1 error(s).", nullptr, trQuandity(critical)).arg(critical));
            } else if (warnings) {
                statusMsg = tr("Saving has been canceled and there is/are %1 warning(s).", nullptr, trQuandity(warnings)).arg(warnings);
            } else {
                statusMsg = tr("Saving tags has been canceled.");
            }
            m_ui->makingNotificationWidget->setNotificationType(critical ? NotificationType::Critical : NotificationType::Warning);

        } else if (warnings || critical) {
            if (critical) {
                statusMsg = tr("The tags have been saved, but there is/are %1 warning(s) ", nullptr, trQuandity(warnings)).arg(warnings);
                statusMsg.append(tr("and %1 error(s).", nullptr, trQuandity(critical)).arg(critical));
            } else {
                statusMsg = tr("The tags have been saved, but there is/are %1 warning(s).", nullptr, trQuandity(warnings)).arg(warnings);
            }
            m_ui->makingNotificationWidget->setNotificationType(critical ? NotificationType::Critical : NotificationType::Warning);

        } else {
            statusMsg = tr("The tags have been saved.");
        }
        m_ui->makingNotificationWidget->setText(statusMsg);
        emit statusMessage(statusMsg);
        // let the main window know that the current file has been saved
        // -> the main window will ensure the current file is still selected
        emit currentPathChanged(m_currentPath);
        // show next file (only if there are critical notifications)
        if (!critical && m_nextFileAfterSaving) {
            emit nextFileSelected();
        } else {
            // the current path might have changed through "save file path" mechanism
            startParsing(m_currentPath = fromNativeFileName(m_fileInfo.path()), true);
        }
        m_nextFileAfterSaving = false;
    } else {
        // fatal errors occured

        // -> show status
        static const QString processingErrorMsg = tr("The tags could not be saved. Checkout the info box for details.");
        static const QString ioErrorMsg = tr("The tags could not be saved because an IO error occured: %1");
        const auto errorMsg = !ioError.isEmpty() ? ioErrorMsg.arg(ioError) : processingErrorMsg;
        auto msgBox = new QMessageBox(this);
        msgBox->setIcon(QMessageBox::Critical);
        msgBox->setAttribute(Qt::WA_DeleteOnClose, true);
        msgBox->setWindowTitle(tr("Saving file - ") + QCoreApplication::applicationName());
        msgBox->setText(errorMsg);
        msgBox->setInformativeText(tr("Tried to save file: ") + m_currentPath);
        msgBox->show();
        emit statusMessage(errorMsg);
        m_ui->makingNotificationWidget->setText(errorMsg);
        m_ui->makingNotificationWidget->setNotificationType(NotificationType::Critical);

        // -> reset "save as path" in any case after fatal error
        m_fileInfo.setSaveFilePath(string());

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
    msgBox.setText(tr("The container format of the selected file is not supported. The file can be treated as MP3 file (an ID3 tag according to the "
                      "settings will be created). This might break the file. Do you want to continue?"));
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
    if (!m_fileChangedOnDisk && m_fileInfo.isOpen() && path == m_currentPath) {
        auto &notifyWidget = *m_ui->parsingNotificationWidget;
        notifyWidget.appendLine(tr("The currently opened file changed on the disk."));
        notifyWidget.setNotificationType(
            notifyWidget.notificationType() == NotificationType::Critical ? NotificationType::Critical : NotificationType::Warning);
        m_fileChangedOnDisk = true;
    }
}

/*!
 * \brief Closes the currently opened file and disables all related widgets.
 */
void TagEditorWidget::closeFile()
{
    if (isFileOperationOngoing()) {
        emit statusMessage("Unable to close the file because the current process hasn't been finished yet.");
        return;
    }

    // close file
    m_fileInfo.close();
    // remove current path from file watcher
    m_fileWatcher->removePath(m_currentPath);
    // update ui
    emit statusMessage("The file has been closed.");
    updateFileStatusStatus();
}

bool TagEditorWidget::handleFileInfoUnavailable()
{
    if (isFileOperationOngoing()) {
        emit statusMessage(tr("Unable to save file information because the current process hasn't been finished yet."));
        return true;
    }
    if (!fileInfo().isOpen()) {
        QMessageBox::information(this, QCoreApplication::applicationName(), tr("No file is opened."));
        return true;
    }

    if (generateFileInfoHtml().isEmpty()) {
        QMessageBox::information(this, QCoreApplication::applicationName(), tr("No file information available."));
        return true;
    }
    return false;
}

bool TagEditorWidget::writeFileInfoToFile(QFile &file)
{
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        QMessageBox::critical(this, QCoreApplication::applicationName(), tr("Unable to open file \"%1\".").arg(file.fileName()));
        return false;
    }
    file.write(fileInfoHtml());
    file.close();

    if (file.error() != QFileDevice::NoError) {
        QMessageBox::critical(
            this, QCoreApplication::applicationName(), tr("Unable to write to file \"%1\".\n%2").arg(file.fileName(), file.errorString()));
        return false;
    }
    return true;
}

/*!
 * \brief Saves the file information generated to be displayed in the info web view in a file.
 */
void TagEditorWidget::saveFileInfo()
{
    if (handleFileInfoUnavailable()) {
        return;
    }

    const QString path(QFileDialog::getSaveFileName(this, tr("Save file information - ") + QCoreApplication::applicationName()));
    if (path.isEmpty()) {
        QMessageBox::critical(this, QCoreApplication::applicationName(), tr("Unable to open file."));
        return;
    }

    QFile file(path);
    writeFileInfoToFile(file);
}

void TagEditorWidget::openFileInfoInBrowser()
{
    if (handleFileInfoUnavailable()) {
        return;
    }

    m_temporaryInfoFile = make_unique<QTemporaryFile>(QDir::tempPath() + QStringLiteral("/" PROJECT_NAME "-fileinfo-XXXXXX.xhtml"));
    if (!writeFileInfoToFile(*m_temporaryInfoFile)) {
        return;
    }

    QDesktopServices::openUrl(QStringLiteral("file://") + m_temporaryInfoFile->fileName());
}

/*!
 * \brief This private slot is invoked the the return key has been pressed in a tag edit.
 *
 * The file will be saved and then the next opened if the user selected that option.
 */
void TagEditorWidget::handleReturnPressed()
{
    if (Settings::values().editor.saveAndShowNextOnEnter && m_fileInfo.isOpen()) {
        saveAndShowNextFile();
    }
}

void TagEditorWidget::handleKeepPreviousValuesActionTriggered(QAction *action)
{
    auto &adoptFields = Settings::values().editor.adoptFields;
    if (action == m_ui->actionKeep_previous_values_never) {
        adoptFields = Settings::AdoptFields::Never;
    } else if (action == m_ui->actionKeep_previous_values_within_same_dir) {
        adoptFields = Settings::AdoptFields::WithinDirectory;
    } else if (action == m_ui->actionKeep_previous_values_always) {
        adoptFields = Settings::AdoptFields::Always;
    }
    updateKeepPreviousValuesButton();
}

/*!
 * \brief Applies settings from Settings namespace. Only settings configurable through the SettingsDialog
 *        will be applied and not settings like the main window's geometry and state.
 */
void TagEditorWidget::applySettingsFromDialog()
{
    const auto &settings = Settings::values();
    switch (settings.editor.adoptFields) {
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
    updateKeepPreviousValuesButton();
    m_ui->actionManage_tags_automatically_when_loading_file->setChecked(settings.tagPocessing.autoTagManagement);
    foreachTagEdit(bind(&TagEdit::setCoverButtonsHidden, _1, settings.editor.hideCoverButtons));
    // ensure info view is displayed/not displayed according to settings
    initInfoView();
    updateInfoView();
}

void TagEditorWidget::addParsingNotificationLine(const QString &line)
{
    m_ui->parsingNotificationWidget->appendLine(line);
}

/*!
 * \brief Adds a tag (using the specified \a createTag function) to the currently opened file.
 *
 * Shows an error message if no file is opened. Tag edits, tag management menu und UI status will be updated.
 */
void TagEditorWidget::addTag(const function<TagParser::Tag *(TagParser::MediaFileInfo &)> &createTag)
{
    if (isFileOperationOngoing()) {
        emit statusMessage("Unable to add a tag because the current process hasn't been finished yet.");
        return;
    }
    if (!m_fileInfo.isOpen()) {
        emit statusMessage("Unable to add a tag because no file is opened.");
        return;
    }

    Tag *const tag = createTag(m_fileInfo);
    if (!tag) {
        QMessageBox::warning(this, windowTitle(), tr("The tag can not be created."));
        return;
    }
    if (std::find(m_tags.cbegin(), m_tags.cend(), tag) != m_tags.cend()) {
        QMessageBox::warning(this, windowTitle(), tr("A tag (with the selected target) already exists."));
        return;
    }

    m_tags.push_back(tag);
    updateTagEditsAndAttachmentEdits(true, m_tags.size() > 1 ? PreviousValueHandling::Keep : PreviousValueHandling::Auto);
    updateTagSelectionComboBox();
    updateTagManagementMenu();
    updateFileStatusStatus();
    insertTitleFromFilename();
}

/*!
 * \brief Removes the specified \a tag from the currently opened file.
 *
 * Shows an error message if the removal is (currently) not possible. Tag edits, tag management menu und UI status will be updated.
 */
void TagEditorWidget::removeTag(Tag *tag)
{
    if (!tag) {
        return;
    }
    if (isFileOperationOngoing()) {
        emit statusMessage(tr("Unable to remove the tag because the current process hasn't been finished yet."));
        return;
    }
    if (!m_fileInfo.isOpen()) {
        emit statusMessage(tr("Unable to remove the tag because no file is opened."));
        return;
    }

    // remove tag itself
    m_fileInfo.removeTag(tag);
    m_tags.erase(remove(m_tags.begin(), m_tags.end(), tag), m_tags.end());

    // remove tag from all TagEdit widgets
    vector<TagEdit *> toRemove;
    for (int index = 0, count = m_ui->stackedWidget->count(); index < count; ++index) {
        auto *const edit = qobject_cast<TagEdit *>(m_ui->stackedWidget->widget(index));
        if (!edit || !edit->tags().contains(tag)) {
            continue;
        }

        QList<Tag *> tagsOfEdit = edit->tags();
        tagsOfEdit.removeAll(tag);

        // reassign remaining tags (keeping the previous values)
        if (!tagsOfEdit.empty()) {
            edit->setPreviousValueHandling(PreviousValueHandling::Keep);
            edit->setTags(tagsOfEdit, true);
            continue;
        }

        // handle case when no tags left in the edit
        if (m_tags.empty()) {
            // disable the edit if there are no other tag edits
            edit->setTag(nullptr, false);
        } else {
            // remove the edit if there are still other tag edits
            toRemove.push_back(edit);
        }
    }

    // remove TagEdit widgets
    for (TagEdit *const edit : toRemove) {
        m_ui->tagSelectionComboBox->removeItem(m_ui->stackedWidget->indexOf(edit));
        m_ui->stackedWidget->removeWidget(edit);
        delete edit;
    }

    // update affected widgets
    updateTagSelectionComboBox();
    updateTagManagementMenu();
    updateFileStatusStatus();
}

/*!
 * \brief Changes the target of the specified \a tag; prompts the user to enter tag target information.
 *
 * Shows an error message if the change is (currently) not possible. Tag management menu and tag selection combo box will be updated.
 */
void TagEditorWidget::changeTarget(Tag *tag)
{
    if (!tag) {
        return;
    }
    if (isFileOperationOngoing()) {
        emit statusMessage(tr("Unable to change the target because the current process hasn't been finished yet."));
        return;
    }
    if (!m_fileInfo.isOpen()) {
        emit statusMessage(tr("Unable to change the target because no file is opened."));
        return;
    }
    if (!tag->supportsTarget()) {
        QMessageBox::warning(this, windowTitle(), tr("Can not change the target of the selected tag because the tag does not support targets."));
        return;
    }

    EnterTargetDialog targetDlg(this);
    targetDlg.setTarget(tag->target(), &m_fileInfo);
    if (targetDlg.exec() != QDialog::Accepted) {
        return;
    }
    tag->setTarget(targetDlg.target());
    updateTagSelectionComboBox();
    updateTagManagementMenu();
}

} // namespace QtGui
