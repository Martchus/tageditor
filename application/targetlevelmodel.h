#ifndef TARGETLEVELMODEL_H
#define TARGETLEVELMODEL_H

#include <qtutilities/models/checklistmodel.h>

#include <QAbstractListModel>
#include <QList>

#define TAGEDITOR_ENUM_CLASS enum class
namespace TagParser {
TAGEDITOR_ENUM_CLASS TagTargetLevel : unsigned char;
}
#undef TAGEDITOR_ENUM_CLASS

namespace Settings {

class TargetLevelModel : public QtUtilities::ChecklistModel {
    Q_OBJECT
public:
    enum class DefaultSelection { None, MostUsefulTargets };

    static const char *fieldName(TagParser::TagTargetLevel targetLevel);
    static QString translatedFieldName(TagParser::TagTargetLevel targetLevel);
    static QtUtilities::ChecklistItem mkItem(TagParser::TagTargetLevel targetLevel, Qt::CheckState checkState = Qt::Checked);

    explicit TargetLevelModel(QObject *parent = nullptr, DefaultSelection defaultSelection = DefaultSelection::None);
    explicit TargetLevelModel(const QList<QtUtilities::ChecklistItem> &items, QObject *parent = nullptr);

    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const;
    virtual QString labelForId(const QVariant &id) const;
};

inline QtUtilities::ChecklistItem TargetLevelModel::mkItem(TagParser::TagTargetLevel field, Qt::CheckState checkState)
{
    return QtUtilities::ChecklistItem(static_cast<int>(field), translatedFieldName(field), checkState);
}

} // namespace Settings

#endif // TARGETLEVELMODEL_H
