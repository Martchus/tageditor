#include "fileinfomodel.h"

#include <taginfo/parsingexception.h>
#include <taginfo/tag.h>
#include <taginfo/abstractcontainer.h>
#include <taginfo/abstracttrack.h>
#include <taginfo/id3/id3v1tag.h>
#include <taginfo/id3/id3v2tag.h>
#include <taginfo/mp4/mp4atom.h>
#include <taginfo/mp4/mp4container.h>
#include <taginfo/matroska/matroskaid.h>
#include <taginfo/matroska/ebmlelement.h>
#include <taginfo/matroska/matroskacontainer.h>

#include <martchusutilities/misc/misc.h>
#include <martchusutilities/conversion/stringconversion.h>

using namespace std;
using namespace ChronoUtilities;
using namespace Media;

namespace QtGui {

TreeItem::TreeItem(const QList<QVariant> &data, TreeItem *parent) :
    m_itemData(data),
    m_parentItem(parent)
{}

TreeItem::TreeItem(const QString &data0, const QString &data1, TreeItem *parent) :
    m_itemData(QList<QVariant>() << data0 << data1),
    m_parentItem(parent)
{}

TreeItem::TreeItem(const QString &data0, const string &data1, TreeItem *parent) :
    m_itemData(QList<QVariant>() << data0 << QString::fromLocal8Bit(data1.c_str())),
    m_parentItem(parent)
{}

TreeItem::TreeItem(const std::string &data0, const std::string &data1, TreeItem *parent) :
    m_itemData(QList<QVariant>() << QString::fromLocal8Bit(data0.c_str()) << QString::fromLocal8Bit(data1.c_str())),
    m_parentItem(parent)
{}

TreeItem::~TreeItem()
{
    qDeleteAll(m_childItems);
}

void TreeItem::appendChild(TreeItem *item)
{
    m_childItems.append(item);
}

TreeItem *TreeItem::appendChild(const QList<QVariant> &data)
{
    TreeItem *item = new TreeItem(data, this);
    m_childItems.append(item);
    return item;
}

TreeItem *TreeItem::appendChild(const QString &data)
{
    TreeItem *item = new TreeItem(data, QString(), this);
    m_childItems.append(item);
    return item;
}

TreeItem *TreeItem::appendChild(const QString &data0, const QString &data1)
{
    TreeItem *item = new TreeItem(data0, data1, this);
    m_childItems.append(item);
    return item;
}

TreeItem *TreeItem::appendChild(const QString &data0, const string &data1)
{
    TreeItem *item = new TreeItem(data0, data1, this);
    m_childItems.append(item);
    return item;
}

TreeItem *TreeItem::appendChild(const string &data)
{
    TreeItem *item = new TreeItem(data, string(), this);
    m_childItems.append(item);
    return item;
}

TreeItem *TreeItem::appendChild(const std::string &data0, const std::string &data1)
{
    TreeItem *item = new TreeItem(data0, data1, this);
    m_childItems.append(item);
    return item;
}

TreeItem *TreeItem::appendChild(const QString &data0, int data1)
{
    TreeItem *item = new TreeItem(data0, QString::number(data1), this);
    m_childItems.append(item);
    return item;
}

TreeItem *TreeItem::child(int row)
{
    return m_childItems.value(row);
}

int TreeItem::childCount() const
{
    return m_childItems.count();
}

int TreeItem::row() const
{
    if (m_parentItem)
        return m_parentItem->m_childItems.indexOf(const_cast<TreeItem *>(this));

    return 0;
}

int TreeItem::columnCount() const
{
    return m_itemData.count();
}

QVariant TreeItem::data(int column) const
{
    return m_itemData.value(column);
}

TreeItem *TreeItem::parent()
{
    return m_parentItem;
}

void TreeItem::clearChilds()
{
    qDeleteAll(m_childItems);
    m_childItems.clear();
}

void TreeItem::clearData()
{
    m_itemData.clear();
}

const QIcon &TreeItem::icon() const
{
    return m_icon;
}

void TreeItem::setIcon(const QIcon &value)
{
    m_icon = value;
}

FileInfoModel::FileInfoModel(QObject *parent) :
    QAbstractItemModel(parent),
    m_fileInfo(0),
    m_anyNotifications(false),
    m_criticalNotifications(false)
{
    m_rootItem = new TreeItem(QList<QVariant>() << "Property" << "Value");
    m_generalItem = m_rootItem->appendChild(QStringLiteral("General information and container/main format"));
    m_generalItem->setIcon(QIcon::fromTheme(QStringLiteral("document-properties")));
    m_audioTracksItem = m_rootItem->appendChild(QStringLiteral("Audio tracks"));
    m_audioTracksItem->setIcon(QIcon::fromTheme(QStringLiteral("audio-x-generic"), QIcon(QStringLiteral("/images/music"))));
    m_videoTracksItem = m_rootItem->appendChild(QStringLiteral("Video tracks"));
    m_videoTracksItem->setIcon(QIcon::fromTheme(QStringLiteral("video-x-generic"), QIcon(QStringLiteral("/images/video"))));
    m_subtitleTracksItem = m_rootItem->appendChild(QStringLiteral("Subtitle tracks"));
    m_otherTracksItem = m_rootItem->appendChild(QStringLiteral("Other tracks"));
    m_tagItem = m_rootItem->appendChild(QStringLiteral("Tag(s)"));
    m_tagItem->setIcon(QIcon::fromTheme(QStringLiteral("multimedia-player"), QIcon(QStringLiteral("/images/tag"))));
}

FileInfoModel::FileInfoModel(MediaFileInfo *fileInfo, QObject *parent) :
    FileInfoModel(parent)
{
    show(fileInfo);
}

FileInfoModel::~FileInfoModel()
{
    delete m_rootItem;
}

QVariant FileInfoModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid()) {
        return QVariant();
    }
    switch(role) {
    case Qt::DisplayRole:
        return static_cast<TreeItem *>(index.internalPointer())->data(index.column());
    case Qt::DecorationRole:
        if(index.column() == 0) {
            TreeItem *item = static_cast<TreeItem *>(index.internalPointer());
            if(!item->icon().isNull())
                return item->icon();
        }
    default:
        ;
    }
    return QVariant();
}

Qt::ItemFlags FileInfoModel::flags(const QModelIndex &index) const
{
    if (!index.isValid()) {
            return 0;
    }
    return QAbstractItemModel::flags(index);
}

QVariant FileInfoModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (orientation == Qt::Horizontal && role == Qt::DisplayRole) {
            return m_rootItem->data(section);
    }
    return QVariant();
}

QModelIndex FileInfoModel::index(int row, int column, const QModelIndex &parent) const
{
    if (!hasIndex(row, column, parent)) {
        return QModelIndex();
    }

    TreeItem *parentItem;
    if (!parent.isValid()) {
        parentItem = m_rootItem;
    } else {
        parentItem = static_cast<TreeItem *>(parent.internalPointer());
    }

    if (TreeItem *childItem = parentItem->child(row)) {
        return createIndex(row, column, childItem);
    } else {
        return QModelIndex();
    }
}

QModelIndex FileInfoModel::parent(const QModelIndex &index) const
{
    if (!index.isValid()) {
        return QModelIndex();
    }
    TreeItem *childItem = static_cast<TreeItem*>(index.internalPointer());
    TreeItem *parentItem = childItem->parent();
    if (parentItem == m_rootItem) {
        return QModelIndex();
    }
    return createIndex(parentItem->row(), 0, parentItem);
}

int FileInfoModel::rowCount(const QModelIndex &parent) const
{
    TreeItem *parentItem;
    if (parent.column() > 0) {
        return 0;
    }
    if (!parent.isValid()) {
        parentItem = m_rootItem;
    } else {
        parentItem = static_cast<TreeItem *>(parent.internalPointer());
    }
    return parentItem->childCount();
}

int FileInfoModel::columnCount(const QModelIndex &parent) const
{
    if (parent.isValid()) {
        return static_cast<TreeItem *>(parent.internalPointer())->columnCount();
    } else {
        return m_rootItem->columnCount();
    }
}

const MediaFileInfo *FileInfoModel::shownFileInfo() const
{
    return m_fileInfo;
}

MediaFileInfo *FileInfoModel::shownFileInfo()
{
    return m_fileInfo;
}

void FileInfoModel::show(MediaFileInfo *fileInfo)
{
    m_fileInfo = fileInfo;
    update();
}

void FileInfoModel::update()
{
    beginResetModel();

    m_anyNotifications = false;
    m_criticalNotifications = false;
    m_generalItem->clearChilds();
    m_audioTracksItem->clearChilds();
    m_videoTracksItem->clearChilds();
    m_otherTracksItem->clearChilds();
    m_tagItem->clearChilds();

    showGeneralInformation();
    showTracks();
    showTagInformation();

    endResetModel();
}

bool FileInfoModel::containsAnyNotifications() const
{
    return m_anyNotifications;
}

bool FileInfoModel::containsCriticalNotifications() const
{
    return m_criticalNotifications;
}

void FileInfoModel::showGeneralInformation()
{
    if(m_fileInfo && m_fileInfo->isContainerParsed()) {
        AbstractContainer *container = m_fileInfo->container();
        m_generalItem->appendChild(tr("Path"), m_fileInfo->path());
        m_generalItem->appendChild(tr("Extension"), m_fileInfo->extension());
        m_generalItem->appendChild(tr("Size"), ConversionUtilities::dataSizeToString(m_fileInfo->size()));
        TreeItem *containerItem = m_generalItem->appendChild("Container format", QString::fromLocal8Bit(m_fileInfo->containerFormatName()));
        QString version;
        if(container) {
            if(!container->duration().isNull()) {
                m_generalItem->appendChild(tr("Duration"), container->duration().toString(TimeSpanOutputFormat::WithMeasures));
            }
            if(!container->title().empty()) {
                m_generalItem->appendChild(tr("Title"), container->title());
            }
            if(!container->documentType().empty()) {
                containerItem->appendChild(tr("Document type"), container->documentType());
            }
            version = QString::number(container->version());
        } else {
            version = QString::fromLocal8Bit(m_fileInfo->containerFormatSubversion());
        }
        containerItem->appendChild(tr("Version"), version);
        containerItem->appendChild(tr("Abbreviation"), QString::fromLocal8Bit(m_fileInfo->containerFormatAbbreviation()));
        if(Mp4Container *container = m_fileInfo->mp4Container()) { // show atom structure if its an MP4 file
            if(Mp4Atom *atom = container->firstElement()) {
                TreeItem *atomsItem = containerItem->appendChild(tr("Atom structure"));
                showAtoms(atomsItem, atom);
            }
        }
        if(MatroskaContainer *container = m_fileInfo->matroskaContainer()) { // show EBML elements if its an EBML/Matroska file
            if(EbmlElement *element = container->firstElement()) {
                TreeItem *elementItem = containerItem->appendChild(tr("EBML element structure (to first cluster)"));
                showEbmlElements(elementItem, element);
            }
        }
        showNotifications(m_fileInfo->lastParsingNotifications(), m_generalItem, tr("General parsing notifications"));
        showNotifications(m_fileInfo->lastMakingNotifications(), m_generalItem, tr("Last saving notifications"));
    }
}

void FileInfoModel::showAtoms(TreeItem *item, Mp4Atom *atom)
{
    while(atom) {
        try {
            atom->parse();
            if(atom->lastParsingNotifications().size())
                showNotifications(atom->lastParsingNotifications(), item, tr("Expand to display parsing warnings"));
            showAtoms(item->appendChild(
                          QString::fromLatin1(atom->idToString().c_str()),
                             tr("@%1, size: %2").arg(atom->startOffset()).arg(atom->totalSize())),
                         atom->firstChild());
            atom = atom->nextSibling();
        } catch(ParsingException &) {
            showNotifications(atom->lastParsingNotifications(), item, tr("Invalid atom found, expand for detail."));
            return;
        } catch(ios_base::failure &) {
            showNotifications(atom->lastParsingNotifications(), item, tr("IO error occured when parsing atom."));
            return;
        }
    }
}

void FileInfoModel::showEbmlElements(TreeItem *item, EbmlElement *element)
{
    while(element) {
        try {
            element->parse();
            if(element->lastParsingNotifications().size())
                showNotifications(element->lastParsingNotifications(), item, tr("Expand to display parsing warnings"));
            showEbmlElements(item->appendChild(
                          QString::fromLatin1(element->idToString().c_str()),
                             tr("@%1, size: %2").arg(element->startOffset()).arg(element->totalSize())),
                         element->firstChild());
            if(element->id() == MatroskaId::Cluster) {
                break; // cluster reached; just display the first cluster
            }
            element = element->nextSibling();
        } catch(ParsingException &) {
            showNotifications(element->lastParsingNotifications(), item, tr("Invalid EBML element found, expand for detail."));
            return;
        } catch(ios_base::failure &) {
            showNotifications(element->lastParsingNotifications(), item, tr("IO error occured when parsing atom."));
            return;
        }
    }
}

void FileInfoModel::showTracks()
{
    TreeItem *item, *formatItem;
    int i = 1;
    for(AbstractTrack *track : m_fileInfo->tracks()) {
        list<string> identifiers;
        identifiers.push_back(track->name());
        identifiers.push_back("ID: " + ConversionUtilities::numberToString<uint32>(track->id()));
        QString name = QString::fromLocal8Bit(ConversionUtilities::concateStrings(identifiers, ", ", true).c_str());
        if(name.isEmpty()) {
            name = tr("Unnamed track");
        }
        switch(track->mediaType()) {
        case MediaType::Audio:
            item = m_audioTracksItem->appendChild(name);
            break;
        case MediaType::Video:
            item = m_videoTracksItem->appendChild(name);
            break;
        case MediaType::Subtitle:
            item = m_subtitleTracksItem->appendChild(name);
            break;
        default:
            item = m_otherTracksItem->appendChild(name);
        }
        if(track->isHeaderValid()) {
            formatItem = item->appendChild(tr("Format"), QString::fromLocal8Bit(track->formatName()));
            formatItem->appendChild(tr("Version"), QStringLiteral("%1").arg(track->version()));
            formatItem->appendChild(tr("Abbreviation"), QString::fromLocal8Bit(track->formatAbbreviation()));
            if(track->bitrate() > 0.0)
                item->appendChild(tr("Bitrate"), ConversionUtilities::bitrateToString(track->bitrate()));
            if(!track->duration().isNull())
                item->appendChild(tr("Duration"), track->duration().toString(TimeSpanOutputFormat::WithMeasures));
            if(track->size())
                item->appendChild(tr("Size"), ConversionUtilities::dataSizeToString(track->size()));
            if(!track->compressorName().empty())
                item->appendChild(tr("Compressor"), track->compressorName());
            if(!track->creationTime().isNull())
                item->appendChild(tr("Creation time"), track->creationTime().toString());
            if(!track->modificationTime().isNull())
                item->appendChild(tr("Modification time"), track->modificationTime().toString());
            if(track->sampleCount())
                item->appendChild(tr("Sample count"), track->sampleCount());
            if(track->samplesPerSecond())
                item->appendChild(tr("Samples per second"), track->samplesPerSecond());
            if(track->quality())
                item->appendChild(tr("Quality"), track->quality());
            if(!track->language().empty())
                item->appendChild(tr("Language"), track->language());
            list<string> labels;
            if(!track->isEnabled())
                labels.push_back("disabled");
            if(track->isDefault())
                labels.push_back("default");
            if(track->isForced())
                labels.push_back("forced");
            if(track->isEncrypted())
                labels.push_back("encrypted");
            string concatedLabels = ConversionUtilities::concateStrings(labels, ", ");
            if(!concatedLabels.empty())
                item->appendChild(tr("Language"), concatedLabels);
            switch(track->mediaType()) {
            case MediaType::Audio:
                if(track->bitsPerSample())
                    item->appendChild(tr("Bits per sample"), track->bitsPerSample());
                if(track->channelCount())
                    item->appendChild(tr("Channel count"), track->channelCount());
                break;
            case MediaType::Video:
                if(!track->pixelSize().isNull())
                    item->appendChild(tr("Size in px"), track->pixelSize().toString());
                if(!track->displaySize().isNull())
                    item->appendChild(tr("Display size in px"), track->displaySize().toString());
                if(!track->cropping().isNull())
                    item->appendChild(tr("Cropping in px"), track->cropping().toString());
                if(track->depth())
                    item->appendChild(tr("Depth"), track->depth());
                if(track->fps())
                    item->appendChild(tr("Frames per second"), track->fps());
                break;
            default:
                ;
            }
        } else {
            item = m_otherTracksItem->appendChild(tr("Invalid/unsupported segment %1").arg(i));
            item->appendChild(tr("Unable to parse file segment."), tr("See notifications for details."));
        }
        showNotifications(track->lastParsingNotifications(), item, tr("Parsing notifications"));
        ++i;
    }
}

void FileInfoModel::showTagInformation()
{
    if(m_fileInfo) {
        static const QString expand(tr("expand for details"));
        if(m_fileInfo->hasMp4Tag()) {
            showTagDetails(m_fileInfo->mp4Tag(), m_tagItem->appendChild(tr("MP4 atoms store tag information"), expand));
        } else if(m_fileInfo->hasMatroskaTag()) {
            for(const auto &tag : m_fileInfo->matroskaTags()) {
                showTagDetails(tag.get(), m_tagItem->appendChild(tr("Matroska container stores tag information"), expand));
            }
        }
        const vector<unique_ptr<Id3v2Tag>> &id3v2Tags = m_fileInfo->id3v2Tags();
        for(vector<unique_ptr<Id3v2Tag>>::const_iterator i = id3v2Tags.begin(), end = id3v2Tags.end();
            i != end; ++i) {
            showTagDetails((*i).get(), m_tagItem->appendChild(tr("ID3v2 tag"), expand));
        }
        if(m_fileInfo->hasId3v1Tag())
            showTagDetails(m_fileInfo->id3v1Tag(), m_tagItem->appendChild(tr("ID3v1 tag"), expand));
    }
}

void FileInfoModel::showTagDetails(const Tag *tag, TreeItem *item)
{
    // display version
    //static const QString version(QStringLiteral("%1"));
    //static const QString unknownVersion(QStringLiteral("unknown or invalid tag"));
    item->appendChild(tr("Version"), QString::fromLocal8Bit(tag->version().c_str()));

    item->appendChild(tr("Size"), ConversionUtilities::dataSizeToString(tag->size()));        // size
    item->appendChild(tr("Field count"), tag->fieldCount());                   // field count

    showNotifications(tag->lastParsingNotifications(), item, tr("Parsing notifications"));
    showNotifications(tag->lastMakingNotifications(), item, tr("Saving notifications"));
}

void FileInfoModel::showNotifications(const list<Notification> &notifications, TreeItem *item, const QString &label, bool showEmptyEntry)
{
    static const QIcon informationIcon = QIcon::fromTheme(QStringLiteral("dialog-information"), QIcon(QStringLiteral("/images/information")));
    static const QIcon warningIcon = QIcon::fromTheme(QStringLiteral("dialog-warning"), QIcon(QStringLiteral("/images/warning")));
    static const QIcon errorIcon = QIcon::fromTheme(QStringLiteral("dialog-error"), QIcon(QStringLiteral("/images/error")));
    static const QIcon debugIcon = QIcon::fromTheme(QStringLiteral("utilities-terminal"), QIcon(QStringLiteral("/images/bug")));

    if(notifications.size()) {
        m_anyNotifications = true;

        TreeItem *notificationsItem = item->appendChild(label, QStringLiteral("%1").arg(notifications.size()));
        TreeItem *notificationItem;
        for(list<Notification>::const_iterator i = notifications.cbegin(), end = notifications.cend(); i != end; ++i) {
            notificationItem = notificationsItem->appendChild(i->message());
            switch(i->type()) {
            case NotificationType::Warning:
                notificationItem->setIcon(warningIcon);
                break;
            case NotificationType::Critical:
                m_criticalNotifications = true;
                notificationItem->setIcon(errorIcon);
                break;
            case NotificationType::Information:
                notificationItem->setIcon(informationIcon);
                break;
            case NotificationType::Debug:
                notificationItem->setIcon(debugIcon);
                break;
            default:
                ;
            }
        }
    } else if(showEmptyEntry) {
        item->appendChild(label, tr("none"));
    }
}

}
