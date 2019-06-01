#ifndef FILEINFOMODEL_H
#define FILEINFOMODEL_H

#include <QIcon>
#include <QStandardItemModel>

#include <list>

namespace TagParser {
class MediaFileInfo;
class Diagnostics;
} // namespace TagParser

namespace QtGui {

class FileInfoModel : public QStandardItemModel {
    Q_OBJECT
public:
    explicit FileInfoModel(QObject *parent = nullptr);

    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;

    const TagParser::MediaFileInfo *fileInfo() const;
    void setFileInfo(TagParser::MediaFileInfo &fileInfo, TagParser::Diagnostics &diag, TagParser::Diagnostics *diagReparsing = nullptr);

    static const QIcon &informationIcon();
    static const QIcon &warningIcon();
    static const QIcon &errorIcon();
    static const QIcon &debugIcon();

private:
    void updateCache();

private:
    TagParser::MediaFileInfo *m_file;
    TagParser::Diagnostics *m_diag;
    TagParser::Diagnostics *m_diagReparsing;
};

} // namespace QtGui

#endif // FILEINFOMODEL_H
