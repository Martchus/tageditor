#ifndef NOTIFICATIONMODEL_H
#define NOTIFICATIONMODEL_H

#include <tagparser/diagnostics.h>

#include <QAbstractListModel>

namespace QtGui {

class DiagModel : public QAbstractListModel {
    Q_OBJECT
public:
    explicit DiagModel(QObject *parent = nullptr);

    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const;
    int columnCount(const QModelIndex &parent) const;
    int rowCount(const QModelIndex &parent = QModelIndex()) const;
    Qt::ItemFlags flags(const QModelIndex &index) const;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;

    const TagParser::Diagnostics &diagnostics() const;
    void setDiagnostics(const TagParser::Diagnostics &diagnostics);

    static const QIcon &informationIcon();
    static const QIcon &warningIcon();
    static const QIcon &errorIcon();
    static const QIcon &debugIcon();

signals:

public slots:

private:
    TagParser::Diagnostics m_diag;
};

} // namespace QtGui

#endif // NOTIFICATIONMODEL_H
