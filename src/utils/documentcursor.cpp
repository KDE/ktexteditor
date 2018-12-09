/* This file is part of the KDE project
 *
 *  Copyright (C) 2010 Christoph Cullmann <cullmann@kde.org>
 *  Copyright (C) 2012 Dominik Haumann <dhaumann@kde.org>
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

#include "documentcursor.h"

namespace KTextEditor
{

DocumentCursor::DocumentCursor(KTextEditor::Document *document)
    : m_document(document)
    , m_cursor(KTextEditor::Cursor::invalid())
{
    // we require a valid document
    Q_ASSERT(m_document);
}

DocumentCursor::DocumentCursor(KTextEditor::Document *document, const KTextEditor::Cursor &position)
    : m_document(document)
    , m_cursor(position)
{
    // we require a valid document
    Q_ASSERT(m_document);
}

DocumentCursor::DocumentCursor(KTextEditor::Document *document, int line, int column)
    : m_document(document)
    , m_cursor(line, column)
{
    // we require a valid document
    Q_ASSERT(m_document);
}

DocumentCursor::DocumentCursor(const DocumentCursor &other)
    : m_document(other.m_document)
    , m_cursor(other.m_cursor)
{
}

void DocumentCursor::setPosition(const KTextEditor::Cursor &position)
{
    if (position.isValid()) {
        m_cursor = position;
    } else {
        m_cursor = KTextEditor::Cursor::invalid();
    }
}

void DocumentCursor::makeValid()
{
    const int line = m_cursor.line();
    const int col = m_cursor.line();

    if (line < 0) {
        m_cursor.setPosition(0, 0);
    } else if (line >= m_document->lines()) {
        m_cursor = m_document->documentEnd();
    } else if (col > m_document->lineLength(line)) {
        m_cursor.setColumn(m_document->lineLength(line));
    } else if (col < 0) {
        m_cursor.setColumn(0);
    } else if (!isValidTextPosition()) {
        // inside a unicode surrogate (utf-32 character)
        // -> move half one char left to the start of the utf-32 char
        m_cursor.setColumn(col - 1);
    }

    Q_ASSERT(isValidTextPosition());
}

void DocumentCursor::setPosition(int line, int column)
{
    m_cursor.setPosition(line, column);
}

void DocumentCursor::setLine(int line)
{
    setPosition(line, column());
}

void DocumentCursor::setColumn(int column)
{
    setPosition(line(), column);
}

bool DocumentCursor::atStartOfLine() const
{
    return isValidTextPosition() && column() == 0;
}

bool DocumentCursor::atEndOfLine() const
{
    return isValidTextPosition() && column() == document()->lineLength(line());
}

bool DocumentCursor::atStartOfDocument() const
{
    return line() == 0 && column() == 0;
}

bool DocumentCursor::atEndOfDocument() const
{
    // avoid costly lineLength computation if we are not in the last line
    // this is called often e.g. during search & replace, >> 2% of the total costs
    const auto lastLine = document()->lines() - 1;
    return line() == lastLine && column() == document()->lineLength(lastLine);
}

bool DocumentCursor::gotoNextLine()
{
    // only allow valid cursors
    const bool ok = isValid() && (line() + 1 < document()->lines());

    if (ok) {
        setPosition(Cursor(line() + 1, 0));
    }

    return ok;
}

bool DocumentCursor::gotoPreviousLine()
{
    // only allow valid cursors
    bool ok = (line() > 0) && (column() >= 0);

    if (ok) {
        setPosition(Cursor(line() - 1, 0));
    }

    return ok;
}

bool DocumentCursor::move(int chars, WrapBehavior wrapBehavior)
{
    if (!isValid()) {
        return false;
    }

    // create temporary cursor to modify
    Cursor c(m_cursor);

    // forwards?
    if (chars > 0) {
        // cache lineLength to minimize calls of KTextEditor::DocumentPrivate::lineLength(), as
        // results in locating the correct block in the text buffer every time,
        // which is relatively slow
        int lineLength = document()->lineLength(c.line());

        // special case: cursor position is not in valid text, then the algo does
        // not work for Wrap mode. Hence, catch this special case by setting
        // c.column() to the lineLength()
        if (wrapBehavior == Wrap && c.column() > lineLength) {
            c.setColumn(lineLength);
        }

        while (chars != 0) {
            if (wrapBehavior == Wrap) {
                const int advance = qMin(lineLength - c.column(), chars);

                if (chars > advance) {
                    if (c.line() + 1 >= document()->lines()) {
                        return false;
                    }

                    c.setPosition(c.line() + 1, 0);
                    chars -= advance + 1; // +1 because of end-of-line wrap

                    // advanced one line, so cache correct line length again
                    lineLength = document()->lineLength(c.line());
                } else {
                    c.setColumn(c.column() + chars);
                    chars = 0;
                }
            } else { // NoWrap
                c.setColumn(c.column() + chars);
                chars = 0;
            }
        }
    }

    // backwards?
    else {
        while (chars != 0) {
            const int back = qMin(c.column(), -chars);
            if (-chars > back) {
                if (c.line() == 0) {
                    return false;
                }

                c.setPosition(c.line() - 1, document()->lineLength(c.line() - 1));
                chars += back + 1; // +1 because of wrap-around at start-of-line
            } else {
                c.setColumn(c.column() + chars);
                chars = 0;
            }
        }
    }

    if (c != m_cursor) {
        setPosition(c);
    }
    return true;
}

}

