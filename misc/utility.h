#ifndef UTILITYFEATURES_H
#define UTILITYFEATURES_H

#include <tagparser/tagvalue.h>

#include <QString>

QT_FORWARD_DECLARE_CLASS(QDir)
QT_FORWARD_DECLARE_CLASS(QAbstractItemModel)
QT_FORWARD_DECLARE_CLASS(QModelIndex)

namespace TagParser {
class MediaFileInfo;
class Tag;
enum class ElementPosition;
} // namespace TagParser

namespace Utility {

const char *textEncodingToCodecName(TagParser::TagTextEncoding textEncoding);
QString tagValueToQString(const TagParser::TagValue &value);
QString dataToQString(const char *data, std::size_t dataSize, TagParser::TagTextEncoding encoding);
QString stringToQString(const std::string &value, TagParser::TagTextEncoding textEncoding);
std::string qstringToString(const QString &value, TagParser::TagTextEncoding textEncoding);
TagParser::TagValue qstringToTagValue(const QString &value, TagParser::TagTextEncoding textEncoding);
QString elementPositionToQString(TagParser::ElementPosition elementPosition);
QString formatName(const QString &str, bool underscoreToWhitespace = true);
QString fixUmlauts(const QString &str);
void parseFileName(const QString &fileName, QString &title, int &trackNumber);
QString printModel(QAbstractItemModel *model);
void printModelIndex(const QModelIndex &index, QString &res, int level);

constexpr int sizeToInt(std::size_t size)
{
    return size > std::numeric_limits<int>::max() ? std::numeric_limits<int>::max() : static_cast<int>(size);
}

constexpr int trQuandity(quint64 quandity)
{
    return quandity > std::numeric_limits<int>::max() ? std::numeric_limits<int>::max() : static_cast<int>(quandity);
}

} // namespace Utility

#endif // UTILITYFEATURES_H
