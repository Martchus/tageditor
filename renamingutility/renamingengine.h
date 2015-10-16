#ifndef RENAMINGUTILITY_RENAMINGENGINE_H
#define RENAMINGUTILITY_RENAMINGENGINE_H

#include "./filesystemitem.h"

#include <QObject>
#include <QList>
#include <QDir>
#include <QScriptProgram>
#include <QScriptEngine>
#include <QScriptValue>

#include <memory>
#include <mutex>
#include <atomic>

QT_FORWARD_DECLARE_CLASS(QFileInfo)
QT_FORWARD_DECLARE_CLASS(QScriptProgram)
QT_FORWARD_DECLARE_CLASS(QScriptContext)

namespace RenamingUtility {

class FileSystemItemModel;
class FilteredFileSystemItemModel;

class RemamingEngine : public QObject
{
    Q_OBJECT

public:
    RemamingEngine(QObject *parent = nullptr);
    virtual ~RemamingEngine();

    FileSystemItem *rootItem() const;
    const QScriptProgram &scriptProgram() const;
    const QDir &rootDirectory() const;
    bool subdirsIncluded() const;
    bool isBusy();
    bool isAborted();
    bool clearPreview();
    FileSystemItemModel *model();
    FilteredFileSystemItemModel *currentModel();
    FilteredFileSystemItemModel *previewModel();

public slots:
    bool generatePreview(const QScriptProgram &scriptProgram, const QDir &rootDirectory, bool includeSubdirs);
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
    void setupGlobalObject(const QFileInfo &file, FileSystemItem *item);

    QScriptEngine m_engine;
    QScriptValue m_go;
    QScriptValue m_persistent;
    std::unique_ptr<FileSystemItem> m_rootItem;
    std::unique_ptr<FileSystemItem> m_newlyGeneratedRootItem;
    int m_itemsProcessed;
    int m_errorsOccured;
    std::atomic<bool> m_aborted;
    QScriptProgram m_program;
    QDir m_dir;
    bool m_includeSubdirs;
    std::mutex m_mutex;
    FileSystemItemModel *m_model;
    FilteredFileSystemItemModel *m_currentModel;
    FilteredFileSystemItemModel *m_previewModel;
};

inline FileSystemItem *RemamingEngine::rootItem() const
{
    return m_rootItem.get();
}

inline const QScriptProgram &RemamingEngine::scriptProgram() const
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

} // namespace RenamingUtility

#endif // RENAMINGUTILITY_RENAMINGENGINE_H
