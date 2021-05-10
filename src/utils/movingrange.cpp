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

void MovingRange::setRange(const Cursor &start, const Cursor &end)
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
