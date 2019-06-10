#ifndef PATHLINEEDIT_H
#define PATHLINEEDIT_H

#include <qtutilities/widgets/clearlineedit.h>

#include <QLineEdit>

QT_BEGIN_NAMESPACE
class QAbstractItemModel;
QT_END_NAMESPACE

namespace QtGui {

class PathLineEdit : public QtUtilities::ClearLineEdit {
    Q_OBJECT
    Q_PROPERTY(QAbstractItemModel *completionModel READ completionModel WRITE setCompletionModel)

public:
    explicit PathLineEdit(QWidget *parent = nullptr);

    QAbstractItemModel *completionModel() const;
    void setCompletionModel(QAbstractItemModel *model);

    void editText(const QString &text);

protected:
    void contextMenuEvent(QContextMenuEvent *event) override;
};

} // namespace QtGui

#endif // PATHLINEEDIT_H
