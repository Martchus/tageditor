#ifndef MAINFEATURES_H
#define MAINFEATURES_H

#include <tagparser/tagvalue.h>

#include <c++utilities/application/argumentparser.h>

#include <vector>

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
    ApplicationUtilities::Argument encodingArg;
    ApplicationUtilities::Argument removeTargetsArg;
    ApplicationUtilities::Argument attachmentsArg;
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
    ApplicationUtilities::Argument setTagInfoArg;
};

extern const char *const fieldNames;
void printFieldNames(const ApplicationUtilities::ArgumentOccurance &occurance);
void displayFileInfo(const ApplicationUtilities::ArgumentOccurance &, const ApplicationUtilities::Argument &filesArg, const ApplicationUtilities::Argument &verboseArg);
void generateFileInfo(const ApplicationUtilities::ArgumentOccurance &, const ApplicationUtilities::Argument &inputFileArg, const ApplicationUtilities::Argument &outputFileArg, const ApplicationUtilities::Argument &validateArg);
void displayTagInfo(const ApplicationUtilities::Argument &fieldsArg, const ApplicationUtilities::Argument &filesArg, const ApplicationUtilities::Argument &verboseArg);
void setTagInfo(const Cli::SetTagInfoArgs &args);
void extractField(const ApplicationUtilities::Argument &fieldsArg, const ApplicationUtilities::Argument &inputFileArg, const ApplicationUtilities::Argument &outputFileArg, const ApplicationUtilities::Argument &verboseArg);

}

#endif // MAINFEATURES_H
