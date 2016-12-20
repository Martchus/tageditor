#include "./notificationmodel.h"

#include <c++utilities/chrono/datetime.h>

#include <QApplication>
#include <QStyle>
#include <QIcon>

using namespace std;
using namespace ChronoUtilities;
using namespace Media;

namespace QtGui {

NotificationModel::NotificationModel(QObject *parent) :
    QAbstractListModel(parent)
{}

QVariant NotificationModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    switch(orientation) {
    case Qt::Horizontal:
        switch(role) {
        case Qt::DisplayRole:
            switch(section) {
            case 0:
                return tr("Context");
            case 1:
                return tr("Message");
            case 2:
                return tr("Time");
            default:
                ;
            }
            break;
        }        
        break;
    default:
        ;
    }
    return QVariant();
}

int NotificationModel::columnCount(const QModelIndex &parent) const
{
    if(!parent.isValid()) {
        return 3;
    }
    return 0;
}

int NotificationModel::rowCount(const QModelIndex &parent) const
{
    if(!parent.isValid()) {
        return m_notifications.size();
    }
    return 0;
}

Qt::ItemFlags NotificationModel::flags(const QModelIndex &index) const
{
    return QAbstractListModel::flags(index);
}

QVariant NotificationModel::data(const QModelIndex &index, int role) const
{
    if(index.isValid() && index.row() < m_notifications.size()) {
        switch(role) {
        case Qt::DisplayRole:
            switch(index.column()) {
            case 0: {
                const string &context = m_notifications.at(index.row()).context();
                if(context.empty()) {
                    return tr("unspecified");
                } else {
                    return QString::fromUtf8(context.c_str());
                }
            }
            case 1:
                return QString::fromUtf8(m_notifications.at(index.row()).message().c_str());
            case 2:
                return QString::fromUtf8(m_notifications.at(index.row()).creationTime().toString(DateTimeOutputFormat::DateAndTime, true).c_str());
            default:
                ;
            }
            break;
        case Qt::DecorationRole:
            switch(index.column()) {
            case 0:
                switch(m_notifications.at(index.row()).type()) {
                case NotificationType::Information:
                    return informationIcon();
                case NotificationType::Warning:
                    return warningIcon();
                case NotificationType::Critical:
                    return errorIcon();
                case NotificationType::Debug:
                    return debugIcon();
                default:
                    ;
                }
            default:
                ;
            }
            break;
        default:
            ;
        }
    }
    return QVariant();
}

const QList<Notification> &NotificationModel::notifications() const
{
    return m_notifications;
}

void NotificationModel::setNotifications(const QList<Notification> &notifications)
{
    beginResetModel();
    m_notifications = notifications;
    endResetModel();
}

void NotificationModel::setNotifications(const NotificationList &notifications)
{
    setNotifications(QList<Notification>::fromStdList(notifications));
}

const QIcon &NotificationModel::informationIcon()
{
    static const QIcon icon = QApplication::style()->standardIcon(QStyle::SP_MessageBoxInformation);
    return icon;
}

const QIcon &NotificationModel::warningIcon()
{
    static const QIcon icon = QApplication::style()->standardIcon(QStyle::SP_MessageBoxWarning);
    return icon;
}

const QIcon &NotificationModel::errorIcon()
{
    static const QIcon icon = QApplication::style()->standardIcon(QStyle::SP_MessageBoxCritical);
    return icon;
}

const QIcon &NotificationModel::debugIcon()
{
    static const QIcon icon = QIcon(QStringLiteral("/images/bug"));
    return icon;
}

}
