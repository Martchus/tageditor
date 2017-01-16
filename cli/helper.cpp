#include "./helper.h"

#include "../application/knownfieldmodel.h"

#include <tagparser/mediafileinfo.h>

#include <c++utilities/application/argumentparser.h>

#include <iostream>
#include <cstring>

using namespace std;
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

void printField(const FieldScope &scope, const Tag *tag, bool skipEmpty)
{
    const auto &values = tag->values(scope.field);
    if(!skipEmpty || !values.empty()) {
        // write field name
        const char *fieldName = KnownFieldModel::fieldName(scope.field);
        const auto fieldNameLen = strlen(fieldName);

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
            // parse the denoted filed
            if(!strncmp(fieldDenotationString, "title", fieldNameLen)) {
                scope.field = KnownField::Title;
            } else if(!strncmp(fieldDenotationString, "album", fieldNameLen)) {
                scope.field = KnownField::Album;
            } else if(!strncmp(fieldDenotationString, "artist", fieldNameLen)) {
                scope.field = KnownField::Artist;
            } else if(!strncmp(fieldDenotationString, "genre", fieldNameLen)) {
                scope.field = KnownField::Genre;
            } else if(!strncmp(fieldDenotationString, "year", fieldNameLen)) {
                scope.field = KnownField::Year;
            } else if(!strncmp(fieldDenotationString, "comment", fieldNameLen)) {
                scope.field = KnownField::Comment;
            } else if(!strncmp(fieldDenotationString, "bpm", fieldNameLen)) {
                scope.field = KnownField::Bpm;
            } else if(!strncmp(fieldDenotationString, "bps", fieldNameLen)) {
                scope.field = KnownField::Bps;
            } else if(!strncmp(fieldDenotationString, "lyricist", fieldNameLen)) {
                scope.field = KnownField::Lyricist;
            } else if(!strncmp(fieldDenotationString, "track", fieldNameLen)) {
                scope.field = KnownField::TrackPosition;
            } else if(!strncmp(fieldDenotationString, "disk", fieldNameLen)) {
                scope.field = KnownField::DiskPosition;
            } else if(!strncmp(fieldDenotationString, "part", fieldNameLen)) {
                scope.field = KnownField::PartNumber;
            } else if(!strncmp(fieldDenotationString, "totalparts", fieldNameLen)) {
                scope.field = KnownField::TotalParts;
            } else if(!strncmp(fieldDenotationString, "encoder", fieldNameLen)) {
                scope.field = KnownField::Encoder;
            } else if(!strncmp(fieldDenotationString, "recorddate", fieldNameLen)) {
                scope.field = KnownField::RecordDate;
            } else if(!strncmp(fieldDenotationString, "performers", fieldNameLen)) {
                scope.field = KnownField::Performers;
            } else if(!strncmp(fieldDenotationString, "duration", fieldNameLen)) {
                scope.field = KnownField::Length;
            } else if(!strncmp(fieldDenotationString, "language", fieldNameLen)) {
                scope.field = KnownField::Language;
            } else if(!strncmp(fieldDenotationString, "encodersettings", fieldNameLen)) {
                scope.field = KnownField::EncoderSettings;
            } else if(!strncmp(fieldDenotationString, "lyrics", fieldNameLen)) {
                scope.field = KnownField::Lyrics;
            } else if(!strncmp(fieldDenotationString, "synchronizedlyrics", fieldNameLen)) {
                scope.field = KnownField::SynchronizedLyrics;
            } else if(!strncmp(fieldDenotationString, "grouping", fieldNameLen)) {
                scope.field = KnownField::Grouping;
            } else if(!strncmp(fieldDenotationString, "recordlabel", fieldNameLen)) {
                scope.field = KnownField::RecordLabel;
            } else if(!strncmp(fieldDenotationString, "cover", fieldNameLen)) {
                scope.field = KnownField::Cover;
                type = DenotationType::File; // read cover always from file
            } else if(!strncmp(fieldDenotationString, "composer", fieldNameLen)) {
                scope.field = KnownField::Composer;
            } else if(!strncmp(fieldDenotationString, "rating", fieldNameLen)) {
                scope.field = KnownField::Rating;
            } else if(!strncmp(fieldDenotationString, "description", fieldNameLen)) {
                scope.field = KnownField::Description;
            } else {
                // no "KnownField" value matching -> discard the field denotation
                cerr << "Warning: The field name \"" << string(fieldDenotationString, fieldNameLen) << "\" is unknown and will be ingored." << endl;
                continue;
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

}
