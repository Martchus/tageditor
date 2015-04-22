#ifndef NOTIFICATIONMODEL_H
#define NOTIFICATIONMODEL_H

#include <tagparser/notification.h>

#include <QAbstractListModel>

namespace QtGui {

class NotificationModel : public QAbstractListModel
{
    Q_OBJECT
public:
    explicit NotificationModel(QObject *parent = nullptr);

    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const;
    int columnCount(const QModelIndex &parent) const;
    int rowCount(const QModelIndex &parent = QModelIndex()) const;
    Qt::ItemFlags flags(const QModelIndex &index) const;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;

    const QList<Media::Notification> &notifications() const;
    void setNotifications(const QList<Media::Notification> &notifications);
    void setNotifications(const Media::NotificationList &notifications);

    static const QIcon &informationIcon();
    static const QIcon &warningIcon();
    static const QIcon &errorIcon();
    static const QIcon &debugIcon();

signals:

public slots:

private:
    QList<Media::Notification> m_notifications;

};

}

#endif // NOTIFICATIONMODEL_H
