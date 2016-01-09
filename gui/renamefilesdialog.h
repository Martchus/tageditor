#ifndef RENAMEFILESDIALOG_H
#define RENAMEFILESDIALOG_H

#include <QDialog>
#include <QDir>

#include <memory>

QT_FORWARD_DECLARE_CLASS(QItemSelection)

namespace RenamingUtility {
class RemamingEngine;
}

namespace QtGui {

namespace Ui {
class RenameFilesDialog;
}

class JavaScriptHighlighter;

class RenameFilesDialog : public QDialog
{
    Q_OBJECT
    Q_PROPERTY(QString directory READ directory WRITE setDirectory)
    
public:
    explicit RenameFilesDialog(QWidget *parent = nullptr);
    ~RenameFilesDialog();
    
    QString directory() const;
    void setDirectory(const QString &directory);

protected:
    bool event(QEvent *event);

private slots:
    void showDirectorySelectionDlg();
    void showScriptFileSelectionDlg();
    void startGeneratingPreview();
    void startApplyChangings();
    void showPreviewProgress(int itemsProcessed, int errorsOccured);
    void showPreviewResults();
    void showChangsingsResults();
    void currentItemSelected(const QItemSelection & selected, const QItemSelection & deselected);
    void previewItemSelected(const QItemSelection & selected, const QItemSelection & deselected);
    void pasteScriptFromFile(const QString &fileName);
    void pasteScriptFromClipboard();
    void pasteDefaultExampleScript();
    void showTreeViewContextMenu();
    void showSelectScriptFileDlg();
    void abortClose();
    void toggleScriptSource();

private:
    std::unique_ptr<Ui::RenameFilesDialog> m_ui;
    JavaScriptHighlighter *m_highlighter;
    RenamingUtility::RemamingEngine *m_engine;
    int m_itemsProcessed;
    int m_errorsOccured;
    bool m_changingSelection;
};

}

#endif // RENAMEFILESDIALOG_H
