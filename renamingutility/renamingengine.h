#ifndef RENAMINGUTILITY_RENAMINGENGINE_H
#define RENAMINGUTILITY_RENAMINGENGINE_H

#include "./filesystemitem.h"
#include "./scriptdefs.h"

#include <QObject>
#include <QList>
#include <QDir>

#if TAGEDITOR_USE_JSENGINE
# include <QJSEngine>
# include <QJSValue>
#else
# include <QScriptEngine>
# include <QScriptValue>
#endif

#include <memory>
#include <mutex>
#include <atomic>

QT_FORWARD_DECLARE_CLASS(QFileInfo)

namespace RenamingUtility {

class FileSystemItemModel;
class FilteredFileSystemItemModel;
class TagEditorObject;

class RemamingEngine : public QObject
{
    Q_OBJECT

public:
    RemamingEngine(QObject *parent = nullptr);

    FileSystemItem *rootItem() const;
    const TAGEDITOR_JS_VALUE &scriptProgram() const;
    bool setProgram(const TAGEDITOR_JS_VALUE &program);
    bool setProgram(const QString &program);
    const QDir &rootDirectory() const;
    bool subdirsIncluded() const;
    bool isBusy();
    bool isAborted();
    bool clearPreview();
    FileSystemItemModel *model();
    FilteredFileSystemItemModel *currentModel();
    FilteredFileSystemItemModel *previewModel();
    const QString &errorMessage() const;
    int errorLineNumber() const;

public slots:
    bool generatePreview(const QDir &rootDirectory, bool includeSubdirs);
    bool applyChangings();
    void abort();

signals:
    void previewGenerated();
    void changingsApplied();
    void progress(int itemsProcessed, int errorsOccured);

private slots:
    void processPreviewGenerated();
    void processChangingsApplied();

private:
    void setRootItem(std::unique_ptr<FileSystemItem> &&rootItem = std::unique_ptr<FileSystemItem>());
    void updateModel(FileSystemItem *rootItem);
    std::unique_ptr<FileSystemItem> generatePreview(const QDir &dir, FileSystemItem *parent = nullptr);
    void applyChangings(FileSystemItem *parentItem);
    static void setError(const QList<FileSystemItem *> items);
    void executeScriptForItem(const QFileInfo &fileInfo, FileSystemItem *item);

    TagEditorObject *m_tagEditorQObj;
    TAGEDITOR_JS_ENGINE m_engine;
    TAGEDITOR_JS_VALUE m_tagEditorJsObj;
    std::unique_ptr<FileSystemItem> m_rootItem;
    std::unique_ptr<FileSystemItem> m_newlyGeneratedRootItem;
    int m_itemsProcessed;
    int m_errorsOccured;
    std::atomic<bool> m_aborted;
    TAGEDITOR_JS_VALUE m_program;
    QDir m_dir;
    bool m_includeSubdirs;
    std::mutex m_mutex;
    FileSystemItemModel *m_model;
    FilteredFileSystemItemModel *m_currentModel;
    FilteredFileSystemItemModel *m_previewModel;
    QString m_errorMessage;
    int m_errorLineNumber;
};

inline FileSystemItem *RemamingEngine::rootItem() const
{
    return m_rootItem.get();
}

inline const TAGEDITOR_JS_VALUE &RemamingEngine::scriptProgram() const
{
    return m_program;
}

inline const QDir &RemamingEngine::rootDirectory() const
{
    return m_dir;
}

inline bool RemamingEngine::subdirsIncluded() const
{
    return m_includeSubdirs;
}

inline const QString &RemamingEngine::errorMessage() const
{
    return m_errorMessage;
}

inline int RemamingEngine::errorLineNumber() const
{
    return m_errorLineNumber;
}

} // namespace RenamingUtility

#endif // RENAMINGUTILITY_RENAMINGENGINE_H
