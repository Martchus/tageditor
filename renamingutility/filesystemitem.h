#ifndef FILESYSTEMITEM_H
#define FILESYSTEMITEM_H

#include <QList>
#include <QString>

namespace RenamingUtility {

enum class ActionType { None, Rename, Skip };

enum class ItemStatus { Current, New };

enum class ItemType { Dir, File };

class FileSystemItem {

public:
    explicit FileSystemItem(
        ItemStatus status = ItemStatus::Current, ItemType type = ItemType::File, const QString &name = QString(), FileSystemItem *parent = nullptr);
    ~FileSystemItem();
    FileSystemItem(const FileSystemItem &other) = delete;
    FileSystemItem &operator=(const FileSystemItem &other) = delete;

    FileSystemItem *root();
    FileSystemItem *parent() const;
    void setParent(FileSystemItem *parent);
    const QList<FileSystemItem *> &children() const;
    FileSystemItem *counterpart() const;
    void setCounterpart(FileSystemItem *counterpart);
    FileSystemItem *findChild(const QString &name) const;
    FileSystemItem *findChild(const QString &name, const FileSystemItem *exclude) const;
    FileSystemItem *makeChildAvailable(const QString &relativePath);
    ItemStatus status() const;
    ItemType type() const;
    bool errorOccured() const;
    void setErrorOccured(bool errorOccured);
    bool applied() const;
    void setApplied(bool applied);
    const QString &name() const;
    void setName(const QString &name);
    const QString &currentName() const;
    bool setCurrentName(const QString &currentName);
    const QString &newName() const;
    bool setNewName(const QString &newName);
    const QString &note() const;
    void setNote(const QString &note);
    bool checked() const;
    void setChecked(bool checked);
    bool checkable() const;
    void setCheckable(bool checkable);
    int row() const;
    void relativeDir(QString &res) const;
    QString relativeDir() const;
    void relativePath(QString &res) const;
    QString relativePath() const;
    bool hasSibling(const QString &name) const;

private:
    FileSystemItem *m_parent;
    QList<FileSystemItem *> m_children;
    FileSystemItem *m_counterpart;
    ItemStatus m_status;
    ItemType m_type;
    bool m_error;
    bool m_applied;
    QString m_name;
    QString m_note;
    bool m_checked;
    bool m_checkable;
};

inline FileSystemItem *FileSystemItem::root()
{
    return m_parent ? m_parent->root() : this;
}

inline FileSystemItem *FileSystemItem::parent() const
{
    return m_parent;
}

inline const QList<FileSystemItem *> &FileSystemItem::children() const
{
    return m_children;
}

inline FileSystemItem *FileSystemItem::counterpart() const
{
    return m_counterpart;
}

inline void FileSystemItem::setCounterpart(FileSystemItem *counterpart)
{
    if (m_counterpart) {
        m_counterpart->m_counterpart = nullptr;
    }
    m_counterpart = counterpart;
    if (m_counterpart) {
        m_counterpart->m_counterpart = this;
    }
}

inline ItemStatus FileSystemItem::status() const
{
    return m_status;
}

inline ItemType FileSystemItem::type() const
{
    return m_type;
}

inline bool FileSystemItem::errorOccured() const
{
    if (m_counterpart) {
        return m_error || m_counterpart->m_error;
    } else {
        return m_error;
    }
}

inline void FileSystemItem::setErrorOccured(bool error)
{
    m_error = error;
}

inline bool FileSystemItem::applied() const
{
    if (m_counterpart) {
        return m_applied || m_counterpart->m_applied;
    } else {
        return m_applied;
    }
}

inline void FileSystemItem::setApplied(bool applied)
{
    m_applied = applied;
}

inline const QString &FileSystemItem::name() const
{
    return m_name;
}

inline void FileSystemItem::setName(const QString &name)
{
    m_name = name;
}

inline const QString &FileSystemItem::note() const
{
    if (m_note.isEmpty() && m_counterpart) {
        return m_counterpart->note();
    } else {
        return m_note;
    }
}

inline void FileSystemItem::setNote(const QString &name)
{
    m_note = name;
}

inline bool FileSystemItem::checked() const
{
    return m_checked;
}

inline void FileSystemItem::setChecked(bool checked)
{
    m_checked = checked;
}

inline bool FileSystemItem::checkable() const
{
    if (m_counterpart) {
        return m_counterpart->m_checkable || m_checkable;
    } else {
        return m_checkable;
    }
}

inline void FileSystemItem::setCheckable(bool checkable)
{
    m_checkable = checkable;
}

inline int FileSystemItem::row() const
{
    return m_parent ? m_parent->children().indexOf(const_cast<FileSystemItem *>(this)) : -1;
}

} // namespace RenamingUtility

#endif // FILESYSTEMITEM_H
