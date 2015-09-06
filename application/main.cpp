#include "./main.h"
#include "./knownfieldmodel.h"

#include "../cli/mainfeatures.h"
#if defined(GUI_QTWIDGETS)
# include "../gui/initiate.h"
#elif defined(GUI_QTQUICK)
#endif

#if defined(GUI_QTWIDGETS) || defined(GUI_QTQUICK)
# include <qtutilities/resources/qtconfigarguments.h>
#else
# include <c++utilities/application/fakeqtconfigarguments.h>
#endif
#include <c++utilities/application/argumentparser.h>
#include <c++utilities/application/commandlineutils.h>
#include <c++utilities/application/failure.h>

#include <iostream>
#include <memory>
#include <functional>

using namespace std;
using namespace std::placeholders;
using namespace ApplicationUtilities;

int main(int argc, char *argv[])
{
    // setup argument parser
    ArgumentParser parser;
    SET_APPLICATION_INFO;
    QT_CONFIG_ARGUMENTS qtConfigArgs;
    HelpArgument helpArg(parser);
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
    Argument displayFileInfoArg("display-file-info", "file-info", "displays general file information");
    displayFileInfoArg.setDenotesOperation(true);
    displayFileInfoArg.setCallback(std::bind(Cli::displayFileInfo, _1, std::cref(filesArg)));
    displayFileInfoArg.setSecondaryArguments({&filesArg});
    // display tag info
    Argument displayTagInfoArg("display-tag-info", "get", "displays the values of all specified tag fields (displays all fields if none specified)");
    displayTagInfoArg.setDenotesOperation(true);
    displayTagInfoArg.setCallback(std::bind(Cli::displayTagInfo, _1, std::cref(filesArg)));
    displayTagInfoArg.setRequiredValueCount(-1);
    displayTagInfoArg.setValueNames({"title", "album", "artist", "trackpos"});
    displayTagInfoArg.setSecondaryArguments({&filesArg});
    // set tag info
    Argument removeOtherFieldsArg("remove-other-fields", string(), "if present ALL unspecified tag fields will be removed; otherwise use eg. \"album=\" to remove a specific field");
    removeOtherFieldsArg.setCombinable(true);
    Argument treatUnknownFilesAsMp3FilesArg("treat-unknown-as-mp3", string(), "if present unknown files will be treatet as MP3 files");
    treatUnknownFilesAsMp3FilesArg.setCombinable(true);
    Argument id3v1UsageArg("id3v1-usage", string(), "specifies the ID3v1 usage (only used when already present by default); only relevant when dealing with MP3 files (or files treated as such)");
    id3v1UsageArg.setRequiredValueCount(1);
    id3v1UsageArg.setValueNames({"always/keepexisting/never"});
    id3v1UsageArg.setCombinable(true);
    Argument id3v2UsageArg("id3v2-usage", string(), "specifies the ID3v2 usage (always used by default); only relevant when dealing with MP3 files (or files treated as such)");
    id3v2UsageArg.setRequiredValueCount(1);
    id3v2UsageArg.setValueNames({"always/keepexisting/never"});
    id3v2UsageArg.setCombinable(true);
    Argument mergeMultipleSuccessiveTagsArg("merge-successive-tags", string(), "if present multiple successive ID3v2 tags will be merged");
    mergeMultipleSuccessiveTagsArg.setCombinable(true);
    Argument id3v2VersionArg("id3v2-version", string(), "forces a specific ID3v2 version to be used; only relevant when ID3v2 is used");
    id3v2VersionArg.setRequiredValueCount(1);
    id3v2VersionArg.setValueNames({"ID3v2 version"});
    id3v2VersionArg.setCombinable(true);
    Argument encodingArg("encoding", string(), "specifies the preferred encoding");
    encodingArg.setRequiredValueCount(1);
    encodingArg.setValueNames({"latin1/utf8/utf16le/utf16be"});
    encodingArg.setCombinable(true);
    Argument setTagInfoArg("set-tag-info", "set", "sets the values of all specified tag fields");
    setTagInfoArg.setDenotesOperation(true);
    setTagInfoArg.setCallback(std::bind(Cli::setTagInfo, _1, std::cref(filesArg), std::cref(removeOtherFieldsArg), std::cref(treatUnknownFilesAsMp3FilesArg), std::cref(id3v1UsageArg), std::cref(id3v2UsageArg), std::cref(mergeMultipleSuccessiveTagsArg), std::cref(id3v2VersionArg), std::cref(encodingArg)));
    setTagInfoArg.setRequiredValueCount(-1);
    setTagInfoArg.setValueNames({"title=foo", "album=bar", "cover=/path/to/file"});
    setTagInfoArg.setSecondaryArguments({&filesArg, &removeOtherFieldsArg, &treatUnknownFilesAsMp3FilesArg, &id3v1UsageArg, &id3v2UsageArg, &mergeMultipleSuccessiveTagsArg, &id3v2VersionArg, &encodingArg});
    // extract cover
    Argument extractFieldArg("extract", "ext", "extracts the specified field from the specified file");
    extractFieldArg.setRequiredValueCount(1);
    extractFieldArg.setValueNames({"field"});
    extractFieldArg.setSecondaryArguments({&fileArg, &outputFileArg});
    extractFieldArg.setDenotesOperation(true);
    extractFieldArg.setCallback(std::bind(Cli::extractField, _1, std::cref(fileArg), std::cref(outputFileArg)));
    // file info
    Argument validateArg("validate", "v", "validates the file integrity as accurately as possible; the structure of the file will be parsed completely");
    validateArg.setDenotesOperation(true);
    validateArg.setCombinable(true);
    Argument genInfoArg("gen-info", "info", "generates technical information about the specified file as HTML document");
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
    parser.setMainArguments({&printFieldNamesArg, &displayFileInfoArg, &displayTagInfoArg, &setTagInfoArg, &extractFieldArg, &genInfoArg, &remBackupFilesArg, &qtConfigArgs.qtWidgetsGuiArg(), &helpArg});
    // parse given arguments
    try {
        parser.parseArgs(argc, argv);
        if(qtConfigArgs.areQtGuiArgsPresent()) {
#ifdef GUI_QTWIDGETS
            return QtGui::runWidgetsGui(argc, argv, qtConfigArgs, fileArg.values().empty() ? QString() : QString::fromLocal8Bit(fileArg.values().front().data()));
#else
            CMD_UTILS_START_CONSOLE;
            cout << "Application has not been build with Qt widgets GUI support." << endl;
#endif
        }
    } catch(Failure &ex) {
        CMD_UTILS_START_CONSOLE;
        cout << "Unable to parse arguments. " << ex.what() << "\nSee --help for available commands." << endl;
    }    
    return 0;
}
