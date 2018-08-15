#ifndef JAVASCRIPTHIGHLIGHTER_H
#define JAVASCRIPTHIGHLIGHTER_H

#include <QRegularExpression>
#include <QSyntaxHighlighter>

namespace QtGui {

class JavaScriptHighlighter : public QSyntaxHighlighter {
    Q_OBJECT

public:
    JavaScriptHighlighter(QTextDocument *parent = nullptr);

protected:
    void highlightBlock(const QString &text) override;

private:
    struct HighlightingRule {
        QRegularExpression pattern;
        QTextCharFormat format;
    };
    QVector<HighlightingRule> m_highlightingRules;

    QRegularExpression m_commentStartExpression;
    QRegularExpression m_commentEndExpression;

    QTextCharFormat m_keywordFormat;
    QTextCharFormat m_classFormat;
    QTextCharFormat m_singleLineCommentFormat;
    QTextCharFormat m_multiLineCommentFormat;
    QTextCharFormat m_stringFormat;
    QTextCharFormat m_regexFormat;
    QTextCharFormat m_functionFormat;
};

} // namespace QtGui

#endif // JAVASCRIPTHIGHLIGHTER_H
