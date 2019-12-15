#ifndef RENAMINGUTILITY_RENAMINGENGINE_H
#define RENAMINGUTILITY_RENAMINGENGINE_H

#include "./filesystemitem.h"
#include "./jsdefs.h"
#include "./jsincludes.h"

#include <QAtomicInteger>
#include <QDir>
#include <QList>
#include <QObject>
#include <QThread>

#include <memory>

QT_FORWARD_DECLARE_CLASS(QFileInfo)

namespace RenamingUtility {

class FileSystemItemModel;
class FilteredFileSystemItemModel;
class TagEditorObject;
class RenamingEngine;

class PreviewGenerator final : public QThread {
    Q_OBJECT
public:
    explicit PreviewGenerator(RenamingEngine *engine);

protected:
    void run() final;

private:
    RenamingEngine *m_engine;
};

class RenamingThing final : public QThread {
    Q_OBJECT
public:
    explicit RenamingThing(RenamingEngine *engine);

protected:
    void run() final;

private:
    RenamingEngine *m_engine;
};

class RenamingEngine : public QObject {
    Q_OBJECT

    friend class PreviewGenerator;
    friend class RenamingThing;

public:
    explicit RenamingEngine(QObject *parent = nullptr);

    FileSystemItem *rootItem() const;
#ifndef TAGEDITOR_NO_JSENGINE
    const TAGEDITOR_JS_VALUE &scriptProgram() const;
    bool setProgram(const TAGEDITOR_JS_VALUE &program);
#endif
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
    void resetStatus();
    void finalizeTaskCompletion();
    void setRootItem(std::unique_ptr<FileSystemItem> &&rootItem = std::unique_ptr<FileSystemItem>());
    void updateModel(FileSystemItem *rootItem);
#ifndef TAGEDITOR_NO_JSENGINE
    std::unique_ptr<FileSystemItem> generatePreview(const QDir &dir, FileSystemItem *parent = nullptr);
#endif
    void applyChangings(FileSystemItem *parentItem);
    static void setError(const QList<FileSystemItem *> items);
#ifndef TAGEDITOR_NO_JSENGINE
    void executeScriptForItem(const QFileInfo &fileInfo, FileSystemItem *item);
#endif

#ifndef TAGEDITOR_NO_JSENGINE
    TagEditorObject *m_tagEditorQObj;
    TAGEDITOR_JS_ENGINE m_engine;
    TAGEDITOR_JS_VALUE m_tagEditorJsObj;
#endif
    std::unique_ptr<FileSystemItem> m_rootItem;
    std::unique_ptr<FileSystemItem> m_newlyGeneratedRootItem;
    int m_itemsProcessed;
    int m_errorsOccured;
    QAtomicInteger<unsigned char> m_aborted;
#ifndef TAGEDITOR_NO_JSENGINE
    TAGEDITOR_JS_VALUE m_program;
#endif
    QDir m_dir;
    bool m_includeSubdirs;
    bool m_isBusy;
    FileSystemItemModel *m_model;
    FilteredFileSystemItemModel *m_currentModel;
    FilteredFileSystemItemModel *m_previewModel;
    QString m_errorMessage;
    int m_errorLineNumber;
};

inline FileSystemItem *RenamingEngine::rootItem() const
{
    return m_rootItem.get();
}

#ifndef TAGEDITOR_NO_JSENGINE
inline const TAGEDITOR_JS_VALUE &RenamingEngine::scriptProgram() const
{
    return m_program;
}
#endif

inline const QDir &RenamingEngine::rootDirectory() const
{
    return m_dir;
}

inline bool RenamingEngine::subdirsIncluded() const
{
    return m_includeSubdirs;
}

inline const QString &RenamingEngine::errorMessage() const
{
    return m_errorMessage;
}

inline int RenamingEngine::errorLineNumber() const
{
    return m_errorLineNumber;
}

inline bool RenamingEngine::isBusy()
{
    return m_isBusy;
}

inline void RenamingEngine::abort()
{
#if (QT_VERSION >= QT_VERSION_CHECK(5, 14, 0))
    m_aborted.storeRelaxed(1);
#else
    m_aborted.store(1);
#endif
}

inline bool RenamingEngine::isAborted()
{
#if (QT_VERSION >= QT_VERSION_CHECK(5, 14, 0))
    return m_aborted.loadRelaxed();
#else
    return m_aborted.load();
#endif
}

} // namespace RenamingUtility

#endif // RENAMINGUTILITY_RENAMINGENGINE_H
