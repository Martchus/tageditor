#ifndef FILEFILTERPROXYMODEL_H
#define FILEFILTERPROXYMODEL_H

#include <QSortFilterProxyModel>
#include <QStringList>

namespace QtGui {

class FileFilterProxyModel : public QSortFilterProxyModel
{
    Q_OBJECT

public:
    FileFilterProxyModel(QObject *parent = nullptr);

    bool isFilterEnabled() const;
    void setFilterEnabled(bool value);
    const QStringList &extensionsToBeFiltered() const;
    void setExtensionsToBeFiltered(const QStringList &extensions);
    bool isFileAccepted(const QString &path) const;

protected:
    virtual bool filterAcceptsRow(int source_row, const QModelIndex& sourceParent) const;

private:
    bool m_filterEnabled;
    QStringList m_extensionsToBeFiltered;
    
};

}

#endif // FILEFILTERPROXYMODEL_H
