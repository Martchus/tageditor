#include "./picturepreviewselection.h"

#include "../application/settings.h"
#include "../misc/utility.h"

#include "ui_picturepreviewselection.h"

#include <tagparser/mediafileinfo.h>
#include <tagparser/tag.h>
#include <tagparser/id3/id3v2tag.h>
#include <tagparser/id3/id3v2frame.h>
#include <tagparser/vorbis/vorbiscomment.h>
#include <tagparser/vorbis/vorbiscommentfield.h>

#include <qtutilities/misc/conversion.h>

#include <c++utilities/io/catchiofailure.h>

#include <QEvent>
#include <QGraphicsScene>
#include <QGraphicsTextItem>
#include <QGraphicsRectItem>
#include <QFile>
#include <QImage>
#include <QFileDialog>
#include <QInputDialog>
#include <QMessageBox>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QMimeData>
#include <QCursor>
#include <QMenu>
#include <QAction>

#include <stdexcept>
#include <functional>
#include <cassert>
#include <memory>

using namespace std;
using namespace Media;
using namespace ConversionUtilities;

namespace QtGui {

/*!
 * \brief Constructs a new PicturePreviewSelection for the specified \a tag and \a field.
 */
PicturePreviewSelection::PicturePreviewSelection(Tag *tag, KnownField field, QWidget *parent) :
    QWidget(parent),
    m_ui(new Ui::PicturePreviewSelection),
    m_scene(nullptr),
    m_textItem(nullptr),
    m_pixmapItem(nullptr),
    m_rectItem(nullptr),
    m_tag(tag),
    m_field(field),
    m_currentTypeIndex(0)
{
    m_ui->setupUi(this);
    m_ui->coverButtonsWidget->setHidden(Settings::values().editor.hideCoverButtons);
    connect(m_ui->addButton, &QPushButton::clicked, this, static_cast<void (PicturePreviewSelection::*)(void)>(&PicturePreviewSelection::addOfSelectedType));
    connect(m_ui->removeButton, &QPushButton::clicked, this, &PicturePreviewSelection::removeSelected);
    connect(m_ui->extractButton, &QPushButton::clicked, this, &PicturePreviewSelection::extractSelected);
    connect(m_ui->displayButton, &QPushButton::clicked, this, &PicturePreviewSelection::displaySelected);
    connect(m_ui->restoreButton, &QPushButton::clicked, std::bind(&PicturePreviewSelection::setup, this, PreviousValueHandling::Clear));
    connect(m_ui->previewGraphicsView, &QGraphicsView::customContextMenuRequested, this, &PicturePreviewSelection::showContextMenu);
    setup();
    setAcceptDrops(true);
}

/*!
 * \brief Destroys the instance.
 */
PicturePreviewSelection::~PicturePreviewSelection()
{}

/*!
 * \brief Sets the \a value of the current tag field manually using the given \a previousValueHandling.
 */
void PicturePreviewSelection::setValue(const TagValue &value, PreviousValueHandling previousValueHandling)
{
    assert(m_currentTypeIndex < m_values.size());
    TagValue &currentValue = m_values[m_currentTypeIndex];
    if(previousValueHandling == PreviousValueHandling::Clear || !value.isEmpty()) {
        if(previousValueHandling != PreviousValueHandling::Keep || currentValue.isEmpty()) {
            currentValue = value; // TODO: move(value);
            emit pictureChanged();
        }
    }
    updatePreview(m_currentTypeIndex);
}

/*!
 * \brief Defines the predicate to get relevant fields.
 */
template<class TagType>
bool fieldPredicate(int i, const std::pair<typename TagType::IdentifierType, typename TagType::FieldType> &pair)
{
    return pair.second.isTypeInfoAssigned() ? (pair.second.typeInfo() == static_cast<typename TagType::FieldType::TypeInfoType>(i)) : (i == 0);
}

/*!
 * \brief Fetches the ID3v2 cover values from the specified \a tag.
 * \param tag Specifies a tag to get the values from.
 * \param field Specifies the field.
 * \param values Specifies a list to push the fetched values in.
 * \param valueCount Specifies the number of cover types.
 * \param previousValueHandling Specifies the "previous value handling policy".
 */
template<class TagType>
int fetchId3v2CoverValues(const TagType *tag, KnownField field, QList<Media::TagValue> &values, const int valueCount, const PreviousValueHandling previousValueHandling)
{
    values.reserve(valueCount);
    int first = -1;
    const auto &fields = tag->fields();
    auto range = fields.equal_range(tag->fieldId(field));
    for(int i = 0; i < valueCount; ++i) {
        if(i >= values.size()) {
            values << TagValue();
        }
        auto pair = find_if(range.first, range.second, std::bind(fieldPredicate<TagType>, i, placeholders::_1));
        if(pair != range.second) {
            const TagValue &value = pair->second.value();
            if((previousValueHandling == PreviousValueHandling::Clear || !value.isEmpty())
                    && (previousValueHandling != PreviousValueHandling::Keep || values[i].isEmpty())) {
                values[i] = value;
            }
        } else if(previousValueHandling == PreviousValueHandling::Clear) {
            values[i].clearDataAndMetadata();
        }
        if(first < 0 && !values[i].isEmpty()) {
            first = i;
        }
    }
    values.erase(values.begin() + valueCount, values.end());
    return first;
}

/*!
 * \brief Sets the widget up.
 */
void PicturePreviewSelection::setup(PreviousValueHandling previousValueHandling)
{
    if(previousValueHandling == PreviousValueHandling::Auto) {
        previousValueHandling = PreviousValueHandling::Update;
    }
    m_currentTypeIndex = 0;
    if(m_tag) {
        if(m_field == KnownField::Cover && (m_tag->type() == TagType::Id3v2Tag || m_tag->type() == TagType::VorbisComment)) {
            m_ui->switchTypeComboBox->setHidden(false);
            m_ui->switchTypeLabel->setHidden(false);
            if(!m_ui->switchTypeComboBox->count()) {
                m_ui->switchTypeComboBox->addItems(QStringList()
                                                   << tr("Other")
                                                   << tr("32x32 File icon")
                                                   << tr("Other file icon")
                                                   << tr("Cover (front)")
                                                   << tr("Cover (back)")
                                                   << tr("Leaflet page")
                                                   << tr("Media (e. g. label side of CD)")
                                                   << tr("Lead artist/performer/soloist")
                                                   << tr("Artist/performer")
                                                   << tr("Conductor")
                                                   << tr("Band/Orchestra")
                                                   << tr("Composer")
                                                   << tr("Lyricist/text writer")
                                                   << tr("Recording Location")
                                                   << tr("During recording")
                                                   << tr("During performance")
                                                   << tr("Movie/video screen capture")
                                                   << tr("A bright coloured fish")
                                                   << tr("Illustration")
                                                   << tr("Band/artist logotype")
                                                   << tr("Publisher/Studio logotype")
                                                   );
            }
            int first;
            switch(m_tag->type()) {
            case TagType::Id3v2Tag:
                first = fetchId3v2CoverValues(static_cast<Id3v2Tag *>(m_tag), m_field, m_values, m_ui->switchTypeComboBox->count(), previousValueHandling);
                break;
            case TagType::VorbisComment:
            case TagType::OggVorbisComment:
                first = fetchId3v2CoverValues(static_cast<VorbisComment *>(m_tag), m_field, m_values, m_ui->switchTypeComboBox->count(), previousValueHandling);
                break;
            default:
                first = 0;
            }
            if(first >= 0) {
                m_ui->switchTypeComboBox->setCurrentIndex(first);
            }
            m_currentTypeIndex = m_ui->switchTypeComboBox->currentIndex();
            connect(m_ui->switchTypeComboBox, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged), this, &PicturePreviewSelection::typeSwitched);
        } else {
            m_ui->switchTypeComboBox->setHidden(true);
            m_ui->switchTypeLabel->setHidden(true);
            m_currentTypeIndex = 0;
            const TagValue &value = m_tag->value(m_field);
            if(previousValueHandling == PreviousValueHandling::Clear || !value.isEmpty()) {
                if(m_values.size()) {
                    if(previousValueHandling != PreviousValueHandling::Keep || m_values[0].isEmpty()) {
                        m_values[0] = value;
                    }
                } else {
                    m_values << value;
                }
            }
            if(m_values.size()) {
                m_values.erase(m_values.begin() + 1, m_values.end());
            } else {
                m_values << TagValue();
            }
        }
        bool hideDesc = !m_tag->supportsDescription(m_field);
        m_ui->descriptionLineEdit->setHidden(hideDesc);
        m_ui->descriptionLabel->setHidden(hideDesc);
        updatePreview(m_currentTypeIndex);
        updateDescription(m_currentTypeIndex);
        setEnabled(true);
    } else {
        m_currentTypeIndex = 0;
        setEnabled(false);
    }
}

/*!
 * \brief Pushes the ID3v2 cover values to the specified \a tag.
 * \param tag Specifies a tag to push the values to.
 * \param field Specifies the field.
 * \param values Specifies the values to be pushed.
 */
template<class TagType>
void pushId3v2CoverValues(TagType *tag, KnownField field, const QList<Media::TagValue> &values)
{
    auto &fields = tag->fields();
    const auto id = tag->fieldId(field);
    const auto range = fields.equal_range(id);
    const auto first = range.first;
    // iterate through all tag values
    for(int index = 0, valueCount = values.size(); index < valueCount; ++index) {
        // check whether there is already a tag value with the current index/type
        auto pair = find_if(first, range.second, std::bind(fieldPredicate<TagType>, index, placeholders::_1));
        if(pair != range.second) {
            // there is already a tag value with the current index/type
            // -> update this value
            pair->second.setValue(values[index]);
            // check whether there are more values with the current index/type assigned
            while((pair = find_if(++pair, range.second, std::bind(fieldPredicate<TagType>, index, placeholders::_1))) != range.second) {
                // -> remove these values as we only support one value of a type in the same tag
                pair->second.setValue(TagValue());
            }
        } else if(!values[index].isEmpty()) {
            typename TagType::FieldType field(id, values[index]);
            field.setTypeInfo(index);
            fields.insert(std::make_pair(id, field));
        }
    }
}

/*!
 * \brief Applies the selection to the tag.
 */
void PicturePreviewSelection::apply()
{
    if(m_tag) {
        if(m_field == KnownField::Cover && (m_tag->type() == TagType::Id3v2Tag || m_tag->type() == TagType::VorbisComment)) {
            switch(m_tag->type()) {
            case TagType::Id3v2Tag:
                pushId3v2CoverValues(static_cast<Id3v2Tag *>(m_tag), m_field, m_values);
                break;
            case TagType::VorbisComment:
                pushId3v2CoverValues(static_cast<VorbisComment *>(m_tag), m_field, m_values);
                break;
            default:
                ;
            }
        } else {
            if(m_values.size()) {
                m_tag->setValue(m_field, m_values.first());
            } else {
                m_tag->setValue(m_field, TagValue());
            }
        }
    }
}

/*!
 * \brief Clears all selected pictures (without applying the change to the tag).
 */
void PicturePreviewSelection::clear()
{
    for(int i = 0, count = m_values.count(); i < count; ++i) {
        m_values[i].clearDataAndMetadata();
    }
    updatePreview(m_currentTypeIndex);
    updateDescription(m_currentTypeIndex);
}

/*!
 * \brief Asks the user to add a picture of the selected type.
 */
void PicturePreviewSelection::addOfSelectedType()
{
    assert(m_currentTypeIndex < m_values.size());
    QString path = QFileDialog::getOpenFileName(this, tr("Select a picture to add as cover"));
    if(!path.isEmpty()) {
        addOfSelectedType(path);
    }
}

/*!
 * \brief Adds the picture from the specified \a path of the selected type.
 */
void PicturePreviewSelection::addOfSelectedType(const QString &path)
{
    assert(m_currentTypeIndex < m_values.size());
    TagValue &selectedCover = m_values[m_currentTypeIndex];
    try {
        MediaFileInfo fileInfo(toNativeFileName(path).constData());
        fileInfo.open(true);
        fileInfo.parseContainerFormat();
        auto mimeType = QString::fromUtf8(fileInfo.mimeType());
        bool ok;
        mimeType = QInputDialog::getText(this, tr("Enter/confirm mime type"), tr("Confirm or enter the mime type of the selected file."), QLineEdit::Normal, mimeType, &ok);
        if(ok) {
            if((fileInfo.size() < 10485760)
                    || (QMessageBox::warning(this, QApplication::applicationName(), tr("The selected file is very large (for a cover). Do you want to continue?"), QMessageBox::Yes, QMessageBox::No) == QMessageBox::No)) {
                auto buff = make_unique<char []>(fileInfo.size());
                fileInfo.stream().seekg(0);
                fileInfo.stream().read(buff.get(), fileInfo.size());
                selectedCover.assignData(std::move(buff), fileInfo.size(), TagDataType::Picture);
                selectedCover.setMimeType(mimeType.toUtf8().constData());
                emit pictureChanged();
            }
        }
    } catch (const Media::Failure &) {
        QMessageBox::critical(this, QApplication::applicationName(), tr("Unable to parse specified cover file."));
    } catch(...) {
        ::IoUtilities::catchIoFailure();
        QMessageBox::critical(this, QApplication::applicationName(), tr("An IO error occured when parsing the specified cover file."));
    }
    updatePreview(m_currentTypeIndex);
}

/*!
 * \brief Removes the selected picture.
 */
void PicturePreviewSelection::removeSelected()
{
    if(m_currentTypeIndex < m_values.size()) {
        if(m_values[m_currentTypeIndex].isEmpty()) {
            QMessageBox::information(this, QApplication::applicationName(), tr("There is no cover to remove."));
        } else {
            m_values[m_currentTypeIndex].clearData();
            updatePreview(m_currentTypeIndex);
            emit pictureChanged();
        }
    } else {
        throw logic_error("Invalid type selected (no corresponding value assigned).");
    }
}

/*!
 * \brief Extracts the selected picture.
 */
void PicturePreviewSelection::extractSelected()
{
    assert(m_currentTypeIndex < m_values.size());
    TagValue &value = m_values[m_currentTypeIndex];
    if(value.isEmpty()) {
        QMessageBox::information(this, QApplication::applicationName(), tr("There is no image attached to be extracted."));
    } else {
        const auto path = QFileDialog::getSaveFileName(this, tr("Where do you want to save the cover?"));
        if(!path.isEmpty()) {
            QFile file(path);
            if(file.open(QIODevice::WriteOnly)) {
                if(file.write(value.dataPointer(), value.dataSize()) > 0) {
                    QMessageBox::information(this, QApplication::applicationName(), tr("The cover has extracted."));
                } else {
                    QMessageBox::warning(this, QApplication::applicationName(), tr("Unable to write to output file."));
                }
                file.close();
            } else {
                QMessageBox::warning(this, QApplication::applicationName(), tr("Unable to open output file."));
            }
        }
    }
}

/*!
 * \brief Displays the selected picture in a modal dialog.
 */
void PicturePreviewSelection::displaySelected()
{
    assert(m_currentTypeIndex < m_values.size());
    TagValue &value = m_values[m_currentTypeIndex];
    if(!value.isEmpty()) {
        QImage img;
        if(value.mimeType() == "-->") {
            QFile file(Utility::stringToQString(value.toString(), value.dataEncoding()));
            if(file.open(QFile::ReadOnly)) {
                img = QImage::fromData(file.readAll());
            } else {
                QMessageBox::warning(this, QApplication::applicationName(), tr("The attached image can't be found."));
                return;
            }
        } else {
            img = QImage::fromData(reinterpret_cast<const uchar *>(value.dataPointer()), value.dataSize());
        }
        if(img.isNull()) {
            QMessageBox::warning(this, QApplication::applicationName(), tr("The attached image can't be displayed."));
        } else {
            QDialog dlg;
            dlg.setWindowFlags(Qt::Tool);
            dlg.setWindowTitle(tr("Cover - %1").arg(QApplication::applicationName()));
            QBoxLayout layout(QBoxLayout::Up);
            layout.setMargin(0);
            QGraphicsView view(&dlg);
            QGraphicsScene scene;
            layout.addWidget(&view);
            scene.addItem(new QGraphicsPixmapItem(QPixmap::fromImage(img)));
            view.setScene(&scene);
            view.show();
            dlg.setLayout(&layout);
            dlg.exec();
        }
    } else {
        QMessageBox::warning(this, QApplication::applicationName(), tr("There is no image attached."));
    }
}

/*!
 * \brief Asks the user to alter the MIME-type of the selected cover.
 */
void PicturePreviewSelection::changeMimeTypeOfSelected()
{
    assert(m_currentTypeIndex < m_values.size());
    TagValue &selectedCover = m_values[m_currentTypeIndex];
    auto mimeType = QString::fromUtf8(selectedCover.mimeType().data());
    bool ok;
    mimeType = QInputDialog::getText(this, tr("Enter/confirm mime type"), tr("Confirm or enter the mime type of the selected file."), QLineEdit::Normal, mimeType, &ok);
    if(ok) {
        selectedCover.setMimeType(mimeType.toUtf8().data());
    }

}

/*!
 * \brief Sets whether cover buttons are hidden.
 */
void PicturePreviewSelection::setCoverButtonsHidden(bool hideCoverButtons)
{
    m_ui->coverButtonsWidget->setHidden(hideCoverButtons);
}

void PicturePreviewSelection::changeEvent(QEvent *event)
{
    switch(event->type()) {
    case QEvent::EnabledChange:
        if(m_rectItem) {
            m_rectItem->setVisible(!isEnabled());
        }
        break;
    default:
        ;
    }
}

void PicturePreviewSelection::resizeEvent(QResizeEvent *)
{
    if(m_pixmapItem && !m_pixmap.isNull()) {
        m_pixmapItem->setPixmap(m_pixmap.scaled(m_ui->previewGraphicsView->size(), Qt::KeepAspectRatio));
    }
}

void PicturePreviewSelection::dragEnterEvent(QDragEnterEvent *event)
{
    const auto *mimeData = event->mimeData();
    if(mimeData->hasUrls()) {
        for(const auto &url : mimeData->urls()) {
            if(url.scheme() == QLatin1String("file")) {
                event->accept();
                return;
            }
        }
    }
    event->ignore();
}

void PicturePreviewSelection::dropEvent(QDropEvent *event)
{
    const auto *mimeData = event->mimeData();
    if(mimeData->hasUrls()) {
        for(const auto &url : mimeData->urls()) {
            if(url.scheme() == QLatin1String("file")) {
#ifdef Q_OS_WIN32
                // remove leading slash
                QString path = url.path();
                int index = 0;
                for(const auto &c : path) {
                    if(c == QChar('/')) {
                        ++index;
                    } else {
                        break;
                    }
                }
                if(index) {
                    path = path.mid(index);
                }
                addOfSelectedType(path);
#else
                addOfSelectedType(url.path());
#endif
                event->accept();
                return;
            }
        }
    }
    event->ignore();
}

/*!
 * \brief Handles selected type being switched.
 */
void PicturePreviewSelection::typeSwitched(int index)
{
    assert(m_currentTypeIndex < m_values.size());
    int lastIndex = m_currentTypeIndex;
    if(index < 0 || index >= m_values.size()) {
        throw logic_error("current type index is invalid");
    } else {
        m_currentTypeIndex = index;
    }
    updateDescription(lastIndex, index);
    updatePreview(index);
}

/*!
 * \brief Shows the description of type with the specified \a newIndex.
 */
void PicturePreviewSelection::updateDescription(int newIndex)
{
    // show description of selected type
    m_ui->descriptionLineEdit->setText(Utility::stringToQString(m_values[newIndex].description(), m_values[newIndex].descriptionEncoding()));
}

/*!
 * \brief Shows the description of type with the specified \a newIndex and saves the description of the type with the specified \a lastIndex before.
 */
void PicturePreviewSelection::updateDescription(int lastIndex, int newIndex)
{
    TagTextEncoding enc;
    if(m_tag) {
        TagTextEncoding preferredEncoding = Settings::values().tagPocessing.preferredEncoding;
        enc = m_tag->canEncodingBeUsed(preferredEncoding) ? preferredEncoding : m_tag->proposedTextEncoding();
    } else {
        enc = m_values[lastIndex].descriptionEncoding();
    }
    // save entered description
    m_values[lastIndex].setDescription(Utility::qstringToString(m_ui->descriptionLineEdit->text(), enc), enc);
    // show description of selected type
    updateDescription(newIndex);
}

/*!
 * \brief Updates the preview to show the type with the specified \a index.
 */
void PicturePreviewSelection::updatePreview(int index)
{
    if(!m_scene) {
        m_scene = new QGraphicsScene(m_ui->previewGraphicsView);
        m_ui->previewGraphicsView->setScene(m_scene);
    }
    if(!m_textItem) {
        m_textItem = new QGraphicsTextItem;
        m_textItem->setTextWidth(m_ui->previewGraphicsView->width());
        m_scene->addItem(m_textItem);
    }
    if(!m_pixmapItem) {
        m_pixmapItem = new QGraphicsPixmapItem;
        m_scene->addItem(m_pixmapItem);
    }
    if(!m_rectItem) {
        m_rectItem = new QGraphicsRectItem;
        m_rectItem->setPen(Qt::NoPen);
        m_rectItem->setBrush(QBrush(QColor(120, 120, 120, 120)));
        m_rectItem->setVisible(!isEnabled());
        m_scene->addItem(m_rectItem);
    }
    TagValue &value = m_values[index];
    if(value.isEmpty()) {
        m_textItem->setVisible(true);
        m_textItem->setPlainText(tr("No image (of the selected type) attached."));
        m_pixmapItem->setVisible(false);
        m_ui->addButton->setText(tr("Add"));
    } else {
        QImage img;
        if(value.mimeType() == "-->") {
            QFile file(Utility::stringToQString(value.toString(), value.dataEncoding()));
            if(file.open(QFile::ReadOnly))
                img = QImage::fromData(file.readAll());
            else {
                m_textItem->setPlainText(tr("The attached image can't be found."));
                m_textItem->setVisible(true);
                m_pixmapItem->setVisible(false);
                return;
            }
        } else {
            img = QImage::fromData(reinterpret_cast<const uchar *>(value.dataPointer()), value.dataSize());
        }
        if(img.isNull()) {
            m_pixmap = QPixmap();
            m_textItem->setPlainText(tr("Unable to display attached image."));
            m_textItem->setVisible(true);
            m_pixmapItem->setVisible(false);
        } else {
            m_textItem->setVisible(false);
            m_pixmap = QPixmap::fromImage(img);
            m_pixmapItem->setPixmap(m_pixmap.scaled(m_ui->previewGraphicsView->size(), Qt::KeepAspectRatio));
            m_pixmapItem->setVisible(true);
        }
        m_ui->addButton->setText(tr("Change"));
    }
    m_rectItem->setRect(0, 0, m_ui->previewGraphicsView->width(), m_ui->previewGraphicsView->height());
}

void PicturePreviewSelection::showContextMenu()
{
    QMenu menu;
    QAction *addAction = menu.addAction(m_ui->addButton->text());
    addAction->setIcon(QIcon::fromTheme(QStringLiteral("list-add")));
    connect(addAction, &QAction::triggered, this, static_cast<void (PicturePreviewSelection::*)(void)>(&PicturePreviewSelection::addOfSelectedType));
    if(m_ui->extractButton->isEnabled()) {
        QAction *mimeAction = menu.addAction(tr("Change MIME-type"));
        mimeAction->setIcon(QIcon::fromTheme(QStringLiteral("document-properties")));
        connect(mimeAction, &QAction::triggered, this, &PicturePreviewSelection::changeMimeTypeOfSelected);
    }
    menu.addSeparator();
    if(m_ui->removeButton->isEnabled()) {
        QAction *removeAction = menu.addAction(m_ui->removeButton->text());
        removeAction->setIcon(QIcon::fromTheme(QStringLiteral("edit-delete")));
        connect(removeAction, &QAction::triggered, this, &PicturePreviewSelection::removeSelected);
    }
    if(m_ui->restoreButton->isEnabled()) {
        QAction *restoreAction = menu.addAction(m_ui->restoreButton->text());
        restoreAction->setIcon(QIcon::fromTheme(QStringLiteral("document-revert")));
        connect(restoreAction, &QAction::triggered, std::bind(&PicturePreviewSelection::setup, this, PreviousValueHandling::Clear));
    }
    menu.addSeparator();
    if(m_ui->extractButton->isEnabled()) {
        QAction *extractAction = menu.addAction(m_ui->extractButton->text());
        extractAction->setIcon(QIcon::fromTheme(QStringLiteral("document-save")));
        connect(extractAction, &QAction::triggered, this, &PicturePreviewSelection::extractSelected);
    }
    if(m_ui->displayButton->isEnabled()) {
        QAction *displayAction = menu.addAction(m_ui->displayButton->text());
        displayAction->setIcon(QIcon::fromTheme(QStringLiteral("image-x-generic")));
        connect(displayAction, &QAction::triggered, this, &PicturePreviewSelection::displaySelected);
    }
    menu.exec(QCursor::pos());
}

}
