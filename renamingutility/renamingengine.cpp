#include "./renamingengine.h"
#include "./filesystemitemmodel.h"
#include "./filteredfilesystemitemmodel.h"
#include "./tageditorobject.h"

#include <c++utilities/misc/memory.h>

#include <QDir>
#include <QStringBuilder>

#include <thread>

using namespace std;

namespace RenamingUtility {

RemamingEngine::RemamingEngine(QObject *parent) :
    QObject(parent),
    m_tagEditorQObj(new TagEditorObject(&m_engine)),
    m_tagEditorJsObj(TAGEDITOR_JS_QOBJECT(m_engine, m_tagEditorQObj)),
    m_itemsProcessed(0),
    m_errorsOccured(0),
    m_aborted(false),
    m_includeSubdirs(false),
    m_model(nullptr),
    m_currentModel(nullptr),
    m_previewModel(nullptr)
{
    m_engine.globalObject().setProperty(QStringLiteral("tageditor"), m_tagEditorJsObj);
    connect(this, &RemamingEngine::previewGenerated, this, &RemamingEngine::processPreviewGenerated);
    connect(this, &RemamingEngine::changingsApplied, this, &RemamingEngine::processChangingsApplied);
}

bool RemamingEngine::setProgram(const TAGEDITOR_JS_VALUE &program)
{
    if(TAGEDITOR_JS_IS_VALID_PROG(program)) {
        m_errorMessage.clear();
        m_errorLineNumber = 0;
        m_program = program;
        return true;
    } else if(program.isError()) {
        m_errorMessage = program.property(QStringLiteral("message")).toString();
        m_errorLineNumber = TAGEDITOR_JS_INT(program.property(QStringLiteral("lineNumber")));
    } else {
        m_errorMessage = tr("Program is not callable.");
        m_errorLineNumber = 0;
    }
    return false;
}

bool RemamingEngine::setProgram(const QString &program)
{
    return setProgram(m_engine.evaluate(QStringLiteral("(function(){") % program % QStringLiteral("})")));
}

bool RemamingEngine::generatePreview(const QDir &rootDirectory, bool includeSubdirs)
{
    if(!m_mutex.try_lock()) {
        return false;
    }
    lock_guard<mutex> guard(m_mutex, adopt_lock);
    setRootItem();
    m_includeSubdirs = includeSubdirs;
    m_dir = rootDirectory;
    auto startFunc = [this] () {
        {
            lock_guard<mutex> guard(m_mutex);
            m_aborted.store(false);
            m_itemsProcessed = 0;
            m_errorsOccured = 0;
            m_newlyGeneratedRootItem = generatePreview(m_dir);
        }
        emit previewGenerated();
    };
    std::thread thread(startFunc);
    thread.detach();
    return true;
}

bool RemamingEngine::applyChangings()
{
    if(!m_rootItem) {
        return false;
    }
    if(!m_mutex.try_lock()) {
        return false;
    }
    lock_guard<mutex> guard(m_mutex, adopt_lock);
    auto startFunc = [this] () {
        {
            lock_guard<mutex> guard(m_mutex);
            m_aborted.store(false);
            m_itemsProcessed = 0;
            m_errorsOccured = 0;
            applyChangings(m_rootItem.get());
        }
        emit changingsApplied();
    };
    std::thread thread(startFunc);
    thread.detach();
    return true;
}

bool RemamingEngine::isBusy()
{
    if(m_mutex.try_lock()) {
        m_mutex.unlock();
        return false;
    } else {
        return true;
    }
}

void RemamingEngine::abort()
{
    m_aborted.store(true);
}

bool RemamingEngine::isAborted()
{
    return m_aborted.load();
}

bool RemamingEngine::clearPreview()
{
    if(m_mutex.try_lock()) {
        lock_guard<mutex> guard(m_mutex, adopt_lock);
        updateModel(nullptr);
        m_rootItem.reset();
        return true;
    } else {
        return false;
    }
}

FileSystemItemModel *RemamingEngine::model()
{
    if(!m_model) {
        m_model = new FileSystemItemModel(m_rootItem.get(), this);
    }
    return m_model;
}

FilteredFileSystemItemModel *RemamingEngine::currentModel()
{
    if(!m_currentModel) {
        m_currentModel = new FilteredFileSystemItemModel(ItemStatus::Current, this);
        m_currentModel->setSourceModel(model());
    }
    return m_currentModel;
}

FilteredFileSystemItemModel *RemamingEngine::previewModel()
{
    if(!m_previewModel) {
        m_previewModel = new FilteredFileSystemItemModel(ItemStatus::New, this);
        m_previewModel->setSourceModel(model());
    }
    return m_previewModel;
}

void RemamingEngine::processPreviewGenerated()
{
    setRootItem(move(m_newlyGeneratedRootItem));
}

void RemamingEngine::processChangingsApplied()
{
    updateModel(nullptr);
    updateModel(m_rootItem.get());
}

inline void RemamingEngine::setRootItem(unique_ptr<FileSystemItem> &&rootItem)
{
    updateModel(rootItem.get());
    m_rootItem = move(rootItem);
}

void RemamingEngine::updateModel(FileSystemItem *rootItem)
{
    if(m_model) {
        m_model->setRootItem(rootItem);
    }
}

unique_ptr<FileSystemItem> RemamingEngine::generatePreview(const QDir &dir, FileSystemItem *parent)
{
    auto item = make_unique<FileSystemItem>(ItemStatus::Current, ItemType::Dir, dir.dirName(), parent);
    item->setApplied(false);
    for(const QFileInfo &entry : dir.entryInfoList()) {
        if(entry.fileName() == QLatin1String("..")
                || entry.fileName() == QLatin1String(".")) {
            continue;
        }
        FileSystemItem *subItem; // will be deleted by parent
        if(entry.isDir() && m_includeSubdirs) {
            subItem = generatePreview(QDir(entry.absoluteFilePath()), item.get()).release();
        } else if(entry.isFile()) {
            subItem = new FileSystemItem(ItemStatus::Current, ItemType::File, entry.fileName(), item.get());
            subItem->setApplied(false);
        } else {
            subItem = nullptr;
        }
        if(subItem) {
            executeScriptForItem(entry, subItem);
            if(subItem->errorOccured()) {
                ++m_errorsOccured;
            }
        }
        ++m_itemsProcessed;
        if(isAborted()) {
            return item;
        }
    }
    emit progress(m_itemsProcessed, m_errorsOccured);
    return item;
}

void RemamingEngine::applyChangings(FileSystemItem *parentItem)
{
    foreach(FileSystemItem *item, parentItem->children()) {
        if(!item->applied() && !item->errorOccured()) {
            switch(item->status()) {
            case ItemStatus::New: {
                FileSystemItem *counterpartItem = item->counterpart(); // holds current name
                QString currentPath = counterpartItem ? counterpartItem->relativePath() : QString();
                QString newPath = item->relativePath();
                if(item->name().isEmpty()) {
                    // new item name mustn't be empty
                    item->setNote(tr("generated name is empty"));
                    item->setErrorOccured(true);
                } else if(counterpartItem && !counterpartItem->name().isEmpty()) {
                    // rename current item
                    if(item->parent() != counterpartItem->parent() || item->name() != counterpartItem->name()) {
                        if(m_dir.exists(newPath)) {
                            if(item->parent() == counterpartItem->parent()) {
                                item->setNote(tr("unable to rename, there is already an entry with the same name"));
                            } else {
                                item->setNote(tr("unable to move, there is already an entry with the same name"));
                            }
                            item->setErrorOccured(true);
                        } if(m_dir.rename(currentPath, newPath)) {
                            if(item->parent() == counterpartItem->parent()) {
                                item->setNote(tr("renamed"));
                            } else {
                                item->setNote(tr("moved"));
                            }
                            item->setApplied(true);
                        } else {
                            item->setNote(tr("unable to rename"));
                            item->setErrorOccured(true);
                        }
                    } else {
                        item->setNote(tr("nothing to be changed"));
                        item->setApplied(true);
                    }
                } else if(item->type() == ItemType::Dir) {
                    // create new item, but only if its a dir
                    if(m_dir.exists(newPath)) {
                        item->setNote(tr("directory already existed"));
                        item->setApplied(true);
                    } else if(m_dir.mkpath(newPath)) {
                        item->setNote(tr("directory created"));
                        item->setApplied(true);
                    } else {
                        item->setNote(tr("unable to create directory"));
                        item->setErrorOccured(true);
                    }
                } else {
                    // can not create new file
                    item->setNote(tr("unable to create file"));
                    item->setErrorOccured(true);
                }
                break;
            } case ItemStatus::Current:
                break;
            default:
                ;
            }
        }
        if(item->errorOccured()) {
            ++m_errorsOccured;
        }
        // apply changings for child items as well
        if(item->type() == ItemType::Dir) {
            applyChangings(item);
        }
    }
    m_itemsProcessed += parentItem->children().size();
    emit progress(m_itemsProcessed, m_errorsOccured);
}

void RemamingEngine::setError(const QList<FileSystemItem *> items)
{
    for(FileSystemItem *item : items) {
        item->setErrorOccured(true);
        item->setNote(tr("skipped due to error of superior item"));
    }
}

void RemamingEngine::executeScriptForItem(const QFileInfo &fileInfo, FileSystemItem *item)
{
    // make file info for the specified item available in the script
    m_tagEditorQObj->setFileInfo(fileInfo, item);
    // execute script
    auto scriptResult = m_program.call();
    if(scriptResult.isError()) {
        // handle error
        item->setErrorOccured(true);
        item->setNote(scriptResult.toString());
    } else {
        // create preview for action
        const QString &newName = m_tagEditorQObj->newName();
        const QString &newRelativeDirectory = m_tagEditorQObj->newRelativeDirectory();
        switch(m_tagEditorQObj->action()) {
        case ActionType::None:
            item->setNote(tr("no action specified"));
            break;
        case ActionType::Rename:
            if(!newRelativeDirectory.isEmpty()) {
                FileSystemItem *counterpartParent = item->root()->makeChildAvailable(newRelativeDirectory);
                if(counterpartParent->status() == ItemStatus::New
                        && counterpartParent->note().isEmpty()) {
                    counterpartParent->setNote(tr("will be created"));
                }
                const QString &counterpartName = newName.isEmpty() ? item->name() : newName;
                if(counterpartParent->findChild(counterpartName, item)) {
                    item->setNote(tr("name is already used at new location"));
                    item->setErrorOccured(true);
                } else {
                    auto *counterpart = new FileSystemItem(ItemStatus::New, item->type(), counterpartName, counterpartParent);
                    item->setCounterpart(counterpart);
                    counterpart->setCheckable(true);
                    counterpart->setChecked(true);
                }
            } else if(!newName.isEmpty()) {
                item->setNewName(newName);
            }
            if(FileSystemItem *newItem = item->counterpart()) {
                if((newItem->name().isEmpty() || newItem->name() == item->name())
                    && (newItem->parent() == item->parent())) {
                    item->setNote(tr("name doesn't change"));
                } else if(newItem->parent() && newItem->parent()->findChild(newItem->name(), newItem)) {
                    item->setNote(tr("generated name is already used"));
                    item->setErrorOccured(true);
                } else if(newItem->parent() == item->parent()) {
                    item->setNote(tr("will be renamed"));
                    newItem->setCheckable(true);
                    newItem->setChecked(true);
                } else {
                    item->setNote(tr("will be moved"));
                }
            } else {
                item->setNote(tr("can not be renamed"));
            }
            break;
        default:
            item->setNote(tr("skipped"));
        }
    }
}

} // namespace RenamingUtility
