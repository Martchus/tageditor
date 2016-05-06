#include "./renamefilesdialog.h"
#include "./javascripthighlighter.h"

#include "../renamingutility/renamingengine.h"
#include "../renamingutility/filesystemitem.h"
#include "../renamingutility/filesystemitemmodel.h"
#include "../renamingutility/filteredfilesystemitemmodel.h"

#include "../application/settings.h"

#include "ui_renamefilesdialog.h"

#include <qtutilities/misc/dialogutils.h>

#include <QMessageBox>
#include <QFileDialog>
#include <QDir>
#include <QItemSelectionModel>
#include <QMenu>
#include <QClipboard>
#include <QTextStream>

using namespace Dialogs;
using namespace RenamingUtility;

namespace QtGui {

RenameFilesDialog::RenameFilesDialog(QWidget *parent) :
    QDialog(parent),
    m_ui(new Ui::RenameFilesDialog),
    m_itemsProcessed(0),
    m_errorsOccured(0),
    m_changingSelection(false),
    m_scriptModified(false)
{
    setAttribute(Qt::WA_QuitOnClose, false);
    m_ui->setupUi(this);
#ifdef Q_OS_WIN32
    setStyleSheet(dialogStyle() + QStringLiteral("QSplitter:handle { background-color: palette(base); }"));
#endif
    // setup javascript editor and script file selection
    QFont font(QStringLiteral("Courier"), 10);
    font.setFixedPitch(true);
    m_ui->javaScriptPlainTextEdit->setFont(font);
    m_highlighter = new JavaScriptHighlighter(m_ui->javaScriptPlainTextEdit->document());
    m_ui->externalScriptPage->setBackgroundRole(QPalette::Base);

    // setup preview tree view
    m_engine = new RemamingEngine(this);
    m_ui->currentTreeView->setModel(m_engine->currentModel());
    m_ui->previewTreeView->setModel(m_engine->previewModel());

    // setup notification label
    m_ui->notificationLabel->setHidden(true);

    // setup pasteScriptButton menu
    QMenu *pasteScriptButtonMenu = new QMenu(m_ui->pasteScriptPushButton);
    pasteScriptButtonMenu->addAction(tr("from file"), this, SLOT(showSelectScriptFileDlg()));
    pasteScriptButtonMenu->addAction(tr("from clipboard"), this, SLOT(pasteScriptFromClipboard()));
    pasteScriptButtonMenu->addAction(tr("default script"), this, SLOT(pasteDefaultExampleScript()));
    m_ui->pasteScriptPushButton->setMenu(pasteScriptButtonMenu);

    // setup icons
    m_ui->generatePreviewPushButton->setIcon(style()->standardIcon(QStyle::SP_BrowserReload, nullptr, m_ui->generatePreviewPushButton));
    m_ui->applyChangingsPushButton->setIcon(style()->standardIcon(QStyle::SP_DialogApplyButton, nullptr, m_ui->applyChangingsPushButton));
    m_ui->applyChangingsPushButton->setEnabled(false);
    m_ui->abortClosePushButton->setIcon(style()->standardIcon(QStyle::SP_DialogCancelButton, nullptr, m_ui->abortClosePushButton));

    // restore settings
    if(Settings::scriptSource() < m_ui->sourceFileStackedWidget->count()) {
        m_ui->sourceFileStackedWidget->setCurrentIndex(Settings::scriptSource());
    }
    m_ui->scriptFilePathLineEdit->setText(Settings::externalScript());
    if(!Settings::editorScript().isEmpty()) {
        m_ui->javaScriptPlainTextEdit->setPlainText(Settings::editorScript());
        m_scriptModified = true;
    } else {
        pasteDefaultExampleScript();
    }

    // connect signals and slots
    connect(m_ui->generatePreviewPushButton, &QPushButton::clicked, this, &RenameFilesDialog::startGeneratingPreview);
    connect(m_ui->applyChangingsPushButton, &QPushButton::clicked, this, &RenameFilesDialog::startApplyChangings);
    connect(m_ui->abortClosePushButton, &QPushButton::clicked, this, &RenameFilesDialog::abortClose);
    connect(m_engine, &RemamingEngine::previewGenerated, this, &RenameFilesDialog::showPreviewResults);
    connect(m_engine, &RemamingEngine::changingsApplied, this, &RenameFilesDialog::showChangsingsResults);
    connect(m_engine, &RemamingEngine::progress, this, &RenameFilesDialog::showPreviewProgress);
    connect(m_ui->currentTreeView, &QTreeView::customContextMenuRequested, this, &RenameFilesDialog::showTreeViewContextMenu);
    connect(m_ui->previewTreeView, &QTreeView::customContextMenuRequested, this, &RenameFilesDialog::showTreeViewContextMenu);
    connect(m_ui->currentTreeView->selectionModel(), &QItemSelectionModel::selectionChanged, this, &RenameFilesDialog::currentItemSelected);
    connect(m_ui->previewTreeView->selectionModel(), &QItemSelectionModel::selectionChanged, this, &RenameFilesDialog::previewItemSelected);
    connect(m_ui->toggleScriptSourcePushButton, &QPushButton::clicked, this, &RenameFilesDialog::toggleScriptSource);
    connect(m_ui->selectScriptFilePushButton, &QPushButton::clicked, this, &RenameFilesDialog::showScriptFileSelectionDlg);
    connect(m_ui->javaScriptPlainTextEdit, &QPlainTextEdit::undoAvailable, this, &RenameFilesDialog::setScriptModified);
}

RenameFilesDialog::~RenameFilesDialog()
{}

QString RenameFilesDialog::directory() const
{
    return m_ui->directoryWidget->lineEdit()->text();
}

void RenameFilesDialog::setDirectory(const QString &directory)
{
    m_ui->directoryWidget->lineEdit()->setText(directory);
}

bool RenameFilesDialog::event(QEvent *event)
{
    switch(event->type()) {
    case QEvent::Close:
        // save settings
        Settings::scriptSource() = m_ui->sourceFileStackedWidget->currentIndex();
        Settings::externalScript() = m_ui->scriptFilePathLineEdit->text();
        if(m_scriptModified) {
            Settings::editorScript() = m_ui->javaScriptPlainTextEdit->toPlainText();
        } else {
            Settings::editorScript().clear();
        }
        break;
    default:
        ;
    }
    return QDialog::event(event);
}

void RenameFilesDialog::showScriptFileSelectionDlg()
{
    QString file = QFileDialog::getOpenFileName(this, QApplication::applicationName(), m_ui->scriptFilePathLineEdit->text());
    if(!file.isEmpty()) {
        m_ui->scriptFilePathLineEdit->setText(file);
    }
}

void RenameFilesDialog::startGeneratingPreview()
{
    if(!m_engine->isBusy()) {
        QDir selectedDir(directory());
        m_ui->notificationLabel->setHidden(false);
        if(selectedDir.exists()) {
            QString program;
            if(m_ui->sourceFileStackedWidget->currentIndex() == 0) {
                program = m_ui->javaScriptPlainTextEdit->toPlainText();
            } else {
                QString fileName = m_ui->scriptFilePathLineEdit->text();
                if(fileName.isEmpty()) {
                    m_ui->notificationLabel->setText(tr("There is no external script file is selected."));
                } else {
                    QFile file(fileName);
                    if(file.open(QFile::ReadOnly)) {
                        QTextStream textStream(&file);
                        program = textStream.readAll();
                    } else {
                        m_ui->notificationLabel->setText(tr("Unable to open external script file."));
                    }
                }
            }
            if(!program.isEmpty()) {
                if(m_engine->setProgram(program)) {
                    m_ui->notificationLabel->setText(tr("Generating preview ..."));
                    m_ui->notificationLabel->setNotificationType(NotificationType::Progress);
                    m_ui->abortClosePushButton->setText(tr("Abort"));
                    m_ui->generatePreviewPushButton->setHidden(true);
                    m_ui->applyChangingsPushButton->setHidden(true);
                    m_engine->generatePreview(directory(), m_ui->includeSubdirsCheckBox->isChecked());
                } else {
                    m_engine->clearPreview();
                    if(m_engine->errorLineNumber()) {
                        m_ui->notificationLabel->setText(tr("The script is not valid.\nError in line %1: %3")
                                                        .arg(m_engine->errorLineNumber()).arg(m_engine->errorMessage()));
                    } else {
                        m_ui->notificationLabel->setText(tr("An error occured when parsing the script: %1").arg(m_engine->errorMessage()));
                    }
                    m_ui->notificationLabel->setNotificationType(NotificationType::Warning);
                }
            } else {
                m_engine->clearPreview();
                m_ui->notificationLabel->setNotificationType(NotificationType::Warning);
                if(m_ui->notificationLabel->text().isEmpty()) {
                    m_ui->notificationLabel->setText(tr("The script is empty."));
                }
            }
        } else {
            m_engine->clearPreview();
            m_ui->notificationLabel->setText(tr("The selected directory doesn't exist."));
            m_ui->notificationLabel->setNotificationType(NotificationType::Warning);
        }
    }
}


void RenameFilesDialog::startApplyChangings()
{
    if(!m_engine->isBusy()) {
        m_ui->notificationLabel->setText(tr("Applying changings ..."));
        m_ui->notificationLabel->setNotificationType(NotificationType::Progress);
        m_ui->abortClosePushButton->setText(tr("Abort"));
        m_ui->generatePreviewPushButton->setHidden(true);
        m_ui->applyChangingsPushButton->setHidden(true);
        m_engine->applyChangings();
    }
}

void RenameFilesDialog::showPreviewProgress(int itemsProcessed, int errorsOccured)
{
    m_itemsProcessed = itemsProcessed;
    m_errorsOccured = errorsOccured;
    QString text = tr("%1 files/directories processed", 0, itemsProcessed).arg(itemsProcessed);
    if(m_errorsOccured > 0) {
        text.append(QChar('\n'));
        text.append(tr("%1 error(s) occured", 0, errorsOccured).arg(errorsOccured));
    }
    m_ui->notificationLabel->setText(text);
}

void RenameFilesDialog::showPreviewResults()
{
    m_ui->abortClosePushButton->setText(tr("Close"));
    m_ui->generatePreviewPushButton->setHidden(false);
    m_ui->applyChangingsPushButton->setHidden(false);
    if(m_engine->rootItem()) {
        m_ui->notificationLabel->setText(tr("Preview has been generated."));
        m_ui->notificationLabel->appendLine(tr("%1 files/directories have been processed.", 0, m_itemsProcessed).arg(m_itemsProcessed));
        m_ui->notificationLabel->setNotificationType(NotificationType::Information);
        m_ui->applyChangingsPushButton->setEnabled(true);
    } else {
        m_ui->notificationLabel->setText(tr("No files and directories have been found."));
        m_ui->notificationLabel->setNotificationType(NotificationType::Warning);
        m_ui->applyChangingsPushButton->setEnabled(false);
    }
    if(m_engine->isAborted()) {
        m_ui->notificationLabel->appendLine(tr("Generation of preview has been aborted prematurely."));
        m_ui->notificationLabel->setNotificationType(NotificationType::Warning);
    }
    if(m_errorsOccured) {
        m_ui->notificationLabel->appendLine(tr("%1 error(s) occured.", 0, m_errorsOccured).arg(m_errorsOccured));
        m_ui->notificationLabel->setNotificationType(NotificationType::Warning);
    }
}

void RenameFilesDialog::showChangsingsResults()
{
    m_ui->abortClosePushButton->setText(tr("Close"));
    m_ui->generatePreviewPushButton->setHidden(false);
    m_ui->applyChangingsPushButton->setHidden(false);
    m_ui->notificationLabel->setText(tr("Changins applied."));
    m_ui->notificationLabel->appendLine(tr("%1 files/directories have been processed.", 0, m_itemsProcessed).arg(m_itemsProcessed));
    m_ui->notificationLabel->setNotificationType(NotificationType::Information);
    if(m_engine->isAborted()) {
        m_ui->notificationLabel->appendLine(tr("Applying has been aborted prematurely."));
        m_ui->notificationLabel->setNotificationType(NotificationType::Warning);
    }
    if(m_errorsOccured) {
        m_ui->notificationLabel->appendLine(tr("%1 error(s) occured.", 0, m_errorsOccured).arg(m_errorsOccured));
        m_ui->notificationLabel->setNotificationType(NotificationType::Warning);
    }
}

void RenameFilesDialog::currentItemSelected(const QItemSelection &, const QItemSelection &)
{
    if(!m_changingSelection) {
        m_changingSelection = true;
        m_ui->previewTreeView->selectionModel()->clear();
        for(const QModelIndex &row : m_ui->currentTreeView->selectionModel()->selectedRows()) {
            QModelIndex currentIndex = m_engine->currentModel()->mapToSource(row);
            QModelIndex counterpartIndex = m_engine->model()->counterpart(
                        currentIndex, 1);
            if(!counterpartIndex.isValid()) {
                counterpartIndex = currentIndex;
            }
            QModelIndex previewIndex = m_engine->previewModel()->mapFromSource(counterpartIndex);
            if(previewIndex.isValid()) {
                QModelIndex parent = previewIndex.parent();
                if(parent.isValid()) {
                    m_ui->previewTreeView->expand(m_engine->previewModel()->index(parent.row(), parent.column(), parent.parent()));
                }
                m_ui->previewTreeView->scrollTo(previewIndex);
                m_ui->previewTreeView->selectionModel()->select(previewIndex, QItemSelectionModel::Rows | QItemSelectionModel::Select);
            }
        }
        m_changingSelection = false;
    }
}

void RenameFilesDialog::previewItemSelected(const QItemSelection &, const QItemSelection &)
{
    if(!m_changingSelection) {
        m_changingSelection = true;
        m_ui->currentTreeView->selectionModel()->clear();
        for(const QModelIndex &row : m_ui->previewTreeView->selectionModel()->selectedRows()) {
            QModelIndex previewIndex = m_engine->previewModel()->mapToSource(row);
            QModelIndex counterpartIndex = m_engine->model()->counterpart(
                        previewIndex, 0);
            if(!counterpartIndex.isValid()) {
                counterpartIndex = previewIndex;
            }
            QModelIndex currentIndex = m_engine->currentModel()->mapFromSource(counterpartIndex);
            if(currentIndex.isValid()) {
                QModelIndex parent = currentIndex.parent();
                if(parent.isValid()) {
                    m_ui->currentTreeView->expand(m_engine->currentModel()->index(parent.row(), parent.column(), parent.parent()));
                }
                m_ui->currentTreeView->scrollTo(currentIndex);
                m_ui->currentTreeView->selectionModel()->select(currentIndex, QItemSelectionModel::Rows | QItemSelectionModel::Select);
            }
        }
        m_changingSelection = false;
    }
}

void RenameFilesDialog::pasteScriptFromFile(const QString &fileName)
{
    QFile file(fileName);
    if(file.open(QFile::ReadOnly)) {
        QTextStream textStream(&file);
        m_ui->javaScriptPlainTextEdit->setPlainText(textStream.readAll());
    } else {
        QMessageBox::warning(this, windowTitle(), tr("Unable to open script file."));
    }
}

void RenameFilesDialog::showSelectScriptFileDlg()
{
    QString fileName = QFileDialog::getOpenFileName(this, tr("Select script"));
    if(!fileName.isEmpty()) {
        pasteScriptFromFile(fileName);
    }
}

void RenameFilesDialog::abortClose()
{
    if(m_engine->isBusy()) {
        m_engine->abort();
    } else {
        close();
    }
}

void RenameFilesDialog::pasteScriptFromClipboard()
{
    QString script = QApplication::clipboard()->text();
    if(!script.isEmpty()) {
        m_ui->javaScriptPlainTextEdit->setPlainText(script);
    } else {
        QMessageBox::warning(this, windowTitle(), tr("Clipboard contains no text."));
    }
}

void RenameFilesDialog::pasteDefaultExampleScript()
{
    pasteScriptFromFile(QStringLiteral(":/scripts/renamefiles/example1"));
}

void RenameFilesDialog::showTreeViewContextMenu()
{
    if(QObject *sender = QObject::sender()) {
        QMenu menu;
        menu.addAction(tr("Expand all"), sender, SLOT(expandAll()));
        menu.addAction(tr("Collapse all"), sender, SLOT(collapseAll()));
        menu.exec(QCursor::pos());
    }
}

void RenameFilesDialog::toggleScriptSource()
{
    int nextIndex;
    switch(m_ui->sourceFileStackedWidget->currentIndex()) {
    case 0:
        nextIndex = 1;
        break;
    default:
        nextIndex = 0;
    }
    m_ui->sourceFileStackedWidget->setCurrentIndex(nextIndex);
    switch(nextIndex) {
    case 0:
        m_ui->pasteScriptPushButton->setVisible(true);
        m_ui->toggleScriptSourcePushButton->setText("Use external file");
        break;
    default:
        m_ui->pasteScriptPushButton->setVisible(false);
        m_ui->toggleScriptSourcePushButton->setText("Use editor");
    }
}

void RenameFilesDialog::setScriptModified(bool scriptModified)
{
    m_scriptModified = scriptModified;
}

}
