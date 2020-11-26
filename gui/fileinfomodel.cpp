#include "./fileinfomodel.h"
#include "../misc/utility.h"

#include <qtutilities/misc/conversion.h>

#include <tagparser/abstractattachment.h>
#include <tagparser/abstractcontainer.h>
#include <tagparser/abstracttrack.h>
#include <tagparser/diagnostics.h>
#include <tagparser/language.h>
#include <tagparser/matroska/matroskacontainer.h>
#include <tagparser/matroska/matroskaeditionentry.h>
#include <tagparser/mediafileinfo.h>
#include <tagparser/mp4/mp4container.h>
#include <tagparser/signature.h>

#include <c++utilities/chrono/datetime.h>
#include <c++utilities/conversion/stringconversion.h>
#include <c++utilities/misc/traits.h>

#if defined(TAGEDITOR_GUI_QTWIDGETS)
#include <QApplication>
#include <QIcon>
#include <QStyle>
#elif defined(TAGEDITOR_GUI_QTQUICK)
#include <QGuiApplication>
#include <QIcon>
#endif
#include <QStringBuilder>

using namespace std;
using namespace CppUtilities;
using namespace QtUtilities;
using namespace TagParser;
using namespace Utility;

namespace QtGui {

/*!
 * \cond
 */

// helper functions and methods

QStandardItem *defaultItem(const QString &text)
{
    auto *item = new QStandardItem(text);
    item->setEditable(false);
    return item;
}

class ItemHelper {
public:
    ItemHelper(QStandardItem *item)
        : m_item(item)
    {
        m_item->setEditable(false);
    }

    void appendRow(const QString &label, const QString &text)
    {
        if (!text.isEmpty()) {
            m_item->appendRow(QList<QStandardItem *>() << defaultItem(label) << defaultItem(text));
        }
    }

    void appendRow(const QString &label, const char *text)
    {
        appendRow(label, QString::fromUtf8(text));
    }

    void appendRow(const QString &label, const string &text)
    {
        if (!text.empty()) {
            appendRow(label, text.data());
        }
    }

    void appendRow(const QString &label, DateTime dateTime)
    {
        if (!dateTime.isNull()) {
            appendRow(label, dateTime.toString());
        }
    }

    void appendRow(const QString &label, TimeSpan timeSpan)
    {
        if (!timeSpan.isNull()) {
            appendRow(label, timeSpan.toString(TimeSpanOutputFormat::WithMeasures));
        }
    }

    template <typename IntegralType, Traits::EnableIf<std::is_integral<IntegralType>> * = nullptr>
    void appendRow(const QString &label, IntegralType number)
    {
        if (number) {
            appendRow(label, QString::number(number));
        }
    }

    void appendRow(const QString &label, const Margin &margin)
    {
        if (!margin.isNull()) {
            appendRow(label, margin.toString());
        }
    }

    void appendRow(const QString &label, const Size &size)
    {
        if (!size.isNull()) {
            appendRow(label, size.toString());
        }
    }

private:
    QStandardItem *m_item;
};

void addDiagMessages(TagParser::Diagnostics *diag, QStandardItem *parent)
{
    for (const auto &msg : *diag) {
        QList<QStandardItem *> notificationRow;
        notificationRow.reserve(3);

        auto *firstItem = defaultItem(QString::fromUtf8(msg.creationTime().toString().data()));
        switch (msg.level()) {
        case DiagLevel::None:
        case DiagLevel::Debug:
            firstItem->setIcon(FileInfoModel::debugIcon());
            break;
        case DiagLevel::Information:
            firstItem->setIcon(FileInfoModel::informationIcon());
            break;
        case DiagLevel::Warning:
            firstItem->setIcon(FileInfoModel::warningIcon());
            break;
        case DiagLevel::Critical:
        case DiagLevel::Fatal:
            firstItem->setIcon(FileInfoModel::errorIcon());
            break;
        }
        parent->appendRow(QList<QStandardItem *>() << firstItem << defaultItem(QString::fromUtf8(msg.message().data()))
                                                   << defaultItem(QString::fromUtf8(msg.context().data())));
    }
}

template <class ElementType, bool isAdditional = false> void addElementNode(const ElementType *element, QStandardItem *parent)
{
    while (element) {
        if (element->isParsed()) {
            auto *firstItem = defaultItem(QString::fromLatin1(element->idToString().data()));
            parent->appendRow(QList<QStandardItem *>() << firstItem
                                                       << defaultItem(QStringLiteral("offset: 0x") % QString::number(element->startOffset(), 16)
                                                              % QStringLiteral(", size: 0x") % QString::number(element->totalSize(), 16)));
            if (element->firstChild()) {
                addElementNode(element->firstChild(), firstItem);
            }
            element = element->nextSibling();
        } else {
            if (!isAdditional) {
                auto *notAnalyzedItem = defaultItem(QStringLiteral("not analyzed"));
                notAnalyzedItem->setForeground(QBrush(QColor(Qt::red)));
                parent->appendRow(notAnalyzedItem);
            }
            break;
        }
    }
}

/*!
 * \endcond
 */

/*!
 * \class FileInfoModel
 * \brief The FileInfoModel displays overall information from a TagParser::MediaFileInfo instance.
 *
 * The model assumes that the specified TagParser::MediaFileInfo instance has been parsed already.
 * The model is not updated automatically when the state of the TagParser::MediaFileInfo changes.
 * To update the model, just call setFileInfo() again.
 */

/*!
 * \brief Constructs a new instance with the specified \a fileInfo which might be nullptr.
 */
FileInfoModel::FileInfoModel(QObject *parent)
    : QStandardItemModel(parent)
    , m_file(nullptr)
    , m_diag(nullptr)
    , m_diagReparsing(nullptr)
{
}

QVariant FileInfoModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    switch (orientation) {
    case Qt::Horizontal:
        switch (role) {
        case Qt::DisplayRole:
            switch (section) {
            case 0:
                return tr("Property");
            case 1:
                return tr("Value");
            default:;
            }
            break;
        }
        break;
    default:;
    }
    return QVariant();
}

/*!
 * \brief Returns the currently assigned TagParser::MediaFileInfo.
 */
const MediaFileInfo *FileInfoModel::fileInfo() const
{
    return m_file;
}

/*!
 * \brief Assigns a TagParser::MediaFileInfo.
 * \remarks Causes updating the internal cache and resets the model.
 */
void FileInfoModel::setFileInfo(MediaFileInfo &fileInfo, Diagnostics &diag, Diagnostics *diagReparsing)
{
    m_file = &fileInfo;
    m_diag = &diag;
    m_diagReparsing = diagReparsing;
    updateCache();
}

const QIcon &FileInfoModel::informationIcon()
{
    static const QIcon icon = QIcon::fromTheme(QStringLiteral("emblem-information")
#ifdef GUI_QTWIDGETS
                                                   ,
        QApplication::style()->standardIcon(QStyle::SP_MessageBoxInformation)
#endif
    );
    return icon;
}

const QIcon &FileInfoModel::warningIcon()
{
    static const QIcon icon = QIcon::fromTheme(QStringLiteral("emblem-warning")
#ifdef GUI_QTWIDGETS
                                                   ,
        QApplication::style()->standardIcon(QStyle::SP_MessageBoxWarning)
#endif
    );
    return icon;
}

const QIcon &FileInfoModel::errorIcon()
{
    static const QIcon icon = QIcon::fromTheme(QStringLiteral("emblem-error")
#ifdef GUI_QTWIDGETS
                                                   ,
        QApplication::style()->standardIcon(QStyle::SP_MessageBoxCritical)
#endif
    );
    return icon;
}

const QIcon &FileInfoModel::debugIcon()
{
    static const QIcon icon = QIcon::fromTheme(QStringLiteral("emblem-question"));
    return icon;
}

/*!
 * \brief Internally called to cache the items.
 */
void FileInfoModel::updateCache()
{
    beginResetModel();
    clear();
    if (m_file) {
        // get diag
        Diagnostics &diag = m_diagReparsing ? *m_diagReparsing : *m_diag;

        // get container
        auto *const container = m_file->container();

        // get root item from model
        QStandardItem *rootItem = invisibleRootItem();
        ItemHelper rootHelper(rootItem);

        // add general information
        rootHelper.appendRow(tr("Path"), fromNativeFileName(m_file->path().data()));
        rootHelper.appendRow(tr("Size"), dataSizeToString(m_file->size()));
        const TimeSpan duration = m_file->duration();
        if (!duration.isNull()) {
            rootHelper.appendRow(tr("Duration"), duration);
            rootHelper.appendRow(tr("Overall avg. bitrate"), bitrateToString(m_file->overallAverageBitrate()));
        }
        const char *const mimeType = m_file->mimeType();
        if (*mimeType) {
            rootHelper.appendRow(tr("Mime-type"), mimeType);
        }

        // 3 columns
        setItem(0, 2, new QStandardItem);

        int currentRow;

        // add container item (last top-level-item which is always present)
        auto *containerItem = defaultItem(tr("Container"));
        ItemHelper containerHelper(containerItem);
        setItem(currentRow = rowCount(), containerItem);

        // -> add container name
        QString containerName;
        const char *const subversion = m_file->containerFormatSubversion();
        if (*subversion) {
            containerName = QString::fromUtf8(m_file->containerFormatName()) % QChar(' ') % QString::fromUtf8(m_file->containerFormatSubversion());
        } else {
            containerName = QString::fromUtf8(m_file->containerFormatName());
        }
        setItem(currentRow, 1, defaultItem(containerName));

        // container details
        if (container) {
            size_t segmentIndex = 0;
            for (const auto &title : container->titles()) {
                if (segmentIndex) {
                    containerHelper.appendRow(tr("Title (segment %1)").arg(++segmentIndex), title);
                } else {
                    ++segmentIndex;
                    containerHelper.appendRow(tr("Title"), title);
                }
            }
            containerHelper.appendRow(tr("Version"), container->version());
            containerHelper.appendRow(tr("Read version"), container->readVersion());
            containerHelper.appendRow(tr("Document type"), container->documentType());
            containerHelper.appendRow(tr("Document version"), container->doctypeVersion());
            containerHelper.appendRow(tr("Document read version"), container->doctypeReadVersion());
            containerHelper.appendRow(tr("Tag position"), Utility::elementPositionToQString(container->determineTagPosition(diag)));
            containerHelper.appendRow(tr("Index position"), Utility::elementPositionToQString(container->determineIndexPosition(diag)));
        }
        containerHelper.appendRow(tr("Padding size"), m_file->paddingSize());

        // tags
        {
            const auto tags = m_file->tags();
            if (!tags.empty()) {
                auto *tagsItem = defaultItem(tr("Tags"));
                setItem(++currentRow, tagsItem);
                setItem(currentRow, 1, defaultItem(tr("%1 tag(s) assigned", nullptr, trQuandity(tags.size())).arg(tags.size())));

                for (const Tag *const tag : tags) {
                    auto *const tagItem = defaultItem(tag->typeName());
                    ItemHelper tagHelper(tagItem);
                    tagHelper.appendRow(tr("Version"), tag->version());
                    if (tag->supportsTarget() && !tag->target().isEmpty()) {
                        tagHelper.appendRow(tr("Target level"), tag->targetString());
                    }
                    tagHelper.appendRow(tr("Size"), dataSizeToString(tag->size(), true));
                    tagHelper.appendRow(tr("Field count"), tag->fieldCount());
                    tagsItem->appendRow(tagItem);
                }
            }
        }

        // tracks
        {
            const auto tracks = m_file->tracks();
            if (!tracks.empty()) {
                auto *tracksItem = defaultItem(tr("Tracks"));
                setItem(++currentRow, tracksItem);
                const string summary(m_file->technicalSummary());
                if (summary.empty()) {
                    setItem(currentRow, 1, defaultItem(tr("%1 track(s) contained", nullptr, trQuandity(tracks.size())).arg(tracks.size())));
                } else {
                    setItem(currentRow, 1,
                        defaultItem(tr("%1 track(s): ", nullptr, trQuandity(tracks.size())).arg(tracks.size())
                            + QString::fromUtf8(summary.data(), trQuandity(summary.size()))));
                }

                size_t number = 0;
                for (const AbstractTrack *const track : tracks) {
                    auto *const trackItem = defaultItem(tr("Track #%1").arg(++number));
                    ItemHelper trackHelper(trackItem);
                    trackHelper.appendRow(tr("ID"), track->id());
                    trackHelper.appendRow(tr("Number"), track->trackNumber());
                    trackHelper.appendRow(tr("Name"), track->name());
                    trackHelper.appendRow(tr("Type"), track->mediaTypeName());
                    const char *fmtName = track->formatName(), *fmtAbbr = track->formatAbbreviation();
                    trackHelper.appendRow(tr("Format"), fmtName);
                    if (track->format() != GeneralMediaFormat::Unknown && strcmp(fmtName, fmtAbbr)) { // format name and abbreviation differ
                        trackHelper.appendRow(tr("Abbreviation"), fmtAbbr);
                    }
                    if (track->version() > 0) {
                        switch (track->format().general) {
                        case GeneralMediaFormat::Mpeg4Video:
                        case GeneralMediaFormat::Avc:
                            trackHelper.appendRow(tr("Level"), QChar('L') + QString::number(track->version()));
                            break;
                        default:
                            trackHelper.appendRow(tr("Version"), QString::number(track->version()));
                        }
                    }
                    fmtName = track->format().extensionName();
                    if (*fmtName) {
                        trackHelper.appendRow(tr("Extension"), fmtName);
                    }
                    if (!track->formatId().empty()) {
                        trackHelper.appendRow(tr("Format/codec ID"), track->formatId());
                    }
                    if (track->size()) {
                        trackHelper.appendRow(tr("Size"), dataSizeToString(track->size(), true));
                    }
                    if (!track->duration().isNull()) {
                        trackHelper.appendRow(tr("Duration"), track->duration());
                    }
                    if (track->bitrate() > 0.0) {
                        trackHelper.appendRow(tr("Avg. bitrate"), bitrateToString(track->bitrate()));
                    }
                    if (track->maxBitrate() > 0.0) {
                        trackHelper.appendRow(tr("Max. bitrate"), bitrateToString(track->maxBitrate()));
                    }
                    if (!track->creationTime().isNull()) {
                        trackHelper.appendRow(tr("Creation time"), track->creationTime());
                    }
                    if (!track->modificationTime().isNull()) {
                        trackHelper.appendRow(tr("Modification time"), track->modificationTime());
                    }
                    if (!track->language().empty()) {
                        trackHelper.appendRow(tr("Language"), languageNameFromIsoWithFallback(track->language()));
                    }
                    if (!track->compressorName().empty()) {
                        trackHelper.appendRow(tr("Compressor name"), track->compressorName());
                    }
                    if (track->samplingFrequency()) {
                        trackHelper.appendRow(tr("Sampling frequency"),
                            track->extensionSamplingFrequency() ? QString::number(track->extensionSamplingFrequency()) % QStringLiteral(" Hz / ")
                                    % QString::number(track->samplingFrequency()) % QStringLiteral(" Hz")
                                                                : QString::number(track->samplingFrequency()) + QStringLiteral(" Hz"));
                    }
                    if (track->sampleCount()) {
                        trackHelper.appendRow(track->mediaType() == MediaType::Video ? tr("Frame count") : tr("Sample count"), track->sampleCount());
                    }
                    if (track->bitsPerSample()) {
                        trackHelper.appendRow(tr("Bits per sample"), track->bitsPerSample());
                    }
                    if (track->quality()) {
                        trackHelper.appendRow(tr("Quality"), track->quality());
                    }
                    if (!track->pixelSize().isNull()) {
                        trackHelper.appendRow(tr("Pixel size"), track->pixelSize());
                    }
                    if (!track->displaySize().isNull()) {
                        trackHelper.appendRow(tr("Display size"), track->displaySize());
                    }
                    if (track->pixelAspectRatio().isValid()) {
                        trackHelper.appendRow(tr("Pixel Aspect Ratio"), track->pixelAspectRatio().toString());
                    }
                    if (!track->cropping().isNull()) {
                        trackHelper.appendRow(tr("Cropping"), track->cropping());
                    }
                    if (!track->resolution().isNull()) {
                        trackHelper.appendRow(tr("Resolution"), track->resolution());
                    }
                    if (track->channelConfigString()) {
                        trackHelper.appendRow(tr("Channel config"),
                            track->extensionChannelConfigString() ? QString::fromUtf8(track->extensionChannelConfigString()) % QStringLiteral(" / ")
                                    % QString::fromUtf8(track->channelConfigString())
                                                                  : QString::fromUtf8(track->channelConfigString()));
                    } else {
                        trackHelper.appendRow(tr("Channel count"), track->channelCount());
                    }
                    trackHelper.appendRow(tr("Bit depth"), track->depth());
                    trackHelper.appendRow(tr("Frames per second"), track->fps());
                    trackHelper.appendRow(tr("Chroma format"), track->chromaFormat());
                    QStringList labels;
                    if (track->isInterlaced()) {
                        labels << tr("interlaced");
                    }
                    if (!track->isEnabled()) {
                        labels << tr("disabled");
                    }
                    if (track->isDefault()) {
                        labels << tr("default");
                    }
                    if (track->isForced()) {
                        labels << tr("forced");
                    }
                    if (track->hasLacing()) {
                        labels << tr("has lacing");
                    }
                    if (track->isEncrypted()) {
                        labels << tr("encrypted");
                    }
                    if (!labels.isEmpty()) {
                        trackHelper.appendRow(tr("Labeled as"), labels.join(QStringLiteral(", ")));
                    }
                    tracksItem->appendRow(trackItem);
                }
            }
        }

        // attachments
        {
            const auto attachments = m_file->attachments();
            if (!attachments.empty()) {
                auto *attachmentsItem = defaultItem(tr("Attachments"));
                setItem(++currentRow, attachmentsItem);
                setItem(currentRow, 1, defaultItem(tr("%1 attachment(s) present", nullptr, trQuandity(attachments.size())).arg(attachments.size())));

                size_t number = 0;
                for (const AbstractAttachment *const attachment : attachments) {
                    auto *const attachmentItem = defaultItem(tr("Attachment #%1").arg(++number));
                    ItemHelper attachHelper(attachmentItem);
                    attachHelper.appendRow(tr("ID"), attachment->id());
                    attachHelper.appendRow(tr("Name"), attachment->name());
                    attachHelper.appendRow(tr("Size"), dataSizeToString(static_cast<std::uint64_t>(attachment->data()->size())));
                    attachHelper.appendRow(tr("Mime-type"), attachment->mimeType());
                    attachHelper.appendRow(tr("Description"), attachment->description());
                    attachmentsItem->appendRow(attachmentItem);
                }
            }
        }

        // chapters/editions
        {
            size_t number = 0;
            function<void(const AbstractChapter *, QStandardItem *)> addChapter;
            addChapter = [&addChapter, &number](const AbstractChapter *chapter, QStandardItem *parent) {
                auto *chapterItem = defaultItem(tr("Chapter #%1").arg(++number));
                ItemHelper chapterHelper(chapterItem);
                chapterHelper.appendRow(tr("ID"), chapter->id());
                for (const LocaleAwareString &name : chapter->names()) {
                    static const string delim(", ");
                    const string locale = joinStrings(
                        initializer_list<string>{ joinStrings(name.languages(), delim, true), joinStrings(name.countries(), delim, true) }, delim,
                        true);
                    chapterHelper.appendRow(tr("Name (%1)").arg(QString::fromUtf8(locale.data())), name);
                }
                if (!chapter->startTime().isNegative()) {
                    chapterHelper.appendRow(tr("Start time"), chapter->startTime());
                }
                if (!chapter->endTime().isNegative()) {
                    chapterHelper.appendRow(tr("End time"), chapter->endTime());
                }
                QStringList labels;
                if (chapter->isHidden()) {
                    labels << tr("hidden");
                }
                if (!chapter->isEnabled()) {
                    labels << tr("disabled");
                }
                if (!labels.empty()) {
                    chapterHelper.appendRow(tr("Labeled as"), labels.join(QStringLiteral(", ")));
                }
                if (!chapter->tracks().empty()) {
                    QStringList trackIds;
                    for (const auto id : chapter->tracks()) {
                        trackIds << QString::number(id);
                    }
                    chapterHelper.appendRow(tr("Tracks"), trackIds.join(QStringLiteral(", ")));
                }
                for (size_t i = 0, nestedChapters = chapter->nestedChapterCount(); i < nestedChapters; ++i) {
                    addChapter(chapter->nestedChapter(i), chapterItem);
                }
                parent->appendRow(chapterItem);
            };

            if (m_file->containerFormat() == ContainerFormat::Matroska) {
                const auto &editionEntries = static_cast<const MatroskaContainer *>(container)->editionEntires();
                if (!editionEntries.empty()) {
                    auto *editionsItem = defaultItem(tr("Editions"));
                    setItem(++currentRow, editionsItem);
                    setItem(currentRow, 1,
                        defaultItem(tr("%1 edition(s) present", nullptr, trQuandity(editionEntries.size())).arg(editionEntries.size())));
                    size_t editionNumber = 0;
                    for (const auto &edition : editionEntries) {
                        auto *editionItem = defaultItem(tr("Edition #%1").arg(++editionNumber));
                        ItemHelper editionHelper(editionItem);
                        editionHelper.appendRow(tr("ID"), edition->id());
                        QStringList labels;
                        if (edition->isHidden()) {
                            labels << tr("hidden");
                        }
                        if (edition->isDefault()) {
                            labels << tr("default");
                        }
                        if (edition->isOrdered()) {
                            labels << tr("ordered");
                        }
                        if (!labels.isEmpty()) {
                            editionHelper.appendRow(tr("Labeled as"), labels.join(QStringLiteral(", ")));
                        }
                        for (const auto &chapter : edition->chapters()) {
                            addChapter(chapter.get(), editionItem);
                        }
                        editionsItem->appendRow(editionItem);
                    }
                }
            } else {
                const auto chapters = m_file->chapters();
                if (!chapters.empty()) {
                    auto *chaptersItem = defaultItem(tr("Chapters"));
                    setItem(++currentRow, chaptersItem);
                    setItem(currentRow, 1, defaultItem(tr("%1 chapter(s) present", nullptr, trQuandity(chapters.size())).arg(chapters.size())));
                    for (const AbstractChapter *chapter : chapters) {
                        addChapter(chapter, chaptersItem);
                    }
                }
            }
        }

        // structure
        switch (m_file->containerFormat()) {
        case ContainerFormat::Mp4:
        case ContainerFormat::QuickTime:
        case ContainerFormat::Matroska:
        case ContainerFormat::Webm:
        case ContainerFormat::Ebml: {
            auto *structureItem = defaultItem(tr("Structure"));
            switch (m_file->containerFormat()) {
            case ContainerFormat::Mp4:
            case ContainerFormat::QuickTime:
                addElementNode(static_cast<const Mp4Container *>(container)->firstElement(), structureItem);
                break;
            case ContainerFormat::Matroska:
            case ContainerFormat::Webm:
            case ContainerFormat::Ebml:
                addElementNode(static_cast<const MatroskaContainer *>(container)->firstElement(), structureItem);
                break;
            default:;
            }
            setItem(++currentRow, structureItem);
            setItem(currentRow, 1, defaultItem(QString()));
        } break;
        default:;
        }

        // notifications
        auto *const diagItem = defaultItem(tr("Diagnostic messages"));
        addDiagMessages(m_diag, diagItem);
        setItem(++currentRow, diagItem);
        if (m_diagReparsing) {
            auto *diagReparsingItem = defaultItem(tr("Diagnostic messages from reparsing"));
            addDiagMessages(m_diagReparsing, diagReparsingItem);
            setItem(++currentRow, diagReparsingItem);
        }
    }
    endResetModel();
}

} // namespace QtGui
