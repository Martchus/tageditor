#include "./pathlineedit.h"

#include <qtutilities/misc/desktoputils.h>

#include <QCompleter>
#include <QAbstractItemModel>
#include <QContextMenuEvent>
#include <QFileDialog>
#include <QFileSystemModel>
#include <QFileInfo>
#include <QMenu>

#include <memory>
#include <functional>

using namespace std;
using namespace Widgets;

namespace QtGui {

PathLineEdit::PathLineEdit(QWidget *parent) :
    ClearLineEdit(parent)
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
        const QString path = QFileDialog::getExistingDirectory(this);
        if(!path.isEmpty()) {
            editText(path);
        }
    });
    QFileInfo fileInfo(text());
    if(fileInfo.exists()) {
        if(fileInfo.isFile()) {
            connect(menu->addAction(tr("Open")), &QAction::triggered, bind(&DesktopUtils::openLocalFileOrDir, text()));
        } else if(fileInfo.isDir()) {
            connect(menu->addAction(tr("Explore")), &QAction::triggered, bind(&DesktopUtils::openLocalFileOrDir, text()));
        }
    }
    menu->exec(event->globalPos());
}

}
