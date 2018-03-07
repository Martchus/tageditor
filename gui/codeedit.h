#ifndef CODEEDITOR_H
#define CODEEDITOR_H

#include <QPlainTextEdit>

namespace QtGui {

class CodeEdit : public QPlainTextEdit {
public:
    CodeEdit(QWidget *parent = nullptr);
    ~CodeEdit();

    const QString &indentation() const;
    void setIndentation(const QString &indentation);

protected:
    void keyPressEvent(QKeyEvent *e);

private:
    void handleReturn(QKeyEvent *e);
    void handleDelete(QKeyEvent *e);

    QString m_indentation;
};

inline const QString &CodeEdit::indentation() const
{
    return m_indentation;
}

inline void CodeEdit::setIndentation(const QString &indentation)
{
    m_indentation = indentation;
}

} // namespace QtGui

#endif // CODEEDITOR_H
