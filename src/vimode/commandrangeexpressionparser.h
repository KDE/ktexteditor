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
#ifndef KATEVI_COMMAND_RANGE_EXPRESSION_PARSER
#define KATEVI_COMMAND_RANGE_EXPRESSION_PARSER

#include <ktexteditor/range.h>

namespace KateVi
{
class InputModeManager;

class CommandRangeExpressionParser
{
public:
    explicit CommandRangeExpressionParser(InputModeManager *vimanager);

    /**
     * Attempt to parse any leading range expression (e.g. "%", "'<,'>", ".,+6" etc) in @c command and
     * return it as a Range.  If parsing was successful, the range will be valid and the command with
     * the range stripped will be placed in @c destTransformedCommand.  In some special cases,
     * the @c destTransformedCommand will be further re-written e.g. a command in the form of just a number
     * will be rewritten as "goto <number>".
     *
     * An invalid Range is returned if no leading range expression could be found.
     */
    KTextEditor::Range parseRange(const QString &command, QString &destTransformedCommand) const;

    /**
     * Attempts to find range expression for vi command
     * @returns range sub string of passed command or empty if not found
     */
    QString parseRangeString(const QString &command) const;

private:
    int calculatePosition(const QString &string) const;

    bool matchLineNumber(const QString &line, QList<int> &values) const;
    bool matchLastLine(const QString &line, QList<int> &values) const;
    bool matchThisLine(const QString &line, QList<int> &values) const;
    bool matchMark(const QString &line, QList<int> &values) const;
    bool matchForwardSearch(const QString &line, QList<int> &values) const;
    bool matchBackwardSearch(const QString &line, QList<int> &values) const;

private:
    InputModeManager *m_viInputModeManager;
};

}

#endif
