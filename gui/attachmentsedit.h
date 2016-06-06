#ifndef ATTACHMENTSEDIT_H
#define ATTACHMENTSEDIT_H

#include <QWidget>

#include <memory>

namespace Media {
class MediaFileInfo;
class AbstractAttachment;
}

namespace QtGui {

class AttachmentsModel;

namespace Ui {
class AttachmentsEdit;
}

class AttachmentsEdit : public QWidget
{
    Q_OBJECT

public:
    explicit AttachmentsEdit(Media::MediaFileInfo *fileInfo, QWidget *parent = nullptr);
    ~AttachmentsEdit();

    Media::MediaFileInfo *fileInfo();
    void setFileInfo(Media::MediaFileInfo *fileInfo, bool updateUi = true);
    const QList<Media::AbstractAttachment *> &currentAttachments() const;

public slots:
    void clear();
    void restore();
    void apply();
    void invalidate();
    void addFile(const QString &path);

    void showFileSelection();
    void extractSelected();

private slots:
    void selectionChanged(const QModelIndex &current, const QModelIndex &previous);

private:
    void setupUi();

    std::unique_ptr<Ui::AttachmentsEdit> m_ui;
    Media::MediaFileInfo *m_fileInfo;
    QList<Media::AbstractAttachment *> m_currentAttachments;
    QList<Media::AbstractAttachment *> m_addedAttachments;
    AttachmentsModel *m_model;
};

inline Media::MediaFileInfo *AttachmentsEdit::fileInfo()
{
    return m_fileInfo;
}

inline const QList<Media::AbstractAttachment *> &AttachmentsEdit::currentAttachments() const
{
    return m_currentAttachments;
}

}

#endif // ATTACHMENTSEDIT_H
