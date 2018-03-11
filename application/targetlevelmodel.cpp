#include "./targetlevelmodel.h"

#include <tagparser/tagtarget.h>

using namespace Models;
using namespace TagParser;

namespace Settings {

/*
    TRANSLATOR Settings::TargetLevelModel
    Necessary for lupdate.
*/

const char *TargetLevelModel::fieldName(TagTargetLevel targetLevel)
{
    switch (targetLevel) {
    case TagTargetLevel::Unspecified:
        return QT_TR_NOOP("unspecified, everything");
    case TagTargetLevel::Shot:
        return QT_TR_NOOP("shot");
    case TagTargetLevel::Subtrack:
        return QT_TR_NOOP("subtrack, part, movement, scene");
    case TagTargetLevel::Track:
        return QT_TR_NOOP("track, song, chapter");
    case TagTargetLevel::Part:
        return QT_TR_NOOP("part, session");
    case TagTargetLevel::Album:
        return QT_TR_NOOP("album, opera, concert, movie, episode");
    case TagTargetLevel::Edition:
        return QT_TR_NOOP("edition, issue, volume, opus, season, sequel");
    case TagTargetLevel::Collection:
        return QT_TR_NOOP("collection");
    default:
        return QT_TR_NOOP("unknown");
    }
}

QString TargetLevelModel::translatedFieldName(TagTargetLevel targetLevel)
{
    return tr(fieldName(targetLevel));
}

QString TargetLevelModel::labelForId(const QVariant &id) const
{
    return translatedFieldName(static_cast<TagTargetLevel>(id.toInt()));
}

TargetLevelModel::TargetLevelModel(QObject *parent, DefaultSelection defaultSelection)
    : ChecklistModel(parent)
{
    QList<ChecklistItem> items;
    items.reserve(8);
    Qt::CheckState defaultSelected = defaultSelection == DefaultSelection::MostUsefulTargets ? Qt::Checked : Qt::Unchecked;
    items << mkItem(TagTargetLevel::Unspecified, defaultSelected);
    items << mkItem(TagTargetLevel::Shot, Qt::Unchecked);
    items << mkItem(TagTargetLevel::Subtrack, Qt::Unchecked);
    items << mkItem(TagTargetLevel::Track, defaultSelected);
    items << mkItem(TagTargetLevel::Part, Qt::Unchecked);
    items << mkItem(TagTargetLevel::Album, Qt::Unchecked);
    items << mkItem(TagTargetLevel::Edition, Qt::Unchecked);
    items << mkItem(TagTargetLevel::Collection, Qt::Unchecked);
    setItems(items);
}

TargetLevelModel::TargetLevelModel(const QList<Models::ChecklistItem> &items, QObject *parent)
    : ChecklistModel(parent)
{
    setItems(items);
}

QVariant TargetLevelModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    switch (orientation) {
    case Qt::Horizontal:
        switch (role) {
        case Qt::DisplayRole:
            switch (section) {
            case 0:
                return tr("Target level");
            default:;
            }
        }
        break;
    default:;
    }
    return QVariant();
}

} // namespace Settings
