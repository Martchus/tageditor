#include "./mainfeatures.h"
#include "./helper.h"
#include "./attachmentinfo.h"

#include "../application/knownfieldmodel.h"
#if defined(GUI_QTWIDGETS) || defined(GUI_QTQUICK)
# include "../misc/utility.h"
# include "../misc/htmlinfo.h"
#endif

#include <tagparser/mediafileinfo.h>
#include <tagparser/tag.h>
#include <tagparser/tagvalue.h>
#include <tagparser/abstracttrack.h>
#include <tagparser/abstractattachment.h>
#include <tagparser/abstractchapter.h>

#include <c++utilities/application/failure.h>
#include <c++utilities/application/commandlineutils.h>
#include <c++utilities/conversion/stringconversion.h>
#include <c++utilities/conversion/conversionexception.h>
#include <c++utilities/io/ansiescapecodes.h>
#include <c++utilities/io/catchiofailure.h>
#include <c++utilities/io/nativefilestream.h>
#include <c++utilities/misc/memory.h>

#if defined(GUI_QTWIDGETS) || defined(GUI_QTQUICK)
# include <QDir>
# include <qtutilities/misc/conversion.h>
#endif

#include <iostream>
#include <cstring>
#include <algorithm>

using namespace std;
using namespace ApplicationUtilities;
using namespace ConversionUtilities;
using namespace ChronoUtilities;
using namespace IoUtilities;
using namespace EscapeCodes;
using namespace Settings;
using namespace Media;
#if defined(GUI_QTWIDGETS) || defined(GUI_QTQUICK)
using namespace Utility;
#endif

namespace Cli {

#define FIELD_NAMES "title album artist genre year comment bpm bps lyricist track disk part totalparts encoder\n" \
    "recorddate performers duration language encodersettings lyrics synchronizedlyrics grouping\n" \
    "recordlabel cover composer rating description"

#define TAG_MODIFIER "tag=id3v1 tag=id3v2 tag=id3 tag=itunes tag=vorbis tag=matroska tag=all"
#define TARGET_MODIFIER "target-level target-levelname target-tracks target-tracks\n" \
    "target-chapters target-editions target-attachments target-reset"

const char *const fieldNames = FIELD_NAMES;
const char *const fieldNamesForSet = FIELD_NAMES " " TAG_MODIFIER " " TARGET_MODIFIER;

void printFieldNames(const ArgumentOccurrence &occurrence)
{
    CMD_UTILS_START_CONSOLE;
    VAR_UNUSED(occurrence)
            cout << fieldNames;
    cout << "\nTag modifier: " << TAG_MODIFIER;
    cout << "\nTarget modifier: " << TARGET_MODIFIER << endl;
}

void generateFileInfo(const ArgumentOccurrence &, const Argument &inputFileArg, const Argument &outputFileArg, const Argument &validateArg)
{
    CMD_UTILS_START_CONSOLE;
#if defined(GUI_QTWIDGETS) || defined(GUI_QTQUICK)
    try {
        // parse tags
        MediaFileInfo inputFileInfo(inputFileArg.values().front());
        inputFileInfo.setForceFullParse(validateArg.isPresent());
        inputFileInfo.open(true);
        inputFileInfo.parseEverything();
        (outputFileArg.isPresent() ? cout : cerr) << "Saving file info for \"" << inputFileArg.values().front() << "\" ..." << endl;
        NotificationList origNotify;
        if(outputFileArg.isPresent()) {
            QFile file(fromNativeFileName(outputFileArg.values().front()));
            if(file.open(QFile::WriteOnly) && file.write(HtmlInfo::generateInfo(inputFileInfo, origNotify)) && file.flush()) {
                cout << "File information has been saved to \"" << outputFileArg.values().front() << "\"." << endl;
            } else {
                cerr << "Error: An IO error occured when writing the file \"" << outputFileArg.values().front() << "\"." << endl;
            }
        } else {
            cout << HtmlInfo::generateInfo(inputFileInfo, origNotify).data() << endl;
        }
    } catch(const ApplicationUtilities::Failure &) {
        cerr << "Error: A parsing failure occured when reading the file \"" << inputFileArg.values().front() << "\"." << endl;
    } catch(...) {
        ::IoUtilities::catchIoFailure();
        cerr << "Error: An IO failure occured when reading the file \"" << inputFileArg.values().front() << "\"." << endl;
    }
#else
    VAR_UNUSED(inputFileArg);
    VAR_UNUSED(outputFileArg);
    VAR_UNUSED(validateArg);
    cerr << "Error: Generating HTML info is only available if built with Qt support." << endl;
#endif
}

void displayFileInfo(const ArgumentOccurrence &, const Argument &filesArg, const Argument &verboseArg)
{
    CMD_UTILS_START_CONSOLE;
    if(!filesArg.isPresent() || filesArg.values().empty()) {
        cerr << "Error: No files have been specified." << endl;
        return;
    }
    MediaFileInfo fileInfo;
    for(const char *file : filesArg.values()) {
        try {
            // parse tags
            fileInfo.setPath(file);
            fileInfo.open(true);
            fileInfo.parseContainerFormat();
            fileInfo.parseTracks();
            fileInfo.parseAttachments();
            fileInfo.parseChapters();
            cout << "Technical information for \"" << file << "\":" << endl;
            cout << "  Container format: " << fileInfo.containerFormatName() << endl;
            {
                if(const auto container = fileInfo.container()) {
                    size_t segmentIndex = 0;
                    for(const auto &title : container->titles()) {
                        if(segmentIndex) {
                            printProperty("Title", title + " (segment " + numberToString(++segmentIndex) + ")");
                        } else {
                            ++segmentIndex;
                            printProperty("Title", title);
                        }
                    }
                    printProperty("Document type", container->documentType());
                    printProperty("Read version", container->readVersion());
                    printProperty("Version", container->version());
                    printProperty("Document read version", container->doctypeReadVersion());
                    printProperty("Document version", container->doctypeVersion());
                    printProperty("Duration", container->duration());
                    printProperty("Creation time", container->creationTime());
                    printProperty("Modification time", container->modificationTime());
                    printProperty("Tag position", container->determineTagPosition());
                    printProperty("Index position", container->determineIndexPosition());
                }
                if(fileInfo.paddingSize()) {
                    printProperty("Padding", dataSizeToString(fileInfo.paddingSize()));
                }
            }
            { // tracks
                const auto tracks = fileInfo.tracks();
                if(!tracks.empty()) {
                    cout << "  Tracks:" << endl;
                    for(const auto *track : tracks) {
                        printProperty("ID", track->id(), nullptr, true);
                        printProperty("Name", track->name());
                        printProperty("Type", track->mediaTypeName());
                        const char *fmtName = track->formatName(), *fmtAbbr = track->formatAbbreviation();
                        printProperty("Format", fmtName);
                        if(strcmp(fmtName, fmtAbbr)) {
                            printProperty("Abbreviation", fmtAbbr);
                        }
                        printProperty("Extensions", track->format().extensionName());
                        printProperty("Raw format ID", track->formatId());
                        if(track->size()) {
                            printProperty("Size", dataSizeToString(track->size(), true));
                        }
                        printProperty("Duration", track->duration());
                        printProperty("FPS", track->fps());
                        if(track->channelConfigString()) {
                            printProperty("Channel config", track->channelConfigString());
                        } else {
                            printProperty("Channel count", track->channelCount());
                        }
                        if(track->extensionChannelConfigString()) {
                            printProperty("Extension channel config", track->extensionChannelConfigString());
                        }
                        printProperty("Bitrate", track->bitrate(), "kbit/s");
                        printProperty("Bits per sample", track->bitsPerSample());
                        printProperty("Sampling frequency", track->samplingFrequency(), "Hz");
                        printProperty("Extension sampling frequency", track->extensionSamplingFrequency(), "Hz");
                        printProperty("Sample count", track->sampleCount());
                        printProperty("Creation time", track->creationTime());
                        printProperty("Modification time", track->modificationTime());
                        cout << endl;
                    }
                } else {
                    cout << " File has no (supported) tracks." << endl;
                }
            }
            { // attachments
                const auto attachments = fileInfo.attachments();
                if(!attachments.empty()) {
                    cout << "Attachments:" << endl;
                    for(const auto *attachment : attachments) {
                        printProperty("ID", attachment->id());
                        printProperty("Name", attachment->name());
                        printProperty("MIME-type", attachment->mimeType());
                        printProperty("Description", attachment->description());
                        if(attachment->data()) {
                            printProperty("Size", dataSizeToString(attachment->data()->size(), true));
                        }
                        cout << endl;
                    }
                }
            }
            { // chapters
                const auto chapters = fileInfo.chapters();
                if(!chapters.empty()) {
                    cout << "Chapters:" << endl;
                    for(const auto *chapter : chapters) {
                        printProperty("ID", chapter->id());
                        if(!chapter->names().empty()) {
                            printProperty("Name", static_cast<string>(chapter->names().front()));
                        }
                        if(!chapter->startTime().isNegative()) {
                            printProperty("Start time", chapter->startTime().toString());
                        }
                        if(!chapter->endTime().isNegative()) {
                            printProperty("End time", chapter->endTime().toString());
                        }
                        cout << endl;
                    }
                }
            }
        } catch(const ApplicationUtilities::Failure &) {
            cerr << "Error: A parsing failure occured when reading the file \"" << file << "\"." << endl;
        } catch(...) {
            ::IoUtilities::catchIoFailure();
            cerr << "Error: An IO failure occured when reading the file \"" << file << "\"." << endl;
        }
        printNotifications(fileInfo, "Parsing notifications:", verboseArg.isPresent());
        cout << endl;
    }
}

void displayTagInfo(const Argument &fieldsArg, const Argument &filesArg, const Argument &verboseArg)
{
    CMD_UTILS_START_CONSOLE;
    if(!filesArg.isPresent() || filesArg.values().empty()) {
        cerr << "Error: No files have been specified." << endl;
        return;
    }
    const auto fields = parseFieldDenotations(fieldsArg, true);
    MediaFileInfo fileInfo;
    for(const char *file : filesArg.values()) {
        try {
            // parse tags
            fileInfo.setPath(file);
            fileInfo.open(true);
            fileInfo.parseContainerFormat();
            fileInfo.parseTags();
            cout << file << endl;
            cout << "Tag information for \"" << file << "\":" << endl;
            const auto tags = fileInfo.tags();
            if(!tags.empty()) {
                // iterate through all tags
                for(const auto *tag : tags) {
                    // determine tag type
                    const TagType tagType = tag->type();
                    // write tag name and target, eg. MP4/iTunes tag
                    cout << tag->typeName();
                    if(tagType == TagType::Id3v2Tag) {
                        // version only interesting for ID3v2 tags?
                        cout << " (version " << tag->version() << ')';
                    }
                    if(tagType == TagType::MatroskaTag || !tag->target().isEmpty()) {
                        cout << " targeting \"" << tag->targetString() << '\"';
                    }
                    cout << endl;
                    // iterate through fields specified by the user
                    if(fields.empty()) {
                        for(auto field = firstKnownField; field != KnownField::Invalid; field = nextKnownField(field)) {
                            const auto &values = tag->values(field);
                            if(!values.empty()) {
                                const char *fieldName = KnownFieldModel::fieldName(field);
                                const auto fieldNameLen = strlen(fieldName);
                                for(const auto &value : values) {
                                    // write field name
                                    cout << ' ' << fieldName;
                                    // write padding
                                    for(auto i = fieldNameLen; i < 18; ++i) {
                                        cout << ' ';
                                    }
                                    // write value
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
                                    cout << endl;
                                }
                            }
                        }
                    } else {
                        for(const auto &fieldDenotation : fields) {
                            const FieldScope &denotedScope = fieldDenotation.first;
                            if(denotedScope.tagType == TagType::Unspecified || (denotedScope.tagType | tagType) != TagType::Unspecified) {
                                const auto &values = tag->values(denotedScope.field);
                                const char *fieldName = KnownFieldModel::fieldName(denotedScope.field);
                                const auto fieldNameLen = strlen(fieldName);
                                if(values.empty()) {
                                    // write field name
                                    const char *fieldName = KnownFieldModel::fieldName(denotedScope.field);
                                    cout << ' ' << fieldName;
                                    // write padding
                                    for(auto i = fieldNameLen; i < 18; ++i) {
                                        cout << ' ';
                                    }
                                    cout << "none";
                                } else {
                                    for(const auto &value : values) {
                                        // write field name
                                        const char *fieldName = KnownFieldModel::fieldName(denotedScope.field);
                                        cout << ' ' << fieldName;
                                        // write padding
                                        for(auto i = fieldNameLen; i < 18; ++i) {
                                            cout << ' ';
                                        }
                                        // write value
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
                                        cout << endl;
                                    }
                                }
                            }
                        }
                    }
                }
            } else {
                cout << " File has no (supported) tag information." << endl;
            }
        } catch(const ApplicationUtilities::Failure &) {
            cerr << "Error: A parsing failure occured when reading the file \"" << file << "\"." << endl;
        } catch(...) {
            ::IoUtilities::catchIoFailure();
            cerr << "Error: An IO failure occured when reading the file \"" << file << "\"." << endl;
        }
        printNotifications(fileInfo, "Parsing notifications:", verboseArg.isPresent());
        cout << endl;
    }
}

void setTagInfo(const SetTagInfoArgs &args)
{
    CMD_UTILS_START_CONSOLE;
    if(!args.filesArg.isPresent() || args.filesArg.values().empty()) {
        cerr << "Error: No files have been specified." << endl;
        return;
    }
    if(args.outputFilesArg.isPresent() && args.outputFilesArg.values().size() != args.filesArg.values().size()) {
        cerr << "Error: The number of output files does not match the number of input files." << endl;
        return;
    }
    auto &outputFiles = args.outputFilesArg.isPresent() ? args.outputFilesArg.values() : vector<const char *>();
    auto currentOutputFile = outputFiles.cbegin(), noMoreOutputFiles = outputFiles.cend();
    auto fields = parseFieldDenotations(args.valuesArg, false);
    if(fields.empty()
            && (!args.removeTargetArg.isPresent() || args.removeTargetArg.values().empty())
            && (!args.addAttachmentArg.isPresent() || args.addAttachmentArg.values().empty())
            && (!args.updateAttachmentArg.isPresent() || args.updateAttachmentArg.values().empty())
            && (!args.removeAttachmentArg.isPresent() || args.removeAttachmentArg.values().empty())
            && (!args.docTitleArg.isPresent() || args.docTitleArg.values().empty())
            && !args.id3v1UsageArg.isPresent()
            && !args.id3v2UsageArg.isPresent()
            && !args.id3v2VersionArg.isPresent()) {
        cerr << "Warning: No fields/attachments have been specified." << endl;
    }
    // determine required targets
    vector<TagTarget> requiredTargets;
    for(const auto &fieldDenotation : fields) {
        const FieldScope &scope = fieldDenotation.first;
        if(find(requiredTargets.cbegin(), requiredTargets.cend(), scope.tagTarget) == requiredTargets.cend()) {
            requiredTargets.push_back(scope.tagTarget);
        }
    }
    // determine targets to remove
    vector<TagTarget> targetsToRemove;
    bool validRemoveTargetsSpecified = false;
    for(size_t i = 0, max = args.removeTargetArg.occurrences(); i != max; ++i) {
        for(const auto &targetDenotation : args.removeTargetArg.values(i)) {
            targetsToRemove.emplace_back();
            if(!strcmp(targetDenotation, ",")) {
                if(validRemoveTargetsSpecified) {
                    targetsToRemove.emplace_back();
                }
            } else if(applyTargetConfiguration(targetsToRemove.back(), targetDenotation)) {
                validRemoveTargetsSpecified = true;
            } else {
                cerr << "Warning: The given target specification \"" << targetDenotation << "\" is invalid and will be ignored." << endl;
            }
        }
    }
    // parse other settings
    uint32 id3v2Version = 3;
    if(args.id3v2VersionArg.isPresent()) {
        try {
            id3v2Version = stringToNumber<uint32>(args.id3v2VersionArg.values().front());
            if(id3v2Version < 1 || id3v2Version > 4) {
                throw ConversionException();
            }
        } catch (const ConversionException &) {
            id3v2Version = 3;
            cerr << "Warning: The specified ID3v2 version \"" << args.id3v2VersionArg.values().front() << "\" is invalid and will be ingored." << endl;
        }
    }
    const TagTextEncoding denotedEncoding = parseEncodingDenotation(args.encodingArg, TagTextEncoding::Utf8);
    const TagUsage id3v1Usage = parseUsageDenotation(args.id3v1UsageArg, TagUsage::KeepExisting);
    const TagUsage id3v2Usage = parseUsageDenotation(args.id3v2UsageArg, TagUsage::Always);
    MediaFileInfo fileInfo;
    fileInfo.setMinPadding(parseUInt64(args.minPaddingArg, 0));
    fileInfo.setMaxPadding(parseUInt64(args.maxPaddingArg, 0));
    fileInfo.setPreferredPadding(parseUInt64(args.prefPaddingArg, 0));
    fileInfo.setTagPosition(parsePositionDenotation(args.tagPosArg, args.tagPosValueArg, ElementPosition::BeforeData));
    fileInfo.setForceTagPosition(args.forceTagPosArg.isPresent());
    fileInfo.setIndexPosition(parsePositionDenotation(args.indexPosArg, args.indexPosValueArg, ElementPosition::BeforeData));
    fileInfo.setForceIndexPosition(args.forceIndexPosArg.isPresent());
    fileInfo.setForceRewrite(args.forceRewriteArg.isPresent());
    // iterate through all specified files
    unsigned int fileIndex = 0;
    static const string context("setting tags");
    NotificationList notifications;
    for(const char *file : args.filesArg.values()) {
        try {
            // parse tags
            cout << "Setting tag information for \"" << file << "\" ..." << endl;
            notifications.clear();
            fileInfo.setPath(file);
            fileInfo.parseContainerFormat();
            fileInfo.parseTags();
            fileInfo.parseTracks();
            vector<Tag *> tags;
            // remove tags with the specified targets
            if(validRemoveTargetsSpecified) {
                fileInfo.tags(tags);
                for(auto *tag : tags) {
                    if(find(targetsToRemove.cbegin(), targetsToRemove.cend(), tag->target()) != targetsToRemove.cend()) {
                        fileInfo.removeTag(tag);
                    }
                }
                tags.clear();
            }
            // create new tags according to settings
            fileInfo.createAppropriateTags(args.treatUnknownFilesAsMp3FilesArg.isPresent(), id3v1Usage, id3v2Usage, args.id3InitOnCreateArg.isPresent(), args.id3TransferOnRemovalArg.isPresent(), args.mergeMultipleSuccessiveTagsArg.isPresent(), !args.id3v2VersionArg.isPresent(), id3v2Version, requiredTargets);
            auto container = fileInfo.container();
            if(args.docTitleArg.isPresent() && !args.docTitleArg.values().empty()) {
                if(container && container->supportsTitle()) {
                    size_t segmentIndex = 0, segmentCount = container->titles().size();
                    for(const auto &newTitle : args.docTitleArg.values()) {
                        if(segmentIndex < segmentCount) {
                            container->setTitle(newTitle, segmentIndex);
                        } else {
                            cerr << "Warning: The specified document title \"" << newTitle << "\" can not be set because the file has not that many segments." << endl;
                        }
                        ++segmentIndex;
                    }
                } else {
                    cerr << "Warning: Setting the document title is not supported for the file." << endl;
                }
            }
            fileInfo.tags(tags);
            if(!tags.empty()) {
                // iterate through all tags
                for(auto *tag : tags) {
                    // clear current values if option is present
                    if(args.removeOtherFieldsArg.isPresent()) {
                        tag->removeAllFields();
                    }
                    // determine required information for deciding whether specified values match the scope of the current tag
                    const auto tagType = tag->type();
                    const bool targetSupported = tag->supportsTarget();
                    const auto tagTarget = tag->target();
                    // iterate through all denoted field values
                    for(auto &fieldDenotation : fields) {
                        const FieldScope &denotedScope = fieldDenotation.first;
                        FieldValues &denotedValues = fieldDenotation.second;
                        // decide whether the scope of the denotation matches of the current tag
                        if((denotedScope.tagType == TagType::Unspecified
                            || (denotedScope.tagType & tagType) != TagType::Unspecified)
                                && (!targetSupported || denotedScope.tagTarget == tagTarget)) {
                            // select the relevant values for the current file index
                            vector<FieldValue *> relevantDenotedValues;
                            unsigned int currentFileIndex = 0;
                            for(FieldValue &denotatedValue : denotedValues) {
                                if(denotatedValue.fileIndex <= fileIndex) {
                                    if(relevantDenotedValues.empty() || (denotatedValue.fileIndex >= currentFileIndex)) {
                                        if(currentFileIndex != denotatedValue.fileIndex) {
                                            currentFileIndex = denotatedValue.fileIndex;
                                            relevantDenotedValues.clear();
                                        }
                                        relevantDenotedValues.push_back(&denotatedValue);
                                    }
                                }
                            }
                            // convert the values to TagValue
                            vector<TagValue> convertedValues;
                            for(FieldValue *relevantDenotedValue : relevantDenotedValues) {
                                // one of the denoted values
                                if(!relevantDenotedValue->value.empty()) {
                                    if(relevantDenotedValue->type == DenotationType::File) {
                                        try {
                                            // assume the file refers to a picture
                                            MediaFileInfo fileInfo(relevantDenotedValue->value);
                                            fileInfo.open(true);
                                            fileInfo.parseContainerFormat();
                                            auto buff = make_unique<char []>(fileInfo.size());
                                            fileInfo.stream().seekg(0);
                                            fileInfo.stream().read(buff.get(), fileInfo.size());
                                            TagValue value(move(buff), fileInfo.size(), TagDataType::Picture);
                                            value.setMimeType(fileInfo.mimeType());
                                            convertedValues.emplace_back(move(value));
                                        } catch(const Media::Failure &) {
                                            fileInfo.addNotification(NotificationType::Critical, "Unable to parse specified cover file.", context);
                                        } catch(...) {
                                            ::IoUtilities::catchIoFailure();
                                            fileInfo.addNotification(NotificationType::Critical, "An IO error occured when parsing the specified cover file.", context);
                                        }
                                    } else {
                                        TagTextEncoding usedEncoding = denotedEncoding;
                                        if(!tag->canEncodingBeUsed(denotedEncoding)) {
                                            usedEncoding = tag->proposedTextEncoding();
                                        }
                                        convertedValues.emplace_back(relevantDenotedValue->value, TagTextEncoding::Utf8, usedEncoding);
                                        if(relevantDenotedValue->type == DenotationType::Increment && tag == tags.back()) {
                                            relevantDenotedValue->value = incremented(relevantDenotedValue->value);
                                        }
                                    }
                                } else {
                                    // if the denoted value is empty, just assign an empty TagValue to remove the field
                                    convertedValues.emplace_back();
                                }
                            }
                            // finally set the values
                            tag->setValues(denotedScope.field, convertedValues);
                        }
                    }
                }
            } else {
                fileInfo.addNotification(NotificationType::Critical, "Can not create appropriate tags for file.", context);
            }
            bool attachmentsModified = false;
            if(args.addAttachmentArg.isPresent() || args.updateAttachmentArg.isPresent() || args.removeAttachmentArg.isPresent() || args.removeExistingAttachmentsArg.isPresent()) {
                static const string context("setting attachments");
                fileInfo.parseAttachments();
                if(fileInfo.attachmentsParsingStatus() == ParsingStatus::Ok) {
                    if(container) {
                        // ignore all existing attachments if argument is specified
                        if(args.removeExistingAttachmentsArg.isPresent()) {
                            for(size_t i = 0, count = container->attachmentCount(); i < count; ++i) {
                                container->attachment(i)->setIgnored(false);
                            }
                            attachmentsModified = true;
                        }
                        // add/update/remove attachments
                        AttachmentInfo currentInfo;
                        currentInfo.action = AttachmentAction::Add;
                        for(size_t i = 0, occurrences = args.addAttachmentArg.occurrences(); i != occurrences; ++i) {
                            for(const char *value : args.addAttachmentArg.values(i)) {
                                currentInfo.parseDenotation(value);
                            }
                            attachmentsModified |= currentInfo.next(container);
                        }
                        currentInfo.action = AttachmentAction::Update;
                        for(size_t i = 0, occurrences = args.updateAttachmentArg.occurrences(); i != occurrences; ++i) {
                            for(const char *value : args.updateAttachmentArg.values(i)) {
                                currentInfo.parseDenotation(value);
                            }
                            attachmentsModified |= currentInfo.next(container);
                        }
                        currentInfo.action = AttachmentAction::Remove;
                        for(size_t i = 0, occurrences = args.removeAttachmentArg.occurrences(); i != occurrences; ++i) {
                            for(const char *value : args.removeAttachmentArg.values(i)) {
                                currentInfo.parseDenotation(value);
                            }
                            attachmentsModified |= currentInfo.next(container);
                        }
                    } else {
                        fileInfo.addNotification(NotificationType::Critical, "Unable to assign attachments because the container object has not been initialized.", context);
                    }
                } else {
                    // notification will be added by the file info automatically
                }
            }
            try {
                // save parsing notifications because notifications of sub objects like tags, tracks, ... will be gone after applying changes
                fileInfo.setSaveFilePath(currentOutputFile != noMoreOutputFiles ? string(*currentOutputFile) : string());
                fileInfo.gatherRelatedNotifications(notifications);
                fileInfo.invalidateNotifications();
                fileInfo.applyChanges();
                fileInfo.gatherRelatedNotifications(notifications);
                cout << "Changes have been applied." << endl;
            } catch(const ApplicationUtilities::Failure &) {
                cerr << "Error: Failed to apply changes." << endl;
            }
        } catch(const ApplicationUtilities::Failure &) {
            cerr << "Error: A parsing failure occured when reading/writing the file \"" << file << "\"." << endl;
        } catch(...) {
            ::IoUtilities::catchIoFailure();
            cerr << "Error: An IO failure occured when reading/writing the file \"" << file << "\"." << endl;
        }
        printNotifications(notifications, "Notifications:", args.verboseArg.isPresent());

        ++fileIndex;
        if(currentOutputFile != noMoreOutputFiles) {
            ++currentOutputFile;
        }
    }
}

void extractField(const Argument &fieldArg, const Argument &attachmentArg, const Argument &inputFilesArg, const Argument &outputFileArg, const Argument &verboseArg)
{
    CMD_UTILS_START_CONSOLE;

    // parse specified field and attachment
    const auto fieldDenotations = parseFieldDenotations(fieldArg, true);
    AttachmentInfo attachmentInfo;
    if(attachmentArg.isPresent()) {
        attachmentInfo.parseDenotation(attachmentArg.values().front());
    }
    if(((fieldDenotations.size() != 1) || (!attachmentInfo.hasId && !attachmentInfo.name))
            && ((fieldDenotations.size() == 1) && (attachmentInfo.hasId || attachmentInfo.name))) {
        cerr << "Error: Excactly one field or attachment needs to be specified." << endl;
        return;
    }
    if(!inputFilesArg.isPresent() || inputFilesArg.values().empty()) {
        cerr << "Error: No files have been specified." << endl;
        return;
    }

    MediaFileInfo inputFileInfo;
    for(const char *file : inputFilesArg.values()) {
        try {
            // parse tags
            inputFileInfo.setPath(file);
            inputFileInfo.open(true);

            if(!fieldDenotations.empty()) {
                // extract tag field
                (outputFileArg.isPresent() ? cout : cerr) << "Extracting field " << fieldArg.values().front() << " of \"" << file << "\" ..." << endl;
                inputFileInfo.parseContainerFormat();
                inputFileInfo.parseTags();
                auto tags = inputFileInfo.tags();
                vector<pair<const TagValue *, string> > values;
                // iterate through all tags
                for(const Tag *tag : tags) {
                    for(const auto &fieldDenotation : fieldDenotations) {
                        const auto &value = tag->value(fieldDenotation.first.field);
                        if(!value.isEmpty()) {
                            values.emplace_back(&value, joinStrings({tag->typeName(), numberToString(values.size())}, "-", true));
                        }
                    }
                }
                if(values.empty()) {
                    cerr << " None of the specified files has a (supported) " << fieldArg.values().front() << " field." << endl;
                } else if(outputFileArg.isPresent()) {
                    string outputFilePathWithoutExtension, outputFileExtension;
                    if(values.size() > 1) {
                        outputFilePathWithoutExtension = BasicFileInfo::pathWithoutExtension(outputFileArg.values().front());
                        outputFileExtension = BasicFileInfo::extension(outputFileArg.values().front());
                    }
                    for(const auto &value : values) {
                        NativeFileStream outputFileStream;
                        outputFileStream.exceptions(ios_base::failbit | ios_base::badbit);
                        auto path = values.size() > 1 ? joinStrings({outputFilePathWithoutExtension, "-", value.second, outputFileExtension}) : outputFileArg.values().front();
                        try {
                            outputFileStream.open(path, ios_base::out | ios_base::binary);
                            outputFileStream.write(value.first->dataPointer(), value.first->dataSize());
                            outputFileStream.flush();
                            cout << "Value has been saved to \"" << path << "\"." << endl;
                        } catch(...) {
                            ::IoUtilities::catchIoFailure();
                            cerr << "Error: An IO error occured when writing the file \"" << path << "\"." << endl;
                        }
                    }
                } else {
                    // write data to stdout if no output file has been specified
                    for(const auto &value : values) {
                        cout.write(value.first->dataPointer(), value.first->dataSize());
                    }
                }
            } else {
                // extract attachment
                auto &logStream = (outputFileArg.isPresent() ? cout : cerr);
                logStream << "Extracting attachment with ";
                if(attachmentInfo.hasId) {
                    logStream << "ID " << attachmentInfo.id;
                } else {
                    logStream << "name \"" << attachmentInfo.name << '\"';
                }
                logStream << " of \"" << file << "\" ..." << endl;

                inputFileInfo.parseContainerFormat();
                inputFileInfo.parseAttachments();
                vector<pair<const AbstractAttachment *, string> > attachments;
                // iterate through all attachments
                for(const AbstractAttachment *attachment : inputFileInfo.attachments()) {
                    if((attachmentInfo.hasId && attachment->id() == attachmentInfo.id) || (attachment->name() == attachmentInfo.name)) {
                        attachments.emplace_back(attachment, joinStrings({attachment->name(), numberToString(attachments.size())}, "-", true));
                    }
                }
                if(attachments.empty()) {
                    cerr << " None of the specified files has a (supported) attachment with the specified ID/name." << endl;
                } else if(outputFileArg.isPresent()) {
                    string outputFilePathWithoutExtension, outputFileExtension;
                    if(attachments.size() > 1) {
                        outputFilePathWithoutExtension = BasicFileInfo::pathWithoutExtension(outputFileArg.values().front());
                        outputFileExtension = BasicFileInfo::extension(outputFileArg.values().front());
                    }
                    for(const auto &attachment : attachments) {
                        NativeFileStream outputFileStream;
                        outputFileStream.exceptions(ios_base::failbit | ios_base::badbit);
                        auto path = attachments.size() > 1 ? joinStrings({outputFilePathWithoutExtension, "-", attachment.second, outputFileExtension}) : outputFileArg.values().front();
                        try {
                            outputFileStream.open(path, ios_base::out | ios_base::binary);
                            attachment.first->data()->copyTo(outputFileStream);
                            outputFileStream.flush();
                            cout << "Value has been saved to \"" << path << "\"." << endl;
                        } catch(...) {
                            ::IoUtilities::catchIoFailure();
                            cerr << "Error: An IO error occured when writing the file \"" << path << "\"." << endl;
                        }
                    }
                } else {
                    for(const auto &attachment : attachments) {
                        attachment.first->data()->copyTo(cout);
                    }
                }
            }

        } catch(const ApplicationUtilities::Failure &) {
            cerr << "Error: A parsing failure occured when reading the file \"" << file << "\"." << endl;
        } catch(...) {
            ::IoUtilities::catchIoFailure();
            cerr << "Error: An IO failure occured when reading the file \"" << file << "\"." << endl;
        }
        printNotifications(inputFileInfo, "Parsing notifications:", verboseArg.isPresent());
    }
}

}
