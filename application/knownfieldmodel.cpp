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
    case KnownField::Subtitle:
        return QT_TR_NOOP("Subtitle");
    case KnownField::LeadPerformer:
        return QT_TR_NOOP("Lead performer");
    case KnownField::Arranger:
        return QT_TR_NOOP("Arranger");
    case KnownField::Conductor:
        return QT_TR_NOOP("Conductor");
    case KnownField::Director:
        return QT_TR_NOOP("Director");
    case KnownField::AssistantDirector:
        return QT_TR_NOOP("Assistant directory");
    case KnownField::DirectorOfPhotography:
        return QT_TR_NOOP("Director of photography");
    case KnownField::SoundEngineer:
        return QT_TR_NOOP("Sounc engineer");
    case KnownField::ArtDirector:
        return QT_TR_NOOP("Art director");
    case KnownField::ProductionDesigner:
        return QT_TR_NOOP("Production designer");
    case KnownField::Choregrapher:
        return QT_TR_NOOP("Choregrapher");
    case KnownField::CostumeDesigner:
        return QT_TR_NOOP("Costume designer");
    case KnownField::Actor:
        return QT_TR_NOOP("Actor");
    case KnownField::Character:
        return QT_TR_NOOP("Character");
    case KnownField::WrittenBy:
        return QT_TR_NOOP("Written by");
    case KnownField::ScreenplayBy:
        return QT_TR_NOOP("Screenplay by");
    case KnownField::EditedBy:
        return QT_TR_NOOP("Edited by");
    case KnownField::Producer:
        return QT_TR_NOOP("Producer");
    case KnownField::Coproducer:
        return QT_TR_NOOP("Coproducer");
    case KnownField::ExecutiveProducer:
        return QT_TR_NOOP("Executive producer");
    case KnownField::DistributedBy:
        return QT_TR_NOOP("Distributed by");
    case KnownField::MasteredBy:
        return QT_TR_NOOP("Mastered by");
    case KnownField::EncodedBy:
        return QT_TR_NOOP("Encoded by");
    case KnownField::MixedBy:
        return QT_TR_NOOP("Mixed by");
    case KnownField::RemixedBy:
        return QT_TR_NOOP("Remixed by");
    case KnownField::ProductionStudio:
        return QT_TR_NOOP("Production studio");
    case KnownField::ThanksTo:
        return QT_TR_NOOP("Thanks to");
    case KnownField::Publisher:
        return QT_TR_NOOP("Publisher");
    case KnownField::Mood:
        return QT_TR_NOOP("Mood");
    case KnownField::OriginalMediaType:
        return QT_TR_NOOP("Original media-type");
    case KnownField::ContentType:
        return QT_TR_NOOP("Content-type");
    case KnownField::Subject:
        return QT_TR_NOOP("Subject");
    case KnownField::Keywords:
        return QT_TR_NOOP("Keywords");
    case KnownField::Summary:
        return QT_TR_NOOP("Summary");
    case KnownField::Synopsis:
        return QT_TR_NOOP("Synopsis");
    case KnownField::InitialKey:
        return QT_TR_NOOP("Initial key");
    case KnownField::Period:
        return QT_TR_NOOP("Period");
    case KnownField::LawRating:
        return QT_TR_NOOP("Law rating");
    case KnownField::EncodingDate:
        return QT_TR_NOOP("Encoding date");
    case KnownField::TaggingDate:
        return QT_TR_NOOP("Tagging date");
    case KnownField::DigitalizationDate:
        return QT_TR_NOOP("Digitalization date");
    case KnownField::WritingDate:
        return QT_TR_NOOP("Writing date");
    case KnownField::PurchasingDate:
        return QT_TR_NOOP("Purchasing date");
    case KnownField::RecordingLocation:
        return QT_TR_NOOP("Recording location");
    case KnownField::CompositionLocation:
        return QT_TR_NOOP("Composition location");
    case KnownField::ComposerNationality:
        return QT_TR_NOOP("Composer nationality");
    case KnownField::PlayCounter:
        return QT_TR_NOOP("Play counter");
    case KnownField::Measure:
        return QT_TR_NOOP("Measure");
    case KnownField::Tuning:
        return QT_TR_NOOP("Tuning");
    case KnownField::ISRC:
        return QT_TR_NOOP("ISRC");
    case KnownField::MCDI:
        return QT_TR_NOOP("MCDI");
    case KnownField::ISBN:
        return QT_TR_NOOP("ISBN");
    case KnownField::Barcode:
        return QT_TR_NOOP("Barcode");
    case KnownField::CatalogNumber:
        return QT_TR_NOOP("Catalog number");
    case KnownField::LabelCode:
        return QT_TR_NOOP("Label code");
    case KnownField::LCCN:
        return QT_TR_NOOP("LCCN");
    case KnownField::IMDB:
        return QT_TR_NOOP("IMDB-ID");
    case KnownField::TMDB:
        return QT_TR_NOOP("TMDB-ID");
    case KnownField::TVDB:
        return QT_TR_NOOP("TVDB-ID");
    case KnownField::PurchaseItem:
        return QT_TR_NOOP("Purchase item");
    case KnownField::PurchaseInfo:
        return QT_TR_NOOP("Purchase info");
    case KnownField::PurchaseOwner:
        return QT_TR_NOOP("Purchase owner");
    case KnownField::PurchasePrice:
        return QT_TR_NOOP("Purchase price");
    case KnownField::PurchaseCurrency:
        return QT_TR_NOOP("Purchase currency");
    case KnownField::Copyright:
        return QT_TR_NOOP("Copyright");
    case KnownField::ProductionCopyright:
        return QT_TR_NOOP("Production copyright");
    case KnownField::License:
        return QT_TR_NOOP("License");
    case KnownField::TermsOfUse:
        return QT_TR_NOOP("Terms of use");
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
         mkItem(KnownField::ReleaseDate, Qt::Checked),
         mkItem(KnownField::Subtitle, Qt::Unchecked),
         mkItem(KnownField::LeadPerformer, Qt::Unchecked),
         mkItem(KnownField::Arranger, Qt::Unchecked),
         mkItem(KnownField::Conductor, Qt::Unchecked),
         mkItem(KnownField::Director, Qt::Unchecked),
         mkItem(KnownField::AssistantDirector, Qt::Unchecked),
         mkItem(KnownField::DirectorOfPhotography, Qt::Unchecked),
         mkItem(KnownField::SoundEngineer, Qt::Unchecked),
         mkItem(KnownField::ArtDirector, Qt::Unchecked),
         mkItem(KnownField::ProductionDesigner, Qt::Unchecked),
         mkItem(KnownField::Choregrapher, Qt::Unchecked),
         mkItem(KnownField::CostumeDesigner, Qt::Unchecked),
         mkItem(KnownField::Actor, Qt::Unchecked),
         mkItem(KnownField::Character, Qt::Unchecked),
         mkItem(KnownField::WrittenBy, Qt::Unchecked),
         mkItem(KnownField::ScreenplayBy, Qt::Unchecked),
         mkItem(KnownField::EditedBy, Qt::Unchecked),
         mkItem(KnownField::Producer, Qt::Unchecked),
         mkItem(KnownField::Coproducer, Qt::Unchecked),
         mkItem(KnownField::ExecutiveProducer, Qt::Unchecked),
         mkItem(KnownField::DistributedBy, Qt::Unchecked),
         mkItem(KnownField::MasteredBy, Qt::Unchecked),
         mkItem(KnownField::EncodedBy, Qt::Unchecked),
         mkItem(KnownField::MixedBy, Qt::Unchecked),
         mkItem(KnownField::RemixedBy, Qt::Unchecked),
         mkItem(KnownField::ProductionStudio, Qt::Unchecked),
         mkItem(KnownField::ThanksTo, Qt::Unchecked),
         mkItem(KnownField::Publisher, Qt::Unchecked),
         mkItem(KnownField::Mood, Qt::Unchecked),
         mkItem(KnownField::OriginalMediaType, Qt::Unchecked),
         mkItem(KnownField::ContentType, Qt::Unchecked),
         mkItem(KnownField::Subject, Qt::Unchecked),
         mkItem(KnownField::Keywords, Qt::Unchecked),
         mkItem(KnownField::Summary, Qt::Unchecked),
         mkItem(KnownField::Synopsis, Qt::Unchecked),
         mkItem(KnownField::InitialKey, Qt::Unchecked),
         mkItem(KnownField::Period, Qt::Unchecked),
         mkItem(KnownField::LawRating, Qt::Unchecked),
         mkItem(KnownField::EncodingDate, Qt::Unchecked),
         mkItem(KnownField::TaggingDate, Qt::Unchecked),
         mkItem(KnownField::OriginalReleaseDate, Qt::Unchecked),
         mkItem(KnownField::DigitalizationDate, Qt::Unchecked),
         mkItem(KnownField::WritingDate, Qt::Unchecked),
         mkItem(KnownField::PurchasingDate, Qt::Unchecked),
         mkItem(KnownField::RecordingLocation, Qt::Unchecked),
         mkItem(KnownField::CompositionLocation, Qt::Unchecked),
         mkItem(KnownField::ComposerNationality, Qt::Unchecked),
         mkItem(KnownField::PlayCounter, Qt::Unchecked),
         mkItem(KnownField::Measure, Qt::Unchecked),
         mkItem(KnownField::Tuning, Qt::Unchecked),
         mkItem(KnownField::ISBN, Qt::Unchecked),
         mkItem(KnownField::Barcode, Qt::Unchecked),
         mkItem(KnownField::CatalogNumber, Qt::Unchecked),
         mkItem(KnownField::LabelCode, Qt::Unchecked),
         mkItem(KnownField::LCCN, Qt::Unchecked),
         mkItem(KnownField::IMDB, Qt::Unchecked),
         mkItem(KnownField::TMDB, Qt::Unchecked),
         mkItem(KnownField::TVDB, Qt::Unchecked),
         mkItem(KnownField::PurchaseItem, Qt::Unchecked),
         mkItem(KnownField::PurchaseInfo, Qt::Unchecked),
         mkItem(KnownField::PurchaseOwner, Qt::Unchecked),
         mkItem(KnownField::PurchasePrice, Qt::Unchecked),
         mkItem(KnownField::PurchaseCurrency, Qt::Unchecked),
         mkItem(KnownField::Copyright, Qt::Unchecked),
         mkItem(KnownField::ProductionCopyright, Qt::Unchecked),
         mkItem(KnownField::License, Qt::Unchecked),
         mkItem(KnownField::TermsOfUse, Qt::Unchecked),
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
