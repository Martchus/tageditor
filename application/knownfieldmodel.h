#ifndef KNOWNFIELDMODEL_H
#define KNOWNFIELDMODEL_H

#if defined(TAGEDITOR_GUI_QTWIDGETS) || defined(TAGEDITOR_GUI_QTQUICK)
#include <QAbstractListModel>
#include <QList>
#include <qtutilities/models/checklistmodel.h>
#else
#include <c++utilities/application/global.h>
#define Q_OBJECT
#endif

namespace TagParser {
DECLARE_ENUM_CLASS(KnownField, unsigned int);
}

namespace Settings {

class KnownFieldModel
#if defined(TAGEDITOR_GUI_QTWIDGETS) || defined(TAGEDITOR_GUI_QTQUICK)
    : public Models::ChecklistModel
#endif
{
    Q_OBJECT
public:
    enum class DefaultSelection { None, CommonFields };

    static const char *fieldName(TagParser::KnownField field);

#if defined(TAGEDITOR_GUI_QTWIDGETS) || defined(TAGEDITOR_GUI_QTQUICK)
    static QString translatedFieldName(TagParser::KnownField field);
    static Models::ChecklistItem mkItem(TagParser::KnownField field, Qt::CheckState checkState = Qt::Checked);

    explicit KnownFieldModel(QObject *parent = nullptr, DefaultSelection defaultSelection = DefaultSelection::None);
    explicit KnownFieldModel(const QList<Models::ChecklistItem> &items, QObject *parent = nullptr);

    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;
    QString labelForId(const QVariant &id) const override;
#endif
};

#if defined(TAGEDITOR_GUI_QTWIDGETS) || defined(TAGEDITOR_GUI_QTQUICK)
inline Models::ChecklistItem KnownFieldModel::mkItem(TagParser::KnownField field, Qt::CheckState checkState)
{
    return Models::ChecklistItem(static_cast<int>(field), translatedFieldName(field), checkState);
}
#endif

} // namespace Settings

#endif // KNOWNFIELDMODEL_H
