/*
    SPDX-FileCopyrightText: 2010 Christoph Cullmann <cullmann@kde.org>
    SPDX-FileCopyrightText: 2010-2018 Dominik Haumann <dhaumann@kde.org>

    Based on code of the SmartCursor/Range by:
    SPDX-FileCopyrightText: 2003-2005 Hamish Rodda <rodda@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "movingcursor.h"
#include "document.h"
#include "documentcursor.h"

using namespace KTextEditor;

MovingCursor::MovingCursor()
{
}

MovingCursor::~MovingCursor()
{
}

void MovingCursor::setPosition(int line, int column)
{
    // just use setPosition
    setPosition(Cursor(line, column));
}

void MovingCursor::setLine(int line)
{
    // just use setPosition
    setPosition(line, column());
}

void MovingCursor::setColumn(int column)
{
    // just use setPosition
    setPosition(line(), column);
}

bool MovingCursor::atStartOfLine() const
{
    return isValidTextPosition() && column() == 0;
}

bool MovingCursor::atEndOfLine() const
{
    return isValidTextPosition() && column() == document()->lineLength(line());
}

bool MovingCursor::atEndOfDocument() const
{
    return *this == document()->documentEnd();
}

bool MovingCursor::atStartOfDocument() const
{
    return line() == 0 && column() == 0;
}

bool MovingCursor::gotoNextLine()
{
    // only touch valid cursors
    const bool ok = isValid() && (line() + 1 < document()->lines());

    if (ok) {
        setPosition(Cursor(line() + 1, 0));
    }

    return ok;
}

bool MovingCursor::gotoPreviousLine()
{
    // only touch valid cursors
    bool ok = (line() > 0) && (column() >= 0);

    if (ok) {
        setPosition(Cursor(line() - 1, 0));
    }

    return ok;
}

bool MovingCursor::move(int chars, WrapBehavior wrapBehavior)
{
    DocumentCursor dc(document(), toCursor());

    const bool success = dc.move(chars, static_cast<DocumentCursor::WrapBehavior>(wrapBehavior));

    if (success && dc.toCursor() != toCursor()) {
        setPosition(dc.toCursor());
    }

    return success;
}
