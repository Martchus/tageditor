#ifndef PATHLINEEDIT_H
#define PATHLINEEDIT_H

#include <qtutilities/widgets/clearlineedit.h>

#include <QLineEdit>

QT_BEGIN_NAMESPACE
class QAbstractItemModel;
QT_END_NAMESPACE

namespace QtGui {

class PathLineEdit : public Widgets::ClearLineEdit
{
    Q_OBJECT
    Q_PROPERTY(QAbstractItemModel *completionModel READ completionModel WRITE setCompletionModel)

public:
    explicit PathLineEdit(QWidget *parent = nullptr);

    QAbstractItemModel *completionModel() const;
    void setCompletionModel(QAbstractItemModel *model);

    void editText(const QString &text);

protected:
    virtual void contextMenuEvent(QContextMenuEvent *event);
    
};

}

#endif // PATHLINEEDIT_H
