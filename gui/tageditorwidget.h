#ifndef TAGEDITORWIDGET_H
#define TAGEDITORWIDGET_H

#include "./previousvaluehandling.h"

#include <tagparser/mediafileinfo.h>

#include <QWidget>
#include <QByteArray>
#include <QMutex>

#include <functional>

#if defined(TAGEDITOR_NO_WEBVIEW)
#elif defined(TAGEDITOR_USE_WEBENGINE)
# define WEB_VIEW_PROVIDER QWebEngineView
#elif defined(TAGEDITOR_USE_WEBKIT)
# define WEB_VIEW_PROVIDER QWebView
#else
# error "Macro for WebView provider not specified."
#endif

QT_FORWARD_DECLARE_CLASS(QFileSystemWatcher)
QT_FORWARD_DECLARE_CLASS(QMenu)
QT_FORWARD_DECLARE_CLASS(QTreeView)
#ifndef TAGEDITOR_NO_WEBVIEW
QT_FORWARD_DECLARE_CLASS(WEB_VIEW_PROVIDER)
#endif

namespace Media {
DECLARE_ENUM_CLASS(TagType, unsigned int);
}

namespace QtGui {

namespace Ui {
class TagEditorWidget;
}

class TagEdit;
class FileInfoModel;

class TagEditorWidget : public QWidget
{
    Q_OBJECT
    Q_PROPERTY(QString currentPath READ currentPath NOTIFY currentPathChanged)
    Q_PROPERTY(QByteArray fileInfoHtml READ fileInfoHtml)
    Q_PROPERTY(bool fileNameVisible READ isFileNameVisible WRITE setFileNameVisible)
    Q_PROPERTY(bool buttonsVisible READ areButtonsVisible WRITE setButtonVisible)
    
public:
    explicit TagEditorWidget(QWidget *parent = nullptr);
    ~TagEditorWidget();

public:
    QMutex &fileOperationMutex();
    const QString &currentPath() const;
    Media::MediaFileInfo &fileInfo();
    Media::NotificationList &originalNotifications();
    bool isTagEditShown() const;
    const QByteArray &fileInfoHtml() const;
    bool isFileNameVisible() const;
    void setFileNameVisible(bool visible);
    bool areButtonsVisible() const;
    void setButtonVisible(bool visible);
    void foreachTagEdit(const std::function<void (TagEdit *)> &function);
    TagEdit *activeTagEdit();

public slots:
    // operations with the currently opened file: load, save, delete, close
    bool startParsing(const QString &path, bool forceRefresh = false);
    bool startSaving();
    void saveAndShowNextFile();
    bool reparseFile();
    bool applyEntriesAndSaveChangings();
    bool deleteAllTagsAndSave();
    void closeFile();
    // misc
    void applySettingsFromDialog();

signals:
    /// \brief Emitted when loading the next file has been triggered.
    void nextFileSelected();
    /// \brief Emitted to show a status message.
    void statusMessage(const QString &message, int timeout = 0);
    /// \brief Emmited when the file status (opened/closed) has changed.
    void fileStatusChanged(bool opened, bool hasTag);
    /// \brief Emitted when the current path has changed; always emitted a saving.
    void currentPathChanged(const QString &newPath);

protected:
    bool event(QEvent *event);

private slots:
    // editor
    void fileChangedOnDisk(const QString &path);
    void showFile(char result);
    void handleReturnPressed();
    void handleKeepPreviousValuesActionTriggered(QAction *action);
    void addTag(const std::function<Media::Tag *(Media::MediaFileInfo &)> &createTag);
    void removeTag(Media::Tag *tag);
    void changeTarget(Media::Tag *tag);

    // saving
    void showSavingResult(bool processingError, bool ioError);

    // info (web) view
    void initInfoView();
    void updateInfoView();
    void showInfoTreeViewContextMenu(const QPoint &);
#ifndef TAGEDITOR_NO_WEBVIEW
    void showInfoWebViewContextMenu(const QPoint &);
#endif

private:
    void updateDocumentTitleEdits();
    void updateTagEditsAndAttachmentEdits(bool updateUi = true, PreviousValueHandling previousValueHandling = PreviousValueHandling::Auto);
    void updateTagSelectionComboBox();
    void updateFileStatusStatus();
    void updateTagManagementMenu();
    void insertTitleFromFilename();
    bool confirmCreationOfId3TagForUnsupportedFile();

    // UI
    std::unique_ptr<Ui::TagEditorWidget> m_ui;
    QMenu *m_keepPreviousValuesMenu;
    QMenu *m_tagOptionsMenu;
    QMenu *m_addTagMenu;
    QMenu *m_removeTagMenu;
    QMenu *m_changeTargetMenu;
#ifndef TAGEDITOR_NO_WEBVIEW
    WEB_VIEW_PROVIDER *m_infoWebView;
#endif
    FileInfoModel *m_infoModel;
    QTreeView *m_infoTreeView;
    // tag, file, directory management
    QString m_currentPath;
    QFileSystemWatcher *m_fileWatcher;
    bool m_fileChangedOnDisk;
    Media::MediaFileInfo m_fileInfo;
    std::vector<Media::Tag *> m_tags;
    QByteArray m_fileInfoHtml;
    /// \brief This is the actual direcotry of the opened file which may differ from the directory selected in the tree view of the main window.
    QString m_currentDir;
    QString m_lastDir;
    QString m_saveFilePath;
    // status
    bool m_nextFileAfterSaving;
    bool m_makingResultsAvailable;
    Media::NotificationList m_originalNotifications;
    bool m_abortClicked;
    QMutex m_fileOperationMutex;
};

/*!
 * \brief Returns the mutex which is internally used for thread-synchronization.
 */
inline QMutex &TagEditorWidget::fileOperationMutex()
{
    return m_fileOperationMutex;
}

/*!
 * \brief Returns the current path.
 */
inline const QString &TagEditorWidget::currentPath() const
{
    return m_currentPath;
}

/*!
 * \brief Return file info.
 */
inline Media::MediaFileInfo &TagEditorWidget::fileInfo()
{
    return m_fileInfo;
}

/*!
 * \brief Returns the original notifications.
 */
inline Media::NotificationList &TagEditorWidget::originalNotifications()
{
    return m_originalNotifications;
}

/*!
 * \brief Returns the HTML source of the info website.
 */
inline const QByteArray &TagEditorWidget::fileInfoHtml() const
{
    return m_fileInfoHtml;
}

/*!
 * \brief Returns whether currently a tag edit is shown.
 */
inline bool TagEditorWidget::isTagEditShown() const
{
    return !m_tags.empty();
}

}

#endif // TAGEDITORWIDGET_H
