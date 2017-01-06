#ifndef RENAMINGUTILITY_FILESYSTEMITEMMODEL_H
#define RENAMINGUTILITY_FILESYSTEMITEMMODEL_H

#include <QAbstractItemModel>

namespace RenamingUtility {

enum FileSystemItemModelRole {
    CheckableRole = Qt::UserRole + 1,
    ErrorStatusRole = Qt::UserRole + 2
};

class FileSystemItem;

class FileSystemItemModel : public QAbstractItemModel
{
    Q_OBJECT

public:
    explicit FileSystemItemModel(FileSystemItem *rootItem, QObject *parent = nullptr);
    ~FileSystemItemModel();

    void setRootItem(FileSystemItem *rootItem);
    QVariant data(const QModelIndex &index, int role) const;
    bool setData(const QModelIndex &index, const QVariant &value, int role);
    QVariant headerData(int section, Qt::Orientation orientation,
                        int role = Qt::DisplayRole) const;
    QModelIndex index(int row, int column,
                      const QModelIndex &parent = QModelIndex()) const;
    QModelIndex index(FileSystemItem *item, int column = 0) const;
    QModelIndex parent(const QModelIndex &index) const;
    QModelIndex counterpart(const QModelIndex &index, int column);
    int rowCount(const QModelIndex &parent = QModelIndex()) const;
    bool hasChildren(const QModelIndex &parent = QModelIndex()) const;
    int columnCount(const QModelIndex &index = QModelIndex()) const;


private:
    FileSystemItem *m_rootItem;
};

}

#endif // RENAMINGUTILITY_FILESYSTEMITEMMODEL_H
