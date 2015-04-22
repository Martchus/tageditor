#include "mainfeatures.h"
#include "application/knownfieldmodel.h"
#include "misc/utility.h"
#include "misc/htmlinfo.h"

#include <tagparser/mediafileinfo.h>
#include <tagparser/tag.h>

#include <c++utilities/application/failure.h>
#include <c++utilities/application/argumentparser.h>
#include <c++utilities/conversion/stringconversion.h>
#include <c++utilities/conversion/conversionexception.h>
#include <c++utilities/io/ansiescapecodes.h>
#include <c++utilities/misc/memory.h>

#include <QDir>

#include <iostream>

using namespace std;
using namespace ApplicationUtilities;
using namespace ConversionUtilities;
using namespace ChronoUtilities;
using namespace EscapeCodes;
using namespace Utility;
using namespace Settings;
using namespace Media;

namespace Cli {

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

inline TagType &operator|= (TagType &lhs, TagType rhs)
{
    return lhs = static_cast<TagType>(static_cast<unsigned int>(lhs) | static_cast<unsigned int>(rhs));
}

struct FieldDenotation
{
    FieldDenotation();
    bool present;
    DenotationType type;
    TagType tagType;
    TagTarget tagTarget;
    std::vector<std::pair<unsigned int, QString> > values;
};

FieldDenotation::FieldDenotation() :
    present(false),
    type(DenotationType::Normal),
    tagType(TagType::Unspecified)
{}

inline bool isDigit(char c)
{
    return c >= '0' && c <= '9';
}

QString incremented(const QString &str, unsigned int toIncrement = 1)
{
    QString res;
    res.reserve(str.size());
    unsigned int value = 0;
    bool hasValue = false;
    for(const QChar &c : str) {
        if(toIncrement && c.isDigit()) {
            value = value * 10 + static_cast<unsigned int>(c.digitValue());
            hasValue = true;
        } else {
            if(hasValue) {
                res.append(QString::number(value + 1));
                hasValue = false;
                --toIncrement;
            }
            res.append(c);
        }
    }
    return res;
}

void printNotifications(const MediaFileInfo &fileInfo, const char *head = nullptr)
{
    auto notifications = fileInfo.gatherRelatedNotifications();
    if(!notifications.empty()) {
        if(head) {
            cout << head << endl;
        }
        Notification::sortByTime(notifications);
        for(const Notification &notification : notifications) {
            switch(notification.type()) {
            case NotificationType::Critical:
                cout << "Error        ";
                break;
            case NotificationType::Debug:
                cout << "Debug        ";
                break;
            case NotificationType::Warning:
                cout << "Warning      ";
                break;
            default:
                cout << "Information  ";
            }
            cout << notification.creationTime().toString(DateTimeOutputFormat::TimeOnly) << "   ";
            cout << notification.context() << ": ";
            cout << notification.message() << endl;
        }
    }
}

void printFieldNames(const StringVector &parameterValues)
{
    VAR_UNUSED(parameterValues)
    cout << "title album artist genre year comment bpm bps lyricist track disk part totalparts encoder\n"
            "recorddate performers duration language encodersettings lyrics synchronizedlyrics grouping\n"
            "recordlabel cover composer rating description" << endl;
}

void removeBackupFiles(const StringVector &parameterValues, const Argument &recursiveArg)
{
    QDir dir(QString::fromStdString(parameterValues.at(0)));
    QStringList affectedFiles;
    int filesFound = Utility::removeBackupFiles(dir, affectedFiles, &cout, recursiveArg.isPresent());
    cout << affectedFiles.size() << " of " << filesFound << " backup files have been removed." << endl;
}

TagUsage parseUsageDenotation(const Argument &usageArg, TagUsage defaultUsage)
{
    if(usageArg.isPresent()) {
        const auto &val = usageArg.values().front();
        if(val == "never") {
            return TagUsage::Never;
        } else if(val == "keepexisting") {
            return TagUsage::KeepExisting;
        } else if(val == "always") {
            return TagUsage::Always;
        } else {
            cout << "Warning: The specified tag usage \"" << val << "\" is invalid and will be ignored." << endl;
        }
    }
    return defaultUsage;
}

TagTextEncoding parseEncodingDenotation(const Argument &encodingArg, TagTextEncoding defaultEncoding)
{
    if(encodingArg.isPresent()) {
        const auto &val = encodingArg.values().front();
        if(val == "utf8") {
            return TagTextEncoding::Utf8;
        } else if(val == "latin1") {
            return TagTextEncoding::Latin1;
        } else if(val == "utf16be") {
            return TagTextEncoding::Utf16BigEndian;
        } else if(val == "utf16le") {
            return TagTextEncoding::Utf16LittleEndian;
        } else if(val == "auto") {
        } else {
            cout << "Warning: The specified encoding \"" << val << "\" is invalid and will be ignored." << endl;
        }
    }
    return defaultEncoding;
}

TagTarget::IdContainerType parseIds(const std::string &concatenatedIds)
{
    auto splittedIds = splitString(concatenatedIds, ",", EmptyPartsTreat::Omit);
    TagTarget::IdContainerType convertedIds;
    convertedIds.reserve(splittedIds.size());
    for(const auto &id : splittedIds) {
        try {
            convertedIds.push_back(stringToNumber<TagTarget::IdType>(id));
        } catch(ConversionException &) {
            cout << "Warning: The specified ID \"" << id << "\" is invalid and will be ignored." << endl;
        }
    }
    return convertedIds;
}

bool applyTargetConfiguration(TagTarget &target, const std::string &configStr)
{
    if(strncmp(configStr.c_str(), "target-level:", 13) == 0) {
        try {
            target.setLevel(stringToNumber<uint64>(configStr.substr(13)));
        } catch (ConversionException &) {
            cout << "Warning: The specified target level \"" << configStr.substr(13) << "\" is invalid and will be ignored." << endl;
        }
    } else if(configStr.compare(0, 17, "target-levelname:") == 0) {
        target.setLevelName(configStr.substr(17));
    } else if(configStr.compare(0, 14, "target-tracks:") == 0) {
        target.tracks() = parseIds(configStr.substr(14));
    } else if(configStr.compare(0, 16, "target-chapters:") == 0) {
        target.chapters() = parseIds(configStr.substr(16));
    } else if(configStr.compare(0, 16, "target-editions:") == 0) {
        target.editions() = parseIds(configStr.substr(16));
    } else if(configStr.compare(0, 17, "target-attachments:") == 0) {
        target.attachments() = parseIds(configStr.substr(17));
    } else if(configStr == "target-reset") {
        target.clear();
    } else {
        return false;
    }
    return true;
}

unsigned int parseFieldDenotations(const StringVector &fieldDenotations, bool readOnly, FieldDenotation *fields)
{
    unsigned int validDenotations = 0;
    TagType currentTagType = TagType::Unspecified;
    TagTarget currentTagTarget;
    for(const string &fieldDenotationString : fieldDenotations) {
        // check for tag or target specifier
        if(strncmp(fieldDenotationString.c_str(), "tag:", 4) == 0) {
            if(fieldDenotationString.size() == 4) {
                cout << "Warning: The \"tag\"-specifier has been used with no value(s) and hence is ignored. Possible values are: id3,id3v1,id3v2,itunes,vorbis,matroska,all" << endl;
            } else {
                TagType tagType = TagType::Unspecified;
                for(const auto &part : splitString(fieldDenotationString.substr(4), ",", EmptyPartsTreat::Omit)) {
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
                        cout << "Warning: The value provided with the \"tag\"-specifier is invalid and will be ignored. Possible values are: id3,id3v1,id3v2,itunes,vorbis,matroska,all" << endl;
                        tagType = currentTagType;
                        break;
                    }
                }
                currentTagType = tagType;
                break;
            }
        } else if(applyTargetConfiguration(currentTagTarget, fieldDenotationString)) {
            continue;
        }
        // read field name
        auto equationPos = fieldDenotationString.find('=');
        auto fieldName = equationPos != string::npos ? fieldDenotationString.substr(0, equationPos) : fieldDenotationString;        
        // field name might denote increment ("+") or path disclosure (">")
        auto fieldNamePos = fieldName.size();
        DenotationType type = DenotationType::Normal;
        if(fieldNamePos) {
            switch(fieldName.at(--fieldNamePos)) {
            case '+':
                type = DenotationType::Increment;
                --fieldNamePos;
                break;
            case '>':
                type = DenotationType::File;
                --fieldNamePos;
                break;
            default:
                ;
            }
        }
        // field name might specify a file index
        unsigned int fileIndex = 0, mult = 1;
        for(; fieldNamePos != static_cast<string::size_type>(-1) && isDigit(fieldName.at(fieldNamePos)); --fieldNamePos, mult *= 10) {
            fileIndex += static_cast<unsigned int>(fieldName.at(fieldNamePos) - '0') * mult;
        }
        if(fieldNamePos == static_cast<string::size_type>(-1)) {
            cout << "Warning: Ignoring field denotation \"" << fieldDenotationString << "\" because no field name has been specified." << endl;
            continue;
        } else if(++fieldNamePos < fieldName.size()) {
            fieldName = fieldName.substr(0, fieldNamePos);
        }
        // parse the denoted filed
        KnownField field;
        if(fieldName == "title") {
            field = KnownField::Title;
        } else if(fieldName == "album") {
            field = KnownField::Album;
        } else if(fieldName == "artist") {
            field = KnownField::Artist;
        } else if(fieldName == "genre") {
            field = KnownField::Genre;
        } else if(fieldName == "year") {
            field = KnownField::Year;
        } else if(fieldName == "comment") {
            field = KnownField::Comment;
        } else if(fieldName == "bpm") {
            field = KnownField::Bpm;
        } else if(fieldName == "bps") {
            field = KnownField::Bps;
        } else if(fieldName == "lyricist") {
            field = KnownField::Lyricist;
        } else if(fieldName == "track") {
            field = KnownField::TrackPosition;
        } else if(fieldName == "disk") {
            field = KnownField::DiskPosition;
        } else if(fieldName == "part") {
            field = KnownField::PartNumber;
        } else if(fieldName == "totalparts") {
            field = KnownField::TotalParts;
        } else if(fieldName == "encoder") {
            field = KnownField::Encoder;
        } else if(fieldName == "recorddate") {
            field = KnownField::RecordDate;
        } else if(fieldName == "performers") {
            field = KnownField::Performers;
        } else if(fieldName == "duration") {
            field = KnownField::Length;
        } else if(fieldName == "language") {
            field = KnownField::Language;
        } else if(fieldName == "encodersettings") {
            field = KnownField::EncoderSettings;
        } else if(fieldName == "lyrics") {
            field = KnownField::Lyrics;
        } else if(fieldName == "synchronizedlyrics") {
            field = KnownField::SynchronizedLyrics;
        } else if(fieldName == "grouping") {
            field = KnownField::Grouping;
        } else if(fieldName == "recordlabel") {
            field = KnownField::RecordLabel;
        } else if(fieldName == "cover") {
            field = KnownField::Cover;
            type = DenotationType::File; // read cover always from file
        } else if(fieldName == "composer") {
            field = KnownField::Composer;
        } else if(fieldName == "rating") {
            field = KnownField::Rating;
        } else if(fieldName == "description") {
            field = KnownField::Description;
        } else {
            // no "KnownField" value matching -> discard the field denotation
            cout << "The field name \"" << fieldName << "\" is unknown and will be ingored." << endl;
            continue;
        }
        // add field denotation with parsed values
        FieldDenotation &fieldDenotation = fields[static_cast<unsigned int>(field)];
        fieldDenotation.present = true;
        fieldDenotation.type = type;
        fieldDenotation.tagType = currentTagType;
        fieldDenotation.tagTarget = currentTagTarget;
        if(equationPos != string::npos) {
            if(readOnly) {
                cout << "Warning: Specified value for \"" << fieldName << "\" will be ignored." << endl;
            } else {
                fieldDenotation.values.emplace_back(make_pair(mult == 1 ? fieldDenotation.values.size() : fileIndex, QString::fromLocal8Bit(fieldDenotationString.c_str() + equationPos + 1)));
            }
        }
        ++validDenotations;
    }
    return validDenotations;
}

void generateFileInfo(const StringVector &parameterValues, const Argument &inputFileArg, const Argument &outputFileArg, const Argument &validateArg)
{
    Q_UNUSED(parameterValues)
    try {
        // parse tags
        MediaFileInfo inputFileInfo(inputFileArg.values().front());
        inputFileInfo.setForceFullParse(validateArg.isPresent());
        inputFileInfo.open(true);
        inputFileInfo.parseEverything();
        cout << "Saving file info of \"" << inputFileArg.values().front() << "\" ..." << endl;
        NotificationList origNotify;
        QFile file(QString::fromLocal8Bit(outputFileArg.values().front().c_str()));
        if(file.open(QFile::WriteOnly) && file.write(HtmlInfo::generateInfo(inputFileInfo, origNotify)) && file.flush()) {
            cout << "File information has been saved to \"" << outputFileArg.values().front() << "\"." << endl;
        } else {
            cout << "Error: An IO error occured when writing the file \"" << outputFileArg.values().front() << "\"." << endl;
        }
    } catch(ios_base::failure &) {
        cout << "Error: An IO failure occured when reading the file \"" << inputFileArg.values().front() << "\"." << endl;
    } catch(ApplicationUtilities::Failure &) {
        cout << "Error: A parsing failure occured when reading the file \"" << inputFileArg.values().front() << "\"." << endl;
    }
}

void displayTagInfo(const StringVector &parameterValues, const Argument &filesArg)
{
    if(!filesArg.valueCount()) {
        cout << "Error: No files have been specified." << endl;
        return;
    }
    FieldDenotation fields[knownFieldArraySize];
    unsigned int validDenotations = parseFieldDenotations(parameterValues, true, fields);
    MediaFileInfo fileInfo;
    for(const auto &file : filesArg.values()) {
        try {
            // parse tags
            fileInfo.setPath(file);
            fileInfo.open(true);
            fileInfo.parseTags();
            cout << "Tag information for \"" << file << "\":" << endl;
            auto tags = fileInfo.tags();
            if(tags.size()) {
                // iterate through all tags
                for(const auto *tag : tags) {
                    // determine tag type
                    TagType tagType = tag->type();
                    // write tag name and target, eg. MP4/iTunes tag
                    cout << tag->typeName();
                    if(!tag->target().isEmpty()) {
                        cout << " targeting \"" << tag->target().toString() << "\"";
                    }
                    cout << endl;
                    // iterate through fields specified by the user
                    KnownField field = firstKnownField;
                    for(const FieldDenotation &fieldDenotation : fields) {
                        const auto &value = tag->value(field);
                        if((!validDenotations && !value.isEmpty())
                                || (fieldDenotation.present
                                    && (fieldDenotation.tagType == TagType::Unspecified
                                        || (fieldDenotation.tagType | tagType) != TagType::Unspecified))) {
                            // write field name
                            const char *fieldName = KnownFieldModel::fieldName(field);
                            cout << ' ' << fieldName;
                            // write padding
                            for(auto i = strlen(fieldName); i < 18; ++i) {
                                cout << ' ';
                            }
                            // write value
                            if(value.isEmpty()) {
                                cout << "none";
                            } else {
                                try {
                                    QString textValue = tagValueToQString(value);
                                    if(textValue.isEmpty()) {
                                        cout << "can't display here (see --extract)";
                                    } else {
                                        cout << textValue.toLocal8Bit().data();
                                    }
                                } catch(ConversionException &) {
                                    cout << "conversion error";
                                }
                            }
                            cout << endl;
                        }
                        field = nextKnownField(field);
                    }
                }
            } else {
                cout << " File has no (supported) tag information." << endl;
            }
        } catch(ios_base::failure &) {
            cout << "Error: An IO failure occured when reading the file \"" << file << "\"." << endl;
        } catch(ApplicationUtilities::Failure &) {
            cout << "Error: A parsing failure occured when reading the file \"" << file << "\"." << endl;
        }
        printNotifications(fileInfo, "Parsing notifications:");
    }
}

void setTagInfo(const StringVector &parameterValues, const Argument &filesArg, const Argument &removeOtherFieldsArg,
                const Argument &treatUnknownFilesAsMp3FilesArg, const Argument &id3v1UsageArg, const Argument &id3v2UsageArg,
                const Argument &mergeMultipleSuccessiveTagsArg, const Argument &id3v2VersionArg, const Argument &encodingArg)
{
    if(!filesArg.valueCount()) {
        cout << "Error: No files have been specified." << endl;
        return;
    }
    FieldDenotation fields[knownFieldArraySize];
    unsigned int validDenotations = parseFieldDenotations(parameterValues, false, fields);
    if(!validDenotations) {
        cout << "Error: No fields have been specified." << endl;
        return;
    }
    uint32 id3v2Version = 3;
    if(id3v2VersionArg.isPresent()) {
        try {
            id3v2Version = stringToNumber<uint32>(id3v2VersionArg.values().front());
            if(id3v2Version < 1 || id3v2Version > 4) {
                throw ConversionException();
            }
        } catch (ConversionException &) {
            id3v2Version = 3;
            cout << "Warning: The specified ID3v2 version \"" << id3v2VersionArg.values().front() << "\" is invalid and will be ingored." << endl;
        }
    }
    TagTextEncoding encoding = parseEncodingDenotation(encodingArg, TagTextEncoding::Utf8);
    TagUsage id3v1Usage = parseUsageDenotation(id3v1UsageArg, TagUsage::KeepExisting);
    TagUsage id3v2Usage = parseUsageDenotation(id3v2UsageArg, TagUsage::Always);
    unsigned int fileIndex = 0;
    MediaFileInfo fileInfo;
    for(const auto &file : filesArg.values()) {
        try {
            // parse tags
            fileInfo.setPath(file);
            fileInfo.parseTags();
            cout << "Setting tag information for \"" << file << "\" ..." << endl;
            auto tags = fileInfo.tags();       
            fileInfo.createAppropriateTags(treatUnknownFilesAsMp3FilesArg.isPresent(), id3v1Usage, id3v2Usage, mergeMultipleSuccessiveTagsArg.isPresent(), !id3v2VersionArg.isPresent(), id3v2Version);
            if(tags.size()) {
                // iterate through all tags
                for(auto *tag : tags) {
                    if(removeOtherFieldsArg.isPresent()) {
                        tag->removeAllFields();
                    }
                    TagType tagType = tag->type();
                    bool targetSupported = tag->supportsTarget();
                    TagTarget tagTarget = tag->target();
                    KnownField field = firstKnownField;
                    for(FieldDenotation &fieldDenotation : fields) {
                        if(fieldDenotation.present
                                && (fieldDenotation.tagType == TagType::Unspecified
                                    || (fieldDenotation.tagType | tagType) != TagType::Unspecified)
                                && (!targetSupported || fieldDenotation.tagTarget == tagTarget)) {
                            pair<unsigned int, QString> *selectedDenotatedValue = nullptr;
                            for(auto &someDenotatedValue : fieldDenotation.values) {
                                if(someDenotatedValue.first <= fileIndex) {
                                    if(!selectedDenotatedValue || (someDenotatedValue.first > selectedDenotatedValue->first)) {
                                        selectedDenotatedValue = &someDenotatedValue;
                                    }
                                }
                            }
                            if(selectedDenotatedValue) {
                                if(fieldDenotation.type == DenotationType::File) {
                                    if(selectedDenotatedValue->second.isEmpty()) {
                                        tag->setValue(field, TagValue());
                                    } else {
                                        try {
                                            MediaFileInfo fileInfo(selectedDenotatedValue->second.toLocal8Bit().constData());
                                            fileInfo.open(true);
                                            fileInfo.parseContainerFormat();
                                            unique_ptr<char[]> buff = make_unique<char []>(fileInfo.size());
                                            fileInfo.stream().seekg(0);
                                            fileInfo.stream().read(buff.get(), fileInfo.size());
                                            TagValue value(move(buff), fileInfo.size(), TagDataType::Picture);
                                            value.setMimeType(fileInfo.mimeType());
                                            tag->setValue(field, move(value));
                                        } catch (ios_base::failure &) {
                                            cout << "An IO error occured when parsing the specified cover file." << endl;
                                        } catch (Media::Failure &) {
                                            cout << "Unable to parse specified cover file." << endl;
                                        }
                                    }
                                } else {
                                    TagTextEncoding usedEncoding = encoding;
                                    if(!tag->canEncodingBeUsed(encoding)) {
                                        usedEncoding = tag->proposedTextEncoding();
                                    }
                                    tag->setValue(field, qstringToTagValue(selectedDenotatedValue->second, usedEncoding));
                                    if(fieldDenotation.type == DenotationType::Increment && tag == tags.back()) {
                                        selectedDenotatedValue->second = incremented(selectedDenotatedValue->second);
                                    }
                                }
                            }
                        }
                        field = nextKnownField(field);
                    }
                }
                try {
                    fileInfo.applyChanges();
                    cout << "Changes have been applied." << endl;
                } catch(ApplicationUtilities::Failure &) {
                    cout << "A failure occured when applying the changes." << endl;
                }
            } else {
                cout << " Can not create appropriate tags for file." << endl;
            }
        } catch(ios_base::failure &) {
            cout << "Error: An IO failure occured when reading/writing the file \"" << file << "\"." << endl;
        } catch(ApplicationUtilities::Failure &) {
            cout << "Error: A parsing failure occured when reading/writing the file \"" << file << "\"." << endl;
        }
        printNotifications(fileInfo, "Notifications:");
        ++fileIndex;
    }
}

void extractField(const StringVector &parameterValues, const Argument &inputFileArg, const Argument &outputFileArg)
{
    FieldDenotation fields[knownFieldArraySize];
    unsigned int validDenotations = parseFieldDenotations(parameterValues, true, fields);
    if(validDenotations != 1) {
        cout << "Error: Excactly one field needs to be specified." << endl;
        return;
    }
    MediaFileInfo inputFileInfo;
    try {
        // parse tags
        inputFileInfo.setPath(inputFileArg.values().front());
        inputFileInfo.open(true);
        inputFileInfo.parseTags();
        cout << "Extracting " << parameterValues.front() << " of \"" << inputFileArg.values().front() << "\" ..." << endl;
        auto tags = inputFileInfo.tags();
        vector<pair<const TagValue *, string> > values;
        // iterate through all tags
        for(const Tag *tag : tags) {
            auto field = firstKnownField;
            for(const auto &fieldDenotation : fields) {
                if(fieldDenotation.present) {
                    const auto &value = tag->value(field);
                    if(!value.isEmpty()) {
                        values.emplace_back(&value, joinStrings({tag->typeName(), numberToString(values.size())}, "-"));
                    }
                }
                field = nextKnownField(field);
            }
        }
        if(values.empty()) {
            cout << "File has no (supported) " << parameterValues.front() << " field." << endl;
        } else {
            string outputFilePathWithoutExtension, outputFileExtension;
            if(values.size() > 1) {
                outputFilePathWithoutExtension = BasicFileInfo::pathWithoutExtension(outputFileArg.values().front());
                outputFileExtension = BasicFileInfo::extension(outputFileArg.values().front());
            }
            for(const auto &value : values) {
                fstream outputFileStream;
                outputFileStream.exceptions(ios_base::failbit | ios_base::badbit);
                auto path = values.size() > 1 ? joinStrings({outputFilePathWithoutExtension, "-", value.second, outputFileExtension}) : outputFileArg.values().front();
                try {
                    outputFileStream.open(path, ios_base::out | ios_base::binary);
                    outputFileStream.write(value.first->dataPointer(), value.first->dataSize());
                    outputFileStream.flush();
                    cout << "Value has been saved to \"" << path << "\"." << endl;
                } catch(ios_base::failure &) {
                    cout << "An IO error occured when writing the file \"" << path << "\"." << endl;
                }
            }
        }
    } catch(ios_base::failure &) {
        cout << "Error: An IO failure occured when reading the file \"" << inputFileArg.values().front() << "\"." << endl;
    } catch(ApplicationUtilities::Failure &) {
        cout << "Error: A parsing failure occured when reading the file \"" << inputFileArg.values().front() << "\"." << endl;
    }
    printNotifications(inputFileInfo, "Parsing notifications:");
}

}
