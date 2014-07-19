/*  This file is part of the KDE libraries and the Kate part.
 *
 *  Copyright (C) 2008 Erlend Hamberg <ehamberg@gmail.com>
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

#ifndef KATEVI_RANGE_H
#define KATEVI_RANGE_H

#include <QDebug>
#include <ktexteditor_export.h>
#include <vimode/definitions.h>

namespace KTextEditor {
    class Cursor;
    class Range;
}

enum MotionType {
    ExclusiveMotion = 0,
    InclusiveMotion
};

namespace KateVi
{

class KTEXTEDITOR_EXPORT Range
{
public:
    Range();

    /**
     * For motions which only return a position, in contrast to
     * "text objects" which returns a full blown range.
     */
    explicit Range(int elin, int ecol, MotionType inc);

    explicit Range(int slin, int scol, int elin, int ecol, MotionType mt);
    explicit Range(const KTextEditor::Cursor &c, MotionType mt);
    explicit Range(const KTextEditor::Cursor &c1, const KTextEditor::Cursor c2, MotionType mt);

    /**
     * Modifies this range so the start attributes are lesser than
     * the end attributes.
     */
    void normalize();

    /**
     * @returns an equivalent KTextEditor::Range for this Range.
     */
    KTextEditor::Range toEditorRange() const;

    /**
     * Writes this KateViRange to the debug output in a nicely formatted way.
     */
    friend QDebug operator<< (QDebug s, const Range &range);

    /**
     * @returns an invalid KateViRange allocated on stack.
     */
    static KateVi::Range invalid();

public:
    int startLine, startColumn;
    int endLine, endColumn;
    MotionType motionType;
    bool valid, jump;
};

}

#endif /* KATEVI_RANGE_H */
