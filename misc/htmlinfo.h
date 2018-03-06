#ifndef HTMLINFO_H
#define HTMLINFO_H

#include <QByteArray>

#include <list>

namespace Media {
class MediaFileInfo;
class Diagnostics;
}

namespace HtmlInfo {

QByteArray generateInfo(const Media::MediaFileInfo &file, Media::Diagnostics &diag, Media::Diagnostics &diagReparsing);

}

#endif // HTMLINFO_H
