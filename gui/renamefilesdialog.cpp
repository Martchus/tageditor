#include "./renamefilesdialog.h"
#include "./javascripthighlighter.h"

#include "../renamingutility/filesystemitem.h"
#include "../renamingutility/filesystemitemmodel.h"
#include "../renamingutility/filteredfilesystemitemmodel.h"
#include "../renamingutility/renamingengine.h"

#include "../application/settings.h"

#include "ui_renamefilesdialog.h"

#include <qtutilities/misc/dialogutils.h>

#include <QClipboard>
#include <QDir>
#include <QFileDialog>
#include <QFontDatabase>
#include <QItemSelectionModel>
#include <QMenu>
#include <QMessageBox>
#include <QStyle>
#include <QTextStream>

using namespace QtUtilities;
using namespace RenamingUtility;

namespace QtGui {

RenameFilesDialog::RenameFilesDialog(QWidget *parent)
    : QDialog(parent)
    , m_ui(new Ui::RenameFilesDialog)
    , m_itemsProcessed(0)
    , m_errorsOccured(0)
    , m_changingSelection(false)
    , m_scriptModified(false)
{
    setAttribute(Qt::WA_QuitOnClose, false);
    m_ui->setupUi(this);
#ifdef Q_OS_WIN32
    setStyleSheet(dialogStyle() + QStringLiteral("QSplitter:handle { background-color: palette(base); }"));
#endif
    // setup javascript editor and script file selection
    m_ui->javaScriptPlainTextEdit->setFont(QFontDatabase::systemFont(QFontDatabase::FixedFont));
    m_highlighter = new JavaScriptHighlighter(m_ui->javaScriptPlainTextEdit->document());
    m_ui->externalScriptPage->setBackgroundRole(QPalette::Base);

    // setup preview tree view
    m_engine = new RenamingEngine(this);
    m_ui->currentTreeView->setModel(m_engine->currentModel());
    m_ui->previewTreeView->setModel(m_engine->previewModel());

    // setup notification label
    m_ui->notificationLabel->setHidden(true);

    // setup pasteScriptButton menu
    auto *const pasteScriptButtonMenu = new QMenu(m_ui->pasteScriptPushButton);
    pasteScriptButtonMenu->addAction(tr("File"), this, &RenameFilesDialog::showSelectScriptFileDlg);
    pasteScriptButtonMenu->addAction(tr("Paste from clipboard"), this, &RenameFilesDialog::pasteScriptFromClipboard);
    pasteScriptButtonMenu->addAction(tr("Simple example"), this, &RenameFilesDialog::pasteSimpleExampleScript);
    pasteScriptButtonMenu->addAction(tr("Advanced example"), this, &RenameFilesDialog::pasteAdvancedExampleScript);
    m_ui->pasteScriptPushButton->setMenu(pasteScriptButtonMenu);

    // setup icons
    m_ui->generatePreviewPushButton->setIcon(style()->standardIcon(QStyle::SP_BrowserReload, nullptr, m_ui->generatePreviewPushButton));
    m_ui->applyChangingsPushButton->setIcon(style()->standardIcon(QStyle::SP_DialogApplyButton, nullptr, m_ui->applyChangingsPushButton));
    m_ui->applyChangingsPushButton->setEnabled(false);
    m_ui->abortClosePushButton->setIcon(style()->standardIcon(QStyle::SP_DialogCancelButton, nullptr, m_ui->abortClosePushButton));

    // restore settings
    const auto &settings = Settings::values().renamingUtility;
    if (Settings::values().renamingUtility.scriptSource < m_ui->sourceFileStackedWidget->count()) {
        m_ui->sourceFileStackedWidget->setCurrentIndex(settings.scriptSource);
    }
    m_ui->scriptFilePathLineEdit->setText(settings.externalScript);
    if (!Settings::values().renamingUtility.editorScript.isEmpty()) {
        m_ui->javaScriptPlainTextEdit->setPlainText(settings.editorScript);
        m_scriptModified = true;
    } else {
        pasteSimpleExampleScript();
    }

    // connect signals and slots
    connect(m_ui->generatePreviewPushButton, &QPushButton::clicked, this, &RenameFilesDialog::startGeneratingPreview);
    connect(m_ui->applyChangingsPushButton, &QPushButton::clicked, this, &RenameFilesDialog::startApplyChangings);
    connect(m_ui->abortClosePushButton, &QPushButton::clicked, this, &RenameFilesDialog::abortClose);
    connect(m_engine, &RenamingEngine::previewGenerated, this, &RenameFilesDialog::showPreviewResults);
    connect(m_engine, &RenamingEngine::changingsApplied, this, &RenameFilesDialog::showChangsingsResults);
    connect(m_engine, &RenamingEngine::progress, this, &RenameFilesDialog::showPreviewProgress);
    connect(m_ui->currentTreeView, &QTreeView::customContextMenuRequested, this, &RenameFilesDialog::showTreeViewContextMenu);
    connect(m_ui->previewTreeView, &QTreeView::customContextMenuRequested, this, &RenameFilesDialog::showTreeViewContextMenu);
    connect(m_ui->currentTreeView->selectionModel(), &QItemSelectionModel::selectionChanged, this, &RenameFilesDialog::currentItemSelected);
    connect(m_ui->previewTreeView->selectionModel(), &QItemSelectionModel::selectionChanged, this, &RenameFilesDialog::previewItemSelected);
    connect(m_ui->toggleScriptSourcePushButton, &QPushButton::clicked, this, &RenameFilesDialog::toggleScriptSource);
    connect(m_ui->selectScriptFilePushButton, &QPushButton::clicked, this, &RenameFilesDialog::showScriptFileSelectionDlg);
    connect(m_ui->javaScriptPlainTextEdit, &QPlainTextEdit::undoAvailable, this, &RenameFilesDialog::setScriptModified);
}

RenameFilesDialog::~RenameFilesDialog()
{
}

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
    auto &settings = Settings::values().renamingUtility;
    switch (event->type()) {
    case QEvent::Close:
        // save settings
        settings.scriptSource = m_ui->sourceFileStackedWidget->currentIndex();
        settings.externalScript = m_ui->scriptFilePathLineEdit->text();
        if (m_scriptModified) {
            settings.editorScript = m_ui->javaScriptPlainTextEdit->toPlainText();
        } else {
            settings.editorScript.clear();
        }
        break;
    default:;
    }
    return QDialog::event(event);
}

void RenameFilesDialog::showScriptFileSelectionDlg()
{
    QString file = QFileDialog::getOpenFileName(this, QApplication::applicationName(), m_ui->scriptFilePathLineEdit->text());
    if (!file.isEmpty()) {
        m_ui->scriptFilePathLineEdit->setText(file);
    }
}

void RenameFilesDialog::startGeneratingPreview()
{
    if (m_engine->isBusy()) {
        return;
    }
    QDir selectedDir(directory());
    m_ui->notificationLabel->setHidden(false);
    if (selectedDir.exists()) {
        QString program;
        if (m_ui->sourceFileStackedWidget->currentIndex() == 0) {
            program = m_ui->javaScriptPlainTextEdit->toPlainText();
        } else {
            QString fileName = m_ui->scriptFilePathLineEdit->text();
            if (fileName.isEmpty()) {
                m_ui->notificationLabel->setText(tr("There is no external script file is selected."));
            } else {
                QFile file(fileName);
                if (file.open(QFile::ReadOnly)) {
                    QTextStream textStream(&file);
                    program = textStream.readAll();
                } else {
                    m_ui->notificationLabel->setText(tr("Unable to open external script file."));
                }
            }
        }
        if (!program.isEmpty()) {
            if (m_engine->setProgram(program)) {
                m_ui->notificationLabel->setText(tr("Generating preview ..."));
                m_ui->notificationLabel->setNotificationType(NotificationType::Progress);
                m_ui->abortClosePushButton->setText(tr("Abort"));
                m_ui->generatePreviewPushButton->setHidden(true);
                m_ui->applyChangingsPushButton->setHidden(true);
                m_engine->generatePreview(directory(), m_ui->includeSubdirsCheckBox->isChecked());
            } else {
                m_engine->clearPreview();
                if (m_engine->errorLineNumber()) {
                    m_ui->notificationLabel->setText(
                        tr("The script is not valid.\nError in line %1: %3").arg(m_engine->errorLineNumber()).arg(m_engine->errorMessage()));
                } else {
                    m_ui->notificationLabel->setText(tr("An error occured when parsing the script: %1").arg(m_engine->errorMessage()));
                }
                m_ui->notificationLabel->setNotificationType(NotificationType::Warning);
            }
        } else {
            m_engine->clearPreview();
            m_ui->notificationLabel->setNotificationType(NotificationType::Warning);
            if (m_ui->notificationLabel->text().isEmpty()) {
                m_ui->notificationLabel->setText(tr("The script is empty."));
            }
        }
    } else {
        m_engine->clearPreview();
        m_ui->notificationLabel->setText(tr("The selected directory doesn't exist."));
        m_ui->notificationLabel->setNotificationType(NotificationType::Warning);
    }
}

void RenameFilesDialog::startApplyChangings()
{
    if (m_engine->isBusy()) {
        return;
    }
    m_ui->notificationLabel->setText(tr("Applying changings ..."));
    m_ui->notificationLabel->setNotificationType(NotificationType::Progress);
    m_ui->abortClosePushButton->setText(tr("Abort"));
    m_ui->generatePreviewPushButton->setHidden(true);
    m_ui->applyChangingsPushButton->setHidden(true);
    m_engine->applyChangings();
}

void RenameFilesDialog::showPreviewProgress(int itemsProcessed, int errorsOccured)
{
    m_itemsProcessed = itemsProcessed;
    m_errorsOccured = errorsOccured;
    QString text = tr("%1 files/directories processed", nullptr, itemsProcessed).arg(itemsProcessed);
    if (m_errorsOccured > 0) {
        text.append(QChar('\n'));
        text.append(tr("%1 error(s) occured", nullptr, errorsOccured).arg(errorsOccured));
    }
    m_ui->notificationLabel->setText(text);
}

void RenameFilesDialog::showPreviewResults()
{
    m_ui->abortClosePushButton->setText(tr("Close"));
    m_ui->generatePreviewPushButton->setHidden(false);
    m_ui->applyChangingsPushButton->setHidden(false);
    if (m_engine->rootItem()) {
        m_ui->notificationLabel->setText(tr("Preview has been generated."));
        m_ui->notificationLabel->appendLine(tr("%1 files/directories have been processed.", nullptr, m_itemsProcessed).arg(m_itemsProcessed));
        m_ui->notificationLabel->setNotificationType(NotificationType::Information);
        m_ui->applyChangingsPushButton->setEnabled(true);
    } else {
        m_ui->notificationLabel->setText(tr("No files and directories have been found."));
        m_ui->notificationLabel->setNotificationType(NotificationType::Warning);
        m_ui->applyChangingsPushButton->setEnabled(false);
    }
    if (m_engine->isAborted()) {
        m_ui->notificationLabel->appendLine(tr("Generation of preview has been aborted prematurely."));
        m_ui->notificationLabel->setNotificationType(NotificationType::Warning);
    }
    if (m_errorsOccured) {
        m_ui->notificationLabel->appendLine(tr("%1 error(s) occured.", nullptr, m_errorsOccured).arg(m_errorsOccured));
        m_ui->notificationLabel->setNotificationType(NotificationType::Warning);
    }
}

void RenameFilesDialog::showChangsingsResults()
{
    m_ui->abortClosePushButton->setText(tr("Close"));
    m_ui->generatePreviewPushButton->setHidden(false);
    m_ui->applyChangingsPushButton->setHidden(false);
    m_ui->notificationLabel->setText(tr("Changins applied."));
    m_ui->notificationLabel->appendLine(tr("%1 files/directories have been processed.", nullptr, m_itemsProcessed).arg(m_itemsProcessed));
    m_ui->notificationLabel->setNotificationType(NotificationType::Information);
    if (m_engine->isAborted()) {
        m_ui->notificationLabel->appendLine(tr("Applying has been aborted prematurely."));
        m_ui->notificationLabel->setNotificationType(NotificationType::Warning);
    }
    if (m_errorsOccured) {
        m_ui->notificationLabel->appendLine(tr("%1 error(s) occured.", nullptr, m_errorsOccured).arg(m_errorsOccured));
        m_ui->notificationLabel->setNotificationType(NotificationType::Warning);
    }
}

void RenameFilesDialog::currentItemSelected(const QItemSelection &, const QItemSelection &)
{
    if (m_changingSelection) {
        return;
    }

    m_changingSelection = true;
    m_ui->previewTreeView->selectionModel()->clear();
    for (const QModelIndex &row : m_ui->currentTreeView->selectionModel()->selectedRows()) {
        QModelIndex currentIndex = m_engine->currentModel()->mapToSource(row);
        QModelIndex counterpartIndex = m_engine->model()->counterpart(currentIndex, 1);
        if (!counterpartIndex.isValid()) {
            counterpartIndex = currentIndex;
        }
        QModelIndex previewIndex = m_engine->previewModel()->mapFromSource(counterpartIndex);
        if (previewIndex.isValid()) {
            QModelIndex parent = previewIndex.parent();
            if (parent.isValid()) {
                m_ui->previewTreeView->expand(m_engine->previewModel()->index(parent.row(), parent.column(), parent.parent()));
            }
            m_ui->previewTreeView->scrollTo(previewIndex);
            m_ui->previewTreeView->selectionModel()->select(previewIndex, QItemSelectionModel::Rows | QItemSelectionModel::Select);
        }
    }
    m_changingSelection = false;
}

void RenameFilesDialog::previewItemSelected(const QItemSelection &, const QItemSelection &)
{
    if (m_changingSelection) {
        return;
    }

    m_changingSelection = true;
    m_ui->currentTreeView->selectionModel()->clear();
    for (const QModelIndex &row : m_ui->previewTreeView->selectionModel()->selectedRows()) {
        QModelIndex previewIndex = m_engine->previewModel()->mapToSource(row);
        QModelIndex counterpartIndex = m_engine->model()->counterpart(previewIndex, 0);
        if (!counterpartIndex.isValid()) {
            counterpartIndex = previewIndex;
        }
        QModelIndex currentIndex = m_engine->currentModel()->mapFromSource(counterpartIndex);
        if (currentIndex.isValid()) {
            QModelIndex parent = currentIndex.parent();
            if (parent.isValid()) {
                m_ui->currentTreeView->expand(m_engine->currentModel()->index(parent.row(), parent.column(), parent.parent()));
            }
            m_ui->currentTreeView->scrollTo(currentIndex);
            m_ui->currentTreeView->selectionModel()->select(currentIndex, QItemSelectionModel::Rows | QItemSelectionModel::Select);
        }
    }
    m_changingSelection = false;
}

void RenameFilesDialog::pasteScriptFromFile(const QString &fileName)
{
    QFile file(fileName);
    if (file.open(QFile::ReadOnly)) {
        QTextStream textStream(&file);
        m_ui->javaScriptPlainTextEdit->setPlainText(textStream.readAll());
    } else {
        QMessageBox::warning(this, windowTitle(), tr("Unable to open script file."));
    }
}

void RenameFilesDialog::showSelectScriptFileDlg()
{
    QString fileName = QFileDialog::getOpenFileName(this, tr("Select script"));
    if (!fileName.isEmpty()) {
        pasteScriptFromFile(fileName);
    }
}

void RenameFilesDialog::abortClose()
{
    if (m_engine->isBusy()) {
        m_engine->abort();
    } else {
        close();
    }
}

void RenameFilesDialog::pasteScriptFromClipboard()
{
    const QString script = QApplication::clipboard()->text();
    if (script.isEmpty()) {
        QMessageBox::warning(this, windowTitle(), tr("Clipboard contains no text."));
        return;
    }

    m_ui->javaScriptPlainTextEdit->setPlainText(script);
}

void RenameFilesDialog::pasteSimpleExampleScript()
{
    pasteScriptFromFile(QStringLiteral(":/scripts/renamefiles/simple-example"));
}

void RenameFilesDialog::pasteAdvancedExampleScript()
{
    pasteScriptFromFile(QStringLiteral(":/scripts/renamefiles/advanced-example"));
}

void RenameFilesDialog::showTreeViewContextMenu(const QPoint &position)
{
    if (const QTreeView *sender = qobject_cast<const QTreeView *>(QObject::sender())) {
        QMenu menu;
        menu.addAction(tr("Expand all"), sender, &QTreeView::expandAll);
        menu.addAction(tr("Collapse all"), sender, &QTreeView::collapseAll);
        menu.exec(sender->viewport()->mapToGlobal(position));
    }
}

void RenameFilesDialog::toggleScriptSource()
{
    int nextIndex;
    switch (m_ui->sourceFileStackedWidget->currentIndex()) {
    case 0:
        nextIndex = 1;
        break;
    default:
        nextIndex = 0;
    }
    m_ui->sourceFileStackedWidget->setCurrentIndex(nextIndex);
    switch (nextIndex) {
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

} // namespace QtGui
