#include "./json.h"
#include "./fieldmapping.h"

#include <reflective-rapidjson/lib/json/reflector-chronoutilities.h>

#include <tagparser/mediafileinfo.h>
#include <tagparser/tag.h>

#include <c++utilities/conversion/stringconversion.h>

using namespace std;
using namespace Media;

namespace ReflectiveRapidJSON {
namespace JsonReflector {

/*!
 * \brief Allows moving a RAPIDJSON_NAMESPACE::Value wrapped as Cli::Json::TagValue::ValueAllowedToMove for serialization.
 * \remarks The const-cast and move operation are ok in this particular case, since the object to be serialized
 *          is not const and not used anymore after serialization.
 * \todo Add an overload for &&reflectable to Reflective RapidJSON.
 */
template <>
inline void push<Cli::Json::TagValue::ValueAllowedToMove>(
    const Cli::Json::TagValue::ValueAllowedToMove &reflectable, RAPIDJSON_NAMESPACE::Value &value, RAPIDJSON_NAMESPACE::Document::AllocatorType &)
{
    value = const_cast<Cli::Json::TagValue::ValueAllowedToMove &>(reflectable).Move();
}

} // namespace JsonReflector
} // namespace ReflectiveRapidJSON

namespace Cli {
namespace Json {

TagValue::TagValue(const Media::TagValue &tagValue, RAPIDJSON_NAMESPACE::Document::AllocatorType &allocator)
    : mimeType(tagValue.mimeType())
{
    if (tagValue.isEmpty()) {
        value.SetNull();
        return;
    }
    try {
        switch(tagValue.type()) {
        case TagDataType::Text:
        case TagDataType::StandardGenreIndex:
            ReflectiveRapidJSON::JsonReflector::push(tagValue.toString(TagTextEncoding::Utf8), value, allocator);
            kind = "text";
            break;
        case TagDataType::Integer:
            ReflectiveRapidJSON::JsonReflector::push(tagValue.toInteger(), value, allocator);
            kind = "integer";
            break;
        case TagDataType::PositionInSet: {
            const auto position(tagValue.toPositionInSet());
            value.SetObject();
            if (position.position()) {
                value.AddMember("position", RAPIDJSON_NAMESPACE::Value(position.position()), allocator);
            }
            if (position.total()) {
                value.AddMember("total", RAPIDJSON_NAMESPACE::Value(position.total()), allocator);
            }
            kind = "position";
            break;
         }
        case TagDataType::TimeSpan:
            ReflectiveRapidJSON::JsonReflector::push(tagValue.toTimeSpan(), value, allocator);
            break;
        case TagDataType::DateTime:
            ReflectiveRapidJSON::JsonReflector::push(tagValue.toDateTime(), value, allocator);
            break;
        case TagDataType::Picture:
            if (tagValue.dataSize() > (1024 * 1024)) {
                throw ConversionUtilities::ConversionException("size is too big");
            }
            ReflectiveRapidJSON::JsonReflector::push(ConversionUtilities::encodeBase64(reinterpret_cast<const byte *>(tagValue.dataPointer()), static_cast<uint32>(tagValue.dataSize())), value, allocator);
            kind = "picture";
            break;
        case TagDataType::Binary:
            if (tagValue.dataSize() > (1024 * 1024)) {
                throw ConversionUtilities::ConversionException("size is too big");
            }
            ReflectiveRapidJSON::JsonReflector::push(ConversionUtilities::encodeBase64(reinterpret_cast<const byte *>(tagValue.dataPointer()), static_cast<uint32>(tagValue.dataSize())), value, allocator);
            kind = "binary";
            break;
        default:
            value.SetNull();
        }
    } catch (const ConversionUtilities::ConversionException &e) {
        ReflectiveRapidJSON::JsonReflector::push(e.what(), value, allocator);
        kind = "error";
    }
}

TagInfo::TagInfo(const Tag &tag, RAPIDJSON_NAMESPACE::Document::AllocatorType &allocator)
    : format(tag.typeName())
    , target(tag.targetString())
{
    for (auto field = firstKnownField; field != KnownField::Invalid; field = nextKnownField(field)) {
        const auto &tagValues(tag.values(field));
        if (tagValues.empty()) {
            continue;
        }
        std::vector<TagValue> valueObjects;
        valueObjects.reserve(tagValues.size());
        for (const auto *tagValue : tagValues) {
            valueObjects.emplace_back(*tagValue, allocator);
        }
        fields.insert(make_pair(FieldMapping::fieldDenotation(field), move(valueObjects)));
    }
}

FileInfo::FileInfo(const Media::MediaFileInfo &mediaFileInfo, RAPIDJSON_NAMESPACE::Document::AllocatorType &allocator)
    : fileName(mediaFileInfo.fileName())
    , size(mediaFileInfo.size())
    , mimeType(mediaFileInfo.mimeType())
    , formatSummary(mediaFileInfo.technicalSummary())
    , duration(mediaFileInfo.duration())
{
    for (const Tag *tag : mediaFileInfo.tags()) {
        tags.emplace_back(*tag, allocator);
    }
}

}

}

#include "reflection/json.h"
