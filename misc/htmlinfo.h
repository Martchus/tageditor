#ifndef HTMLINFO_H
#define HTMLINFO_H

#include <QByteArray>

#include <list>

namespace TagParser {
class MediaFileInfo;
class Diagnostics;
} // namespace TagParser

namespace HtmlInfo {

QByteArray generateInfo(const TagParser::MediaFileInfo &file, TagParser::Diagnostics &diag, TagParser::Diagnostics &diagReparsing);
}

#endif // HTMLINFO_H
