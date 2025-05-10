/*
    SPDX-FileCopyrightText: 2010 Christoph Cullmann <cullmann@kde.org>

    Based on code of the SmartCursor/Range by:
    SPDX-FileCopyrightText: 2003-2005 Hamish Rodda <rodda@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef KTEXTEDITOR_MOVINGRANGE_H
#define KTEXTEDITOR_MOVINGRANGE_H

#include <ktexteditor/attribute.h>
#include <ktexteditor/linerange.h>
#include <ktexteditor/movingcursor.h>
#include <ktexteditor/range.h>
#include <ktexteditor_export.h>

class QDebug;

namespace KTextEditor
{
class Document;
class View;
class MovingRangeFeedback;

/*!
 * \class KTextEditor::MovingRange
 * \inmodule KTextEditor
 * \inheaderfile KTextEditor/MovingRange
 *
 * \brief A range that is bound to a specific Document, and maintains its
 * position.
 *
 * \ingroup kte_group_moving_classes
 *
 * \target movingrange_intro
 * \section1 Introduction
 *
 * A MovingRange is an extension of the basic Range class. It maintains its
 * position in the document. As a result of this, MovingRange%s may not be
 * copied, as they need to maintain a connection to the associated Document.
 *
 * Create a new MovingRange like this:
 * \code
 * auto* movingRange = aDocument->newMovingRange(range);
 * \endcode
 *
 * The ownership of the range is passed to the user. When finished with a MovingRange,
 * simply delete it.
 *
 * \target movingrange_behavior
 * \section1 Editing Behavior
 *
 * The insert behavior controls how the range reacts to characters inserted
 * at the range boundaries, i.e. at the start of the range or the end of the
 * range. Either the range boundary moves with text insertion, or it stays.
 * Use setInsertBehaviors() and insertBehaviors() to set and query the current
 * insert behavior.
 *
 * When the start() and end() Cursor of a range equal, isEmpty() returns true.
 * Further, the empty-behavior can be changed such that the start() and end()
 * Cursor%s of MovingRange%s that get empty are automatically set to (-1, -1).
 * Use setEmptyBehavior() and emptyBehavior() to control the empty behavior.
 *
 * \warning MovingRanges may be set to (-1, -1, -1, -1) at any time, if the
 * user reloads a document (F5)! Use a MovingRangeFeedback to get notified
 * if you need to catch this case, and/or listen to the signal
 * MovingInterface::aboutToInvalidateMovingInterfaceContent().
 *
 * \target movingrange_feedback
 * \section1 MovingRange Feedback
 *
 * With setFeedback() a feedback instance can be associated with the moving
 * range. The MovingRangeFeedback notifies about the following events:
 * \list
 * \li the text cursor (caret) entered the range,
 * \li the text cursor (caret) left the range,
 * \li the mouse cursor entered the range,
 * \li the mouse cursor left the range,
 * \li the range became empty, i.e. start() == end(),
 * \li the range became invalid, i.e. start() == end() == (-1, -1).
 * \endlist
 *
 * If a feedback is not needed anymore, call setFeedback(0).
 *
 * \target movingrange_details
 * \section1 Working with Ranges
 *
 * There are several convenience methods that make working with MovingRanges
 * very simple. For instance, use isEmpty() to check if the start() Cursor
 * equals the end() Cursor. Use contains(), containsLine() or containsColumn()
 * to check whether the MovingRange contains a Range, a Cursor, a line or
 * column. The same holds for overlaps(), overlapsLine() and overlapsColumn().
 * Besides onSingleLine() returns whether a MovingRange spans only one line.
 *
 * For compatibility, a MovingRange can be explicitly converted to a simple
 * Range by calling toRange(), or implicitly by the Range operator.
 *
 * \target movingrange_highlighting
 * \section1 Arbitrary Highlighting
 *
 * With setAttribute() highlighting Attribute%s can be assigned to a
 * MovingRange. By default, this highlighting is used in all views of a
 * document. Use setView(), if the highlighting should only appear in a
 * specific view. Further, if the additional highlighting should not be
 * printed call setAttributeOnlyForViews() with the parameter true.
 *
 * \target movingrange_example
 * \section1 MovingRange Example
 *
 * In the following example, we assume the KTextEditor::Document has the
 * contents:
 * \code
 * void printText(const std::string & text); // this is line 3
 * \endcode
 * In order to highlight the function name \e printText with a yellow background
 * color, the following code is needed:
 * \code
 * KTextEditor::View * view = ...;
 * KTextEditor::Document * doc = view->document();
 *
 * // range is of type KTextEditor::MovingRange*
 * auto range = doc->newMovingRange(KTextEditor::Range(3, 5, 3, 14));
 *
 * KTextEditor::Attribute::Ptr attrib = new KTextEditor::Attribute();
 * attrib->setBackground(Qt::yellow);
 *
 * range->setAttribute(attrib);
 * \endcode
 *
 * MovingRange%s are invalidated automatically when a document is cleared or closed.
 *
 * \sa Cursor, MovingCursor, Range, MovingInterface, MovingRangeFeedback
 *
 * \since 4.5
 */
class KTEXTEDITOR_EXPORT MovingRange
{
    //
    // sub types
    //
public:
    /*!
       \enum KTextEditor::MovingRange::InsertBehavior

       \brief Determines how the range reacts to characters inserted immediately outside the range.

       \value DoNotExpand
       Don't expand to encapsulate new characters in either direction. This is the default.

       \value ExpandLeft
       Expand to encapsulate new characters to the left of the range.

       \value ExpandRight
       Expand to encapsulate new characters to the right of the range.

       \sa InsertBehaviors
     */
    enum InsertBehavior {
        DoNotExpand = 0x0,
        ExpandLeft = 0x1,
        ExpandRight = 0x2
    };
    Q_DECLARE_FLAGS(InsertBehaviors, InsertBehavior)

    /*!
       \enum KTextEditor::MovingRange::EmptyBehavior

       Behavior of range if it becomes empty.

       \value AllowEmpty
       Allow range to be empty
       \value InvalidateIfEmpty
       Invalidate range if it becomes empty
     */
    enum EmptyBehavior {
        AllowEmpty = 0x0,
        InvalidateIfEmpty = 0x1
    };

    //
    // stuff that needs to be implemented by editor part cursors
    //
public:
    /*!
     * Set insert behaviors.
     *
     * \a insertBehaviors are the new insert behaviors
     *
     */
    virtual void setInsertBehaviors(InsertBehaviors insertBehaviors) = 0;

    /*!
     * Get current insert behaviors.
     * Returns current insert behaviors
     */
    virtual InsertBehaviors insertBehaviors() const = 0;

    /*!
     * Set if this range will invalidate itself if it becomes empty.
     *
     * \a emptyBehavior is the behavior on becoming empty
     *
     */
    virtual void setEmptyBehavior(EmptyBehavior emptyBehavior) = 0;

    /*!
     * Will this range invalidate itself if it becomes empty?
     * Returns behavior on becoming empty
     */
    virtual EmptyBehavior emptyBehavior() const = 0;

    /*!
     * Gets the document to which this range is bound.
     *
     * Returns a pointer to the document
     */
    virtual Document *document() const = 0;

    /*!
     * Set the range of this range.
     *
     * A TextRange is not allowed to be empty, as soon as start == end position, it will become
     * automatically invalid!
     *
     * \a range new range for this clever range
     *
     */
    virtual void setRange(KTextEditor::Range range) = 0;

    /*!
     * Set the range of this range and the connected attribute.
     * Avoids internal overhead of separate setting that.
     *
     * A TextRange is not allowed to be empty, as soon as start == end position, it will become
     * automatically invalid!
     *
     * \a range is the new range for this clever range
     *
     * \a attribute is the attribute to assign to this range. If null, simply removes the previous Attribute.
     *
     * \since 6.0
     */
    virtual void setRange(KTextEditor::Range range, Attribute::Ptr attribute) = 0;

    /*!
     * Set the range of this range and the connected attribute and Z-depth.
     * Avoids internal overhead of separate setting that.
     *
     * A TextRange is not allowed to be empty, as soon as start == end position, it will become
     * automatically invalid!
     *
     * \a range is the new range for this clever range
     *
     * \a attribute is the attribute to assign to this range. If null, simply removes the previous Attribute.
     *
     * \a zDepth is the new Z-depth of this range
     *
     * \since 6.0
     */
    virtual void setRange(KTextEditor::Range range, Attribute::Ptr attribute, qreal zDepth) = 0;

    /*!
     * Retrieve start cursor of this range, read-only.
     * Returns start cursor
     */
    virtual const MovingCursor &start() const = 0;

    /*!
     * Retrieve end cursor of this range, read-only.
     * Returns end cursor
     */
    virtual const MovingCursor &end() const = 0;

    /*!
     * Gets the active view for this range. Might be already invalid, internally only used for pointer comparisons.
     *
     * Returns a pointer to the active view
     */
    virtual View *view() const = 0;

    /*!
     * Sets the currently active view for this range.
     * This will trigger update of the relevant view parts, if the view changed.
     * Set view before the attribute, that will avoid not needed redraws.
     *
     * \a view is the View to assign to this range. If null, simply
     * removes the previous view.
     *
     */
    virtual void setView(View *view) = 0;

    /*!
     * Gets the active Attribute for this range.
     *
     * Returns a pointer to the active attribute
     */
    virtual const Attribute::Ptr &attribute() const = 0;

    /*!
     * Sets the currently active attribute for this range.
     * This will trigger update of the relevant view parts, if the attribute changed.
     *
     * \a attribute is the attribute to assign to this range. If null, simply
     * removes the previous Attribute.
     *
     */
    virtual void setAttribute(Attribute::Ptr attribute) = 0;

    /*!
     * Is this range's attribute only visible in views, not for example prints?
     * Default is false.
     * Returns range visible only for views
     */
    virtual bool attributeOnlyForViews() const = 0;

    /*!
     * Set if this range's attribute is only visible in views, not for example prints.
     *
     * \a onlyForViews if true, the attribute is only valid for views
     *
     */
    virtual void setAttributeOnlyForViews(bool onlyForViews) = 0;

    /*!
     * Gets the active MovingRangeFeedback for this range.
     *
     * Returns a pointer to the active MovingRangeFeedback
     */
    virtual MovingRangeFeedback *feedback() const = 0;

    /*!
     * Sets the currently active MovingRangeFeedback for this range.
     * This will trigger evaluation if feedback must be send again (for example if mouse is already inside range).
     *
     * \a feedback is the MovingRangeFeedback to assign to this range. If null, simply
     * removes the previous MovingRangeFeedback.
     *
     */
    virtual void setFeedback(MovingRangeFeedback *feedback) = 0;

    /*!
     * Gets the current Z-depth of this range.
     * Ranges with smaller Z-depth than others will win during rendering.
     * Default is 0.0.
     *
     * Defined depths for common kind of ranges use in editor components implementing this interface,
     * smaller depths are more more in the foreground and will win during rendering:
     * \list
     * \li Selection == -100000.0
     * \li Search == -10000.0
     * \li Bracket Highlighting == -1000.0
     * \li Folding Hover == -100.0
     * \endlist
     *
     * Returns current Z-depth of this range
     */
    virtual qreal zDepth() const = 0;

    /*!
     * Set the current Z-depth of this range.
     * Ranges with smaller Z-depth than others will win during rendering.
     * This will trigger update of the relevant view parts, if the depth changed.
     * Set depth before the attribute, that will avoid not needed redraws.
     * Default is 0.0.
     *
     * \a zDepth is new Z-depth of this range
     *
     */
    virtual void setZDepth(qreal zDepth) = 0;

    /*!
     * Destruct the moving range.
     */
    virtual ~MovingRange();

    //
    // forbidden stuff
    //
protected:
    /*!
     * For inherited class only.
     */
    MovingRange();

public:
    /*!
     * no copy constructor, don't allow this to be copied.
     */
    MovingRange(const MovingRange &) = delete;

    /*!
     * no assignment operator, no copying around clever ranges.
     */
    MovingRange &operator=(const MovingRange &) = delete;

    //
    // convenience API
    //
public:
    /*!
     * \overload
     * Set the range of this range
     * A TextRange is not allowed to be empty, as soon as start == end position, it will become
     * automatically invalid!
     *
     * \a start is the new start for this clever range
     *
     * \a end is the new end for this clever range
     *
     */
    void setRange(Cursor start, Cursor end);

    /*!
     * Convert this clever range into a dumb one.
     * Returns normal range
     */
    const Range toRange() const
    {
        return Range(start().toCursor(), end().toCursor());
    }

    /*!
     * Convert this clever range into a dumb one. Equal to toRange, allowing to use implicit conversion.
     * Returns normal range
     */
    operator Range() const
    {
        return Range(start().toCursor(), end().toCursor());
    }

    /*!
     * Convert this MovingRange to a simple LineRange.
     * Returns LineRange from the start line to the end line of this range.
     */
    inline LineRange toLineRange() const Q_DECL_NOEXCEPT
    {
        return {start().line(), end().line()};
    }

    /*!
     * Returns true if this range contains no characters, ie. the start() and
     * end() positions are the same.
     *
     * Returns \e true if the range contains no characters, otherwise \e false
     */
    inline bool isEmpty() const
    {
        return start() == end();
    }

    // BEGIN comparison functions
    /*
     * \name Comparison
     *
     * The following functions perform checks against this range in comparison
     * to other lines, columns, cursors, and ranges.
     */
    /*!
     * Check whether the this range wholly encompasses \e range.
     *
     * \a range is the range to check
     *
     * Returns \e true, if this range contains \e range, otherwise \e false
     */
    inline bool contains(const Range &range) const
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
    inline bool contains(Cursor cursor) const
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
    inline bool containsLine(int line) const
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
    inline bool containsColumn(int column) const
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
    bool overlaps(const Range &range) const;

    /*!
     * Check whether the range overlaps at least part of \e line.
     *
     * \a line is the line to check
     *
     * Returns \e true, if the range overlaps at least part of \e line, otherwise \e false
     */
    inline bool overlapsLine(int line) const
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
    inline bool overlapsColumn(int column) const
    {
        return start().column() <= column && end().column() > column;
    }

    /*!
     * Check whether the start() and end() cursors of this range
     * are on the same line.
     *
     * Returns \e true if both the start and end positions are on the same
     *         line, otherwise \e false
     */
    inline bool onSingleLine() const
    {
        return start().line() == end().line();
    }

    /*!
     * Returns the number of lines separating the start() and end() positions.
     *
     * Returns the number of lines separating the start() and end() positions;
     *         0 if the start and end lines are the same.
     */
    inline int numberOfLines() const Q_DECL_NOEXCEPT
    {
        return end().line() - start().line();
    }

    // END comparison functions
};

Q_DECLARE_OPERATORS_FOR_FLAGS(MovingRange::InsertBehaviors)

/*!
 * qDebug() stream operator. Writes this range to the debug output in a nicely formatted way.
 *
 * \a s is the debug stream
 *
 * \a range is the range to print
 *
 * Returns debug stream
 *
 * \relates KTextEditor::MovingRange
 */
KTEXTEDITOR_EXPORT QDebug operator<<(QDebug s, const MovingRange *range);

/*!
 * qDebug() stream operator. Writes this range to the debug output in a nicely formatted way.
 *
 * \a s is the debug stream
 *
 * \a range is the range to print
 *
 * Returns debug stream
 *
 * \relates KTextEditor::MovingRange
 */
KTEXTEDITOR_EXPORT QDebug operator<<(QDebug s, const MovingRange &range);
}

#endif
