#include "./picturepreviewselection.h"

#include "../application/settings.h"
#include "../misc/utility.h"

#include "ui_imageconversiondialog.h"
#include "ui_picturepreviewselection.h"

#include <tagparser/diagnostics.h>
#include <tagparser/id3/id3v2frame.h>
#include <tagparser/id3/id3v2tag.h>
#include <tagparser/mediafileinfo.h>
#include <tagparser/progressfeedback.h>
#include <tagparser/tag.h>
#include <tagparser/vorbis/vorbiscomment.h>
#include <tagparser/vorbis/vorbiscommentfield.h>

#include <qtutilities/misc/conversion.h>
#include <qtutilities/misc/dialogutils.h>

#include <c++utilities/conversion/stringconversion.h>
#include <c++utilities/misc/traits.h>

#include <QAction>
#include <QCoreApplication>
#include <QCursor>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QEvent>
#include <QFile>
#include <QFileDialog>
#include <QGraphicsRectItem>
#include <QGraphicsScene>
#include <QGraphicsTextItem>
#include <QImage>
#include <QInputDialog>
#include <QMenu>
#include <QMessageBox>
#include <QMimeData>
#include <QStringBuilder>
#ifndef QT_NO_CLIPBOARD
#include <QBuffer>
#include <QClipboard>
#endif

#include <cassert>
#include <functional>
#include <limits>
#include <memory>
#include <stdexcept>

using namespace std;
using namespace TagParser;
using namespace CppUtilities;
using namespace QtUtilities;

namespace QtGui {

/*!
 * \brief Constructs a new PicturePreviewSelection for the specified \a tag and \a field.
 */
PicturePreviewSelection::PicturePreviewSelection(Tag *tag, KnownField field, QWidget *parent)
    : QWidget(parent)
    , m_ui(new Ui::PicturePreviewSelection)
    , m_imageConversionDialog(nullptr)
    , m_scene(nullptr)
    , m_textItem(nullptr)
    , m_pixmapItem(nullptr)
    , m_rectItem(nullptr)
    , m_tag(tag)
    , m_field(field)
    , m_currentFileSize(0)
    , m_currentTypeIndex(0)
{
    m_ui->setupUi(this);
    m_ui->coverButtonsWidget->setHidden(Settings::values().editor.hideCoverButtons);
    connect(m_ui->addButton, &QPushButton::clicked, this,
        static_cast<void (PicturePreviewSelection::*)(void)>(&PicturePreviewSelection::addOfSelectedType));
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
{
}

/*!
 * \brief Sets the \a value of the current tag field manually using the given \a previousValueHandling.
 *
 * Used for editing tags programmatically, eg. in TagEditorWidget::insertTitleFromFilename() and DbQueryWidget::applyResults().
 */
bool PicturePreviewSelection::setValue(const TagValue &value, PreviousValueHandling previousValueHandling)
{
    assert(m_currentTypeIndex < m_values.size());
    TagValue &currentValue = m_values[m_currentTypeIndex];
    bool updated = false;
    if ((previousValueHandling == PreviousValueHandling::Clear || !value.isEmpty())
        && (previousValueHandling != PreviousValueHandling::Keep || currentValue.isEmpty())) {
        currentValue = value; // TODO: move(value);
        updated = true;
        emit pictureChanged();
    }
    updatePreview(m_currentTypeIndex);
    return updated;
}

/*!
 * \brief Defines the predicate to get relevant fields.
 */
template <class TagType> bool fieldPredicate(int i, const std::pair<typename TagType::IdentifierType, typename TagType::FieldType> &pair)
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
template <class TagType>
int fetchId3v2CoverValues(const TagType *tag, KnownField field, std::vector<TagParser::TagValue> &values, const std::size_t valueCount,
    const PreviousValueHandling previousValueHandling)
{
    values.reserve(valueCount);
    int first = -1;
    const auto &fields = tag->fields();
    auto range = fields.equal_range(tag->fieldId(field));
    for (std::size_t i = 0; i < valueCount; ++i) {
        if (i >= values.size()) {
            values.emplace_back();
        }
        auto pair = find_if(range.first, range.second, std::bind(fieldPredicate<TagType>, i, placeholders::_1));
        if (pair != range.second) {
            const TagValue &value = pair->second.value();
            if ((previousValueHandling == PreviousValueHandling::Clear || !value.isEmpty())
                && (previousValueHandling != PreviousValueHandling::Keep || values[i].isEmpty())) {
                values[i] = value;
            }
        } else if (previousValueHandling == PreviousValueHandling::Clear) {
            values[i].clearDataAndMetadata();
        }
        if (first < 0 && !values[i].isEmpty()) {
            first = static_cast<int>(i);
        }
    }
    values.erase(values.begin() + static_cast<typename std::remove_reference_t<decltype(values)>::difference_type>(valueCount), values.end());
    return first;
}

/*!
 * \brief Sets the widget up.
 */
bool PicturePreviewSelection::setup(PreviousValueHandling previousValueHandling)
{
    if (previousValueHandling == PreviousValueHandling::Auto) {
        previousValueHandling = PreviousValueHandling::Update;
    }
    m_currentTypeIndex = 0;
    if (!m_tag) {
        setEnabled(false);
        return false;
    }
    if (m_field == KnownField::Cover && (m_tag->type() == TagType::Id3v2Tag || m_tag->type() == TagType::VorbisComment)) {
        m_ui->switchTypeComboBox->setHidden(false);
        m_ui->switchTypeLabel->setHidden(false);
        if (!m_ui->switchTypeComboBox->count()) {
            m_ui->switchTypeComboBox->addItems(QStringList({ tr("Other"), tr("32x32 File icon"), tr("Other file icon"), tr("Cover (front)"),
                tr("Cover (back)"), tr("Leaflet page"), tr("Media (e. g. label side of CD)"), tr("Lead artist/performer/soloist"),
                tr("Artist/performer"), tr("Conductor"), tr("Band/Orchestra"), tr("Composer"), tr("Lyricist/text writer"), tr("Recording Location"),
                tr("During recording"), tr("During performance"), tr("Movie/video screen capture"), tr("A bright coloured fish"), tr("Illustration"),
                tr("Band/artist logotype"), tr("Publisher/Studio logotype") }));
        }
        int first;
        switch (m_tag->type()) {
        case TagType::Id3v2Tag:
            first = fetchId3v2CoverValues(static_cast<Id3v2Tag *>(m_tag), m_field, m_values,
                static_cast<std::size_t>(m_ui->switchTypeComboBox->count()), previousValueHandling);
            break;
        case TagType::VorbisComment:
        case TagType::OggVorbisComment:
            first = fetchId3v2CoverValues(static_cast<VorbisComment *>(m_tag), m_field, m_values,
                static_cast<std::size_t>(m_ui->switchTypeComboBox->count()), previousValueHandling);
            break;
        default:
            first = 0;
        }
        if (first >= 0) {
            m_ui->switchTypeComboBox->setCurrentIndex(first);
        }
        m_currentTypeIndex = static_cast<std::size_t>(m_ui->switchTypeComboBox->currentIndex());
        connect(m_ui->switchTypeComboBox, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged), this,
            &PicturePreviewSelection::typeSwitched);
    } else {
        m_ui->switchTypeComboBox->setHidden(true);
        m_ui->switchTypeLabel->setHidden(true);
        m_currentTypeIndex = 0;
        const TagValue &value = m_tag->value(m_field);
        if (previousValueHandling == PreviousValueHandling::Clear || !value.isEmpty()) {
            if (m_values.size()) {
                if (previousValueHandling != PreviousValueHandling::Keep || m_values[0].isEmpty()) {
                    m_values[0] = value;
                }
            } else {
                m_values.emplace_back(value);
            }
        }
        if (m_values.size()) {
            m_values.erase(m_values.begin() + 1, m_values.end());
        } else {
            m_values.emplace_back();
        }
    }
    const auto hideDesc = !m_tag->supportsDescription(m_field);
    m_ui->descriptionLineEdit->setHidden(hideDesc);
    m_ui->descriptionLabel->setHidden(hideDesc);
    updatePreview(m_currentTypeIndex);
    updateDescription(m_currentTypeIndex);
    setEnabled(true);
    return true;
}

/*!
 * \brief Updates the tooltip to show the current file size and MIME-type.
 */
void PicturePreviewSelection::updateSizeAndMimeType(size_t fileSize, const QSize &resolution, const QString &mimeType)
{
    QStringList info;
    info.reserve(3);
    if (fileSize) {
        info << QString::fromStdString(dataSizeToString(fileSize));
    }
    if (!resolution.isEmpty()) {
        info << QString::number(resolution.width()) % QStringLiteral(" x ") % QString::number(resolution.height()) % QStringLiteral(" px");
    }
    if (!mimeType.isEmpty()) {
        info << mimeType;
    }

    m_currentFileSize = fileSize;
    m_currentResolution = resolution;
    m_currentMimeType = mimeType;
    setToolTip(info.join(QChar('\n')));
}

/*!
 * \brief Assigns the specified \a image to the specified \a tagValue using the specified \a format.
 * \remarks Shows a message box if an error occurs and returns a "null" QImage.
 */
QImage PicturePreviewSelection::convertTagValueToImage(const TagValue &value)
{
    QImage img;
    if (value.mimeType() == "-->") {
        const auto fileName(Utility::stringToQString(value.toString(), value.dataEncoding()));
        QFile file(fileName);
        if (file.open(QFile::ReadOnly)) {
            img = QImage::fromData(file.readAll());
        } else {
            QMessageBox::warning(this, QCoreApplication::applicationName(),
                tr("The attached image can't be found. It is supposed to be stored as external file \"%1\".").arg(fileName));
            return img;
        }
    } else if (value.dataSize() < numeric_limits<int>::max()) {
        img = QImage::fromData(reinterpret_cast<const uchar *>(value.dataPointer()), static_cast<int>(value.dataSize()));
    }
    if (img.isNull()) {
        QMessageBox::warning(this, QCoreApplication::applicationName(), tr("The attached image format is not supported."));
    }
    return img;
}

/*!
 * \brief Assigns the specified \a image to the specified \a tagValue using the specified \a format.
 * \remarks Shows a message box if an error occurs.
 */
void PicturePreviewSelection::assignImageToTagValue(const QImage &image, TagValue &tagValue, const char *format)
{
    // set default MIME type
    QString mimeType;
    if (strcmp(format, "JPEG") == 0) {
        mimeType = QStringLiteral("image/jpeg");
    } else if (strcmp(format, "PNG") == 0) {
        mimeType = QStringLiteral("image/png");
    }

    // save image to buffer
    QByteArray imageData;
    QBuffer buffer(&imageData);
    buffer.open(QIODevice::WriteOnly);
    if (!image.save(&buffer, format)) {
        QMessageBox::critical(this, QCoreApplication::applicationName(), tr("Unable to save image from clipboard."));
        return;
    }

    // ask for MIME type
    if (mimeType.isEmpty()) {
        bool ok;
        mimeType
            = QInputDialog::getText(this, tr("Enter MIME type"), tr("Enter the MIME type for the pasted image."), QLineEdit::Normal, mimeType, &ok);
        if (!ok) {
            return;
        }
    }

    // assign image
    const auto mimeTypeUtf8(mimeType.toUtf8());
    tagValue.assignData(imageData.data(), static_cast<size_t>(imageData.size()), TagDataType::Picture);
    tagValue.setMimeType(mimeTypeUtf8.constData());
}

/*!
 * \brief Pushes the ID3v2 cover values to the specified \a tag.
 * \param tag Specifies a tag to push the values to.
 * \param field Specifies the field.
 * \param values Specifies the values to be pushed.
 */
template <class TagType> void pushId3v2CoverValues(TagType *tag, KnownField field, const std::vector<TagParser::TagValue> &values)
{
    auto &fields = tag->fields();
    const auto id = tag->fieldId(field);
    const auto range = fields.equal_range(id);
    const auto first = range.first;

    for (std::size_t index = 0, valueCount = values.size(); index < valueCount; ++index) {
        // check whether there is already a tag value with the current index/type
        auto pair = find_if(first, range.second, std::bind(fieldPredicate<TagType>, index, placeholders::_1));
        if (pair != range.second) {
            // there is already a tag value with the current index/type
            // -> update this value
            pair->second.setValue(values[index]);
            // check whether there are more values with the current index/type assigned
            while ((pair = find_if(++pair, range.second, std::bind(fieldPredicate<TagType>, index, placeholders::_1))) != range.second) {
                // -> remove these values as we only support one value of a type in the same tag
                pair->second.setValue(TagValue());
            }
        } else if (!values[static_cast<std::size_t>(index)].isEmpty()) {
            using FieldType = typename TagType::FieldType;
            using TypeInfoType = typename FieldType::TypeInfoType;
            using IndexCompareType = typename Traits::Conditional<std::is_unsigned<TypeInfoType>, make_unsigned<decltype(index)>::type, TypeInfoType>;
            auto newField = FieldType(id, values[index]);
            if (static_cast<IndexCompareType>(index) < numeric_limits<TypeInfoType>::max()) {
                newField.setTypeInfo(static_cast<TypeInfoType>(index));
            }
            fields.insert(std::pair(id, std::move(newField)));
        }
    }
}

/*!
 * \brief Applies the selection to the tag.
 */
void PicturePreviewSelection::apply()
{
    if (!m_tag) {
        return;
    }
    if (m_field == KnownField::Cover && (m_tag->type() == TagType::Id3v2Tag || m_tag->type() == TagType::VorbisComment)) {
        switch (m_tag->type()) {
        case TagType::Id3v2Tag:
            pushId3v2CoverValues(static_cast<Id3v2Tag *>(m_tag), m_field, m_values);
            break;
        case TagType::VorbisComment:
            pushId3v2CoverValues(static_cast<VorbisComment *>(m_tag), m_field, m_values);
            break;
        default:;
        }
    } else {
        if (!m_values.empty()) {
            m_tag->setValue(m_field, m_values.front());
        } else {
            m_tag->setValue(m_field, TagValue());
        }
    }
}

/*!
 * \brief Clears all selected pictures (without applying the change to the tag).
 */
void PicturePreviewSelection::clear()
{
    for (auto &value : m_values) {
        value.clearDataAndMetadata();
    }
    updatePreview(m_currentTypeIndex);
    updateDescription(m_currentTypeIndex);
    updateSizeAndMimeType(0, QSize(), QString());
}

/*!
 * \brief Asks the user to add a picture of the selected type.
 */
void PicturePreviewSelection::addOfSelectedType()
{
    assert(m_currentTypeIndex < m_values.size());
    const auto path = QFileDialog::getOpenFileName(this, tr("Select a picture to add as cover"));
    if (!path.isEmpty()) {
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
        auto fileInfo = MediaFileInfo(std::string_view(toNativeFileName(path).constData()));
        auto diag = Diagnostics(); // FIXME: show diagnostic messages
        auto progress = AbortableProgressFeedback(); // FIXME: actually use the progress object
        fileInfo.open(true);
        fileInfo.parseContainerFormat(diag, progress);

        auto mimeType = qstringFromStdStringView(fileInfo.mimeType());
        bool ok;
        mimeType = QInputDialog::getText(
            this, tr("Enter/confirm MIME type"), tr("Confirm or enter the MIME type of the selected file."), QLineEdit::Normal, mimeType, &ok);
        if (!ok) {
            return;
        }

        if ((fileInfo.size() >= 10485760)
            && (QMessageBox::warning(this, QCoreApplication::applicationName(),
                    tr("The selected file is very large (for a cover). Do you want to continue?"), QMessageBox::Yes, QMessageBox::No)
                == QMessageBox::No)) {
            return;
        }

        const auto mimeTypeUtf8(mimeType.toUtf8());
        auto buff = make_unique<char[]>(fileInfo.size());
        fileInfo.stream().seekg(static_cast<streamsize>(fileInfo.containerOffset()));
        fileInfo.stream().read(buff.get(), static_cast<streamsize>(fileInfo.size()));
        selectedCover.assignData(std::move(buff), fileInfo.size(), TagDataType::Picture);
        selectedCover.setMimeType(mimeTypeUtf8.constData());
        emit pictureChanged();

    } catch (const TagParser::Failure &) {
        QMessageBox::critical(this, QCoreApplication::applicationName(), tr("Unable to parse specified cover file."));
    } catch (const std::ios_base::failure &) {
        QMessageBox::critical(this, QCoreApplication::applicationName(), tr("An IO error occurred when parsing the specified cover file."));
    }
    updatePreview(m_currentTypeIndex);
}

#ifndef QT_NO_CLIPBOARD
void PicturePreviewSelection::pasteOfSelectedTypeAsJpeg()
{
    pasteOfSelectedType("JPEG");
}

void PicturePreviewSelection::pasteOfSelectedTypeAsPng()
{
    pasteOfSelectedType("PNG");
}

void PicturePreviewSelection::pasteOfSelectedType(const char *format)
{
    // load image from clipboard
    auto *const clipboard = QGuiApplication::clipboard();
    const auto image = clipboard ? clipboard->image() : QImage();
    if (image.isNull()) {
        QMessageBox::critical(this, QCoreApplication::applicationName(), tr("Unable to load image from clipboard."));
        return;
    }

    // assign image
    assert(m_currentTypeIndex < m_values.size());
    assignImageToTagValue(image, m_values[m_currentTypeIndex], format);

    updatePreview(m_currentTypeIndex);
}
#endif

/*!
 * \brief Removes the selected picture.
 */
void PicturePreviewSelection::removeSelected()
{
    if (m_currentTypeIndex >= m_values.size()) {
        throw logic_error("Invalid type selected (no corresponding value assigned).");
    }
    if (m_values[m_currentTypeIndex].isEmpty()) {
        QMessageBox::information(this, QCoreApplication::applicationName(), tr("There is no cover to remove."));
    } else {
        m_values[m_currentTypeIndex].clearData();
        updatePreview(m_currentTypeIndex);
        emit pictureChanged();
    }
}

/*!
 * \brief Extracts the selected picture.
 */
void PicturePreviewSelection::extractSelected()
{
    assert(m_currentTypeIndex < m_values.size());
    TagValue &value = m_values[m_currentTypeIndex];
    if (value.isEmpty()) {
        QMessageBox::information(this, QCoreApplication::applicationName(), tr("There is no image attached to be extracted."));
        return;
    }

    const auto path = QFileDialog::getSaveFileName(this, tr("Where do you want to save the cover?"));
    if (path.isEmpty()) {
        return;
    }
    QFile file(path);
    if (!file.open(QIODevice::WriteOnly)) {
        QMessageBox::warning(this, QCoreApplication::applicationName(), tr("Unable to open output file."));
        return;
    }
    if (value.dataSize() <= static_cast<std::size_t>(numeric_limits<qint64>::max()) && file.write(value.dataPointer(), static_cast<qint64>(value.dataSize())) > 0) {
        QMessageBox::information(this, QCoreApplication::applicationName(), tr("The cover has extracted."));
    } else {
        QMessageBox::warning(this, QCoreApplication::applicationName(), tr("Unable to write to output file."));
    }
    file.close();
}

/*!
 * \brief Displays the selected picture in a modal dialog.
 */
void PicturePreviewSelection::displaySelected()
{
    assert(m_currentTypeIndex < m_values.size());
    const auto &value = m_values[m_currentTypeIndex];
    if (value.isEmpty()) {
        QMessageBox::warning(this, QCoreApplication::applicationName(), tr("There is no image attached."));
        return;
    }

    // load image
    const auto img = convertTagValueToImage(value);
    if (img.isNull()) {
        return;
    }

    // show dialog
    QDialog dlg;
    dlg.setWindowFlags(Qt::Tool);
    const auto additionalInfo = toolTip().replace(QLatin1String("\n"), QLatin1String(", "));
    const auto title = !additionalInfo.isEmpty() ? tr("Cover - %1 - %2").arg(additionalInfo, QCoreApplication::applicationName())
                                                 : tr("Cover - %1").arg(QCoreApplication::applicationName());
    dlg.setWindowTitle(title);
    QBoxLayout layout(QBoxLayout::Up);
    layout.setContentsMargins(0, 0, 0, 0);
    QGraphicsView view(&dlg);
    QGraphicsScene scene;
    layout.addWidget(&view);
    scene.addItem(new QGraphicsPixmapItem(QPixmap::fromImage(img)));
    view.setScene(&scene);
    view.show();
    dlg.setLayout(&layout);
    dlg.exec();
}

/*!
 * \brief Asks the user to alter the MIME-type of the selected cover.
 */
void PicturePreviewSelection::changeMimeTypeOfSelected()
{
    assert(m_currentTypeIndex < m_values.size());
    auto &selectedCover = m_values[m_currentTypeIndex];
    auto mimeType = QString::fromUtf8(selectedCover.mimeType().data());
    bool ok;
    mimeType = QInputDialog::getText(this, tr("Enter/confirm MIME type"),
        tr("Confirm or enter the MIME type of the selected file. This merely changes the <i>assumed</i> format. <i>No</i> image format conversion "
           "done."),
        QLineEdit::Normal, mimeType, &ok);
    if (!ok) {
        return;
    }
    selectedCover.setMimeType(mimeType.toUtf8().data());
    updateSizeAndMimeType(m_currentFileSize, m_currentResolution, mimeType);
}

void PicturePreviewSelection::convertSelected()
{
    assert(m_currentTypeIndex < m_values.size());
    auto &selectedCover = m_values[m_currentTypeIndex];

    // load image
    const auto img = convertTagValueToImage(selectedCover);
    if (img.isNull()) {
        return;
    }

    // show image conversion dialog
    if (!m_imageConversionDialog) {
        m_imageConversionDialog = new QDialog(this);
        m_imageConversionUI = make_unique<Ui::ImageConversionDialog>();
        m_imageConversionUI->setupUi(m_imageConversionDialog);
#ifdef Q_OS_WINDOWS
        m_imageConversionDialog->setStyleSheet(dialogStyleForPalette(palette()));
#endif
        m_imageConversionUI->formatComboBox->addItems({ tr("JPEG"), tr("PNG") });
        m_imageConversionUI->aspectRatioComboBox->addItems({ tr("Ignore"), tr("Keep"), tr("Keep by expanding") });
        m_imageConversionUI->aspectRatioComboBox->setCurrentIndex(1);
        connect(m_imageConversionUI->confirmPushButton, &QPushButton::clicked, m_imageConversionDialog, &QDialog::accept);
        connect(m_imageConversionUI->abortPushButton, &QPushButton::clicked, m_imageConversionDialog, &QDialog::reject);
    }
    m_imageConversionUI->widthSpinBox->setValue(img.width());
    m_imageConversionUI->heightSpinBox->setValue(img.height());
    if (m_imageConversionDialog->exec() != QDialog::Accepted) {
        return;
    }

    // scale image
    const auto scaledImg = img.scaled(m_imageConversionUI->widthSpinBox->value(), m_imageConversionUI->heightSpinBox->value(),
        static_cast<Qt::AspectRatioMode>(m_imageConversionUI->aspectRatioComboBox->currentIndex()), Qt::SmoothTransformation);
    if (scaledImg.isNull()) {
        QMessageBox::warning(this, QCoreApplication::applicationName(), tr("Unable to scale image."));
        return;
    }

    // assign image
    assignImageToTagValue(scaledImg, selectedCover, m_imageConversionUI->formatComboBox->currentIndex() == 0 ? "JPEG" : "PNG");

    updatePreview(m_currentTypeIndex);
}

/*!
 * \brief Sets whether cover buttons are hidden.
 */
void PicturePreviewSelection::setCoverButtonsHidden(bool hideCoverButtons)
{
    m_ui->coverButtonsWidget->setHidden(hideCoverButtons);
}

bool PicturePreviewSelection::event(QEvent *event)
{
    const auto res = QWidget::event(event);
#ifdef Q_OS_WINDOWS
    switch (event->type()) {
    case QEvent::PaletteChange:
        if (m_imageConversionDialog) {
            m_imageConversionDialog->setStyleSheet(dialogStyleForPalette(palette()));
        }
        break;
    default:;
    }
#endif
    return res;
}

void PicturePreviewSelection::changeEvent(QEvent *event)
{
    switch (event->type()) {
    case QEvent::EnabledChange:
        if (m_rectItem) {
            m_rectItem->setVisible(!isEnabled());
        }
        break;
    default:;
    }
}

void PicturePreviewSelection::resizeEvent(QResizeEvent *)
{
    if (m_pixmapItem && !m_pixmap.isNull()) {
        m_pixmapItem->setPixmap(m_pixmap.scaled(m_ui->previewGraphicsView->size(), Qt::KeepAspectRatio));
    }
}

void PicturePreviewSelection::dragEnterEvent(QDragEnterEvent *event)
{
    const auto *const mimeData = event->mimeData();
    if (mimeData->hasUrls()) {
        for (const auto &url : mimeData->urls()) {
            if (url.scheme() == QLatin1String("file")) {
                event->accept();
                return;
            }
        }
    }
    event->ignore();
}

void PicturePreviewSelection::dropEvent(QDropEvent *event)
{
    const auto *const mimeData = event->mimeData();
    if (mimeData->hasUrls()) {
        for (const auto &url : mimeData->urls()) {
            if (url.scheme() == QLatin1String("file")) {
#ifdef Q_OS_WIN32
                // remove leading slash
                QString path = url.path();
                int index = 0;
                for (const auto &c : path) {
                    if (c == QChar('/')) {
                        ++index;
                    } else {
                        break;
                    }
                }
                if (index) {
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
void PicturePreviewSelection::typeSwitched(std::size_t index)
{
    assert(m_currentTypeIndex < m_values.size());
    const auto lastIndex = m_currentTypeIndex;
    if (index >= m_values.size()) {
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
void PicturePreviewSelection::updateDescription(std::size_t newIndex)
{
    // show description of selected type
    m_ui->descriptionLineEdit->setText(Utility::stringToQString(m_values[newIndex].description(), m_values[newIndex].descriptionEncoding()));
}

/*!
 * \brief Shows the description of type with the specified \a newIndex and saves the description of the type with the specified \a lastIndex before.
 */
void PicturePreviewSelection::updateDescription(std::size_t lastIndex, std::size_t newIndex)
{
    TagTextEncoding enc;
    if (m_tag) {
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
void PicturePreviewSelection::updatePreview(std::size_t index)
{
    if (!m_scene) {
        m_scene = new QGraphicsScene(m_ui->previewGraphicsView);
        m_ui->previewGraphicsView->setScene(m_scene);
    }
    if (!m_textItem) {
        m_textItem = new QGraphicsTextItem;
        m_textItem->setTextWidth(m_ui->previewGraphicsView->width());
        m_scene->addItem(m_textItem);
    }
    if (!m_pixmapItem) {
        m_pixmapItem = new QGraphicsPixmapItem;
        m_scene->addItem(m_pixmapItem);
    }
    if (!m_rectItem) {
        m_rectItem = new QGraphicsRectItem;
        m_rectItem->setPen(Qt::NoPen);
        m_rectItem->setBrush(QBrush(QColor(120, 120, 120, 120)));
        m_rectItem->setVisible(!isEnabled());
        m_scene->addItem(m_rectItem);
    }
    const auto &value = m_values[index];
    if (value.isEmpty()) {
        m_textItem->setVisible(true);
        m_textItem->setPlainText(tr("No image (of the selected type) attached."));
        m_pixmapItem->setVisible(false);
        m_ui->addButton->setText(tr("Add"));
    } else {
        QImage img;
        if (value.mimeType() == "-->") {
            QFile file(Utility::stringToQString(value.toString(), value.dataEncoding()));
            if (file.open(QFile::ReadOnly)) {
                img = QImage::fromData(file.readAll());
                updateSizeAndMimeType(static_cast<std::size_t>(file.size()), img.size(), QString());
            } else {
                m_textItem->setPlainText(tr("The attached image can't be found."));
                m_textItem->setVisible(true);
                m_pixmapItem->setVisible(false);
                return;
            }
        } else if (value.dataSize() < numeric_limits<int>::max()) {
            img = QImage::fromData(reinterpret_cast<const uchar *>(value.dataPointer()), static_cast<int>(value.dataSize()));
            updateSizeAndMimeType(value.dataSize(), img.size(), QString::fromStdString(value.mimeType()));
        }
        if (img.isNull()) {
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

void PicturePreviewSelection::showContextMenu(const QPoint &position)
{
    QMenu menu;
    auto *const addAction = menu.addAction(m_ui->addButton->text());
    addAction->setIcon(QIcon::fromTheme(QStringLiteral("list-add")));
    connect(addAction, &QAction::triggered, this, static_cast<void (PicturePreviewSelection::*)(void)>(&PicturePreviewSelection::addOfSelectedType));
#ifndef QT_NO_CLIPBOARD
    const auto *const clipboard = QGuiApplication::clipboard();
    const auto *const mimeData = clipboard ? clipboard->mimeData() : nullptr;
    if (mimeData && mimeData->hasImage()) {
        auto *const pasteJpegAction = menu.addAction(tr("Paste from clipboard as JPEG"));
        pasteJpegAction->setIcon(QIcon::fromTheme(QStringLiteral("edit-paste")));
        connect(pasteJpegAction, &QAction::triggered, this,
            static_cast<void (PicturePreviewSelection::*)(void)>(&PicturePreviewSelection::pasteOfSelectedTypeAsJpeg));
        auto *const pastePngAction = menu.addAction(tr("Paste from clipboard as PNG"));
        pastePngAction->setIcon(QIcon::fromTheme(QStringLiteral("edit-paste")));
        connect(pastePngAction, &QAction::triggered, this,
            static_cast<void (PicturePreviewSelection::*)(void)>(&PicturePreviewSelection::pasteOfSelectedTypeAsPng));
    }
#endif
    if (m_ui->extractButton->isEnabled()) {
        auto *const mimeAction = menu.addAction(tr("Change MIME-type"));
        mimeAction->setIcon(QIcon::fromTheme(QStringLiteral("document-properties")));
        connect(mimeAction, &QAction::triggered, this, &PicturePreviewSelection::changeMimeTypeOfSelected);
        auto *const convertAction = menu.addAction(tr("Resize/convert assigned image"));
        convertAction->setIcon(QIcon::fromTheme(QStringLiteral("image-resize-symbolic")));
        connect(convertAction, &QAction::triggered, this, &PicturePreviewSelection::convertSelected);
    }
    menu.addSeparator();
    if (m_ui->removeButton->isEnabled()) {
        auto *const removeAction = menu.addAction(m_ui->removeButton->text());
        removeAction->setIcon(QIcon::fromTheme(QStringLiteral("edit-delete")));
        connect(removeAction, &QAction::triggered, this, &PicturePreviewSelection::removeSelected);
    }
    if (m_ui->restoreButton->isEnabled()) {
        auto *const restoreAction = menu.addAction(m_ui->restoreButton->text());
        restoreAction->setIcon(QIcon::fromTheme(QStringLiteral("document-revert")));
        connect(restoreAction, &QAction::triggered, std::bind(&PicturePreviewSelection::setup, this, PreviousValueHandling::Clear));
    }
    menu.addSeparator();
    if (m_ui->extractButton->isEnabled()) {
        auto *const extractAction = menu.addAction(m_ui->extractButton->text());
        extractAction->setIcon(QIcon::fromTheme(QStringLiteral("document-save")));
        connect(extractAction, &QAction::triggered, this, &PicturePreviewSelection::extractSelected);
    }
    if (m_ui->displayButton->isEnabled()) {
        auto *const displayAction = menu.addAction(m_ui->displayButton->text());
        displayAction->setIcon(QIcon::fromTheme(QStringLiteral("image-x-generic")));
        connect(displayAction, &QAction::triggered, this, &PicturePreviewSelection::displaySelected);
    }
    menu.exec(m_ui->previewGraphicsView->viewport()->mapToGlobal(position));
}

} // namespace QtGui
