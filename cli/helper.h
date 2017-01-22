#ifndef CLI_HELPER
#define CLI_HELPER

#include "../application/knownfieldmodel.h"

#include <tagparser/tag.h>

#include <c++utilities/application/commandlineutils.h>
#include <c++utilities/chrono/datetime.h>
#include <c++utilities/chrono/timespan.h>
#include <c++utilities/conversion/stringconversion.h>

#include <vector>
#include <unordered_map>
#include <functional>

namespace ApplicationUtilities {
class Argument;
}

namespace Media {
class MediaFileInfo;
enum class TagUsage;
enum class ElementPosition;
}

using namespace Media;

namespace Cli {

// define enums, operators and structs to handle specified field denotations

enum class DenotationType
{
    Normal,
    Increment,
    File
};

inline TagType operator| (TagType lhs, TagType rhs)
{
    return static_cast<TagType>(static_cast<unsigned int>(lhs) | static_cast<unsigned int>(rhs));
}

inline TagType operator& (TagType lhs, TagType rhs)
{
    return static_cast<TagType>(static_cast<unsigned int>(lhs) & static_cast<unsigned int>(rhs));
}

inline TagType &operator|= (TagType &lhs, TagType rhs)
{
    return lhs = static_cast<TagType>(static_cast<unsigned int>(lhs) | static_cast<unsigned int>(rhs));
}

class FieldId
{
public:
    FieldId(KnownField m_knownField = KnownField::Invalid);
    static FieldId fromDenotation(const char *denotation, std::size_t denotationSize);
    bool operator ==(const FieldId &other) const;
    KnownField knownField() const;
    const char *nativeField() const;
    const char *name() const;
    std::vector<const TagValue *> values(const Tag *tag, TagType tagType) const;
    bool setValues(Tag *tag, TagType tagType, const std::vector<TagValue> &values) const;

private:
    typedef std::function<std::vector<const TagValue *>(const Tag *, TagType)> GetValuesForNativeFieldType;
    typedef std::function<bool(Tag *, TagType, const std::vector<TagValue> &)> SetValuesForNativeFieldType;
    FieldId(const char *m_nativeField, const GetValuesForNativeFieldType &valuesForNativeField, const SetValuesForNativeFieldType &setValuesForNativeField);
    template<class ConcreteTag>
    static FieldId fromNativeField(const char *nativeFieldId, std::size_t nativeFieldIdSize = std::string::npos);

    KnownField m_knownField;
    const char *m_nativeField;
    GetValuesForNativeFieldType m_valuesForNativeField;
    SetValuesForNativeFieldType m_setValuesForNativeField;
};

inline FieldId::FieldId(KnownField knownField) :
    m_knownField(knownField),
    m_nativeField(nullptr)
{}

inline bool FieldId::operator ==(const FieldId &other) const
{
    return m_knownField == other.m_knownField && m_nativeField == other.m_nativeField;
}

inline KnownField FieldId::knownField() const
{
    return m_knownField;
}

inline const char *FieldId::nativeField() const
{
    return m_nativeField;
}

inline const char *FieldId::name() const
{
    return m_nativeField ? m_nativeField : Settings::KnownFieldModel::fieldName(m_knownField);
}

struct FieldScope
{
    FieldScope(KnownField field = KnownField::Invalid, TagType tagType = TagType::Unspecified, TagTarget tagTarget = TagTarget());
    bool operator ==(const FieldScope &other) const;
    FieldId field;
    TagType tagType;
    TagTarget tagTarget;
};

inline FieldScope::FieldScope(KnownField field, TagType tagType, TagTarget tagTarget) :
    field(field),
    tagType(tagType),
    tagTarget(tagTarget)
{}

inline bool FieldScope::operator ==(const FieldScope &other) const
{
    return field == other.field && tagType == other.tagType && tagTarget == other.tagTarget;
}

struct FieldValue
{
    FieldValue(DenotationType type, unsigned int fileIndex, const char *value);
    DenotationType type;
    unsigned int fileIndex;
    std::string value;
};

inline FieldValue::FieldValue(DenotationType type, unsigned int fileIndex, const char *value) :
    type(type),
    fileIndex(fileIndex),
    value(value)
{}

}

// define hash functions for custom data types

namespace std {

using namespace Cli;

template <> struct hash<KnownField>
{
    std::size_t operator()(const KnownField &ids) const
    {
        using type = typename std::underlying_type<KnownField>::type;
        return std::hash<type>()(static_cast<type>(ids));
    }
};

template <> struct hash<TagType>
{
    std::size_t operator()(const TagType &ids) const
    {
        using type = typename std::underlying_type<TagType>::type;
        return std::hash<type>()(static_cast<type>(ids));
    }
};

template <> struct hash<TagTarget::IdContainerType>
{
    std::size_t operator()(const TagTarget::IdContainerType &ids) const
    {
        using std::hash;
        auto seed = ids.size();
        for(auto id : ids) {
            seed ^= id + 0x9e3779b9 + (seed << 6) + (seed >> 2);
        }
        return seed;
    }
};

template <> struct hash<TagTarget>
{
    std::size_t operator()(const TagTarget &target) const
    {
        using std::hash;
        return ((hash<uint64>()(target.level())
                 ^ (hash<TagTarget::IdContainerType>()(target.tracks()) << 1)) >> 1)
                ^ (hash<TagTarget::IdContainerType>()(target.attachments()) << 1);
    }
};

template <> struct hash<FieldId>
{
    std::size_t operator()(const FieldId &id) const
    {
        using std::hash;
        return (hash<KnownField>()(id.knownField())
                 ^ (hash<const char *>()(id.nativeField()) << 1));
    }
};

template <> struct hash<FieldScope>
{
    std::size_t operator()(const FieldScope &scope) const
    {
        using std::hash;
        return ((hash<FieldId>()(scope.field)
                 ^ (hash<TagType>()(scope.tagType) << 1)) >> 1)
                ^ (hash<TagTarget>()(scope.tagTarget) << 1);
    }
};

}

namespace Cli {

typedef std::vector<FieldValue> FieldValues;
typedef std::unordered_map<FieldScope, FieldValues> FieldDenotations;

// declare/define actual helpers

constexpr bool isDigit(char c)
{
    return c >= '0' && c <= '9';
}

std::string incremented(const std::string &str, unsigned int toIncrement = 1);

void printNotifications(NotificationList &notifications, const char *head = nullptr, bool beVerbose = false);
void printNotifications(const MediaFileInfo &fileInfo, const char *head = nullptr, bool beVerbose = false);
void printProperty(const char *propName, const char *value, const char *suffix = nullptr, ApplicationUtilities::Indentation indentation = 4);
void printProperty(const char *propName, ElementPosition elementPosition, const char *suffix = nullptr, ApplicationUtilities::Indentation indentation = 4);

inline void printProperty(const char *propName, const std::string &value, const char *suffix = nullptr, ApplicationUtilities::Indentation indentation = 4)
{
    printProperty(propName, value.data(), suffix, indentation);
}

inline void printProperty(const char *propName, ChronoUtilities::TimeSpan timeSpan, const char *suffix = nullptr, ApplicationUtilities::Indentation indentation = 4)
{
    if(!timeSpan.isNull()) {
        printProperty(propName, timeSpan.toString(ChronoUtilities::TimeSpanOutputFormat::WithMeasures), suffix, indentation);
    }
}

inline void printProperty(const char *propName, ChronoUtilities::DateTime dateTime, const char *suffix = nullptr, ApplicationUtilities::Indentation indentation = 4)
{
    if(!dateTime.isNull()) {
        printProperty(propName, dateTime.toString(), suffix, indentation);
    }
}

template<typename intType>
inline void printProperty(const char *propName, const intType value, const char *suffix = nullptr, bool force = false, ApplicationUtilities::Indentation indentation = 4)
{
    if(value != 0 || force) {
        printProperty(propName, ConversionUtilities::numberToString<intType>(value), suffix, indentation);
    }
}

void printField(const FieldScope &scope, const Tag *tag, TagType tagType, bool skipEmpty);

TagUsage parseUsageDenotation(const ApplicationUtilities::Argument &usageArg, TagUsage defaultUsage);
TagTextEncoding parseEncodingDenotation(const ApplicationUtilities::Argument &encodingArg, TagTextEncoding defaultEncoding);
ElementPosition parsePositionDenotation(const ApplicationUtilities::Argument &posArg, const ApplicationUtilities::Argument &valueArg, ElementPosition defaultPos);
uint64 parseUInt64(const ApplicationUtilities::Argument &arg, uint64 defaultValue);
TagTarget::IdContainerType parseIds(const std::string &concatenatedIds);
bool applyTargetConfiguration(TagTarget &target, const std::string &configStr);
FieldDenotations parseFieldDenotations(const ApplicationUtilities::Argument &fieldsArg, bool readOnly);

}

#endif // CLI_HELPER
