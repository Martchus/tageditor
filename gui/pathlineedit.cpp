#include "pathlineedit.h"

#include <memory>

#include <QCompleter>
#include <QAbstractItemModel>
#include <QContextMenuEvent>
#include <QFileDialog>
#include <QDesktopServices>
#include <QFileSystemModel>
#include <QMenu>

using namespace std;
using namespace Widgets;

namespace QtGui {

/*
    TRANSLATOR QtGui::PathLineEdit
    Necessary for lupdate.
*/

PathLineEdit::PathLineEdit(QWidget *parent) :
    ClearLineEdit(parent)
{}

PathLineEdit::~PathLineEdit()
{}

QAbstractItemModel *PathLineEdit::completionModel() const
{
    if(QCompleter *c = completer()) {
        return c->model();
    } else {
        return nullptr;
    }
}

void PathLineEdit::setCompletionModel(QAbstractItemModel *model)
{
    QCompleter *c = completer();
    if(!c) {
        c = new QCompleter(this);
        c->setCompletionMode(QCompleter::PopupCompletion);
        setCompleter(c);
    }
    c->setModel(model);
}

void PathLineEdit::editText(const QString &text)
{
    setText(text);
    emit textEdited(text);
}

void PathLineEdit::contextMenuEvent(QContextMenuEvent *event)
{
    unique_ptr<QMenu> menu(createStandardContextMenu());
    menu->addSeparator();
    connect(menu->addAction(tr("Browse")), &QAction::triggered, [this] {
        QString path = QFileDialog::getExistingDirectory(this);
        if(!path.isEmpty()) {
            editText(path);
        }
    });
    connect(menu->addAction(tr("Explore")), &QAction::triggered, [this] {
        QDesktopServices::openUrl(QUrl(QStringLiteral("file://") + text(), QUrl::TolerantMode));
    });
    menu->exec(event->globalPos());
}

}
