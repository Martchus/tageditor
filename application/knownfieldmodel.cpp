#include "./knownfieldmodel.h"

#include <tagparser/tag.h>

using namespace Models;
using namespace Media;

namespace Settings {

/*
    TRANSLATOR Settings::KnownFieldModel
    Necessary for lupdate.
*/

const char *KnownFieldModel::fieldName(KnownField field)
{
    switch(field) {
    case KnownField::Title: return QT_TR_NOOP("Title");
    case KnownField::Album: return QT_TR_NOOP("Album");
    case KnownField::Artist: return QT_TR_NOOP("Artist");
    case KnownField::Genre: return QT_TR_NOOP("Genre");
    case KnownField::Year: return QT_TR_NOOP("Year");
    case KnownField::Comment: return QT_TR_NOOP("Comment");
    case KnownField::Bpm: return QT_TR_NOOP("BPM");
    case KnownField::Bps: return QT_TR_NOOP("BPS");
    case KnownField::Lyricist: return QT_TR_NOOP("Lyricist");
    case KnownField::TrackPosition: return QT_TR_NOOP("Track");
    case KnownField::DiskPosition: return QT_TR_NOOP("Disk");
    case KnownField::PartNumber: return QT_TR_NOOP("Part");
    case KnownField::TotalParts: return QT_TR_NOOP("Total parts");
    case KnownField::Encoder: return QT_TR_NOOP("Encoder");
    case KnownField::RecordDate: return QT_TR_NOOP("Record date");
    case KnownField::Performers: return QT_TR_NOOP("Performers");
    case KnownField::Length: return QT_TR_NOOP("Duration");
    case KnownField::Language: return QT_TR_NOOP("Language");
    case KnownField::EncoderSettings: return QT_TR_NOOP("Encoder settings");
    case KnownField::Lyrics: return QT_TR_NOOP("Lyrics");
    case KnownField::SynchronizedLyrics: return QT_TR_NOOP("Synchronized lyrics");
    case KnownField::Grouping: return QT_TR_NOOP("Grouping");
    case KnownField::RecordLabel:return QT_TR_NOOP("Record label");
    case KnownField::Cover: return QT_TR_NOOP("Cover");
    case KnownField::Composer: return QT_TR_NOOP("Composer");
    case KnownField::Rating: return QT_TR_NOOP("Rating");
    case KnownField::Description: return QT_TR_NOOP("Description");
    case KnownField::Vendor: return QT_TR_NOOP("Vendor");
    default: return "";
    }
}

QString KnownFieldModel::translatedFieldName(KnownField field)
{
    return tr(fieldName(field));
}

QString KnownFieldModel::labelForId(const QVariant &id) const
{
    return translatedFieldName(static_cast<KnownField>(id.toInt()));
}

inline ChecklistItem KnownFieldModel::mkItem(KnownField field, Qt::CheckState checkState)
{
    return ChecklistItem(static_cast<int>(field), translatedFieldName(field), checkState);
}

KnownFieldModel::KnownFieldModel(QObject *parent, DefaultSelection defaultSelection) :
    ChecklistModel(parent)
{
    QList<ChecklistItem> items;
    Qt::CheckState defaultSelected = defaultSelection == DefaultSelection::CommonFields ? Qt::Checked : Qt::Unchecked;
    items << mkItem(KnownField::Title, defaultSelected);
    items << mkItem(KnownField::Album, defaultSelected);
    items << mkItem(KnownField::Artist, defaultSelected);
    items << mkItem(KnownField::Genre, defaultSelected);
    items << mkItem(KnownField::Year, defaultSelected);
    items << mkItem(KnownField::Comment, defaultSelected);
    items << mkItem(KnownField::Bpm, Qt::Unchecked);
    items << mkItem(KnownField::Bps, Qt::Unchecked);
    items << mkItem(KnownField::Lyricist, defaultSelected);
    items << mkItem(KnownField::TrackPosition, defaultSelected);
    items << mkItem(KnownField::DiskPosition, defaultSelected);
    items << mkItem(KnownField::PartNumber, defaultSelected);
    items << mkItem(KnownField::TotalParts, defaultSelected);
    items << mkItem(KnownField::Encoder, defaultSelected);
    items << mkItem(KnownField::RecordDate, defaultSelected);
    items << mkItem(KnownField::Performers, defaultSelected);
    items << mkItem(KnownField::Length, Qt::Unchecked);
    items << mkItem(KnownField::Language, Qt::Unchecked);
    items << mkItem(KnownField::EncoderSettings, Qt::Unchecked);
    items << mkItem(KnownField::Lyrics, defaultSelected);
    items << mkItem(KnownField::SynchronizedLyrics, Qt::Unchecked);
    items << mkItem(KnownField::Grouping, Qt::Unchecked);
    items << mkItem(KnownField::RecordLabel, Qt::Unchecked);
    items << mkItem(KnownField::Cover, defaultSelected);
    items << mkItem(KnownField::Composer, Qt::Unchecked);
    items << mkItem(KnownField::Rating, Qt::Unchecked);
    items << mkItem(KnownField::Description, Qt::Unchecked);
    setItems(items);
}

QVariant KnownFieldModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    switch(orientation) {
    case Qt::Horizontal:
        switch(role) {
        case Qt::DisplayRole:
            switch(section) {
            case 0:
                return tr("Field");
            default:
                ;
            }
        }
        break;
    default:
        ;
    }
    return QVariant();
}

}
