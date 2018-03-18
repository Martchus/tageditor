#include "./javascripthighlighter.h"

namespace QtGui {

JavaScriptHighlighter::JavaScriptHighlighter(QTextDocument *parent)
    : QSyntaxHighlighter(parent)
{
    HighlightingRule rule;

    m_keywordFormat.setForeground(Qt::darkBlue);
    m_keywordFormat.setFontWeight(QFont::Bold);
    static const QStringList keywordPatterns{ QStringLiteral("\\bvar\\b"), QStringLiteral("\\bArray\\b"), QStringLiteral("\\bfunction\\b"),
        QStringLiteral("\\breturn\\b"), QStringLiteral("\\barguments\\b"), QStringLiteral("\\bif\\b"), QStringLiteral("\\belse\\b"),
        QStringLiteral("\\bfor\\b"), QStringLiteral("\\bswitch\\b"), QStringLiteral("\\bcase\\b"), QStringLiteral("\\bbreak\\b"),
        QStringLiteral("\\bwhile\\b"), QStringLiteral("\\bundefined\\b"), QStringLiteral("\\continue\\b") };
    for (const QString &pattern : keywordPatterns) {
        rule.pattern = QRegExp(pattern);
        rule.format = m_keywordFormat;
        m_highlightingRules.append(rule);
    }

    m_singleLineCommentFormat.setForeground(Qt::red);
    rule.pattern = QRegExp(QStringLiteral("//[^\n]*"));
    rule.format = m_singleLineCommentFormat;
    m_highlightingRules.append(rule);

    m_multiLineCommentFormat.setForeground(Qt::red);

    m_quotationFormat.setForeground(Qt::darkGreen);
    rule.pattern = QRegExp(QStringLiteral("\".*\""));
    rule.format = m_quotationFormat;
    m_highlightingRules.append(rule);

    m_functionFormat.setFontItalic(true);
    m_functionFormat.setForeground(Qt::blue);
    rule.pattern = QRegExp(QStringLiteral("(?!if)\\b[A-Za-z0-9_]+(?=\\()"));
    rule.format = m_functionFormat;
    m_highlightingRules.append(rule);

    m_commentStartExpression = QRegExp(QStringLiteral("/\\*"));
    m_commentEndExpression = QRegExp(QStringLiteral("\\*/"));
}

void JavaScriptHighlighter::highlightBlock(const QString &text)
{
    for (const HighlightingRule &rule : m_highlightingRules) {
        QRegExp expression(rule.pattern);
        int index = expression.indexIn(text);
        while (index >= 0) {
            int length = expression.matchedLength();
            setFormat(index, length, rule.format);
            index = expression.indexIn(text, index + length);
        }
    }
    setCurrentBlockState(0);

    int startIndex = 0;
    if (previousBlockState() != 1) {
        startIndex = m_commentStartExpression.indexIn(text);
    }

    while (startIndex >= 0) {
        int endIndex = m_commentEndExpression.indexIn(text, startIndex);
        int commentLength;
        if (endIndex == -1) {
            setCurrentBlockState(1);
            commentLength = text.length() - startIndex;
        } else {
            commentLength = endIndex - startIndex + m_commentEndExpression.matchedLength();
        }
        setFormat(startIndex, commentLength, m_multiLineCommentFormat);
        startIndex = m_commentStartExpression.indexIn(text, startIndex + commentLength);
    }
}

} // namespace QtGui
