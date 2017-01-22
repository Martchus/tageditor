#include "./helper.h"

#include <tagparser/mediafileinfo.h>
#include <tagparser/matroska/matroskatag.h>
#include <tagparser/mp4/mp4tag.h>
#include <tagparser/vorbis/vorbiscomment.h>
#include <tagparser/id3/id3v2tag.h>

#include <c++utilities/application/argumentparser.h>

#include <iostream>
#include <cstring>

using namespace std;
using namespace std::placeholders;
using namespace ApplicationUtilities;
using namespace ConversionUtilities;
using namespace ChronoUtilities;
using namespace Media;
using namespace Settings;

namespace Cli {

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
    if(!notifications.empty()) {
printNotifications:
        if(head) {
            cout << head << endl;
        }
        Notification::sortByTime(notifications);
        for(const auto &notification : notifications) {
            switch(notification.type()) {
            case NotificationType::Debug:
                if(beVerbose) {
                    cout << "Debug        ";
                    break;
                } else {
                    continue;
                }
            case NotificationType::Information:
                if(beVerbose) {
                    cout << "Information  ";
                    break;
                } else {
                    continue;
                }
            case NotificationType::Warning:
                cout << "Warning      ";
                break;
            case NotificationType::Critical:
                cout << "Error        ";
                break;
            default:
                ;
            }
            cout << notification.creationTime().toString(DateTimeOutputFormat::TimeOnly) << "   ";
            cout << notification.context() << ": ";
            cout << notification.message() << endl;
        }
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
    const char *pos;
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
    cout << ' ' << fieldName;
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
        const auto &values = scope.field.values(tag, tagType);
        if(!skipEmpty || !values.empty()) {
            // write value
            if(values.empty()) {
                printFieldName(fieldName, fieldNameLen);
                cout << "none\n";
            } else {
                for(const auto &value : values) {
                    printFieldName(fieldName, fieldNameLen);
                    try {
                        const auto textValue = value->toString(TagTextEncoding::Utf8);
                        if(textValue.empty()) {
                            cout << "can't display here (see --extract)";
                        } else {
                            cout << textValue;
                        }
                    } catch(const ConversionException &) {
                        cout << "conversion error";
                    }
                    cout << '\n';
                }
            }
        }
    } catch(const ConversionException &e) {
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
            cerr << "Warning: The specified tag usage \"" << val << "\" is invalid and will be ignored." << endl;
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
            cerr << "Warning: The specified encoding \"" << val << "\" is invalid and will be ignored." << endl;
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
            cerr << "Warning: The specified position \"" << val << "\" is invalid and will be ignored." << endl;
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
            cerr << "Warning: The specified value \"" << arg.values().front() << "\" is no valid unsigned integer and will be ignored." << endl;
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
            cerr << "Warning: The specified ID \"" << id << "\" is invalid and will be ignored." << endl;
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
                cerr << "Warning: The specified target level \"" << configStr.substr(13) << "\" is invalid and will be ignored." << endl;
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
                cerr << "Warning: Invalid assignment " << (configStr.data() + 13) << " for target-reset will be ignored." << endl;
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
    if(fieldsArg.isPresent()) {
        const vector<const char *> &fieldDenotations = fieldsArg.values();
        FieldScope scope;
        for(const char *fieldDenotationString : fieldDenotations) {
            // check for tag or target specifier
            const auto fieldDenotationLen = strlen(fieldDenotationString);
            if(!strncmp(fieldDenotationString, "tag=", 4)) {
                if(fieldDenotationLen == 4) {
                    cerr << "Warning: The \"tag\"-specifier has been used with no value(s) and hence is ignored. Possible values are: id3,id3v1,id3v2,itunes,vorbis,matroska,all" << endl;
                } else {
                    TagType tagType = TagType::Unspecified;
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
                            cerr << "Warning: The value provided with the \"tag\"-specifier is invalid and will be ignored. Possible values are: id3,id3v1,id3v2,itunes,vorbis,matroska,all" << endl;
                            tagType = scope.tagType;
                            break;
                        }
                    }
                    scope.tagType = tagType;
                    break;
                }
            } else if(applyTargetConfiguration(scope.tagTarget, fieldDenotationString)) {
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
                cerr << "Warning: Ignoring field denotation \"" << fieldDenotationString << "\" because no field name has been specified." << endl;
                continue;
            }
            // parse the denoted field ID
            try {
                scope.field = FieldId::fromDenotation(fieldDenotationString, fieldNameLen);
            } catch(const ConversionException &e) {
                // unable to parse field ID denotation -> discard the field denotation
                cerr << "Warning: The field denotation \"" << string(fieldDenotationString, fieldNameLen) << "\" could not be parsed and will be ignored: " << e.what() << endl;
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
                    cerr << "Warning: Specified value for \"" << string(fieldDenotationString, fieldNameLen) << "\" will be ignored." << endl;
                } else {
                    // file index might have been specified explicitely
                    // if not (mult == 1) use the index of the last value and increase it by one if the value is not an additional one
                    // if there are no previous values, just use the index 0
                    fieldValues.emplace_back(FieldValue(type, mult == 1 ? (fieldValues.empty() ? 0 : fieldValues.back().fileIndex + (additionalValue ? 0 : 1)) : fileIndex, (equationPos + 1)));
                }
            }
            if(additionalValue && readOnly) {
                cerr << "Warning: Indication of an additional value for \"" << string(fieldDenotationString, fieldNameLen) << "\" will be ignored." << endl;
            }
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

inline FieldId::FieldId(const char *nativeField, const GetValuesForNativeFieldType &valuesForNativeField, const SetValuesForNativeFieldType &setValuesForNativeField) :
    m_knownField(KnownField::Invalid),
    m_nativeField(nativeField),
    m_valuesForNativeField(valuesForNativeField),
    m_setValuesForNativeField(setValuesForNativeField)
{}

/// \remarks This wrapper is required because specifying c'tor template args is not possible.
template<class ConcreteTag>
FieldId FieldId::fromNativeField(const char *nativeFieldId, std::size_t nativeFieldIdSize)
{
    return FieldId(
        nativeFieldId,
        bind(&valuesForNativeField<ConcreteTag>, nativeFieldId, nativeFieldIdSize, _1, _2),
        bind(&setValuesForNativeField<ConcreteTag>, nativeFieldId, nativeFieldIdSize, _1, _2, _3)
    );
}

FieldId FieldId::fromDenotation(const char *denotation, size_t denotationSize)
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
    if(!strncmp(denotation, "title", denotationSize)) {
        return KnownField::Title;
    } else if(!strncmp(denotation, "album", denotationSize)) {
        return KnownField::Album;
    } else if(!strncmp(denotation, "artist", denotationSize)) {
        return KnownField::Artist;
    } else if(!strncmp(denotation, "genre", denotationSize)) {
        return KnownField::Genre;
    } else if(!strncmp(denotation, "year", denotationSize)) {
        return KnownField::Year;
    } else if(!strncmp(denotation, "comment", denotationSize)) {
        return KnownField::Comment;
    } else if(!strncmp(denotation, "bpm", denotationSize)) {
        return KnownField::Bpm;
    } else if(!strncmp(denotation, "bps", denotationSize)) {
        return KnownField::Bps;
    } else if(!strncmp(denotation, "lyricist", denotationSize)) {
        return KnownField::Lyricist;
    } else if(!strncmp(denotation, "track", denotationSize)) {
        return KnownField::TrackPosition;
    } else if(!strncmp(denotation, "disk", denotationSize)) {
        return KnownField::DiskPosition;
    } else if(!strncmp(denotation, "part", denotationSize)) {
        return KnownField::PartNumber;
    } else if(!strncmp(denotation, "totalparts", denotationSize)) {
        return KnownField::TotalParts;
    } else if(!strncmp(denotation, "encoder", denotationSize)) {
        return KnownField::Encoder;
    } else if(!strncmp(denotation, "recorddate", denotationSize)) {
        return KnownField::RecordDate;
    } else if(!strncmp(denotation, "performers", denotationSize)) {
        return KnownField::Performers;
    } else if(!strncmp(denotation, "duration", denotationSize)) {
        return KnownField::Length;
    } else if(!strncmp(denotation, "language", denotationSize)) {
        return KnownField::Language;
    } else if(!strncmp(denotation, "encodersettings", denotationSize)) {
        return KnownField::EncoderSettings;
    } else if(!strncmp(denotation, "lyrics", denotationSize)) {
        return KnownField::Lyrics;
    } else if(!strncmp(denotation, "synchronizedlyrics", denotationSize)) {
        return KnownField::SynchronizedLyrics;
    } else if(!strncmp(denotation, "grouping", denotationSize)) {
        return KnownField::Grouping;
    } else if(!strncmp(denotation, "recordlabel", denotationSize)) {
        return KnownField::RecordLabel;
    } else if(!strncmp(denotation, "cover", denotationSize)) {
        return KnownField::Cover;
    } else if(!strncmp(denotation, "composer", denotationSize)) {
        return KnownField::Composer;
    } else if(!strncmp(denotation, "rating", denotationSize)) {
        return KnownField::Rating;
    } else if(!strncmp(denotation, "description", denotationSize)) {
        return KnownField::Description;
    } else {
        throw ConversionException("generic field name is unknown");
    }
}

std::vector<const TagValue *> FieldId::values(const Tag *tag, TagType tagType) const
{
    if(m_nativeField) {
        return m_valuesForNativeField(tag, tagType);
    } else {
        return tag->values(m_knownField);
    }
}

bool FieldId::setValues(Tag *tag, TagType tagType, const std::vector<TagValue> &values) const
{
    if(m_nativeField) {
        return m_setValuesForNativeField(tag, tagType, values);
    } else {
        return tag->setValues(m_knownField, values);
    }
}

}
