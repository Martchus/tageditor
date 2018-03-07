#include "./filesystemitem.h"

#include <QCoreApplication>
#include <QDir>

namespace RenamingUtility {

/*
    TRANSLATOR RenamingUtility::FileSystemItem
    Necessary for lupdate.
*/

const QString &emptyStr()
{
    static QString emptyStr;
    return emptyStr;
}

FileSystemItem::FileSystemItem(ItemStatus status, ItemType type, const QString &name, FileSystemItem *parent)
    : m_parent(parent)
    , m_counterpart(nullptr)
    , m_status(status)
    , m_type(type)
    , m_error(false)
    , m_applied(false)
    , m_name(name)
    , m_checked(false)
    , m_checkable(false)
{
    if (m_parent) {
        m_parent->m_children << this;
    }
}

FileSystemItem::~FileSystemItem()
{
    for (FileSystemItem *child : m_children) {
        child->m_parent = nullptr;
        delete child;
    }
    if (m_parent) {
        m_parent->m_children.removeAll(this);
    }
}

void FileSystemItem::setParent(FileSystemItem *parent)
{
    if (parent != m_parent) {
        if (m_parent) {
            m_parent->m_children.removeAll(this);
        }
        m_parent = parent;
        if (m_parent && !m_parent->m_children.contains(this)) {
            m_parent->m_children << this;
        }
    }
}

const QString &FileSystemItem::currentName() const
{
    switch (m_status) {
    case ItemStatus::New:
        return m_counterpart ? m_counterpart->name() : emptyStr();
    case ItemStatus::Current:
        return m_name;
    }
    return emptyStr();
}

bool FileSystemItem::setCurrentName(const QString &currentName)
{
    switch (m_status) {
    case ItemStatus::New:
        if (!m_counterpart) {
            // creating an imaginary current file doesn't make sense here
            return false;
        } else {
            m_counterpart->setName(currentName);
        }
        break;
    case ItemStatus::Current:
        setName(currentName);
        break;
    }
    return false;
}

const QString &FileSystemItem::newName() const
{
    switch (m_status) {
    case ItemStatus::Current:
        return m_counterpart ? m_counterpart->name() : emptyStr();
    case ItemStatus::New:
        return m_name;
    }
    return emptyStr();
}

bool FileSystemItem::setNewName(const QString &newName)
{
    switch (m_status) {
    case ItemStatus::Current:
        if (!m_counterpart) {
            if (m_parent) {
                m_counterpart = new FileSystemItem(ItemStatus::New, m_type, newName);
                m_counterpart->m_counterpart = this;
                m_counterpart->setParent(m_parent);
            } else {
                // parent required
                return false;
            }
        } else {
            m_counterpart->setName(newName);
        }
        return true;
    case ItemStatus::New:
        setName(newName);
        return true;
    }
    return false;
}

FileSystemItem *FileSystemItem::findChild(const QString &name) const
{
    for (FileSystemItem *child : m_children) {
        if (child->name() == name) {
            return child;
        }
    }
    return nullptr;
}

FileSystemItem *FileSystemItem::findChild(const QString &name, const FileSystemItem *exclude) const
{
    for (FileSystemItem *child : m_children) {
        if (child != exclude && child->name() == name) {
            return child;
        }
    }
    return nullptr;
}

FileSystemItem *FileSystemItem::makeChildAvailable(const QString &relativePath)
{
    QStringList dirs = relativePath.split(QDir::separator(), QString::SkipEmptyParts);
    FileSystemItem *parent = this;
    if (!dirs.isEmpty()) {
        if (relativePath.startsWith(QChar('/'))) {
            // we actually just got an absolute path
            // -> just leave the / there to handle absolute path as well
            dirs.front().prepend(QChar('/'));
        }
        for (const QString &dir : dirs) {
            FileSystemItem *child = parent->findChild(dir);
            if (!child) {
                child = new FileSystemItem(ItemStatus::New, ItemType::Dir, dir);
                child->setParent(parent);
                child->setNote(QCoreApplication::translate("RenamingUtility::FileSystemItem", "will be created"));
            }
            parent = child;
        }
    }
    return parent;
}

void FileSystemItem::relativeDir(QString &res) const
{
    if (m_parent) {
        m_parent->relativePath(res);
    }
}

QString FileSystemItem::relativeDir() const
{
    QString dir;
    relativeDir(dir);
    return dir;
}

void FileSystemItem::relativePath(QString &res) const
{
    if (m_parent) {
        m_parent->relativePath(res);
        if (!res.isEmpty()) {
            res.append(QLatin1Char('/'));
        }
        res.append(name());
    }
}

QString FileSystemItem::relativePath() const
{
    QString path;
    relativePath(path);
    return path;
}

bool FileSystemItem::hasSibling(const QString &name) const
{
    if (m_parent) {
        const QList<FileSystemItem *> &siblings = m_parent->children();
        for (const FileSystemItem *siblingItem : siblings) {
            if (siblingItem == this) {
                continue;
            }
            if (!siblingItem->newName().isEmpty() && siblingItem->newName() == name) {
                return true;
            } else if (siblingItem->name() == name) {
                return true;
            }
        }
    }
    return false;
}

} // namespace RenamingUtility
