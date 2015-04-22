#include "filefilterproxymodel.h"

#include <QFileSystemModel>

namespace QtGui {

/*
    TRANSLATOR QtGui::FileFilterProxyModel
    Necessary for lupdate.
*/

FileFilterProxyModel::FileFilterProxyModel(QObject *parent) :
    QSortFilterProxyModel(parent),
    m_filterEnabled(true)
{
}

bool FileFilterProxyModel::isFilterEnabled() const
{
    return m_filterEnabled;
}

void FileFilterProxyModel::setFilterEnabled(bool value)
{
    if(m_filterEnabled != value) {
        beginResetModel();
        m_filterEnabled = value;
        endResetModel();
    }
}

const QStringList &FileFilterProxyModel::extensionsToBeFiltered() const
{
    return m_extensionsToBeFiltered;
}

void FileFilterProxyModel::setExtensionsToBeFiltered(const QStringList &extensions)
{
    m_extensionsToBeFiltered = extensions;
}

bool FileFilterProxyModel::isFileAccepted(const QString &path) const
{
    return !m_extensionsToBeFiltered.contains(QFileInfo(path).suffix());
}

bool FileFilterProxyModel::filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const
{
    if(!m_filterEnabled)
        return QSortFilterProxyModel::filterAcceptsRow(sourceRow, sourceParent);
    QModelIndex index0 = sourceModel()->index(sourceRow, 0, sourceParent);
    QFileSystemModel *fileModel = qobject_cast<QFileSystemModel *>(sourceModel());
    return fileModel
            ? (!m_extensionsToBeFiltered.contains(fileModel->fileInfo(index0).suffix()))
            : QSortFilterProxyModel::filterAcceptsRow(sourceRow, sourceParent);
}

}
