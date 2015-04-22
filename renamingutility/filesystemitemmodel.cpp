#include "filesystemitemmodel.h"
#include "filesystemitem.h"

#include <QApplication>
#include <QStyle>
#include <QFont>
#include <QBrush>

#include <forward_list>

using namespace std;

namespace RenamingUtility {

/*
    TRANSLATOR RenamingUtility::FileSystemItemModel
    Necessary for lupdate.
*/

FileSystemItemModel::FileSystemItemModel(FileSystemItem *rootItem, QObject *parent) :
    QAbstractItemModel(parent),
    m_rootItem(rootItem)
{}

FileSystemItemModel::~FileSystemItemModel()
{}

void FileSystemItemModel::setRootItem(FileSystemItem *rootItem)
{
    if(m_rootItem != rootItem) {
        beginResetModel();
        m_rootItem = rootItem;
        endResetModel();
    }
}

QVariant FileSystemItemModel::data(const QModelIndex &index, int role) const
{
    if(index.isValid()) {
        if(FileSystemItem *item = reinterpret_cast<FileSystemItem *>(index.internalPointer())) {
            switch(role) {
            case Qt::DisplayRole:
                switch(index.column()) {
                case 0:
                    switch(item->status()) {
                    case ItemStatus::Current:
                        return item->name();
                    case ItemStatus::New:
                        return item->counterpart()
                                ? item->counterpart()->name()
                                : item->name();
                    }
                    break;
                case 1:
                    switch(item->status()) {
                    case ItemStatus::Current:
                        return item->counterpart()
                                ? item->counterpart()->name()
                                : item->name();
                    case ItemStatus::New:
                        return item->name();
                    }
                    break;
                case 2: return item->note();
                default: ;
                }
                break;
            case Qt::DecorationRole:
                switch(index.column()) {
                case 0:
                case 1:
                    switch(item->type()) {
                    case ItemType::Dir: return QApplication::style()->standardIcon(QStyle::SP_DirIcon);
                    default: ;
                    }
                    break;
                default: ;
                }
                break;
            case Qt::FontRole: {
                QFont font;
                if((index.column() == 0 && item->status() == ItemStatus::New && !item->counterpart())
                        || (index.column() == 1 && item->status() == ItemStatus::Current && !item->counterpart())) {
                    font.setItalic(true);
                }
                return font;
            } case Qt::ForegroundRole:
                if(item->errorOccured()) {
                    return QBrush(Qt::red);
                } else if(item->applied()) {
                    return QBrush(Qt::darkGreen);
                } else if((index.column() == 0 && item->status() == ItemStatus::New && !item->counterpart())
                          || (index.column() == 1 && item->status() == ItemStatus::Current && !item->counterpart())) {
                    return QBrush(Qt::gray);
                }
                break;
            /*case Qt::CheckStateRole:
                if(item->checkable() && index.column() == 1) {
                    return item->checked() ? Qt::Checked : Qt::Unchecked;
                }
                break;
            case CheckableRole:
                if(index.column() == 1) {
                    return item->checkable();
                }
                break;*/
            case ErrorStatusRole:
                return item->errorOccured();
            default:
                ;
            }
        }
    }
    return QVariant();
}

bool FileSystemItemModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    if(index.isValid()) {
        if(FileSystemItem *item = reinterpret_cast<FileSystemItem *>(index.internalPointer())) {
            switch(role) {
            case Qt::DisplayRole:
                switch(index.column()) {
                case 0:
                    if(item->setCurrentName(value.toString())) {
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
                default: ;
                }
                break;
            /*case Qt::CheckStateRole:
                item->setChecked(value.toInt() == Qt::Checked);
                emit dataChanged(index, index, QVector<int>() << role);
                return true;*/
            case ErrorStatusRole:
                item->setErrorOccured(value.toBool());
                emit dataChanged(index, index, QVector<int>() << role << Qt::DecorationRole);
                return true;
            default:
                ;
            }
        }
    }
    return false;
}

Qt::ItemFlags FileSystemItemModel::flags(const QModelIndex &index) const
{
    /*if(index.isValid() && index.column() == 1) {
        if(FileSystemItem *item = reinterpret_cast<FileSystemItem *>(index.internalPointer())) {
            if(item->checkable()) {
                return Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsUserCheckable;
            }
        }
    }*/
    return QAbstractItemModel::flags(index);
}

QVariant FileSystemItemModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    switch(orientation) {
    case Qt::Horizontal:
        switch(role) {
        case Qt::DisplayRole:
            switch(section) {
            case 0: return tr("Current name");
            case 1: return tr("New name");
            case 2: return tr("Notes");
            default: ;
            }
            break;
        default: ;
        }
        break;
    default: ;
    }
    return QVariant();
}

QModelIndex FileSystemItemModel::index(int row, int column, const QModelIndex &parent) const
{
    if(FileSystemItem *parentItem = parent.isValid()
            ? reinterpret_cast<FileSystemItem *>(parent.internalPointer())
            : m_rootItem) {
        const QList<FileSystemItem *> &children = parentItem->children();
        if(row < children.length()) {
            return createIndex(row, column, children.at(row));
        }
    }
    return QModelIndex();
}

QModelIndex FileSystemItemModel::index(FileSystemItem *item, int column) const
{
    forward_list<FileSystemItem *> path;
    path.push_front(item);
    FileSystemItem *parent = item->parent();
    while(parent) {
        path.push_front(parent);
        parent = parent->parent();
    }
    if(path.front() == m_rootItem) {
        path.pop_front();
        QModelIndex index;
        for(FileSystemItem *pathItem : path) {
            index = this->index(pathItem->row(), column, index);
            if(pathItem == item || !index.isValid()) {
                return index;
            }
        }
    }
    return QModelIndex();
}

QModelIndex FileSystemItemModel::parent(const QModelIndex &index) const
{
    if(index.isValid()) {
        if(FileSystemItem *item = reinterpret_cast<FileSystemItem *>(index.internalPointer())) {
            FileSystemItem *parent = item->parent();
            if(parent && (index.row() < parent->children().length())) {
                return createIndex(parent->row(), index.column(), parent);
            }
        }
    }
    return QModelIndex();
}

QModelIndex FileSystemItemModel::counterpart(const QModelIndex &index, int column = -1)
{
    if(index.isValid()) {
        if(column < 0) {
            column = index.column();
        }
        if(FileSystemItem *item = reinterpret_cast<FileSystemItem *>(index.internalPointer())) {
            if(item->counterpart()) {
                return this->index(item->counterpart(), column);
            }
        }
    }
    return QModelIndex();
}

int FileSystemItemModel::rowCount(const QModelIndex &parent) const
{
    if(FileSystemItem *parentItem = parent.isValid()
            ? reinterpret_cast<FileSystemItem *>(parent.internalPointer())
            : m_rootItem) {
        return parentItem->children().length();
    } else {
        return 0;
    }
}

bool FileSystemItemModel::hasChildren(const QModelIndex &parent) const
{
    if(FileSystemItem *parentItem = parent.isValid()
            ? reinterpret_cast<FileSystemItem *>(parent.internalPointer())
            : m_rootItem) {
        return parentItem->children().length() > 0;
    } else {
        return false;
    }
}

int FileSystemItemModel::columnCount(const QModelIndex &) const
{
    return 3;
}

}
