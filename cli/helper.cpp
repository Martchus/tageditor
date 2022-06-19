#include "./helper.h"
#include "./fieldmapping.h"
#include "./mainfeatures.h"

#include <tagparser/diagnostics.h>
#include <tagparser/id3/id3v2tag.h>
#include <tagparser/matroska/matroskatag.h>
#include <tagparser/mediafileinfo.h>
#include <tagparser/mp4/mp4tag.h>
#include <tagparser/progressfeedback.h>
#include <tagparser/vorbis/vorbiscomment.h>

#include <c++utilities/application/argumentparser.h>
#include <c++utilities/conversion/stringbuilder.h>
#include <c++utilities/io/ansiescapecodes.h>

#include <unistd.h>

#include <csignal>
#include <cstring>
#include <iostream>

using namespace std;
using namespace std::placeholders;
using namespace CppUtilities;
using namespace CppUtilities::EscapeCodes;
using namespace TagParser;
using namespace Settings;

namespace Cli {

const std::vector<std::string_view> &id3v2CoverTypeNames()
{
    static const auto t
        = std::vector<std::string_view>{ "other"sv, "file-icon"sv, "other-file-icon"sv, "front-cover"sv, "back-cover"sv, "leaflet-page"sv, "media"sv,
              "lead-performer"sv, "artist"sv, "conductor"sv, "band"sv, "composer"sv, "lyricist"sv, "recording-location"sv, "during-recording"sv,
              "during-performance"sv, "movie-screen-capture"sv, "bright-colored-fish"sv, "illustration"sv, "artist-logotype"sv, "publisher"sv };
    return t;
}

CoverType id3v2CoverType(std::string_view coverName)
{
    static const auto mapping = [] {
        const auto &names = id3v2CoverTypeNames();
        auto map = std::map<std::string_view, CoverType>();
        auto index = CoverType();
        for (const auto name : names) {
            map[name] = index++;
        }
        return map;
    }();
    if (const auto i = mapping.find(coverName); i != mapping.end()) {
        return i->second;
    } else {
        return invalidCoverType;
    }
}

std::string_view id3v2CoverName(CoverType coverType)
{
    const auto &names = id3v2CoverTypeNames();
    return coverType < names.size() ? names[coverType] : "?"sv;
}

CppUtilities::TimeSpanOutputFormat timeSpanOutputFormat = TimeSpanOutputFormat::WithMeasures;

/*!
 * \class InterruptHandler
 * \brief The InterruptHandler class allows to register an interrupt handler within a scope.
 * \remarks Only one instance of the class can exist at a time.
 */

std::function<void()> InterruptHandler::s_handler;
bool InterruptHandler::s_handlerRegistered = false;

/*!
 * \brief Registers the specified \a handler for SIGINT and SIGTERM as long as this object is alive.
 * \remarks The specified \a handler should only call functions which are permitted to be used in signal handlers
 *          (eg. use POSIX write() instead of std::cout).
 * \throws Throws std::runtime_error when attempting to create a 2nd instance.
 */
InterruptHandler::InterruptHandler(std::function<void()> &&handler)
{
    // set handler function or throw if an instance has already been created
    if (s_handler) {
        throw runtime_error("Only one instance of InterruptHandler can exist at a time.");
    }
    s_handler = std::move(handler);

    // register handler if not registered yet
    if (!s_handlerRegistered) {
        s_handlerRegistered = true;
        signal(SIGINT, &InterruptHandler::handler);
        signal(SIGTERM, &InterruptHandler::handler);
    }
}

/*!
 * \brief Unregisters the handler.
 */
InterruptHandler::~InterruptHandler()
{
    s_handler = function<void()>();
}

/*!
 * \brief Internal handler method.
 */
void InterruptHandler::handler(int signum)
{
    // just exit if no custom handler has been defined
    if (!s_handler) {
        _exit(signum);
    }

    // finalize log and print warning, prevent using std::cout which could lead to undefined behaviour
    if (!logLineFinalized) {
        logLineFinalized = true;
        write(STDOUT_FILENO, "\n", 1);
    }
    if (EscapeCodes::enabled) {
        write(STDOUT_FILENO, "\e[1;33mWarning:\e[0m \e[1mSignal received, trying to abort ongoing process ...\e[0m\n", 82);
    } else {
        write(STDOUT_FILENO, "Warning: Signal received, trying to abort ongoing process ...\n", 63);
    }

    // call custom handler
    s_handler();
}

string incremented(const string &str, unsigned int toIncrement)
{
    string res;
    res.reserve(str.size());
    unsigned int value = 0;
    bool hasValue = false;
    for (const char &c : str) {
        if (toIncrement && c >= '0' && c <= '9') {
            value = value * 10 + static_cast<unsigned int>(c - '0');
            hasValue = true;
        } else {
            if (hasValue) {
                res += numberToString(value + 1);
                hasValue = false;
                --toIncrement;
            }
            res += c;
        }
    }
    if (hasValue) {
        res += numberToString(value + 1);
    }
    return res;
}

void printDiagMessages(const Diagnostics &diag, const char *head, bool beVerbose)
{
    if (diag.empty()) {
        return;
    }
    if (!beVerbose) {
        for (const auto &message : diag) {
            switch (message.level()) {
            case DiagLevel::Debug:
            case DiagLevel::Information:
                break;
            default:
                goto printDiagMsg;
            }
        }
        return;
    }

printDiagMsg:
    if (head) {
        cerr << " - " << head << endl;
    }
    for (const auto &message : diag) {
        switch (message.level()) {
        case DiagLevel::Debug:
            if (!beVerbose) {
                continue;
            }
            cerr << "    Debug        ";
            break;
        case DiagLevel::Information:
            if (!beVerbose) {
                continue;
            }
            cerr << "    Information  ";
            break;
        case DiagLevel::Warning:
            setStyle(cerr, Color::Yellow, ColorContext::Foreground, TextAttribute::Bold);
            setStyle(cerr, TextAttribute::Reset);
            setStyle(cerr, TextAttribute::Bold);
            cerr << "    Warning      ";
            setStyle(cerr, TextAttribute::Reset);
            break;
        case DiagLevel::Critical:
        case DiagLevel::Fatal:
            setStyle(cerr, Color::Red, ColorContext::Foreground, TextAttribute::Bold);
            setStyle(cerr, TextAttribute::Reset);
            setStyle(cerr, TextAttribute::Bold);
            cerr << "    Error        ";
            setStyle(cerr, TextAttribute::Reset);
            if (message.level() == DiagLevel::Fatal && exitCode == EXIT_SUCCESS) {
                exitCode = EXIT_PARSING_FAILURE;
            }
            break;
        default:;
        }
        cerr << message.creationTime().toString(DateTimeOutputFormat::TimeOnly) << "   ";
        cerr << message.context() << ": ";
        cerr << message.message() << '\n';
    }
}

void printProperty(const char *propName, std::string_view value, const char *suffix, Indentation indentation)
{
    if (value.empty()) {
        return;
    }
    const auto propLen(strlen(propName));
    cout << indentation << propName << Indentation(static_cast<unsigned char>(propLen < 30 ? 30 - propLen : 1)) << value;
    if (suffix) {
        cout << ' ' << suffix;
    }
    cout << '\n';
}

void printProperty(const char *propName, ElementPosition elementPosition, const char *suffix, Indentation indentation)
{
    const char *pos = nullptr;
    switch (elementPosition) {
    case ElementPosition::BeforeData:
        pos = "before data";
        break;
    case ElementPosition::AfterData:
        pos = "after data";
        break;
    case ElementPosition::Keep:
        pos = nullptr;
    }
    if (pos) {
        printProperty(propName, pos, suffix, indentation);
    }
}

static void printFieldName(std::string_view fieldName)
{
    cout << "    " << fieldName;
    // also write padding
    constexpr auto defaultIndent = 18;
    if (fieldName.size() >= defaultIndent) {
        // write at least one space
        cout << ' ';
        return;
    }
    for (auto i = fieldName.size(); i < defaultIndent; ++i) {
        cout << ' ';
    }
}

static void printTagValue(const TagValue &value)
{
    switch (value.type()) {
    case TagDataType::Binary:
    case TagDataType::Picture: {
        const auto type = !value.mimeType().empty() ? std::string_view(value.mimeType()) : std::string_view("data");
        std::cout << "can't display " << type << " as string (use --extract)";
        break;
    }
    default:
        std::cout << value.toDisplayString();
    }
    cout << '\n';
}

static void printDescription(const TagValue &value)
{
    if (value.description().empty()) {
        return;
    }
    printFieldName("  description:");
    if (value.descriptionEncoding() == TagTextEncoding::Utf8) {
        cout << value.description();
    } else {
        auto tempValue = TagValue();
        tempValue.setDescription(value.description(), value.descriptionEncoding());
        tempValue.convertDescriptionEncoding(TagTextEncoding::Utf8);
        cout << tempValue.description();
    }
    cout << '\n';
}

template <class TagType> static void printId3v2CoverValues(TagType *tag)
{
    const auto &fields = tag->fields();
    const auto id = tag->fieldId(KnownField::Cover);
    for (auto range = fields.equal_range(id); range.first != range.second; ++range.first) {
        const auto &field = range.first->second;
        printFieldName(argsToString("Cover (", id3v2CoverName(static_cast<CoverType>(field.typeInfo())), ")"));
        printTagValue(field.value());
        printDescription(field.value());
    }
}

void printField(const FieldScope &scope, const Tag *tag, TagType tagType, bool skipEmpty)
{
    const auto fieldName = std::string_view(scope.field.name());
    try {
        if (scope.field.knownFieldForTag(tag, tagType) == KnownField::Cover) {
            if (tagType == TagType::Id3v2Tag) {
                printId3v2CoverValues(static_cast<const Id3v2Tag *>(tag));
                return;
            } else if (tagType == TagType::VorbisComment) {
                printId3v2CoverValues(static_cast<const VorbisComment *>(tag));
                return;
            }
        }

        // parse field denotation
        const auto &values = scope.field.values(tag, tagType);

        // skip values if format-specific field ID used which is not applicable for the current tag
        if (!values.second) {
            return;
        }

        // skip empty values (unless prevented)
        if (skipEmpty && values.first.empty()) {
            return;
        }

        // print empty value (if not prevented)
        if (values.first.empty()) {
            printFieldName(fieldName);
            cout << "none\n";
            return;
        }

        // print values
        for (const auto &value : values.first) {
            printFieldName(fieldName);
            printTagValue(*value);
            printDescription(*value);
        }

    } catch (const ConversionException &e) {
        // handle conversion error which might happen when parsing field denotation
        printFieldName(fieldName);
        cout << "unable to parse - " << e.what() << '\n';
    }
}

template <typename ConcreteTag> void printNativeFields(const Tag *tag)
{
    const auto *const concreteTag = static_cast<const ConcreteTag *>(tag);
    for (const auto &field : concreteTag->fields()) {
        // skip all fields which are supported anyways
        if (concreteTag->knownField(field.first) != KnownField::Invalid) {
            continue;
        }

        const auto fieldId(ConcreteTag::FieldType::fieldIdToString(field.first));
        printFieldName(fieldId);
        printTagValue(field.second.value());
    }
}

void printNativeFields(const Tag *tag)
{
    switch (tag->type()) {
    case TagType::Id3v2Tag:
        printNativeFields<Id3v2Tag>(tag);
        break;
    case TagType::Mp4Tag:
        printNativeFields<Mp4Tag>(tag);
        break;
    case TagType::MatroskaTag:
        printNativeFields<MatroskaTag>(tag);
        break;
    case TagType::VorbisComment:
    case TagType::OggVorbisComment:
        printNativeFields<VorbisComment>(tag);
        break;
    default:;
    }
}

TimeSpanOutputFormat parseTimeSpanOutputFormat(const Argument &timeSpanFormatArg, TimeSpanOutputFormat defaultFormat)
{
    if (timeSpanFormatArg.isPresent()) {
        const auto &val = timeSpanFormatArg.values().front();
        if (!strcmp(val, "measures")) {
            return TimeSpanOutputFormat::WithMeasures;
        } else if (!strcmp(val, "colons")) {
            return TimeSpanOutputFormat::Normal;
        } else if (!strcmp(val, "seconds")) {
            return TimeSpanOutputFormat::TotalSeconds;
        } else {
            cerr << Phrases::Error << "The specified time span format \"" << val << "\" is invalid." << Phrases::End
                 << "note: Valid formats are measures, colons and seconds." << endl;
            exit(-1);
        }
    }
    return defaultFormat;
}

TagUsage parseUsageDenotation(const Argument &usageArg, TagUsage defaultUsage)
{
    if (usageArg.isPresent()) {
        const auto &val = usageArg.values().front();
        if (!strcmp(val, "never")) {
            return TagUsage::Never;
        } else if (!strcmp(val, "keepexisting")) {
            return TagUsage::KeepExisting;
        } else if (!strcmp(val, "always")) {
            return TagUsage::Always;
        } else {
            cerr << Phrases::Error << "The specified tag usage \"" << val << "\" is invalid." << Phrases::End
                 << "note: Valid values are never, keepexisting and always." << endl;
            exit(-1);
        }
    }
    return defaultUsage;
}

TagTextEncoding parseEncodingDenotation(const Argument &encodingArg, TagTextEncoding defaultEncoding)
{
    if (encodingArg.isPresent()) {
        const auto &val = encodingArg.values().front();
        if (!strcmp(val, "utf8")) {
            return TagTextEncoding::Utf8;
        } else if (!strcmp(val, "latin1")) {
            return TagTextEncoding::Latin1;
        } else if (!strcmp(val, "utf16be")) {
            return TagTextEncoding::Utf16BigEndian;
        } else if (!strcmp(val, "utf16le")) {
            return TagTextEncoding::Utf16LittleEndian;
        } else if (!strcmp(val, "auto")) {
        } else {
            cerr << Phrases::Error << "The specified encoding \"" << val << "\" is invalid." << Phrases::End
                 << "note: Valid encodings are utf8, latin1, utf16be, utf16le and auto." << endl;
            exit(-1);
        }
    }
    return defaultEncoding;
}

ElementPosition parsePositionDenotation(const Argument &posArg, const Argument &valueArg, ElementPosition defaultPos)
{
    if (posArg.isPresent()) {
        const char *val = valueArg.values(0).front();
        if (!strcmp(val, "front")) {
            return ElementPosition::BeforeData;
        } else if (!strcmp(val, "back")) {
            return ElementPosition::AfterData;
        } else if (!strcmp(val, "keep")) {
            return ElementPosition::Keep;
        } else {
            cerr << Phrases::Error << "The specified position \"" << val << "\" is invalid." << Phrases::End
                 << "note: Valid positions are front, back and keep." << endl;
            exit(-1);
        }
    }
    return defaultPos;
}

std::uint64_t parseUInt64(const Argument &arg, std::uint64_t defaultValue)
{
    if (arg.isPresent()) {
        try {
            if (*arg.values().front() == '0' && *(arg.values().front() + 1) == 'x') {
                return stringToNumber<std::uint64_t>(arg.values().front() + 2, 16);
            } else {
                return stringToNumber<std::uint64_t>(arg.values().front());
            }
        } catch (const ConversionException &) {
            cerr << Phrases::Error << "The specified value \"" << arg.values().front() << "\" is no valid unsigned integer." << Phrases::EndFlush;
            exit(-1);
        }
    }
    return defaultValue;
}

TagTarget::IdContainerType parseIds(std::string_view concatenatedIds)
{
    const auto splittedIds = splitStringSimple(concatenatedIds, ",");
    auto convertedIds = TagTarget::IdContainerType();
    convertedIds.reserve(splittedIds.size());
    for (const auto &id : splittedIds) {
        if (id.empty()) {
            continue;
        }
        try {
            convertedIds.push_back(stringToNumber<TagTarget::IdType>(id));
        } catch (const ConversionException &) {
            cerr << Phrases::Error << "The specified ID \"" << id << "\" is invalid." << Phrases::End << "note: IDs must be unsigned integers."
                 << endl;
            exit(-1);
        }
    }
    return convertedIds;
}

bool applyTargetConfiguration(TagTarget &target, std::string_view configStr)
{
    if (!configStr.empty()) {
        if (configStr.compare(0, 13, "target-level=") == 0) {
            try {
                target.setLevel(stringToNumber<std::uint64_t>(configStr.substr(13)));
            } catch (const ConversionException &) {
                cerr << Phrases::Error << "The specified target level \"" << configStr.substr(13) << "\" is invalid." << Phrases::End
                     << "note: The target level must be an unsigned integer." << endl;
                exit(-1);
            }
        } else if (configStr.compare(0, 17, "target-levelname=") == 0) {
            target.setLevelName(std::string(configStr.substr(17)));
        } else if (configStr.compare(0, 14, "target-tracks=") == 0) {
            target.tracks() = parseIds(configStr.substr(14));
        } else if (configStr.compare(0, 16, "target-chapters=") == 0) {
            target.chapters() = parseIds(configStr.substr(16));
        } else if (configStr.compare(0, 16, "target-editions=") == 0) {
            target.editions() = parseIds(configStr.substr(16));
        } else if (configStr.compare(0, 19, "target-attachments=") == 0) {
            target.attachments() = parseIds(configStr.substr(19));
        } else if (configStr.compare(0, 13, "target-reset=") == 0) {
            if (*(configStr.data() + 13)) {
                cerr << Phrases::Error << "Invalid assignment " << (configStr.data() + 13) << " for target-reset." << Phrases::EndFlush;
                exit(-1);
            }
            target.clear();
        } else if (configStr == "target-reset") {
            target.clear();
        } else {
            return false;
        }
        return true;
    } else {
        return false;
    }
}

FieldDenotations parseFieldDenotations(const Argument &fieldsArg, bool readOnly)
{
    auto fields = FieldDenotations();
    if (!fieldsArg.isPresent()) {
        return fields;
    }

    const std::vector<const char *> &fieldDenotations = fieldsArg.values();
    auto scope = FieldScope();

    for (std::string_view fieldDenotationString : fieldDenotations) {
        // check for tag or target specifier
        if (startsWith(fieldDenotationString, "tag=")) {
            const auto tagTypeString = fieldDenotationString.substr(4);
            if (tagTypeString.empty()) {
                cerr << Phrases::Error << "The \"tag\"-specifier has been used with no value(s)." << Phrases::End
                     << "note: Possible values are id3,id3v1,id3v2,itunes,vorbis,matroska and all." << endl;
                std::exit(-1);
            }
            auto tagType = TagType::Unspecified;
            for (const auto &part : splitStringSimple<std::vector<std::string_view>>(tagTypeString, ",")) {
                if (part.empty()) {
                    continue;
                }
                if (part == "id3v1") {
                    tagType |= TagType::Id3v1Tag;
                } else if (part == "id3v2") {
                    tagType |= TagType::Id3v2Tag;
                } else if (part == "id3") {
                    tagType |= TagType::Id3v1Tag | TagType::Id3v2Tag;
                } else if (part == "itunes" || part == "mp4") {
                    tagType |= TagType::Mp4Tag;
                } else if (part == "vorbis") {
                    tagType |= TagType::VorbisComment | TagType::OggVorbisComment;
                } else if (part == "matroska") {
                    tagType |= TagType::MatroskaTag;
                } else if (part == "all" || part == "any") {
                    tagType = TagType::Unspecified;
                    break;
                } else {
                    cerr << Phrases::Error << "The value \"" << part << " for the \"tag\"-specifier is invalid." << Phrases::End
                         << "note: Possible values are id3,id3v1,id3v2,itunes,vorbis,matroska and all." << endl;
                    std::exit(-1);
                }
            }
            scope.tagType = tagType;
            scope.allTracks = false;
            scope.trackIds.clear();
            continue;
        } else if (applyTargetConfiguration(scope.tagTarget, fieldDenotationString)) {
            continue;
        } else if (fieldDenotationString == "target-matching=exact") {
            scope.exactTargetMatching = true;
            continue;
        } else if (fieldDenotationString == "target-matching=relaxed") {
            scope.exactTargetMatching = false;
            continue;
        } else if (startsWith(fieldDenotationString, "track-id=")) {
            const auto parts = splitStringSimple<std::vector<std::string_view>>(fieldDenotationString.substr(9), ",");
            auto allTracks = false;
            auto trackIds = vector<std::uint64_t>();
            trackIds.reserve(parts.size());
            for (const auto &part : parts) {
                if (part.empty()) {
                    continue;
                }
                if (part == "all" || part == "any") {
                    allTracks = true;
                    break;
                }
                try {
                    trackIds.emplace_back(stringToNumber<std::uint64_t>(part));
                } catch (const ConversionException &) {
                    cerr << Phrases::Error << "The value provided with the \"track\"-specifier is invalid." << Phrases::End
                         << "note: It must be a comma-separated list of track IDs." << endl;
                    std::exit(-1);
                }
            }
            scope.allTracks = allTracks;
            scope.trackIds = move(trackIds);
            continue;
        }

        // check whether field name starts with + indicating an additional value
        auto additionalValue = !fieldDenotationString.empty() && fieldDenotationString.front() == '+';
        if (additionalValue) {
            fieldDenotationString = fieldDenotationString.substr(1);
        }

        // read field name
        const auto equationPos = fieldDenotationString.find('=');
        auto fieldNameLen = equationPos != std::string_view::npos ? equationPos : fieldDenotationString.size();
        // field name might denote increment ("+") or path disclosure (">")
        auto type = DenotationType::Normal;
        if (fieldNameLen && equationPos != std::string_view::npos && equationPos) {
            switch (fieldDenotationString[equationPos - 1]) {
            case '+':
                type = DenotationType::Increment;
                --fieldNameLen;
                break;
            case '>':
                type = DenotationType::File;
                --fieldNameLen;
                break;
            default:;
            }
        }

        // field name might specify a file index
        auto fileIndex = 0u, mult = 1u;
        for (auto digitPos = fieldNameLen - 1; fieldNameLen && isDigit(fieldDenotationString[digitPos]); --fieldNameLen, --digitPos, mult *= 10) {
            fileIndex += static_cast<unsigned int>(fieldDenotationString[digitPos] - '0') * mult;
        }
        if (!fieldNameLen) {
            cerr << Phrases::Error << "The field denotation \"" << fieldDenotationString << "\" has no field name." << Phrases::EndFlush;
            exit(-1);
        }

        // parse the denoted field ID
        const auto fieldName = fieldDenotationString.substr(0, fieldNameLen);
        try {
            if (scope.isTrack()) {
                scope.field = FieldId::fromTrackDenotation(fieldName);
            } else {
                scope.field = FieldId::fromTagDenotation(fieldName);
            }
        } catch (const ConversionException &e) {
            // unable to parse field ID denotation -> discard the field denotation
            cerr << Phrases::Error << "The field denotation \"" << fieldName << "\" could not be parsed: " << e.what() << Phrases::EndFlush;
            std::exit(-1);
        }

        // read cover always from file
        if (scope.field.knownField() == KnownField::Cover) {
            type = DenotationType::File;
        }

        // add field denotation scope
        auto &fieldValues = fields[scope];
        // add value to the scope (if present)
        if (equationPos != std::string_view::npos) {
            if (readOnly) {
                cerr << Phrases::Error << "A value has been specified for \"" << fieldName << "\"." << Phrases::End
                     << "note: This is only possible when the \"set\"-operation is used." << endl;
                std::exit(-1);
            } else {
                // file index might have been specified explicitly
                // if not (mult == 1) use the index of the last value and increase it by one if the value is not an additional one
                // if there are no previous values, just use the index 0
                fieldValues.allValues.emplace_back(type,
                    mult == 1 ? (fieldValues.allValues.empty() ? 0 : fieldValues.allValues.back().fileIndex + (additionalValue ? 0 : 1)) : fileIndex,
                    fieldDenotationString.substr(equationPos + 1));
            }
        }
        if (additionalValue && readOnly) {
            cerr << Phrases::Error << "Indication of an additional value for \"" << fieldName << "\" is invalid." << Phrases::End
                 << "note: This is only possible when the \"set\"-operation is used." << endl;
            std::exit(-1);
        }
    }
    return fields;
}

template <class ConcreteTag, TagType tagTypeMask = ConcreteTag::tagType>
static std::pair<std::vector<const TagValue *>, bool> valuesForNativeField(std::string_view idString, const Tag *tag, TagType tagType)
{
    auto res = make_pair<std::vector<const TagValue *>, bool>({}, false);
    if (!(tagType & tagTypeMask)) {
        return res;
    }
    res.first = static_cast<const ConcreteTag *>(tag)->values(ConcreteTag::FieldType::fieldIdFromString(idString));
    res.second = true;
    return res;
}

template <class ConcreteTag, TagType tagTypeMask = ConcreteTag::tagType>
static bool setValuesForNativeField(std::string_view idString, Tag *tag, TagType tagType, const std::vector<TagValue> &values)
{
    if (!(tagType & tagTypeMask)) {
        return false;
    }
    return static_cast<ConcreteTag *>(tag)->setValues(ConcreteTag::FieldType::fieldIdFromString(idString), values);
}

template <class ConcreteTag, TagType tagTypeMask = ConcreteTag::tagType>
static KnownField knownFieldForNativeField(std::string_view idString, const Tag *tag, TagType tagType)
{
    if (!(tagType & tagTypeMask)) {
        return KnownField::Invalid;
    }
    try {
        return static_cast<const ConcreteTag *>(tag)->knownField(ConcreteTag::FieldType::fieldIdFromString(idString));
    } catch (const ConversionException &) {
        return KnownField::Invalid;
    }
}

inline FieldId::FieldId(std::string_view nativeField, GetValuesForNativeFieldType &&valuesForNativeField,
    SetValuesForNativeFieldType &&setValuesForNativeField, KnownFieldForNativeFieldType &&knownFieldForNativeField)
    : m_knownField(KnownField::Invalid)
    , m_nativeField(nativeField)
    , m_valuesForNativeField(std::move(valuesForNativeField))
    , m_setValuesForNativeField(std::move(setValuesForNativeField))
    , m_knownFieldForNativeField(std::move(knownFieldForNativeField))
{
}

/// \remarks This wrapper is required because specifying c'tor template args is not possible.
template <class ConcreteTag, TagType tagTypeMask> FieldId FieldId::fromNativeField(std::string_view nativeFieldId)
{
    return FieldId(nativeFieldId, std::bind(&valuesForNativeField<ConcreteTag, tagTypeMask>, nativeFieldId, _1, _2),
        std::bind(&setValuesForNativeField<ConcreteTag, tagTypeMask>, nativeFieldId, _1, _2, _3),
        std::bind(&knownFieldForNativeField<ConcreteTag, tagTypeMask>, nativeFieldId, _1, _2));
}

FieldId FieldId::fromTagDenotation(std::string_view denotation)
{
    // check for native, format-specific denotation
    if (startsWith(denotation, "mkv:")) {
        return FieldId::fromNativeField<MatroskaTag>(denotation.substr(4));
    } else if (startsWith(denotation, "mp4:")) {
        return FieldId::fromNativeField<Mp4Tag>(denotation.substr(4));
    } else if (startsWith(denotation, "vorbis:")) {
        return FieldId::fromNativeField<VorbisComment, TagType::VorbisComment | TagType::OggVorbisComment>(denotation.substr(7));
    } else if (startsWith(denotation, "id3:")) {
        return FieldId::fromNativeField<Id3v2Tag>(denotation.substr(4));
    } else if (startsWith(denotation, "generic:")) {
        // allow prefix 'generic:' for consistency
        denotation = denotation.substr(8);
    }

    // determine KnownField for generic denotation
    const auto knownField(FieldMapping::knownField(denotation.data(), denotation.size()));
    if (knownField == KnownField::Invalid) {
        throw ConversionException("generic field name is unknown");
    }
    return FieldId(knownField, nullptr, 0);
}

FieldId FieldId::fromTrackDenotation(std::string_view denotation)
{
    return FieldId(KnownField::Invalid, denotation.data(), denotation.size());
}

std::pair<std::vector<const TagValue *>, bool> FieldId::values(const Tag *tag, TagType tagType) const
{
    auto res = make_pair<std::vector<const TagValue *>, bool>({}, false);
    if (!m_nativeField.empty()) {
        res = m_valuesForNativeField(tag, tagType);
    } else {
        res.first = tag->values(m_knownField);
        res.second = true;
    }
    return res;
}

bool FieldId::setValues(Tag *tag, TagType tagType, const std::vector<TagValue> &values) const
{
    if (!m_nativeField.empty()) {
        return m_setValuesForNativeField(tag, tagType, values);
    } else {
        return tag->setValues(m_knownField, values);
    }
}

KnownField FieldId::knownFieldForTag(const Tag *tag, TagType tagType) const
{
    if (!m_nativeField.empty()) {
        return m_knownFieldForNativeField(tag, tagType);
    } else {
        return m_knownField;
    }
}

string tagName(const Tag *tag)
{
    stringstream ss;
    const TagType tagType = tag->type();
    // write tag name and target, eg. MP4/iTunes tag
    ss << tag->typeName();
    if (tagType == TagType::Id3v2Tag) {
        // version only interesting for ID3v2 tags?
        ss << " (version " << tag->version() << ')';
    }
    if (tagType == TagType::MatroskaTag || !tag->target().isEmpty()) {
        ss << " targeting \"" << tag->targetString() << '\"';
    }
    return ss.str();
}

bool stringToBool(const string &str)
{
    if (str == "yes" || str == "true" || str == "on" || str == "1") {
        return true;
    } else if (str == "no" || str == "false" || str == "off" || str == "0") {
        return false;
    }
    throw ConversionException(argsToString('\"', str, "\" is not yes or no"));
}

bool logLineFinalized = true;
static string lastStep;
void logNextStep(const AbortableProgressFeedback &progress)
{
    // finalize previous step
    if (!logLineFinalized) {
        cout << "\r - [100%] " << lastStep << endl;
        logLineFinalized = true;
    }
    // print line for next step
    lastStep = progress.step();
    cout << "\r - [" << setw(3) << static_cast<unsigned int>(progress.stepPercentage()) << "%] " << lastStep << flush;
    logLineFinalized = false;
}

void logStepPercentage(const TagParser::AbortableProgressFeedback &progress)
{
    cout << "\r - [" << setw(3) << static_cast<unsigned int>(progress.stepPercentage()) << "%] " << lastStep << flush;
}

void finalizeLog()
{
    if (logLineFinalized) {
        return;
    }
    cout << "\r - [100%] " << lastStep << '\n';
    logLineFinalized = true;
    lastStep.clear();
}

} // namespace Cli
