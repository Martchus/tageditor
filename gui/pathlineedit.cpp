#include "./pathlineedit.h"

#include <qtutilities/misc/desktoputils.h>

#include <QAbstractItemModel>
#include <QCompleter>
#include <QContextMenuEvent>
#include <QFileDialog>
#include <QFileInfo>
#include <QFileSystemModel>
#include <QMenu>

#include <functional>
#include <memory>

using namespace std;
using namespace QtUtilities;

namespace QtGui {

PathLineEdit::PathLineEdit(QWidget *parent)
    : ClearLineEdit(parent)
{
}

QAbstractItemModel *PathLineEdit::completionModel() const
{
    if (QCompleter *c = completer()) {
        return c->model();
    } else {
        return nullptr;
    }
}

void PathLineEdit::setCompletionModel(QAbstractItemModel *model)
{
    QCompleter *c = completer();
    if (!c) {
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
    connect(menu->addAction(QIcon::fromTheme(QStringLiteral("document-open")), tr("Select directory ...")), &QAction::triggered, [this] {
        const QString path = QFileDialog::getExistingDirectory(this);
        if (!path.isEmpty()) {
            editText(path);
        }
    });
    QFileInfo fileInfo(text());
    if (fileInfo.exists()) {
        if (fileInfo.isFile()) {
            connect(
                menu->addAction(QIcon::fromTheme(QStringLiteral("system-run")), tr("Open")), &QAction::triggered, bind(&openLocalFileOrDir, text()));
        } else if (fileInfo.isDir()) {
            connect(menu->addAction(QIcon::fromTheme(QStringLiteral("system-file-manager")), tr("Explore")), &QAction::triggered,
                bind(&openLocalFileOrDir, text()));
        }
    }
    menu->exec(event->globalPos());
}

} // namespace QtGui
