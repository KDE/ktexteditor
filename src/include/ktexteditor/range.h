/*
    SPDX-FileCopyrightText: 2003-2005 Hamish Rodda <rodda@kde.org>
    SPDX-FileCopyrightText: 2001-2005 Christoph Cullmann <cullmann@kde.org>
    SPDX-FileCopyrightText: 2002 Christian Couder <christian@kdevelop.org>
    SPDX-FileCopyrightText: 2001 Joseph Wenninger <jowenn@kde.org>
    SPDX-FileCopyrightText: 1999 Jochen Wilhelmy <digisnap@cs.tu-berlin.de>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef KTEXTEDITOR_RANGE_H
#define KTEXTEDITOR_RANGE_H

#include <ktexteditor_export.h>

#include <ktexteditor/cursor.h>
#include <ktexteditor/linerange.h>

#include <QtGlobal>

class QDebug;
class QString;
class QStringView;

namespace KTextEditor
{
/*!
 * \class KTextEditor::Range
 * \inmodule KTextEditor
 * \inheaderfile KTextEditor/Range
 *
 * \brief An object representing a section of text, from one Cursor to another.
 *
 * A Range is a basic class which represents a range of text with two Cursors,
 * from a start() position to an end() position.
 *
 * For simplicity and convenience, ranges always maintain their start position to
 * be before or equal to their end position.  Attempting to set either the
 * start or end of the range beyond the respective end or start will result in
 * both values being set to the specified position.  In the constructor, the
 * start and end will be swapped if necessary.
 *
 * If you want additional functionality such as the ability to maintain position
 * in a document, see MovingRange.
 *
 * \sa MovingRange
 */
class KTEXTEDITOR_EXPORT Range
{
public:
    /*!
     * Default constructor. Creates a valid range from position (0, 0) to
     * position (0, 0).
     */
    constexpr Range() noexcept = default;

    /*!
     * Constructor which creates a range from \e start to \e end.
     * If start is after end, they will be swapped.
     *
     * \a start is the start position
     *
     * \a end is the end position
     *
     */
    constexpr Range(Cursor start, Cursor end) noexcept
        : m_start(qMin(start, end))
        , m_end(qMax(start, end))
    {
    }

    /*!
     * Constructor which creates a single-line range from \a start,
     * extending \a width characters along the same line.
     *
     * \a start is the start position
     *
     * \a width is the width of this range in columns along the same line
     *
     */
    constexpr Range(Cursor start, int width) noexcept
        : m_start(qMin(start, Cursor(start.line(), start.column() + width)))
        , m_end(qMax(start, Cursor(start.line(), start.column() + width)))
    {
    }

    /*!
     * Constructor which creates a range from \a start, to \a endLine, \a endColumn.
     *
     * \a start is the start position
     *
     * \a endLine is the end line
     *
     * \a endColumn is the end column
     *
     */
    constexpr Range(Cursor start, int endLine, int endColumn) noexcept
        : m_start(qMin(start, Cursor(endLine, endColumn)))
        , m_end(qMax(start, Cursor(endLine, endColumn)))
    {
    }

    /*!
     * Constructor which creates a range from \e startLine, \e startColumn to \e endLine, \e endColumn.
     *
     * \a startLine is the start line
     *
     * \a startColumn is the start column
     *
     * \a endLine is the end line
     *
     * \a endColumn is the end column
     *
     */
    constexpr Range(int startLine, int startColumn, int endLine, int endColumn) noexcept
        : m_start(qMin(Cursor(startLine, startColumn), Cursor(endLine, endColumn)))
        , m_end(qMax(Cursor(startLine, startColumn), Cursor(endLine, endColumn)))
    {
    }

    /*!
     * Validity check.  In the base class, returns true unless the range starts before (0,0).
     */
    constexpr bool isValid() const noexcept
    {
        return start().isValid() && end().isValid();
    }

    /*!
     * Returns an invalid range.
     */
    constexpr static Range invalid() noexcept
    {
        return Range(Cursor::invalid(), Cursor::invalid());
    }

    /*!
     * Returns the cursor position as string in the format
     * "start-line:start-column,endl-line:end-column".
     * \sa fromString()
     */
    QString toString() const;

    /*!
     * Returns a Range created from the string \a str containing the format
     * "[(start-line, start-column), (endl-line:end-column)]".
     * In case the string cannot be parsed, an Range::invalid() is returned.
     * \sa toString()
     */
    static Range fromString(QStringView str) noexcept;

    /*
     * Position
     *
     * The following functions provide access to, and manipulation of, the range's position.
     * \{
     */

    /*!
     * Get the start position of this range. This will always be <= end().
     *
     * Returns const reference to the start position of this range.
     */
    constexpr Cursor start() const noexcept
    {
        return m_start;
    }

    /*!
     * Get the end position of this range. This will always be >= start().
     *
     * Returns const reference to the end position of this range.
     */
    constexpr Cursor end() const noexcept
    {
        return m_end;
    }

    /*!
     * Convert this Range to a LineRange
     *
     * Returns LineRange from the start line to the end line of this range.
     */
    constexpr LineRange toLineRange() const noexcept
    {
        return {start().line(), end().line()};
    }

    /*!
     * Convenience function.  Set the start and end lines to \a line.
     *
     * \a line is the line number to assign to start() and end()
     *
     */
    void setBothLines(int line) noexcept;

    /*!
     * Convenience function.  Set the start and end columns to \a column.
     *
     * \a column is the column number to assign to start() and end()
     *
     */
    void setBothColumns(int column) noexcept;

    /*!
     * Set the start and end cursors to \e range.start() and \e range.end() respectively.
     *
     * \a range is the range to assign to this range
     *
     */
    void setRange(Range range) noexcept;

    /*!
     * \overload
     * \n \n
     * Set the start and end cursors to \e start and \e end respectively.
     *
     * \note If \e start is after \e end, they will be reversed.
     *
     * \a start is the start cursor
     *
     * \a end is the end cursor
     *
     */
    void setRange(Cursor start, Cursor end) noexcept;

    /*!
     * Set the start cursor to \e start.
     *
     * \note If \e start is after current end, start and end will be set to new start value.
     *
     * \a start is the new start cursor
     *
     */
    void setStart(Cursor start) noexcept
    {
        if (start > end()) {
            setRange(start, start);
        } else {
            setRange(start, end());
        }
    }

    /*!
     * Set the end cursor to \e end.
     *
     * \note If \e end is in front of current start, start and end will be set to new end value.
     *
     * \a end is the new end cursor
     *
     */
    void setEnd(Cursor end) noexcept
    {
        if (end < start()) {
            setRange(end, end);
        } else {
            setRange(start(), end);
        }
    }

    /*!
     * Expand this range if necessary to contain \a range.
     *
     * \a range is the range which this range should contain
     *
     * Returns \e true if expansion occurred, \e false otherwise
     */
    bool expandToRange(Range range) noexcept;

    /*!
     * Confine this range if necessary to fit within \a range.
     *
     * \a range is the range which should contain this range
     *
     * Returns \e true if confinement occurred, \e false otherwise
     */
    bool confineToRange(Range range) noexcept;

    /*!
     * Check whether this range is wholly contained within one line, ie. if
     * the start() and end() positions are on the same line.
     *
     * Returns \e true if both the start and end positions are on the same
     *         line, otherwise \e false
     */
    constexpr bool onSingleLine() const noexcept
    {
        return start().line() == end().line();
    }

    /*!
     * Returns the number of lines separating the start() and end() positions.
     *
     * Returns the number of lines separating the start() and end() positions;
     *         0 if the start and end lines are the same.
     */
    constexpr int numberOfLines() const noexcept
    {
        return end().line() - start().line();
    }

    /*!
     * Returns the number of columns separating the start() and end() positions.
     *
     * Returns the number of columns separating the start() and end() positions;
     *         0 if the start and end columns are the same.
     */
    constexpr int columnWidth() const noexcept
    {
        return end().column() - start().column();
    }

    /*!
     * Returns true if this range contains no characters, ie. the start() and
     * end() positions are the same.
     *
     * Returns \e true if the range contains no characters, otherwise \e false
     */
    constexpr bool isEmpty() const noexcept
    {
        return start() == end();
    }

    // BEGIN comparison functions
    /*
     * \}
     *
     * Comparison
     *
     * The following functions perform checks against this range in comparison
     * to other lines, columns, cursors, and ranges.
     * \{
     */
    /*!
     * Check whether the this range wholly encompasses \e range.
     *
     * \a range is the range to check
     *
     * Returns \e true, if this range contains \e range, otherwise \e false
     */
    constexpr bool contains(Range range) const noexcept
    {
        return range.start() >= start() && range.end() <= end();
    }

    /*!
     * Check to see if \a cursor is contained within this range, ie >= start() and \< end().
     *
     * \a cursor is the position to test for containment
     *
     * Returns \e true if the cursor is contained within this range, otherwise \e false.
     */
    constexpr bool contains(Cursor cursor) const noexcept
    {
        return cursor >= start() && cursor < end();
    }

    /*!
     * Returns true if this range wholly encompasses \a line.
     *
     * \a line is the line to check
     *
     * Returns \e true if the line is wholly encompassed by this range, otherwise \e false.
     */
    constexpr bool containsLine(int line) const noexcept
    {
        return (line > start().line() || (line == start().line() && !start().column())) && line < end().line();
    }

    /*!
     * Check whether the range contains \e column.
     *
     * \a column is the column to check
     *
     * Returns \e true if the range contains \e column, otherwise \e false
     */
    constexpr bool containsColumn(int column) const noexcept
    {
        return column >= start().column() && column < end().column();
    }

    /*!
     * Check whether the this range overlaps with \e range.
     *
     * \a range is the range to check against
     *
     * Returns \e true, if this range overlaps with \e range, otherwise \e false
     */
    constexpr bool overlaps(Range range) const noexcept
    {
        return (range.start() <= start()) ? (range.end() > start()) : (range.end() >= end()) ? (range.start() < end()) : contains(range);
    }

    /*!
     * Check whether the range overlaps at least part of \e line.
     *
     * \a line is the line to check
     *
     * Returns \e true, if the range overlaps at least part of \e line, otherwise \e false
     */
    constexpr bool overlapsLine(int line) const noexcept
    {
        return line >= start().line() && line <= end().line();
    }

    /*!
     * Check to see if this range overlaps \a column; that is, if \a column is
     * between start().column() and end().column().  This function is most likely
     * to be useful in relation to block text editing.
     *
     * \a column is the column to test
     *
     * Returns \e true if the column is between the range's starting and ending
     *         columns, otherwise \e false.
     */
    constexpr bool overlapsColumn(int column) const noexcept
    {
        return start().column() <= column && end().column() > column;
    }

    /*!
     * Check whether \a cursor is located at either of the start() or end()
     * boundaries.
     *
     * \a cursor is the cursor to check
     *
     * Returns \e true if the cursor is equal to start() or end(),
     *         otherwise \e false.
     */
    constexpr bool boundaryAtCursor(Cursor cursor) const noexcept
    {
        return cursor == start() || cursor == end();
    }
    //!\}
    // END

    /*!
     * Intersects this range with another, returning the shared area of
     * the two ranges.
     *
     * \a range is the other range to intersect with this
     *
     * Returns the intersection of this range and the supplied \a range.
     */
    constexpr Range intersect(Range range) const noexcept
    {
        return ((!isValid() || !range.isValid() || *this > range || *this < range)) ? invalid() : Range(qMax(start(), range.start()), qMin(end(), range.end()));
    }

    /*!
     * Returns the smallest range which encompasses this range and the
     * supplied \a range.
     *
     * \a range is the other range to encompass
     *
     * Returns the smallest range which contains this range and the supplied \a range.
     */
    constexpr Range encompass(Range range) const noexcept
    {
        return (!isValid())      ? (range.isValid() ? range : invalid())
            : (!range.isValid()) ? (*this)
                                 : Range(qMin(start(), range.start()), qMax(end(), range.end()));
    }

    /*!
     * Addition operator. Takes two ranges and returns their summation.
     *
     * \a r1 is the first range
     *
     * \a r2 is the second range
     *
     * Returns a the summation of the two input ranges
     */
    constexpr friend Range operator+(Range r1, Range r2) noexcept
    {
        return Range(r1.start() + r2.start(), r1.end() + r2.end());
    }

    /*!
     * Addition assignment operator. Adds \a r2 to this range.
     *
     * \a r1 is the first range
     *
     * \a r2 is the second range
     *
     * Returns a reference to the cursor which has just been added to
     */
    friend Range &operator+=(Range &r1, Range r2) noexcept
    {
        r1.setRange(r1.start() + r2.start(), r1.end() + r2.end());
        return r1;
    }

    /*!
     * Subtraction operator. Takes two ranges and returns the subtraction
     * of \a r2 from \a r1.
     *
     * \a r1 is the first range
     *
     * \a r2 is the second range
     *
     * Returns a range representing the subtraction of \a r2 from \a r1
     */
    constexpr friend Range operator-(Range r1, Range r2) noexcept
    {
        return Range(r1.start() - r2.start(), r1.end() - r2.end());
    }

    /*!
     * Subtraction assignment operator. Subtracts \a r2 from \a r1.
     *
     * \a r1 is the first range
     *
     * \a r2 is the second range
     *
     * Returns a reference to the range which has just been subtracted from
     */
    friend Range &operator-=(Range &r1, Range r2) noexcept
    {
        r1.setRange(r1.start() - r2.start(), r1.end() - r2.end());
        return r1;
    }

    /*!
     * Intersects \a r1 and \a r2.
     *
     * \a r1 is the first range
     *
     * \a r2 is the second range
     *
     * Returns the intersected range, invalid() if there is no overlap
     */
    constexpr friend Range operator&(Range r1, Range r2) noexcept
    {
        return r1.intersect(r2);
    }

    /*!
     * Intersects \a r1 with \a r2 and assigns the result to \a r1.
     *
     * \a r1 is the range to assign the intersection to
     *
     * \a r2 is the range to intersect \a r1 with
     *
     * Returns a reference to this range, after the intersection has taken place
     */
    friend Range &operator&=(Range &r1, Range r2) noexcept
    {
        r1.setRange(r1.intersect(r2));
        return r1;
    }

    /*!
     * Equality operator.
     *
     * \a r1 is the first range to compare
     *
     * \a r2 is the second range to compare
     *
     * Returns \e true if \e r1 and \e r2 equal, otherwise \e false
     */
    constexpr friend bool operator==(Range r1, Range r2) noexcept
    {
        return r1.start() == r2.start() && r1.end() == r2.end();
    }

    /*!
     * Inequality operator.
     *
     * \a r1 is the first range to compare
     *
     * \a r2 is the second range to compare
     *
     * Returns \e true if \e r1 and \e r2 do \e not equal, otherwise \e false
     */
    constexpr friend bool operator!=(Range r1, Range r2) noexcept
    {
        return r1.start() != r2.start() || r1.end() != r2.end();
    }

    /*!
     * Greater than operator.  Looks only at the position of the two ranges,
     * does not consider their size.
     *
     * \a r1 is the first range to compare
     *
     * \a r2 is the second range to compare
     *
     * Returns \e true if \e r1 starts after where \e r2 ends, otherwise \e false
     */
    constexpr friend bool operator>(Range r1, Range r2) noexcept
    {
        return r1.start() > r2.end();
    }

    /*!
     * Less than operator.  Looks only at the position of the two ranges,
     * does not consider their size.
     *
     * \a r1 is the first range to compare
     *
     * \a r2 is the second range to compare
     *
     * Returns \e true if \e r1 ends before \e r2 begins, otherwise \e false
     */
    constexpr friend bool operator<(Range r1, Range r2) noexcept
    {
        return r1.end() < r2.start();
    }

private:
    /*!
     * This range's start cursor pointer.
     *
     * \internal
     */
    Cursor m_start;

    /*!
     * This range's end cursor pointer.
     *
     * \internal
     */
    Cursor m_end;
};

KTEXTEDITOR_EXPORT size_t qHash(KTextEditor::Range range, size_t seed = 0) noexcept;
}

Q_DECLARE_TYPEINFO(KTextEditor::Range, Q_RELOCATABLE_TYPE);

/*!
 * qDebug() stream operator.  Writes this range to the debug output in a nicely formatted way.
 *
 * \relates KTextEditor::Range
 */
KTEXTEDITOR_EXPORT QDebug operator<<(QDebug s, KTextEditor::Range range);

namespace QTest
{
// forward declaration of template in qtestcase.h
template<typename T>
char *toString(const T &);

/*
 * QTestLib integration to have nice output in e.g. QCOMPARE failures.
 */
template<>
KTEXTEDITOR_EXPORT char *toString(const KTextEditor::Range &range);
}

#endif
