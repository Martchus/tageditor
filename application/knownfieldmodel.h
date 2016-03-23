#ifndef KNOWNFIELDSELECTION_H
#define KNOWNFIELDSELECTION_H

#include <qtutilities/models/checklistmodel.h>

#include <QAbstractListModel>
#include <QList>

namespace Media {
DECLARE_ENUM(KnownField, unsigned int)
}

namespace Settings {

class KnownFieldModel : public Models::ChecklistModel
{
    Q_OBJECT
public:
    enum class DefaultSelection
    {
        None,
        CommonFields
    };

    static const char *fieldName(Media::KnownField field);
    static QString translatedFieldName(Media::KnownField field);
    static Models::ChecklistItem mkItem(Media::KnownField field, Qt::CheckState checkState = Qt::Checked);

    explicit KnownFieldModel(QObject *parent = nullptr, DefaultSelection defaultSelection = DefaultSelection::None);
    explicit KnownFieldModel(const QList<Models::ChecklistItem> &items, QObject *parent = nullptr);

    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const;
    virtual QString labelForId(const QVariant &id) const;
};

inline Models::ChecklistItem KnownFieldModel::mkItem(Media::KnownField field, Qt::CheckState checkState)
{
    return Models::ChecklistItem(static_cast<int>(field), translatedFieldName(field), checkState);
}

}

#endif // KNOWNFIELDSELECTION_H
