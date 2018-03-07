#ifndef FILEFILTERPROXYMODEL_H
#define FILEFILTERPROXYMODEL_H

#include <QSortFilterProxyModel>
#include <QStringList>

namespace QtGui {

class FileFilterProxyModel : public QSortFilterProxyModel {
    Q_OBJECT
    Q_PROPERTY(bool filterEnabled READ isFilterEnabled WRITE setFilterEnabled)
    Q_PROPERTY(QStringList extensionsToBeFiltered READ extensionsToBeFiltered WRITE setExtensionsToBeFiltered)

public:
    FileFilterProxyModel(QObject *parent = nullptr);

    bool isFilterEnabled() const;
    void setFilterEnabled(bool value);
    const QStringList &extensionsToBeFiltered() const;
    void setExtensionsToBeFiltered(const QStringList &extensions);
    bool isFileAccepted(const QString &path) const;

protected:
    virtual bool filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const;
    void sort(int column, Qt::SortOrder order = Qt::AscendingOrder);

private:
    bool m_filterEnabled;
    QStringList m_extensionsToBeFiltered;
};

} // namespace QtGui

#endif // FILEFILTERPROXYMODEL_H
