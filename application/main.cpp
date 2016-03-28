#include "./main.h"
#include "./knownfieldmodel.h"

#include "../cli/mainfeatures.h"
#if defined(GUI_QTWIDGETS)
# include "../gui/initiate.h"
#elif defined(GUI_QTQUICK)
#endif

// include configuration from separate header file when building with CMake
#ifndef APP_METADATA_AVAIL
#include "resources/config.h"
#endif

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
    docTitleArg("doc-title", "d", "specifies the document title (has no affect if not supported by the container)"),
    removeOtherFieldsArg("remove-other-fields", string(), "if present ALL unspecified tag fields will be removed (to remove a specific field use eg. \"album=\")"),
    treatUnknownFilesAsMp3FilesArg("treat-unknown-as-mp3", string(), "if present unknown files will be treatet as MP3 files"),
    id3v1UsageArg("id3v1-usage", string(), "specifies the ID3v1 usage (only used when already present by default); only relevant when dealing with MP3 files (or files treated as such)"),
    id3v2UsageArg("id3v2-usage", string(), "specifies the ID3v2 usage (always used by default); only relevant when dealing with MP3 files (or files treated as such)"),
    mergeMultipleSuccessiveTagsArg("merge-successive-tags", string(), "if present multiple successive ID3v2 tags will be merged"),
    id3v2VersionArg("id3v2-version", string(), "forces a specific ID3v2 version to be used; only relevant when ID3v2 is used"),
    encodingArg("encoding", string(), "specifies the preferred encoding"),
    removeTargetsArg("remove-targets", string(), "removes all tags with the specified targets (which must be separated by \",\")"),
    attachmentsArg("attachments", string(), "specifies attachments to be added/updated/removed (multiple attachments must be separated by \",\""),
    removeExistingAttachmentsArg("remove-existing-attachments", "ra", "specifies names/IDs of existing attachments to be removed"),
    minPaddingArg("min-padding", string(), "specifies the minimum padding before the media data"),
    maxPaddingArg("max-padding", string(), "specifies the maximum padding before the media data"),
    prefPaddingArg("preferred-padding", string(), "specifies the preferred padding before the media data"),
    tagPosArg("tag-pos", string(), "specifies the preferred tag position"),
    forceTagPosArg("force-tag-pos", string(), "forces the specified tag postion to be used even if it requires the file to be rewritten"),
    indexPosArg("index-pos", string(), "specifies the preferred index position"),
    forceIndexPosArg("force-index-pos", string(), "forces the specified index postion to be used even if it requires the file to be rewritten"),
    forceRewriteArg("force-rewrite", string(), "forces the file to rewritten from the scratch"),
    setTagInfoArg("set-tag-info", "set", "sets the values of all specified tag fields")
{
    docTitleArg.setCombinable(true);
    docTitleArg.setRequiredValueCount(-1);
    docTitleArg.setValueNames({"title of first segment", "title of second segment"});
    removeOtherFieldsArg.setCombinable(true);
    treatUnknownFilesAsMp3FilesArg.setCombinable(true);
    id3v1UsageArg.setRequiredValueCount(1);
    id3v1UsageArg.setValueNames({"always/keepexisting/never"});
    id3v1UsageArg.setCombinable(true);
    id3v2UsageArg.setRequiredValueCount(1);
    id3v2UsageArg.setValueNames({"always/keepexisting/never"});
    id3v2UsageArg.setCombinable(true);
    mergeMultipleSuccessiveTagsArg.setCombinable(true);
    id3v2VersionArg.setRequiredValueCount(1);
    id3v2VersionArg.setValueNames({"ID3v2 version"});
    id3v2VersionArg.setCombinable(true);
    encodingArg.setRequiredValueCount(1);
    encodingArg.setValueNames({"latin1/utf8/utf16le/utf16be"});
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
    tagPosArg.setRequiredValueCount(1);
    tagPosArg.setValueNames({"front/back/current"});
    tagPosArg.setCombinable(true);
    tagPosArg.setSecondaryArguments({&forceTagPosArg});
    indexPosArg.setRequiredValueCount(1);
    indexPosArg.setValueNames({"front/back/current"});
    indexPosArg.setCombinable(true);
    indexPosArg.setSecondaryArguments({&forceIndexPosArg});
    forceRewriteArg.setCombinable(true);
    setTagInfoArg.setDenotesOperation(true);
    setTagInfoArg.setCallback(std::bind(Cli::setTagInfo, _1, std::cref(*this)));
    setTagInfoArg.setRequiredValueCount(-1);
    setTagInfoArg.setValueNames({"title=foo", "album=bar", "cover=/path/to/file"});
    setTagInfoArg.setSecondaryArguments({&filesArg, &docTitleArg, &removeOtherFieldsArg, &treatUnknownFilesAsMp3FilesArg, &id3v1UsageArg, &id3v2UsageArg,
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
    Argument verboseArg("verbose", "v", "be verbose");
    verboseArg.setCombinable(true);
    // recursive option
    Argument recursiveArg("recursive", "r", "includes subdirectories");
    // input/output file/files
    Argument filesArg("files", "f", "specifies the path of the file(s) to be opened");
    filesArg.setValueNames({"path 1", "path 2"});
    filesArg.setRequiredValueCount(-1);
    filesArg.setRequired(false);
    filesArg.setImplicit(true);
    qtConfigArgs.qtWidgetsGuiArg().addSecondaryArgument(&filesArg);
    Argument fileArg("file", "if", "specifies the path of the input file");
    fileArg.setValueNames({"path"});
    fileArg.setRequiredValueCount(1);
    fileArg.setRequired(true);
    Argument outputFileArg("output-file", "of", "specifies the path of the output file");
    outputFileArg.setValueNames({"path"});
    outputFileArg.setRequiredValueCount(1);
    outputFileArg.setRequired(true);
    outputFileArg.setCombinable(true);
    // print field names
    Argument printFieldNamesArg("print-field-names", string(), "prints available field names");
    printFieldNamesArg.setCallback(Cli::printFieldNames);
    // display general file info
    Argument displayFileInfoArg("display-file-info", "info", "displays general file information");
    displayFileInfoArg.setDenotesOperation(true);
    displayFileInfoArg.setCallback(std::bind(Cli::displayFileInfo, _1, std::cref(filesArg), std::cref(verboseArg)));
    displayFileInfoArg.setSecondaryArguments({&filesArg, &verboseArg});
    // display tag info
    Argument displayTagInfoArg("display-tag-info", "get", "displays the values of all specified tag fields (displays all fields if none specified)");
    displayTagInfoArg.setDenotesOperation(true);
    displayTagInfoArg.setCallback(std::bind(Cli::displayTagInfo, _1, std::cref(filesArg), std::cref(verboseArg)));
    displayTagInfoArg.setRequiredValueCount(-1);
    displayTagInfoArg.setValueNames({"title", "album", "artist", "trackpos"});
    displayTagInfoArg.setSecondaryArguments({&filesArg, &verboseArg});
    // set tag info
    Cli::SetTagInfoArgs setTagInfoArgs(filesArg, verboseArg);
    // extract cover
    Argument extractFieldArg("extract", "ext", "extracts the specified field from the specified file");
    extractFieldArg.setRequiredValueCount(1);
    extractFieldArg.setValueNames({"field"});
    extractFieldArg.setSecondaryArguments({&fileArg, &outputFileArg, &verboseArg});
    extractFieldArg.setDenotesOperation(true);
    extractFieldArg.setCallback(std::bind(Cli::extractField, _1, std::cref(fileArg), std::cref(outputFileArg), std::cref(verboseArg)));
    // file info
    Argument validateArg("validate", "c", "validates the file integrity as accurately as possible; the structure of the file will be parsed completely");
    validateArg.setDenotesOperation(true);
    validateArg.setCombinable(true);
    Argument genInfoArg("html-info", string(), "generates technical information about the specified file as HTML document");
    genInfoArg.setDenotesOperation(true);
    genInfoArg.setSecondaryArguments({&fileArg, &validateArg, &outputFileArg});
    genInfoArg.setCallback(std::bind(Cli::generateFileInfo, _1, std::cref(fileArg), std::cref(outputFileArg), std::cref(validateArg)));
    // remove backup files
    Argument remBackupFilesArg("remove-backup-files", string(), "removes all files with \".bak\" suffix in the given directory and in subdirectories if recursive option is present");
    remBackupFilesArg.setDenotesOperation(true);
    remBackupFilesArg.setCallback(std::bind(Cli::removeBackupFiles, _1, std::cref(recursiveArg)));
    remBackupFilesArg.setValueNames({"directory"});
    remBackupFilesArg.setRequiredValueCount(1);
    remBackupFilesArg.setSecondaryArguments({&recursiveArg});
    // renaming utility
    Argument renamingUtilityArg("renaming-utility", string(), "launches the renaming utility instead of the main GUI");
    renamingUtilityArg.setCombinable(true);
    // set arguments to parser
    qtConfigArgs.qtWidgetsGuiArg().addSecondaryArgument(&filesArg);
    qtConfigArgs.qtWidgetsGuiArg().addSecondaryArgument(&renamingUtilityArg);
    parser.setMainArguments({&printFieldNamesArg, &displayFileInfoArg, &displayTagInfoArg, &setTagInfoArgs.setTagInfoArg, &extractFieldArg, &genInfoArg, &remBackupFilesArg, &qtConfigArgs.qtWidgetsGuiArg(), &helpArg});
    // parse given arguments
    try {
        parser.parseArgs(argc, argv);
        if(qtConfigArgs.areQtGuiArgsPresent()) {
#ifdef GUI_QTWIDGETS
            return QtGui::runWidgetsGui(argc, argv, qtConfigArgs, filesArg.values().empty() ? QString() : QString::fromLocal8Bit(filesArg.values().front().data()), renamingUtilityArg.isPresent());
#else
            CMD_UTILS_START_CONSOLE;
            cout << "Application has not been build with Qt widgets GUI support." << endl;
#endif
        }
    } catch(const Failure &ex) {
        CMD_UTILS_START_CONSOLE;
        cout << "Unable to parse arguments. " << ex.what() << "\nSee --help for available commands." << endl;
    }    
    return 0;
}

