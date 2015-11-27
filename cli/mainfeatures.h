#ifndef MAINFEATURES_H
#define MAINFEATURES_H

#include <tagparser/tagvalue.h>

#include <c++utilities/application/argumentparser.h>

#include <vector>

namespace ApplicationUtilities {

typedef std::vector<std::string> StringVector;
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
    ApplicationUtilities::Argument tagPosArg;
    ApplicationUtilities::Argument forceTagPosArg;
    ApplicationUtilities::Argument indexPosArg;
    ApplicationUtilities::Argument forceIndexPosArg;
    ApplicationUtilities::Argument forceRewriteArg;
    ApplicationUtilities::Argument setTagInfoArg;
};

void printFieldNames(const ApplicationUtilities::StringVector &parameterValues);
void displayFileInfo(const ApplicationUtilities::StringVector &, const ApplicationUtilities::Argument &filesArg, const ApplicationUtilities::Argument &verboseArg);
void generateFileInfo(const ApplicationUtilities::StringVector &parameterValues, const ApplicationUtilities::Argument &inputFileArg, const ApplicationUtilities::Argument &outputFileArg, const ApplicationUtilities::Argument &validateArg);
void displayTagInfo(const ApplicationUtilities::StringVector &parameterValues, const ApplicationUtilities::Argument &filesArg, const ApplicationUtilities::Argument &verboseArg);
void setTagInfo(const ApplicationUtilities::StringVector &parameterValues, const Cli::SetTagInfoArgs &args);
void extractField(const ApplicationUtilities::StringVector &parameterValues, const ApplicationUtilities::Argument &inputFileArg, const ApplicationUtilities::Argument &outputFileArg, const ApplicationUtilities::Argument &verboseArg);
void removeBackupFiles(const ApplicationUtilities::StringVector &parameterValues, const ApplicationUtilities::Argument &recursiveArg);

}

#endif // MAINFEATURES_H
