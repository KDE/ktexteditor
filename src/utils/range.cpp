/* This file is part of the KDE libraries
 *  Copyright (C) 2016 Dominik Haumann <dhaumann@kde.org>
 *  Copyright (C) 2007 Mirko Stocker <me@misto.ch>
 *  Copyright (C) 2003-2005 Hamish Rodda <rodda@kde.org>
 *  Copyright (C) 2002 Christian Couder <christian@kdevelop.org>
 *  Copyright (C) 2001, 2003 Christoph Cullmann <cullmann@kde.org>
 *  Copyright (C) 2001 Joseph Wenninger <jowenn@kde.org>
 *  Copyright (C) 1999 Jochen Wilhelmy <digisnap@cs.tu-berlin.de>
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

#include "range.h"

using namespace KTextEditor;

Range Range::fromString(const QStringRef& str) Q_DECL_NOEXCEPT
{
    const int startIndex = str.indexOf(QLatin1Char('['));
    const int endIndex = str.indexOf(QLatin1Char(']'));
    const int closeIndex = str.indexOf(QLatin1Char(')')); // end of first cursor

    if (startIndex < 0 || endIndex < 0 || closeIndex < 0 ||
        closeIndex < startIndex || endIndex < closeIndex ||
        endIndex < startIndex)
    {
        return invalid();
    }

    return Range(Cursor::fromString(str.mid(startIndex + 1, closeIndex - startIndex)),
                 Cursor::fromString(str.mid(closeIndex + 2, endIndex - closeIndex - 2)));
}

void Range::setRange(const Range &range) Q_DECL_NOEXCEPT
{
    m_start = range.start();
    m_end = range.end();
}

void Range::setRange(const Cursor &start, const Cursor &end) Q_DECL_NOEXCEPT
{
    if (start > end) {
        setRange(Range(end, start));
    } else {
        setRange(Range(start, end));
    }
}

bool Range::confineToRange(const Range &range) Q_DECL_NOEXCEPT
{
    if (start() < range.start())
        if (end() > range.end()) {
            setRange(range);
        } else {
            setStart(range.start());
        }
    else if (end() > range.end()) {
        setEnd(range.end());
    } else {
        return false;
    }

    return true;
}

bool Range::expandToRange(const Range &range) Q_DECL_NOEXCEPT
{
    if (start() > range.start())
        if (end() < range.end()) {
            setRange(range);
        } else {
            setStart(range.start());
        }
    else if (end() < range.end()) {
        setEnd(range.end());
    } else {
        return false;
    }

    return true;
}

void Range::setBothLines(int line) Q_DECL_NOEXCEPT
{
    setRange(Range(line, start().column(), line, end().column()));
}

void KTextEditor::Range::setBothColumns(int column) Q_DECL_NOEXCEPT
{
    setRange(Range(start().line(), column, end().line(), column));
}

namespace QTest {
    // Cursor: template specialization for QTest::toString()
    template<>
    char *toString(const KTextEditor::Cursor &cursor)
    {
        QByteArray ba = "Cursor[" + QByteArray::number(cursor.line())
                        + ", " + QByteArray::number(cursor.column()) + ']';
        return qstrdup(ba.data());
    }

    // Range: template specialization for QTest::toString()
    template<>
    char *toString(const KTextEditor::Range &range)
    {
        QByteArray ba = "Range[";
        ba += QByteArray::number(range.start().line()) + ", " + QByteArray::number(range.start().column()) + " - ";
        ba += QByteArray::number(range.end().line())   + ", " + QByteArray::number(range.end().column());
        ba += ']';
        return qstrdup(ba.data());
    }
}
