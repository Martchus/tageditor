#ifndef CLI_ATTACHMENT_INFO
#define CLI_ATTACHMENT_INFO

#include <tagparser/diagnostics.h>

#include <c++utilities/conversion/types.h>

namespace TagParser {
class AbstractContainer;
class AbstractAttachment;
}

namespace Cli {

enum class AttachmentAction {
    Add,
    Update,
    Remove
};

class AttachmentInfo
{
public:
    AttachmentInfo();
    void parseDenotation(const char *denotation);
    void apply(TagParser::AbstractContainer *container, TagParser::Diagnostics &diag);
    void apply(TagParser::AbstractAttachment *attachment, TagParser::Diagnostics &diag);
    void reset();
    bool next(TagParser::AbstractContainer *container, TagParser::Diagnostics &diag);

    AttachmentAction action;
    uint64 id;
    bool hasId;
    const char *path;
    const char *name;
    const char *mime;
    const char *desc;
};

inline AttachmentInfo::AttachmentInfo() :
    action(AttachmentAction::Add),
    id(0),
    hasId(false),
    path(nullptr),
    name(nullptr),
    mime(nullptr),
    desc(nullptr)
{}

}

#endif // CLI_ATTACHMENT_INFO
