#include "./fieldmapping.h"

#include <tagparser/caseinsensitivecomparer.h>
#include <tagparser/tag.h>

using namespace TagParser;

namespace Cli {
namespace FieldMapping {

static constexpr struct {
    const char *knownDenotation;
    KnownField knownField;
} fieldMapping[] = {
    { "title", KnownField::Title },
    { "album", KnownField::Album },
    { "artist", KnownField::Artist },
    { "genre", KnownField::Genre },
    { "year", KnownField::RecordDate },
    { "comment", KnownField::Comment },
    { "bpm", KnownField::Bpm },
    { "bps", KnownField::Bps },
    { "lyricist", KnownField::Lyricist },
    { "track", KnownField::TrackPosition },
    { "disk", KnownField::DiskPosition },
    { "part", KnownField::PartNumber },
    { "totalparts", KnownField::TotalParts },
    { "encoder", KnownField::Encoder },
    { "recorddate", KnownField::RecordDate },
    { "releasedate", KnownField::ReleaseDate },
    { "performers", KnownField::Performers },
    { "duration", KnownField::Length },
    { "language", KnownField::Language },
    { "encodersettings", KnownField::EncoderSettings },
    { "lyrics", KnownField::Lyrics },
    { "synchronizedlyrics", KnownField::SynchronizedLyrics },
    { "grouping", KnownField::Grouping },
    { "recordlabel", KnownField::RecordLabel },
    { "cover", KnownField::Cover },
    { "composer", KnownField::Composer },
    { "rating", KnownField::Rating },
    { "description", KnownField::Description },
    { "vendor", KnownField::Vendor },
    { "albumartist", KnownField::AlbumArtist },
    { "Subtitle", KnownField::Subtitle },
    { "LeadPerformer", KnownField::LeadPerformer },
    { "Arranger", KnownField::Arranger },
    { "Conductor", KnownField::Conductor },
    { "Director", KnownField::Director },
    { "AssistantDirector", KnownField::AssistantDirector },
    { "DirectorOfPhotography", KnownField::DirectorOfPhotography },
    { "SoundEngineer", KnownField::SoundEngineer },
    { "ArtDirector", KnownField::ArtDirector },
    { "ProductionDesigner", KnownField::ProductionDesigner },
    { "Choregrapher", KnownField::Choregrapher },
    { "CostumeDesigner", KnownField::CostumeDesigner },
    { "Actor", KnownField::Actor },
    { "Character", KnownField::Character },
    { "WrittenBy", KnownField::WrittenBy },
    { "ScreenplayBy", KnownField::ScreenplayBy },
    { "EditedBy", KnownField::EditedBy },
    { "Producer", KnownField::Producer },
    { "Coproducer", KnownField::Coproducer },
    { "ExecutiveProducer", KnownField::ExecutiveProducer },
    { "DistributedBy", KnownField::DistributedBy },
    { "MasteredBy", KnownField::MasteredBy },
    { "EncodedBy", KnownField::EncodedBy },
    { "MixedBy", KnownField::MixedBy },
    { "RemixedBy", KnownField::RemixedBy },
    { "ProductionStudio", KnownField::ProductionStudio },
    { "ThanksTo", KnownField::ThanksTo },
    { "Publisher", KnownField::Publisher },
    { "Mood", KnownField::Mood },
    { "OriginalMediaType", KnownField::OriginalMediaType },
    { "ContentType", KnownField::ContentType },
    { "Subject", KnownField::Subject },
    { "Keywords", KnownField::Keywords },
    { "Summary", KnownField::Summary },
    { "Synopsis", KnownField::Synopsis },
    { "InitialKey", KnownField::InitialKey },
    { "Period", KnownField::Period },
    { "LawRating", KnownField::LawRating },
    { "EncodingDate", KnownField::EncodingDate },
    { "TaggingDate", KnownField::TaggingDate },
    { "OriginalReleaseDate", KnownField::OriginalReleaseDate },
    { "DigitalizationDate", KnownField::DigitalizationDate },
    { "WritingDate", KnownField::WritingDate },
    { "PurchasingDate", KnownField::PurchasingDate },
    { "RecordingLocation", KnownField::RecordingLocation },
    { "CompositionLocation", KnownField::CompositionLocation },
    { "ComposerNationality", KnownField::ComposerNationality },
    { "PlayCounter", KnownField::PlayCounter },
    { "Measure", KnownField::Measure },
    { "Tuning", KnownField::Tuning },
    { "ISRC", KnownField::ISRC },
    { "MCDI", KnownField::MCDI },
    { "ISBN", KnownField::ISBN },
    { "Barcode", KnownField::Barcode },
    { "CatalogNumber", KnownField::CatalogNumber },
    { "LabelCode", KnownField::LabelCode },
    { "LCCN", KnownField::LCCN },
    { "IMDB", KnownField::IMDB },
    { "TMDB", KnownField::TMDB },
    { "TVDB", KnownField::TVDB },
    { "PurchaseItem", KnownField::PurchaseItem },
    { "PurchaseInfo", KnownField::PurchaseInfo },
    { "PurchaseOwner", KnownField::PurchaseOwner },
    { "PurchasePrice", KnownField::PurchasePrice },
    { "PurchaseCurrency", KnownField::PurchaseCurrency },
    { "Copyright", KnownField::Copyright },
    { "ProductionCopyright", KnownField::ProductionCopyright },
    { "License", KnownField::License },
    { "TermsOfUse", KnownField::TermsOfUse },
};

const char *fieldDenotation(TagParser::KnownField knownField)
{
    for (const auto &mapping : fieldMapping) {
        if (mapping.knownField == knownField) {
            return mapping.knownDenotation;
        }
    }
    return nullptr;
}

TagParser::KnownField knownField(const char *fieldDenotation, std::size_t fieldDenotationSize)
{
    const auto d = std::string_view(fieldDenotation, fieldDenotationSize);
    for (const auto &mapping : fieldMapping) {
        const char *mc = mapping.knownDenotation;
        for (auto dc : d) {
            if (!*mc) {
                mc = mapping.knownDenotation;
                break;
            }
            if (static_cast<unsigned char>(dc) != CaseInsensitiveCharComparer::toLower(static_cast<unsigned char>(*mc))) {
                break;
            }
            ++mc;
        }
        if (!*mc) {
            return mapping.knownField;
        }
    }
    return KnownField::Invalid;
}

} // namespace FieldMapping
} // namespace Cli
