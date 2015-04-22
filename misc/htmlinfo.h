#ifndef HTMLINFO_H
#define HTMLINFO_H

#include <QByteArray>

#include <list>

namespace Media {
class MediaFileInfo;
class Notification;
typedef std::list<Notification> NotificationList;
}

namespace HtmlInfo {

QByteArray generateInfo(const Media::MediaFileInfo &file, Media::NotificationList &originalNotifications);

}

#endif // HTMLINFO_H
