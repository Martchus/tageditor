#include "../cli/mainfeatures.h"
#if defined(TAGEDITOR_GUI_QTWIDGETS)
# include "../gui/initiate.h"
# include "./knownfieldmodel.h"
#endif

#include "resources/config.h"

#if defined(TAGEDITOR_GUI_QTWIDGETS) || defined(TAGEDITOR_GUI_QTQUICK)
# include <qtutilities/resources/qtconfigarguments.h>
# include <qtutilities/misc/conversion.h>
#else
# include <c++utilities/application/fakeqtconfigarguments.h>
#endif
#include <c++utilities/application/commandlineutils.h>
#include <c++utilities/application/failure.h>

#include <iostream>
#include <memory>
#include <functional>

using namespace std;
using namespace std::placeholders;
using namespace ApplicationUtilities;

namespace Cli {

SetTagInfoArgs::SetTagInfoArgs(Argument &filesArg, Argument &verboseArg) :
    filesArg(filesArg),
    verboseArg(verboseArg),
    docTitleArg("doc-title", 'd', "specifies the document title (has no affect if not supported by the container)"),
    removeOtherFieldsArg("remove-other-fields", '\0', "removes ALL fields where no value has been provided for (to remove a specific field use eg. \"album=\")"),
    treatUnknownFilesAsMp3FilesArg("treat-unknown-as-mp3", '\0', "if present unknown files will be treatet as MP3 files"),
    id3v1UsageArg("id3v1-usage", '\0', "specifies the ID3v1 usage (only used when already present by default); only relevant when dealing with MP3 files (or files treated as such)"),
    id3v2UsageArg("id3v2-usage", '\0', "specifies the ID3v2 usage (always used by default); only relevant when dealing with MP3 files (or files treated as such)"),
    mergeMultipleSuccessiveTagsArg("merge-successive-tags", '\0', "if present multiple successive ID3v2 tags will be merged"),
    id3v2VersionArg("id3v2-version", '\0', "forces a specific ID3v2 version to be used; only relevant when ID3v2 is used"),
    id3InitOnCreateArg("id3-init-on-create", '\0', "indicates whether to initialize newly created ID3 tags (according to specified usage) with the values of the already present ID3 tags"),
    id3TransferOnRemovalArg("id3-transfer-on-removal", '\0', "indicates whether values of removed ID3 tags (according to specified usage) should be transfered to remaining ID3 tags (no values will be overwritten)"),
    encodingArg("encoding", '\0', "specifies the preferred encoding"),
    removeTargetArg("remove-target", '\0', "removes all tags with the specified target"),
    addAttachmentArg("add-attachment", '\0', "adds a new attachment"),
    updateAttachmentArg("update-attachment", '\0', "updates an existing attachment"),
    removeAttachmentArg("remove-attachment", '\0', "removes an existing attachment"),
    removeExistingAttachmentsArg("remove-existing-attachments", 'r', "removes ALL existing attachments (to remove a specific attachment use --remove-attachment)"),
    minPaddingArg("min-padding", '\0', "specifies the minimum padding before the media data (enforces rewriting the file is the padding would be less)"),
    maxPaddingArg("max-padding", '\0', "specifies the maximum padding before the media data (enforces rewriting the file is the padding would be more)"),
    prefPaddingArg("preferred-padding", '\0', "specifies the preferred padding before the media data (used when the file is rewritten)"),
    tagPosValueArg("value", '\0', "specifies the position, either front, back or current"),
    forceTagPosArg("force", '\0', "forces the specified position even if the file needs to be rewritten"),
    tagPosArg("tag-pos", '\0', "specifies the preferred tag position"),
    indexPosValueArg("value", '\0', "specifies the position, either front, back or current"),
    forceIndexPosArg("force", '\0', "forces the specified position even if the file needs to be rewritten"),
    indexPosArg("index-pos", '\0', "specifies the preferred index position"),
    forceRewriteArg("force-rewrite", '\0', "forces the file to rewritten from the scratch which ensures a backup is created and the preferred padding is used"),
    valuesArg("values", 'n', "specifies the values to be set"),
    outputFilesArg("output-files", 'o', "specifies the output files; if present, the files specified with --files will not be modified"),
    setTagInfoArg("set", 's', "sets the specified tag information and attachments")
{
    docTitleArg.setCombinable(true);
    docTitleArg.setRequiredValueCount(Argument::varValueCount);
    docTitleArg.setValueNames({"title of first segment", "title of second segment"});
    removeOtherFieldsArg.setCombinable(true);
    treatUnknownFilesAsMp3FilesArg.setCombinable(true);
    id3v1UsageArg.setRequiredValueCount(1);
    id3v1UsageArg.setValueNames({"always/keepexisting/never"});
    id3v1UsageArg.setPreDefinedCompletionValues("always keepexisting never");
    id3v1UsageArg.setCombinable(true);
    id3v2UsageArg.setRequiredValueCount(1);
    id3v2UsageArg.setValueNames({"always/keepexisting/never"});
    id3v2UsageArg.setPreDefinedCompletionValues("always keepexisting never");
    id3v2UsageArg.setCombinable(true);
    mergeMultipleSuccessiveTagsArg.setCombinable(true);
    id3v2VersionArg.setRequiredValueCount(1);
    id3v2VersionArg.setValueNames({"1/2/3/4"});
    id3v2VersionArg.setCombinable(true);
    id3v2VersionArg.setPreDefinedCompletionValues("1 2 3 4");
    id3InitOnCreateArg.setCombinable(true);
    id3TransferOnRemovalArg.setCombinable(true);
    encodingArg.setRequiredValueCount(1);
    encodingArg.setValueNames({"latin1/utf8/utf16le/utf16be"});
    encodingArg.setPreDefinedCompletionValues("latin1 utf8 utf16le utf16be");
    encodingArg.setCombinable(true);
    removeTargetArg.setRequiredValueCount(Argument::varValueCount);
    removeTargetArg.setValueNames({});
    removeTargetArg.setCombinable(true);
    removeTargetArg.setConstraints(0, Argument::varValueCount);
    addAttachmentArg.setRequiredValueCount(Argument::varValueCount);
    addAttachmentArg.setValueNames({"path=some/file", "name=Some name", "desc=Some desc", "mime=mime/type"});
    addAttachmentArg.setCombinable(true);
    addAttachmentArg.setConstraints(0, Argument::varValueCount);
    addAttachmentArg.setValueCompletionBehavior(ValueCompletionBehavior::PreDefinedValues | ValueCompletionBehavior::AppendEquationSign);
    addAttachmentArg.setPreDefinedCompletionValues("name id path desc mime");
    updateAttachmentArg.setRequiredValueCount(Argument::varValueCount);
    updateAttachmentArg.setValueNames({"path=some/file", "name=Some name", "desc=Some desc", "mime=mime/type"});
    updateAttachmentArg.setCombinable(true);
    updateAttachmentArg.setConstraints(0, Argument::varValueCount);
    updateAttachmentArg.setValueCompletionBehavior(ValueCompletionBehavior::PreDefinedValues | ValueCompletionBehavior::AppendEquationSign);
    updateAttachmentArg.setPreDefinedCompletionValues("name id path desc mime");
    removeAttachmentArg.setRequiredValueCount(1);
    removeAttachmentArg.setValueNames({"name=to_remove"});
    removeAttachmentArg.setCombinable(true);
    removeAttachmentArg.setConstraints(0, Argument::varValueCount);
    removeAttachmentArg.setPreDefinedCompletionValues("name id");
    removeAttachmentArg.setValueCompletionBehavior(ValueCompletionBehavior::PreDefinedValues | ValueCompletionBehavior::AppendEquationSign);
    removeExistingAttachmentsArg.setCombinable(true);
    minPaddingArg.setRequiredValueCount(1);
    minPaddingArg.setValueNames({"min padding in byte"});
    minPaddingArg.setCombinable(true);
    maxPaddingArg.setRequiredValueCount(1);
    maxPaddingArg.setValueNames({"min padding in byte"});
    maxPaddingArg.setCombinable(true);
    prefPaddingArg.setRequiredValueCount(1);
    prefPaddingArg.setValueNames({"preferred padding in byte"});
    prefPaddingArg.setCombinable(true);
    tagPosValueArg.setRequiredValueCount(1);
    tagPosValueArg.setValueNames({"front/back/current"});
    tagPosValueArg.setPreDefinedCompletionValues("front back current");
    tagPosValueArg.setImplicit(true);
    tagPosValueArg.setRequired(true);
    forceTagPosArg.setCombinable(true);    
    tagPosArg.setCombinable(true);
    tagPosArg.setSubArguments({&tagPosValueArg, &forceTagPosArg});
    forceIndexPosArg.setCombinable(true);
    indexPosValueArg.setRequiredValueCount(1);
    indexPosValueArg.setValueNames({"front/back/current"});
    indexPosValueArg.setPreDefinedCompletionValues("front back current");
    indexPosValueArg.setImplicit(true);
    indexPosValueArg.setRequired(true);
    indexPosArg.setCombinable(true);
    indexPosArg.setExample(PROJECT_NAME " set comment=\"with faststart (enforced)\" --index-pos front --force -f /some/dir/*.m4a");
    indexPosArg.setSubArguments({&indexPosValueArg, &forceIndexPosArg});
    forceRewriteArg.setCombinable(true);
    valuesArg.setValueNames({"title=foo", "album=bar", "cover=/path/to/file"});
    valuesArg.setRequiredValueCount(Argument::varValueCount);
    valuesArg.setImplicit(true);
    valuesArg.setPreDefinedCompletionValues(Cli::fieldNamesForSet);
    valuesArg.setValueCompletionBehavior(ValueCompletionBehavior::PreDefinedValues | ValueCompletionBehavior::AppendEquationSign);
    outputFilesArg.setValueNames({"path 1", "path 2"});
    outputFilesArg.setRequiredValueCount(Argument::varValueCount);
    outputFilesArg.setCombinable(true);
    setTagInfoArg.setDenotesOperation(true);
    setTagInfoArg.setCallback(std::bind(Cli::setTagInfo, std::cref(*this)));
    setTagInfoArg.setExample(PROJECT_NAME " set title=\"Title of \"{1st,2nd,3rd}\" file\" title=\"Title of \"{4..16}\"th file\" album=\"The Album\" -f /some/dir/*.m4a\n"
                             PROJECT_NAME " set mkv:FOO=bar1 mp4:©foo=bar2 -f file.mkv file.m4a\n"
                             PROJECT_NAME " set title0=\"Title for both files\" album1=\"Album for 2nd file\" -f file1.ogg file2.mp3\n"
                             PROJECT_NAME " set target-level=30 target-tracks=3134325680 title=\"Title for track 3134325680\" \\\n"
                                          "             --remove-targets target-level=50 , target-level=30 -f file.mka\n"
                             "For more examples and detailed descriptions see " APP_URL "#writing-tags");
    setTagInfoArg.setSubArguments({&valuesArg, &filesArg, &docTitleArg, &removeOtherFieldsArg, &treatUnknownFilesAsMp3FilesArg, &id3v1UsageArg, &id3v2UsageArg, &id3InitOnCreateArg, &id3TransferOnRemovalArg,
                                         &mergeMultipleSuccessiveTagsArg, &id3v2VersionArg, &encodingArg, &removeTargetArg, &addAttachmentArg, &updateAttachmentArg, &removeAttachmentArg,
                                         &removeExistingAttachmentsArg, &minPaddingArg, &maxPaddingArg, &prefPaddingArg, &tagPosArg,
                                         &indexPosArg, &forceRewriteArg, &verboseArg, &outputFilesArg});
}

}

int main(int argc, char *argv[])
{
    // setup argument parser
    ArgumentParser parser;
    CMD_UTILS_CONVERT_ARGS_TO_UTF8;
    SET_APPLICATION_INFO;
    QT_CONFIG_ARGUMENTS qtConfigArgs;
    HelpArgument helpArg(parser);
    NoColorArgument noColorArg;
    // verbose option
    Argument verboseArg("verbose", 'v', "be verbose");
    verboseArg.setCombinable(true);
    // input/output file/files
    Argument fileArg("file", 'f', "specifies the path of the file to be opened");
    fileArg.setValueNames({"path"});
    fileArg.setRequiredValueCount(1);
    fileArg.setCombinable(true);
    Argument defaultFileArg(fileArg);
    defaultFileArg.setImplicit(true);
    fileArg.setRequired(true);
    Argument filesArg("files", 'f', "specifies the path of the file(s) to be opened");
    filesArg.setValueNames({"path 1", "path 2"});
    filesArg.setRequiredValueCount(Argument::varValueCount);
    filesArg.setCombinable(true);
    Argument outputFileArg("output-file", 'o', "specifies the path of the output file");
    outputFileArg.setValueNames({"path"});
    outputFileArg.setRequiredValueCount(1);
    outputFileArg.setCombinable(true);
    // print field names
    Argument printFieldNamesArg("print-field-names", '\0', "prints available field names");
    printFieldNamesArg.setCallback(Cli::printFieldNames);
    // display general file info
    Argument displayFileInfoArg("info", 'i', "displays general file information");
    displayFileInfoArg.setDenotesOperation(true);
    displayFileInfoArg.setExample(PROJECT_NAME " info -f /some/dir/*.m4a");
    displayFileInfoArg.setCallback(std::bind(Cli::displayFileInfo, _1, std::cref(filesArg), std::cref(verboseArg)));
    displayFileInfoArg.setSubArguments({&filesArg, &verboseArg});
    // display tag info
    Argument fieldsArg("fields", 'n', "specifies the field names to be displayed");
    fieldsArg.setValueNames({"title", "album", "artist", "trackpos"});
    fieldsArg.setRequiredValueCount(Argument::varValueCount);
    fieldsArg.setPreDefinedCompletionValues(Cli::fieldNames);
    fieldsArg.setImplicit(true);
    Argument displayTagInfoArg("get", 'g', "displays the values of all specified tag fields (displays all fields if none specified)");
    displayTagInfoArg.setDenotesOperation(true);
    displayTagInfoArg.setCallback(std::bind(Cli::displayTagInfo, std::cref(fieldsArg), std::cref(filesArg), std::cref(verboseArg)));
    displayTagInfoArg.setExample(PROJECT_NAME " get title album artist -f /some/dir/*.m4a");
    displayTagInfoArg.setSubArguments({&fieldsArg, &filesArg, &verboseArg});
    // set tag info
    Cli::SetTagInfoArgs setTagInfoArgs(filesArg, verboseArg);
    // extract cover
    Argument fieldArg("field", 'n', "specifies the field to be extracted");
    fieldArg.setValueNames({"field name"});
    fieldArg.setRequiredValueCount(1);
    fieldArg.setImplicit(true);
    Argument attachmentArg("attachment", 'a', "specifies the attachment to be extracted");
    attachmentArg.setValueNames({"id=..."});
    attachmentArg.setRequiredValueCount(1);
    Argument extractFieldArg("extract", 'e', "saves the value of the specified field (eg. cover or other binary field) or attachment to the specified file or writes it to stdout if no output file has been specified");
    extractFieldArg.setSubArguments({&fieldArg, &attachmentArg, &fileArg, &outputFileArg, &verboseArg});
    extractFieldArg.setDenotesOperation(true);
    extractFieldArg.setCallback(std::bind(Cli::extractField, std::cref(fieldArg), std::cref(attachmentArg), std::cref(fileArg), std::cref(outputFileArg), std::cref(verboseArg)));
    // file info
    Argument validateArg("validate", 'c', "validates the file integrity as accurately as possible; the structure of the file will be parsed completely");
    validateArg.setCombinable(true);
    Argument genInfoArg("html-info", '\0', "generates technical information about the specified file as HTML document");
    genInfoArg.setDenotesOperation(true);
    genInfoArg.setSubArguments({&fileArg, &validateArg, &outputFileArg});
    genInfoArg.setCallback(std::bind(Cli::generateFileInfo, _1, std::cref(fileArg), std::cref(outputFileArg), std::cref(validateArg)));
    // renaming utility
    Argument renamingUtilityArg("renaming-utility", '\0', "launches the renaming utility instead of the main GUI");
    renamingUtilityArg.setCombinable(true);
    // set arguments to parser
    qtConfigArgs.qtWidgetsGuiArg().setAbbreviation('\0');
    qtConfigArgs.qtWidgetsGuiArg().addSubArgument(&defaultFileArg);
    qtConfigArgs.qtWidgetsGuiArg().addSubArgument(&renamingUtilityArg);
    parser.setMainArguments({&qtConfigArgs.qtWidgetsGuiArg(), &printFieldNamesArg, &displayFileInfoArg, &displayTagInfoArg, &setTagInfoArgs.setTagInfoArg, &extractFieldArg, &genInfoArg, &noColorArg, &helpArg});
    // parse given arguments
    parser.parseArgsOrExit(argc, argv);
    if(qtConfigArgs.areQtGuiArgsPresent()) {
#if defined(TAGEDITOR_GUI_QTWIDGETS)
        return QtGui::runWidgetsGui(argc, argv, qtConfigArgs, defaultFileArg.isPresent() && !defaultFileArg.values().empty() ? ConversionUtilities::fromNativeFileName(defaultFileArg.values().front()) : QString(), renamingUtilityArg.isPresent());
#else
        CMD_UTILS_START_CONSOLE;
        cerr << "Application has not been build with Qt widgets GUI support." << endl;
#endif
    }
    return 0;
}

