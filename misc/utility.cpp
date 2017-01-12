#include "./utility.h"

#include "../application/settings.h"

#include <tagparser/exceptions.h>
#include <tagparser/signature.h>
#include <tagparser/mediafileinfo.h>
#include <tagparser/tag.h>
#include <tagparser/id3/id3v1tag.h>
#include <tagparser/id3/id3v2tag.h>

#include <c++utilities/io/path.h>

#include <QDir>
#include <QFileInfo>
#include <QDirIterator>
#include <QTextCodec>
#include <QAbstractItemModel>

#include <ios>
#include <stdexcept>
#include <iostream>

using namespace std;
using namespace Media;

namespace Utility
{

const char *textEncodingToCodecName(TagTextEncoding textEncoding)
{
    switch(textEncoding) {
    case TagTextEncoding::Latin1:
        return "ISO 8859-1";
    case TagTextEncoding::Utf8:
        return "UTF-8";
    case TagTextEncoding::Utf16BigEndian:
        return "UTF-16BE";
    case TagTextEncoding::Utf16LittleEndian:
        return "UTF-16LE";
    case TagTextEncoding::Unspecified:
        return "ISO 8859-1"; // assumption
    default:
        return nullptr;
    }
}

QString tagValueToQString(const TagValue &value)
{
    if(!value.isEmpty()) {
        switch(value.type()) {
        case TagDataType::Text:
            return dataToQString(value.dataPointer(), value.dataSize(), value.dataEncoding());
        case TagDataType::Integer:
            return QStringLiteral("%1").arg(value.toInteger());
        case TagDataType::StandardGenreIndex:
        case TagDataType::TimeSpan:
        case TagDataType::PositionInSet:
            return QString::fromUtf8(value.toString().c_str());
        default:
            ;
        }
    }
    return QString();
}

QString dataToQString(const char *data, size_t dataSize, TagTextEncoding encoding)
{
    if(data && dataSize) {
        const char *codecName = textEncodingToCodecName(encoding);
        auto *codec = QTextCodec::codecForName(codecName);
        if(!codec) {
            codec = QTextCodec::codecForLocale();
        }
        return codec->toUnicode(data, static_cast<int>(dataSize));
    }
    return QString();
}

QString stringToQString(const string &value, TagTextEncoding textEncoding)
{
    if(!value.empty()) {
        const char *codecName = textEncodingToCodecName(textEncoding);
        auto *codec = QTextCodec::codecForName(codecName);
        if(!codec) {
            codec = QTextCodec::codecForLocale();
        }
        return codec->toUnicode(value.c_str());
    }
    return QString();
}

string qstringToString(const QString &value, TagTextEncoding textEncoding)
{
    if(!value.isEmpty()) {
        const char *codecName = textEncodingToCodecName(textEncoding);
        auto *codec = QTextCodec::codecForName(codecName);
        if(!codec) {
            codec = QTextCodec::codecForLocale();
        }
        const auto encodedString = codec->fromUnicode(value);
        return string(encodedString.data(), encodedString.size());
    }
    return string();
}

TagValue qstringToTagValue(const QString &value, TagTextEncoding textEncoding)
{
    return value.isEmpty() ? TagValue() : TagValue(qstringToString(value, textEncoding), textEncoding);
}

QString formatName(const QString &str, bool underscoreToWhitespace)
{
    QString res;
    bool whitespace = true;
    for(int i = 0, size = str.size(); i != size; ++i) {
        const QChar current = str.at(i);
        if(current.isSpace() || current == QChar('(') || current == QChar('[')) {
            whitespace = true;
            res += current;
        } else if(underscoreToWhitespace && current == QChar('_')) {
            whitespace = true;
            res += ' ';
        } else if(whitespace) {
            if(i) {
                auto rest = str.midRef(i);
                static const char *const connectingWords[] = {"the ", "a ", "an ", "of ", "or ", "and ", "in ", "to ", "at ", "on ", "as "};
                for(const char *word : connectingWords) {
                    if(rest.startsWith(QLatin1String(word), Qt::CaseInsensitive)) {
                        res += current.toLower();
                        whitespace = false;
                        break;
                    }
                }
            }
            if(whitespace) {
                res += current.toUpper();
                whitespace = false;
            }
        } else {
            res += current.toLower();
        }
    }
    return res;
}

QString fixUmlauts(const QString &str)
{
    auto words = str.split(QChar(' '));
    static const QLatin1String exceptions[] = {
        QLatin1String("reggae"), QLatin1String("blues"), QLatin1String("auer"), QLatin1String("aues"), QLatin1String("manuel"),
        QLatin1String("duet"), QLatin1String("neue"), QLatin1String("prologue")
    };
    static const QLatin1String pairs[6][2] = {
        {QLatin1String("ae"), QLatin1String("\xe4")}, {QLatin1String("ue"), QLatin1String("\xfc")}, {QLatin1String("oe"), QLatin1String("\xf6")},
        {QLatin1String("Ae"), QLatin1String("\xc4")}, {QLatin1String("Ue"), QLatin1String("\xdc")}, {QLatin1String("Oe"), QLatin1String("\xd6")}
    };
    for(auto &word : words) {
        // preserve words containing any of the exceptions
        for(const auto &exception : exceptions) {
            if(word.contains(exception, Qt::CaseInsensitive)) {
                goto continueOuterLoop;
            }
        }
        // fix all umlauts
        for(const auto *pair : pairs) {
            word = word.replace(pair[0], pair[1], Qt::CaseSensitive);
        }
        continueOuterLoop:;
    }
    return words.join(' ');
}

void parseFileName(const QString &fileName, QString &title, int &trackNumber)
{
    title = fileName.trimmed();
    trackNumber = 0;
    int lastPoint = title.lastIndexOf(QChar('.'));
    if(lastPoint > 0) {
        title.truncate(lastPoint);
    } else if(lastPoint == 0) {
        title.remove(0, 1);
    }
    static const QLatin1String delims[] = {
        QLatin1String(" - "), QLatin1String(", "), QLatin1String("-"), QLatin1String(" ")
    };
    for(const auto &delim : delims) {
        int lastDelimIndex = 0;
        int delimIndex = title.indexOf(delim);
        while(delimIndex > lastDelimIndex) {
            bool ok = false;
            trackNumber = title.midRef(lastDelimIndex, delimIndex - lastDelimIndex).toInt(&ok);
            if(ok) {
                int titleStart = delimIndex + delim.size();
                for(const auto &delim : delims) {
                    if(title.midRef(titleStart).startsWith(delim)) {
                        titleStart += delim.size();
                        break;
                    }
                }
                title.remove(0, titleStart);
            } else {
                delimIndex = title.indexOf(delim, lastDelimIndex = delimIndex + delim.size());
            }
        }
    }
}

QString printModel(QAbstractItemModel *model)
{
    const auto index = model->index(0, 0);
    QString res;
    printModelIndex(index, res, 0);
    return res;
}

void printModelIndex(const QModelIndex &index, QString &res, int level)
{
    if(index.isValid()) {
        const auto data = index.data().toString();
        if(!data.isEmpty()) {
            switch(index.column()) {
            case 0:
                for(int i = 0; i < level; ++i) {
                    res += "\t";
                }
                break;
            default:
                res += "\t";
                break;
            }
            res += data;
        }
        const auto nextInCol = index.sibling(index.row(), index.column() + 1);
        const auto child = index.child(0, 0);
        const auto next = index.sibling(index.row() + 1, 0);
        if(nextInCol.isValid()) {
            printModelIndex(nextInCol, res, level);
        } else {
            res += "\n";
        }
        if(index.column() == 0) {
            printModelIndex(child, res, level + 1);
            printModelIndex(next, res, level);
        }
    }
}

}
