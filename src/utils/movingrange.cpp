/*
    SPDX-FileCopyrightText: 2010 Christoph Cullmann <cullmann@kde.org>

    Based on code of the SmartCursor/Range by:
    SPDX-FileCopyrightText: 2003-2005 Hamish Rodda <rodda@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "movingrange.h"

using namespace KTextEditor;

MovingRange::MovingRange() = default;

MovingRange::~MovingRange() = default;

void MovingRange::setRange(Cursor start, Cursor end)
{
    // just use other function, KTextEditor::Range will handle some normalization
    setRange(Range(start, end));
}

bool MovingRange::overlaps(const Range &range) const
{
    if (range.start() <= start()) {
        return range.end() > start();
    }

    else if (range.end() >= end()) {
        return range.start() < end();
    }

    else {
        return contains(range);
    }
}

QDebug KTextEditor::operator<<(QDebug s, const MovingRange *range)
{
    if (range) {
        s << "[" << range->start() << " -> " << range->end() << "]";
    } else {
        s << "(null range)";
    }
    return s.space();
}

/**
 * qDebug() stream operator. Writes this range to the debug output in a nicely formatted way.
 * @param s debug stream
 * @param range range to print
 * @return debug stream
 */
QDebug KTextEditor::operator<<(QDebug s, const MovingRange &range)
{
    return s << &range;
}
