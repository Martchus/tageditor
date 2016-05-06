#ifndef RENAIMINGUTILITY_FILTEREDFILESYSTEMITEMMODEL_H
#define RENAIMINGUTILITY_FILTEREDFILESYSTEMITEMMODEL_H

#include "filesystemitem.h"

#include <QSortFilterProxyModel>

namespace RenamingUtility {

class FilteredFileSystemItemModel : public QSortFilterProxyModel
{
    Q_OBJECT

public:
    FilteredFileSystemItemModel(ItemStatus statusFilter = ItemStatus::Current, QObject *parent = nullptr);

    ItemStatus statusFilter() const;
    void setStatusFilter(ItemStatus statusFilter);

protected:
    bool filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const;
    bool filterAcceptsColumn(int sourceColumn, const QModelIndex &sourceParent) const;

private:
    ItemStatus m_statusFilter;
};

inline ItemStatus FilteredFileSystemItemModel::statusFilter() const
{
    return m_statusFilter;
}

inline void FilteredFileSystemItemModel::setStatusFilter(ItemStatus statusFilter)
{
    m_statusFilter = statusFilter;
    invalidateFilter();
}

}

#endif // RENAIMINGUTILITY_FILTEREDFILESYSTEMITEMMODEL_H
