#ifndef TAGEDITORDIALOG_H
#define TAGEDITORDIALOG_H

#include "filefilterproxymodel.h"
#include "previousvaluehandling.h"

#include <tagparser/mediafileinfo.h>
#include <tagparser/tagvalue.h>

#include <QMainWindow>
#include <QByteArray>

#include <string>
#include <mutex>
#include <functional>

QT_BEGIN_NAMESPACE
class QLineEdit;
class QComboBox;
class QSpinBox;
class QPlainTextEdit;
class QGraphicsScene;
class QFileSystemModel;
QT_END_NAMESPACE

namespace Media {
DECLARE_ENUM(TagType, unsigned int)
}

namespace Dialogs {
class AboutDialog;
class SettingsDialog;
}

namespace QtGui {

namespace Ui {
class MainWindow;
}

class TagEdit;
class RenameFilesDialog;

class MainWindow : public QMainWindow
{
    Q_OBJECT
    
public:
    explicit MainWindow(QWidget *parent = nullptr);
    virtual ~MainWindow();

    // file browser
    QString currentDirectory();
    void setCurrentDirectory(const QString &path);

    // opened file: load, save, delete, close
    bool startParsing(const QString &path, bool forceRefresh = false);
    bool applyEntriesAndSaveChangings();
    bool deleteAllTagsAndSave();
    void closeFile();

protected:
    virtual void closeEvent(QCloseEvent *);
    virtual bool eventFilter(QObject *obj, QEvent *event);

private slots:
    // file selection
    void pathEntered();
    void fileSelected();
    void saveAndShowNextFile();
    void selectNextFile();
    void showOpenFileDlg();

    // editor
    void showFile(char result);
    void saveFileInformation();
    void handleReturnPressed();
    void handleKeepPreviousValuesActionTriggered(QAction *action);
    void applySettingsFromDialog();
    void addTag(const std::function<Media::Tag *(Media::MediaFileInfo &)> &createTag);
    void removeTag(Media::Tag *tag);
    void changeTarget(Media::Tag *tag);

    // saving
    bool startSaving();
    void showSavingResult(bool sucess);

    // misc
    void showAboutDlg();
    void showSettingsDlg();
    void showRenameFilesDlg();
    void updateInfoWebView();

private:
    void updateTagEditsAndAttachmentEdits(bool updateUi = true, PreviousValueHandling previousValueHandling = PreviousValueHandling::Auto);
    void updateTagSelectionComboBox();
    void updateUiStatus();
    void updateTagManagementMenu();
    void insertTitleFromFilename();
    void foreachTagEdit(const std::function<void (TagEdit *)> &function);
    bool confirmCreationOfId3TagForUnsupportedFile();

    // UI
    std::unique_ptr<Ui::MainWindow> m_ui;
    QMenu *m_keepPreviousValuesMenu;
    QMenu *m_tagOptionsMenu;
    QMenu *m_addTagMenu;
    QMenu *m_removeTagMenu;
    QMenu *m_changeTargetMenu;
    // models
    QFileSystemModel *m_fileModel;
    FileFilterProxyModel *m_fileFilterModel;
    // tag, file, directory management
    QString m_currentPath;
    QString m_currentDir; // this is the actual direcotry of the opened file and may differ from the directory selected in the tree view
    QString m_lastDir;
    Media::MediaFileInfo m_fileInfo;
    QString m_nextFilePath;
    std::vector<Media::Tag *> m_tags;
    QByteArray m_fileInfoHtml;
    // dialogs
    Dialogs::AboutDialog *m_aboutDlg;
    Dialogs::SettingsDialog *m_settingsDlg;
    std::unique_ptr<RenameFilesDialog> m_renameFilesDlg;
    // status
    bool m_makingResultsAvailable;
    Media::NotificationList m_originalNotifications;
    bool m_abortClicked;
    std::mutex m_fileOperationMutex;
};

}

#endif // TAGEDITORDIALOG_H
