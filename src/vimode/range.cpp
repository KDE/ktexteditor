/*  This file is part of the KDE libraries and the Kate part.
 *
 *  Copyright (C) 2008 Erlend Hamberg <ehamberg@gmail.com>
 *  Copyright (C) 2011 Svyatoslav Kuzmich <svatoslav1@gmail.com>
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

#include <ktexteditor/range.h>
#include <vimode/range.h>

using namespace KateVi;

Range::Range()
    : Range(-1, -1, -1, -1, InclusiveMotion)
{
}

Range::Range(int slin, int scol, int elin, int ecol, MotionType inc)
    : startLine(slin)
    , startColumn(scol)
    , endLine(elin)
    , endColumn(ecol)
    , motionType(inc)
    , valid(true)
    , jump(false)
{
}

Range::Range(int elin, int ecol, MotionType inc)
    : Range(-1, -1, elin, ecol, inc)
{
}

Range::Range(const KTextEditor::Cursor &c, MotionType mt)
    : Range(-1, -1, c.line(), c.column(), mt)
{
}

Range::Range(const KTextEditor::Cursor &c1, const KTextEditor::Cursor c2, MotionType mt)
    : Range(c1.line(), c1.column(), c2.line(), c2.column(), mt)
{
}

void Range::normalize()
{
    int sl = startLine, el = endLine, sc = startColumn, ec = endColumn;

    if (sl < el) {
        startLine = sl;
        startColumn = sc;
        endLine = el;
        endColumn = ec;
    } else {
        startLine = el;
        endLine = sl;
        if (sl != el) {
            startColumn = ec;
            endColumn = sc;
        } else {
            startColumn = qMin(sc, ec);
            endColumn = qMax(sc, ec);
        }
    }
}

KTextEditor::Range Range::toEditorRange() const
{
    return KTextEditor::Range(startLine, startColumn, endLine, endColumn);
}

Range Range::invalid()
{
    Range r;
    r.valid = false;
    return r;
}

QDebug operator<<(QDebug s, const Range &range)
{
    s << "["
      << " (" << range.startLine << ", " << range.startColumn << ")"
      << " -> "
      << " (" << range.endLine << ", " << range.endColumn << ")"
      << "]"
      << " (" << (range.motionType == InclusiveMotion ? "Inclusive" : "Exclusive") << ") (jump: " << (range.jump ? "true" : "false") << ")";
    return s;
}
