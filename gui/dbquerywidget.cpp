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
    m_model(nullptr)
{
    m_ui->setupUi(this);
#ifdef Q_OS_WIN32
    setStyleSheet(dialogStyle());
#endif

    m_ui->notificationLabel->setText(tr("Search hasn't been started."));
    m_ui->searchGroupBox->installEventFilter(this);

    // initialize buttons
    m_ui->abortPushButton->setIcon(style()->standardIcon(QStyle::SP_DialogCancelButton, nullptr, m_ui->abortPushButton));
    m_ui->abortPushButton->setVisible(false);
    m_ui->applyPushButton->setIcon(style()->standardIcon(QStyle::SP_DialogApplyButton, nullptr, m_ui->applyPushButton));

    // initialize fields model
    m_ui->fieldsListView->setModel(&Settings::dbQueryFields());

    // initialize search terms form
    insertSearchTermsFromTagEdit(m_tagEditorWidget->activeTagEdit());

    // restore settings
    m_ui->overrideCheckBox->setChecked(Settings::dbQueryOverride());

    // connect signals and slots
    connect(m_ui->abortPushButton, &QPushButton::clicked, this, &DbQueryWidget::abortSearch);
    connect(m_ui->startPushButton, &QPushButton::clicked, this, &DbQueryWidget::startSearch);
    connect(m_ui->applyPushButton, &QPushButton::clicked, this, &DbQueryWidget::applyResults);
    connect(m_tagEditorWidget, &TagEditorWidget::fileStatusChange, this, &DbQueryWidget::fileStatusChanged);
}

DbQueryWidget::~DbQueryWidget()
{
    Settings::dbQueryOverride() = m_ui->overrideCheckBox->isChecked();
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
                m_ui->trackSpinBox->setValue(trackValue.toPositionIntSet().position());
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

void DbQueryWidget::startSearch()
{
    // check whether enought search terms are supplied
    if(m_ui->titleLineEdit->text().isEmpty() && (!m_ui->trackSpinBox->value() || m_ui->albumLineEdit->text().isEmpty() || m_ui->artistLineEdit->text().isEmpty())) {
        m_ui->notificationLabel->setNotificationType(NotificationType::Critical);
        m_ui->notificationLabel->setText(tr("Insufficient search criteria supplied"));
        return;
    }

    // delete current model
    m_ui->resultsTreeView->setModel(nullptr);
    delete m_model;

    // show status
    m_ui->notificationLabel->setNotificationType(NotificationType::Progress);
    m_ui->notificationLabel->setText(tr("Retrieving ..."));
    setStatus(false);

    // get song description
    SongDescription desc;
    desc.title = m_ui->titleLineEdit->text();
    desc.album = m_ui->albumLineEdit->text();
    desc.artist = m_ui->artistLineEdit->text();
    desc.track = m_ui->trackSpinBox->value();

    // do actual query
    m_ui->resultsTreeView->setModel(m_model = queryMusicBrainz(desc));
    connect(m_model, &QueryResultsModel::resultsAvailable, this, &DbQueryWidget::showResults);
}

void DbQueryWidget::abortSearch()
{
    if(m_model && !m_model->areResultsAvailable()) {
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
            m_ui->resultsTreeView->selectionModel()->setCurrentIndex(m_model->index(0, 0), QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Columns);
            m_ui->applyPushButton->setEnabled(m_tagEditorWidget->activeTagEdit());
        }
        setStatus(true);
    }
}

void DbQueryWidget::setStatus(bool aborted)
{
    m_ui->abortPushButton->setVisible(!aborted);
    m_ui->startPushButton->setVisible(aborted);
    m_ui->applyPushButton->setVisible(aborted);
}

void DbQueryWidget::fileStatusChanged(bool , bool hasTags)
{
    m_ui->applyPushButton->setEnabled(hasTags && m_ui->resultsTreeView->selectionModel() && m_ui->resultsTreeView->selectionModel()->hasSelection());
    insertSearchTermsFromTagEdit(m_tagEditorWidget->activeTagEdit());
}

void DbQueryWidget::applyResults()
{
    if(m_model) {
        if(TagEdit *tagEdit = m_tagEditorWidget->activeTagEdit()) {
            if(const QItemSelectionModel *selectionModel = m_ui->resultsTreeView->selectionModel()) {
                const QModelIndexList selection = selectionModel->selection().indexes();
                if(!selection.isEmpty()) {
                    PreviousValueHandling previousValueHandling = m_ui->overrideCheckBox->isChecked()
                            ? PreviousValueHandling::Update : PreviousValueHandling::Keep;
                    for(const ChecklistItem &item : Settings::dbQueryFields().items()) {
                        if(item.isChecked()) {
                            const auto field = static_cast<KnownField>(item.id().toInt());
                            tagEdit->setValue(field, m_model->fieldValue(selection.front().row(), field), previousValueHandling);
                        }
                    }
                }
            }
        }
    }
}

bool DbQueryWidget::eventFilter(QObject *obj, QEvent *event)
{
    if(obj = m_ui->searchGroupBox) {
        switch(event->type()) {
        case QEvent::KeyRelease: {
            auto *keyEvent = static_cast<QKeyEvent *>(event);
            int key = keyEvent->key();
            switch(key) {
            case Qt::Key_Return:
                startSearch();
                break;
            default:
                ;
            }
            break;
        } default:
            ;
        }
        return QWidget::eventFilter(obj, event);
    }
}

}
