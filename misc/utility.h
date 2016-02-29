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
}

namespace Utility {

const char *textEncodingToCodecName(Media::TagTextEncoding textEncoding);
QString tagValueToQString(const Media::TagValue &value);
QString dataToQString(const char *data, std::size_t dataSize, Media::TagTextEncoding encoding);
QString stringToQString(const std::string &value, Media::TagTextEncoding textEncoding);
std::string qstringToString(const QString &value, Media::TagTextEncoding textEncoding);
Media::TagValue qstringToTagValue(const QString &value, Media::TagTextEncoding textEncoding);
QString formatName(const QString &str, bool underscoreToWhitespace = true);
QString fixUmlauts(const QString &str);
void parseFileName(const QString &fileName, QString &title, int &trackNumber);
int removeBackupFiles(const QDir &directory, QStringList &affectedFiles, std::ostream *log = nullptr, bool recursive = false);
QString printModel(QAbstractItemModel *model);
void printModelIndex(const QModelIndex &index, QString &res, int level);

}

#endif // UTILITYFEATURES_H
