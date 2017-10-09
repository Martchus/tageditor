#include "./helper.h"

#include <tagparser/mediafileinfo.h>
#include <tagparser/matroska/matroskatag.h>
#include <tagparser/mp4/mp4tag.h>
#include <tagparser/vorbis/vorbiscomment.h>
#include <tagparser/id3/id3v2tag.h>

#include <c++utilities/application/argumentparser.h>
#include <c++utilities/conversion/stringbuilder.h>
#include <c++utilities/io/ansiescapecodes.h>

#include <iostream>
#include <cstring>
#include <csignal>

using namespace std;
using namespace std::placeholders;
using namespace ApplicationUtilities;
using namespace ConversionUtilities;
using namespace ChronoUtilities;
using namespace Media;
using namespace Settings;
using namespace EscapeCodes;

namespace Cli {

std::function<void()> InterruptHandler::s_handler;
bool InterruptHandler::s_handlerRegistered = false;

InterruptHandler::InterruptHandler(std::function<void ()> handler)
{
    // set handler function or throw if an instance has already been created
    if(s_handler) {
        throw runtime_error("Only one instance of InterruptHandler can exist at a time.");
    }
    s_handler = handler;

    // register handler if not registered yet
    if(!s_handlerRegistered) {
        s_handlerRegistered = true;
        signal(SIGINT, &InterruptHandler::handler);
    }
}

InterruptHandler::~InterruptHandler()
{
    s_handler = function<void()>();
}

void InterruptHandler::handler(int signum)
{
    // just exit if no custom handler has been defined
    if(!s_handler) {
        exit(signum);
    }

    // print warning
    finalizeLog();
    cout << Phrases::Warning << "Interrupt received, trying to abort ongoing process ..." << Phrases::End << flush;

    // call custom handler
    s_handler();
}

string incremented(const string &str, unsigned int toIncrement)
{
    string res;
    res.reserve(str.size());
    unsigned int value = 0;
    bool hasValue = false;
    for(const char &c : str) {
        if(toIncrement && c >= '0' && c <= '9') {
            value = value * 10 + static_cast<unsigned int>(c - '0');
            hasValue = true;
        } else {
            if(hasValue) {
                res += numberToString(value + 1);
                hasValue = false;
                --toIncrement;
            }
            res += c;
        }
    }
    if(hasValue) {
        res += numberToString(value + 1);
    }
    return res;
}

void printNotifications(NotificationList &notifications, const char *head, bool beVerbose)
{
    if(notifications.empty()) {
        return;
    }
    if(!beVerbose) {
        for(const auto &notification : notifications) {
            switch(notification.type()) {
            case NotificationType::Debug:
            case NotificationType::Information:
                break;
            default:
                goto printNotifications;
            }
        }
        return;
    }

printNotifications:
    if(head) {
        cout << " - " << head << endl;
    }
    Notification::sortByTime(notifications);
    for(const auto &notification : notifications) {
        switch(notification.type()) {
        case NotificationType::Debug:
            if(beVerbose) {
                cout << "    Debug        ";
                break;
            } else {
                continue;
            }
        case NotificationType::Information:
            if(beVerbose) {
                cout << "    Information  ";
                break;
            } else {
                continue;
            }
        case NotificationType::Warning:
            cout << "    Warning      ";
            break;
        case NotificationType::Critical:
            cout << "    Error        ";
            break;
        default:
            ;
        }
        cout << notification.creationTime().toString(DateTimeOutputFormat::TimeOnly) << "   ";
        cout << notification.context() << ": ";
        cout << notification.message() << '\n';
    }
}

void printNotifications(const MediaFileInfo &fileInfo, const char *head, bool beVerbose)
{
    NotificationList notifications;
    fileInfo.gatherRelatedNotifications(notifications);
    printNotifications(notifications, head, beVerbose);
}

void printProperty(const char *propName, const char *value, const char *suffix, Indentation indentation)
{
    if(*value) {
        cout << indentation << propName << Indentation(30 - strlen(propName)) << value;
        if(suffix) {
            cout << ' ' << suffix;
        }
        cout << '\n';
    }
}

void printProperty(const char *propName, ElementPosition elementPosition, const char *suffix, Indentation indentation)
{
    const char *pos = nullptr;
    switch(elementPosition) {
    case ElementPosition::BeforeData:
        pos = "before data";
        break;
    case ElementPosition::AfterData:
        pos = "after data";
        break;
    case ElementPosition::Keep:
        pos = nullptr;
    }
    if(pos) {
        printProperty(propName, pos, suffix, indentation);
    }
}

void printFieldName(const char *fieldName, size_t fieldNameLen)
{
    cout << "    " << fieldName;
    // also write padding
    for(auto i = fieldNameLen; i < 18; ++i) {
        cout << ' ';
    }
}

void printField(const FieldScope &scope, const Tag *tag, TagType tagType, bool skipEmpty)
{
    // write field name
    const char *fieldName = scope.field.name();
    const auto fieldNameLen = strlen(fieldName);

    try {
        // parse field denotation
        const auto &values = scope.field.values(tag, tagType);

        // skip empty values (unless prevented)
        if(skipEmpty && values.empty()) {
            return;
        }

        // print empty value (if not prevented)
        if(values.empty()) {
            printFieldName(fieldName, fieldNameLen);
            cout << "none\n";
            return;
        }

        // print values
        for(const auto &value : values) {
            printFieldName(fieldName, fieldNameLen);
            try {
                cout << value->toString(TagTextEncoding::Utf8);
            } catch(const ConversionException &) {
                // handle case when value can not be displayed as string
                cout << "can't display as string (see --extract)";
            }
            cout << '\n';
        }

    } catch(const ConversionException &e) {
        // handle conversion error which might happen when parsing field denotation
        printFieldName(fieldName, fieldNameLen);
        cout << "unable to parse - " << e.what() << '\n';
    }
}

TagUsage parseUsageDenotation(const Argument &usageArg, TagUsage defaultUsage)
{
    if(usageArg.isPresent()) {
        const auto &val = usageArg.values().front();
        if(!strcmp(val, "never")) {
            return TagUsage::Never;
        } else if(!strcmp(val, "keepexisting")) {
            return TagUsage::KeepExisting;
        } else if(!strcmp(val, "always")) {
            return TagUsage::Always;
        } else {
            cerr << Phrases::Warning << "The specified tag usage \"" << val << "\" is invalid and will be ignored." << Phrases::EndFlush;
        }
    }
    return defaultUsage;
}

TagTextEncoding parseEncodingDenotation(const Argument &encodingArg, TagTextEncoding defaultEncoding)
{
    if(encodingArg.isPresent()) {
        const auto &val = encodingArg.values().front();
        if(!strcmp(val, "utf8")) {
            return TagTextEncoding::Utf8;
        } else if(!strcmp(val, "latin1")) {
            return TagTextEncoding::Latin1;
        } else if(!strcmp(val, "utf16be")) {
            return TagTextEncoding::Utf16BigEndian;
        } else if(!strcmp(val, "utf16le")) {
            return TagTextEncoding::Utf16LittleEndian;
        } else if(!strcmp(val, "auto")) {
        } else {
            cerr << Phrases::Warning << "The specified encoding \"" << val << "\" is invalid and will be ignored." << Phrases::EndFlush;
        }
    }
    return defaultEncoding;
}

ElementPosition parsePositionDenotation(const Argument &posArg, const Argument &valueArg, ElementPosition defaultPos)
{
    if(posArg.isPresent()) {
        const char *val = valueArg.values(0).front();
        if(!strcmp(val, "front")) {
            return ElementPosition::BeforeData;
        } else if(!strcmp(val, "back")) {
            return ElementPosition::AfterData;
        } else if(!strcmp(val, "keep")) {
            return ElementPosition::Keep;
        } else {
            cerr << Phrases::Warning << "The specified position \"" << val << "\" is invalid and will be ignored." << Phrases::EndFlush;
        }
    }
    return defaultPos;
}

uint64 parseUInt64(const Argument &arg, uint64 defaultValue)
{
    if(arg.isPresent()) {
        try {
            if(*arg.values().front() == '0' && *(arg.values().front() + 1) == 'x') {
                return stringToNumber<uint64>(arg.values().front() + 2, 16);
            } else {
                return stringToNumber<uint64>(arg.values().front());
            }
        } catch(const ConversionException &) {
            cerr << Phrases::Warning << "The specified value \"" << arg.values().front() << "\" is no valid unsigned integer and will be ignored." << Phrases::EndFlush;
        }
    }
    return defaultValue;
}

TagTarget::IdContainerType parseIds(const std::string &concatenatedIds)
{
    auto splittedIds = splitString(concatenatedIds, ",", EmptyPartsTreat::Omit);
    TagTarget::IdContainerType convertedIds;
    convertedIds.reserve(splittedIds.size());
    for(const auto &id : splittedIds) {
        try {
            convertedIds.push_back(stringToNumber<TagTarget::IdType>(id));
        } catch(const ConversionException &) {
            cerr << Phrases::Warning << "The specified ID \"" << id << "\" is invalid and will be ignored." << Phrases::EndFlush;
        }
    }
    return convertedIds;
}

bool applyTargetConfiguration(TagTarget &target, const std::string &configStr)
{
    if(!configStr.empty()) {
        if(configStr.compare(0, 13, "target-level=") == 0) {
            try {
                target.setLevel(stringToNumber<uint64>(configStr.substr(13)));
            } catch (const ConversionException &) {
                cerr << Phrases::Warning << "The specified target level \"" << configStr.substr(13) << "\" is invalid and will be ignored." << Phrases::EndFlush;
            }
        } else if(configStr.compare(0, 17, "target-levelname=") == 0) {
            target.setLevelName(configStr.substr(17));
        } else if(configStr.compare(0, 14, "target-tracks=") == 0) {
            target.tracks() = parseIds(configStr.substr(14));
        } else if(configStr.compare(0, 16, "target-chapters=") == 0) {
            target.chapters() = parseIds(configStr.substr(16));
        } else if(configStr.compare(0, 16, "target-editions=") == 0) {
            target.editions() = parseIds(configStr.substr(16));
        } else if(configStr.compare(0, 19, "target-attachments=") == 0) {
            target.attachments() = parseIds(configStr.substr(17));
        } else if(configStr.compare(0, 13, "target-reset=") == 0) {
            if(*(configStr.data() + 13)) {
                cerr << Phrases::Warning << "Invalid assignment " << (configStr.data() + 13) << " for target-reset will be ignored." << Phrases::EndFlush;
            }
            target.clear();
        } else if(configStr == "target-reset") {
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
    if(!fieldsArg.isPresent()) {
        return fields;
    }

    const vector<const char *> &fieldDenotations = fieldsArg.values();
    FieldScope scope;

    for(const char *fieldDenotationString : fieldDenotations) {
        // check for tag or target specifier
        const auto fieldDenotationLen = strlen(fieldDenotationString);
        if(!strncmp(fieldDenotationString, "tag=", 4)) {
            if(fieldDenotationLen == 4) {
                cerr << Phrases::Warning << "The \"tag\"-specifier has been used with no value(s) and hence is ignored. Possible values are: id3,id3v1,id3v2,itunes,vorbis,matroska,all" << Phrases::EndFlush;
            } else {
                TagType tagType = TagType::Unspecified;
                bool error = false;
                for(const auto &part : splitString(fieldDenotationString + 4, ",", EmptyPartsTreat::Omit)) {
                    if(part == "id3v1") {
                        tagType |= TagType::Id3v1Tag;
                    } else if(part == "id3v2") {
                        tagType |= TagType::Id3v2Tag;
                    } else if(part == "id3") {
                        tagType |= TagType::Id3v1Tag | TagType::Id3v2Tag;
                    } else if(part == "itunes" || part == "mp4") {
                        tagType |= TagType::Mp4Tag;
                    } else if(part == "vorbis") {
                        tagType |= TagType::VorbisComment;
                    } else if(part == "matroska") {
                        tagType |= TagType::MatroskaTag;
                    } else if(part == "all" || part == "any") {
                        tagType = TagType::Unspecified;
                        break;
                    } else {
                        cerr << Phrases::Warning << "The value provided with the \"tag\"-specifier is invalid and will be ignored. Possible values are: id3,id3v1,id3v2,itunes,vorbis,matroska,all" << Phrases::EndFlush;
                        error = true;
                        break;
                    }
                }
                if(!error) {
                    scope.tagType = tagType;
                    scope.allTracks = false;
                    scope.trackIds.clear();
                }
            }
            continue;
        } else if(applyTargetConfiguration(scope.tagTarget, fieldDenotationString)) {
            continue;
        } else if(!strncmp(fieldDenotationString, "track=", 6)) {
            const vector<string> parts = splitString<vector<string>>(fieldDenotationString + 6, ",", EmptyPartsTreat::Omit);
            bool allTracks = scope.allTracks;
            vector<uint64> trackIds;
            trackIds.reserve(parts.size());
            bool error = false;
            for(const auto &part : parts) {
                if(part == "all" || part == "any") {
                    allTracks = true;
                    break;
                } else {
                    try {
                        trackIds.emplace_back(stringToNumber<uint64>(part));
                    } catch(const ConversionException &) {
                        cerr << Phrases::Warning << "The value provided with the \"track\"-specifier is invalid and will be ignored. It must be a comma-separated list of track IDs." << Phrases::EndFlush;
                        error = true;
                        break;
                    }
                }
            }
            if(!error) {
                scope.allTracks = allTracks;
                scope.trackIds = move(trackIds);
            }
            continue;
        }

        // check whether field name starts with + indicating an additional value
        bool additionalValue = *fieldDenotationString == '+';
        if(additionalValue) {
            ++fieldDenotationString;
        }

        // read field name
        const auto equationPos = strchr(fieldDenotationString, '=');
        size_t fieldNameLen = equationPos ? static_cast<size_t>(equationPos - fieldDenotationString) : strlen(fieldDenotationString);
        // field name might denote increment ("+") or path disclosure (">")
        DenotationType type = DenotationType::Normal;
        if(fieldNameLen && equationPos) {
            switch(*(equationPos - 1)) {
            case '+':
                type = DenotationType::Increment;
                --fieldNameLen;
                break;
            case '>':
                type = DenotationType::File;
                --fieldNameLen;
                break;
            default:
                ;
            }
        }

        // field name might specify a file index
        unsigned int fileIndex = 0, mult = 1;
        for(const char *digitPos = fieldDenotationString + fieldNameLen - 1; fieldNameLen && isDigit(*digitPos); --fieldNameLen, --digitPos, mult *= 10) {
            fileIndex += static_cast<unsigned int>(*digitPos - '0') * mult;
        }
        if(!fieldNameLen) {
            cerr << Phrases::Warning << "Ignoring field denotation \"" << fieldDenotationString << "\" because no field name has been specified." << Phrases::EndFlush;
            continue;
        }

        // parse the denoted field ID
        try {
            if(scope.isTrack()) {
                scope.field = FieldId::fromTrackDenotation(fieldDenotationString, fieldNameLen);
            } else {
                scope.field = FieldId::fromTagDenotation(fieldDenotationString, fieldNameLen);
            }
        } catch(const ConversionException &e) {
            // unable to parse field ID denotation -> discard the field denotation
            cerr << Phrases::Warning << "The field denotation \"" << string(fieldDenotationString, fieldNameLen) << "\" could not be parsed and will be ignored: " << e.what() << Phrases::EndFlush;
            continue;
        }

        // read cover always from file
        if(scope.field.knownField() == KnownField::Cover) {
            type = DenotationType::File;
        }

        // add field denotation scope
        auto &fieldValues = fields[scope];
        // add value to the scope (if present)
        if(equationPos) {
            if(readOnly) {
                cerr << Phrases::Warning << "Specified value for \"" << string(fieldDenotationString, fieldNameLen) << "\" will be ignored." << Phrases::EndFlush;
            } else {
                // file index might have been specified explicitely
                // if not (mult == 1) use the index of the last value and increase it by one if the value is not an additional one
                // if there are no previous values, just use the index 0
                fieldValues.allValues.emplace_back(type, mult == 1 ? (fieldValues.allValues.empty() ? 0 : fieldValues.allValues.back().fileIndex + (additionalValue ? 0 : 1)) : fileIndex, equationPos + 1);
            }
        }
        if(additionalValue && readOnly) {
            cerr << Phrases::Warning << "Indication of an additional value for \"" << string(fieldDenotationString, fieldNameLen) << "\" will be ignored." << Phrases::EndFlush;
        }
    }
    return fields;
}

template<class ConcreteTag>
std::vector<const TagValue *> valuesForNativeField(const char *idString, std::size_t idStringSize, const Tag *tag, TagType tagType)
{
    if(tagType != ConcreteTag::tagType) {
        return vector<const TagValue *>();
    }
    return static_cast<const ConcreteTag *>(tag)->values(ConcreteTag::fieldType::fieldIdFromString(idString, idStringSize));
}

template<class ConcreteTag>
bool setValuesForNativeField(const char *idString, std::size_t idStringSize, Tag *tag, TagType tagType, const std::vector<TagValue> &values)
{
    if(tagType != ConcreteTag::tagType) {
        return false;
    }
    return static_cast<ConcreteTag *>(tag)->setValues(ConcreteTag::fieldType::fieldIdFromString(idString, idStringSize), values);
}

inline FieldId::FieldId(const char *nativeField, std::size_t nativeFieldSize, const GetValuesForNativeFieldType &valuesForNativeField, const SetValuesForNativeFieldType &setValuesForNativeField) :
    m_knownField(KnownField::Invalid),
    m_nativeField(nativeField, nativeFieldSize),
    m_valuesForNativeField(valuesForNativeField),
    m_setValuesForNativeField(setValuesForNativeField)
{}

/// \remarks This wrapper is required because specifying c'tor template args is not possible.
template<class ConcreteTag>
FieldId FieldId::fromNativeField(const char *nativeFieldId, std::size_t nativeFieldIdSize)
{
    return FieldId(
        nativeFieldId, nativeFieldIdSize,
        bind(&valuesForNativeField<ConcreteTag>, nativeFieldId, nativeFieldIdSize, _1, _2),
        bind(&setValuesForNativeField<ConcreteTag>, nativeFieldId, nativeFieldIdSize, _1, _2, _3)
    );
}

FieldId FieldId::fromTagDenotation(const char *denotation, size_t denotationSize)
{
    // check for native, format-specific denotation
    if(!strncmp(denotation, "mkv:", 4)) {
        return FieldId::fromNativeField<MatroskaTag>(denotation + 4, denotationSize - 4);
    } else if(!strncmp(denotation, "mp4:", 4)) {
        return FieldId::fromNativeField<Mp4Tag>(denotation + 4, denotationSize - 4);
    } else if(!strncmp(denotation, "vorbis:", 7)) {
        return FieldId::fromNativeField<VorbisComment>(denotation + 7, denotationSize - 7);
    } else if(!strncmp(denotation, "id3:", 7)) {
        return FieldId::fromNativeField<Id3v2Tag>(denotation + 4, denotationSize - 4);
    } else if(!strncmp(denotation, "generic:", 8)) {
        // allow prefix 'generic:' for consistency
        denotation += 8, denotationSize -= 8;
    }

    // determine KnownField for generic denotation
    static const struct {
        const char *knownDenotation;
        KnownField knownField;
    } fieldMapping[] = {
        {"title", KnownField::Title},
        {"album", KnownField::Album},
        {"artist", KnownField::Artist},
        {"genre", KnownField::Genre},
        {"year", KnownField::Year},
        {"comment", KnownField::Comment},
        {"bpm", KnownField::Bpm},
        {"bps", KnownField::Bps},
        {"lyricist", KnownField::Lyricist},
        {"track", KnownField::TrackPosition},
        {"disk", KnownField::DiskPosition},
        {"part", KnownField::PartNumber},
        {"totalparts", KnownField::TotalParts},
        {"encoder", KnownField::Encoder},
        {"recorddate", KnownField::RecordDate},
        {"performers", KnownField::Performers},
        {"duration", KnownField::Length},
        {"language", KnownField::Language},
        {"encodersettings", KnownField::EncoderSettings},
        {"lyrics", KnownField::Lyrics},
        {"synchronizedlyrics", KnownField::SynchronizedLyrics},
        {"grouping", KnownField::Grouping},
        {"recordlabel", KnownField::RecordLabel},
        {"cover", KnownField::Cover},
        {"composer", KnownField::Composer},
        {"rating", KnownField::Rating},
        {"description", KnownField::Description},
    };
    for(const auto &mapping : fieldMapping) {
        if(!strncmp(denotation, mapping.knownDenotation, denotationSize)) {
            return FieldId(mapping.knownField, nullptr, 0);
        }
    }
    throw ConversionException("generic field name is unknown");
}

FieldId FieldId::fromTrackDenotation(const char *denotation, size_t denotationSize)
{
    return FieldId(KnownField::Invalid, denotation, denotationSize);
}

std::vector<const TagValue *> FieldId::values(const Tag *tag, TagType tagType) const
{
    if(!m_nativeField.empty()) {
        return m_valuesForNativeField(tag, tagType);
    } else {
        return tag->values(m_knownField);
    }
}

bool FieldId::setValues(Tag *tag, TagType tagType, const std::vector<TagValue> &values) const
{
    if(!m_nativeField.empty()) {
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
    if(tagType == TagType::Id3v2Tag) {
        // version only interesting for ID3v2 tags?
        ss << " (version " << tag->version() << ')';
    }
    if(tagType == TagType::MatroskaTag || !tag->target().isEmpty()) {
        ss << " targeting \"" << tag->targetString() << '\"';
    }
    return ss.str();
}

bool stringToBool(const string &str)
{
    if(str == "yes" || str == "true" || str == "1") {
        return true;
    } else if(str == "no" || str == "false" || str == "0") {
        return false;
    }
    throw ConversionException(argsToString('\"', str, "\" is not yes or no"));
}

bool logLineFinalized = true;
static string lastLoggedStatus;
void logStatus(const StatusProvider &statusProvider)
{
    if(statusProvider.currentStatus() != lastLoggedStatus) {
        // the ongoing operation ("status") has changed
        //  -> finalize previous line and make new line
        if(!logLineFinalized) {
            cout << "\r - [100%] " << lastLoggedStatus << endl;
            logLineFinalized = true;
        }
        // -> update lastStatus
        lastLoggedStatus = statusProvider.currentStatus();
    }

    // update current line if an operation is ongoing (status is not empty)
    if(!lastLoggedStatus.empty()) {
        int percentage = static_cast<int>(statusProvider.currentPercentage() * 100);
        if(percentage < 0) {
            percentage = 0;
        }
        cout << "\r - [" << setw(3) << percentage << "%] " << lastLoggedStatus << flush;
        logLineFinalized = false;
    }
}

void finalizeLog()
{
    if(logLineFinalized) {
        return;
    }
    cout << '\n';
    logLineFinalized = true;
    lastLoggedStatus.clear();
}

}
