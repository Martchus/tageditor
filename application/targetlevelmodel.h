#ifndef TARGETLEVELMODEL_H
#define TARGETLEVELMODEL_H

#include <qtutilities/models/checklistmodel.h>

#include <QAbstractListModel>
#include <QList>

namespace Media {
DECLARE_ENUM_CLASS(TagTargetLevel, unsigned char);
}

namespace Settings {

class TargetLevelModel : public Models::ChecklistModel
{
    Q_OBJECT
public:
    enum class DefaultSelection
    {
        None,
        MostUsefulTargets
    };

    static const char *fieldName(Media::TagTargetLevel targetLevel);
    static QString translatedFieldName(Media::TagTargetLevel targetLevel);
    static Models::ChecklistItem mkItem(Media::TagTargetLevel targetLevel, Qt::CheckState checkState = Qt::Checked);

    explicit TargetLevelModel(QObject *parent = nullptr, DefaultSelection defaultSelection = DefaultSelection::None);
    explicit TargetLevelModel(const QList<Models::ChecklistItem> &items, QObject *parent = nullptr);

    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const;
    virtual QString labelForId(const QVariant &id) const;
};

inline Models::ChecklistItem TargetLevelModel::mkItem(Media::TagTargetLevel field, Qt::CheckState checkState)
{
    return Models::ChecklistItem(static_cast<int>(field), translatedFieldName(field), checkState);
}

}

#endif // TARGETLEVELMODEL_H
