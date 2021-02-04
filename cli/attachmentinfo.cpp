#include "./attachmentinfo.h"

#include <tagparser/abstractattachment.h>
#include <tagparser/abstractcontainer.h>
#include <tagparser/progressfeedback.h>

#include <c++utilities/conversion/conversionexception.h>
#include <c++utilities/conversion/stringbuilder.h>
#include <c++utilities/conversion/stringconversion.h>

#include <cstring>
#include <iostream>

using namespace std;
using namespace CppUtilities;
using namespace TagParser;

namespace Cli {

void AttachmentInfo::parseDenotation(const char *denotation)
{
    if (!strncmp(denotation, "id=", 3)) {
        try {
            id = stringToNumber<std::uint64_t, string>(denotation + 3);
            hasId = true;
        } catch (const ConversionException &) {
            cerr << "The specified attachment ID \"" << (denotation + 3) << "\" is invalid.";
        }
    } else if (!strncmp(denotation, "path=", 5)) {
        path = denotation + 5;
    } else if (!strncmp(denotation, "name=", 5)) {
        name = denotation + 5;
    } else if (!strncmp(denotation, "mime=", 5)) {
        mime = denotation + 5;
    } else if (!strncmp(denotation, "desc=", 5)) {
        desc = denotation + 5;
    } else {
        cerr << "The attachment specification \"" << denotation << "\" is invalid and will be ignored.";
    }
}

void AttachmentInfo::apply(AbstractContainer *container, TagParser::Diagnostics &diag)
{
    static const string context("applying specified attachments");
    AbstractAttachment *attachment = nullptr;
    bool attachmentFound = false;
    switch (action) {
    case AttachmentAction::Add:
        if (!path || !name) {
            cerr << "Argument --update-argument specified but no name/path provided." << endl;
            return;
        }
        apply(container->createAttachment(), diag);
        break;
    case AttachmentAction::Update:
        if (hasId) {
            for (size_t i = 0, count = container->attachmentCount(); i < count; ++i) {
                attachment = container->attachment(i);
                if (attachment->id() == id) {
                    apply(attachment, diag);
                    attachmentFound = true;
                }
            }
            if (!attachmentFound) {
                diag.emplace_back(DiagLevel::Critical,
                    argsToString("Attachment with the specified ID \"", id, "\" does not exist and hence can't be updated."), context);
            }
        } else if (name) {
            for (size_t i = 0, count = container->attachmentCount(); i < count; ++i) {
                attachment = container->attachment(i);
                if (attachment->name() == name) {
                    apply(attachment, diag);
                    attachmentFound = true;
                }
            }
            if (!attachmentFound) {
                diag.emplace_back(DiagLevel::Critical,
                    argsToString("Attachment with the specified name \"", name, "\" does not exist and hence can't be updated."), context);
            }
        } else {
            cerr << "Argument --update-argument specified but no ID/name provided." << endl;
        }
        break;
    case AttachmentAction::Remove:
        if (hasId) {
            for (size_t i = 0, count = container->attachmentCount(); i < count; ++i) {
                attachment = container->attachment(i);
                if (attachment->id() == id) {
                    attachment->setIgnored(true);
                    attachmentFound = true;
                }
            }
            if (!attachmentFound) {
                diag.emplace_back(DiagLevel::Critical,
                    "Attachment with the specified ID \"" + numberToString(id) + "\" does not exist and hence can't be removed.", context);
            }
        } else if (name) {
            for (size_t i = 0, count = container->attachmentCount(); i < count; ++i) {
                attachment = container->attachment(i);
                if (attachment->name() == name) {
                    attachment->setIgnored(true);
                    attachmentFound = true;
                }
            }
            if (!attachmentFound) {
                diag.emplace_back(DiagLevel::Critical,
                    "Attachment with the specified name \"" + string(name) + "\" does not exist and hence can't be removed.", context);
            }
        } else {
            cerr << "Argument --remove-argument specified but no ID/name provided." << endl;
        }
        break;
    }
}

void AttachmentInfo::apply(AbstractAttachment *attachment, TagParser::Diagnostics &diag)
{
    if (hasId) {
        attachment->setId(id);
    }
    if (path) {
        AbortableProgressFeedback progress;
        attachment->setFile(path, diag, progress);
    }
    if (name) {
        attachment->setName(name);
    }
    if (mime) {
        attachment->setMimeType(mime);
    }
    if (desc) {
        attachment->setDescription(desc);
    }
}

void AttachmentInfo::reset()
{
    action = AttachmentAction::Add;
    id = 0;
    hasId = false;
    path = name = mime = desc = nullptr;
}

bool AttachmentInfo::next(AbstractContainer *container, TagParser::Diagnostics &diag)
{
    if (!id && !path && !name && !mime && !desc) {
        // skip empty attachment info
        return false;
    }
    apply(container, diag);
    reset();
    return true;
}

} // namespace Cli
