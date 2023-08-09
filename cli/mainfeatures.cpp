#include "./mainfeatures.h"
#include "./attachmentinfo.h"
#include "./helper.h"
#ifdef TAGEDITOR_JSON_EXPORT
#include "./json.h"
#endif

#include "../application/knownfieldmodel.h"

// includes for JavaScript support of set operation
#ifdef TAGEDITOR_USE_JSENGINE
#include "./scriptapi.h"
#endif

// includes for generating HTML info
#if defined(TAGEDITOR_GUI_QTWIDGETS) || defined(TAGEDITOR_GUI_QTQUICK)
#include "../misc/htmlinfo.h"
#include "../misc/utility.h"
#endif

#include "resources/config.h"

#include <tagparser/abstractattachment.h>
#include <tagparser/abstractchapter.h>
#include <tagparser/abstracttrack.h>
#include <tagparser/backuphelper.h>
#include <tagparser/diagnostics.h>
#include <tagparser/id3/id3v2tag.h>
#include <tagparser/localehelper.h>
#include <tagparser/mediafileinfo.h>
#include <tagparser/progressfeedback.h>
#include <tagparser/tag.h>
#include <tagparser/tagvalue.h>
#include <tagparser/vorbis/vorbiscomment.h>

#ifdef TAGEDITOR_JSON_EXPORT
#include <reflective_rapidjson/json/reflector.h>
#endif

#include <c++utilities/application/commandlineutils.h>
#include <c++utilities/conversion/conversionexception.h>
#include <c++utilities/conversion/stringbuilder.h>
#include <c++utilities/conversion/stringconversion.h>
#include <c++utilities/io/ansiescapecodes.h>
#include <c++utilities/io/nativefilestream.h>
#include <c++utilities/io/path.h>
#include <c++utilities/misc/parseerror.h>

// includes for JavaScript support of set operation
#ifdef TAGEDITOR_USE_JSENGINE
#include <QCoreApplication>
#include <QFile>
#include <QJSValue>
#include <QQmlEngine>
#include <QTextStream>
#endif

// includes for generating HTML info
#if defined(TAGEDITOR_GUI_QTWIDGETS) || defined(TAGEDITOR_GUI_QTQUICK)
#include <QDir>
#include <qtutilities/misc/conversion.h>
#endif

#ifdef TAGEDITOR_JSON_EXPORT
#include <rapidjson/ostreamwrapper.h>
#include <rapidjson/prettywriter.h>
#include <rapidjson/writer.h>
#endif

#include <algorithm>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <iomanip>
#include <iostream>
#include <memory>
#include <optional>
#include <string_view>

using namespace std;
using namespace CppUtilities;
using namespace CppUtilities::EscapeCodes;
using namespace Settings;
using namespace TagParser;
#if defined(TAGEDITOR_GUI_QTWIDGETS) || defined(TAGEDITOR_GUI_QTQUICK)
using namespace QtUtilities;
#endif

namespace Cli {

#define FIELD_NAMES                                                                                                                                  \
    "title album artist genre recorddate comment bpm bps lyricist track disk part totalparts encoder\n"                                              \
    "      releasedate performers duration language encodersettings lyrics synchronizedlyrics grouping\n"                                            \
    "      recordlabel cover composer rating description vendor albumartist\n"                                                                       \
    "      subtitle leadperformer arranger conductor director assistantdirector\n"                                                                   \
    "      directorofphotography soundengineer artdirector productiondesigner choregrapher\n"                                                        \
    "      costumedesigner actor character writtenby screenplayby editedby producer\n"                                                               \
    "      coproducer exectiveproducer distributedby masteredby encodedby mixedby\n"                                                                 \
    "      remixedby productionstudio thanksto publisher mood originalmediatype\n"                                                                   \
    "      contenttype subject keywords summary synopsis initialkey period lawrating\n"                                                              \
    "      encodingdate taggingdate originalreleasedate digitalizationdate writingdate\n"                                                            \
    "      purchasingdate recordinglocation compositionlocation composernationality\n"                                                               \
    "      playcounter measure tuning isrc mcdi isbn barcode catalognumber labelcode\n"                                                              \
    "      lccn imdb tmdb tvdb purchaseitem purchaseinfo purchaseowner purchaseprice\n"                                                              \
    "      purchasecurrency copyright productioncopyright license termsofuse"

#define TRACK_ATTRIBUTE_NAMES "name tracknumber enabled=yes enabled=no forced=yes forced=no default=yes default=no"

#define TAG_MODIFIER "tag=id3v1 tag=id3v2 tag=id3 tag=itunes tag=vorbis tag=matroska tag=all"

#define TRACK_MODIFIER "track-id= track=all"

#define TARGET_MODIFIER                                                                                                                              \
    "target-level target-levelname target-tracks target-tracks\n"                                                                                    \
    "      target-chapters target-editions target-attachments target-reset"

const char *const fieldNames = FIELD_NAMES;
const char *const fieldNamesForSet = TAG_MODIFIER " " FIELD_NAMES " " TRACK_MODIFIER " " TRACK_ATTRIBUTE_NAMES " " TARGET_MODIFIER;
int exitCode = EXIT_SUCCESS;

void printFieldNames(const ArgumentOccurrence &)
{
    CMD_UTILS_START_CONSOLE;

    cout << "Field and track attribute names allow referring to a field or track attribute in a format-independent way.\n"
            " - Field names:\n      " FIELD_NAMES "\n"
            " - Track attribute names: " TRACK_ATTRIBUTE_NAMES "\n\n"
            "Modifier specify to which tags and tracks the subsequent values should be applied.\n"
            " - Tag modifier: " TAG_MODIFIER "\n"
            " - Track modifier: track=id1,id2,id3,... track=all\n"
            " - Target modifier:\n      " TARGET_MODIFIER "\n"
            "ID3v2 cover types:\n"
         << joinStrings<std::remove_reference_t<decltype(id3v2CoverTypeNames())>, std::string>(id3v2CoverTypeNames(), "\n"sv, false, " - "sv, ""sv)
         << '\n'
         << flush;
}

void generateFileInfo(const ArgumentOccurrence &, const Argument &inputFileArg, const Argument &outputFileArg, const Argument &validateArg)
{
    CMD_UTILS_START_CONSOLE;

#if defined(TAGEDITOR_GUI_QTWIDGETS) || defined(TAGEDITOR_GUI_QTQUICK)
    try {
        // parse tags
        auto inputFileInfo = MediaFileInfo(std::string(inputFileArg.values().front()));
        inputFileInfo.setForceFullParse(validateArg.isPresent());
        inputFileInfo.open(true);
        auto diag = Diagnostics();
        auto progress = AbortableProgressFeedback(); // FIXME: actually use the progress object
        inputFileInfo.parseEverything(diag, progress);

        // generate and save info
        auto diagReparsing = Diagnostics();
        (outputFileArg.isPresent() ? cout : cerr) << "Saving file info for \"" << inputFileArg.values().front() << "\" ..." << endl;
        if (!outputFileArg.isPresent()) {
            cout << HtmlInfo::generateInfo(inputFileInfo, diag, diagReparsing).data() << endl;
            return;
        }
        auto file = QFile(fromNativeFileName(outputFileArg.values().front()));
        if (file.open(QFile::WriteOnly) && file.write(HtmlInfo::generateInfo(inputFileInfo, diag, diagReparsing)) && file.flush()) {
            cout << "File information has been saved to \"" << outputFileArg.values().front() << "\"." << endl;
        } else {
            const auto errorMessage = file.errorString().toUtf8();
            cerr << Phrases::Error << "An IO error occurred when writing the file \"" << outputFileArg.values().front()
                 << "\": " << std::string_view(errorMessage.data(), static_cast<std::string_view::size_type>(errorMessage.size()))
                 << Phrases::EndFlush;
            exitCode = EXIT_IO_FAILURE;
        }
    } catch (const TagParser::Failure &) {
        cerr << Phrases::Error << "A parsing failure occurred when reading the file \"" << inputFileArg.values().front() << "\"."
             << Phrases::EndFlush;
        exitCode = EXIT_PARSING_FAILURE;
    } catch (const std::ios_base::failure &e) {
        cerr << Phrases::Error << "An IO error occurred when reading the file \"" << inputFileArg.values().front() << "\": " << e.what()
             << Phrases::EndFlush;
        exitCode = EXIT_IO_FAILURE;
    }
#else
    CPP_UTILITIES_UNUSED(inputFileArg);
    CPP_UTILITIES_UNUSED(outputFileArg);
    CPP_UTILITIES_UNUSED(validateArg);
    cerr << Phrases::Error << "Generating HTML info is only available if built with Qt support." << Phrases::EndFlush;
    exitCode = EXIT_FAILURE;
#endif
}

void displayFileInfo(
    const ArgumentOccurrence &, const Argument &filesArg, const Argument &verboseArg, const Argument &pedanticArg, const Argument &validateArg)
{
    CMD_UTILS_START_CONSOLE;

    // check whether files have been specified
    if (!filesArg.isPresent() || filesArg.values().empty()) {
        cerr << Phrases::Error << "No files have been specified." << Phrases::End;
        std::exit(EXIT_FAILURE);
    }

    MediaFileInfo fileInfo;
    for (const char *file : filesArg.values()) {
        Diagnostics diag;
        AbortableProgressFeedback progress; // FIXME: actually use the progress object
        try {
            // parse tags
            if (validateArg.isPresent()) {
                fileInfo.setForceFullParse(true);
            }
            fileInfo.setPath(std::string(file));
            fileInfo.open(true);
            fileInfo.parseContainerFormat(diag, progress);
            fileInfo.parseEverything(diag, progress);

            // print general/container-related info
            cout << "Technical information for \"" << file << "\":\n";
            cout << " - " << TextAttribute::Bold << "Container format: " << fileInfo.containerFormatName() << Phrases::End;
            printProperty("Size", dataSizeToString(fileInfo.size()));
            if (const auto mimeType = fileInfo.mimeType(); !mimeType.empty()) {
                printProperty("Mime-type", mimeType);
            }
            const auto duration = fileInfo.duration();
            if (!duration.isNull()) {
                printProperty("Duration", duration);
                printProperty("Overall avg. bitrate", bitrateToString(fileInfo.overallAverageBitrate()));
            }
            if (const auto *const container = fileInfo.container()) {
                size_t segmentIndex = 0;
                for (const auto &title : container->titles()) {
                    if (segmentIndex) {
                        printProperty("Title", title % " (segment " % ++segmentIndex + ")");
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
                printProperty("Creation time", container->creationTime());
                printProperty("Modification time", container->modificationTime());
                printProperty("Tag position", container->determineTagPosition(diag));
                printProperty("Index position", container->determineIndexPosition(diag));
                if (const auto &muxingApps = container->muxingApplications(); !muxingApps.empty()) {
                    printProperty("Muxing application", joinStrings(muxingApps, ", "));
                }
                if (const auto &writingApps = container->writingApplications(); !writingApps.empty()) {
                    printProperty("Writing application", joinStrings(writingApps, ", "));
                }
            }
            if (fileInfo.paddingSize()) {
                printProperty("Padding", dataSizeToString(fileInfo.paddingSize()));
            }

            // print tracks
            const auto tracks = fileInfo.tracks();
            if (!tracks.empty()) {
                cout << " - " << TextAttribute::Bold << "Tracks: " << fileInfo.technicalSummary() << Phrases::End;
                for (const auto *track : tracks) {
                    printProperty("ID", track->id(), nullptr, true);
                    printProperty("Name", track->name());
                    printProperty("Type", track->mediaTypeName());
                    if (const auto &language = track->locale().fullOrSomeAbbreviatedName(); !language.empty()) {
                        printProperty("Language", language);
                    }
                    const auto fmtName = track->formatName(), fmtAbbr = track->formatAbbreviation();
                    printProperty("Format", fmtName);
                    if (fmtName != fmtAbbr) {
                        printProperty("Abbreviation", fmtAbbr);
                    }
                    printProperty("Extensions", track->format().extensionName());
                    printProperty("Raw format ID", track->formatId());
                    if (track->size()) {
                        printProperty("Size", dataSizeToString(track->size(), true));
                    }
                    printProperty("Duration", track->duration());
                    printProperty("FPS", track->fps());
                    if (!track->pixelSize().isNull()) {
                        printProperty("Pixel size", track->pixelSize().toString());
                    }
                    if (!track->displaySize().isNull()) {
                        printProperty("Display size", track->displaySize().toString());
                    }
                    if (track->pixelAspectRatio().isValid()) {
                        printProperty("Pixel Aspect Ratio", track->pixelAspectRatio().toString());
                    }
                    if (const auto cc = track->channelConfigString(); !cc.empty()) {
                        printProperty("Channel config", cc);
                    } else {
                        printProperty("Channel count", track->channelCount());
                    }
                    if (const auto ecc = track->extensionChannelConfigString(); !ecc.empty()) {
                        printProperty("Extension channel config", ecc);
                    }
                    if (track->bitrate() > 0.0) {
                        printProperty("Bitrate", bitrateToString(track->bitrate()));
                    }
                    printProperty("Bits per sample", track->bitsPerSample());
                    printProperty("Sampling frequency", track->samplingFrequency(), "Hz");
                    printProperty("Extension sampling frequency", track->extensionSamplingFrequency(), "Hz");
                    printProperty(track->mediaType() == MediaType::Video ? "Frame count" : "Sample count", track->sampleCount());
                    printProperty("Creation time", track->creationTime());
                    printProperty("Modification time", track->modificationTime());
                    vector<string> labels;
                    labels.reserve(7);
                    if (track->isInterlaced()) {
                        labels.emplace_back("interlaced");
                    }
                    if (!track->isEnabled()) {
                        labels.emplace_back("disabled");
                    }
                    if (track->isDefault()) {
                        labels.emplace_back("default");
                    }
                    if (track->isForced()) {
                        labels.emplace_back("forced");
                    }
                    if (track->hasLacing()) {
                        labels.emplace_back("has lacing");
                    }
                    if (track->isEncrypted()) {
                        labels.emplace_back("encrypted");
                    }
                    if (!labels.empty()) {
                        printProperty("Labeled as", joinStrings(labels, ", "));
                    }
                    cout << '\n';
                }
            } else {
                cout << " - File has no (supported) tracks.\n";
            }

            // print attachments
            const auto attachments = fileInfo.attachments();
            if (!attachments.empty()) {
                cout << " - " << TextAttribute::Bold << "Attachments:" << TextAttribute::Reset << '\n';
                for (const auto *attachment : attachments) {
                    printProperty("ID", attachment->id());
                    printProperty("Name", attachment->name());
                    printProperty("MIME-type", attachment->mimeType());
                    printProperty("Description", attachment->description());
                    if (attachment->data()) {
                        printProperty("Size", dataSizeToString(static_cast<std::uint64_t>(attachment->data()->size()), true));
                    }
                    cout << '\n';
                }
            }

            // print chapters
            const auto chapters = fileInfo.chapters();
            if (!chapters.empty()) {
                cout << " - " << TextAttribute::Bold << "Chapters:" << TextAttribute::Reset << '\n';
                for (const auto *chapter : chapters) {
                    printProperty("ID", chapter->id());
                    if (!chapter->names().empty()) {
                        printProperty("Name", static_cast<string>(chapter->names().front()));
                    }
                    if (!chapter->startTime().isNegative()) {
                        printProperty("Start time", chapter->startTime().toString());
                    }
                    if (!chapter->endTime().isNegative()) {
                        printProperty("End time", chapter->endTime().toString());
                    }
                    cout << '\n';
                }
            }

        } catch (const TagParser::Failure &) {
            cerr << Phrases::Error << "A parsing failure occurred when reading the file \"" << file << "\"." << Phrases::EndFlush;
            exitCode = EXIT_PARSING_FAILURE;
        } catch (const std::ios_base::failure &) {
            cerr << Phrases::Error << "An IO error occurred when reading the file \"" << file << "\"" << Phrases::EndFlush;
            exitCode = EXIT_IO_FAILURE;
        }

        printDiagMessages(diag, "Diagnostic messages:", verboseArg.isPresent(), &pedanticArg);
        cout << endl;
    }
}

void displayTagInfo(
    const Argument &fieldsArg, const Argument &showUnsupportedArg, const Argument &filesArg, const Argument &verboseArg, const Argument &pedanticArg)
{
    CMD_UTILS_START_CONSOLE;

    // check whether files have been specified
    if (!filesArg.isPresent() || filesArg.values().empty()) {
        std::cerr << Phrases::Error << "No files have been specified." << Phrases::End;
        std::exit(EXIT_FAILURE);
    }

    // parse specified fields
    const auto fields = parseFieldDenotations(fieldsArg, true);

    MediaFileInfo fileInfo;
    for (const char *file : filesArg.values()) {
        Diagnostics diag;
        AbortableProgressFeedback progress; // FIXME: actually use the progress object
        try {
            // parse tags
            fileInfo.setPath(std::string(file));
            fileInfo.open(true);
            fileInfo.parseContainerFormat(diag, progress);
            fileInfo.parseTags(diag, progress);
            cout << "Tag information for \"" << file << "\":\n";
            const auto tags = fileInfo.tags();
            if (tags.empty()) {
                cout << " - File has no (supported) tag information.\n";
                continue;
            }
            // iterate through all tags
            for (const auto *tag : tags) {
                // determine tag type
                const TagType tagType = tag->type();
                // write tag name and target, eg. MP4/iTunes tag
                cout << " - " << TextAttribute::Bold << tagName(tag) << TextAttribute::Reset << '\n';
                // iterate through fields specified by the user
                if (fields.empty()) {
                    for (auto field = firstKnownField; field != KnownField::Invalid; field = nextKnownField(field)) {
                        printField(FieldScope(field), tag, tagType, true);
                    }
                    if (showUnsupportedArg.isPresent()) {
                        printNativeFields(tag);
                    }
                } else {
                    for (const auto &fieldDenotation : fields) {
                        const FieldScope &denotedScope = fieldDenotation.first;
                        if (denotedScope.tagType == TagType::Unspecified || (denotedScope.tagType | tagType) != TagType::Unspecified) {
                            printField(denotedScope, tag, tagType, false);
                        }
                    }
                }
            }
        } catch (const TagParser::Failure &) {
            cerr << Phrases::Error << "A parsing failure occurred when reading the file \"" << file << "\"." << Phrases::EndFlush;
            exitCode = EXIT_PARSING_FAILURE;
        } catch (const std::ios_base::failure &) {
            cerr << Phrases::Error << "An IO error occurred when reading the file \"" << file << "\"." << Phrases::EndFlush;
            exitCode = EXIT_IO_FAILURE;
        }
        printDiagMessages(diag, "Diagnostic messages:", verboseArg.isPresent(), &pedanticArg);
        cout << endl;
    }
}

struct Id3v2Cover {
    explicit Id3v2Cover(TagValue &&value, CoverType type, std::optional<std::string_view> description)
        : value(std::move(value))
        , type(type)
        , description(description)
    {
    }
    TagValue value;
    CoverType type;
    std::optional<std::string_view> description;
};

template <class TagType> static void setId3v2CoverValues(TagType *tag, std::vector<Id3v2Cover> &&values)
{
    auto &fields = tag->fields();
    const auto id = tag->fieldId(KnownField::Cover);
    const auto range = fields.equal_range(id);
    const auto first = range.first;

    for (auto &[tagValue, coverType, description] : values) {
        // check whether there is already a tag value with the current type and description
        auto pair = std::find_if(first, range.second, std::bind(&fieldPredicate<TagType>, coverType, description, placeholders::_1));
        if (pair != range.second) {
            // there is already a tag value with the current type and description
            // -> update this value
            pair->second.setValue(tagValue);
            // check whether there are more values with the current type and description
            while ((pair = std::find_if(++pair, range.second, std::bind(&fieldPredicate<TagType>, coverType, description, placeholders::_1)))
                != range.second) {
                // -> remove these values as we only support one value of a type/description in the same tag
                pair->second.setValue(TagValue());
            }
        } else if (!tagValue.isEmpty()) {
            using FieldType = typename TagType::FieldType;
            auto newField = FieldType(id, tagValue);
            newField.setTypeInfo(static_cast<typename FieldType::TypeInfoType>(coverType));
            fields.insert(std::pair(id, std::move(newField)));
        }
    }
}

#ifdef TAGEDITOR_USE_JSENGINE
class JavaScriptProcessor {
public:
    explicit JavaScriptProcessor(const SetTagInfoArgs &args);
    JavaScriptProcessor(const JavaScriptProcessor &) = delete;

    QJSValue callMain(MediaFileInfo &mediaFileInfo, Diagnostics &diag);

private:
    static void addWarnings(Diagnostics &diag, const std::string &context, const QList<QQmlError> &warnings);

    int argc;
    QCoreApplication app;
    Diagnostics diag;
    QQmlEngine engine; // not using QJSEngine as otherwise XMLHttpRequest is not available
    QJSValue module, main;
    UtilityObject *utility;
};

/*!
 * \brief Initializes JavaScript processing for the specified \a args.
 * \remarks
 * - Comes with its own QCoreApplication. Only for use within the CLI parts of the app!
 * - Exits the app on fatal errors.
 * - Logs status/problems directly in accordance with other parts of the CLI.
 */
JavaScriptProcessor::JavaScriptProcessor(const SetTagInfoArgs &args)
    : argc(0)
    , app(argc, nullptr)
    , utility(new UtilityObject(&engine))
{
    // print status message
    const auto jsPath = args.jsArg.firstValue();
    if (!jsPath) {
        return;
    }
    if (!args.quietArg.isPresent()) {
        std::cout << TextAttribute::Bold << "Loading JavaScript file \"" << jsPath << "\" ..." << Phrases::EndFlush;
    }

    // print warnings later via the usual helper function for consistent formatting
    engine.setOutputWarningsToStandardError(false);
    QObject::connect(&engine, &QQmlEngine::warnings, &engine,
        [this, context = std::string("loading JavaScript")](const auto &warnings) { addWarnings(diag, context, warnings); });

    // assign utility object and load specified JavaScript file as module
    engine.globalObject().setProperty(QStringLiteral("utility"), engine.newQObject(utility));
    module = engine.importModule(QString::fromUtf8(jsPath));
    if (module.isError()) {
        std::cerr << Phrases::Error << "Unable to load the specified JavaScript file \"" << jsPath << "\":" << Phrases::End;
        std::cerr << "Uncaught exception at line " << module.property(QStringLiteral("lineNumber")).toInt() << ':' << ' '
                  << module.toString().toStdString() << '\n';
        std::exit(EXIT_FAILURE);
    }
    main = module.property(QStringLiteral("main"));
    if (!main.isCallable()) {
        std::cerr << Phrases::Error << "The specified JavaScript file \"" << jsPath << "\" does not export a main() function." << Phrases::End;
        std::exit(EXIT_FAILURE);
    }

    // assign settings specified via CLI argument
    auto settings = engine.newObject();
    if (args.jsSettingsArg.isPresent()) {
        for (const auto *const setting : args.jsSettingsArg.values()) {
            if (const auto *const value = std::strchr(setting, '=')) {
                settings.setProperty(QString::fromUtf8(setting, value - setting), QJSValue(QString::fromUtf8(value + 1)));
            } else {
                settings.setProperty(QString::fromUtf8(setting), QJSValue());
            }
        }
    }
    engine.globalObject().setProperty(QStringLiteral("settings"), settings);

    // print warnings
    printDiagMessages(diag, "Diagnostic messages:", args.verboseArg.isPresent(), &args.pedanticArg);
    diag.clear();
}

/*!
 * \brief Calls the JavaScript's main() function for the specified \a mediaFileInfo populating \a diag.
 * \returns Returns what the main() function has returned.
 */
QJSValue JavaScriptProcessor::callMain(MediaFileInfo &mediaFileInfo, Diagnostics &diag)
{
    auto fileInfoObject = MediaFileInfoObject(mediaFileInfo, diag, &engine);
    auto fileInfoObjectValue = engine.newQObject(&fileInfoObject);
    auto context = argsToString("executing JavaScript for ", mediaFileInfo.fileName());
    utility->setDiag(&context, &diag);
    QObject::connect(
        &engine, &QQmlEngine::warnings, &fileInfoObject, [&diag, &context](const auto &warnings) { addWarnings(diag, context, warnings); });
    diag.emplace_back(DiagLevel::Information, "entering main() function", context);
    auto res = main.call(QJSValueList({ fileInfoObjectValue }));
    if (res.isError()) {
        diag.emplace_back(DiagLevel::Fatal,
            argsToString(res.toString().toStdString(), " at line ", res.property(QStringLiteral("lineNumber")).toInt(), '.'), context);
    } else if (!res.isUndefined()) {
        diag.emplace_back(DiagLevel::Information, argsToString("done with return value: ", res.toString().toStdString()), context);
    } else {
        diag.emplace_back(DiagLevel::Debug, "done without return value", context);
    }
    return res;
}

/*!
 * \brief Adds the \a warnings to the specified \a diag object with the specified \a context.
 */
void JavaScriptProcessor::addWarnings(Diagnostics &diag, const string &context, const QList<QQmlError> &warnings)
{
    for (const auto &warning : warnings) {
        diag.emplace_back(DiagLevel::Warning, warning.toString().toStdString(), context);
    }
}
#endif

/*!
 * \brief Implements the "set"-operation of the CLI.
 */
void setTagInfo(const SetTagInfoArgs &args)
{
    CMD_UTILS_START_CONSOLE;

    // check whether files have been specified
    if (!args.filesArg.isPresent() || args.filesArg.values().empty()) {
        std::cerr << Phrases::Error << "No files have been specified." << Phrases::EndFlush;
        std::exit(EXIT_FAILURE);
    }
    if (args.outputFilesArg.isPresent() && args.outputFilesArg.values().size() != args.filesArg.values().size()) {
        std::cerr << Phrases::Error << "The number of output files does not match the number of input files." << Phrases::EndFlush;
        std::exit(EXIT_FAILURE);
    }

    // get output files
    auto &outputFiles = args.outputFilesArg.isPresent() ? args.outputFilesArg.values() : vector<const char *>();
    auto currentOutputFile = outputFiles.cbegin(), noMoreOutputFiles = outputFiles.cend();

    // parse field denotations and check whether there's an operation to be done (changing fields or some other settings)
    auto fields = parseFieldDenotations(args.valuesArg, false);
    if (fields.empty() && (!args.removeTargetArg.isPresent() || args.removeTargetArg.values().empty())
        && (!args.addAttachmentArg.isPresent() || args.addAttachmentArg.values().empty())
        && (!args.updateAttachmentArg.isPresent() || args.updateAttachmentArg.values().empty())
        && (!args.removeAttachmentArg.isPresent() || args.removeAttachmentArg.values().empty())
        && (!args.docTitleArg.isPresent() || args.docTitleArg.values().empty()) && !args.id3v1UsageArg.isPresent() && !args.id3v2UsageArg.isPresent()
        && !args.id3v2VersionArg.isPresent() && !args.jsArg.isPresent()) {
        if (!args.layoutOnlyArg.isPresent()) {
            std::cerr << Phrases::Error << "No fields/attachments have been specified." << Phrases::End
                      << "note: This is usually a mistake. Use --layout-only to prevent this error and apply file layout options only." << endl;
            std::exit(EXIT_FAILURE);
        }
    } else if (args.layoutOnlyArg.isPresent()) {
        std::cerr << Phrases::Error << "Fields/attachments and --layout-only have been specified." << Phrases::End
                  << "note: Don't use --layout-only if you actually want to alter tag information or attachments." << endl;
        std::exit(EXIT_FAILURE);
    }

    auto settings = TagCreationSettings();
    settings.flags = TagCreationFlags::None;

    // determine targets to remove
    auto targetsToRemove = std::vector<TagTarget>();
    for (size_t i = 0, max = args.removeTargetArg.occurrences(); i != max; ++i) {
        auto &target = targetsToRemove.emplace_back();
        for (const auto &targetDenotation : args.removeTargetArg.values(i)) {
            if (!applyTargetConfiguration(target, targetDenotation)) {
                std::cerr << Phrases::Error << "The given target specification \"" << targetDenotation << "\" is invalid." << Phrases::EndFlush;
                std::exit(EXIT_FAILURE);
            }
        }
    }

    // parse ID3v2 version
    if (args.id3v2VersionArg.isPresent()) {
        try {
            settings.id3v2MajorVersion = stringToNumber<std::uint8_t>(args.id3v2VersionArg.values().front());
            if (settings.id3v2MajorVersion < 1 || settings.id3v2MajorVersion > 4) {
                throw ConversionException();
            }
        } catch (const ConversionException &) {
            settings.id3v2MajorVersion = 3;
            std::cerr << Phrases::Error << "The specified ID3v2 version \"" << args.id3v2VersionArg.values().front() << "\" is invalid."
                      << Phrases::End << "note: Valid versions are 1, 2, 3 and 4." << endl;
            std::exit(EXIT_FAILURE);
        }
    }

    // parse flags
    if (args.treatUnknownFilesAsMp3FilesArg.isPresent()) {
        settings.flags |= TagCreationFlags::TreatUnknownFilesAsMp3Files;
    }
    if (args.id3InitOnCreateArg.isPresent()) {
        settings.flags |= TagCreationFlags::Id3InitOnCreate;
    }
    if (args.id3TransferOnRemovalArg.isPresent()) {
        settings.flags |= TagCreationFlags::Id3TransferValuesOnRemoval;
    }
    if (args.mergeMultipleSuccessiveTagsArg.isPresent()) {
        settings.flags |= TagCreationFlags::MergeMultipleSuccessiveId3v2Tags;
    }
    if (!args.id3v2VersionArg.isPresent()) {
        settings.flags |= TagCreationFlags::KeepExistingId3v2Version;
    }

    // parse other settings
    const auto denotedEncoding = parseEncodingDenotation(args.encodingArg, TagTextEncoding::Utf8);
    settings.id3v1usage = parseUsageDenotation(args.id3v1UsageArg, TagUsage::KeepExisting);
    settings.id3v2usage = parseUsageDenotation(args.id3v2UsageArg, TagUsage::Always);

    // setup media file info
    auto fileInfo = MediaFileInfo();
    auto tags = std::vector<Tag *>();
    fileInfo.setMinPadding(parseUInt64(args.minPaddingArg, 0));
    fileInfo.setMaxPadding(parseUInt64(args.maxPaddingArg, 0));
    fileInfo.setPreferredPadding(parseUInt64(args.prefPaddingArg, 0));
    fileInfo.setTagPosition(parsePositionDenotation(args.tagPosArg, args.tagPosValueArg, ElementPosition::BeforeData));
    fileInfo.setForceTagPosition(args.forceTagPosArg.isPresent());
    fileInfo.setIndexPosition(parsePositionDenotation(args.indexPosArg, args.indexPosValueArg, ElementPosition::BeforeData));
    fileInfo.setForceIndexPosition(args.forceIndexPosArg.isPresent());
    fileInfo.setForceRewrite(args.forceRewriteArg.isPresent());
    fileInfo.setWritingApplication(APP_NAME " v" APP_VERSION);
    if (args.preserveMuxingAppArg.isPresent()) {
        fileInfo.setFileHandlingFlags(fileInfo.fileHandlingFlags() | MediaFileHandlingFlags::PreserveMuxingApplication);
    }
    if (args.preserveWritingAppArg.isPresent()) {
        fileInfo.setFileHandlingFlags(fileInfo.fileHandlingFlags() | MediaFileHandlingFlags::PreserveWritingApplication);
    }

    // set backup path
    if (args.backupDirArg.isPresent()) {
        fileInfo.setBackupDirectory(std::string(args.backupDirArg.values().front()));
    }

    // initialize JavaScript processing if --java-script argument is present
#ifdef TAGEDITOR_USE_JSENGINE
    auto js = args.jsArg.isPresent() ? std::make_unique<JavaScriptProcessor>(args) : std::unique_ptr<JavaScriptProcessor>();
#else
    if (args.jsArg.isPresent()) {
        std::cerr << Phrases::Error << "A JavaScript has been specified but support for this has been disabled at compile-time." << Phrases::EndFlush;
        std::exit(EXIT_FAILURE);
    }
#endif

    // iterate through all specified files
    const auto quiet = args.quietArg.isPresent();
    auto fileIndex = 0u;
    static auto context = std::string("setting tags");
    const auto continueWithNextFile = [&](Diagnostics &diag) {
        printDiagMessages(diag, "Diagnostic messages:", args.verboseArg.isPresent(), &args.pedanticArg);
        ++fileIndex;
        if (currentOutputFile != noMoreOutputFiles) {
            ++currentOutputFile;
        }
    };
    for (const char *file : args.filesArg.values()) {
        auto diag = Diagnostics();
        auto parsingProgress = AbortableProgressFeedback(); // FIXME: actually use the progress object
        try {
            // parse tags and tracks (tracks are relevant because track meta-data such as language can be changed as well)
            if (!quiet) {
                cout << TextAttribute::Bold << "Setting tag information for \"" << file << "\" ..." << Phrases::EndFlush;
            }
            fileInfo.setPath(std::string(file));
            fileInfo.parseContainerFormat(diag, parsingProgress);
            fileInfo.parseTags(diag, parsingProgress);
            fileInfo.parseTracks(diag, parsingProgress);
            fileInfo.parseAttachments(diag, parsingProgress);

            // remove tags with the specified targets
            if (!targetsToRemove.empty()) {
                tags.clear();
                fileInfo.tags(tags);
                for (auto *const tag : tags) {
                    if (find(targetsToRemove.cbegin(), targetsToRemove.cend(), tag->target()) != targetsToRemove.cend()) {
                        fileInfo.removeTag(tag);
                    }
                }
            }

            // select the relevant values for the current file index
            for (auto &fieldDenotation : fields) {
                FieldValues &denotedValues = fieldDenotation.second;
                std::vector<FieldValue *> &relevantDenotedValues = denotedValues.relevantValues;
                relevantDenotedValues.clear();
                unsigned int currentFileIndex = 0;
                for (FieldValue &denotatedValue : denotedValues.allValues) {
                    if ((denotatedValue.fileIndex <= fileIndex)
                        && (relevantDenotedValues.empty() || (denotatedValue.fileIndex >= currentFileIndex))) {
                        if (currentFileIndex != denotatedValue.fileIndex) {
                            currentFileIndex = denotatedValue.fileIndex;
                            relevantDenotedValues.clear();
                        }
                        relevantDenotedValues.push_back(&denotatedValue);
                    }
                }
            }

            // determine required targets
            settings.requiredTargets.clear();
            for (const auto &fieldDenotation : fields) {
                const FieldScope &scope = fieldDenotation.first;
                if (scope.isTrack() || !scope.exactTargetMatching) {
                    continue;
                }
                const std::vector<FieldValue *> &relevantDenotedValues = fieldDenotation.second.relevantValues;
                auto hasNonEmptyValues = false;
                for (const auto &value : relevantDenotedValues) {
                    if (!value->value.empty()) {
                        hasNonEmptyValues = true;
                        break;
                    }
                }
                if (hasNonEmptyValues
                    && std::find(settings.requiredTargets.cbegin(), settings.requiredTargets.cend(), scope.tagTarget)
                        == settings.requiredTargets.cend()) {
                    settings.requiredTargets.emplace_back(scope.tagTarget);
                }
            }

            // create new tags according to settings
            fileInfo.createAppropriateTags(settings);
            auto container = fileInfo.container();
            if (args.docTitleArg.isPresent() && !args.docTitleArg.values().empty()) {
                if (container && container->supportsTitle()) {
                    size_t segmentIndex = 0, segmentCount = container->titles().size();
                    for (const auto &newTitle : args.docTitleArg.values()) {
                        if (segmentIndex < segmentCount) {
                            container->setTitle(newTitle, segmentIndex);
                        } else {
                            diag.emplace_back(DiagLevel::Warning,
                                argsToString(
                                    "The specified document title \"", newTitle, "\" can not be set because the file has not that many segments."),
                                context);
                        }
                        ++segmentIndex;
                    }
                } else {
                    diag.emplace_back(DiagLevel::Warning, "Setting the document title is not supported for the file.", context);
                }
            }

            // process tag fields via the specified JavaScript
#ifdef TAGEDITOR_USE_JSENGINE
            if (js) {
                const auto res = js->callMain(fileInfo, diag);
                if (res.isError() || diag.has(DiagLevel::Fatal)) {
                    if (!quiet) {
                        std::cout << " - Skipping file due to fatal error when executing JavaScript.\n";
                    }
                    continueWithNextFile(diag);
                    continue;
                }
                if (!res.isUndefined() && !res.toBool()) {
                    if (!quiet) {
                        std::cout << " - Skipping file because JavaScript returned a falsy value other than undefined.\n";
                    }
                    continueWithNextFile(diag);
                    continue;
                }
            }
#endif

            // alter tags
            tags.clear();
            fileInfo.tags(tags);
            if (tags.empty()) {
                diag.emplace_back(DiagLevel::Critical, "Can not create appropriate tags for file.", context);
            } else {
                // iterate through all tags
                const auto willWriteAnId3v2Tag = fileInfo.hasId3v2Tag();
                for (auto *tag : tags) {
                    // clear current values if option is present
                    if (args.removeOtherFieldsArg.isPresent()) {
                        tag->removeAllFields();
                    }
                    // determine required information for deciding whether specified values match the scope of the current tag
                    const auto tagType = tag->type();
                    const bool targetSupported = tag->supportsTarget();
                    const auto tagTarget = tag->target();
                    // determine the encoding to store text values
                    TagTextEncoding usedEncoding = denotedEncoding;
                    if (!tag->canEncodingBeUsed(denotedEncoding)) {
                        usedEncoding = tag->proposedTextEncoding();
                        if (args.encodingArg.isPresent()) {
                            diag.emplace_back(DiagLevel::Warning,
                                argsToString("Can't use specified encoding \"", args.encodingArg.values().front(), "\" in ", tagName(tag),
                                    " because the tag format/version doesn't support it."),
                                context);
                        }
                    }
                    // iterate through all denoted field values
                    for (const auto &fieldDenotation : fields) {
                        const FieldScope &denotedScope = fieldDenotation.first;
                        // skip values which scope does not match the current tag
                        if (denotedScope.isTrack() || !(denotedScope.tagType == TagType::Unspecified || (denotedScope.tagType & tagType))
                            || !(!targetSupported || (tagType == TagType::OggVorbisComment && denotedScope.tagTarget.isEmpty())
                                || (denotedScope.exactTargetMatching ? denotedScope.tagTarget == tagTarget
                                                                     : denotedScope.tagTarget.matches(tagTarget)))) {
                            continue;
                        }
                        // convert the values to TagValue
                        auto convertedValues = std::vector<TagValue>();
                        auto convertedId3v2CoverValues = std::vector<Id3v2Cover>();
                        for (const FieldValue *relevantDenotedValue : fieldDenotation.second.relevantValues) {
                            // assign an empty TagValue to remove the field if denoted value is empty
                            if (relevantDenotedValue->value.empty()) {
                                convertedValues.emplace_back();
                                continue;
                            }
                            // add text value
                            if (relevantDenotedValue->type != DenotationType::File) {
                                try {
                                    convertedValues.emplace_back(relevantDenotedValue->value, TagTextEncoding::Utf8, usedEncoding);
                                } catch (const ConversionException &e) {
                                    diag.emplace_back(DiagLevel::Critical,
                                        argsToString("Unable to parse value specified for field \"", denotedScope.field.name(), "\": ", e.what()),
                                        context);
                                }
                                continue;
                            }
                            // add value from file
                            const auto denotedValue = relevantDenotedValue->value;
                            const auto firstPartIsDriveLetter = denotedValue.size() >= 2 && denotedValue[1] == ':' ? 1u : 0u;
                            const auto maxParts = std::size_t(3u + firstPartIsDriveLetter);
                            const auto parts = splitStringSimple<std::vector<std::string_view>>(denotedValue, ":", static_cast<int>(maxParts));
                            const auto path = parts.empty()
                                ? std::string_view()
                                : (firstPartIsDriveLetter ? std::string_view(denotedValue.data(), parts[0].size() + parts[1].size() + 1)
                                                          : parts.front());
                            const auto fieldType = denotedScope.field.knownFieldForTag(tag, tagType);
                            const auto dataType = fieldType == KnownField::Cover ? TagDataType::Picture : TagDataType::Text;
                            try {
                                // assume the file refers to a picture
                                auto value = TagValue();
                                if (!path.empty()) {
                                    auto coverFileInfo = MediaFileInfo(path);
                                    auto coverDiag = Diagnostics();
                                    auto coverProgress = AbortableProgressFeedback(); // FIXME: actually use the progress object
                                    coverFileInfo.open(true);
                                    coverFileInfo.parseContainerFormat(coverDiag, coverProgress);
                                    auto buff = make_unique<char[]>(coverFileInfo.size());
                                    coverFileInfo.stream().seekg(static_cast<streamoff>(0));
                                    coverFileInfo.stream().read(buff.get(), static_cast<streamoff>(coverFileInfo.size()));
                                    value = TagValue(std::move(buff), coverFileInfo.size(), dataType, TagTextEncoding::Utf8);
                                    value.setMimeType(coverFileInfo.mimeType());
                                }
                                auto description = std::optional<std::string_view>();
                                if (parts.size() > 2u + firstPartIsDriveLetter) {
                                    description = parts[2 + firstPartIsDriveLetter];
                                    value.setDescription(description.value(), TagTextEncoding::Utf8);
                                }
                                if (parts.size() > 1u + firstPartIsDriveLetter && fieldType == KnownField::Cover
                                    && (tagType == TagType::Id3v2Tag || tagType == TagType::VorbisComment)) {
                                    const auto typeSpec = parts[1 + firstPartIsDriveLetter];
                                    const auto coverType = id3v2CoverType(typeSpec);
                                    if (coverType == invalidCoverType) {
                                        diag.emplace_back(DiagLevel::Warning,
                                            argsToString("Specified cover type \"", typeSpec, "\" is invalid. Ignoring the specified field/value."),
                                            context);
                                    } else {
                                        convertedId3v2CoverValues.emplace_back(std::move(value), coverType, description);
                                    }
                                } else {
                                    if (parts.size() > 1u + firstPartIsDriveLetter) {
                                        diag.emplace_back(
                                            tag->type() == TagType::Id3v1Tag && fileInfo.hasId3v2Tag() ? DiagLevel::Information : DiagLevel::Warning,
                                            argsToString("Ignoring cover type \"", parts[1 + firstPartIsDriveLetter], "\" for ", tag->typeName(),
                                                ". It is only supported by the cover field and the tag formats ID3v2 and Vorbis Comment."),
                                            context);
                                    }
                                    convertedValues.emplace_back(std::move(value));
                                }
                            } catch (const TagParser::Failure &) {
                                diag.emplace_back(DiagLevel::Critical,
                                    argsToString("Unable to parse specified file \"", path, "\": unable to determine MIME type"), context);
                            } catch (const ConversionException &e) {
                                diag.emplace_back(
                                    DiagLevel::Critical, argsToString("Unable to parse specified file \"", path, "\": ", e.what()), context);
                            } catch (const std::ios_base::failure &e) {
                                diag.emplace_back(DiagLevel::Critical,
                                    argsToString("An IO error occurred when parsing the specified file \"", path, "\": ", e.what()), context);
                            }
                        }
                        // finally set/clear the values
                        try {
                            // add error if field is not supported unless it is just ID3v1 and we are writing an ID3v2 tag as well
                            if ((!convertedValues.empty() || convertedId3v2CoverValues.empty())
                                && !denotedScope.field.setValues(tag, tagType, convertedValues)
                                && (tagType != TagType::Id3v1Tag || !willWriteAnId3v2Tag)) {
                                diag.emplace_back(DiagLevel::Critical,
                                    argsToString(
                                        "Unable set field \"", denotedScope.field.name(), "\": field is not supported for ", tag->typeName()),
                                    context);
                            }
                        } catch (const ConversionException &e) {
                            diag.emplace_back(DiagLevel::Critical,
                                argsToString("Unable to parse denoted field ID \"", denotedScope.field.name(), "\": ", e.what()), context);
                        }
                        if (!convertedId3v2CoverValues.empty()) {
                            switch (tagType) {
                            case TagType::Id3v2Tag:
                                setId3v2CoverValues(static_cast<Id3v2Tag *>(tag), std::move(convertedId3v2CoverValues));
                                break;
                            case TagType::VorbisComment:
                                setId3v2CoverValues(static_cast<VorbisComment *>(tag), std::move(convertedId3v2CoverValues));
                                break;
                            default:;
                            }
                        }
                    }
                }
            }

            // alter tracks
            for (AbstractTrack *const track : fileInfo.tracks()) {
                for (const auto &fieldDenotation : fields) {
                    // skip empty values
                    const auto &values = fieldDenotation.second.relevantValues;
                    if (values.empty()) {
                        continue;
                    }

                    // skip values which scope does not match the current track
                    const FieldScope &denotedScope = fieldDenotation.first;
                    if (!denotedScope.allTracks
                        && find(denotedScope.trackIds.cbegin(), denotedScope.trackIds.cend(), track->id()) == denotedScope.trackIds.cend()) {
                        continue;
                    }

                    const FieldId &field = denotedScope.field;
                    const string &value = values.front()->value;
                    try {
                        if (field.denotes("name")) {
                            track->setName(value);
                        } else if (field.denotes("language")) {
                            track->setLocale(Locale(std::string_view(value), LocaleFormat::Unknown));
                        } else if (field.denotes("tracknumber")) {
                            track->setTrackNumber(stringToNumber<std::uint32_t>(value));
                        } else if (field.denotes("enabled")) {
                            track->setEnabled(stringToBool(value));
                        } else if (field.denotes("forced")) {
                            track->setForced(stringToBool(value));
                        } else if (field.denotes("default")) {
                            track->setDefault(stringToBool(value));
                        } else {
                            diag.emplace_back(DiagLevel::Critical,
                                argsToString("Denoted track property name \"", field.denotation(), "\" is invalid"),
                                argsToString("setting meta-data of track ", track->id()));
                        }
                    } catch (const ConversionException &e) {
                        diag.emplace_back(DiagLevel::Critical,
                            argsToString("Unable to parse value for track property \"", field.denotation(), "\": ", e.what()),
                            argsToString("setting meta-data of track ", track->id()));
                    }
                }
            }

            // increment relevant values
            for (auto &fieldDenotation : fields) {
                for (FieldValue *relevantDenotedValue : fieldDenotation.second.relevantValues) {
                    if (!relevantDenotedValue->value.empty() && relevantDenotedValue->type == DenotationType::Increment) {
                        relevantDenotedValue->value = incremented(relevantDenotedValue->value);
                    }
                }
            }

            // alter attachments
            if (args.addAttachmentArg.isPresent() || args.updateAttachmentArg.isPresent() || args.removeAttachmentArg.isPresent()
                || args.removeExistingAttachmentsArg.isPresent()) {
                static const string attachmentsContext("setting attachments");
                fileInfo.parseAttachments(diag, parsingProgress);
                if (fileInfo.attachmentsParsingStatus() == ParsingStatus::Ok && container) {
                    // ignore all existing attachments if argument is specified
                    if (args.removeExistingAttachmentsArg.isPresent()) {
                        for (size_t i = 0, count = container->attachmentCount(); i < count; ++i) {
                            container->attachment(i)->setIgnored(false);
                        }
                    }
                    // add/update/remove attachments
                    auto currentInfo = AttachmentInfo();
                    currentInfo.action = AttachmentAction::Add;
                    for (size_t i = 0, occurrences = args.addAttachmentArg.occurrences(); i != occurrences; ++i) {
                        for (const char *value : args.addAttachmentArg.values(i)) {
                            currentInfo.parseDenotation(value);
                        }
                        currentInfo.next(container, diag);
                    }
                    currentInfo.action = AttachmentAction::Update;
                    for (size_t i = 0, occurrences = args.updateAttachmentArg.occurrences(); i != occurrences; ++i) {
                        for (const char *value : args.updateAttachmentArg.values(i)) {
                            currentInfo.parseDenotation(value);
                        }
                        currentInfo.next(container, diag);
                    }
                    currentInfo.action = AttachmentAction::Remove;
                    for (size_t i = 0, occurrences = args.removeAttachmentArg.occurrences(); i != occurrences; ++i) {
                        for (const char *value : args.removeAttachmentArg.values(i)) {
                            currentInfo.parseDenotation(value);
                        }
                        currentInfo.next(container, diag);
                    }
                } else if (fileInfo.attachmentsParsingStatus() == ParsingStatus::NotSupported) {
                    diag.emplace_back(
                        DiagLevel::Critical, "Unable to assign attachments because that is not supported for the file's format.", attachmentsContext);
                } else {
                    diag.emplace_back(DiagLevel::Critical, "Unable to assign attachments because the container object has not been initialized.",
                        attachmentsContext);
                }
            }

            // apply changes
            auto modificationDateError = std::error_code();
            auto modificationDate = std::filesystem::file_time_type();
            auto modifiedFilePath = std::filesystem::path();
            fileInfo.setSaveFilePath(currentOutputFile != noMoreOutputFiles ? string(*currentOutputFile) : string());
            if (args.preserveModificationTimeArg.isPresent()) {
                modifiedFilePath = makeNativePath(fileInfo.saveFilePath().empty() ? fileInfo.path() : fileInfo.saveFilePath());
                modificationDate = std::filesystem::last_write_time(modifiedFilePath, modificationDateError);
            }
            try {
                // create handler for progress updates and aborting
                auto applyProgress = quiet ? AbortableProgressFeedback() : AbortableProgressFeedback(logNextStep, logStepPercentage);
                const auto handler = InterruptHandler(std::bind(&AbortableProgressFeedback::tryToAbort, std::ref(applyProgress)));

                // apply changes
                fileInfo.applyChanges(diag, applyProgress);

                // notify about completion
                finalizeLog();
                if (!quiet) {
                    cout << " - Changes have been applied." << endl;
                }
            } catch (const TagParser::OperationAbortedException &) {
                finalizeLog();
                cerr << Phrases::Warning << "The operation has been aborted." << Phrases::EndFlush;
                return;
            } catch (const TagParser::Failure &) {
                finalizeLog();
                cerr << " - " << Phrases::Error << "Failed to apply changes." << Phrases::EndFlush;
                exitCode = EXIT_PARSING_FAILURE;
            }
            if (args.preserveModificationTimeArg.isPresent()) {
                if (!modificationDateError) {
                    std::filesystem::last_write_time(modifiedFilePath, modificationDate, modificationDateError);
                }
                if (modificationDateError) {
                    diag.emplace_back(DiagLevel::Critical, "Unable to preserve modification time: " + modificationDateError.message(), context);
                }
            }
        } catch (const TagParser::Failure &) {
            finalizeLog();
            cerr << " - " << Phrases::Error << "A parsing failure occurred when reading/writing the file \"" << file << "\"." << Phrases::EndFlush;
            exitCode = EXIT_PARSING_FAILURE;
        } catch (const std::ios_base::failure &e) {
            finalizeLog();
            cerr << " - " << Phrases::Error << "An IO error occurred when reading/writing the file \"" << file << "\": " << e.what()
                 << Phrases::EndFlush;
            exitCode = EXIT_IO_FAILURE;
        }
        continueWithNextFile(diag);
    }
}

void extractField(const Argument &fieldArg, const Argument &attachmentArg, const Argument &inputFilesArg, const Argument &outputFileArg,
    const Argument &indexArg, const Argument &verboseArg)
{
    CMD_UTILS_START_CONSOLE;

    // parse specified field and attachment
    const auto fieldDenotations = parseFieldDenotations(fieldArg, true);
    auto attachmentInfo = AttachmentInfo();
    if (attachmentArg.isPresent()) {
        attachmentInfo.parseDenotation(attachmentArg.values().front());
    }
    if (((fieldDenotations.size() != 1) || (!attachmentInfo.hasId && !attachmentInfo.name))
        && ((fieldDenotations.size() == 1) && (attachmentInfo.hasId || attachmentInfo.name))) {
        std::cerr << Phrases::Error << "Exactly one field or attachment needs to be specified." << Phrases::EndFlush;
        std::exit(EXIT_FAILURE);
    }
    if (!inputFilesArg.isPresent() || inputFilesArg.values().empty()) {
        std::cerr << Phrases::Error << "No files have been specified." << Phrases::EndFlush;
        std::exit(EXIT_FAILURE);
    }

    static constexpr auto noIndex = std::numeric_limits<std::size_t>::max();
    auto index = noIndex;
    if (indexArg.isPresent()) {
        try {
            if ((index = stringToNumber<std::size_t>(indexArg.firstValue())) == noIndex) {
                throw ConversionException();
            }
        } catch (const ConversionException &) {
            std::cerr << Phrases::Error << "Specified index is no valid unsigned integer." << Phrases::EndFlush;
            std::exit(EXIT_FAILURE);
        }
    }

    // read values/attachments
    auto inputFileInfo = MediaFileInfo();
    auto values = std::vector<std::pair<const TagValue *, std::string>>();
    auto attachments = std::vector<std::pair<const AbstractAttachment *, std::string>>();
    auto diag = Diagnostics();
    for (const char *file : inputFilesArg.values()) {
        auto progress = AbortableProgressFeedback(); // FIXME: actually use the progress object
        try {
            // setup media file info
            inputFileInfo.setPath(std::string_view(file));
            inputFileInfo.open(true);

            // extract either tag field or attachment
            if (!fieldDenotations.empty()) {
                // extract tag field
                (outputFileArg.isPresent() ? cout : cerr) << "Extracting field " << fieldArg.values().front() << " of \"" << file << "\" ..." << endl;
                inputFileInfo.parseContainerFormat(diag, progress);
                inputFileInfo.parseTags(diag, progress);
                auto tags = inputFileInfo.tags();
                // iterate through all tags
                for (const Tag *tag : tags) {
                    const TagType tagType = tag->type();
                    for (const auto &fieldDenotation : fieldDenotations) {
                        try {
                            const auto valuesForField = fieldDenotation.first.field.values(tag, tagType);
                            // skip if field ID is format specific and not relevant for the current format
                            if (!valuesForField.second) {
                                continue;
                            }
                            for (const TagValue *value : valuesForField.first) {
                                values.emplace_back(value, joinStrings({ std::string(tag->typeName()), numberToString(values.size()) }, "-", true));
                            }
                        } catch (const ConversionException &e) {
                            diag.emplace_back(DiagLevel::Critical,
                                argsToString("Unable to parse denoted field ID \"", fieldDenotation.first.field.name(), "\": ", e.what()),
                                "extracting field");
                        }
                    }
                }
            } else {
                // extract attachment
                auto &logStream = (outputFileArg.isPresent() ? cout : cerr);
                logStream << "Extracting attachment with ";
                if (attachmentInfo.hasId) {
                    logStream << "ID " << attachmentInfo.id;
                } else {
                    logStream << "name \"" << attachmentInfo.name << '\"';
                }
                logStream << " of \"" << file << "\" ..." << endl;

                inputFileInfo.parseContainerFormat(diag, progress);
                inputFileInfo.parseAttachments(diag, progress);

                // iterate through all attachments
                for (const AbstractAttachment *attachment : inputFileInfo.attachments()) {
                    if ((attachmentInfo.hasId && attachment->id() == attachmentInfo.id) || (attachment->name() == attachmentInfo.name)) {
                        attachments.emplace_back(attachment, joinStrings({ attachment->name(), numberToString(attachments.size()) }, "-", true));
                    }
                }
            }
        } catch (const TagParser::Failure &) {
            cerr << Phrases::Error << "A parsing failure occurred when reading the file \"" << file << "\"." << Phrases::End;
            exitCode = EXIT_PARSING_FAILURE;
        } catch (const std::ios_base::failure &e) {
            cerr << Phrases::Error << "An IO error occurred when reading the file \"" << file << "\": " << e.what() << Phrases::End;
            exitCode = EXIT_IO_FAILURE;
        }
    }

    // write values/attachments
    if (!fieldDenotations.empty()) {
        if (values.empty()) {
            cerr << Phrases::Error << "None of the specified files has a (supported) " << fieldArg.values().front() << " field." << Phrases::End;
            exitCode = exitCode != EXIT_SUCCESS ? exitCode : EXIT_FAILURE;
        } else if (index != noIndex && index >= values.size()) {
            cerr << Phrases::Error << "The specified index is out of range as the specified files/fields have only " << values.size() << " values."
                 << Phrases::End;
            exitCode = exitCode != EXIT_SUCCESS ? exitCode : EXIT_FAILURE;
        } else if (outputFileArg.isPresent()) {
            if (index != noIndex) {
                if (index) {
                    values[index].swap(values.front());
                }
                values.resize(1);
            }
            auto outputFilePathWithoutExtension = std::string(), outputFileExtension = std::string();
            if (values.size() > 1) {
                outputFilePathWithoutExtension = BasicFileInfo::pathWithoutExtension(outputFileArg.values().front());
                outputFileExtension = BasicFileInfo::extension(outputFileArg.values().front());
            }
            for (const auto &value : values) {
                auto outputFileStream = NativeFileStream();
                outputFileStream.exceptions(ios_base::failbit | ios_base::badbit);
                auto path = values.size() > 1 ? joinStrings({ outputFilePathWithoutExtension, "-", value.second, outputFileExtension })
                                              : outputFileArg.values().front();
                try {
                    outputFileStream.open(path, ios_base::out | ios_base::binary);
                    outputFileStream.write(value.first->dataPointer(), static_cast<std::streamsize>(value.first->dataSize()));
                    outputFileStream.flush();
                    cout << "Value has been saved to \"" << path << "\"." << endl;
                } catch (const std::ios_base::failure &e) {
                    cerr << Phrases::Error << "An IO error occurred when writing the file \"" << path << "\": " << e.what() << Phrases::End;
                    exitCode = exitCode != EXIT_SUCCESS ? exitCode : EXIT_IO_FAILURE;
                }
            }
        } else {
            // write data to stdout if no output file has been specified
            for (const auto &value : values) {
                cout.write(value.first->dataPointer(), static_cast<std::streamsize>(value.first->dataSize()));
            }
        }
    } else {
        if (attachments.empty()) {
            cerr << Phrases::Error << "None of the specified files has a (supported) attachment with the specified ID/name." << Phrases::End;
            exitCode = exitCode != EXIT_SUCCESS ? exitCode : EXIT_FAILURE;
        } else if (index != noIndex && index >= values.size()) {
            cerr << Phrases::Error << "The specified index is out of range as the specified files have only " << attachments.size() << " attachments."
                 << Phrases::End;
            exitCode = exitCode != EXIT_SUCCESS ? exitCode : EXIT_FAILURE;
        } else if (outputFileArg.isPresent()) {
            if (index != noIndex) {
                if (index) {
                    attachments[index].swap(attachments.front());
                }
                attachments.resize(1);
            }
            auto outputFilePathWithoutExtension = std::string(), outputFileExtension = std::string();
            if (attachments.size() > 1) {
                outputFilePathWithoutExtension = BasicFileInfo::pathWithoutExtension(outputFileArg.values().front());
                outputFileExtension = BasicFileInfo::extension(outputFileArg.values().front());
            }
            for (const auto &attachment : attachments) {
                auto outputFileStream = NativeFileStream();
                outputFileStream.exceptions(ios_base::failbit | ios_base::badbit);
                auto path = attachments.size() > 1 ? joinStrings({ outputFilePathWithoutExtension, "-", attachment.second, outputFileExtension })
                                                   : outputFileArg.values().front();
                try {
                    outputFileStream.open(path, ios_base::out | ios_base::binary);
                    attachment.first->data()->copyTo(outputFileStream);
                    outputFileStream.flush();
                    cout << "Value has been saved to \"" << path << "\"." << endl;
                } catch (const std::ios_base::failure &e) {
                    cerr << Phrases::Error << "An IO error occurred when writing the file \"" << path << "\": " << e.what() << Phrases::EndFlush;
                    exitCode = exitCode != EXIT_SUCCESS ? exitCode : EXIT_IO_FAILURE;
                }
            }
        } else {
            for (const auto &attachment : attachments) {
                attachment.first->data()->copyTo(cout);
            }
        }
    }

    printDiagMessages(diag, "Diagnostic messages:", verboseArg.isPresent());
}

void exportToJson(const ArgumentOccurrence &, const Argument &filesArg, const Argument &prettyArg)
{
    CMD_UTILS_START_CONSOLE;

#ifdef TAGEDITOR_JSON_EXPORT
    // check whether files have been specified
    if (!filesArg.isPresent() || filesArg.values().empty()) {
        std::cerr << Phrases::Error << "No files have been specified." << Phrases::End;
        std::exit(EXIT_FAILURE);
    }

    RAPIDJSON_NAMESPACE::Document document(RAPIDJSON_NAMESPACE::kArrayType);
    std::vector<Json::FileInfo> jsonData;
    MediaFileInfo fileInfo;

    // gather tags for each file
    Diagnostics diag; // FIXME: actually use diag object
    AbortableProgressFeedback progress; // FIXME: actually use the progress object
    for (const char *file : filesArg.values()) {
        try {
            // parse tags
            fileInfo.setPath(std::string(file));
            fileInfo.open(true);
            fileInfo.parseContainerFormat(diag, progress);
            fileInfo.parseTags(diag, progress);
            fileInfo.parseTracks(diag, progress);
            jsonData.emplace_back(fileInfo, document.GetAllocator());
        } catch (const TagParser::Failure &) {
            cerr << Phrases::Error << "A parsing failure occurred when reading the file \"" << file << "\"." << Phrases::EndFlush;
            exitCode = EXIT_PARSING_FAILURE;
        } catch (const std::ios_base::failure &e) {
            cerr << Phrases::Error << "An IO error occurred when reading the file \"" << file << "\": " << e.what() << Phrases::EndFlush;
            exitCode = EXIT_IO_FAILURE;
        }
    }

    // TODO: serialize diag messages

    // print the gathered data as JSON document
    ReflectiveRapidJSON::JsonReflector::push(jsonData, document, document.GetAllocator());
    RAPIDJSON_NAMESPACE::OStreamWrapper osw(cout);
    if (prettyArg.isPresent()) {
        RAPIDJSON_NAMESPACE::PrettyWriter<RAPIDJSON_NAMESPACE::OStreamWrapper> writer(osw);
        document.Accept(writer);
    } else {
        RAPIDJSON_NAMESPACE::Writer<RAPIDJSON_NAMESPACE::OStreamWrapper> writer(osw);
        document.Accept(writer);
    }
    cout << endl;

#else
    CPP_UTILITIES_UNUSED(filesArg);
    CPP_UTILITIES_UNUSED(prettyArg);
    cerr << Phrases::Error << "JSON export has not been enabled when building the tag editor." << Phrases::EndFlush;
    exitCode = EXIT_FAILURE;
#endif
}

void applyGeneralConfig(const Argument &timeSapnFormatArg)
{
    timeSpanOutputFormat = parseTimeSpanOutputFormat(timeSapnFormatArg, TimeSpanOutputFormat::WithMeasures);
}

} // namespace Cli
