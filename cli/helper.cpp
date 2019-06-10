#include "./helper.h"
#include "./fieldmapping.h"

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

CppUtilities::TimeSpanOutputFormat timeSpanOutputFormat = TimeSpanOutputFormat::WithMeasures;

/*!
 * \class InterruptHandler
 * \brief The InterruptHandler class allows to register an interrupt handler within a scope.
 * \remarks Only one instance of the class can exist at a time.
 */

std::function<void()> InterruptHandler::s_handler;
bool InterruptHandler::s_handlerRegistered = false;

/*!
 * \brief Registers the specified \a handler for SIGINT as long as this object is alive.
 * \remarks The specified \a handler should only call functions which are permitted to be used in signal handlers
 *          (eg. use POSIX write() instread of std::cout).
 * \throws Throws std::runtime_error when attempting to create a 2nd instance.
 */
InterruptHandler::InterruptHandler(std::function<void()> handler)
{
    // set handler function or throw if an instance has already been created
    if (s_handler) {
        throw runtime_error("Only one instance of InterruptHandler can exist at a time.");
    }
    s_handler = handler;

    // register handler if not registered yet
    if (!s_handlerRegistered) {
        s_handlerRegistered = true;
        signal(SIGINT, &InterruptHandler::handler);
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
        write(STDOUT_FILENO, "\e[1;33mWarning:\e[0m \e[1mInterrupt received, trying to abort ongoing process ...\e[0m\n", 84);
    } else {
        write(STDOUT_FILENO, "Warning: Interrupt received, trying to abort ongoing process ...\n", 65);
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
        cout << " - " << head << endl;
    }
    for (const auto &message : diag) {
        switch (message.level()) {
        case DiagLevel::Debug:
            if (!beVerbose) {
                continue;
            }
            cout << "    Debug        ";
            break;
        case DiagLevel::Information:
            if (!beVerbose) {
                continue;
            }
            cout << "    Information  ";
            break;
        case DiagLevel::Warning:
            setStyle(cout, Color::Yellow, ColorContext::Foreground, TextAttribute::Bold);
            setStyle(cout, TextAttribute::Reset);
            setStyle(cout, TextAttribute::Bold);
            cout << "    Warning      ";
            setStyle(cout, TextAttribute::Reset);
            break;
        case DiagLevel::Critical:
            setStyle(cout, Color::Red, ColorContext::Foreground, TextAttribute::Bold);
            setStyle(cout, TextAttribute::Reset);
            setStyle(cout, TextAttribute::Bold);
            cout << "    Error        ";
            setStyle(cout, TextAttribute::Reset);
            break;
        default:;
        }
        cout << message.creationTime().toString(DateTimeOutputFormat::TimeOnly) << "   ";
        cout << message.context() << ": ";
        cout << message.message() << '\n';
    }
}

void printProperty(const char *propName, const char *value, const char *suffix, Indentation indentation)
{
    if (!*value) {
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

void printFieldName(const char *fieldName, size_t fieldNameLen)
{
    cout << "    " << fieldName;
    // also write padding
    if (fieldNameLen >= 18) {
        // write at least one space
        cout << ' ';
        return;
    }
    for (auto i = fieldNameLen; i < 18; ++i) {
        cout << ' ';
    }
}

void printTagValue(const TagValue &value)
{
    try {
        cout << value.toString(TagTextEncoding::Utf8);
    } catch (const ConversionException &) {
        // handle case when value can not be displayed as string
        cout << "can't display as string (see --extract)";
    }
    cout << '\n';
}

void printField(const FieldScope &scope, const Tag *tag, TagType tagType, bool skipEmpty)
{
    // write field name
    const char *fieldName = scope.field.name();
    const auto fieldNameLen = strlen(fieldName);

    try {
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
            printFieldName(fieldName, fieldNameLen);
            cout << "none\n";
            return;
        }

        // print values
        for (const auto &value : values.first) {
            printFieldName(fieldName, fieldNameLen);
            printTagValue(*value);
        }

    } catch (const ConversionException &e) {
        // handle conversion error which might happen when parsing field denotation
        printFieldName(fieldName, fieldNameLen);
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
        printFieldName(fieldId.data(), fieldId.size());
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

TagTarget::IdContainerType parseIds(const std::string &concatenatedIds)
{
    auto splittedIds = splitString(concatenatedIds, ",", EmptyPartsTreat::Omit);
    TagTarget::IdContainerType convertedIds;
    convertedIds.reserve(splittedIds.size());
    for (const auto &id : splittedIds) {
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

bool applyTargetConfiguration(TagTarget &target, const std::string &configStr)
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
            target.setLevelName(configStr.substr(17));
        } else if (configStr.compare(0, 14, "target-tracks=") == 0) {
            target.tracks() = parseIds(configStr.substr(14));
        } else if (configStr.compare(0, 16, "target-chapters=") == 0) {
            target.chapters() = parseIds(configStr.substr(16));
        } else if (configStr.compare(0, 16, "target-editions=") == 0) {
            target.editions() = parseIds(configStr.substr(16));
        } else if (configStr.compare(0, 19, "target-attachments=") == 0) {
            target.attachments() = parseIds(configStr.substr(17));
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
    FieldDenotations fields;
    if (!fieldsArg.isPresent()) {
        return fields;
    }

    const vector<const char *> &fieldDenotations = fieldsArg.values();
    FieldScope scope;

    for (const char *fieldDenotationString : fieldDenotations) {
        // check for tag or target specifier
        const auto fieldDenotationLen = strlen(fieldDenotationString);
        if (!strncmp(fieldDenotationString, "tag=", 4)) {
            if (fieldDenotationLen == 4) {
                cerr << Phrases::Error << "The \"tag\"-specifier has been used with no value(s)." << Phrases::End
                     << "note: Possible values are id3,id3v1,id3v2,itunes,vorbis,matroska and all." << endl;
                exit(-1);
            } else {
                TagType tagType = TagType::Unspecified;
                for (const auto &part : splitString(fieldDenotationString + 4, ",", EmptyPartsTreat::Omit)) {
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
                        exit(-1);
                    }
                }
                scope.tagType = tagType;
                scope.allTracks = false;
                scope.trackIds.clear();
            }
            continue;
        } else if (applyTargetConfiguration(scope.tagTarget, fieldDenotationString)) {
            continue;
        } else if (!strncmp(fieldDenotationString, "track-id=", 9)) {
            const vector<string> parts = splitString<vector<string>>(fieldDenotationString + 9, ",", EmptyPartsTreat::Omit);
            bool allTracks = false;
            vector<std::uint64_t> trackIds;
            trackIds.reserve(parts.size());
            for (const auto &part : parts) {
                if (part == "all" || part == "any") {
                    allTracks = true;
                    break;
                }
                try {
                    trackIds.emplace_back(stringToNumber<std::uint64_t>(part));
                } catch (const ConversionException &) {
                    cerr << Phrases::Error << "The value provided with the \"track\"-specifier is invalid." << Phrases::End
                         << "note: It must be a comma-separated list of track IDs." << endl;
                    exit(-1);
                }
            }
            scope.allTracks = allTracks;
            scope.trackIds = move(trackIds);
            continue;
        }

        // check whether field name starts with + indicating an additional value
        bool additionalValue = *fieldDenotationString == '+';
        if (additionalValue) {
            ++fieldDenotationString;
        }

        // read field name
        const auto equationPos = strchr(fieldDenotationString, '=');
        size_t fieldNameLen = equationPos ? static_cast<size_t>(equationPos - fieldDenotationString) : strlen(fieldDenotationString);
        // field name might denote increment ("+") or path disclosure (">")
        DenotationType type = DenotationType::Normal;
        if (fieldNameLen && equationPos) {
            switch (*(equationPos - 1)) {
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
        unsigned int fileIndex = 0, mult = 1;
        for (const char *digitPos = fieldDenotationString + fieldNameLen - 1; fieldNameLen && isDigit(*digitPos);
             --fieldNameLen, --digitPos, mult *= 10) {
            fileIndex += static_cast<unsigned int>(*digitPos - '0') * mult;
        }
        if (!fieldNameLen) {
            cerr << Phrases::Error << "The field denotation \"" << fieldDenotationString << "\" has no field name." << Phrases::EndFlush;
            exit(-1);
        }

        // parse the denoted field ID
        try {
            if (scope.isTrack()) {
                scope.field = FieldId::fromTrackDenotation(fieldDenotationString, fieldNameLen);
            } else {
                scope.field = FieldId::fromTagDenotation(fieldDenotationString, fieldNameLen);
            }
        } catch (const ConversionException &e) {
            // unable to parse field ID denotation -> discard the field denotation
            cerr << Phrases::Error << "The field denotation \"" << string(fieldDenotationString, fieldNameLen)
                 << "\" could not be parsed: " << e.what() << Phrases::EndFlush;
            exit(-1);
        }

        // read cover always from file
        if (scope.field.knownField() == KnownField::Cover) {
            type = DenotationType::File;
        }

        // add field denotation scope
        auto &fieldValues = fields[scope];
        // add value to the scope (if present)
        if (equationPos) {
            if (readOnly) {
                cerr << Phrases::Error << "A value has been specified for \"" << string(fieldDenotationString, fieldNameLen) << "\"." << Phrases::End
                     << "note: This is only possible when the \"set\"-operation is used." << endl;
                exit(-1);
            } else {
                // file index might have been specified explicitely
                // if not (mult == 1) use the index of the last value and increase it by one if the value is not an additional one
                // if there are no previous values, just use the index 0
                fieldValues.allValues.emplace_back(type,
                    mult == 1 ? (fieldValues.allValues.empty() ? 0 : fieldValues.allValues.back().fileIndex + (additionalValue ? 0 : 1)) : fileIndex,
                    equationPos + 1);
            }
        }
        if (additionalValue && readOnly) {
            cerr << Phrases::Error << "Indication of an additional value for \"" << string(fieldDenotationString, fieldNameLen) << "\" is invalid."
                 << Phrases::End << "note: This is only possible when the \"set\"-operation is used." << endl;
            exit(-1);
        }
    }
    return fields;
}

template <class ConcreteTag, TagType tagTypeMask = ConcreteTag::tagType>
std::pair<std::vector<const TagValue *>, bool> valuesForNativeField(const char *idString, std::size_t idStringSize, const Tag *tag, TagType tagType)
{
    auto res = make_pair<std::vector<const TagValue *>, bool>({}, false);
    if ((tagType & tagTypeMask) == TagType::Unspecified) {
        return res;
    }
    res.first = static_cast<const ConcreteTag *>(tag)->values(ConcreteTag::FieldType::fieldIdFromString(idString, idStringSize));
    res.second = true;
    return res;
}

template <class ConcreteTag, TagType tagTypeMask = ConcreteTag::tagType>
bool setValuesForNativeField(const char *idString, std::size_t idStringSize, Tag *tag, TagType tagType, const std::vector<TagValue> &values)
{
    if ((tagType & tagTypeMask) == TagType::Unspecified) {
        return false;
    }
    return static_cast<ConcreteTag *>(tag)->setValues(ConcreteTag::FieldType::fieldIdFromString(idString, idStringSize), values);
}

inline FieldId::FieldId(const char *nativeField, std::size_t nativeFieldSize, const GetValuesForNativeFieldType &valuesForNativeField,
    const SetValuesForNativeFieldType &setValuesForNativeField)
    : m_knownField(KnownField::Invalid)
    , m_nativeField(nativeField, nativeFieldSize)
    , m_valuesForNativeField(valuesForNativeField)
    , m_setValuesForNativeField(setValuesForNativeField)
{
}

/// \remarks This wrapper is required because specifying c'tor template args is not possible.
template <class ConcreteTag, TagType tagTypeMask> FieldId FieldId::fromNativeField(const char *nativeFieldId, std::size_t nativeFieldIdSize)
{
    return FieldId(nativeFieldId, nativeFieldIdSize, bind(&valuesForNativeField<ConcreteTag, tagTypeMask>, nativeFieldId, nativeFieldIdSize, _1, _2),
        bind(&setValuesForNativeField<ConcreteTag, tagTypeMask>, nativeFieldId, nativeFieldIdSize, _1, _2, _3));
}

FieldId FieldId::fromTagDenotation(const char *denotation, size_t denotationSize)
{
    // check for native, format-specific denotation
    if (!strncmp(denotation, "mkv:", 4)) {
        return FieldId::fromNativeField<MatroskaTag>(denotation + 4, denotationSize - 4);
    } else if (!strncmp(denotation, "mp4:", 4)) {
        return FieldId::fromNativeField<Mp4Tag>(denotation + 4, denotationSize - 4);
    } else if (!strncmp(denotation, "vorbis:", 7)) {
        return FieldId::fromNativeField<VorbisComment, TagType::VorbisComment | TagType::OggVorbisComment>(denotation + 7, denotationSize - 7);
    } else if (!strncmp(denotation, "id3:", 7)) {
        return FieldId::fromNativeField<Id3v2Tag>(denotation + 4, denotationSize - 4);
    } else if (!strncmp(denotation, "generic:", 8)) {
        // allow prefix 'generic:' for consistency
        denotation += 8, denotationSize -= 8;
    }

    // determine KnownField for generic denotation
    const auto knownField(FieldMapping::knownField(denotation, denotationSize));
    if (knownField == KnownField::Invalid) {
        throw ConversionException("generic field name is unknown");
    }
    return FieldId(knownField, nullptr, 0);
}

FieldId FieldId::fromTrackDenotation(const char *denotation, size_t denotationSize)
{
    return FieldId(KnownField::Invalid, denotation, denotationSize);
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
