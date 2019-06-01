#ifndef RENAMINGUTILITY_FILESYSTEMITEMMODEL_H
#define RENAMINGUTILITY_FILESYSTEMITEMMODEL_H

#include <QAbstractItemModel>

namespace RenamingUtility {

enum FileSystemItemModelRole { CheckableRole = Qt::UserRole + 1, ErrorStatusRole = Qt::UserRole + 2 };

class FileSystemItem;

class FileSystemItemModel : public QAbstractItemModel {
    Q_OBJECT

public:
    explicit FileSystemItemModel(FileSystemItem *rootItem, QObject *parent = nullptr);

    void setRootItem(FileSystemItem *rootItem);
    FileSystemItem *fileSystemItemFromIndex(const QModelIndex &index);
    const FileSystemItem *fileSystemItemFromIndex(const QModelIndex &index) const;
    QVariant data(const QModelIndex &index, int role) const override;
    bool setData(const QModelIndex &index, const QVariant &value, int role) override;
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;
    QModelIndex index(int row, int column, const QModelIndex &parent = QModelIndex()) const override;
    QModelIndex index(FileSystemItem *item, int column = 0) const;
    QModelIndex parent(const QModelIndex &index) const override;
    QModelIndex counterpart(const QModelIndex &index, int column);
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    bool hasChildren(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &index = QModelIndex()) const override;

private:
    FileSystemItem *m_rootItem;
};

} // namespace RenamingUtility

#endif // RENAMINGUTILITY_FILESYSTEMITEMMODEL_H
