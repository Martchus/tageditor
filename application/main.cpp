#include "./main.h"
#include "./knownfieldmodel.h"

#include "../cli/mainfeatures.h"
#if defined(GUI_QTWIDGETS)
# include "../gui/initiate.h"
#elif defined(GUI_QTQUICK)
#endif

#include "resources/config.h"

#if defined(GUI_QTWIDGETS) || defined(GUI_QTQUICK)
# include <qtutilities/resources/qtconfigarguments.h>
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
    removeOtherFieldsArg("remove-other-fields", '\0', "if present ALL unspecified tag fields will be removed (to remove a specific field use eg. \"album=\")"),
    treatUnknownFilesAsMp3FilesArg("treat-unknown-as-mp3", '\0', "if present unknown files will be treatet as MP3 files"),
    id3v1UsageArg("id3v1-usage", '\0', "specifies the ID3v1 usage (only used when already present by default); only relevant when dealing with MP3 files (or files treated as such)"),
    id3v2UsageArg("id3v2-usage", '\0', "specifies the ID3v2 usage (always used by default); only relevant when dealing with MP3 files (or files treated as such)"),
    mergeMultipleSuccessiveTagsArg("merge-successive-tags", '\0', "if present multiple successive ID3v2 tags will be merged"),
    id3v2VersionArg("id3v2-version", '\0', "forces a specific ID3v2 version to be used; only relevant when ID3v2 is used"),
    encodingArg("encoding", '\0', "specifies the preferred encoding"),
    removeTargetsArg("remove-targets", '\0', "removes all tags with the specified targets (which must be separated by \",\")"),
    attachmentsArg("attachments", '\0', "specifies attachments to be added/updated/removed (multiple attachments must be separated by \",\""),
    removeExistingAttachmentsArg("remove-existing-attachments", 'r', "specifies names/IDs of existing attachments to be removed"),
    minPaddingArg("min-padding", '\0', "specifies the minimum padding before the media data"),
    maxPaddingArg("max-padding", '\0', "specifies the maximum padding before the media data"),
    prefPaddingArg("preferred-padding", '\0', "specifies the preferred padding before the media data"),
    tagPosValueArg("value", 'v', "specifies the position, either front, back or current"),
    forceTagPosArg("force", 'f', "forces the specified position even if the file to be rewritten"),
    tagPosArg("tag-pos", '\0', "specifies the preferred tag position"),
    indexPosValueArg("value", 'v', "specifies the position, either front, back or current"),
    forceIndexPosArg("force", 'f', "forces the specified position even if the file to be rewritten"),
    indexPosArg("index-pos", '\0', "specifies the preferred index position"),
    forceRewriteArg("force-rewrite", '\0', "forces the file to rewritten from the scratch"),
    valuesArg("values", 'n', "specifies the values to be set"),
    setTagInfoArg("set", 's', "sets the values of all specified tag fields")
{
    docTitleArg.setCombinable(true);
    docTitleArg.setRequiredValueCount(-1);
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
    id3v2VersionArg.setValueNames({"ID3v2 version"});
    id3v2VersionArg.setCombinable(true);
    encodingArg.setRequiredValueCount(1);
    encodingArg.setValueNames({"latin1/utf8/utf16le/utf16be"});
    encodingArg.setPreDefinedCompletionValues("latin1 utf8 utf16le utf16be");
    encodingArg.setCombinable(true);
    removeTargetsArg.setRequiredValueCount(-1);
    removeTargetsArg.setValueNames({});
    removeTargetsArg.setCombinable(true);
    attachmentsArg.setRequiredValueCount(-1);
    attachmentsArg.setValueNames({"path=some/file", "name=Some name", "desc=Some desc", "mime=mime/type", ",", "path=another/file"});
    attachmentsArg.setCombinable(true);
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
    forceTagPosArg.setCombinable(true);    
    tagPosArg.setCombinable(true);
    tagPosArg.setSubArguments({&tagPosValueArg, &forceTagPosArg});
    forceIndexPosArg.setCombinable(true);
    indexPosValueArg.setRequiredValueCount(1);
    indexPosValueArg.setValueNames({"front/back/current"});
    indexPosValueArg.setPreDefinedCompletionValues("front back current");
    indexPosValueArg.setImplicit(true);
    indexPosArg.setCombinable(true);
    indexPosArg.setSubArguments({&indexPosValueArg, &forceIndexPosArg});
    forceRewriteArg.setCombinable(true);
    valuesArg.setValueNames({"title=foo", "album=bar", "cover=/path/to/file"});
    valuesArg.setRequiredValueCount(-1);
    valuesArg.setImplicit(true);
    valuesArg.setPreDefinedCompletionValues(Cli::fieldNames);
    valuesArg.setValueCompletionBehavior(ValueCompletionBehavior::PreDefinedValues | ValueCompletionBehavior::AppendEquationSign);
    setTagInfoArg.setDenotesOperation(true);
    setTagInfoArg.setCallback(std::bind(Cli::setTagInfo, std::cref(*this)));
    setTagInfoArg.setSubArguments({&valuesArg, &filesArg, &docTitleArg, &removeOtherFieldsArg, &treatUnknownFilesAsMp3FilesArg, &id3v1UsageArg, &id3v2UsageArg,
                                         &mergeMultipleSuccessiveTagsArg, &id3v2VersionArg, &encodingArg, &removeTargetsArg, &attachmentsArg,
                                         &removeExistingAttachmentsArg, &minPaddingArg, &maxPaddingArg, &prefPaddingArg, &tagPosArg,
                                         &indexPosArg, &forceRewriteArg, &verboseArg});
}

}

int main(int argc, char *argv[])
{
    // setup argument parser
    ArgumentParser parser;
    SET_APPLICATION_INFO;
    QT_CONFIG_ARGUMENTS qtConfigArgs;
    HelpArgument helpArg(parser);
    // verbose option
    Argument verboseArg("verbose", 'v', "be verbose");
    verboseArg.setCombinable(true);
    // recursive option
    Argument recursiveArg("recursive", 'r', "includes subdirectories");
    // input/output file/files
    Argument fileArg("file", 'f', "specifies the path of the file to be opened");
    fileArg.setValueNames({"path"});
    fileArg.setRequiredValueCount(1);
    fileArg.setCombinable(true);
    Argument defaultFileArg(fileArg);
    defaultFileArg.setImplicit(true);
    Argument filesArg("files", 'f', "specifies the path of the file(s) to be opened");
    filesArg.setValueNames({"path 1", "path 2"});
    filesArg.setRequiredValueCount(-1);
    filesArg.setCombinable(true);
    Argument outputFileArg("output-file", 'o', "specifies the path of the output file");
    outputFileArg.setValueNames({"path"});
    outputFileArg.setRequiredValueCount(1);
    outputFileArg.setRequired(true);
    outputFileArg.setCombinable(true);
    // print field names
    Argument printFieldNamesArg("print-field-names", '\0', "prints available field names");
    printFieldNamesArg.setCallback(Cli::printFieldNames);
    // display general file info
    Argument displayFileInfoArg("display-file-info", 'i', "displays general file information");
    displayFileInfoArg.setDenotesOperation(true);
    displayFileInfoArg.setCallback(std::bind(Cli::displayFileInfo, _1, std::cref(filesArg), std::cref(verboseArg)));
    displayFileInfoArg.setSubArguments({&filesArg, &verboseArg});
    // display tag info
    Argument fieldsArg("fields", 'n', "specifies the field names to be displayed");
    fieldsArg.setValueNames({"title", "album", "artist", "trackpos"});
    fieldsArg.setRequiredValueCount(-1);
    fieldsArg.setPreDefinedCompletionValues(Cli::fieldNames);
    fieldsArg.setImplicit(true);
    Argument displayTagInfoArg("get", 'g', "displays the values of all specified tag fields (displays all fields if none specified)");
    displayTagInfoArg.setDenotesOperation(true);
    displayTagInfoArg.setCallback(std::bind(Cli::displayTagInfo, std::cref(fieldsArg), std::cref(filesArg), std::cref(verboseArg)));
    displayTagInfoArg.setSubArguments({&fieldsArg, &filesArg, &verboseArg});
    // set tag info
    Cli::SetTagInfoArgs setTagInfoArgs(filesArg, verboseArg);
    // extract cover
    Argument fieldArg("fields", 'n', "specifies the field to be extracted");
    fieldArg.setValueNames({"field name"});
    fieldArg.setRequiredValueCount(1);
    fieldArg.setImplicit(true);
    Argument extractFieldArg("extract", 'e', "extracts the specified field from the specified file");
    extractFieldArg.setSubArguments({&fieldArg, &fileArg, &outputFileArg, &verboseArg});
    extractFieldArg.setDenotesOperation(true);
    extractFieldArg.setCallback(std::bind(Cli::extractField, std::cref(fieldsArg), std::cref(fileArg), std::cref(outputFileArg), std::cref(verboseArg)));
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
    parser.setMainArguments({&qtConfigArgs.qtWidgetsGuiArg(), &printFieldNamesArg, &displayFileInfoArg, &displayTagInfoArg, &setTagInfoArgs.setTagInfoArg, &extractFieldArg, &genInfoArg, &helpArg});
    // parse given arguments
    try {
        parser.parseArgs(argc, argv);
        if(qtConfigArgs.areQtGuiArgsPresent()) {
#ifdef GUI_QTWIDGETS
            return QtGui::runWidgetsGui(argc, argv, qtConfigArgs, defaultFileArg.isPresent() && !defaultFileArg.values().empty() ? QString::fromLocal8Bit(defaultFileArg.values().front()) : QString(), renamingUtilityArg.isPresent());
#else
            CMD_UTILS_START_CONSOLE;
            cerr << "Application has not been build with Qt widgets GUI support." << endl;
#endif
        }
    } catch(const Failure &ex) {
        CMD_UTILS_START_CONSOLE;
        cerr << "Unable to parse arguments. " << ex.what() << "\nSee --help for available commands." << endl;
    }    
    return 0;
}

