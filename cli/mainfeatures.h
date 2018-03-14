#ifndef MAINFEATURES_H
#define MAINFEATURES_H

#include <c++utilities/application/argumentparser.h>

namespace ApplicationUtilities {
class Argument;
}

namespace Cli {

struct SetTagInfoArgs {
    SetTagInfoArgs(ApplicationUtilities::Argument &filesArg, ApplicationUtilities::Argument &verboseArg);
    ApplicationUtilities::Argument &filesArg;
    ApplicationUtilities::Argument &verboseArg;
    ApplicationUtilities::ConfigValueArgument docTitleArg;
    ApplicationUtilities::ConfigValueArgument removeOtherFieldsArg;
    ApplicationUtilities::ConfigValueArgument treatUnknownFilesAsMp3FilesArg;
    ApplicationUtilities::ConfigValueArgument id3v1UsageArg;
    ApplicationUtilities::ConfigValueArgument id3v2UsageArg;
    ApplicationUtilities::ConfigValueArgument mergeMultipleSuccessiveTagsArg;
    ApplicationUtilities::ConfigValueArgument id3v2VersionArg;
    ApplicationUtilities::ConfigValueArgument id3InitOnCreateArg;
    ApplicationUtilities::ConfigValueArgument id3TransferOnRemovalArg;
    ApplicationUtilities::ConfigValueArgument encodingArg;
    ApplicationUtilities::ConfigValueArgument removeTargetArg;
    ApplicationUtilities::ConfigValueArgument addAttachmentArg;
    ApplicationUtilities::ConfigValueArgument updateAttachmentArg;
    ApplicationUtilities::ConfigValueArgument removeAttachmentArg;
    ApplicationUtilities::ConfigValueArgument removeExistingAttachmentsArg;
    ApplicationUtilities::ConfigValueArgument minPaddingArg;
    ApplicationUtilities::ConfigValueArgument maxPaddingArg;
    ApplicationUtilities::ConfigValueArgument prefPaddingArg;
    ApplicationUtilities::ConfigValueArgument tagPosValueArg;
    ApplicationUtilities::ConfigValueArgument forceTagPosArg;
    ApplicationUtilities::ConfigValueArgument tagPosArg;
    ApplicationUtilities::ConfigValueArgument indexPosValueArg;
    ApplicationUtilities::ConfigValueArgument forceIndexPosArg;
    ApplicationUtilities::ConfigValueArgument indexPosArg;
    ApplicationUtilities::ConfigValueArgument forceRewriteArg;
    ApplicationUtilities::ConfigValueArgument valuesArg;
    ApplicationUtilities::ConfigValueArgument outputFilesArg;
    ApplicationUtilities::ConfigValueArgument backupDirArg;
    ApplicationUtilities::ConfigValueArgument layoutOnlyArg;
    ApplicationUtilities::OperationArgument setTagInfoArg;
};

extern const char *const fieldNames;
extern const char *const fieldNamesForSet;
void applyGeneralConfig(const ApplicationUtilities::Argument &timeSapnFormatArg);
void printFieldNames(const ApplicationUtilities::ArgumentOccurrence &occurrence);
void displayFileInfo(const ApplicationUtilities::ArgumentOccurrence &, const ApplicationUtilities::Argument &filesArg,
    const ApplicationUtilities::Argument &verboseArg);
void generateFileInfo(const ApplicationUtilities::ArgumentOccurrence &, const ApplicationUtilities::Argument &inputFileArg,
    const ApplicationUtilities::Argument &outputFileArg, const ApplicationUtilities::Argument &validateArg);
void displayTagInfo(const ApplicationUtilities::Argument &fieldsArg, const ApplicationUtilities::Argument &filesArg,
    const ApplicationUtilities::Argument &verboseArg);
void setTagInfo(const Cli::SetTagInfoArgs &args);
void extractField(const ApplicationUtilities::Argument &fieldArg, const ApplicationUtilities::Argument &attachmentArg,
    const ApplicationUtilities::Argument &inputFilesArg, const ApplicationUtilities::Argument &outputFileArg,
    const ApplicationUtilities::Argument &verboseArg);
void exportToJson(const ApplicationUtilities::ArgumentOccurrence &, const ApplicationUtilities::Argument &filesArg,
    const ApplicationUtilities::Argument &prettyArg);

} // namespace Cli

#endif // MAINFEATURES_H
