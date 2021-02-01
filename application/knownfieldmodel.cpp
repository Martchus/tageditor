#include "./knownfieldmodel.h"

#include <tagparser/tag.h>

using namespace TagParser;

#if defined(TAGEDITOR_GUI_QTWIDGETS) || defined(TAGEDITOR_GUI_QTQUICK)
using namespace QtUtilities;
#else
#define QT_TR_NOOP(x) x
#endif

namespace Settings {

const char *KnownFieldModel::fieldName(KnownField field)
{
    switch (field) {
    case KnownField::Title:
        return QT_TR_NOOP("Title");
    case KnownField::Album:
        return QT_TR_NOOP("Album");
    case KnownField::Artist:
        return QT_TR_NOOP("Artist");
    case KnownField::Genre:
        return QT_TR_NOOP("Genre");
    case KnownField::Comment:
        return QT_TR_NOOP("Comment");
    case KnownField::Bpm:
        return QT_TR_NOOP("BPM");
    case KnownField::Bps:
        return QT_TR_NOOP("BPS");
    case KnownField::Lyricist:
        return QT_TR_NOOP("Lyricist");
    case KnownField::TrackPosition:
        return QT_TR_NOOP("Track");
    case KnownField::DiskPosition:
        return QT_TR_NOOP("Disk");
    case KnownField::PartNumber:
        return QT_TR_NOOP("Part");
    case KnownField::TotalParts:
        return QT_TR_NOOP("Total parts");
    case KnownField::Encoder:
        return QT_TR_NOOP("Encoder");
    case KnownField::RecordDate:
        return QT_TR_NOOP("Record date");
    case KnownField::Performers:
        return QT_TR_NOOP("Performers");
    case KnownField::Length:
        return QT_TR_NOOP("Duration");
    case KnownField::Language:
        return QT_TR_NOOP("Language");
    case KnownField::EncoderSettings:
        return QT_TR_NOOP("Encoder settings");
    case KnownField::Lyrics:
        return QT_TR_NOOP("Lyrics");
    case KnownField::SynchronizedLyrics:
        return QT_TR_NOOP("Synchronized lyrics");
    case KnownField::Grouping:
        return QT_TR_NOOP("Grouping");
    case KnownField::RecordLabel:
        return QT_TR_NOOP("Record label");
    case KnownField::Cover:
        return QT_TR_NOOP("Cover");
    case KnownField::Composer:
        return QT_TR_NOOP("Composer");
    case KnownField::Rating:
        return QT_TR_NOOP("Rating");
    case KnownField::Description:
        return QT_TR_NOOP("Description");
    case KnownField::Vendor:
        return QT_TR_NOOP("Vendor");
    case KnownField::AlbumArtist:
        return QT_TR_NOOP("Album artist");
    case KnownField::ReleaseDate:
        return QT_TR_NOOP("Release date");
    default:
        return "";
    }
}

#if defined(TAGEDITOR_GUI_QTWIDGETS) || defined(TAGEDITOR_GUI_QTQUICK)
QString KnownFieldModel::translatedFieldName(KnownField field)
{
    return tr(fieldName(field));
}

QString KnownFieldModel::labelForId(const QVariant &id) const
{
    return translatedFieldName(static_cast<KnownField>(id.toInt()));
}

KnownFieldModel::KnownFieldModel(QObject *parent, DefaultSelection defaultSelection)
    : ChecklistModel(parent)
{
    const auto defaultSelected = defaultSelection == DefaultSelection::CommonFields ? Qt::Checked : Qt::Unchecked;
    // clang-format off
    setItems({
         mkItem(KnownField::Title, defaultSelected),
         mkItem(KnownField::Album, defaultSelected),
         mkItem(KnownField::Artist, defaultSelected),
         mkItem(KnownField::Genre, defaultSelected),
         mkItem(KnownField::RecordDate, defaultSelected),
         mkItem(KnownField::Comment, defaultSelected),
         mkItem(KnownField::AlbumArtist, defaultSelected),
         mkItem(KnownField::Bpm, Qt::Unchecked),
         mkItem(KnownField::Bps, Qt::Unchecked),
         mkItem(KnownField::Lyricist, defaultSelected),
         mkItem(KnownField::TrackPosition, defaultSelected),
         mkItem(KnownField::DiskPosition, defaultSelected),
         mkItem(KnownField::PartNumber, defaultSelected),
         mkItem(KnownField::TotalParts, defaultSelected),
         mkItem(KnownField::Encoder, defaultSelected),
         mkItem(KnownField::Performers, defaultSelected),
         mkItem(KnownField::Length, Qt::Unchecked),
         mkItem(KnownField::Language, Qt::Unchecked),
         mkItem(KnownField::EncoderSettings, Qt::Unchecked),
         mkItem(KnownField::Lyrics, defaultSelected),
         mkItem(KnownField::SynchronizedLyrics, Qt::Unchecked),
         mkItem(KnownField::Grouping, Qt::Unchecked),
         mkItem(KnownField::RecordLabel, Qt::Unchecked),
         mkItem(KnownField::Cover, defaultSelected),
         mkItem(KnownField::Composer, Qt::Unchecked),
         mkItem(KnownField::Rating, Qt::Unchecked),
         mkItem(KnownField::Description, Qt::Unchecked),
         mkItem(KnownField::Vendor, Qt::Unchecked),
         mkItem(KnownField::ReleaseDate, Qt::Unchecked),
    });
    // clang-format on
}

KnownFieldModel::KnownFieldModel(const QList<ChecklistItem> &items, QObject *parent)
    : ChecklistModel(parent)
{
    setItems(items);
}

QVariant KnownFieldModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    switch (orientation) {
    case Qt::Horizontal:
        switch (role) {
        case Qt::DisplayRole:
            switch (section) {
            case 0:
                return tr("Field");
            default:;
            }
        }
        break;
    default:;
    }
    return QVariant();
}
#endif

} // namespace Settings
