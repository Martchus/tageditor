#include "./filesystemitemmodel.h"
#include "./filesystemitem.h"

#include "../misc/utility.h"

#include <QApplication>
#include <QBrush>
#include <QFont>
#include <QStyle>

#include <forward_list>

using namespace std;

namespace RenamingUtility {

/*
    TRANSLATOR RenamingUtility::FileSystemItemModel
    Necessary for lupdate.
*/

FileSystemItemModel::FileSystemItemModel(FileSystemItem *rootItem, QObject *parent)
    : QAbstractItemModel(parent)
    , m_rootItem(rootItem)
{
}

void FileSystemItemModel::setRootItem(FileSystemItem *rootItem)
{
    if (m_rootItem == rootItem) {
        return;
    }
    beginResetModel();
    m_rootItem = rootItem;
    endResetModel();
}

FileSystemItem *FileSystemItemModel::fileSystemItemFromIndex(const QModelIndex &index)
{
    return index.isValid() ? reinterpret_cast<FileSystemItem *>(index.internalPointer()) : nullptr;
}

const FileSystemItem *FileSystemItemModel::fileSystemItemFromIndex(const QModelIndex &index) const
{
    return const_cast<FileSystemItemModel *>(this)->fileSystemItemFromIndex(index);
}

QVariant FileSystemItemModel::data(const QModelIndex &index, int role) const
{
    const auto *const item = fileSystemItemFromIndex(index);
    if (!item) {
        return QVariant();
    }
    switch (role) {
    case Qt::DisplayRole:
        switch (index.column()) {
        case 0:
            switch (item->status()) {
            case ItemStatus::Current:
                return item->name();
            case ItemStatus::New:
                return item->counterpart() ? item->counterpart()->name() : item->name();
            }
            break;
        case 1:
            switch (item->status()) {
            case ItemStatus::Current:
                return item->counterpart() ? item->counterpart()->name() : item->name();
            case ItemStatus::New:
                return item->name();
            }
            break;
        case 2:
            return item->note();
        default:;
        }
        break;
    case Qt::DecorationRole:
        switch (index.column()) {
        case 0:
        case 1:
            switch (item->type()) {
            case ItemType::Dir:
                return QApplication::style()->standardIcon(QStyle::SP_DirIcon);
            default:;
            }
            break;
        default:;
        }
        break;
    case Qt::FontRole: {
        QFont font;
        if ((index.column() == 0 && item->status() == ItemStatus::New && !item->counterpart())
            || (index.column() == 1 && item->status() == ItemStatus::Current && !item->counterpart())) {
            font.setItalic(true);
        }
        return font;
    }
    case Qt::ForegroundRole:
        if (item->errorOccured()) {
            return QBrush(Qt::red);
        } else if (item->applied()) {
            return QBrush(Qt::darkGreen);
        } else if ((index.column() == 0 && item->status() == ItemStatus::New && !item->counterpart())
            || (index.column() == 1 && item->status() == ItemStatus::Current && !item->counterpart())) {
            return QBrush(Qt::gray);
        }
        break;
    case ErrorStatusRole:
        return item->errorOccured();
    default:;
    }
    return QVariant();
}

bool FileSystemItemModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    auto *const item = fileSystemItemFromIndex(index);
    if (!item) {
        return false;
    }
    switch (role) {
    case Qt::DisplayRole:
        switch (index.column()) {
        case 0:
            if (item->setCurrentName(value.toString())) {
                emit dataChanged(index, index, QVector<int>() << role);
                return true;
            } else {
                return false;
            }
        case 1:
            item->setNewName(value.toString());
            emit dataChanged(index, index, QVector<int>() << role);
            return true;
        case 2:
            item->setNote(value.toString());
            emit dataChanged(index, index, QVector<int>() << role);
            return true;
        default:;
        }
        break;
    case ErrorStatusRole:
        item->setErrorOccured(value.toBool());
        emit dataChanged(index, index, QVector<int>() << role << Qt::DecorationRole);
        return true;
    default:;
    }
    return false;
}

QVariant FileSystemItemModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    switch (orientation) {
    case Qt::Horizontal:
        switch (role) {
        case Qt::DisplayRole:
            switch (section) {
            case 0:
                return tr("Current name");
            case 1:
                return tr("New name");
            case 2:
                return tr("Note");
            default:;
            }
            break;
        default:;
        }
        break;
    default:;
    }
    return QVariant();
}

QModelIndex FileSystemItemModel::index(int row, int column, const QModelIndex &parent) const
{
    const auto *const parentItem = parent.isValid() ? reinterpret_cast<FileSystemItem *>(parent.internalPointer()) : m_rootItem;
    if (!parentItem) {
        return QModelIndex();
    }
    const auto &children = parentItem->children();
    return row < children.size() ? createIndex(row, column, children.at(row)) : QModelIndex();
}

QModelIndex FileSystemItemModel::index(FileSystemItem *item, int column) const
{
    forward_list<FileSystemItem *> path;
    path.push_front(item);
    auto *parent = item->parent();
    while (parent) {
        path.push_front(parent);
        parent = parent->parent();
    }
    if (path.front() != m_rootItem) {
        return QModelIndex();
    }
    path.pop_front();
    QModelIndex index;
    for (auto *const pathItem : path) {
        index = this->index(pathItem->row(), column, index);
        if (pathItem == item || !index.isValid()) {
            return index;
        }
    }
    return QModelIndex();
}

QModelIndex FileSystemItemModel::parent(const QModelIndex &index) const
{
    const auto *const item = fileSystemItemFromIndex(index);
    if (!item) {
        return QModelIndex();
    }
    auto *const parent = item->parent();
    if (!parent || index.row() >= parent->children().size()) {
        return QModelIndex();
    }
    return createIndex(parent->row(), index.column(), parent);
}

QModelIndex FileSystemItemModel::counterpart(const QModelIndex &index, int column = -1)
{
    const auto *const item = fileSystemItemFromIndex(index);
    if (!item) {
        return QModelIndex();
    }
    if (column < 0) {
        column = index.column();
    }
    return item->counterpart() ? this->index(item->counterpart(), column) : QModelIndex();
}

int FileSystemItemModel::rowCount(const QModelIndex &parent) const
{
    if (const auto *const parentItem = (parent.isValid() ? reinterpret_cast<FileSystemItem *>(parent.internalPointer()) : m_rootItem)) {
        return Utility::containerSizeToInt(parentItem->children().size());
    } else {
        return 0;
    }
}

bool FileSystemItemModel::hasChildren(const QModelIndex &parent) const
{
    if (const auto *const parentItem = (parent.isValid() ? reinterpret_cast<const FileSystemItem *>(parent.internalPointer()) : m_rootItem)) {
        return parentItem->children().size() > 0;
    } else {
        return false;
    }
}

int FileSystemItemModel::columnCount(const QModelIndex &) const
{
    return 3;
}

} // namespace RenamingUtility
