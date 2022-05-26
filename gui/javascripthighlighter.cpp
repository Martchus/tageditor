#include "./javascripthighlighter.h"

namespace QtGui {

JavaScriptHighlighter::JavaScriptHighlighter(QTextDocument *parent)
    : QSyntaxHighlighter(parent)
{
    HighlightingRule rule;

    m_keywordFormat.setFontWeight(QFont::Bold);
    static const QStringList keywordPatterns{ QStringLiteral("\\bvar\\b"), QStringLiteral("\\blet\\b"), QStringLiteral("\\bconst\\b"),
        QStringLiteral("\\bArray\\b"), QStringLiteral("\\bfunction\\b"), QStringLiteral("\\breturn\\b"), QStringLiteral("\\barguments\\b"),
        QStringLiteral("\\bif\\b"), QStringLiteral("\\belse\\b"), QStringLiteral("\\bfor\\b"), QStringLiteral("\\bswitch\\b"),
        QStringLiteral("\\bcase\\b"), QStringLiteral("\\bbreak\\b"), QStringLiteral("\\bwhile\\b"), QStringLiteral("\\bundefined\\b"),
        QStringLiteral("\\bnull\\b"), QStringLiteral("\\bcontinue\\b"), QStringLiteral("\\bfalse\\b"), QStringLiteral("\\btrue\\b") };
    for (const auto &pattern : keywordPatterns) {
        rule.pattern = QRegularExpression(pattern);
        rule.pattern.setPatternOptions(QRegularExpression::InvertedGreedinessOption);
        rule.format = m_keywordFormat;
        m_highlightingRules.append(rule);
    }

    m_singleLineCommentFormat.setForeground(Qt::darkGray);
    rule.pattern = QRegularExpression(QStringLiteral("//[^\n]*"));
    rule.format = m_singleLineCommentFormat;
    m_highlightingRules.append(rule);

    m_multiLineCommentFormat.setForeground(Qt::darkGray);

    m_regexFormat.setForeground(Qt::darkMagenta);
    rule.pattern = QRegularExpression(QStringLiteral("/.*/"));
    rule.pattern.setPatternOptions(QRegularExpression::InvertedGreedinessOption);
    rule.format = m_regexFormat;
    m_highlightingRules.append(rule);

    m_stringFormat.setForeground(Qt::darkGreen);
    rule.pattern = QRegularExpression(QStringLiteral("\".*\""));
    rule.pattern.setPatternOptions(QRegularExpression::InvertedGreedinessOption);
    rule.format = m_stringFormat;
    m_highlightingRules.append(rule);

    m_functionFormat.setForeground(Qt::darkBlue);
    rule.pattern = QRegularExpression(QStringLiteral("(?!if)\\b[A-Za-z0-9_]+(?=\\()"));
    rule.pattern.setPatternOptions(QRegularExpression::InvertedGreedinessOption);
    rule.format = m_functionFormat;
    m_highlightingRules.append(rule);

    m_commentStartExpression = QRegularExpression(QStringLiteral("/\\*"));
    m_commentEndExpression = QRegularExpression(QStringLiteral("\\*/"));
}

void JavaScriptHighlighter::highlightBlock(const QString &text)
{
    for (const auto &rule : m_highlightingRules) {
        const auto &expression(rule.pattern);
        auto match = expression.match(text);
        while (match.hasMatch()) {
            const auto index = match.capturedStart();
            const auto length = match.capturedLength();
            setFormat(index, length, rule.format);
            match = expression.match(text, index + length);
        }
    }
    setCurrentBlockState(0);

    auto startIndex = 0;
    if (previousBlockState() != 1) {
        startIndex = m_commentStartExpression.match(text).capturedStart();
    }

    while (startIndex >= 0) {
        const auto endMatch = m_commentEndExpression.match(text, startIndex);
        const auto endIndex = endMatch.capturedStart();
        const auto commentLength = [&] {
            if (endIndex >= 0) {
                return endIndex - startIndex + endMatch.capturedLength();
            }
            setCurrentBlockState(1);
            return text.length() - startIndex;
        }();
        setFormat(startIndex, commentLength, m_multiLineCommentFormat);
        startIndex = m_commentStartExpression.match(text, startIndex + commentLength).capturedStart();
    }
}

} // namespace QtGui
