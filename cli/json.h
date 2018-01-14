#ifndef CLI_JSON
#define CLI_JSON

#include <reflective-rapidjson/lib/json/serializable.h>

#include <c++utilities/chrono/timespan.h>

#include <unordered_map>

namespace Media {
class MediaFileInfo;
class Tag;
class TagValue;
}

namespace Cli {
namespace Json {

struct TagValue : ReflectiveRapidJSON::JsonSerializable<TagValue> {
    TagValue(const Media::TagValue &tagValue, RAPIDJSON_NAMESPACE::Document::AllocatorType &allocator);

    const char *kind = "undefined";
    const std::string mimeType;
    struct ValueAllowedToMove : RAPIDJSON_NAMESPACE::Value {
    } value;
};

struct TagInfo : ReflectiveRapidJSON::JsonSerializable<TagInfo> {
    TagInfo(const Media::Tag &tag, RAPIDJSON_NAMESPACE::Document::AllocatorType &allocator);

    const char *format = nullptr;
    std::string target;
    std::unordered_map<std::string, std::vector<TagValue>> fields;
};

struct FileInfo : ReflectiveRapidJSON::JsonSerializable<FileInfo> {
    FileInfo(const Media::MediaFileInfo &mediaFileInfo, RAPIDJSON_NAMESPACE::Document::AllocatorType &allocator);

    std::string fileName;
    std::size_t size;
    const char *mimeType;
    std::vector<TagInfo> tags;
    std::string formatSummary;
    ChronoUtilities::TimeSpan duration;
};

}
}

#endif // CLI_JSON
