#ifndef COVEREDIT_H
#define COVEREDIT_H

#include "./previousvaluehandling.h"

#include <tagparser/tag.h>

#include <QSize>
#include <QWidget>

#include <memory>

QT_FORWARD_DECLARE_CLASS(QEvent)
QT_FORWARD_DECLARE_CLASS(QGraphicsScene)
QT_FORWARD_DECLARE_CLASS(QGraphicsTextItem)
QT_FORWARD_DECLARE_CLASS(QGraphicsPixmapItem)
QT_FORWARD_DECLARE_CLASS(QGraphicsRectItem)

#define TAGEDITOR_ENUM_CLASS enum class
namespace TagParser {
class Tag;
class TagValue;
TAGEDITOR_ENUM_CLASS KnownField : unsigned int;
} // namespace TagParser

namespace QtGui {

TAGEDITOR_ENUM_CLASS PreviousValueHandling : int;
#undef TAGEDITOR_ENUM_CLASS

namespace Ui {
class PicturePreviewSelection;
class ImageConversionDialog;
} // namespace Ui

class PicturePreviewSelection : public QWidget {
    Q_OBJECT

public:
    explicit PicturePreviewSelection(
        TagParser::Tag *tag = nullptr, TagParser::KnownField field = TagParser::KnownField::Invalid, QWidget *parent = nullptr);
    ~PicturePreviewSelection() override;

    TagParser::Tag *tag() const;
    TagParser::KnownField field() const;

public Q_SLOTS:
    bool setTagField(TagParser::Tag *tag, TagParser::KnownField field, PreviousValueHandling previousValueHandling = PreviousValueHandling::Clear);
    bool setValue(const TagParser::TagValue &value, PreviousValueHandling previousValueHandling = PreviousValueHandling::Clear);

    void apply();
    void clear();
    void addOfSelectedType();
    void addOfSelectedType(const QString &path);
#ifndef QT_NO_CLIPBOARD
    void pasteOfSelectedTypeAsJpeg();
    void pasteOfSelectedTypeAsPng();
    void pasteOfSelectedType(const char *format);
#endif
    void removeSelected();
    void extractSelected();
    void displaySelected();
    void changeMimeTypeOfSelected();
    void convertSelected();
    void setCoverButtonsHidden(bool hideCoverButtons);

signals:
    void pictureChanged();

protected:
    void changeEvent(QEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;
    void dragEnterEvent(QDragEnterEvent *event) override;
    void dropEvent(QDropEvent *event) override;

private Q_SLOTS:
    void typeSwitched(int index);
    void updateDescription(int newIndex);
    void updateDescription(int lastIndex, int newIndex);
    void updatePreview(int index);
    void showContextMenu(const QPoint &position);

private:
    bool setup(PreviousValueHandling previousValueHandling = PreviousValueHandling::Clear);
    void updateSizeAndMimeType(std::size_t fileSize, const QSize &resolution, const QString &mimeType);
    QImage convertTagValueToImage(const TagParser::TagValue &value);
    void assignImageToTagValue(const QImage &image, TagParser::TagValue &tagValue, const char *format);

    std::unique_ptr<Ui::PicturePreviewSelection> m_ui;
    std::unique_ptr<Ui::ImageConversionDialog> m_imageConversionUI;
    QDialog *m_imageConversionDialog;
    QGraphicsScene *m_scene;
    QGraphicsTextItem *m_textItem;
    QPixmap m_pixmap;
    QGraphicsPixmapItem *m_pixmapItem;
    QGraphicsRectItem *m_rectItem;
    TagParser::Tag *m_tag;
    TagParser::KnownField m_field;
    QList<TagParser::TagValue> m_values;
    std::size_t m_currentFileSize;
    QSize m_currentResolution;
    QString m_currentMimeType;
    int m_currentTypeIndex;
};

/*!
 * \brief Returns the tied tag if one is assigned; returns nullptr otherwise.
 *
 * This might have been set initially using the constructor or using the setTagField() method.
 */
inline TagParser::Tag *PicturePreviewSelection::tag() const
{
    return m_tag;
}

/*!
 * \brief Returns the field. This is commonly KnownField::Cover.
 */
inline TagParser::KnownField PicturePreviewSelection::field() const
{
    return m_field;
}

/*!
 * \brief Sets the specified \a tag, \a field and \a previousValueHandling.
 *
 * If \a tag is nullptr the widget is disabled. The widget will be re-setup.
 */
inline bool PicturePreviewSelection::setTagField(TagParser::Tag *tag, TagParser::KnownField field, PreviousValueHandling previousValueHandling)
{
    m_tag = tag;
    m_field = field;
    return setup(previousValueHandling);
}

} // namespace QtGui

#endif // COVEREDIT_H
