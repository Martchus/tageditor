#ifndef ATTACHMENTSMODEL_H
#define ATTACHMENTSMODEL_H

#include <QAbstractTableModel>

namespace Media {
class AbstractAttachment;
}

namespace QtGui {

class AttachmentItem
{
public:
    AttachmentItem(Media::AbstractAttachment *attachment, bool activated = true, const QString &location = QString());

    Media::AbstractAttachment *attachment();
    const Media::AbstractAttachment *attachment() const;
    const QString &name() const;
    void setName(const QString &name);
    const QString &description() const;
    void setDescription(const QString &description);
    const QString &mimeType() const;
    void setMimeType(const QString &mimeType);
    const QString &size() const;
    const QString &location() const;
    bool isActivated() const;
    void setActivated(bool isActivated);

    void revert();
    void submit();

private:
    Media::AbstractAttachment *m_attachment;
    QString m_name;
    QString m_description;
    QString m_mimeType;
    QString m_size;
    QString m_location;
    bool m_activated;
};

class AttachmentsModel : public QAbstractTableModel
{
    Q_OBJECT
public:
    AttachmentsModel(QObject *parent = nullptr);
    ~AttachmentsModel();

    QVariant data(const QModelIndex &index, int role) const;
    bool setData(const QModelIndex &index, const QVariant &value, int role);
    Qt::ItemFlags flags(const QModelIndex &index) const;
    QVariant headerData(int section, Qt::Orientation orientation, int role) const;
    int rowCount(const QModelIndex &parent = QModelIndex()) const;
    int columnCount(const QModelIndex &parent = QModelIndex()) const;

    void revert();
    bool submit();
    void repealSelection();

    Media::AbstractAttachment *attachment(const QModelIndex &index);
    void addAttachment(int row, Media::AbstractAttachment *attachment, bool activated = true, const QString &location = QString());
    void setAttachments(const QList<Media::AbstractAttachment *> &attachments, bool activated = true, const QString &location = QString());
    void removeAttachments(int firstRow, int lastRow);

private:
    QList<AttachmentItem> m_attachments;
};

}

#endif // ATTACHMENTSMODEL_H
