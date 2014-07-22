/*  This file is part of the KDE libraries and the Kate part.
 *
 *  Copyright (C) 2008 - 2009 Erlend Hamberg <ehamberg@gmail.com>
 *  Copyright (C) 2011 Svyatoslav Kuzmich <svatoslav1@gmail.com>
 *  Copyright (C) 2012 Vegard Øye
 *  Copyright (C) 2013 Simon St James <kdedevel@etotheipiplusone.com>
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Library General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Library General Public License for more details.
 *
 *  You should have received a copy of the GNU Library General Public License
 *  along with this library; see the file COPYING.LIB.  If not, write to
 *  the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 *  Boston, MA 02110-1301, USA.
 */
#include "commandrangeexpressionparser.h"

#include "kateview.h"
#include "katedocument.h"
#include <vimode/inputmodemanager.h>
#include "marks.h"

#include <QStringList>
#include <QRegularExpression>

using namespace KateVi;

#define RegExp(x) QRegularExpression(QLatin1String(x))

namespace
{
    const QRegularExpression RE_Line = RegExp("\\d+");
    const QRegularExpression RE_LastLine = RegExp("\\$");
    const QRegularExpression RE_ThisLine = RegExp("\\.");
    const QRegularExpression RE_Mark = RegExp("\\'[0-9a-z><\\+\\*\\_]");
    const QRegularExpression RE_ForwardSearch = RegExp("^/([^/]*)/?$");
    const QRegularExpression RE_ForwardSearch2 = RegExp("/[^/]*/?"); // no group
    const QRegularExpression RE_BackwardSearch = RegExp("^\\?([^?]*)\\??$");
    const QRegularExpression RE_BackwardSearch2 = RegExp("\\?[^?]*\\??"); // no group
    const QRegularExpression RE_Base = QRegularExpression(QLatin1String("(?:") + RE_Mark.pattern() + QLatin1String(")|(?:") +
                      RE_Line.pattern() + QLatin1String(")|(?:") +
                      RE_ThisLine.pattern() + QLatin1String(")|(?:") +
                      RE_LastLine.pattern() + QLatin1String(")|(?:") +
                      RE_ForwardSearch2.pattern() + QLatin1String(")|(?:") +
                      RE_BackwardSearch2.pattern() + QLatin1String(")"));
    const QRegularExpression RE_Offset = QRegularExpression(QLatin1String("[+-](?:") + RE_Base.pattern() + QLatin1String(")?"));

    // The position regexp contains two groups: the base and the offset.
    // The offset may be empty.
    const QRegularExpression RE_Position = QRegularExpression(QLatin1String("(") + RE_Base.pattern() + QLatin1String(")((?:") + RE_Offset.pattern() + QLatin1String(")*)"));

    // The range regexp contains seven groups: the first is the start position, the second is
    // the base of the start position, the third is the offset of the start position, the
    // fourth is the end position including a leading comma, the fifth is end position
    // without the comma, the sixth is the base of the end position, and the seventh is the
    // offset of the end position. The third and fourth groups may be empty, and the
    // fifth, sixth and seventh groups are contingent on the fourth group.
    const QRegularExpression RE_CmdRange = QRegularExpression(QLatin1String("^(") + RE_Position.pattern() + QLatin1String(")((?:,(") + RE_Position.pattern() + QLatin1String("))?)"));
}

CommandRangeExpressionParser::CommandRangeExpressionParser(InputModeManager *vimanager)
    : m_viInputModeManager(vimanager)
{
}

QString CommandRangeExpressionParser::parseRangeString(const QString &command) const
{
    if (command.isEmpty()) {
        return QString();
    }

    if (command.at(0) == QLatin1Char('%')) {
        return QLatin1String("%");
    }

    QRegularExpressionMatch rangeMatch = RE_CmdRange.match(command);

    return rangeMatch.hasMatch() ? rangeMatch.captured() : QString();
}

KTextEditor::Range CommandRangeExpressionParser::parseRange(const QString &command, QString &destTransformedCommand) const
{
    if (command.isEmpty()) {
        return KTextEditor::Range::invalid();
    }

    QString commandTmp = command;

    // expand '%' to '1,$' ("all lines") if at the start of the line
    if (commandTmp.at(0) == QLatin1Char('%')) {
        commandTmp.replace(0, 1, QLatin1String("1,$"));
    }

    QRegularExpressionMatch rangeMatch = RE_CmdRange.match(commandTmp);

    if (!rangeMatch.hasMatch()) {
        return KTextEditor::Range::invalid();
    }

    QString position_string1 = rangeMatch.captured(1);
    QString position_string2 = rangeMatch.captured(4);
    int position1 = calculatePosition(position_string1);
    int position2 = (position_string2.isEmpty()) ? position1 : calculatePosition(rangeMatch.captured(5));

    commandTmp.remove(RE_CmdRange);

    // special case: if the command is just a number with an optional +/- prefix, rewrite to "goto"
    if (commandTmp.isEmpty()) {
        destTransformedCommand = QString::fromLatin1("goto %1").arg(position1);
        return KTextEditor::Range::invalid();
    } else {
        destTransformedCommand = commandTmp;
        return KTextEditor::Range(KTextEditor::Range(position1 - 1, 0, position2 - 1, 0));
    }
}

int CommandRangeExpressionParser::calculatePosition(const QString &string) const
{
    int pos = 0;
    QList<bool> operators_list;
    QStringList split = string.split(RegExp("[-+](?!([+-]|$))"));
    QList<int> values;

    Q_FOREACH (const QString &line, split) {
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

        matchLineNumber(line, values) ||
        matchLastLine(line, values) ||
        matchThisLine(line, values) ||
        matchMark(line, values) ||
        matchForwardSearch(line, values) ||
        matchBackwardSearch(line, values);
    }

    if (values.isEmpty()) {
        return -1;
    }

    int result = values.at(0);
    for (int i = 0; i < operators_list.size(); ++i) {
        if (operators_list.at(i)) {
            result += values.at(i + 1);
        } else {
            result -= values.at(i + 1);
        }
    }

    return result;
}

bool CommandRangeExpressionParser::matchLineNumber(const QString &line, QList<int> &values) const
{
    QRegularExpressionMatch match = RE_Line.match(line);

    if (!match.hasMatch() || match.capturedLength() != line.length()) {
        return false;
    }

    values.push_back(line.toInt());
    return true;
}

bool CommandRangeExpressionParser::matchLastLine(const QString &line, QList<int> &values) const
{
    QRegularExpressionMatch match = RE_LastLine.match(line);

    if (!match.hasMatch() || match.capturedLength() != line.length()) {
        return false;
    }

    values.push_back(m_viInputModeManager->view()->doc()->lines());
    return true;
}

bool CommandRangeExpressionParser::matchThisLine(const QString &line, QList<int> &values) const
{
    QRegularExpressionMatch match = RE_ThisLine.match(line);

    if (!match.hasMatch() || match.capturedLength() != line.length()) {
        return false;
    }

    values.push_back(m_viInputModeManager->view()->cursorPosition().line() + 1);
    return true;
}

bool CommandRangeExpressionParser::matchMark(const QString &line, QList<int> &values) const
{
    QRegularExpressionMatch match = RE_Mark.match(line);

    if (!match.hasMatch() || match.capturedLength() != line.length()) {
        return false;
    }

    values.push_back(m_viInputModeManager->marks()->getMarkPosition(line.at(1)).line() + 1);
    return true;
}

bool CommandRangeExpressionParser::matchForwardSearch(const QString &line, QList<int> &values) const
{
    QRegularExpressionMatch match = RE_ForwardSearch.match(line);

    if (!match.hasMatch()) {
        return false;
    }

    QString pattern = match.captured(1);
    KTextEditor::Range range(m_viInputModeManager->view()->cursorPosition(), m_viInputModeManager->view()->doc()->documentEnd());
    QVector<KTextEditor::Range> matchingLines = m_viInputModeManager->view()->doc()->searchText(range, pattern, KTextEditor::Search::Regex);

    if (matchingLines.isEmpty()) {
        return true;
    }

    int lineNumber = matchingLines.first().start().line();

    values.push_back(lineNumber + 1);
    return true;
}

bool CommandRangeExpressionParser::matchBackwardSearch(const QString &line, QList<int> &values) const
{
    QRegularExpressionMatch match = RE_BackwardSearch.match(line);

    if (!match.hasMatch()) {
        return false;
    }

    QString pattern = match.captured(1);
    KTextEditor::Range range(KTextEditor::Cursor(0,0), m_viInputModeManager->view()->cursorPosition());
    QVector<KTextEditor::Range> matchingLines = m_viInputModeManager->view()->doc()->searchText(range, pattern, KTextEditor::Search::Regex);

    if (matchingLines.isEmpty()) {
        return true;
    }

    int lineNumber = matchingLines.first().start().line();

    values.push_back(lineNumber + 1);
    return true;
}
