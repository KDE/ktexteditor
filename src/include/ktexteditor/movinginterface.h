/*
    SPDX-FileCopyrightText: 2010 Christoph Cullmann <cullmann@kde.org>

    Based on code of the SmartCursor/Range by:
    SPDX-FileCopyrightText: 2003-2005 Hamish Rodda <rodda@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef KTEXTEDITOR_MOVINGINTERFACE_H
#define KTEXTEDITOR_MOVINGINTERFACE_H

#include <ktexteditor/movingcursor.h>
#include <ktexteditor/movingrange.h>
#include <ktexteditor/movingrangefeedback.h>
#include <ktexteditor_export.h>

namespace KTextEditor
{
/**
 * \class MovingInterface movinginterface.h <KTextEditor/MovingInterface>
 *
 * \brief Document interface for MovingCursor%s and MovingRange%s.
 *
 * \ingroup kte_group_doc_extensions
 * \ingroup kte_group_moving_classes
 *
 * This class provides the interface for KTextEditor::Documents to create MovingCursors/Ranges.
 *
 * \author Christoph Cullmann \<cullmann@kde.org\>
 *
 * \since 4.5
 */
class KTEXTEDITOR_EXPORT MovingInterface
{
    //
    // Constructor/Destructor
    //
public:
    /**
     * Default constructor
     */
    MovingInterface();

    /**
     * Virtual destructor
     */
    virtual ~MovingInterface();

    //
    // Normal API
    //
public:
    /**
     * Create a new moving cursor for this document.
     * @param position position of the moving cursor to create
     * @param insertBehavior insertion behavior
     * @return new moving cursor for the document
     */
    virtual MovingCursor *newMovingCursor(const Cursor &position, MovingCursor::InsertBehavior insertBehavior = MovingCursor::MoveOnInsert) = 0;

    /**
     * Create a new moving range for this document.
     * @param range range of the moving range to create
     * @param insertBehaviors insertion behaviors
     * @param emptyBehavior behavior on becoming empty
     * @return new moving range for the document
     */
    virtual MovingRange *newMovingRange(const Range &range,
                                        MovingRange::InsertBehaviors insertBehaviors = MovingRange::DoNotExpand,
                                        MovingRange::EmptyBehavior emptyBehavior = MovingRange::AllowEmpty) = 0;

    /**
     * Current revision
     * @return current revision
     */
    virtual qint64 revision() const = 0;

    /**
     * Last revision the buffer got successful saved
     * @return last revision buffer got saved, -1 if none
     */
    virtual qint64 lastSavedRevision() const = 0;

    /**
     * Lock a revision, this will keep it around until released again.
     * But all revisions will always be cleared on buffer clear() (and therefor load())
     * @param revision revision to lock
     */
    virtual void lockRevision(qint64 revision) = 0;

    /**
     * Release a revision.
     * @param revision revision to release
     */
    virtual void unlockRevision(qint64 revision) = 0;

    /**
     * Transform a cursor from one revision to an other.
     * @param cursor cursor to transform
     * @param insertBehavior behavior of this cursor on insert of text at its position
     * @param fromRevision from this revision we want to transform
     * @param toRevision to this revision we want to transform, default of -1 is current revision
     */
    virtual void
    transformCursor(KTextEditor::Cursor &cursor, KTextEditor::MovingCursor::InsertBehavior insertBehavior, qint64 fromRevision, qint64 toRevision = -1) = 0;

    /**
     * Transform a cursor from one revision to an other.
     * @param line line number of the cursor to transform
     * @param column column number of the cursor to transform
     * @param insertBehavior behavior of this cursor on insert of text at its position
     * @param fromRevision from this revision we want to transform
     * @param toRevision to this revision we want to transform, default of -1 is current revision
     */
    virtual void
    transformCursor(int &line, int &column, KTextEditor::MovingCursor::InsertBehavior insertBehavior, qint64 fromRevision, qint64 toRevision = -1) = 0;

    /**
     * Transform a range from one revision to an other.
     * @param range range to transform
     * @param insertBehaviors behavior of this range on insert of text at its position
     * @param emptyBehavior behavior on becoming empty
     * @param fromRevision from this revision we want to transform
     * @param toRevision to this revision we want to transform, default of -1 is current revision
     */
    virtual void transformRange(KTextEditor::Range &range,
                                KTextEditor::MovingRange::InsertBehaviors insertBehaviors,
                                MovingRange::EmptyBehavior emptyBehavior,
                                qint64 fromRevision,
                                qint64 toRevision = -1) = 0;

    //
    // Signals
    //
public:
    /**
     * This signal is emitted before the cursors/ranges/revisions of a document
     * are destroyed as the document is deleted.
     * @param document the document which the interface belongs to which is in the process of being deleted
     */
    void aboutToDeleteMovingInterfaceContent(KTextEditor::Document *document);

    /**
     * This signal is emitted before the ranges of a document are invalidated
     * and the revisions are deleted as the document is cleared (for example on
     * load/reload). While this signal is emitted, the old document content is
     * still valid and accessible before the clear.
     * @param document the document which the interface belongs to which will invalidate its data
     */
    void aboutToInvalidateMovingInterfaceContent(KTextEditor::Document *document);

private:
    /**
     * private d-pointer
     */
    class MovingInterfacePrivate *const d = nullptr;
};

}

Q_DECLARE_INTERFACE(KTextEditor::MovingInterface, "org.kde.KTextEditor.MovingInterface")

#endif
