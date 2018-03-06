#ifndef UTILITYFEATURES_H
#define UTILITYFEATURES_H

#include <tagparser/tagvalue.h>

#include <QString>

QT_FORWARD_DECLARE_CLASS(QDir)
QT_FORWARD_DECLARE_CLASS(QAbstractItemModel)
QT_FORWARD_DECLARE_CLASS(QModelIndex)

namespace Media {
class MediaFileInfo;
class Tag;
enum class ElementPosition;
}

namespace Utility {

const char *textEncodingToCodecName(Media::TagTextEncoding textEncoding);
QString tagValueToQString(const Media::TagValue &value);
QString dataToQString(const char *data, std::size_t dataSize, Media::TagTextEncoding encoding);
QString stringToQString(const std::string &value, Media::TagTextEncoding textEncoding);
std::string qstringToString(const QString &value, Media::TagTextEncoding textEncoding);
Media::TagValue qstringToTagValue(const QString &value, Media::TagTextEncoding textEncoding);
QString elementPositionToQString(Media::ElementPosition elementPosition);
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

}

#endif // UTILITYFEATURES_H
