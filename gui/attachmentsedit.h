#ifndef ATTACHMENTSEDIT_H
#define ATTACHMENTSEDIT_H

#include <QWidget>

#include <memory>

namespace TagParser {
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
    explicit AttachmentsEdit(TagParser::MediaFileInfo *fileInfo, QWidget *parent = nullptr);
    ~AttachmentsEdit();

    TagParser::MediaFileInfo *fileInfo();
    void setFileInfo(TagParser::MediaFileInfo *fileInfo, bool updateUi = true);
    const QList<TagParser::AbstractAttachment *> &currentAttachments() const;

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
    TagParser::MediaFileInfo *m_fileInfo;
    QList<TagParser::AbstractAttachment *> m_currentAttachments;
    QList<TagParser::AbstractAttachment *> m_addedAttachments;
    AttachmentsModel *m_model;
};

inline TagParser::MediaFileInfo *AttachmentsEdit::fileInfo()
{
    return m_fileInfo;
}

inline const QList<TagParser::AbstractAttachment *> &AttachmentsEdit::currentAttachments() const
{
    return m_currentAttachments;
}

}

#endif // ATTACHMENTSEDIT_H
