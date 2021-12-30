#include "../cli/mainfeatures.h"
#if defined(TAGEDITOR_GUI_QTWIDGETS)
#include "../gui/initiate.h"
#include "./knownfieldmodel.h"
#endif

#include "resources/config.h"

#if defined(TAGEDITOR_GUI_QTWIDGETS) || defined(TAGEDITOR_GUI_QTQUICK)
#include <qtutilities/misc/conversion.h>
#include <qtutilities/resources/qtconfigarguments.h>
#else
#include <c++utilities/application/fakeqtconfigarguments.h>
#endif
#include <c++utilities/application/commandlineutils.h>
#include <c++utilities/io/ansiescapecodes.h>
#include <c++utilities/misc/parseerror.h>

#include <functional>
#include <iostream>
#include <memory>

using namespace std;
using namespace std::placeholders;
using namespace CppUtilities;
#if defined(TAGEDITOR_GUI_QTWIDGETS) || defined(TAGEDITOR_GUI_QTQUICK)
using namespace QtUtilities;
#endif

namespace Cli {

SetTagInfoArgs::SetTagInfoArgs(Argument &filesArg, Argument &verboseArg)
    : filesArg(filesArg)
    , verboseArg(verboseArg)
    , quietArg("quiet", 'q', "suppress printing progress information")
    , docTitleArg("doc-title", 'd', "specifies the document title (has no affect if not supported by the container)",
          { "title of first segment", "title of second segment" })
    , removeOtherFieldsArg(
          "remove-other-fields", '\0', "removes ALL fields where no value has been provided for (to remove a specific field use eg. \"album=\")")
    , treatUnknownFilesAsMp3FilesArg("treat-unknown-as-mp3", '\0', "if present unknown files will be treated as MP3 files")
    , id3v1UsageArg("id3v1-usage", '\0',
          "specifies the ID3v1 usage (only used when already present by default); only relevant when dealing with MP3 files (or files treated as "
          "such)",
          { "always/keepexisting/never" })
    , id3v2UsageArg("id3v2-usage", '\0',
          "specifies the ID3v2 usage (always used by default); only relevant when dealing with MP3 files (or files treated as such)",
          { "always/keepexisting/never" })
    , mergeMultipleSuccessiveTagsArg("merge-successive-tags", '\0', "if present multiple successive ID3v2 tags will be merged")
    , id3v2VersionArg("id3v2-version", '\0', "forces a specific ID3v2 version to be used; only relevant when ID3v2 is used", { "1/2/3/4" })
    , id3InitOnCreateArg("id3-init-on-create", '\0',
          "indicates whether to initialize newly created ID3 tags (according to specified usage) with the values of the already present ID3 tags")
    , id3TransferOnRemovalArg("id3-transfer-on-removal", '\0',
          "indicates whether values of removed ID3 tags (according to specified usage) should be transferred to remaining ID3 tags (no values will "
          "be overwritten)")
    , encodingArg("encoding", '\0', "specifies the preferred encoding", { "latin1/utf8/utf16le/utf16be" })
    , removeTargetArg("remove-target", '\0', "removes all tags with the specified target")
    , addAttachmentArg("add-attachment", '\0', "adds a new attachment", { "path=some/file", "name=Some name", "desc=Some desc", "mime=mime/type" })
    , updateAttachmentArg(
          "update-attachment", '\0', "updates an existing attachment", { "path=some/file", "name=Some name", "desc=Some desc", "mime=mime/type" })
    , removeAttachmentArg("remove-attachment", '\0', "removes an existing attachment")
    , removeExistingAttachmentsArg(
          "remove-existing-attachments", 'r', "removes ALL existing attachments (to remove a specific attachment use --remove-attachment)")
    , minPaddingArg("min-padding", '\0',
          "specifies the minimum padding before the media data (enforces rewriting the file is the padding would be less)",
          { "min. padding in byte" })
    , maxPaddingArg("max-padding", '\0',
          "specifies the maximum padding before the media data (enforces rewriting the file is the padding would be more)",
          { "max. padding in byte" })
    , prefPaddingArg("preferred-padding", '\0', "specifies the preferred padding before the media data (used when the file is rewritten)",
          { "preferred padding in byte" })
    , tagPosValueArg("value", '\0', "specifies the position, either front, back or current", { "front/back/current" })
    , forceTagPosArg("force", '\0', "forces the specified position even if the file needs to be rewritten")
    , tagPosArg("tag-pos", '\0', "specifies the preferred tag position")
    , indexPosValueArg("value", '\0', "specifies the position, either front, back or current", { "front/back/current" })
    , forceIndexPosArg("force", '\0', "forces the specified position even if the file needs to be rewritten")
    , indexPosArg("index-pos", '\0', "specifies the preferred index position")
    , forceRewriteArg(
          "force-rewrite", '\0', "forces the file to rewritten from the scratch which ensures a backup is created and the preferred padding is used")
    , valuesArg("values", 'n', "specifies the values to be set", { "title=foo", "album=bar", "cover=/path/to/file" })
    , outputFilesArg("output-files", 'o', "specifies the output files; if present, the files specified with --files will not be modified",
          { "path 1", "path 2" })
    , backupDirArg("temp-dir", '\0', "specifies the directory for temporary/backup files", { "path" })
    , layoutOnlyArg("layout-only", 'l', "confirms layout-only changes")
    , preserveModificationTimeArg("preserve-modification-time", '\0', "preserves the file's modification time")
    , setTagInfoArg("set", 's', "sets the specified tag information and attachments")
{
    docTitleArg.setRequiredValueCount(Argument::varValueCount);
    id3v1UsageArg.setPreDefinedCompletionValues("always keepexisting never");
    id3v2UsageArg.setPreDefinedCompletionValues("always keepexisting never");
    id3v2VersionArg.setPreDefinedCompletionValues("1 2 3 4");
    encodingArg.setPreDefinedCompletionValues("latin1 utf8 utf16le utf16be");
    removeTargetArg.setRequiredValueCount(Argument::varValueCount);
    removeTargetArg.setConstraints(0, Argument::varValueCount);
    addAttachmentArg.setRequiredValueCount(Argument::varValueCount);
    addAttachmentArg.setConstraints(0, Argument::varValueCount);
    addAttachmentArg.setValueCompletionBehavior(ValueCompletionBehavior::PreDefinedValues | ValueCompletionBehavior::AppendEquationSign);
    addAttachmentArg.setPreDefinedCompletionValues("name id path desc mime");
    updateAttachmentArg.setRequiredValueCount(Argument::varValueCount);
    updateAttachmentArg.setConstraints(0, Argument::varValueCount);
    updateAttachmentArg.setValueCompletionBehavior(ValueCompletionBehavior::PreDefinedValues | ValueCompletionBehavior::AppendEquationSign);
    updateAttachmentArg.setPreDefinedCompletionValues("name id path desc mime");
    removeAttachmentArg.setRequiredValueCount(1);
    removeAttachmentArg.setValueNames({ "name=to_remove" });
    removeAttachmentArg.setCombinable(true);
    removeAttachmentArg.setConstraints(0, Argument::varValueCount);
    removeAttachmentArg.setPreDefinedCompletionValues("name id");
    removeAttachmentArg.setValueCompletionBehavior(ValueCompletionBehavior::PreDefinedValues | ValueCompletionBehavior::AppendEquationSign);
    tagPosValueArg.setPreDefinedCompletionValues("front back current");
    tagPosValueArg.setImplicit(true);
    tagPosValueArg.setRequired(true);
    tagPosArg.setSubArguments({ &tagPosValueArg, &forceTagPosArg });
    indexPosValueArg.setPreDefinedCompletionValues("front back current");
    indexPosValueArg.setImplicit(true);
    indexPosValueArg.setRequired(true);
    indexPosArg.setExample(PROJECT_NAME " set comment=\"with faststart\" --index-pos front --force --layout-only -f /some/dir/*.m4a");
    indexPosArg.setSubArguments({ &indexPosValueArg, &forceIndexPosArg });
    valuesArg.setRequiredValueCount(Argument::varValueCount);
    valuesArg.setImplicit(true);
    valuesArg.setPreDefinedCompletionValues(Cli::fieldNamesForSet);
    valuesArg.setValueCompletionBehavior(ValueCompletionBehavior::PreDefinedValues | ValueCompletionBehavior::AppendEquationSign);
    outputFilesArg.setRequiredValueCount(Argument::varValueCount);
    setTagInfoArg.setCallback(std::bind(Cli::setTagInfo, std::cref(*this)));
    setTagInfoArg.setExample(PROJECT_NAME
        " set title=\"Title of \"{1st,2nd,3rd}\" file\" title=\"Title of \"{4..16}\"th file\" album=\"The Album\" -f /some/dir/*.m4a\n" PROJECT_NAME
        " set title=\"Title for track \"{1..25} album=\"Album with 25 tracks on one disk\" track+=1/25 disk=1/1 -f *.m4a" PROJECT_NAME
        " set mkv:FOO=bar1 mp4:©foo=bar2 -f file.mkv file.m4a\n" PROJECT_NAME
        " set title0=\"Title for both files\" album1=\"Album for 2nd file\" -f file1.ogg file2.mp3\n" PROJECT_NAME
        " set target-level=30 target-tracks=3134325680 title=\"Title for track 3134325680\" \\\n"
        "             --remove-target target-level=50 --remove-target target-level=30 -f file.mka\n" PROJECT_NAME
        " set mkv:CUSTOM_FIELD=\"Matroska-only\" vorbis:CUSTOM_FIELD=\"Vorbis-only\" mp4:©ust=\"MP4-only\" \\\n"
        "             -f file.mkv file.ogg file.m4a\n"
        "For more examples and detailed descriptions see " APP_URL "#writing-tags");
    setTagInfoArg.setSubArguments({ &valuesArg, &filesArg, &docTitleArg, &removeOtherFieldsArg, &treatUnknownFilesAsMp3FilesArg, &id3v1UsageArg,
        &id3v2UsageArg, &id3InitOnCreateArg, &id3TransferOnRemovalArg, &mergeMultipleSuccessiveTagsArg, &id3v2VersionArg, &encodingArg,
        &removeTargetArg, &addAttachmentArg, &updateAttachmentArg, &removeAttachmentArg, &removeExistingAttachmentsArg, &minPaddingArg,
        &maxPaddingArg, &prefPaddingArg, &tagPosArg, &indexPosArg, &forceRewriteArg, &backupDirArg, &layoutOnlyArg, &preserveModificationTimeArg,
        &verboseArg, &quietArg, &outputFilesArg });
}

} // namespace Cli

int main(int argc, char *argv[])
{
    // setup argument parser
    ArgumentParser parser;
    CMD_UTILS_CONVERT_ARGS_TO_UTF8;
    SET_APPLICATION_INFO;
    QT_CONFIG_ARGUMENTS qtConfigArgs;
    HelpArgument helpArg(parser);
    NoColorArgument noColorArg;
    ConfigValueArgument timeSpanFormatArg("time-span-format", '\0', "specifies the output format for time spans", { "measures/colons/seconds" });
    timeSpanFormatArg.setPreDefinedCompletionValues("measures colons seconds");
    // verbose option
    ConfigValueArgument verboseArg("verbose", 'v', "be verbose, print debug and info messages");
    // input/output file/files
    ConfigValueArgument fileArg("file", 'f', "specifies the path of the file to be opened", { "path" });
    ConfigValueArgument defaultFileArg(fileArg);
    defaultFileArg.setImplicit(true);
    fileArg.setRequired(true);
    ConfigValueArgument filesArg("files", 'f', "specifies the path of the file(s) to be opened", { "path 1", "path 2" });
    filesArg.setRequiredValueCount(Argument::varValueCount);
    ConfigValueArgument outputFileArg("output-file", 'o', "specifies the path of the output file", { "path" });
    // print field names
    OperationArgument printFieldNamesArg("print-field-names", '\0', "lists available field names, track attribute names and modifier");
    printFieldNamesArg.setCallback(Cli::printFieldNames);
    // display general file info
    ConfigValueArgument validateArg(
        "validate", 'c', "validates the file integrity as accurately as possible; the structure of the file will be parsed completely");
    OperationArgument displayFileInfoArg("info", 'i', "displays general file information", PROJECT_NAME " info -f /some/dir/*.m4a");
    displayFileInfoArg.setCallback(std::bind(Cli::displayFileInfo, _1, std::cref(filesArg), std::cref(verboseArg), std::cref(validateArg)));
    displayFileInfoArg.setSubArguments({ &filesArg, &validateArg, &verboseArg });
    // display tag info
    ConfigValueArgument fieldsArg("fields", 'n', "specifies the field names to be displayed", { "title", "album", "artist", "trackpos" });
    fieldsArg.setRequiredValueCount(Argument::varValueCount);
    fieldsArg.setPreDefinedCompletionValues(Cli::fieldNames);
    fieldsArg.setImplicit(true);
    OperationArgument displayTagInfoArg("get", 'g', "displays the values of all specified tag fields (displays all fields if none specified)",
        PROJECT_NAME " get title album artist -f /some/dir/*.m4a");
    ConfigValueArgument showUnsupportedArg("show-unsupported", 'u', "shows unsupported fields (has only effect when no field names specified)");
    displayTagInfoArg.setCallback(
        std::bind(Cli::displayTagInfo, std::cref(fieldsArg), std::cref(showUnsupportedArg), std::cref(filesArg), std::cref(verboseArg)));
    displayTagInfoArg.setSubArguments({ &fieldsArg, &showUnsupportedArg, &filesArg, &verboseArg });
    // set tag info
    Cli::SetTagInfoArgs setTagInfoArgs(filesArg, verboseArg);
    // extract cover
    ConfigValueArgument fieldArg("field", 'n', "specifies the field to be extracted", { "field name" });
    fieldArg.setImplicit(true);
    ConfigValueArgument attachmentArg("attachment", 'a', "specifies the attachment to be extracted", { "id=..." });
    OperationArgument extractFieldArg("extract", 'e',
        "saves the value of the specified field (eg. cover or other binary field) or attachment to the specified file or writes it to stdout if no "
        "output file has been specified");
    extractFieldArg.setSubArguments({ &fieldArg, &attachmentArg, &fileArg, &outputFileArg, &verboseArg });
    extractFieldArg.setExample(PROJECT_NAME " extract cover --output-file the-cover.jpg --file some-file.opus");
    extractFieldArg.setCallback(std::bind(
        Cli::extractField, std::cref(fieldArg), std::cref(attachmentArg), std::cref(fileArg), std::cref(outputFileArg), std::cref(verboseArg)));
    // export to JSON
    ConfigValueArgument prettyArg("pretty", '\0', "prints with indentation and spacing");
    OperationArgument exportArg("export", 'j', "exports the tag information for the specified files to JSON");
    exportArg.setSubArguments({ &filesArg, &prettyArg });
    exportArg.setCallback(std::bind(Cli::exportToJson, _1, std::cref(filesArg), std::cref(prettyArg)));
    // file info
    OperationArgument genInfoArg("html-info", '\0', "generates technical information about the specified file as HTML document");
    genInfoArg.setSubArguments({ &fileArg, &validateArg, &outputFileArg });
    genInfoArg.setCallback(std::bind(Cli::generateFileInfo, _1, std::cref(fileArg), std::cref(outputFileArg), std::cref(validateArg)));
    // renaming utility
    ConfigValueArgument renamingUtilityArg("renaming-utility", '\0', "launches the renaming utility instead of the main GUI");
    // set arguments to parser
    qtConfigArgs.qtWidgetsGuiArg().setAbbreviation('\0');
    qtConfigArgs.qtWidgetsGuiArg().addSubArgument(&defaultFileArg);
    qtConfigArgs.qtWidgetsGuiArg().addSubArgument(&renamingUtilityArg);
    parser.setMainArguments({ &qtConfigArgs.qtWidgetsGuiArg(), &printFieldNamesArg, &displayFileInfoArg, &displayTagInfoArg,
        &setTagInfoArgs.setTagInfoArg, &extractFieldArg, &exportArg, &genInfoArg, &timeSpanFormatArg, &noColorArg, &helpArg });
    // parse given arguments
    parser.parseArgs(argc, argv, ParseArgumentBehavior::CheckConstraints | ParseArgumentBehavior::ExitOnFailure);

    // start GUI/CLI
    if (qtConfigArgs.areQtGuiArgsPresent()) {
#if defined(TAGEDITOR_GUI_QTWIDGETS)
        return QtGui::runWidgetsGui(argc, argv, qtConfigArgs,
            defaultFileArg.isPresent() && !defaultFileArg.values().empty() ? fromNativeFileName(defaultFileArg.values().front()) : QString(),
            renamingUtilityArg.isPresent());
#else
        CMD_UTILS_START_CONSOLE;
        cerr << EscapeCodes::Phrases::Error
             << "The tag editor has not been built with Qt widgets GUI support. Use --help to show the options of the CLI." << endl;
#endif
    } else {
        // apply general CLI config (concerns currently only the default time span output format)
        Cli::applyGeneralConfig(timeSpanFormatArg);
        // invoke specified CLI operation via callbacks
        parser.invokeCallbacks();
    }
    return 0;
}
