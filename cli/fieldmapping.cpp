#include "./fieldmapping.h"

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
    { "year", KnownField::Year },
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
    for (const auto &mapping : fieldMapping) {
        if (!strncmp(fieldDenotation, mapping.knownDenotation, fieldDenotationSize)) {
            return mapping.knownField;
        }
    }
    return KnownField::Invalid;
}

} // namespace FieldMapping
} // namespace Cli
