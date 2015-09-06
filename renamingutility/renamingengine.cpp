#include "./renamingengine.h"
#include "./filesystemitem.h"
#include "./filesystemitemmodel.h"
#include "./filteredfilesystemitemmodel.h"
#include "./scriptfunctions.h"

#include <QDir>
#include <QScriptEngine>
#include <QScriptProgram>

#include <thread>

using namespace std;

namespace RenamingUtility {

/*
    TRANSLATOR RenamingUtility::RemamingEngine
    Necessary for lupdate.
*/

RemamingEngine::RemamingEngine(QObject *parent) :
    QObject(parent),
    m_itemsProcessed(0),
    m_errorsOccured(0),
    m_aborted(false),
    m_includeSubdirs(false),
    m_model(nullptr),
    m_currentModel(nullptr),
    m_previewModel(nullptr)
{
    connect(this, &RemamingEngine::previewGenerated, this, &RemamingEngine::processPreviewGenerated);
    connect(this, &RemamingEngine::changingsApplied, this, &RemamingEngine::processChangingsApplied);
}

RemamingEngine::~RemamingEngine()
{}

bool RemamingEngine::generatePreview(const QScriptProgram &scriptProgram, const QDir &rootDirectory, bool includeSubdirs)
{
    if(!m_mutex.try_lock()) {
        return false;
    }
    setRootItem(nullptr);
    m_program = scriptProgram;
    m_includeSubdirs = includeSubdirs;
    m_dir = rootDirectory;
    auto startFunc = [this] () {
        {
            lock_guard<mutex> guard(m_mutex);
            {
                lock_guard<mutex> guard(m_abortedMutex);
                m_aborted = false;
            }
            m_itemsProcessed = 0;
            m_errorsOccured = 0;
            m_newlyGeneratedRootItem.reset(generatePreview(m_dir));
        }
        emit previewGenerated();
    };
    std::thread thread(startFunc);
    thread.detach();
    m_mutex.unlock();
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
    auto startFunc = [this] () {
        {
            lock_guard<mutex> guard(m_mutex);
            {
                lock_guard<mutex> guard(m_abortedMutex);
                m_aborted = false;
            }
            m_itemsProcessed = 0;
            m_errorsOccured = 0;
            applyChangings(m_rootItem.get());
        }
        emit changingsApplied();
    };
    std::thread thread(startFunc);
    thread.detach();
    m_mutex.unlock();
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
    lock_guard<mutex> guard(m_abortedMutex);
    m_aborted = true;
}

bool RemamingEngine::isAborted()
{
    lock_guard<mutex> guard(m_abortedMutex);
    return m_aborted;
}

bool RemamingEngine::clearPreview()
{
    if(m_mutex.try_lock()) {
        updateModel(nullptr);
        m_rootItem.reset();
        m_mutex.unlock();
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
    setRootItem(m_newlyGeneratedRootItem.release());
}

void RemamingEngine::processChangingsApplied()
{
    updateModel(nullptr);
    updateModel(m_rootItem.get());
}

inline void RemamingEngine::setRootItem(FileSystemItem *rootItem)
{
    updateModel(rootItem);
    m_rootItem.reset(rootItem);
}

void RemamingEngine::updateModel(FileSystemItem *rootItem)
{
    if(m_model) {
        m_model->setRootItem(rootItem);
    }
}

FileSystemItem *RemamingEngine::generatePreview(const QDir &dir, FileSystemItem *parent)
{
    FileSystemItem *item = new FileSystemItem(ItemStatus::Current, ItemType::Dir, dir.dirName());
    item->setParent(parent);
    item->setApplied(false);
    QFileInfoList entries = dir.entryInfoList();
    foreach(const QFileInfo &entry, entries) {
        if(entry.fileName() == QLatin1String("..")
                || entry.fileName() == QLatin1String(".")) {
            continue;
        }
        FileSystemItem *subItem;
        if(entry.isDir() && m_includeSubdirs) {
            subItem = generatePreview(QDir(entry.absoluteFilePath()), item);
        } else if(entry.isFile()) {
            subItem = new FileSystemItem(ItemStatus::Current, ItemType::File, entry.fileName());
            subItem->setParent(item);
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
    foreach(FileSystemItem *item, items) {
        item->setErrorOccured(true);
        item->setNote(tr("skipped due to error of superior item"));
    }
}

void RemamingEngine::executeScriptForItem(const QFileInfo &fileInfo, FileSystemItem *item)
{
    QScriptEngine engine;
    setupGlobalObject(engine, fileInfo, item);
    QScriptValue res = engine.evaluate(m_program);

    if(engine.hasUncaughtException()) {
        item->setErrorOccured(true);
        item->setNote(res.toString());
    } else {
        QScriptValue go = engine.globalObject();
        QScriptValue newName = go.property("newName");
        QScriptValue newRelativeDirectory = go.property("newRelativeDirectory");
        ActionType action = ActionType::Skip;
        if(engine.globalObject().property("action").isNumber()) {
            action = static_cast<ActionType>(engine.globalObject().property("action").toInt32());
        }
        switch(action) {
        case ActionType::Rename:
            if(newRelativeDirectory.isString()) {
                FileSystemItem *counterpartParent = item->root()->makeChildAvailable(newRelativeDirectory.toString());
                if(counterpartParent->status() == ItemStatus::New
                        && counterpartParent->note().isEmpty()) {
                    counterpartParent->setNote(tr("will be created"));
                }
                QString counterpartName = newName.isString()
                        ? newName.toString()
                        : item->name();
                if(counterpartParent->findChild(counterpartName, item)) {
                    item->setNote(tr("name is already used at new location"));
                    item->setErrorOccured(true);
                } else {
                    FileSystemItem *counterpart = new FileSystemItem(ItemStatus::New, item->type(), counterpartName);
                    counterpart->setParent(counterpartParent);
                    item->setCounterpart(counterpart);
                    counterpart->setCheckable(true);
                    counterpart->setChecked(true);
                }
            } else if(newName.isString()) {
                item->setNewName(newName.toString());
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
            break;
        }
    }
}

void RemamingEngine::setupGlobalObject(QScriptEngine &engine, const QFileInfo &file, FileSystemItem *item)
{
    QScriptValue go = engine.globalObject();
    go.setProperty("currentPath", file.absoluteFilePath(), QScriptValue::ReadOnly);
    go.setProperty("currentName", item->name(), QScriptValue::ReadOnly);
    go.setProperty("currentRelativeDirectory", item->relativeDir(), QScriptValue::ReadOnly);
    go.setProperty("isDir", item->type() == ItemType::Dir, QScriptValue::ReadOnly);
    go.setProperty("isFile", item->type() == ItemType::File, QScriptValue::ReadOnly);
    go.setProperty("action", QScriptValue(static_cast<int>(ActionType::Rename)), QScriptValue::Undeletable);
    go.setProperty("parseFileInfo", engine.newFunction(ScriptFunctions::parseFileInfo), QScriptValue::ReadOnly);
    go.setProperty("parseFileName", engine.newFunction(ScriptFunctions::parseFileName), QScriptValue::ReadOnly);
    go.setProperty("allFiles", engine.newFunction(ScriptFunctions::allFiles), QScriptValue::ReadOnly);
    go.setProperty("firstFile", engine.newFunction(ScriptFunctions::firstFile), QScriptValue::ReadOnly);
    go.setProperty("writeLog", engine.newFunction(ScriptFunctions::writeLog), QScriptValue::ReadOnly);
    QScriptValue actionObject = engine.newObject();
    actionObject.setProperty("rename", QScriptValue(static_cast<int>(ActionType::Rename)), QScriptValue::ReadOnly);
    actionObject.setProperty("skip", QScriptValue(static_cast<int>(ActionType::Skip)), QScriptValue::ReadOnly);
    go.setProperty("actionType", actionObject, QScriptValue::ReadOnly);
}

} // namespace RenamingUtility
