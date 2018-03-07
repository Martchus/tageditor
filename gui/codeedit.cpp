#include "./codeedit.h"

#include <QTextBlock>
#include <QTextDocumentFragment>

namespace QtGui {

CodeEdit::CodeEdit(QWidget *parent)
    : QPlainTextEdit(parent)
    , m_indentation(QStringLiteral("    "))
{
}

CodeEdit::~CodeEdit()
{
}

void CodeEdit::keyPressEvent(QKeyEvent *e)
{
    if (textCursor().selection().isEmpty()) {
        switch (e->key()) {
        case Qt::Key_Tab:
            textCursor().insertText(m_indentation);
            return;
        case Qt::Key_Return:
            handleReturn(e);
            return;
        case Qt::Key_Backspace:
            handleDelete(e);
            return;
        default:;
        }
    }
    QPlainTextEdit::keyPressEvent(e);
}

void CodeEdit::handleReturn(QKeyEvent *)
{
    QTextCursor cursor = textCursor();
    QString line = cursor.block().text();
    int index = 0;
    for (const QChar &c : line) {
        if (c.isSpace()) {
            ++index;
        } else {
            break;
        }
    }
    if (index < line.size() && line.at(index) == QChar('}')) {
        if (index > 0) {
            int beg = index;
            index -= m_indentation.size();
            cursor.select(QTextCursor::BlockUnderCursor);
            cursor.deleteChar();
            cursor.insertBlock();
            cursor.insertText(line.mid(0, index));
            if (index + 1 < line.size()) {
                cursor.insertText(line.mid(beg));
            }
        }
    }
    cursor.insertBlock();
    if (index > 0) {
        cursor.insertText(line.mid(0, index));
    }
    if (line.endsWith(QChar('{'))) {
        cursor.insertText(m_indentation);
    }
}

void CodeEdit::handleDelete(QKeyEvent *e)
{
    QTextCursor cursor = textCursor();
    QString line = cursor.block().text();
    if (line.endsWith(m_indentation)) {
        cursor.select(QTextCursor::BlockUnderCursor);
        cursor.deleteChar();
        cursor.insertBlock();
        cursor.insertText(line.mid(0, line.size() - m_indentation.size()));
    } else {
        QPlainTextEdit::keyPressEvent(e);
    }
}

} // namespace QtGui
