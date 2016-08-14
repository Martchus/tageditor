#ifndef TAGEDITORMAINWINDOW_H
#define TAGEDITORMAINWINDOW_H

#include "./filefilterproxymodel.h"
#include "./previousvaluehandling.h"

#include <tagparser/mediafileinfo.h>
#include <tagparser/tagvalue.h>

#include <QMainWindow>
#include <QByteArray>

QT_FORWARD_DECLARE_CLASS(QFileSystemModel)
QT_FORWARD_DECLARE_CLASS(QItemSelectionModel)
QT_FORWARD_DECLARE_CLASS(QMutex)

namespace Media {
DECLARE_ENUM_CLASS(TagType, unsigned int);
}

namespace Dialogs {
class AboutDialog;
class SettingsDialog;
}

namespace QtGui {

namespace Ui {
class MainWindow;
}

class TagEditorWidget;
class RenameFilesDialog;
class DbQueryWidget;

class MainWindow : public QMainWindow
{
    Q_OBJECT
    
public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

    // file browser
    QString currentDirectory();
    void setCurrentDirectory(const QString &path);
    void startParsing(const QString &path);

protected:
    bool event(QEvent *event);

private slots:
    // file selection
    void pathEntered();
    void fileSelected();
    void selectNextFile();
    void selectNextFile(QItemSelectionModel *selectionModel, const QModelIndex &currentIndex, bool notDeeper);
    void showNextFileNotFound();
    void showOpenFileDlg();
    void showSaveAsDlg();
    void saveFileInformation();
    void handleFileStatusChange(bool opened, bool hasTag);
    void handleCurrentPathChanged(const QString &newPath);

    // settings
    void showSettingsDlg();
    void applySettingsFromDialog();

    // misc
    void showAboutDlg();
    void showRenameFilesDlg();
    void spawnExternalPlayer();
    void showDbQueryWidget();

private:
    QMutex &fileOperationMutex();
    Media::MediaFileInfo &fileInfo();

    // UI
    std::unique_ptr<Ui::MainWindow> m_ui;
    // models
    QFileSystemModel *m_fileModel;
    FileFilterProxyModel *m_fileFilterModel;
    bool m_internalFileSelection;
    // dialogs
    Dialogs::AboutDialog *m_aboutDlg;
    Dialogs::SettingsDialog *m_settingsDlg;
    std::unique_ptr<RenameFilesDialog> m_renameFilesDlg;
    DbQueryWidget *m_dbQueryWidget;
};

}

#endif // TAGEDITORMAINWINDOW_H
