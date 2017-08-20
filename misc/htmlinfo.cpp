#include "./htmlinfo.h"

#include <tagparser/signature.h>
#include <tagparser/mediafileinfo.h>
#include <tagparser/abstractcontainer.h>
#include <tagparser/matroska/matroskacontainer.h>
#include <tagparser/matroska/matroskaeditionentry.h>
#include <tagparser/mp4/mp4container.h>
#include <tagparser/abstracttrack.h>
#include <tagparser/abstractattachment.h>
#include <tagparser/notification.h>

#include <qtutilities/resources/resources.h>

#include <c++utilities/chrono/timespan.h>
#include <c++utilities/chrono/datetime.h>
#include <c++utilities/conversion/stringconversion.h>

#if defined(TAGEDITOR_GUI_QTWIDGETS)
# include <QApplication>
# include <QStyle>
#elif defined(TAGEDITOR_GUI_QTQUICK)
# include <QGuiApplication>
#endif
#if defined(TAGEDITOR_GUI_QTWIDGETS) || defined(TAGEDITOR_GUI_QTQUICK)
# include <QFont>
# include <QFontMetrics>
# include <QIcon>
#endif
#include <QString>
#include <QStringBuilder>
#include <QResource>
#include <QBuffer>
#include <QByteArray>
#include <QXmlStreamWriter>
#ifdef QT_DEBUG
# include <QFile>
#endif

#include <list>

#if defined(TAGEDITOR_GUI_QTWIDGETS) || defined(TAGEDITOR_GUI_QTQUICK)
# define APPEND_GUI_RULE(rule, fallback) \
    if(ApplicationInstances::hasGuiApp()) { \
        res.append(rule); \
    } else { \
        res.append(fallback); \
    }
#else
# define APPEND_GUI_RULE(rule, fallback) \
    res.append(fallback);
#endif

using namespace std;
using namespace ConversionUtilities;
using namespace ChronoUtilities;
using namespace Media;

namespace HtmlInfo {

inline QString qstr(const char *cstr)
{
    return QString::fromUtf8(cstr);
}

inline QString qstr(const string &stdstr)
{
    return QString::fromUtf8(stdstr.data(), static_cast<int>(stdstr.size()));
}

class RowMaker
{
public:
    RowMaker(QXmlStreamWriter &writer) :
        writer(writer),
        m_even(true)
    {}

    void reset()
    {
        m_even = true;
    }

    void startRow(const QString &label, bool head = true)
    {
        writer.writeStartElement(QStringLiteral("tr"));
        if((m_even = !m_even)) {
            writer.writeAttribute(QStringLiteral("class"), QStringLiteral("even"));
        }
        writer.writeTextElement(head ? QStringLiteral("th") : QStringLiteral("td"), label);
        writer.writeStartElement(QStringLiteral("td"));
    }

    void startRow(const QString &label, const QString &helpText)
    {
        writer.writeStartElement(QStringLiteral("tr"));
        if((m_even = !m_even)) {
            writer.writeAttribute(QStringLiteral("class"), QStringLiteral("even"));
        }
        writer.writeAttribute(QStringLiteral("title"), helpText);
        writer.writeTextElement(QStringLiteral("th"), label);
        writer.writeStartElement(QStringLiteral("td"));
    }

    void endRow()
    {
        writer.writeEndElement();
        writer.writeEndElement();
    }

    void mkRow(const QString &label, const QString &text, bool head = true)
    {
        startRow(label, head);
        writer.writeCharacters(text);
        endRow();
    }

    void mkRow(const QString &label, const QString &helpText, const QString &text)
    {
        startRow(label, helpText);
        writer.writeCharacters(text);
        endRow();
    }

    void startSubTab(const QString &subLabel, size_t index, unsigned int level = 0)
    {
        writer.writeStartElement(QStringLiteral("tr"));
        if((m_even = !m_even)) {
            writer.writeAttribute(QStringLiteral("class"), QStringLiteral("even"));
        }
        writer.writeStartElement(QStringLiteral("th"));
        writer.writeCharacters(subLabel % QStringLiteral(" #") % QString::number(index));
        if(level) {
            writer.writeEmptyElement(QStringLiteral("br"));
            writer.writeCharacters(QString::number(level));
        }
        writer.writeEndElement();
        writer.writeStartElement(QStringLiteral("td"));
        writer.writeStartElement(QStringLiteral("table"));
        writer.writeAttribute(QStringLiteral("class"), QStringLiteral("headervertical"));
        writer.writeStartElement(QStringLiteral("tbody"));
    }

    void startSubTab(const QString &subLabel)
    {
        writer.writeStartElement(QStringLiteral("tr"));
        if((m_even = !m_even)) {
            writer.writeAttribute(QStringLiteral("class"), QStringLiteral("even"));
        }
        writer.writeTextElement(QStringLiteral("th"), subLabel);
        writer.writeStartElement(QStringLiteral("td"));
        writer.writeStartElement(QStringLiteral("table"));
        writer.writeAttribute(QStringLiteral("class"), QStringLiteral("headervertical"));
        writer.writeStartElement(QStringLiteral("tbody"));
    }

    void startHorizontalSubTab(const QString &subLabel, const QStringList &headerLabels)
    {
        writer.writeStartElement(QStringLiteral("tr"));
        if((m_even = !m_even)) {
            writer.writeAttribute(QStringLiteral("class"), QStringLiteral("even"));
        }
        writer.writeTextElement(QStringLiteral("th"), subLabel);
        writer.writeStartElement(QStringLiteral("td"));
        writer.writeStartElement(QStringLiteral("table"));
        writer.writeAttribute(QStringLiteral("class"), QStringLiteral("headerhorizontal"));
        writer.writeStartElement(QStringLiteral("thead"));
        writer.writeStartElement(QStringLiteral("tr"));
        for(const auto &label : headerLabels) {
            writer.writeTextElement(QStringLiteral("th"), label);
        }
        writer.writeEndElement();
        writer.writeEndElement();
        writer.writeStartElement(QStringLiteral("tbody"));
    }

    void endSubTab()
    {
        writer.writeEndElement();
        writer.writeEndElement();
        writer.writeEndElement();
        writer.writeEndElement();
    }

private:
    QXmlStreamWriter &writer;
    bool m_even;
};

QString mkFontStyle(const QFont &font)
{
    QString style;
    switch(font.style()) {
    case QFont::StyleItalic:
        style = QStringLiteral("italic");
        break;
    case QFont::StyleOblique:
        style = QStringLiteral("oblique");
        break;
    default:
        style = QStringLiteral("normal");
    }
    QString weight;
    switch(font.weight()) {
    case QFont::Light:
        weight = QStringLiteral("light");
        break;
    case QFont::Bold:
        weight = QStringLiteral("bold");
        break;
    case QFont::Black:
        weight = QStringLiteral("bolder");
        break;
    default:
        weight = QStringLiteral("normal");
    }
    QString size;
    if(font.pixelSize() > 0) {
        size = QStringLiteral("%1px").arg(font.pixelSize());
    } else {
        size = QStringLiteral("%1pt").arg(font.pointSize());
    }
    return QStringLiteral("font-family: \"%1\", sans-serif;"
                          "font-style: %2;"
                          "font-weight: %3;"
                          "font-size: %4;").arg(
                font.family(), style, weight, size);
}

QByteArray mkBase64(const QIcon &icon)
{
    QByteArray bytes;
    QBuffer buffer(&bytes);
    buffer.open(QIODevice::WriteOnly);
    icon.pixmap(16, 16).save(&buffer, "PNG");
    return bytes.toBase64();
}

const QByteArray &mkInfoIconData()
{
    static const QByteArray data = mkBase64(QApplication::style()->standardIcon(QStyle::SP_MessageBoxInformation));
    return data;
}

const QByteArray &mkWarningIconData()
{
    static const QByteArray data = mkBase64(QApplication::style()->standardIcon(QStyle::SP_MessageBoxWarning));
    return data;
}

const QByteArray &mkErrorIconData()
{
    static const QByteArray data = mkBase64(QApplication::style()->standardIcon(QStyle::SP_MessageBoxCritical));
    return data;
}

const QByteArray &mkDebugIconData()
{
    static const QByteArray data = mkBase64(QIcon(QStringLiteral(":/images/bug")));
    return data;
}

template<class ElementType> void mkElementContent(QXmlStreamWriter &, ElementType *)
{}

template<> void mkElementContent(QXmlStreamWriter &writer, EbmlElement *element)
{
    switch(element->id()) {
    case MatroskaIds::SeekID: {
        const uint64 seekId = element->readUInteger();
        writer.writeCharacters(QStringLiteral(", denoted type: 0x"));
        writer.writeCharacters(QString::number(seekId, 16));
        if(seekId <= numeric_limits<uint32>::max()) {
            const char *const seekIdName = matroskaIdName(static_cast<uint32>(seekId));
            if(*seekIdName) {
                writer.writeCharacters(QStringLiteral(" \""));
                writer.writeCharacters(QString::fromLatin1(seekIdName));
                writer.writeCharacters(QStringLiteral("\""));
            }
        }
        break;
    } case MatroskaIds::SeekPosition: {
        writer.writeCharacters(QStringLiteral(", denoted position: "));
        const uint64 seekPos = element->readUInteger();
        const auto seekPosStr = QString::number(seekPos);
        writer.writeStartElement(QStringLiteral("span"));
        writer.writeAttribute(QStringLiteral("data-role"), QStringLiteral("offset"));
        writer.writeAttribute(QStringLiteral("data-dec"), seekPosStr);
        writer.writeAttribute(QStringLiteral("data-hex"), QString::number(seekPos, 16));
        writer.writeCharacters(seekPosStr);
        writer.writeEndElement();
        break;
    } default:
        ;
    }
}

class Generator
{
public:

    Generator(const MediaFileInfo &file, NotificationList &originalNotifications) :
        m_writer(&m_res),
        m_rowMaker(m_writer),
        m_file(file),
        originalNotifications(originalNotifications)
    {}

    QString mkStyle()
    {
        QString res;
        res.append(QStringLiteral("html, body {"
                                  "padding: 1px;"
                                  "margin: 0px;"));
#ifndef GUI_NONE
        QPalette palette;
        if(ApplicationInstances::hasGuiApp()) {
            palette = QGuiApplication::palette();
            res.append(mkFontStyle(QGuiApplication::font()));
            res.append(QStringLiteral("background-color: %1;").arg(palette.color(QPalette::Base).name()));
            res.append(QStringLiteral("color: %1;").arg(palette.color(QPalette::Text).name()));
        } else {
#endif
            res.append(QStringLiteral("background-color: #fff;"
                                      "color: #000;"));
#ifndef GUI_NONE
        }
#endif
        res.append(QStringLiteral("}"
                                  "a:link, #structure .parent-node {"));
        APPEND_GUI_RULE(QStringLiteral("color: %1;").arg(palette.color(QPalette::Link).name()),
                        QStringLiteral("color: #337AB7;"))
                res.append(QStringLiteral("text-decoration: none;"
                                          "}"
                                          "a:focus, a:hover {"));
        APPEND_GUI_RULE(QStringLiteral("color: %1;").arg(palette.link().color().darker(palette.color(QPalette::Background).lightness() > palette.color(QPalette::Link).lightness() ? 150 : 50).name()),
                        QStringLiteral("color: #23527c;"))
                res.append(QStringLiteral("text-decoration: underline;"
                                          "} table {"
                                          "border-collapse: collapse;"
                                          "width: 100%;"
                                          "} tr {"));
        APPEND_GUI_RULE(QStringLiteral("background-color: %1;").arg(palette.color(QPalette::Base).name()),
                        QStringLiteral("background-color: #fff;"))
                res.append(QStringLiteral("} tr.even {"));
        APPEND_GUI_RULE(QStringLiteral("background-color: %1!important;").arg(palette.color(QPalette::AlternateBase).name()), QStringLiteral("background-color: #fafafa!important;"))
                res.append(QStringLiteral("}"
                                          "table.headervertical th, table.headervertical td {"
                                          "padding: 2px 5px;"
                                          "text-align: left;"
                                          "vertical-align: top;"
                                          "}"
                                          "table.headervertical > tbody > tr > th {"
                                          "text-align: right;"
                                          "border: none;"
                                          "width: 25%;"
                                          "}"
                                          "table.headerhorizontal > thead > tr > th {"
                                          "text-align: center;"
                                          "border: none;"
                                          "}"
                                          ".more {"
                                          "display: none;"
                                          "}"
                                          ".more tr > th {"
                                          "font-weight: normal;"
                                          "}"
                                          ".nodecollapsed {"
                                          "display: none;"
                                          "}"
                                          ".nodeexpaned {"
                                          "display: block;"
                                          "}"
                                          "table.headerhorizontal > thead > tr > th {"
                                          "font-weight: bold;"
                                          "}"
                                          "td table.headervertical > thead > tr > th {"
                                          "font-weight: normal;"
                                          "border: none;"
                                          "background-color: transparent;"
                                          "}"
                                          "th {"
                                          "cursor: default;"
                                          "}"
                                          "td.warning, td.critical, td.information {"
                                          "width: 18px;"
                                          "background-repeat: no-repeat;"
                                          "background-position:center;"
                                          "}"
                                          ".has-helptext {"
                                          "cursor: help;"
                                          "}"
                                          "#structure_links a {"
                                          "margin-right: 5px;"
                                          "}"));
#if defined(TAGEDITOR_GUI_QTWIDGETS)
        if(ApplicationInstances::hasWidgetsApp()) {
            res.append(QStringLiteral("td.warning {background-image: url(data:image/png;base64,"));
            res.append(mkWarningIconData());
            res.append(QStringLiteral(");}td.critical {background-image: url(data:image/png;base64,"));
            res.append(mkErrorIconData());
            res.append(QStringLiteral(");}td.information {background-image: url(data:image/png;base64,"));
            res.append(mkInfoIconData());
            res.append(QStringLiteral(");}"));
        }
#endif
        res.append(QStringLiteral("ul {"
                                  "margin: 0px;"
                                  "padding: 0px 0px 0px 15px;"
                                  "list-style-type: none;"
                                  "}"));
        return res;
    }

    QString mkScript()
    {
        QString res;
        res.append(QStringLiteral("function toggleVisibility(link, objid) {"
                                  "var obj = document.getElementById(objid);"
                                  "if(obj.style.display === \"none\" || (obj.style.display === \"\" && obj.className === \"more\")) {"
                                  "if(obj.nodeName === \"TBODY\" || obj.nodeName === \"tbody\") {"
                                  "obj.style.display = \"table-row-group\";"
                                  "} else {"
                                  "obj.style.display = \"block\";"
                                  "}"
                                  "link.defaultText = link.innerHTML;"
                                  "link.innerHTML = \""));
        res.append(QCoreApplication::translate("HtmlInfo", "hide"));
        res.append(QStringLiteral("\";"
                                  "} else {"
                                  "obj.style.display = \"none\";"
                                  "link.innerHTML = link.defaultText;"
                                  "}}"
                                  "function expandCollapse(parent) {"
                                  "for(var children = parent.children, i = 0, c = children.length; i < c; ++i) {"
                                  "if(children[i].tagName === 'UL' || children[i].tagName === 'ul') {"
                                  "children[i].className = children[i].className === 'nodeexpanded' ? 'nodecollapsed' : 'nodeexpanded';"
                                  "}}}"
                                  "function expandAll(objid) {"
                                  "var children = document.getElementById(objid).getElementsByTagName('ul');"
                                  "for(var i = 0; i < children.length; ++i) {"
                                  "children[i].className = 'nodeexpanded';"
                                  "}}"
                                  "function collapseAll(objid) {"
                                  "var children = document.getElementById(objid).getElementsByTagName('ul');"
                                  "for(var i = 0; i < children.length; ++i) {"
                                  "if(children[i].parentNode.tagName === 'LI' || children[i].parentNode.tagName === 'li') {"
                                  "children[i].className = 'nodecollapsed';"
                                  "}}}"
                                  "var baseHex = false;"
                                  "function switchBase(objid) {"
                                  "baseHex = !baseHex;"
                                  "document.getElementById('toggle_hex_link').firstChild.data = baseHex ? 'decimal' : 'hex';"
                                  "var elements = document.getElementById(objid).getElementsByTagName('span');"
                                  "for(var i = 0; i < elements.length; ++i) {"
                                  "var element = elements[i];"
                                  "if(element.getAttribute('data-role') === 'offset') {"
                                  "element.firstChild.data = element.getAttribute(baseHex ? 'data-hex' : 'data-dec');"
                                  "}}}"));
        return res;
    }

    void startVerticalTable()
    {
        m_writer.writeStartElement(QStringLiteral("table"));
        m_writer.writeAttribute(QStringLiteral("class"), QStringLiteral("headervertical"));
    }

    void startTableSection(const QString &id)
    {
        m_writer.writeStartElement(QStringLiteral("tbody"));
        m_writer.writeAttribute(QStringLiteral("id"), id);
    }

    void startTableSection()
    {
        m_writer.writeStartElement(QStringLiteral("tbody"));
    }

    void startExtendedTableSection(const QString &id)
    {
        startTableSection(id);
        m_writer.writeAttribute(QStringLiteral("class"), QStringLiteral("more"));
    }

    void mkSpace()
    {
        m_writer.writeCharacters(QStringLiteral(" "));
    }

    void mkBreak()
    {
        m_writer.writeStartElement(QStringLiteral("br"));
        m_writer.writeEndElement();
    }

    void mkDetailsLink(const QString &id, const QString &text)
    {
        m_writer.writeStartElement(QStringLiteral("a"));
        m_writer.writeAttribute(QStringLiteral("href"), QStringLiteral("#"));
        m_writer.writeAttribute(QStringLiteral("onclick"), QStringLiteral("toggleVisibility(this, '") % id % QStringLiteral("'); return false;"));
        m_writer.writeCharacters(text);
        m_writer.writeEndElement();
    }

    void mkTrack(const AbstractTrack *track, unsigned int trackNumber)
    {
        RowMaker rowMaker(m_writer);
        rowMaker.startSubTab(QCoreApplication::translate("HtmlInfo", "Track"), trackNumber);
        if(track->id()) {
            rowMaker.mkRow(QCoreApplication::translate("HtmlInfo", "ID"), QCoreApplication::translate("HtmlInfo", "The unique number used to identify the track in the container file."), QString::number(track->id()));
        }
        if(track->trackNumber()) {
           rowMaker.mkRow(QCoreApplication::translate("HtmlInfo", "Number"), QCoreApplication::translate("HtmlInfo", "The index of the track in the container file."), QString::number(track->trackNumber()));
        }
        if(!track->name().empty()) {
            rowMaker.mkRow(QCoreApplication::translate("HtmlInfo", "Name"), qstr(track->name()));
        }
        rowMaker.mkRow(QCoreApplication::translate("HtmlInfo", "Type"), qstr(track->mediaTypeName()));
        const char *fmtName = track->formatName(), *fmtAbbr = track->formatAbbreviation();
        rowMaker.mkRow(QCoreApplication::translate("HtmlInfo", "Format"), QCoreApplication::translate("HtmlInfo", "The unabbreviated name of the track's format."), qstr(fmtName));
        if(track->format() != GeneralMediaFormat::Unknown && strcmp(fmtName, fmtAbbr)) { // format name and abbreviation differ
            rowMaker.mkRow(QCoreApplication::translate("HtmlInfo", "Abbreviation"), QCoreApplication::translate("HtmlInfo", "The abbreviated name of the track's format."), qstr(fmtAbbr));
        }
        if(track->version() != 0.0) {
            switch(track->format().general) {
            case GeneralMediaFormat::Mpeg4Video:
            case GeneralMediaFormat::Avc:
                rowMaker.mkRow(QCoreApplication::translate("HtmlInfo", "Level"), QCoreApplication::translate("HtmlInfo", "The version/level of the track's format."), QChar('L') + QString::number(track->version()));
                break;
            default:
                rowMaker.mkRow(QCoreApplication::translate("HtmlInfo", "Version"), QCoreApplication::translate("HtmlInfo", "The version/level of the track's format."), QString::number(track->version()));
            }
        }
        fmtName = track->format().extensionName();
        if(*fmtName) {
            rowMaker.mkRow(QCoreApplication::translate("HtmlInfo", "Extension"), QCoreApplication::translate("HtmlInfo", "Used format extensions."), qstr(fmtName));
        }
        if(!track->formatId().empty()) {
            rowMaker.mkRow(QCoreApplication::translate("HtmlInfo", "Format/codec ID"), QCoreApplication::translate("HtmlInfo", "The raw format/codec identifier extracted from the container."), qstr(track->formatId()));
        }
        if(track->size()) {
            rowMaker.mkRow(QCoreApplication::translate("HtmlInfo", "Size"), qstr(dataSizeToString(track->size(), true)));
        }
        if(!track->duration().isNull()) {
            rowMaker.mkRow(QCoreApplication::translate("HtmlInfo", "Duration"), qstr(track->duration().toString(TimeSpanOutputFormat::WithMeasures)) % QStringLiteral(" (") % QString::number(track->duration().totalTicks()) % QChar(')'));
        }
        if(track->bitrate() != 0.0) {
            rowMaker.mkRow(QCoreApplication::translate("HtmlInfo", "Avg. bitrate"), qstr(bitrateToString(track->bitrate())));
        }
        if(track->maxBitrate() != 0.0) {
            rowMaker.mkRow(QCoreApplication::translate("HtmlInfo", "Maximum bitrate"), qstr(bitrateToString(track->maxBitrate())));
        }
        if(!track->creationTime().isNull()) {
            rowMaker.mkRow(QCoreApplication::translate("HtmlInfo", "Creation time"), qstr(track->creationTime().toString(DateTimeOutputFormat::DateAndTime, true)));
        }
        if(!track->modificationTime().isNull()) {
            rowMaker.mkRow(QCoreApplication::translate("HtmlInfo", "Modification time"), qstr(track->modificationTime().toString(DateTimeOutputFormat::DateAndTime, true)));
        }
        if(!track->language().empty()) {
            rowMaker.mkRow(QCoreApplication::translate("HtmlInfo", "Language"), qstr(track->language()));
        }
        if(!track->compressorName().empty()) {
            rowMaker.mkRow(QCoreApplication::translate("HtmlInfo", "Compressor name"), qstr(track->compressorName()));
        }
        if(track->samplingFrequency()) {
            if(track->extensionSamplingFrequency()) {
                rowMaker.mkRow(QCoreApplication::translate("HtmlInfo", "Sampling frequency"), QString::number(track->extensionSamplingFrequency()) % QStringLiteral(" Hz / ") % QString::number(track->samplingFrequency()) % QStringLiteral(" Hz"));
            } else {
                rowMaker.mkRow(QCoreApplication::translate("HtmlInfo", "Sampling frequency"), QString::number(track->samplingFrequency()) + QStringLiteral(" Hz"));
            }
        }
        if(track->sampleCount()) {
            rowMaker.mkRow(track->mediaType() == MediaType::Video
                           ? QCoreApplication::translate("HtmlInfo", "Frame count")
                           : QCoreApplication::translate("HtmlInfo", "Sample count"),
                           QString::number(track->sampleCount()));
        }
        if(track->bitsPerSample()) {
            rowMaker.mkRow(QCoreApplication::translate("HtmlInfo", "Bits per sample"), QString::number(track->bitsPerSample()));
        }
        if(track->quality()) {
            rowMaker.mkRow(QCoreApplication::translate("HtmlInfo", "Quality"), QString::number(track->quality()));
        }
        if(!track->pixelSize().isNull()) {
            rowMaker.mkRow(QCoreApplication::translate("HtmlInfo", "Pixel size"), qstr(track->pixelSize().toString()));
        }
        if(!track->displaySize().isNull()) {
            rowMaker.mkRow(QCoreApplication::translate("HtmlInfo", "Display size"), qstr(track->displaySize().toString()));
        }
        if(track->pixelAspectRatio().isValid()) {
            rowMaker.mkRow(QCoreApplication::translate("HtmlInfo", "Pixel Aspect Ratio"), QString::number(track->pixelAspectRatio().numerator) % QStringLiteral(" : ") % QString::number(track->pixelAspectRatio().denominator));
        }
        if(!track->cropping().isNull()) {
            rowMaker.mkRow(QCoreApplication::translate("HtmlInfo", "Cropping"), qstr(track->cropping().toString()));
        }
        if(!track->resolution().isNull()) {
            rowMaker.mkRow(QCoreApplication::translate("HtmlInfo", "Resolution"), qstr(track->resolution().toString()));
        }
        if(track->channelConfigString()) {
            if(track->extensionChannelConfigString()) {
                rowMaker.mkRow(QCoreApplication::translate("HtmlInfo", "Channel config"), QCoreApplication::translate("HtmlInfo", "Channel configuration"), QStringLiteral("%1 / %2").arg(QString::fromUtf8(track->extensionChannelConfigString()), QString::fromUtf8(track->channelConfigString())));
            } else {
                rowMaker.mkRow(QCoreApplication::translate("HtmlInfo", "Channel config"), QCoreApplication::translate("HtmlInfo", "Channel configuration"), QString::fromUtf8(track->channelConfigString()));
            }
        } else if(track->channelCount()) {
            rowMaker.mkRow(QCoreApplication::translate("HtmlInfo", "Channel count"), QString::number(track->channelCount()));
        }
        if(track->depth()) {
            rowMaker.mkRow(QCoreApplication::translate("HtmlInfo", "Bit depth"), QString::number(track->depth()));
        }
        if(track->fps()) {
            rowMaker.mkRow(QCoreApplication::translate("HtmlInfo", "Frames per second"), QString::number(track->fps()));
        }
        if(track->chromaFormat()) {
            rowMaker.mkRow(QCoreApplication::translate("HtmlInfo", "Chroma format"), qstr(track->chromaFormat()));
        }
        QStringList labels;
        if(track->isInterlaced()) {
            labels << QCoreApplication::translate("HtmlInfo", "interlaced");
        }
        if(!track->isEnabled()) {
            labels << QCoreApplication::translate("HtmlInfo", "disabled");
        }
        if(track->isDefault()) {
            labels << QCoreApplication::translate("HtmlInfo", "default");
        }
        if(track->isForced()) {
            labels << QCoreApplication::translate("HtmlInfo", "forced");
        }
        if(track->hasLacing()) {
            labels << QCoreApplication::translate("HtmlInfo", "has lacing");
        }
        if(track->isEncrypted()) {
            labels << QCoreApplication::translate("HtmlInfo", "encrypted");
        }
        if(!labels.isEmpty()) {
            rowMaker.mkRow(QCoreApplication::translate("HtmlInfo", "Labeled as"), labels.join(QStringLiteral(", ")));
        }
        rowMaker.endSubTab();
    }

    void mkAttachment(const AbstractAttachment *attachment, unsigned int attachmentNumber)
    {
        RowMaker rowMaker(m_writer);
        m_rowMaker.startSubTab(QCoreApplication::translate("HtmlInfo", "Attachment"), attachmentNumber);
        if(attachment->id()) {
            rowMaker.mkRow(QCoreApplication::translate("HtmlInfo", "ID"), QString::number(attachment->id()));
        }
        if(!attachment->name().empty()) {
            rowMaker.mkRow(QCoreApplication::translate("HtmlInfo", "Name"), qstr(attachment->name()));
        }
        if(attachment->data()) {
            rowMaker.mkRow(QCoreApplication::translate("HtmlInfo", "Size"), qstr(dataSizeToString(static_cast<uint64>(attachment->data()->size()), true)));
        }
        if(!attachment->mimeType().empty()) {
            rowMaker.mkRow(QCoreApplication::translate("HtmlInfo", "Mime-type"), qstr(attachment->mimeType()));
        }
        if(!attachment->description().empty()) {
            rowMaker.mkRow(QCoreApplication::translate("HtmlInfo", "Description"), qstr(attachment->description()));
        }
        rowMaker.endSubTab();
    }

    void mkChapter(const AbstractChapter &chapter, size_t chapterNumber, unsigned int level = 0)
    {
        RowMaker rowMaker(m_writer);
        rowMaker.startSubTab(QCoreApplication::translate("HtmlInfo", "Chapter"), chapterNumber + 1, level);
        if(chapter.id()) {
            rowMaker.mkRow(QCoreApplication::translate("HtmlInfo", "ID"), QString::number(chapter.id()));
        }
        for(const LocaleAwareString &name : chapter.names()) {
            static const string delim(", ");
            const string locale = joinStrings(initializer_list<string>{joinStrings(name.languages(), delim, true), joinStrings(name.countries(), delim, true)}, delim, true);
            rowMaker.mkRow(QCoreApplication::translate("HtmlInfo", "Name (%1)").arg(qstr(locale)), qstr(name));
        }
        if(!chapter.startTime().isNegative()) {
            rowMaker.mkRow(QCoreApplication::translate("HtmlInfo", "Start time"), qstr(chapter.startTime().toString(TimeSpanOutputFormat::WithMeasures)));
        }
        if(!chapter.endTime().isNegative()) {
            rowMaker.mkRow(QCoreApplication::translate("HtmlInfo", "End time"), qstr(chapter.endTime().toString(TimeSpanOutputFormat::WithMeasures)));
        }
        QStringList labels;
        if(chapter.isHidden()) {
            labels << QCoreApplication::translate("HtmlInfo", "hidden");
        }
        if(!chapter.isEnabled()) {
            labels << QCoreApplication::translate("HtmlInfo", "disabled");
        }
        if(!labels.empty()) {
            rowMaker.mkRow(QCoreApplication::translate("HtmlInfo", "Labeled as"), labels.join(QStringLiteral(", ")));
        }
        if(!chapter.tracks().empty()) {
            QStringList trackIds;
            for(uint64 id : chapter.tracks()) {
                trackIds << QString::number(id);
            }
            rowMaker.mkRow(QCoreApplication::translate("HtmlInfo", "Tracks"), trackIds.join(QStringLiteral(", ")));
        }
        rowMaker.endSubTab();
        ++level;
        for(size_t i = 0, nestedChapters = chapter.nestedChapterCount(); i < nestedChapters; ++i) {
            mkChapter(*chapter.nestedChapter(i), i, level);
        }
    }

    void mkEdition(const MatroskaEditionEntry &edition, unsigned int editionNumber)
    {
        RowMaker rowMaker(m_writer);
        rowMaker.startSubTab(QCoreApplication::translate("HtmlInfo", "Edition"), editionNumber + 1);
        if(edition.id()) {
            rowMaker.mkRow(QCoreApplication::translate("HtmlInfo", "ID"), QString::number(edition.id()));
        }
        QStringList labels;
        if(edition.isHidden()) {
            labels << QCoreApplication::translate("HtmlInfo", "hidden");
        }
        if(edition.isDefault()) {
            labels << QCoreApplication::translate("HtmlInfo", "default");
        }
        if(edition.isOrdered()) {
            labels << QCoreApplication::translate("HtmlInfo", "ordered");
        }
        if(!labels.isEmpty()) {
            rowMaker.mkRow(QCoreApplication::translate("HtmlInfo", "Labeled as"), labels.join(QStringLiteral(", ")));
        }
        rowMaker.mkRow(QCoreApplication::translate("HtmlInfo", "Chapters"),
                                  QCoreApplication::translate("HtmlInfo", "edition contains %1 chapter(s)", nullptr, static_cast<int>(edition.chapters().size())).arg(edition.chapters().size()));
        rowMaker.endSubTab();
        unsigned int chapterNumber = 0;
        for(const auto &chapter : edition.chapters()) {
            mkChapter(*chapter, chapterNumber);
            ++chapterNumber;
        }
    }

    template<class ElementType, bool isAdditional = false> void mkElementNode(ElementType *element)
    {
        m_writer.writeStartElement(QStringLiteral("ul"));
        m_writer.writeAttribute(QStringLiteral("class"), element && element->parent() ? QStringLiteral("nodecollapsed") : QStringLiteral("nodeexpanded"));
        while(element) {
            if(element->isParsed()) {
                m_writer.writeStartElement(QStringLiteral("li"));
                if(element->firstChild()) {
                    m_writer.writeStartElement(QStringLiteral("span"));
                    m_writer.writeAttribute(QStringLiteral("class"), QStringLiteral("parent-node"));
                    m_writer.writeAttribute(QStringLiteral("onclick"), QStringLiteral("expandCollapse(this.parentElement);"));
                }
                string idString = element->idToString();
                if(!idString.empty()) {
                    m_writer.writeTextElement(QStringLiteral("em"), QString::fromLatin1(idString.data(), static_cast<int>(idString.size())));
                }

                m_writer.writeCharacters(QStringLiteral(" @"));
                const auto startOffsetStr = QString::number(element->startOffset());
                m_writer.writeStartElement(QStringLiteral("span"));
                m_writer.writeAttribute(QStringLiteral("data-role"), QStringLiteral("offset"));
                m_writer.writeAttribute(QStringLiteral("data-dec"), startOffsetStr);
                m_writer.writeAttribute(QStringLiteral("data-hex"), QString::number(element->startOffset(), 16));
                m_writer.writeCharacters(startOffsetStr);
                m_writer.writeEndElement();

                m_writer.writeCharacters(QCoreApplication::translate("HtmlInfo", ", size: "));
                const auto sizeStr = QString::number(element->totalSize());
                m_writer.writeStartElement(QStringLiteral("span"));
                m_writer.writeAttribute(QStringLiteral("data-role"), QStringLiteral("offset"));
                m_writer.writeAttribute(QStringLiteral("data-dec"), sizeStr);
                m_writer.writeAttribute(QStringLiteral("data-hex"), QString::number(element->totalSize(), 16));
                m_writer.writeCharacters(sizeStr);
                m_writer.writeEndElement();

                mkElementContent(m_writer, element);

                if(element->firstChild()) {
                    m_writer.writeEndElement();
                    mkElementNode(element->firstChild());
                }

                element = element->nextSibling();
                m_writer.writeEndElement();
            } else {
                if(!isAdditional) {
                    m_writer.writeStartElement(QStringLiteral("li"));
                    m_writer.writeAttribute(QStringLiteral("style"), QStringLiteral("color: red;"));
                    m_writer.writeCharacters(QCoreApplication::translate("HtmlInfo", "section has not been analyzed"));
                    m_writer.writeEndElement();
                }
                break;
            }
        }
        m_writer.writeEndElement();
    }

    template<class ContainerType> void mkElementTree(ContainerType *container)
    {
        mkElementNode(container->firstElement());
        for(auto &element : container->additionalElements()) {
            mkElementNode<typename ContainerType::elementType, true>(element.get());
        }
    }

    void mkNotifications(NotificationList &notifications, bool reparsing = false)
    {
        if(notifications.size()) {
            startTableSection();
            const QString moreId(reparsing ? QStringLiteral("notificationsReparsingMore") : QStringLiteral("notificationsMore"));
            m_rowMaker.startRow(reparsing ? QCoreApplication::translate("HtmlInfo", "Notifications (reparsing after saving)") : QCoreApplication::translate("HtmlInfo", "Notifications"));
            m_writer.writeCharacters(QCoreApplication::translate("HtmlInfo", "%1 notification(s) available", 0, static_cast<int>(notifications.size())).arg(notifications.size()));
            mkSpace();
            mkDetailsLink(moreId, QCoreApplication::translate("HtmlInfo", "show notifications"));
            m_rowMaker.endRow();
            m_writer.writeEndElement();

            startExtendedTableSection(moreId);
            m_rowMaker.startHorizontalSubTab(QString(), QStringList() << QString() << QCoreApplication::translate("HtmlInfo", "Context") << QCoreApplication::translate("HtmlInfo", "Message") << QCoreApplication::translate("HtmlInfo", "Time"));
            Notification::sortByTime(notifications);
            for(const Notification &notification : notifications) {
                m_writer.writeStartElement(QStringLiteral("tr"));
                m_writer.writeEmptyElement(QStringLiteral("td"));
                m_writer.writeAttribute(QStringLiteral("class"), qstr(notification.typeName()));
                m_writer.writeTextElement(QStringLiteral("td"), qstr(notification.context()));
                m_writer.writeTextElement(QStringLiteral("td"), qstr(notification.message()));
                m_writer.writeTextElement(QStringLiteral("td"), qstr(notification.creationTime().toString(DateTimeOutputFormat::DateAndTime, false)));
                m_writer.writeEndElement();
            }
            m_rowMaker.endSubTab();
            m_writer.writeEndElement();
        }
    }

    void mkDoc()
    {
#ifdef QT_DEBUG
        m_writer.setAutoFormatting(true);
#endif
        // <!DOCTYPE ... <html> <head>
        m_writer.writeStartDocument();
        m_writer.writeDTD(QStringLiteral("<!DOCTYPE html PUBLIC \"-//W3C//DTD XHTML 1.0 Strict//EN\" \"http://www.w3.org/TR/xhtml1/DTD/xhtml1-strict.dtd\">"));
        m_writer.writeStartElement(QStringLiteral("html"));
        m_writer.writeAttribute(QStringLiteral("xmlns"), QStringLiteral("http://www.w3.org/1999/xhtml"));
        m_writer.writeStartElement(QStringLiteral("head"));

        // <title>
        m_writer.writeTextElement(QStringLiteral("title"), QCoreApplication::translate("HtmlInfo", "Info for %1").arg(qstr(m_file.fileName())));

        // <style>
        m_writer.writeStartElement(QStringLiteral("style"));
        m_writer.writeAttribute(QStringLiteral("type"), QStringLiteral("text/css"));
        m_writer.writeCharacters(mkStyle());
        m_writer.writeEndElement();

        // <script>
        m_writer.writeStartElement(QStringLiteral("script"));
        m_writer.writeAttribute(QStringLiteral("type"), QStringLiteral("text/javascript"));
        m_writer.writeCDATA(mkScript());
        m_writer.writeEndElement();

        // </head>
        m_writer.writeEndElement();

        // <body> <table>
        m_writer.writeStartElement(QStringLiteral("body"));
        startVerticalTable();

        // general information
        startTableSection(QStringLiteral("general"));
        m_rowMaker.mkRow(QCoreApplication::translate("HtmlInfo", "Path"), qstr(m_file.path()));
        m_rowMaker.mkRow(QCoreApplication::translate("HtmlInfo", "Size"), qstr(dataSizeToString(m_file.size(), true)));
        const TimeSpan duration = m_file.duration();
        if(!duration.isNull()) {
            m_rowMaker.mkRow(QCoreApplication::translate("HtmlInfo", "Duration"), qstr(duration.toString(TimeSpanOutputFormat::WithMeasures)));
            m_rowMaker.mkRow(QCoreApplication::translate("HtmlInfo", "Overall avg. bitrate"), qstr(bitrateToString(0.0078125 * m_file.size() / duration.totalSeconds())));
        }
        const char *const mimeType = m_file.mimeType();
        if(*mimeType) {
            m_rowMaker.mkRow(QCoreApplication::translate("HtmlInfo", "Mime-type"), qstr(mimeType));
        }
        m_rowMaker.startRow(QCoreApplication::translate("HtmlInfo", "Container"));
        m_writer.writeCharacters(qstr(m_file.containerFormatName()));
        const char *const subversion = m_file.containerFormatSubversion();
        if(*subversion) {
            mkSpace();
            m_writer.writeCharacters(qstr(subversion));
        }
        AbstractContainer *container = m_file.container();
        if(container || m_file.paddingSize()) {
            mkSpace();
            mkDetailsLink(QStringLiteral("containerMore"), QCoreApplication::translate("HtmlInfo", "show details"));
        }
        m_rowMaker.endRow();
        m_writer.writeEndElement();

        // container
        if(container || m_file.paddingSize()) {
            startExtendedTableSection(QStringLiteral("containerMore"));
            RowMaker rowMaker(m_writer);
            if(container) {
                size_t segmentIndex = 0;
                for(const auto &title : container->titles()) {
                    if(segmentIndex) {
                        rowMaker.mkRow(QCoreApplication::translate("HtmlInfo", "Title (segment %1)").arg(++segmentIndex), qstr(title));
                    } else {
                        ++segmentIndex;
                        rowMaker.mkRow(QCoreApplication::translate("HtmlInfo", "Title"), qstr(title));
                    }
                }
                if(container->version()) {
                    rowMaker.mkRow(QCoreApplication::translate("HtmlInfo", "Version"), QString::number(container->version()));
                }
                if(container->readVersion()) {
                    rowMaker.mkRow(QCoreApplication::translate("HtmlInfo", "Read version"), QString::number(container->readVersion()));
                }
                if(!container->documentType().empty()) {
                    rowMaker.mkRow(QCoreApplication::translate("HtmlInfo", "Document type"), qstr(container->documentType()));
                }
                if(container->doctypeVersion()) {
                    rowMaker.mkRow(QCoreApplication::translate("HtmlInfo", "Document version"), QString::number(container->doctypeVersion()));
                }
                if(container->doctypeReadVersion()) {
                    rowMaker.mkRow(QCoreApplication::translate("HtmlInfo", "Document read version"), QString::number(container->doctypeReadVersion()));
                }
            }
            if(m_file.paddingSize()) {
                rowMaker.mkRow(QCoreApplication::translate("HtmlInfo", "Padding size"), QStringLiteral("%1 (%2 %)").arg(qstr(dataSizeToString(m_file.paddingSize(), true))).arg(static_cast<double>(m_file.paddingSize()) / m_file.size() * 100.0, 0, 'g', 2));
            }

            m_writer.writeEndElement();
        }

        // tags
        auto tags = m_file.tags();
        if(!tags.empty()) {
            startTableSection();
            const QString moreId(QStringLiteral("tagsMore"));
            m_rowMaker.startRow(QCoreApplication::translate("HtmlInfo", "Tags"));
            m_writer.writeCharacters(QCoreApplication::translate("HtmlInfo", "%1 tag(s) assigned", 0, static_cast<int>(tags.size())).arg(tags.size()));
            mkSpace();
            mkDetailsLink(moreId, QCoreApplication::translate("HtmlInfo", "show details"));
            m_rowMaker.endRow();
            m_writer.writeEndElement();

            startExtendedTableSection(moreId);
            for(const Tag *tag : tags) {
                RowMaker rowMaker(m_writer);
                rowMaker.startSubTab(tag->typeName());
                if(!tag->version().empty()) {
                    rowMaker.mkRow(QCoreApplication::translate("HtmlInfo", "Version"), qstr(tag->version()));
                }
                if(tag->supportsTarget() && !tag->target().isEmpty()) {
                    rowMaker.mkRow(QCoreApplication::translate("HtmlInfo", "Target level"), qstr(tag->targetString()));
                }
                rowMaker.mkRow(QCoreApplication::translate("HtmlInfo", "Size"), qstr(dataSizeToString(tag->size(), true)));
                rowMaker.mkRow(QCoreApplication::translate("HtmlInfo", "Field count"), QString::number(tag->fieldCount()));
                rowMaker.endSubTab();
            }
            m_writer.writeEndElement();
        }

        // tracks
        const auto tracks = m_file.tracks();
        if(!tracks.empty()) {
            startTableSection();
            const QString moreId(QStringLiteral("tracksMore"));
            m_rowMaker.startRow(QCoreApplication::translate("HtmlInfo", "Tracks"));
            m_writer.writeCharacters(QCoreApplication::translate("HtmlInfo", "file has %1 track(s)", 0, static_cast<int>(tracks.size())).arg(tracks.size()));
            const string summary(m_file.technicalSummary());
            if(!summary.empty()) {
                m_writer.writeCharacters(QStringLiteral(": "));
                m_writer.writeCharacters(QString::fromUtf8(summary.data(), static_cast<int>(summary.size())));
                mkBreak();
            }
            mkSpace();
            mkDetailsLink(moreId, QCoreApplication::translate("HtmlInfo", "show details"));
            m_rowMaker.endRow();
            m_writer.writeEndElement();

            startExtendedTableSection(moreId);
            unsigned int trackNumber = 1;
            for(const auto *track : tracks) {
                mkTrack(track, trackNumber);
                ++trackNumber;
            }
            m_writer.writeEndElement();
        }

        // attachments
        auto attachments = m_file.attachments();
        if(!attachments.empty()) {
            startTableSection();
            const QString moreId(QStringLiteral("attachmentsMore"));
            m_rowMaker.startRow(QCoreApplication::translate("HtmlInfo", "Attachments"));
            m_writer.writeCharacters(QCoreApplication::translate("HtmlInfo", "%1 attachment(s) assigned", 0, static_cast<int>(attachments.size())).arg(attachments.size()));
            mkSpace();
            mkDetailsLink(moreId, QCoreApplication::translate("HtmlInfo", "show details"));
            m_rowMaker.endRow();
            m_writer.writeEndElement();

            startExtendedTableSection(moreId);
            unsigned int attachmentNumber = 1;
            for(const auto *attachment : attachments) {
                mkAttachment(attachment, attachmentNumber);
                ++attachmentNumber;
            }
            m_writer.writeEndElement();
        }

        // chapters
        if(container) {
            if(m_file.containerFormat() == ContainerFormat::Matroska) {
                const auto &editionEntries = static_cast<const MatroskaContainer *>(container)->editionEntires();
                if(!editionEntries.empty()) {
                    startTableSection();
                    const QString moreId(QStringLiteral("editionsMore"));
                    m_rowMaker.startRow(QCoreApplication::translate("HtmlInfo", "Editions/chapters"));
                    m_writer.writeCharacters(QCoreApplication::translate("HtmlInfo", "file has %1 edition(s)", 0, static_cast<int>(editionEntries.size())).arg(editionEntries.size()));
                    mkSpace();
                    mkDetailsLink(moreId, QCoreApplication::translate("HtmlInfo", "show details"));
                    m_rowMaker.endRow();
                    m_writer.writeEndElement();

                    startExtendedTableSection(moreId);
                    unsigned int editionNumber = 0;
                    for(const auto &edition : static_cast<const MatroskaContainer *>(container)->editionEntires()) {
                        mkEdition(*edition, editionNumber);
                        ++editionNumber;
                    }
                    m_writer.writeEndElement();
                }
            } else if(size_t chapterCount = container->chapterCount()) {
                startTableSection();
                const QString moreId(QStringLiteral("chaptersMore"));
                m_rowMaker.startRow(QCoreApplication::translate("HtmlInfo", "chapters"));
                m_writer.writeCharacters(QCoreApplication::translate("HtmlInfo", "file has %1 chapter(s)", 0, static_cast<int>(chapterCount)).arg(chapterCount));
                mkSpace();
                mkDetailsLink(moreId, QCoreApplication::translate("HtmlInfo", "show details"));
                m_rowMaker.endRow();
                m_writer.writeEndElement();

                startExtendedTableSection(moreId);
                for(size_t i = 0; i < chapterCount; ++i) {
                    mkChapter(*container->chapter(i), i);
                }
                m_writer.writeEndElement();
            }
        }

        // structure
        switch(m_file.containerFormat()) {
        case ContainerFormat::Mp4:
        case ContainerFormat::QuickTime:
        case ContainerFormat::Matroska:
        case ContainerFormat::Webm:
        case ContainerFormat::Ebml:
            startTableSection();
            m_rowMaker.startRow(QCoreApplication::translate("HtmlInfo", "Structure"));
            // links to expand, collapse, switch dec/hex
            m_writer.writeStartElement(QStringLiteral("span"));
            m_writer.writeAttribute(QStringLiteral("id"), QStringLiteral("structure_links"));
            m_writer.writeStartElement(QStringLiteral("a"));
            m_writer.writeAttribute(QStringLiteral("href"), QStringLiteral("#"));
            m_writer.writeAttribute(QStringLiteral("onclick"), QStringLiteral("expandAll('structure'); return false;"));
            m_writer.writeCharacters(QCoreApplication::translate("HtmlInfo", "expand all"));
            m_writer.writeEndElement();
            m_writer.writeStartElement(QStringLiteral("a"));
            m_writer.writeAttribute(QStringLiteral("href"), QStringLiteral("#"));
            m_writer.writeAttribute(QStringLiteral("onclick"), QStringLiteral("collapseAll('structure'); return false;"));
            m_writer.writeCharacters(QCoreApplication::translate("HtmlInfo", "collapse all"));
            m_writer.writeEndElement();
            m_writer.writeStartElement(QStringLiteral("a"));
            m_writer.writeAttribute(QStringLiteral("id"), QStringLiteral("toggle_hex_link"));
            m_writer.writeAttribute(QStringLiteral("href"), QStringLiteral("#"));
            m_writer.writeAttribute(QStringLiteral("onclick"), QStringLiteral("switchBase('structure'); return false;"));
            m_writer.writeCharacters(QCoreApplication::translate("HtmlInfo", "hex"));
            m_writer.writeEndElement();
            m_writer.writeEndElement();
            m_rowMaker.endRow();
            m_writer.writeEndElement();

            startTableSection(QStringLiteral("structure"));
            m_rowMaker.startRow(QString());
            switch(m_file.containerFormat()) {
            case ContainerFormat::Mp4:
            case ContainerFormat::QuickTime:
                mkElementTree(static_cast<Mp4Container *>(m_file.container()));
                break;
            case ContainerFormat::Matroska:
            case ContainerFormat::Webm:
            case ContainerFormat::Ebml:
                mkElementTree(static_cast<MatroskaContainer *>(m_file.container()));
                break;
            default:
                ;
            }
            m_rowMaker.endRow();
            m_writer.writeEndElement();
            break;
        default:
            ;
        }

        // notifications
        auto currentNotifications = m_file.gatherRelatedNotifications();
        mkNotifications(currentNotifications, !originalNotifications.empty());
        if(!originalNotifications.empty()) {
            mkNotifications(originalNotifications);
        }

        // </table> </body> </html>

        // finalize document
        m_writer.writeEndDocument();
    }

    const QByteArray &res()
    {
        return m_res;
    }

private:
    QXmlStreamWriter m_writer;
    QByteArray m_res;
    RowMaker m_rowMaker;
    const MediaFileInfo &m_file;
    NotificationList &originalNotifications;
};

/*!
 * \brief Generates technical information for the specified \a file.
 *
 * The parse methods of the \a file must have already been called.
 *
 * A QGuiApplication instance should be available for setting fonts.
 * A QApplication instance should be available for standard icons.
 */
QByteArray generateInfo(const MediaFileInfo &file, NotificationList &originalNotifications)
{
    Generator gen(file, originalNotifications);
    gen.mkDoc();
#ifdef QT_DEBUG
    QFile test(QStringLiteral("/tmp/test.xhtml"));
    test.open(QFile::WriteOnly | QFile::Truncate);
    test.write(gen.res());
#endif
    return gen.res();
}

}
