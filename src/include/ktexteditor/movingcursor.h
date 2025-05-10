/*
    SPDX-FileCopyrightText: 2010 Christoph Cullmann <cullmann@kde.org>

    Based on code of the SmartCursor/Range by:
    SPDX-FileCopyrightText: 2003-2005 Hamish Rodda <rodda@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef KTEXTEDITOR_MOVINGCURSOR_H
#define KTEXTEDITOR_MOVINGCURSOR_H

#include <ktexteditor/cursor.h>
#include <ktexteditor_export.h>

class QDebug;

namespace KTextEditor
{
class MovingRange;
class Document;

/*!
 * \class KTextEditor::MovingCursor
 * \inmodule KTextEditor
 * \inheaderfile KTextEditor/MovingCursor
 *
 * \brief A Cursor which is bound to a specific Document, and maintains its position.
 *
 * \ingroup kte_group_moving_classes
 *
 * A MovingCursor is an extension of the basic Cursor class. It maintains its
 * position in the document. As a result of this, MovingCursor%s may not be copied, as they need
 * to maintain a connection to the associated Document.
 *
 * Create a new MovingCursor like this:
 * \code
 * auto* movingCursor = aDocument->newMovingCursor(position);
 * \endcode
 *
 * The ownership of the cursor is passed to the user. When finished with a MovingCursor,
 * simply delete it.
 *
 * \sa Cursor, Range, MovingRange, Document
 *
 * \since 4.5
 */
class KTEXTEDITOR_EXPORT MovingCursor
{
    //
    // sub types
    //
public:
    /*!
       \enum KTextEditor::MovingCursor::InsertBehavior

       Insert behavior of this cursor, should it stay if text is insert at its position
       or should it move.

       \value StayOnInsert
       Stay on insert.
       \value MoveOnInsert
       Move on insert.
     */
    enum InsertBehavior {
        StayOnInsert = 0x0,
        MoveOnInsert = 0x1
    };

    /*!
       \enum KTextEditor::MovingCursor::WrapBehavior

       Wrap behavior for end of line treatement used in move().

       \value Wrap
       Wrap at end of line
       \value NoWrap
       Do not wrap at end of line
     */
    enum WrapBehavior {
        Wrap = 0x0,
        NoWrap = 0x1
    };

    //
    // stuff that needs to be implemented by editor part cursors
    //
public:
    /*!
     * Set insert behavior.
     *
     * \a insertBehavior is the new insert behavior
     *
     */
    virtual void setInsertBehavior(InsertBehavior insertBehavior) = 0;

    /*!
     * Get current insert behavior.
     * Returns current insert behavior
     */
    virtual InsertBehavior insertBehavior() const = 0;

    /*!
     * Gets the document to which this cursor is bound.
     *
     * Returns a pointer to the document
     */
    virtual Document *document() const = 0;

    /*!
     * Get range this cursor belongs to, if any
     * Returns range this pointer is part of, else 0
     */
    virtual MovingRange *range() const = 0;

    /*!
     * Set the current cursor position to \e position.
     *
     * \a position is the new cursor position
     *
     */
    virtual void setPosition(KTextEditor::Cursor position) = 0;

    /*!
     * Retrieve the line on which this cursor is situated.
     *
     * Returns line number, where 0 is the first line.
     */
    virtual int line() const = 0;

    /*!
     * Retrieve the column on which this cursor is situated.
     *
     * Returns column number, where 0 is the first column.
     */
    virtual int column() const = 0;

    /*!
     * Destruct the moving cursor.
     */
    virtual ~MovingCursor();

    //
    // forbidden stuff
    //
protected:
    /*!
     * For inherited class only.
     */
    MovingCursor();

public:
    /*!
     * no copy constructor, don't allow this to be copied.
     */
    MovingCursor(const MovingCursor &) = delete;

    /*!
     * no assignment operator, no copying around clever cursors.
     */
    MovingCursor &operator=(const MovingCursor &) = delete;

    //
    // convenience API
    //
public:
    /*!
     * Returns whether the current position of this cursor is a valid position,
     * i.e. whether line() >= 0 and column() >= 0.
     *
     * Returns \e true , if the cursor position is valid, otherwise \e false
     */
    inline bool isValid() const
    {
        return line() >= 0 && column() >= 0;
    }

    /*!
     * Check whether this MovingCursor is located at a valid text position.
     *
     * A cursor position at (line, column) is valid, if:
     * \list
     * \li line >= 0 and line < document()->lines() holds, and
     * \li column >= 0 and column <= lineLength(column).
     * \endlist
     *
     * Further, the text position is also invalid if it is inside a Unicode
     * surrogate (utf-32 character).
     *
     * Returns \e true, if the cursor is a valid text position, otherwise \e false
     *
     * \sa Document::isValidTextPosition()
     */
    bool isValidTextPosition() const;

    /*!
     * \overload
     *
     * Set the cursor position to \e line and \e column.
     *
     * \a line is the new cursor line
     *
     * \a column is the new cursor column
     *
     */
    void setPosition(int line, int column);

    /*!
     * Set the cursor line to \e line.
     *
     * \a line is the new cursor line
     *
     */
    void setLine(int line);

    /*!
     * Set the cursor column to \e column.
     *
     * \a column is the new cursor column
     *
     */
    void setColumn(int column);

    /*!
     * Determine if this cursor is located at column 0 of a valid text line.
     *
     * Returns \e true if cursor is a valid text position and column()=0, otherwise \e false.
     */
    bool atStartOfLine() const;

    /*!
     * Determine if this cursor is located at the end of the current line.
     *
     * Returns \e true if the cursor is situated at the end of the line, otherwise \e false.
     */
    bool atEndOfLine() const;

    /*!
     * Determine if this cursor is located at line 0 and column 0.
     *
     * Returns \e true if the cursor is at start of the document, otherwise \e false.
     */
    bool atStartOfDocument() const;

    /*!
     * Determine if this cursor is located at the end of the last line in the
     * document.
     *
     * Returns \e true if the cursor is at the end of the document, otherwise \e false.
     */
    bool atEndOfDocument() const;

    /*!
     * Moves the cursor to the next line and sets the column to 0. If the cursor
     * position is already in the last line of the document, the cursor position
     * remains unchanged and the return value is \e false.
     *
     * Returns \e true on success, otherwise \e false
     */
    bool gotoNextLine();

    /*!
     * Moves the cursor to the previous line and sets the column to 0. If the
     * cursor position is already in line 0, the cursor position remains
     * unchanged and the return value is \e false.
     *
     * Returns \e true on success, otherwise \e false
     */
    bool gotoPreviousLine();

    /*!
     * Moves the cursor \a chars character forward or backwards. If \a wrapBehavior
     * equals WrapBehavior::Wrap, the cursor is automatically wrapped to the
     * next line at the end of a line.
     *
     * When moving backwards, the WrapBehavior does not have any effect.
     * \note If the cursor could not be moved the amount of chars requested,
     *       the cursor is not moved at all!
     *
     * Returns \e true on success, otherwise \e false
     */
    bool move(int chars, WrapBehavior wrapBehavior = Wrap);

    /*!
     * Convert this clever cursor into a dumb one.
     * Even if this cursor belongs to a range, the created one not.
     * Returns normal cursor
     */
    const Cursor toCursor() const
    {
        return Cursor(line(), column());
    }

    /*!
     * Convert this clever cursor into a dumb one. Equal to toCursor, allowing to use implicit conversion.
     * Even if this cursor belongs to a range, the created one not.
     * Returns normal cursor
     */
    operator Cursor() const
    {
        return Cursor(line(), column());
    }

    //
    // operators for: MovingCursor <-> MovingCursor
    //
    /*!
     * Equality operator.
     *
     * \note comparison between two invalid cursors is undefined.
     *       comparison between an invalid and a valid cursor will always be \e false.
     *
     * \a c1 is the first cursor to compare
     *
     * \a c2 is the second cursor to compare
     *
     * Returns \e true, if c1's and c2's line and column are \e equal.
     */
    inline friend bool operator==(const MovingCursor &c1, const MovingCursor &c2)
    {
        return c1.line() == c2.line() && c1.column() == c2.column();
    }

    /*!
     * Inequality operator.
     *
     * \a c1 is the first cursor to compare
     *
     * \a c2 is the second cursor to compare
     *
     * Returns \e true, if c1's and c2's line and column are \e not equal.
     */
    inline friend bool operator!=(const MovingCursor &c1, const MovingCursor &c2)
    {
        return !(c1 == c2);
    }

    /*!
     * Greater than operator.
     *
     * \a c1 is the first cursor to compare
     *
     * \a c2 is the second cursor to compare
     *
     * Returns \e true, if c1's position is greater than c2's position,
     *         otherwise \e false.
     */
    inline friend bool operator>(const MovingCursor &c1, const MovingCursor &c2)
    {
        return c1.line() > c2.line() || (c1.line() == c2.line() && c1.column() > c2.column());
    }

    /*!
     * Greater than or equal to operator.
     *
     * \a c1 is the first cursor to compare
     *
     * \a c2 is the second cursor to compare
     *
     * Returns \e true, if c1's position is greater than or equal to c2's
     *         position, otherwise \e false.
     */
    inline friend bool operator>=(const MovingCursor &c1, const MovingCursor &c2)
    {
        return c1.line() > c2.line() || (c1.line() == c2.line() && c1.column() >= c2.column());
    }

    /*!
     * Less than operator.
     *
     * \a c1 is the first cursor to compare
     *
     * \a c2 is the second cursor to compare
     *
     * Returns \e true, if c1's position is greater than or equal to c2's
     *         position, otherwise \e false.
     */
    inline friend bool operator<(const MovingCursor &c1, const MovingCursor &c2)
    {
        return !(c1 >= c2);
    }

    /*!
     * Less than or equal to operator.
     *
     * \a c1 is the first cursor to compare
     *
     * \a c2 is the second cursor to compare
     *
     * Returns \e true, if c1's position is lesser than or equal to c2's
     *         position, otherwise \e false.
     */
    inline friend bool operator<=(const MovingCursor &c1, const MovingCursor &c2)
    {
        return !(c1 > c2);
    }
};

/*!
 * qDebug() stream operator. Writes this cursor to the debug output in a nicely formatted way.
 *
 * \a s is the debug stream
 *
 * \a cursor is the cursor to print
 *
 * Returns debug stream
 *
 * \relates KTextEditor::MovingCursor
 */
KTEXTEDITOR_EXPORT QDebug operator<<(QDebug s, const MovingCursor *cursor);

/*!
 * qDebug() stream operator. Writes this cursor to the debug output in a nicely formatted way.
 *
 * \a s is the debug stream
 *
 * \a cursor is the cursor to print
 *
 * Returns debug stream
 *
 * \relates KTextEditor::MovingCursor
 */
KTEXTEDITOR_EXPORT QDebug operator<<(QDebug s, const MovingCursor &cursor);
}

#endif
