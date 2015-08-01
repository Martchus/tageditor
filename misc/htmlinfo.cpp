#include "htmlinfo.h"

#include <tagparser/signature.h>
#include <tagparser/mediafileinfo.h>
#include <tagparser/abstractcontainer.h>
#include <tagparser/matroska/matroskacontainer.h>
#include <tagparser/matroska/matroskaeditionentry.h>
#include <tagparser/mp4/mp4container.h>
#include <tagparser/tag.h>
#include <tagparser/abstracttrack.h>
#include <tagparser/abstractattachment.h>
#include <tagparser/notification.h>

#include <qtutilities/resources/resources.h>

#include <c++utilities/chrono/timespan.h>
#include <c++utilities/chrono/datetime.h>
#include <c++utilities/conversion/stringconversion.h>

#if defined(GUI_QTWIDGETS)
#include <QApplication>
#include <QStyle>
#elif defined(GUI_QTQUICK)
#include <QGuiApplication>
#elif !defined(GUI_NONE)
#define GUI_NONE
#endif
#ifndef GUI_NONE
#include <QFont>
#include <QFontMetrics>
#include <QIcon>
#endif
#include <QString>
#include <QResource>
#include <QBuffer>
#include <QByteArray>

#include <list>

using namespace std;
using namespace ConversionUtilities;
using namespace ChronoUtilities;
using namespace Media;

namespace HtmlInfo {

inline QString qstr(const char *cstr)
{
    return QString::fromLocal8Bit(cstr);
}

inline QString qstr(const string &stdstr)
{
    return qstr(stdstr.c_str());
}

inline QString mkRow(const QString &label, const QString &text, bool head = true)
{

    return head
            ? QStringLiteral("<tr><th>%1</th><td>%2</td></tr>").arg(label, text)
            : QStringLiteral("<tr><td>%1</td><td>%2</td></tr>").arg(label, text);
}

inline QString mkRow(const QString &label, const QString &helpText, const QString &text)
{
    return QStringLiteral("<tr title=\"%1\"><th class=\"has-helptext\">%2</th><td>%3</td></tr>").arg(helpText, label, text);
}

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
    return QStringLiteral("font-family: \"%1\";"
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
    static const QByteArray data = mkBase64(QIcon(QStringLiteral("/images/bug")));
    return data;
}

QString mkMoreLink(const QString &id, const QString &text)
{
    return QStringLiteral(" <a href=\"#\" onclick=\"toggleVisibility(this, '%1'); return false;\">%2</a>").arg(id, text);
}

QString mkExtendedSection(const QString &id)
{
    return QStringLiteral("</tbody><tbody id=\"%1\" class=\"more\">").arg(id);
}

QString mkSection()
{
    return QStringLiteral("</tbody><tbody>");
}

void mkTrack(QByteArray &res, const AbstractTrack *track, unsigned int trackNumber)
{
    res.append(QStringLiteral("<tr><th>%1 #%2</th><td><table class=\"headervertical\"><tbody>").arg(QCoreApplication::translate("HtmlInfo", "Track"), QString::number(trackNumber)));
    if(track->id()) {
        res.append(mkRow(QCoreApplication::translate("HtmlInfo", "ID"), QCoreApplication::translate("HtmlInfo", "The unique number used to identify the track in the container file."), QString::number(track->id())));
    }
    if(track->trackNumber()) {
        res.append(mkRow(QCoreApplication::translate("HtmlInfo", "Number"), QCoreApplication::translate("HtmlInfo", "The index of the track in the container file."), QString::number(track->trackNumber())));
    }
    if(!track->name().empty()) {
        res.append(mkRow(QCoreApplication::translate("HtmlInfo", "Name"), qstr(track->name())));
    }
    res.append(mkRow(QCoreApplication::translate("HtmlInfo", "Type"), qstr(track->mediaTypeName())));
    const char *fmtName = track->formatName(), *fmtAbbr = track->formatAbbreviation();
    res.append(mkRow(QCoreApplication::translate("HtmlInfo", "Format"), QCoreApplication::translate("HtmlInfo", "The unabbreviated name of the track's format."), qstr(fmtName)));
    if(strcmp(fmtName, fmtAbbr)) {
        res.append(mkRow(QCoreApplication::translate("HtmlInfo", "Abbreviation"), QCoreApplication::translate("HtmlInfo", "The abbreviated name of the track's format."), qstr(fmtAbbr)));
    }
    if(!track->formatId().empty()) {
        res.append(mkRow(QCoreApplication::translate("HtmlInfo", "Format/codec ID"), QCoreApplication::translate("HtmlInfo", "The raw format/codec identifier extracted from the container."), qstr(track->formatId())));
    }
    if(track->version()) {
        res.append(mkRow(QCoreApplication::translate("HtmlInfo", "Version"), QString::number(track->version())));
    }
    if(track->size()) {
        res.append(mkRow(QCoreApplication::translate("HtmlInfo", "Size"), qstr(dataSizeToString(track->size()))));
    }
    if(!track->duration().isNull()) {
        res.append(mkRow(QCoreApplication::translate("HtmlInfo", "Duration"), qstr(track->duration().toString(TimeSpanOutputFormat::WithMeasures))));
    }
    if(track->bitrate()) {
        res.append(mkRow(QCoreApplication::translate("HtmlInfo", "Avg. bitrate"), qstr(bitrateToString(track->bitrate()))));
    }
    if(track->maxBitrate()) {
        res.append(mkRow(QCoreApplication::translate("HtmlInfo", "Maximum bitrate"), qstr(bitrateToString(track->maxBitrate()))));
    }
    if(!track->creationTime().isNull()) {
        res.append(mkRow(QCoreApplication::translate("HtmlInfo", "Creation time"), qstr(track->creationTime().toString(DateTimeOutputFormat::DateAndTime, true))));
    }
    if(!track->modificationTime().isNull()) {
        res.append(mkRow(QCoreApplication::translate("HtmlInfo", "Modification time"), qstr(track->modificationTime().toString(DateTimeOutputFormat::DateAndTime, true))));
    }
    if(!track->language().empty()) {
        res.append(mkRow(QCoreApplication::translate("HtmlInfo", "Language"), qstr(track->language())));
    }
    if(!track->compressorName().empty()) {
        res.append(mkRow(QCoreApplication::translate("HtmlInfo", "Compressor name"), qstr(track->compressorName())));
    }
    if(track->samplingFrequency()) {
        if(track->extensionSamplingFrequency()) {
            res.append(mkRow(QCoreApplication::translate("HtmlInfo", "Sampling frequency"), QStringLiteral("%1 Hz / %2 Hz").arg(track->extensionSamplingFrequency()).arg(track->samplingFrequency())));
        } else {
            res.append(mkRow(QCoreApplication::translate("HtmlInfo", "Sampling frequency"), QStringLiteral("%1 Hz").arg(track->samplingFrequency())));
        }
    }
    if(track->sampleCount()) {
        res.append(mkRow(QCoreApplication::translate("HtmlInfo", "Sample count"), QString::number(track->sampleCount())));
    }
    if(track->bitsPerSample()) {
        res.append(mkRow(QCoreApplication::translate("HtmlInfo", "Bits per sample"), QString::number(track->bitsPerSample())));
    }
    if(track->quality()) {
        res.append(mkRow(QCoreApplication::translate("HtmlInfo", "Quality"), QString::number(track->quality())));
    }
    if(!track->pixelSize().isNull()) {
        res.append(mkRow(QCoreApplication::translate("HtmlInfo", "Pixel size"), qstr(track->pixelSize().toString())));
    }
    if(!track->displaySize().isNull()) {
        res.append(mkRow(QCoreApplication::translate("HtmlInfo", "Display size"), qstr(track->displaySize().toString())));
    }
    if(!track->cropping().isNull()) {
        res.append(mkRow(QCoreApplication::translate("HtmlInfo", "Cropping"), qstr(track->cropping().toString())));
    }
    if(!track->resolution().isNull()) {
        res.append(mkRow(QCoreApplication::translate("HtmlInfo", "Resolution"), qstr(track->resolution().toString())));
    }
    if(track->channelConfigString()) {
        res.append(mkRow(QCoreApplication::translate("HtmlInfo", "Channel config"), QCoreApplication::translate("HtmlInfo", "Channel configuration"), QString::fromLocal8Bit(track->channelConfigString())));
    } else if(track->channelCount()) {
        res.append(mkRow(QCoreApplication::translate("HtmlInfo", "Channel count"), QString::number(track->channelCount())));
    }
    if(track->depth()) {
        res.append(mkRow(QCoreApplication::translate("HtmlInfo", "Bit depth"), QString::number(track->depth())));
    }
    if(track->fps()) {
        res.append(mkRow(QCoreApplication::translate("HtmlInfo", "Frames per second"), QString::number(track->fps())));
    }
    list<string> labels;
    if(track->isInterlaced()) {
       labels.push_back("interlaced");
    }
    if(!track->isEnabled()) {
       labels.push_back("disabled");
    }
    if(track->isDefault()) {
       labels.push_back("default");
    }
    if(track->isForced()) {
       labels.push_back("forced");
    }
    if(track->hasLacing()) {
       labels.push_back("has lacing");
    }
    if(track->isEncrypted()) {
       labels.push_back("encrypted");
    }
    if(labels.size()) {
        res.append(mkRow(QCoreApplication::translate("HtmlInfo", "Labeled as"), qstr(joinStrings(labels, ", "))));
    }
    res.append(QStringLiteral("</tbody></table></td></tr>"));
}

void mkAttachment(QByteArray &res, const AbstractAttachment *attachment, unsigned int attachmentNumber)
{
    res.append(QStringLiteral("<tr><th>%1 #%2</th><td><table class=\"headervertical\"><tbody>").arg(QCoreApplication::translate("HtmlInfo", "Attachment"), QString::number(attachmentNumber)));
    if(attachment->id()) {
        res.append(mkRow(QCoreApplication::translate("HtmlInfo", "ID"), QString::number(attachment->id())));
    }
    if(!attachment->name().empty()) {
        res.append(mkRow(QCoreApplication::translate("HtmlInfo", "Name"), qstr(attachment->name())));
    }
    if(attachment->data()) {
        res.append(mkRow(QCoreApplication::translate("HtmlInfo", "Size"), qstr(dataSizeToString(attachment->data()->size()))));
    }
    if(!attachment->mimeType().empty()) {
        res.append(mkRow(QCoreApplication::translate("HtmlInfo", "Mime-type"), qstr(attachment->mimeType())));
    }
    if(!attachment->description().empty()) {
        res.append(mkRow(QCoreApplication::translate("HtmlInfo", "Description"), qstr(attachment->description())));
    }
    res.append(QStringLiteral("</tbody></table></td></tr>"));
}
void mkChapter(QByteArray &res, const AbstractChapter &chapter, unsigned int chapterNumber, unsigned int level = 0)
{
    if(level) {
        res.append(QStringLiteral("<tr><th>%1 #%2<br />level %3</th><td><table class=\"headervertical\" style=\"margin-left: %4px\"><tbody>").arg(QCoreApplication::translate("HtmlInfo", "Chapter"), QString::number(chapterNumber + 1), QString::number(level + 1), QString::number(level * 20)));
    } else {
        res.append(QStringLiteral("<tr><th>%1 #%2</th><td><table class=\"headervertical\"><tbody>").arg(QCoreApplication::translate("HtmlInfo", "Chapter"), QString::number(chapterNumber + 1)));
    }
    if(chapter.id()) {
        res.append(mkRow(QCoreApplication::translate("HtmlInfo", "ID"), QString::number(chapter.id())));
    }
    for(const LocaleAwareString &name : chapter.names()) {
        static const string delim(", ");
        string locale = joinStrings(initializer_list<string>{joinStrings(name.languages(), delim, true), joinStrings(name.countries(), delim, true)}, delim, true);
        res.append(mkRow(QCoreApplication::translate("HtmlInfo", "Name (%1)").arg(qstr(locale)), qstr(name)));
    }
    if(chapter.startTime().totalTicks() > 0) {
        res.append(mkRow(QCoreApplication::translate("HtmlInfo", "Start time"), qstr(chapter.startTime().toString(TimeSpanOutputFormat::WithMeasures))));
    }
    if(chapter.endTime().totalTicks() > 0) {
        res.append(mkRow(QCoreApplication::translate("HtmlInfo", "End time"), qstr(chapter.endTime().toString(TimeSpanOutputFormat::WithMeasures))));
    }
    list<string> labels;
    if(chapter.isHidden()) {
       labels.push_back("hidden");
    }
    if(!chapter.isEnabled()) {
       labels.push_back("disabled");
    }
    if(labels.size()) {
        res.append(mkRow(QCoreApplication::translate("HtmlInfo", "Labeled as"), qstr(joinStrings(labels, ", "))));
    }
    if(chapter.tracks().size()) {
        QStringList trackIds;
        for(uint64 id : chapter.tracks()) {
            trackIds << QString::number(id);
        }
        res.append(mkRow(QCoreApplication::translate("HtmlInfo", "Tracks"), trackIds.join(QStringLiteral(", "))));
    }
    res.append(QStringLiteral("</tbody></table></td></tr>"));
    ++level;
    for(size_t i = 0, nestedChapters = chapter.nestedChapterCount(); i < nestedChapters; ++i) {
        mkChapter(res, *chapter.nestedChapter(i), i, level);
    }
}

void mkEdition(QByteArray &res, const MatroskaEditionEntry &edition, unsigned int editionNumber)
{
    res.append(QStringLiteral("<tr><th>%1 #%2</th><td><table class=\"headervertical\"><tbody>").arg(QCoreApplication::translate("HtmlInfo", "Edition"), QString::number(editionNumber + 1)));
    if(edition.id()) {
        res.append(mkRow(QCoreApplication::translate("HtmlInfo", "ID"), QString::number(edition.id())));
    }
    list<string> labels;
    if(edition.isHidden()) {
       labels.push_back("hidden");
    }
    if(edition.isDefault()) {
       labels.push_back("default");
    }
    if(edition.isOrdered()) {
       labels.push_back("ordered");
    }
    if(labels.size()) {
        res.append(mkRow(QCoreApplication::translate("HtmlInfo", "Labeled as"), qstr(joinStrings(labels, ", "))));
    }
    res.append(mkRow(QCoreApplication::translate("HtmlInfo", "Chapters"),
                     QCoreApplication::translate("HtmlInfo", "edition contains %1 chapter(s)", nullptr, edition.chapters().size()).arg(edition.chapters().size())));
    res.append(QStringLiteral("</tbody></table></td></tr>"));
    unsigned int chapterNumber = 0;
    for(const auto &chapter : edition.chapters()) {
        mkChapter(res, *chapter, chapterNumber);
        ++chapterNumber;
    }
}

template<class ElementType> void mkElementNode(QByteArray &res, const ElementType *element)
{
    if(element && element->parent()) {
        res.append(QStringLiteral("<ul class=\"nodecollapsed\">"));
    } else {
        res.append(QStringLiteral("<ul class=\"nodeexpanded\">"));
    }
    while(element) {
        if(element->isParsed()) {
            res.append(QStringLiteral("<li>"));
            if(element->firstChild()) {
                res.append(QStringLiteral("<a href=\"javascript: void(0)\" onclick=\"expandCollapse(this.parentElement);\">"));
            }
            res.append(QCoreApplication::translate("HtmlInfo", "<li><em>%1</em> @%2, size: %3").arg(
                           QString::fromLatin1(element->idToString().c_str()), QString::number(element->startOffset()),
                           QString::number(element->totalSize())));
            if(element->firstChild()) {
                res.append(QStringLiteral("</a>"));
                mkElementNode(res, element->firstChild());
            }
            res.append(QStringLiteral("</li>"));
            element = element->nextSibling();
        } else {
            res.append(QStringLiteral("<li style=\"color: red;\">"));
            res.append(QCoreApplication::translate("HtmlInfo", "section has not been analyzed"));
            res.append(QStringLiteral("</li>"));
            break;
        }
    }
    res.append(QStringLiteral("</ul>"));
}

template<class ContainerType> void mkElementTree(QByteArray &res, const ContainerType *container)
{
    mkElementNode(res, container->firstElement());
    for(const auto &element : container->additionalElements()) {
        mkElementNode(res, element.get());
    }
}

void mkNotifications(QByteArray &res, NotificationList &notifications, bool reparsing = false)
{
    if(notifications.size()) {
        res.append(mkSection());
        QString moreId = reparsing ? QStringLiteral("notificationsReparsingMore") : QStringLiteral("notificationsMore");
        res.append(mkRow(reparsing ? QCoreApplication::translate("HtmlInfo", "Notifications (reparsing)") : QCoreApplication::translate("HtmlInfo", "Notifications"),
                         QCoreApplication::translate("HtmlInfo", "%1 notification(s) available", 0, notifications.size()).arg(notifications.size())
                         + mkMoreLink(moreId, QCoreApplication::translate("HtmlInfo", "show notifications"))));
        res.append(mkExtendedSection(moreId));
        res.append(QStringLiteral("<tr><th></th><td><table class=\"headerhorizontal\"><thead><tr><th></th><th>%1</th><th>%2</th><th>%3</th></tr></thead><tbody>").arg(
                    QCoreApplication::translate("HtmlInfo", "Context"), QCoreApplication::translate("HtmlInfo", "Message"), QCoreApplication::translate("HtmlInfo", "Time")));
        Notification::sortByTime(notifications);
        for(const Notification &notification : notifications) {
            res.append(QStringLiteral("<tr><td class=\"%4\"></td><td>%1</td><td>%2</td><td>%3</td></tr>").arg(
                                  qstr(notification.context()), qstr(notification.message()),
                                  qstr(notification.creationTime().toString(DateTimeOutputFormat::DateAndTime, false)),
                                  qstr(notification.typeName())));
        }
        res.append(QStringLiteral("</tbody></table></td></tr>"));
    }
}

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
    QByteArray res;
    // header
    res.append(QStringLiteral("<!DOCTYPE html>\n"
                              "<html><head>"
                              "<title>Info for "));
    res.append(qstr(file.fileName()));
    res.append(QStringLiteral("</title>"
                              "<style type=\"text/css\">"
                              "html, body {"
                              "padding: 1px;"
                              "margin: 0px;"
                              "background-color: transparent;"));
#ifndef GUI_NONE
    if(ApplicationInstances::hasGuiApp()) {
        res.append(mkFontStyle(QGuiApplication::font()));
    }
#endif
    res.append(QStringLiteral("}"
                              "a:link {"
                              "color: #337AB7;"
                              "text-decoration: none;"
                              "}"
                              "a:focus, a:hover {"
                              "color: #23527c;"
                              "text-decoration: underline;"
                              "}"
                              "table {"
                              "border-collapse: collapse;"
                              "width: 100%;"
                              "}"
                              "tr:nth-child(2n+2) {"
                              "background-color:#fafafa;"
                              "}"
                              "table.headervertical th, table.headervertical td {"
                              "padding: 2px 5px;"
                              "text-align: left;"
                              "vertical-align: top;"
                              "}"
                              "table.headervertical > tbody > tr > th {"
                              "text-align: right;"
                              "border-right: 1px solid #eee;"
                              "border-bottom: none;"
                              "width: 25%;"
                              "}"
                              "table.headerhorizontal > thead > tr > th {"
                              "text-align: center;"
                              "border-right: none;"
                              "border-bottom: 1px solid #eee;"
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
                              "}"));
#ifdef GUI_QTWIDGETS
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
                              "</style>"
                              "<script type=\"text/javascript\">"
                              "function toggleVisibility(link, objid) {"
                              "var obj = document.getElementById(objid);"
                              "if(obj.style.display == \"none\" || (obj.style.display == \"\" && obj.className == \"more\")) {"
                              "if(obj.nodeName == \"TBODY\") {"
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
                              "if(children[i].tagName == 'UL') {"
                              "children[i].className = children[i].className == 'nodeexpanded' ? 'nodecollapsed' : 'nodeexpanded';"
                              "}}}"
                              "function expandAll(objid) {"
                              "var children = document.getElementById(objid).getElementsByTagName('ul');"
                              "for(var i = 0; i < children.length; ++i) {"
                              "children[i].className = 'nodeexpanded';"
                              "}}"
                              "function collapseAll(objid) {"
                              "var children = document.getElementById(objid).getElementsByTagName('ul');"
                              "for(var i = 0; i < children.length; ++i) {"
                              "if(children[i].parentNode.tagName == 'LI') {"
                              "children[i].className = 'nodecollapsed';"
                              "}}}"
                              "</script></head>"
                              "<body><table class=\"headervertical\"><tbody id=\"general\">"));
    // general information
    res.append(mkRow(QCoreApplication::translate("HtmlInfo", "Path"), qstr(file.path())));
    res.append(mkRow(QCoreApplication::translate("HtmlInfo", "Size"), qstr(dataSizeToString(file.size()))));
    TimeSpan duration = file.duration();
    if(!duration.isNull()) {
        res.append(mkRow(QCoreApplication::translate("HtmlInfo", "Duration"), qstr(duration.toString(TimeSpanOutputFormat::WithMeasures))));
        res.append(mkRow(QCoreApplication::translate("HtmlInfo", "Overall avg. bitrate"), qstr(bitrateToString(0.0078125 * file.size() / duration.totalSeconds()))));
    }
    const char *mimeType = file.mimeType();
    if(*mimeType) {
        res.append(mkRow(QCoreApplication::translate("HtmlInfo", "Mime-type"), qstr(mimeType)));
    }
    AbstractContainer *container = file.container();
    QString containerInfo = qstr(file.containerFormatName());
    const char *subversion = file.containerFormatSubversion();
    if(*subversion) {
        containerInfo.append(' ');
        containerInfo.append(qstr(subversion));
    }
    if(container || file.paddingSize()) {
        containerInfo.append(mkMoreLink(QStringLiteral("containerMore"), QCoreApplication::translate("HtmlInfo", "show details")));
    }
    res.append(mkRow(QCoreApplication::translate("HtmlInfo", "Container"), containerInfo));
    // container details
    if(container || file.paddingSize()) {
        res.append(mkExtendedSection(QStringLiteral("containerMore")));
    }
    if(container) {
        if(!container->title().empty()) {
            res.append(mkRow(QCoreApplication::translate("HtmlInfo", "Title"), qstr(container->title())));
        }
        if(container->version()) {
            res.append(mkRow(QCoreApplication::translate("HtmlInfo", "Version"), QString::number(container->version())));
        }
        if(container->readVersion()) {
            res.append(mkRow(QCoreApplication::translate("HtmlInfo", "Read version"), QString::number(container->readVersion())));
        }
        if(!container->documentType().empty()) {
            res.append(mkRow(QCoreApplication::translate("HtmlInfo", "Document type"), qstr(container->documentType())));
        }
        if(container->doctypeVersion()) {
            res.append(mkRow(QCoreApplication::translate("HtmlInfo", "Document version"), QString::number(container->doctypeVersion())));
        }
        if(container->doctypeReadVersion()) {
            res.append(mkRow(QCoreApplication::translate("HtmlInfo", "Document read version"), QString::number(container->doctypeReadVersion())));
        }
    }
    if(file.paddingSize()) {
        res.append(mkRow(QCoreApplication::translate("HtmlInfo", "Padding size"), QStringLiteral("%1 (%2 %)").arg(qstr(dataSizeToString(file.paddingSize()))).arg(static_cast<double>(file.paddingSize()) / file.size() * 100.0, 0, 'g', 2)));
    }
    // tags
    vector<Tag *> tags = file.tags();
    if(tags.size()) {
        res.append(mkSection());
        QString moreId = QStringLiteral("tagsMore");
        res.append(mkRow(QCoreApplication::translate("HtmlInfo", "Tags"), QCoreApplication::translate("HtmlInfo", "%1 tag(s) assigned, ", 0, tags.size()).arg(tags.size()) + mkMoreLink(moreId, QCoreApplication::translate("HtmlInfo", "show details"))));
        res.append(mkExtendedSection(moreId));
        for(const Tag *tag : tags) {
            res.append(QStringLiteral("<tr><th>%1</th><td><table class=\"headervertical\"><tbody>").arg(tag->typeName()));
            if(!tag->version().empty()) {
                res.append(mkRow(QCoreApplication::translate("HtmlInfo", "Version"), qstr(tag->version())));
            }
            if(tag->supportsTarget() && !tag->target().isEmpty()) {
                res.append(mkRow(QCoreApplication::translate("HtmlInfo", "Target level"), qstr(tag->target().toString())));
            }
            res.append(mkRow(QCoreApplication::translate("HtmlInfo", "Size"), qstr(dataSizeToString(tag->size()))));
            res.append(mkRow(QCoreApplication::translate("HtmlInfo", "Field count"), QString::number(tag->fieldCount())));
            res.append(QStringLiteral("</tbody></table></td></tr>"));
        }
    }
    // tracks
    auto tracks = file.tracks();
    if(tracks.size()) {
        res.append(mkSection());
        QString moreId = QStringLiteral("tracksMore");
        res.append(mkRow(QCoreApplication::translate("HtmlInfo", "Tracks"), QCoreApplication::translate("HtmlInfo", "file contains %1 track(s), ", 0, tracks.size()).arg(tracks.size())
                         + mkMoreLink(moreId, QCoreApplication::translate("HtmlInfo", "show details"))));
        res.append(mkExtendedSection(moreId));
        int trackNumber = 1;
        for(const auto *track : tracks) {
            mkTrack(res, track, trackNumber);
            ++trackNumber;
        }
    }
    auto attachments = file.attachments();
    if(attachments.size()) {
        res.append(mkSection());
        QString moreId = QStringLiteral("attachmentsMore");
        res.append(mkRow(QCoreApplication::translate("HtmlInfo", "Attachments"), QCoreApplication::translate("HtmlInfo", "file contains %1 attachment(s), ", 0, tracks.size()).arg(attachments.size())
                         + mkMoreLink(moreId, QCoreApplication::translate("HtmlInfo", "show details"))));
        res.append(mkExtendedSection(moreId));
        int attachmentNumber = 1;
        for(const auto *attachment : attachments) {
            mkAttachment(res, attachment, attachmentNumber);
            ++attachmentNumber;
        }
    }
    // chapters
    if(container) {
        if(file.containerFormat() == ContainerFormat::Matroska) {
            if(size_t editionCount = static_cast<const MatroskaContainer *>(container)->editionEntires().size()) {
                res.append(mkSection());
                QString moreId = QStringLiteral("editionsMore");
                res.append(mkRow(QCoreApplication::translate("HtmlInfo", "Editions/chapters"), QCoreApplication::translate("HtmlInfo", "file contains %1 edition(s), ", 0, editionCount).arg(editionCount)
                                 + mkMoreLink(moreId, QCoreApplication::translate("HtmlInfo", "show details"))));
                res.append(mkExtendedSection(moreId));
                unsigned int editionNumber = 0;
                for(const auto &edition : static_cast<const MatroskaContainer *>(container)->editionEntires()) {
                    mkEdition(res, *edition, editionNumber);
                    ++editionNumber;
                }
            }
        } else if(size_t chapterCount = container->chapterCount()) {
            res.append(mkSection());
            QString moreId = QStringLiteral("chaptersMore");
            res.append(mkRow(QCoreApplication::translate("HtmlInfo", "Chapters"), QCoreApplication::translate("HtmlInfo", "file contains %1 chapter(s), ", 0, chapterCount).arg(chapterCount)
                             + mkMoreLink(moreId, QCoreApplication::translate("HtmlInfo", "show details"))));
            res.append(mkExtendedSection(moreId));
            for(size_t i = 0; i < chapterCount; ++i) {
                mkChapter(res, *container->chapter(i), i);
            }
        }
    }
    // structure
    if(file.containerFormat() == ContainerFormat::Mp4 || file.containerFormat() == ContainerFormat::Matroska) {
        res.append(mkSection());
        res.append(mkRow(QCoreApplication::translate("HtmlInfo", "Structure"), QCoreApplication::translate("HtmlInfo", "<a href=\"javascript: expandAll('structure');\">expand all</a>, <a href=\"javascript: collapseAll('structure');\">collapse all</a>")));
        res.append(QStringLiteral("</tbody><tbody id=\"structure\"><tr><th></th><td>"));
        switch(file.containerFormat()) {
        case ContainerFormat::Mp4:
            mkElementTree(res, static_cast<Mp4Container *>(file.container()));
            break;
        case ContainerFormat::Matroska:
            mkElementTree(res, static_cast<MatroskaContainer *>(file.container()));
            break;
        default:
            ;
        }
        res.append("</td></tr>");
    }
    // notifications
    NotificationList currentNotifications = file.gatherRelatedNotifications();
    mkNotifications(res, currentNotifications, originalNotifications.size());
    if(originalNotifications.size()) {
        mkNotifications(res, originalNotifications);
    }
    // end
    res.append(QStringLiteral("</tbody></table></body></html>"));
    return res;
}

}
