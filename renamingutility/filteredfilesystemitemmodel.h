#ifndef RENAIMINGUTILITY_FILTEREDFILESYSTEMITEMMODEL_H
#define RENAIMINGUTILITY_FILTEREDFILESYSTEMITEMMODEL_H

#include "filesystemitem.h"

#include <QSortFilterProxyModel>

namespace RenamingUtility {

class FilteredFileSystemItemModel : public QSortFilterProxyModel {
    Q_OBJECT

public:
    explicit FilteredFileSystemItemModel(ItemStatus statusFilter = ItemStatus::Current, QObject *parent = nullptr);

    ItemStatus statusFilter() const;
    void setStatusFilter(ItemStatus statusFilter);

protected:
    bool filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const override;
    bool filterAcceptsColumn(int sourceColumn, const QModelIndex &sourceParent) const override;

private:
    ItemStatus m_statusFilter;
};

inline ItemStatus FilteredFileSystemItemModel::statusFilter() const
{
    return m_statusFilter;
}

inline void FilteredFileSystemItemModel::setStatusFilter(ItemStatus statusFilter)
{
#if QT_VERSION >= QT_VERSION_CHECK(6, 10, 0)
    beginFilterChange();
#endif
    m_statusFilter = statusFilter;
#if QT_VERSION >= QT_VERSION_CHECK(6, 10, 0)
    endFilterChange();
#else
    invalidateFilter();
#endif
}

} // namespace RenamingUtility

#endif // RENAIMINGUTILITY_FILTEREDFILESYSTEMITEMMODEL_H
