/* This file is part of the KDE project
 *
 *  Copyright 2019 Dominik Haumann <dhaumann@kde.org>
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
#include "katemacroexpander.h"

#include <KTextEditor/Editor>

/**
 * Find closing bracket for @p str starting a position @p pos.
 */
static int findClosing(QStringView str, int pos = 0)
{
    const int len = str.size();
    int nesting = 0;

    while (pos < len) {
        ++pos;
        const QChar c = str[pos];
        if (c == QLatin1Char('}')) {
            if (nesting == 0) {
                return pos;
            }
            nesting--;
        } else if (c == QLatin1Char('{')) {
            nesting++;
        }
    }
    return -1;
}

QString KateMacroExpander::expandMacro(const QString& input, KTextEditor::View* view)
{
    QString output = input;
    QString oldStr;
    do {
        oldStr = output;
        const int startIndex = output.indexOf(QStringLiteral("%{"));
        if (startIndex < 0) {
            break;
        }

        const int endIndex = findClosing(output, startIndex + 2);
        if (endIndex <= startIndex) {
            break;
        }

        const int varLen = endIndex - (startIndex + 2);
        QString variable = output.mid(startIndex + 2, varLen);
        variable = expandMacro(variable, view);
        if (KTextEditor::Editor::instance()->expandVariable(variable, view, variable)) {
            output.replace(startIndex, endIndex - startIndex + 1, variable);
        }
    } while (output != oldStr); // str comparison guards against infinite loop
    return output;
}

// kate: space-indent on; indent-width 4; replace-tabs on;
