#ifndef FILEINFOMODEL_H
#define FILEINFOMODEL_H

#include <taginfo/mediafileinfo.h>

#include <QAbstractItemModel>
#include <QIcon>

namespace QtGui {

class TreeItem
{
public:
    explicit TreeItem(const QList<QVariant> &data, TreeItem *parent = nullptr);
    explicit TreeItem(const QString &data0, const QString &data1, TreeItem *parent = nullptr);
    explicit TreeItem(const QString &data0, const std::string &data1, TreeItem *parent = nullptr);
    explicit TreeItem(const std::string &data0, const std::string &data1, TreeItem *parent = nullptr);
    ~TreeItem();

    void appendChild(TreeItem *child);
    TreeItem *appendChild(const QList<QVariant> &data);
    TreeItem *appendChild(const QString &data);
    TreeItem *appendChild(const QString &data0, const QString &data1);
    TreeItem *appendChild(const QString &data0, const std::string &data1);
    TreeItem *appendChild(const std::string &data);
    TreeItem *appendChild(const std::string &data0, const std::string &data1);
    TreeItem *appendChild(const QString &data0, int data1);

    TreeItem *child(int row);
    int childCount() const;
    int columnCount() const;
    QVariant data(int column) const;
    int row() const;
    TreeItem *parent();
    void clearChilds();
    void clearData();
    const QIcon &icon() const;
    void setIcon(const QIcon &value);

private:
    QList<TreeItem *> m_childItems;
    QList<QVariant> m_itemData;
    QIcon m_icon;
    TreeItem *m_parentItem;

};

class FileInfoModel : public QAbstractItemModel
{
    Q_OBJECT

public:
    explicit FileInfoModel(QObject *parent = nullptr);
    explicit FileInfoModel(Media::MediaFileInfo *fileInfo, QObject *parent = nullptr);
    ~FileInfoModel();

    const Media::MediaFileInfo *shownFileInfo() const;
    Media::MediaFileInfo *shownFileInfo();
    void show(Media::MediaFileInfo *fileInfo);
    void update();
    bool containsAnyNotifications() const;
    bool containsCriticalNotifications() const;

    QVariant data(const QModelIndex &index, int role) const;
    Qt::ItemFlags flags(const QModelIndex &index) const;
    QVariant headerData(int section, Qt::Orientation orientation,
                        int role = Qt::DisplayRole) const;
    QModelIndex index(int row, int column,
                      const QModelIndex &parent = QModelIndex()) const;
    QModelIndex parent(const QModelIndex &index) const;
    int rowCount(const QModelIndex &parent = QModelIndex()) const;
    int columnCount(const QModelIndex &parent = QModelIndex()) const;

private:
    void showGeneralInformation();
    void showAtoms(TreeItem *item, Media::Mp4Atom *atom);
    void showEbmlElements(TreeItem *item, Media::EbmlElement *element);
    void showTracks();
    void showTagInformation();
    void showTagDetails(const Media::Tag *tag, TreeItem *item);
    void showNotifications(const std::list<Media::Notification> &notifications, TreeItem *item, const QString &label = tr("Notifications"), bool showEmptyEntry = true);

    TreeItem *m_rootItem;
    TreeItem *m_generalItem;
    TreeItem *m_audioTracksItem;
    TreeItem *m_videoTracksItem;
    TreeItem *m_subtitleTracksItem;
    TreeItem *m_otherTracksItem;
    TreeItem *m_tagItem;
    Media::MediaFileInfo *m_fileInfo;
    bool m_anyNotifications;
    bool m_criticalNotifications;
};

}

#endif // FILEINFOMODEL_H
