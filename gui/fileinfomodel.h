#ifndef FILEINFOMODEL_H
#define FILEINFOMODEL_H

#include <QStandardItemModel>

#include <list>

namespace Media {
class MediaFileInfo;
class Notification;
typedef std::list<Notification> NotificationList;
}

namespace QtGui {

class FileInfoModel : public QStandardItemModel
{
    Q_OBJECT
public:
    explicit FileInfoModel(Media::MediaFileInfo *fileInfo = nullptr, QObject *parent = nullptr);

    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const;

    const Media::MediaFileInfo *fileInfo() const;
    void setFileInfo(Media::MediaFileInfo *fileInfo, Media::NotificationList *originalNotifications = nullptr);

#if defined(GUI_QTWIDGETS)
    static const QIcon &informationIcon();
    static const QIcon &warningIcon();
    static const QIcon &errorIcon();
    static const QIcon &debugIcon();
#endif

private:
    void updateCache();

private:
    Media::MediaFileInfo *m_file;
    Media::NotificationList *m_originalNotifications;
};

}

#endif // FILEINFOMODEL_H
