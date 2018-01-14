#ifndef MAINFEATURES_H
#define MAINFEATURES_H

#include <c++utilities/application/argumentparser.h>

namespace ApplicationUtilities {
class Argument;
}

namespace Cli {

struct SetTagInfoArgs
{
    SetTagInfoArgs(ApplicationUtilities::Argument &filesArg, ApplicationUtilities::Argument &verboseArg);
    ApplicationUtilities::Argument &filesArg;
    ApplicationUtilities::Argument &verboseArg;
    ApplicationUtilities::Argument docTitleArg;
    ApplicationUtilities::Argument removeOtherFieldsArg;
    ApplicationUtilities::Argument treatUnknownFilesAsMp3FilesArg;
    ApplicationUtilities::Argument id3v1UsageArg;
    ApplicationUtilities::Argument id3v2UsageArg;
    ApplicationUtilities::Argument mergeMultipleSuccessiveTagsArg;
    ApplicationUtilities::Argument id3v2VersionArg;
    ApplicationUtilities::Argument id3InitOnCreateArg;
    ApplicationUtilities::Argument id3TransferOnRemovalArg;
    ApplicationUtilities::Argument encodingArg;
    ApplicationUtilities::Argument removeTargetArg;
    ApplicationUtilities::Argument addAttachmentArg;
    ApplicationUtilities::Argument updateAttachmentArg;
    ApplicationUtilities::Argument removeAttachmentArg;
    ApplicationUtilities::Argument removeExistingAttachmentsArg;
    ApplicationUtilities::Argument minPaddingArg;
    ApplicationUtilities::Argument maxPaddingArg;
    ApplicationUtilities::Argument prefPaddingArg;
    ApplicationUtilities::Argument tagPosValueArg;
    ApplicationUtilities::Argument forceTagPosArg;
    ApplicationUtilities::Argument tagPosArg;
    ApplicationUtilities::Argument indexPosValueArg;
    ApplicationUtilities::Argument forceIndexPosArg;
    ApplicationUtilities::Argument indexPosArg;
    ApplicationUtilities::Argument forceRewriteArg;
    ApplicationUtilities::Argument valuesArg;
    ApplicationUtilities::Argument outputFilesArg;
    ApplicationUtilities::Argument setTagInfoArg;
};

extern const char *const fieldNames;
extern const char *const fieldNamesForSet;
void applyGeneralConfig(const ApplicationUtilities::Argument &timeSapnFormatArg);
void printFieldNames(const ApplicationUtilities::ArgumentOccurrence &occurrence);
void displayFileInfo(const ApplicationUtilities::ArgumentOccurrence &, const ApplicationUtilities::Argument &filesArg, const ApplicationUtilities::Argument &verboseArg);
void generateFileInfo(const ApplicationUtilities::ArgumentOccurrence &, const ApplicationUtilities::Argument &inputFileArg, const ApplicationUtilities::Argument &outputFileArg, const ApplicationUtilities::Argument &validateArg);
void displayTagInfo(const ApplicationUtilities::Argument &fieldsArg, const ApplicationUtilities::Argument &filesArg, const ApplicationUtilities::Argument &verboseArg);
void setTagInfo(const Cli::SetTagInfoArgs &args);
void extractField(const ApplicationUtilities::Argument &fieldArg, const ApplicationUtilities::Argument &attachmentArg, const ApplicationUtilities::Argument &inputFilesArg, const ApplicationUtilities::Argument &outputFileArg, const ApplicationUtilities::Argument &verboseArg);
void exportToJson(const ApplicationUtilities::ArgumentOccurrence &, const ApplicationUtilities::Argument &filesArg);

}

#endif // MAINFEATURES_H
