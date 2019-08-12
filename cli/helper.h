#ifndef CLI_HELPER
#define CLI_HELPER

#include "../application/knownfieldmodel.h"

#include <tagparser/tag.h>

#include <c++utilities/application/commandlineutils.h>
#include <c++utilities/chrono/datetime.h>
#include <c++utilities/chrono/timespan.h>
#include <c++utilities/conversion/stringconversion.h>
#include <c++utilities/misc/flagenumclass.h>
#include <c++utilities/misc/traits.h>

#include <functional>
#include <type_traits>
#include <unordered_map>
#include <vector>

namespace CppUtilities {
class Argument;
}

namespace TagParser {
class MediaFileInfo;
class Diagnostics;
class AbortableProgressFeedback;
enum class TagUsage;
enum class ElementPosition;
} // namespace TagParser

using namespace TagParser;

namespace Cli {

// define enums, operators and structs to handle specified field denotations

enum class DenotationType { Normal, Increment, File };

} // namespace Cli

CPP_UTILITIES_MARK_FLAG_ENUM_CLASS(Cli, Cli::DenotationType)
CPP_UTILITIES_MARK_FLAG_ENUM_CLASS(TagParser, TagParser::TagType)

namespace Cli {

class FieldId {
    friend struct std::hash<FieldId>;

public:
    FieldId(KnownField m_knownField = KnownField::Invalid, const char *denotation = nullptr, std::size_t denotationSize = 0);
    static FieldId fromTagDenotation(const char *denotation, std::size_t denotationSize);
    static FieldId fromTrackDenotation(const char *denotation, std::size_t denotationSize);
    bool operator==(const FieldId &other) const;
    KnownField knownField() const;
    const char *name() const;
    bool denotes(const char *knownDenotation) const;
    const std::string &denotation() const;
    std::pair<std::vector<const TagValue *>, bool> values(const Tag *tag, TagType tagType) const;
    bool setValues(Tag *tag, TagType tagType, const std::vector<TagValue> &values) const;

private:
    using GetValuesForNativeFieldType = std::function<std::pair<std::vector<const TagValue *>, bool>(const Tag *, TagType)>;
    using SetValuesForNativeFieldType = std::function<bool(Tag *, TagType, const std::vector<TagValue> &)>;
    FieldId(const char *nativeField, std::size_t nativeFieldSize, const GetValuesForNativeFieldType &valuesForNativeField,
        const SetValuesForNativeFieldType &setValuesForNativeField);
    template <class ConcreteTag, TagType tagTypeMask = ConcreteTag::tagType>
    static FieldId fromNativeField(const char *nativeFieldId, std::size_t nativeFieldIdSize);

    KnownField m_knownField;
    std::string m_denotation;
    std::string m_nativeField;
    GetValuesForNativeFieldType m_valuesForNativeField;
    SetValuesForNativeFieldType m_setValuesForNativeField;
};

inline FieldId::FieldId(KnownField knownField, const char *denotation, std::size_t denotationSize)
    : m_knownField(knownField)
    , m_denotation(denotation, denotationSize)
{
}

inline bool FieldId::operator==(const FieldId &other) const
{
    return (m_knownField == other.m_knownField || m_denotation == other.m_denotation) && m_nativeField == other.m_nativeField;
}

inline KnownField FieldId::knownField() const
{
    return m_knownField;
}

inline const char *FieldId::name() const
{
    return !m_nativeField.empty() ? m_nativeField.data() : Settings::KnownFieldModel::fieldName(m_knownField);
}

inline bool FieldId::denotes(const char *knownDenotation) const
{
    return !std::strncmp(m_denotation.data(), knownDenotation, m_denotation.size());
}

inline const std::string &FieldId::denotation() const
{
    return m_denotation;
}

struct FieldScope {
    FieldScope(KnownField field = KnownField::Invalid, TagType tagType = TagType::Unspecified, TagTarget tagTarget = TagTarget());
    bool operator==(const FieldScope &other) const;
    bool isTrack() const;

    FieldId field;
    TagType tagType;
    TagTarget tagTarget;
    bool allTracks;
    std::vector<std::uint64_t> trackIds;
};

inline FieldScope::FieldScope(KnownField field, TagType tagType, TagTarget tagTarget)
    : field(field)
    , tagType(tagType)
    , tagTarget(tagTarget)
    , allTracks(false)
{
}

inline bool FieldScope::operator==(const FieldScope &other) const
{
    return field == other.field && tagType == other.tagType && tagTarget == other.tagTarget;
}

inline bool FieldScope::isTrack() const
{
    return allTracks || !trackIds.empty();
}

struct FieldValue {
    FieldValue(DenotationType type, unsigned int fileIndex, const char *value);
    DenotationType type;
    unsigned int fileIndex;
    std::string value;
};

inline FieldValue::FieldValue(DenotationType type, unsigned int fileIndex, const char *value)
    : type(type)
    , fileIndex(fileIndex)
    , value(value)
{
}

class InterruptHandler {
public:
    InterruptHandler(std::function<void()> handler);
    ~InterruptHandler();

private:
    static void handler(int signum);

    static std::function<void()> s_handler;
    static bool s_handlerRegistered;
};

} // namespace Cli

// define hash functions for custom data types

namespace std {

using namespace Cli;

template <> struct hash<KnownField> {
    std::size_t operator()(const KnownField &ids) const
    {
        using type = typename std::underlying_type<KnownField>::type;
        return std::hash<type>()(static_cast<type>(ids));
    }
};

template <> struct hash<TagType> {
    std::size_t operator()(const TagType &ids) const
    {
        using type = typename std::underlying_type<TagType>::type;
        return std::hash<type>()(static_cast<type>(ids));
    }
};

template <> struct hash<TagTarget::IdContainerType> {
    std::size_t operator()(const TagTarget::IdContainerType &ids) const
    {
        using std::hash;
        auto seed = ids.size();
        for (auto id : ids) {
            seed ^= id + 0x9e3779b9 + (seed << 6) + (seed >> 2);
        }
        return seed;
    }
};

template <> struct hash<TagTarget> {
    std::size_t operator()(const TagTarget &target) const
    {
        using std::hash;
        return ((hash<std::uint64_t>()(target.level()) ^ (hash<TagTarget::IdContainerType>()(target.tracks()) << 1)) >> 1)
            ^ (hash<TagTarget::IdContainerType>()(target.attachments()) << 1);
    }
};

template <> struct hash<FieldId> {
    std::size_t operator()(const FieldId &id) const
    {
        using std::hash;
        return ((id.knownField() != KnownField::Invalid) ? hash<KnownField>()(id.knownField()) : hash<string>()(id.denotation()))
            ^ (hash<string>()(id.m_nativeField) << 1);
    }
};

template <> struct hash<FieldScope> {
    std::size_t operator()(const FieldScope &scope) const
    {
        using std::hash;
        return (hash<FieldId>()(scope.field) ^ (hash<TagType>()(scope.tagType) << 1) >> 1)
            ^ (hash<TagTarget>()(scope.tagTarget) ^ (static_cast<unsigned long>(scope.allTracks) << 4)
                ^ (hash<vector<std::uint64_t>>()(scope.trackIds) << 1) >> 1);
    }
};

} // namespace std

namespace Cli {

struct FieldValues {
    std::vector<FieldValue> allValues;
    std::vector<FieldValue *> relevantValues;
};
using FieldDenotations = std::unordered_map<FieldScope, FieldValues>;

// declare/define actual helpers

constexpr bool isDigit(char c)
{
    return c >= '0' && c <= '9';
}

std::string incremented(const std::string &str, unsigned int toIncrement = 1);

void printDiagMessages(const TagParser::Diagnostics &diag, const char *head = nullptr, bool beVerbose = false);
void printProperty(const char *propName, const char *value, const char *suffix = nullptr, CppUtilities::Indentation indentation = 4);
void printProperty(const char *propName, ElementPosition elementPosition, const char *suffix = nullptr, CppUtilities::Indentation indentation = 4);

inline void printProperty(const char *propName, const std::string &value, const char *suffix = nullptr, CppUtilities::Indentation indentation = 4)
{
    printProperty(propName, value.data(), suffix, indentation);
}

extern CppUtilities::TimeSpanOutputFormat timeSpanOutputFormat;

inline void printProperty(
    const char *propName, CppUtilities::TimeSpan timeSpan, const char *suffix = nullptr, CppUtilities::Indentation indentation = 4)
{
    if (!timeSpan.isNull()) {
        printProperty(propName, timeSpan.toString(timeSpanOutputFormat), suffix, indentation);
    }
}

inline void printProperty(
    const char *propName, CppUtilities::DateTime dateTime, const char *suffix = nullptr, CppUtilities::Indentation indentation = 4)
{
    if (!dateTime.isNull()) {
        printProperty(propName, dateTime.toString(), suffix, indentation);
    }
}

template <typename NumberType, CppUtilities::Traits::EnableIfAny<std::is_integral<NumberType>, std::is_floating_point<NumberType>> * = nullptr>
inline void printProperty(
    const char *propName, const NumberType value, const char *suffix = nullptr, bool force = false, CppUtilities::Indentation indentation = 4)
{
    if (value != 0 || force) {
        printProperty(propName, CppUtilities::numberToString<NumberType>(value), suffix, indentation);
    }
}

void printField(const FieldScope &scope, const Tag *tag, TagType tagType, bool skipEmpty);
void printNativeFields(const Tag *tag);

CppUtilities::TimeSpanOutputFormat parseTimeSpanOutputFormat(
    const CppUtilities::Argument &usageArg, CppUtilities::TimeSpanOutputFormat defaultFormat);
TagUsage parseUsageDenotation(const CppUtilities::Argument &usageArg, TagUsage defaultUsage);
TagTextEncoding parseEncodingDenotation(const CppUtilities::Argument &encodingArg, TagTextEncoding defaultEncoding);
ElementPosition parsePositionDenotation(const CppUtilities::Argument &posArg, const CppUtilities::Argument &valueArg, ElementPosition defaultPos);
std::uint64_t parseUInt64(const CppUtilities::Argument &arg, std::uint64_t defaultValue);
TagTarget::IdContainerType parseIds(const std::string &concatenatedIds);
bool applyTargetConfiguration(TagTarget &target, const std::string &configStr);
FieldDenotations parseFieldDenotations(const CppUtilities::Argument &fieldsArg, bool readOnly);
std::string tagName(const Tag *tag);
bool stringToBool(const std::string &str);
extern bool logLineFinalized;
void logNextStep(const TagParser::AbortableProgressFeedback &progress);
void logStepPercentage(const TagParser::AbortableProgressFeedback &progress);
void finalizeLog();

} // namespace Cli

#endif // CLI_HELPER
