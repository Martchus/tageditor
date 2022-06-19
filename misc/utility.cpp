#include "./utility.h"

#include "../application/settings.h"

#include <tagparser/exceptions.h>
#include <tagparser/id3/id3v1tag.h>
#include <tagparser/id3/id3v2tag.h>
#include <tagparser/mediafileinfo.h>
#include <tagparser/signature.h>
#include <tagparser/tag.h>

#include <qtutilities/misc/compat.h>

#include <c++utilities/conversion/binaryconversion.h>
#include <c++utilities/conversion/stringconversion.h>
#include <c++utilities/io/path.h>

#include <QAbstractItemModel>
#include <QCoreApplication>
#include <QDir>
#include <QDirIterator>
#include <QFileInfo>

#include <ios>
#include <iostream>
#include <stdexcept>

using namespace std;
using namespace TagParser;

namespace Utility {

const char *textEncodingToCodecName(TagTextEncoding textEncoding)
{
    switch (textEncoding) {
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
    }
    return nullptr;
}

QString tagValueToQString(const TagValue &value)
{
    if (value.isEmpty()) {
        return QString();
    }
    switch (value.type()) {
    case TagDataType::Text:
        return dataToQString(value.dataPointer(), value.dataSize(), value.dataEncoding());
    case TagDataType::Integer:
        return QString::number(value.toInteger());
    case TagDataType::StandardGenreIndex:
    case TagDataType::TimeSpan:
    case TagDataType::DateTime:
    case TagDataType::PositionInSet:
    case TagDataType::Popularity:
        return QString::fromStdString(value.toString());
    default:;
    }
    return QString();
}

QString dataToQString(const char *data, size_t dataSize, TagTextEncoding encoding)
{
    if (!data || !dataSize) {
        return QString();
    }

    switch (encoding) {
    case TagTextEncoding::Latin1:
    case TagTextEncoding::Unspecified:
        return QString::fromLatin1(data, static_cast<int>(dataSize));
    case TagTextEncoding::Utf8:
        return QString::fromUtf8(data, static_cast<int>(dataSize));
#if defined(CONVERSION_UTILITIES_BYTE_ORDER_LITTLE_ENDIAN)
    case TagTextEncoding::Utf16LittleEndian:
#else
    case TagTextEncoding::Utf16BigEndian:
#endif
        return QString::fromUtf16(
            reinterpret_cast<const QtUtilities::Utf16CharType *>(data), static_cast<int>(dataSize / (sizeof(ushort) / sizeof(char))));
    default:;
    }

    const auto utf16Data = CppUtilities::convertString(textEncodingToCodecName(encoding),
#if defined(CONVERSION_UTILITIES_BYTE_ORDER_LITTLE_ENDIAN)
        "UTF-16LE",
#else
        "UTF-16BE",
#endif
        data, dataSize, 2.0f);
    return QString::fromUtf16(reinterpret_cast<const QtUtilities::Utf16CharType *>(utf16Data.first.get()),
        static_cast<int>(utf16Data.second / (sizeof(ushort) / sizeof(char))));
}

QString stringToQString(const string &value, TagTextEncoding textEncoding)
{
    return dataToQString(value.data(), value.size(), textEncoding);
}

string qstringToString(const QString &value, TagTextEncoding textEncoding)
{
    if (value.isEmpty()) {
        return string();
    }
    QByteArray encodedString;
    switch (textEncoding) {
    case TagTextEncoding::Latin1:
    case TagTextEncoding::Unspecified:
        encodedString = value.toLatin1();
        break;
    case TagTextEncoding::Utf8:
        encodedString = value.toUtf8();
        break;
#if defined(CONVERSION_UTILITIES_BYTE_ORDER_LITTLE_ENDIAN)
    case TagTextEncoding::Utf16LittleEndian:
#else
    case TagTextEncoding::Utf16BigEndian:
#endif
        encodedString = QByteArray(reinterpret_cast<const char *>(value.utf16()),
            static_cast<QByteArray::size_type>(value.size()) * static_cast<QByteArray::size_type>(sizeof(ushort) / sizeof(char)));
        break;
#if defined(CONVERSION_UTILITIES_BYTE_ORDER_LITTLE_ENDIAN)
    case TagTextEncoding::Utf16BigEndian: {
#else
    case TagTextEncoding::Utf16LittleEndian: {
#endif
        const auto utf16Data = CppUtilities::convertString(
#if defined(CONVERSION_UTILITIES_BYTE_ORDER_LITTLE_ENDIAN)
            "UTF-16LE",
#else
            "UTF-16BE",
#endif
            textEncodingToCodecName(textEncoding), reinterpret_cast<const char *>(value.utf16()),
            static_cast<std::size_t>(value.size()) * (sizeof(ushort) / sizeof(char)), 2.0f);
        return string(utf16Data.first.get(), utf16Data.second);
    }
    }
    return string(encodedString.data(), static_cast<string::size_type>(encodedString.size()));
}

TagValue qstringToTagValue(const QString &value, TagTextEncoding textEncoding)
{
    return value.isEmpty() ? TagValue() : TagValue(qstringToString(value, textEncoding), textEncoding);
}

QString elementPositionToQString(ElementPosition elementPosition)
{
    switch (elementPosition) {
    case ElementPosition::BeforeData:
        return QCoreApplication::translate("Utility", "before data");
    case ElementPosition::AfterData:
        return QCoreApplication::translate("Utility", "after data");
    case ElementPosition::Keep:;
    }
    return QString();
}

QString formatName(const QString &str, bool underscoreToWhitespace)
{
    QString res;
    res.reserve(str.size());
    bool whitespace = true;
    for (int i = 0, size = str.size(); i != size; ++i) {
        const QChar current = str.at(i);
        if (current.isSpace() || current == QChar('(') || current == QChar('[')) {
            whitespace = true;
            res += current;
        } else if (underscoreToWhitespace && current == QChar('_')) {
            whitespace = true;
            res += ' ';
        } else if (whitespace) {
            if (i) {
                auto rest = QtUtilities::midRef(str, i);
                static const char *const connectingWords[]
                    = { "the ", "a ", "an ", "of ", "or ", "and ", "in ", "to ", "at ", "on ", "as ", "vs ", "vs. " };
                for (const char *word : connectingWords) {
                    if (rest.startsWith(QLatin1String(word), Qt::CaseInsensitive)) {
                        res += current.toLower();
                        whitespace = false;
                        break;
                    }
                }
            }
            if (whitespace) {
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
    static const QLatin1String exceptions[] = { QLatin1String("reggae"), QLatin1String("blues"), QLatin1String("auer"), QLatin1String("aues"),
        QLatin1String("manuel"), QLatin1String("duet"), QLatin1String("duel"), QLatin1String("neue"), QLatin1String("prologue") };
    static const QLatin1String pairs[6][2] = { { QLatin1String("ae"), QLatin1String("\xe4") }, { QLatin1String("ue"), QLatin1String("\xfc") },
        { QLatin1String("oe"), QLatin1String("\xf6") }, { QLatin1String("Ae"), QLatin1String("\xc4") },
        { QLatin1String("Ue"), QLatin1String("\xdc") }, { QLatin1String("Oe"), QLatin1String("\xd6") } };
    for (auto &word : words) {
        // preserve words containing any of the exceptions
        for (const auto &exception : exceptions) {
            if (word.contains(exception, Qt::CaseInsensitive)) {
                goto continueOuterLoop;
            }
        }
        // fix all umlauts
        for (const auto *pair : pairs) {
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
    if (lastPoint > 0) {
        title.truncate(lastPoint);
    } else if (lastPoint == 0) {
        title.remove(0, 1);
    }
    static const QLatin1String delims[] = { QLatin1String(" - "), QLatin1String(", "), QLatin1String("-"), QLatin1String(" ") };
    for (const auto &delim : delims) {
        int lastDelimIndex = 0;
        int delimIndex = title.indexOf(delim);
        while (delimIndex > lastDelimIndex) {
            bool ok = false;
            trackNumber = QtUtilities::midRef(title, lastDelimIndex, delimIndex - lastDelimIndex).toInt(&ok);
            if (ok) {
                int titleStart = delimIndex + delim.size();
                for (const auto &delim2 : delims) {
                    if (QtUtilities::midRef(title, titleStart).startsWith(delim2)) {
                        titleStart += delim2.size();
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
    if (index.isValid()) {
        const auto data = index.data().toString();
        if (!data.isEmpty()) {
            switch (index.column()) {
            case 0:
                for (int i = 0; i < level; ++i) {
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
        const auto child = index.model()->index(0, 0, index);
        const auto next = index.sibling(index.row() + 1, 0);
        if (nextInCol.isValid()) {
            printModelIndex(nextInCol, res, level);
        } else {
            res += "\n";
        }
        if (index.column() == 0) {
            printModelIndex(child, res, level + 1);
            printModelIndex(next, res, level);
        }
    }
}

} // namespace Utility
