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
    static Models::ChecklistItem mkItem(Media::KnownField field, Qt::CheckState checkState);

    explicit KnownFieldModel(QObject *parent = nullptr, DefaultSelection defaultSelection = DefaultSelection::None);

    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const;
    virtual QString labelForId(const QVariant &id) const;
};

}

#endif // KNOWNFIELDSELECTION_H
