/*
    SPDX-FileCopyrightText: 2008-2009 Erlend Hamberg <ehamberg@gmail.com>
    SPDX-FileCopyrightText: 2011 Svyatoslav Kuzmich <svatoslav1@gmail.com>
    SPDX-FileCopyrightText: 2012 Vegard Øye 
    SPDX-FileCopyrightText: 2013 Simon St James <kdedevel@etotheipiplusone.com>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "katecommandrangeexpressionparser.h"

#include "katedocument.h"
#include "kateview.h"

#include <QRegularExpression>
#include <QStringList>

using KTextEditor::Cursor;
using KTextEditor::Range;

CommandRangeExpressionParser::CommandRangeExpressionParser()
{
    m_line.setPattern(QStringLiteral("\\d+"));
    m_lastLine.setPattern(QStringLiteral("\\$"));
    m_thisLine.setPattern(QStringLiteral("\\."));

    m_forwardSearch.setPattern(QStringLiteral("/([^/]*)/?"));
    m_forwardSearch2.setPattern(QStringLiteral("/[^/]*/?")); // no group
    m_backwardSearch.setPattern(QStringLiteral("\\?([^?]*)\\??"));
    m_backwardSearch2.setPattern(QStringLiteral("\\?[^?]*\\??")); // no group
    m_base.setPattern(QLatin1String("(?:") + m_mark.pattern() + QLatin1String(")|(?:") + m_line.pattern() + QLatin1String(")|(?:") + m_thisLine.pattern() + QLatin1String(")|(?:") + m_lastLine.pattern() + QLatin1String(")|(?:") +
                      m_forwardSearch2.pattern() + QLatin1String(")|(?:") + m_backwardSearch2.pattern() + QLatin1Char(')'));
    m_offset.setPattern(QLatin1String("[+-](?:") + m_base.pattern() + QLatin1String(")?"));

    // The position regexp contains two groups: the base and the offset.
    // The offset may be empty.
    m_position.setPattern(QLatin1Char('(') + m_base.pattern() + QLatin1String(")((?:") + m_offset.pattern() + QLatin1String(")*)"));

    // The range regexp contains seven groups: the first is the start position, the second is
    // the base of the start position, the third is the offset of the start position, the
    // fourth is the end position including a leading comma, the fifth is end position
    // without the comma, the sixth is the base of the end position, and the seventh is the
    // offset of the end position. The third and fourth groups may be empty, and the
    // fifth, sixth and seventh groups are contingent on the fourth group.
    m_cmdRange.setPattern(QLatin1String("^(") + m_position.pattern() + QLatin1String(")((?:,(") + m_position.pattern() + QLatin1String("))?)"));
}

Range CommandRangeExpressionParser::parseRangeExpression(const QString &command, KTextEditor::ViewPrivate *view, QString &destRangeExpression, QString &destTransformedCommand)
{
    CommandRangeExpressionParser rangeExpressionParser;
    return rangeExpressionParser.parseRangeExpression(command, destRangeExpression, destTransformedCommand, view);
}

Range CommandRangeExpressionParser::parseRangeExpression(const QString &command, QString &destRangeExpression, QString &destTransformedCommand, KTextEditor::ViewPrivate *view)
{
    Range parsedRange(0, -1, 0, -1);
    if (command.isEmpty()) {
        return parsedRange;
    }
    QString commandTmp = command;
    bool leadingRangeWasPercent = false;
    // expand '%' to '1,$' ("all lines") if at the start of the line
    if (commandTmp.at(0) == QLatin1Char('%')) {
        commandTmp.replace(0, 1, QStringLiteral("1,$"));
        leadingRangeWasPercent = true;
    }
    if (m_cmdRange.indexIn(commandTmp) != -1 && m_cmdRange.matchedLength() > 0) {
        commandTmp.remove(m_cmdRange);

        QString position_string1 = m_cmdRange.capturedTexts().at(1);
        QString position_string2 = m_cmdRange.capturedTexts().at(4);
        int position1 = calculatePosition(position_string1, view);

        int position2;
        if (!position_string2.isEmpty()) {
            // remove the comma
            position_string2 = m_cmdRange.capturedTexts().at(5);
            position2 = calculatePosition(position_string2, view);
        } else {
            position2 = position1;
        }

        // special case: if the command is just a number with an optional +/- prefix, rewrite to "goto"
        if (commandTmp.isEmpty()) {
            commandTmp = QStringLiteral("goto %1").arg(position1);
        } else {
            parsedRange.setRange(KTextEditor::Range(position1 - 1, 0, position2 - 1, 0));
        }

        destRangeExpression = (leadingRangeWasPercent ? QStringLiteral("%") : m_cmdRange.cap(0));
        destTransformedCommand = commandTmp;
    }

    return parsedRange;
}

int CommandRangeExpressionParser::calculatePosition(const QString &string, KTextEditor::ViewPrivate *view)
{
    int pos = 0;
    QList<bool> operators_list;
    const QStringList split = string.split(QRegularExpression(QStringLiteral("[-+](?!([+-]|$))")));
    QList<int> values;

    for (const QString &line : split) {
        pos += line.size();

        if (pos < string.size()) {
            if (string.at(pos) == QLatin1Char('+')) {
                operators_list.push_back(true);
            } else if (string.at(pos) == QLatin1Char('-')) {
                operators_list.push_back(false);
            } else {
                Q_ASSERT(false);
            }
        }

        ++pos;

        if (m_line.exactMatch(line)) {
            values.push_back(line.toInt());
        } else if (m_lastLine.exactMatch(line)) {
            values.push_back(view->doc()->lines());
        } else if (m_thisLine.exactMatch(line)) {
            values.push_back(view->cursorPosition().line() + 1);
        } else if (m_forwardSearch.exactMatch(line)) {
            m_forwardSearch.indexIn(line);
            QString pattern = m_forwardSearch.capturedTexts().at(1);
            int match = view->doc()->searchText(Range(view->cursorPosition(), view->doc()->documentEnd()), pattern, KTextEditor::Regex).first().start().line();
            values.push_back((match < 0) ? -1 : match + 1);
        } else if (m_backwardSearch.exactMatch(line)) {
            m_backwardSearch.indexIn(line);
            QString pattern = m_backwardSearch.capturedTexts().at(1);
            int match = view->doc()->searchText(Range(Cursor(0, 0), view->cursorPosition()), pattern, KTextEditor::Regex).first().start().line();
            values.push_back((match < 0) ? -1 : match + 1);
        }
    }

    if (values.isEmpty()) {
        return -1;
    }

    int result = values.at(0);
    for (int i = 0; i < operators_list.size(); ++i) {
        if (operators_list.at(i) == true) {
            result += values.at(i + 1);
        } else {
            result -= values.at(i + 1);
        }
    }

    return result;
}
