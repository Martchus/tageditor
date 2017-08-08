#include "./dbquerywidget.h"
#include "./tageditorwidget.h"
#include "./tagedit.h"

#include "../application/knownfieldmodel.h"
#include "../application/settings.h"
#include "../dbquery/dbquery.h"
#include "../misc/utility.h"

#include "ui_dbquerywidget.h"

#include <tagparser/tag.h>

#include <qtutilities/misc/dialogutils.h>

#include <c++utilities/conversion/conversionexception.h>

#include <QKeyEvent>
#include <QMenu>
#include <QDialog>
#include <QGraphicsView>
#include <QGraphicsItem>
#include <QTextBrowser>

#include <functional>

using namespace std;
using namespace std::placeholders;
using namespace ConversionUtilities;
using namespace Dialogs;
using namespace Models;
using namespace Settings;
using namespace Utility;
using namespace Media;

namespace QtGui {

DbQueryWidget::DbQueryWidget(TagEditorWidget *tagEditorWidget, QWidget *parent) :
    QWidget(parent),
    m_ui(new Ui::DbQueryWidget),
    m_tagEditorWidget(tagEditorWidget),
    m_model(nullptr),
    m_coverIndex(-1),
    m_lyricsIndex(-1),
    m_menu(new QMenu(parent))
{
    m_ui->setupUi(this);
#ifdef Q_OS_WIN32
    setStyleSheet(dialogStyle());
#else
    setStyleSheet(QStringLiteral("QGroupBox { color: palette(text); background-color: palette(base); }"));
#endif

    m_ui->notificationLabel->setText(tr("Search hasn't been started"));
    m_ui->notificationLabel->setContext(tr("MusicBrainz/LyricsWikia notifications"));
    m_ui->notificationLabel->setMaxLineCount(5);
    m_ui->searchGroupBox->installEventFilter(this);

    // initialize buttons
    m_ui->abortPushButton->setIcon(style()->standardIcon(QStyle::SP_DialogCancelButton, nullptr, m_ui->abortPushButton));
    m_ui->abortPushButton->setVisible(false);
    m_ui->applyPushButton->setIcon(style()->standardIcon(QStyle::SP_DialogApplyButton, nullptr, m_ui->applyPushButton));

    // initialize fields model
    m_ui->fieldsListView->setModel(&values().dbQuery.fields);

    // initialize search terms form
    insertSearchTermsFromTagEdit(m_tagEditorWidget->activeTagEdit());

    // restore settings
    m_ui->overrideCheckBox->setChecked(values().dbQuery.override);

    // setup menu
    m_insertPresentDataAction = m_menu->addAction(tr("Insert present data"));
    m_insertPresentDataAction->setIcon(QIcon::fromTheme(QStringLiteral("edit-copy")));
    m_insertPresentDataAction->setEnabled(m_tagEditorWidget->activeTagEdit());
    connect(m_insertPresentDataAction, &QAction::triggered, this, &DbQueryWidget::insertSearchTermsFromActiveTagEdit);
    QAction *clearSearchCriteria = m_menu->addAction(tr("Clear search criteria"));
    clearSearchCriteria->setIcon(QIcon::fromTheme(QStringLiteral("edit-clear")));
    connect(clearSearchCriteria, &QAction::triggered, this, &DbQueryWidget::clearSearchCriteria);
    m_ui->menuPushButton->setMenu(m_menu);

    // connect signals and slots
    connect(m_ui->abortPushButton, &QPushButton::clicked, this, &DbQueryWidget::abortSearch);
    connect(m_ui->searchMusicBrainzPushButton, &QPushButton::clicked, this, &DbQueryWidget::searchMusicBrainz);
    connect(m_ui->searchLyricsWikiaPushButton, &QPushButton::clicked, this, &DbQueryWidget::searchLyricsWikia);
    connect(m_ui->applyPushButton, &QPushButton::clicked, this, static_cast<void(DbQueryWidget::*)(void)>(&DbQueryWidget::applySelectedResults));
    connect(m_tagEditorWidget, &TagEditorWidget::fileStatusChanged, this, &DbQueryWidget::fileStatusChanged);
    connect(m_ui->resultsTreeView, &QTreeView::customContextMenuRequested, this, &DbQueryWidget::showResultsContextMenu);
}

DbQueryWidget::~DbQueryWidget()
{
    values().dbQuery.override = m_ui->overrideCheckBox->isChecked();
}

void DbQueryWidget::insertSearchTermsFromTagEdit(TagEdit *tagEdit)
{
    if(tagEdit) {
        // set title, album and artist
        m_ui->titleLineEdit->setText(tagValueToQString(tagEdit->value(KnownField::Title)));
        m_ui->albumLineEdit->setText(tagValueToQString(tagEdit->value(KnownField::Album)));
        m_ui->artistLineEdit->setText(tagValueToQString(tagEdit->value(KnownField::Artist)));

        // set track number, or if not available part number
        bool trackValueOk = false;
        try {
            TagValue trackValue = tagEdit->value(KnownField::TrackPosition);
            if(!trackValue.isEmpty()) {
                m_ui->trackSpinBox->setValue(trackValue.toPositionInSet().position());
                trackValueOk = true;
            }
        } catch(const ConversionException &) {
        }
        if(!trackValueOk) {
            TagValue trackValue = tagEdit->value(KnownField::PartNumber);
            if(!trackValue.isEmpty()) {
                m_ui->trackSpinBox->setValue(trackValue.toInteger());
                trackValueOk = true;
            }
        }
        if(!trackValueOk) {
            m_ui->trackSpinBox->clear();
        }
    }
}

SongDescription DbQueryWidget::currentSongDescription() const
{
    SongDescription desc;
    desc.title = m_ui->titleLineEdit->text();
    desc.album = m_ui->albumLineEdit->text();
    desc.artist = m_ui->artistLineEdit->text();
    desc.track = m_ui->trackSpinBox->value();
    return desc;
}

void DbQueryWidget::searchMusicBrainz()
{
    // check whether enough search terms are supplied
    if(m_ui->titleLineEdit->text().isEmpty() && m_ui->albumLineEdit->text().isEmpty() && m_ui->artistLineEdit->text().isEmpty()) {
        m_ui->notificationLabel->setNotificationType(NotificationType::Critical);
        m_ui->notificationLabel->setText(tr("Insufficient search criteria supplied - at least title, album or artist must be specified"));
        return;
    }

    // delete current model
    m_ui->resultsTreeView->setModel(nullptr);
    delete m_model;

    // show status
    m_ui->notificationLabel->setNotificationType(NotificationType::Progress);
    m_ui->notificationLabel->setText(tr("Retrieving meta data from MusicBrainz ..."));
    setStatus(false);

    // do actual query
    useQueryResults(queryMusicBrainz(currentSongDescription()));
}

void DbQueryWidget::searchLyricsWikia()
{
    // check whether enough search terms are supplied
    if(m_ui->artistLineEdit->text().isEmpty()) {
        m_ui->notificationLabel->setNotificationType(NotificationType::Critical);
        m_ui->notificationLabel->setText(tr("Insufficient search criteria supplied - artist is mandatory"));
        return;
    }

    // delete current model
    m_ui->resultsTreeView->setModel(nullptr);
    delete m_model;

    // show status
    m_ui->notificationLabel->setNotificationType(NotificationType::Progress);
    m_ui->notificationLabel->setText(tr("Retrieving meta data from LyricsWikia ..."));
    setStatus(false);

    // do actual query
    useQueryResults(queryLyricsWikia(currentSongDescription()));
}

void DbQueryWidget::abortSearch()
{
    if(m_model) {
        if(m_model->isFetchingCover()) {
            // call abort to abort fetching cover
            m_model->abort();
        } else if(!m_model->areResultsAvailable()) {
            // delete model to abort search
            m_ui->resultsTreeView->setModel(nullptr);
            delete m_model;
            m_model = nullptr;

            // update status
            m_ui->notificationLabel->setNotificationType(NotificationType::Information);
            m_ui->notificationLabel->setText(tr("Aborted"));
            setStatus(true);
        }
    }
}

void DbQueryWidget::showResults()
{
    if(m_model) {
        if(m_model->errorList().isEmpty()) {
            m_ui->notificationLabel->setNotificationType(NotificationType::TaskComplete);
            if(m_model->results().isEmpty()) {
                m_ui->notificationLabel->setText(tr("No results available"));
            } else {
                m_ui->notificationLabel->setText(tr("%1 result(s) available", 0, m_model->results().size()).arg(m_model->results().size()));
            }
        } else {
            m_ui->notificationLabel->setNotificationType(NotificationType::Critical);
            m_ui->notificationLabel->clearText();
            for(const QString &error : m_model->errorList()) {
                m_ui->notificationLabel->appendLine(error);
            }
        }
        if(m_model->results().isEmpty()) {
            m_ui->applyPushButton->setEnabled(false);
        } else {
            m_ui->resultsTreeView->selectionModel()->setCurrentIndex(m_model->index(0, 0), QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Rows);
            m_ui->applyPushButton->setEnabled(m_tagEditorWidget->activeTagEdit());
        }
        setStatus(true);
    }
}

void DbQueryWidget::setStatus(bool aborted)
{
    m_ui->abortPushButton->setVisible(!aborted);
    m_ui->searchMusicBrainzPushButton->setVisible(aborted);
    m_ui->searchLyricsWikiaPushButton->setVisible(aborted);
    m_ui->applyPushButton->setVisible(aborted);
}

void DbQueryWidget::fileStatusChanged(bool, bool hasTags)
{
    m_ui->applyPushButton->setEnabled(hasTags && m_ui->resultsTreeView->selectionModel() && m_ui->resultsTreeView->selectionModel()->hasSelection());
    insertSearchTermsFromActiveTagEdit();
    m_insertPresentDataAction->setEnabled(hasTags);
}

/*!
 * \brief Applies the selected results for the selected fields to the active tag edit.
 * \sa applyResults()
 */
void DbQueryWidget::applySelectedResults()
{
    // check whether model, tag edit and current selection exist
    if(TagEdit *tagEdit = m_tagEditorWidget->activeTagEdit()) {
        if(const QItemSelectionModel *selectionModel = m_ui->resultsTreeView->selectionModel()) {
            const QModelIndexList selection = selectionModel->selection().indexes();
            if(!selection.isEmpty()) {
                applyResults(tagEdit, selection.front());
            }
        }
    }
}

/*!
 * \brief Completes all present tag edits with the best matching result row.
 * \remarks
 * - Does nothing if no result row matches.
 * - Only the selected fields are applied.
 * \sa applyResults()
 */
void DbQueryWidget::applyMatchingResults()
{
    m_tagEditorWidget->foreachTagEdit(bind(static_cast<void(DbQueryWidget::*)(TagEdit *)>(&DbQueryWidget::applyMatchingResults), this, _1));
}

/*!
 * \brief Completes the specified \a tagEdit with the best matching result row.
 * \remarks
 * - Does nothing if no result row matches.
 * - Only the selected fields are applied.
 * \sa applyResults()
 */
void DbQueryWidget::applyMatchingResults(TagEdit *tagEdit)
{
    if(!m_model) {
        return;
    }

    // determine already present title, album and artist
    const TagValue givenTitle = tagEdit->value(KnownField::Title);
    const TagValue givenAlbum = tagEdit->value(KnownField::Album);
    const TagValue givenArtist = tagEdit->value(KnownField::Artist);

    // also determine already present track number (which is a little bit more complex -> TODO: improve backend API)
    int givenTrack;
    try {
        givenTrack = tagEdit->value(KnownField::TrackPosition).toPositionInSet().position();
    } catch (const ConversionException &) {
        givenTrack = 0;
    }
    if(!givenTrack) {
        for(const Tag *tag : tagEdit->tags()) {
            if(!tag->supportsTarget() || tag->targetLevel() == TagTargetLevel::Track) {
                try {
                    givenTrack = tagEdit->value(KnownField::PartNumber).toInteger();
                } catch (const ConversionException &) {
                }
                break;
            }
        }
    }

    if(givenTitle.isEmpty() || !givenTrack) {
        return;
    }

    // find row matching already present data
    for(int row = 0, rowCount = m_model->rowCount(); row != rowCount; ++row) {
        if((!givenTitle.isEmpty() && givenTitle != m_model->fieldValue(row, KnownField::Title))
                || (!givenAlbum.isEmpty() && givenAlbum != m_model->fieldValue(row, KnownField::Album))
                || (!givenArtist.isEmpty() && givenArtist != m_model->fieldValue(row, KnownField::Artist))
                || (givenTrack && givenTrack != m_model->fieldValue(row, KnownField::PartNumber).toInteger())) {
            continue;
        }

        // apply results for matching row
        applyResults(tagEdit, m_model->index(row, 0));

        // just take the first matching row for now
        break;
    }
}

/*!
 * \brief The same as applyMatchingResults() but checks whether auto-insert is enabled before (and does nothing if not).
 */
void DbQueryWidget::autoInsertMatchingResults()
{
    if(m_ui->autoInsertCheckBox->isChecked()) {
        applyMatchingResults();
    }
}

/*!
 * \brief Applies the results at the specified \a resultIndex for the selected fields to the specified \a tagEdit.
 * \remarks
 * - Returns instantly. If cover/lyrics need to be retrieved, this is done asynchronously.
 * - Does nothing if no results are available.
 */
void DbQueryWidget::applyResults(TagEdit *tagEdit, const QModelIndex &resultIndex)
{
    if(m_model) {
        // determine previous value handling
        PreviousValueHandling previousValueHandling = m_ui->overrideCheckBox->isChecked()
                ? PreviousValueHandling::Update : PreviousValueHandling::Keep;

        // loop through all fields
        for(const ChecklistItem &item : values().dbQuery.fields.items()) {
            if(item.isChecked()) {
                // field should be used
                const auto field = static_cast<KnownField>(item.id().toInt());
                int row = resultIndex.row();
                TagValue value = m_model->fieldValue(row, field);

                if(value.isEmpty()) {
                    // cover and lyrics might be fetched belated
                    switch(field) {
                    case KnownField::Cover:
                        if(m_model->fetchCover(resultIndex)) {
                            // cover is available now
                            tagEdit->setValue(KnownField::Cover, m_model->fieldValue(row, KnownField::Cover), previousValueHandling);
                        } else {
                            // cover is fetched asynchronously
                            // -> show status
                            m_ui->notificationLabel->setNotificationType(NotificationType::Progress);
                            m_ui->notificationLabel->appendLine(tr("Retrieving cover art to be applied ..."));
                            setStatus(false);
                            // -> apply cover when available
                            connect(m_model, &QueryResultsModel::coverAvailable, [this, row, previousValueHandling](const QModelIndex &index) {
                                if(row == index.row()) {
                                    if(TagEdit *tagEdit = m_tagEditorWidget->activeTagEdit()) {
                                        tagEdit->setValue(KnownField::Cover, m_model->fieldValue(row, KnownField::Cover), previousValueHandling);
                                    }
                                }
                            });
                        }
                        break;

                    case KnownField::Lyrics:
                        if(m_model->fetchLyrics(resultIndex)) {
                            // lyrics are available now
                            tagEdit->setValue(KnownField::Lyrics, m_model->fieldValue(row, KnownField::Lyrics), previousValueHandling);
                        } else {
                            // lyrics are fetched asynchronously
                            // -> show status
                            m_ui->notificationLabel->setNotificationType(NotificationType::Progress);
                            m_ui->notificationLabel->appendLine(tr("Retrieving lyrics to be applied ..."));
                            setStatus(false);
                            // -> apply cover when available
                            connect(m_model, &QueryResultsModel::lyricsAvailable, [this, row, previousValueHandling](const QModelIndex &index) {
                                if(row == index.row()) {
                                    if(TagEdit *tagEdit = m_tagEditorWidget->activeTagEdit()) {
                                        tagEdit->setValue(KnownField::Lyrics, m_model->fieldValue(row, KnownField::Lyrics), previousValueHandling);
                                    }
                                }
                            });
                        }
                        break;

                    default:
                        ;
                    }
                } else {
                    // any other fields are just set
                    tagEdit->setValue(field, value, previousValueHandling);
                }
            }
        }
    }
}

void DbQueryWidget::insertSearchTermsFromActiveTagEdit()
{
    insertSearchTermsFromTagEdit(m_tagEditorWidget->activeTagEdit());
}

void DbQueryWidget::showResultsContextMenu()
{
    if(const QItemSelectionModel *selectionModel = m_ui->resultsTreeView->selectionModel()) {
        const QModelIndexList selection = selectionModel->selection().indexes();
        if(!selection.isEmpty()) {
            QMenu contextMenu;
            if(m_ui->applyPushButton->isEnabled()) {
                contextMenu.addAction(m_ui->applyPushButton->icon(), tr("Use selected row"), this, static_cast<void(DbQueryWidget::*)(void)>(&DbQueryWidget::applySelectedResults));
            }
            if(m_model && m_model->areResultsAvailable()) {
                contextMenu.addAction(QIcon::fromTheme(QStringLiteral("view-preview")), tr("Show cover"), this, &DbQueryWidget::fetchAndShowCoverForSelection);
                contextMenu.addAction(QIcon::fromTheme(QStringLiteral("view-media-lyrics")), tr("Show lyrics"), this, &DbQueryWidget::fetchAndShowLyricsForSelection);
            }
            contextMenu.exec(QCursor::pos());
        }
    }
}

void DbQueryWidget::fetchAndShowCoverForSelection()
{
    const QModelIndex selectedIndex = this->selectedIndex();
    if(!selectedIndex.isValid()) {
        return;
    }

    if(const QByteArray *cover = m_model->cover(selectedIndex)) {
        showCover(*cover);
    } else {
        if(m_model->fetchCover(selectedIndex)) {
            if(const QByteArray *cover = m_model->cover(selectedIndex)) {
                showCover(*cover);
            } else {
                // cover couldn't be fetched, error tracked via resultsAvailable() signal so nothing to do
            }
        } else {
            // cover is fetched asynchronously
            // -> memorize index to be shown
            m_coverIndex = selectedIndex.row();
            // -> show status
            m_ui->notificationLabel->setNotificationType(NotificationType::Progress);
            m_ui->notificationLabel->setText(tr("Retrieving cover art ..."));
            setStatus(false);
        }
    }
}

void DbQueryWidget::fetchAndShowLyricsForSelection()
{
    const QModelIndex selectedIndex = this->selectedIndex();
    if(!selectedIndex.isValid()) {
        return;
    }

    if(const QString *lyrics = m_model->lyrics(selectedIndex)) {
        showLyrics(*lyrics);
    } else {
        if(m_model->fetchLyrics(selectedIndex)) {
            if(const QByteArray *cover = m_model->cover(selectedIndex)) {
                showLyrics(*cover);
            } else {
                // lyrics couldn't be fetched, error tracked via resultsAvailable() signal so nothing to do
            }
        } else {
            // lyrics are fetched asynchronously
            // -> memorize index to be shown
            m_lyricsIndex = selectedIndex.row();
            // -> show status
            m_ui->notificationLabel->setNotificationType(NotificationType::Progress);
            m_ui->notificationLabel->setText(tr("Retrieving lyrics ..."));
            setStatus(false);
        }
    }
}

void DbQueryWidget::showCover(const QByteArray &data)
{
    QDialog dlg;
    dlg.setWindowFlags(Qt::Tool);
    dlg.setWindowTitle(tr("Cover - %1").arg(QApplication::applicationName()));
    QBoxLayout layout(QBoxLayout::Up);
    layout.setMargin(0);
    QGraphicsView view(&dlg);
    QGraphicsScene scene;
    layout.addWidget(&view);
    scene.addItem(new QGraphicsPixmapItem(QPixmap::fromImage(QImage::fromData(data))));
    view.setScene(&scene);
    view.show();
    dlg.setLayout(&layout);
    dlg.exec();
}

void DbQueryWidget::showCoverFromIndex(const QModelIndex &index)
{
    if(m_coverIndex == index.row()) {
        m_coverIndex = -1;
        showCover(*m_model->cover(index));
    }
}

void DbQueryWidget::showLyrics(const QString &data)
{
    QDialog dlg;
    dlg.setWindowFlags(Qt::Tool);
    dlg.setWindowTitle(tr("Lyrics - %1").arg(QApplication::applicationName()));
    QBoxLayout layout(QBoxLayout::Up);
    layout.setMargin(0);
    QTextBrowser textBrowser;
    layout.addWidget(&textBrowser);
    textBrowser.setText(data);
    dlg.setLayout(&layout);
    dlg.resize(400, 400);
    dlg.exec();
}

void DbQueryWidget::showLyricsFromIndex(const QModelIndex &index)
{
    if(m_lyricsIndex == index.row()) {
        m_lyricsIndex = -1;
        showLyrics(*m_model->lyrics(index));
    }
}

void DbQueryWidget::clearSearchCriteria()
{
    m_ui->titleLineEdit->clear();
    m_ui->albumLineEdit->clear();
    m_ui->artistLineEdit->clear();
    m_ui->trackSpinBox->setValue(0);
}

bool DbQueryWidget::eventFilter(QObject *obj, QEvent *event)
{
    if(obj == m_ui->searchGroupBox) {
        switch(event->type()) {
        case QEvent::KeyRelease:
            switch(static_cast<QKeyEvent *>(event)->key()) {
            case Qt::Key_Return:
                searchMusicBrainz();
                break;
            default:
                ;
            }
            break;
        default:
            ;
        }
    }
    return QWidget::eventFilter(obj, event);
}

void DbQueryWidget::useQueryResults(QueryResultsModel *queryResults)
{
    m_ui->resultsTreeView->setModel(m_model = queryResults);
    connect(queryResults, &QueryResultsModel::resultsAvailable, this, &DbQueryWidget::showResults);
    connect(queryResults, &QueryResultsModel::lyricsAvailable, this, &DbQueryWidget::showLyricsFromIndex);
    connect(queryResults, &QueryResultsModel::coverAvailable, this, &DbQueryWidget::showCoverFromIndex);
}

QModelIndex DbQueryWidget::selectedIndex() const
{
    if(!m_model) {
        return QModelIndex();
    }
    const QItemSelectionModel *selectionModel = m_ui->resultsTreeView->selectionModel();
    if(!selectionModel) {
        return QModelIndex();
    }
    const QModelIndexList selection = selectionModel->selectedRows();
    if(selection.size() != 1) {
        return QModelIndex();
    }
    return selection.at(0);
}

}
