/* This file is part of the KDE libraries

   Copyright 2018 Sven Brauch <mail@svenbrauch.de>
   Copyright 2018 Michal Srb <michalsrb@gmail.com>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 2.1 of the License, or (at your option) version 3, or any
   later version accepted by the membership of KDE e.V. (or its
   successor approved by the membership of KDE e.V.), which shall
   act as a proxy defined in Section 6 of version 3 of the license.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/
#ifndef KTEXTEDITOR_INLINENOTEPROVIDER_H
#define KTEXTEDITOR_INLINENOTEPROVIDER_H

#include <ktexteditor_export.h>

#include <ktexteditor/inlinenote.h>

namespace KTextEditor {
/**
 * @brief A source of inline notes for a document.
 *
 * InlineNoteProvider is a object that can be queried for inline notes in the
 * view. It emits signals when the notes change and should be queried again.
 *
 * @see InlineNoteInterface
 * @since 5.50
 */
class KTEXTEDITOR_EXPORT InlineNoteProvider : public QObject
{
    Q_OBJECT

public:
    /**
     * Default constructor.
     */
    InlineNoteProvider();

    /**
     * Virtual destructor to allow inheritance.
     */
    virtual ~InlineNoteProvider();

    /**
     * Get list of inline notes for given line.
     *
     * Should return a vector of columns on which the notes are located.
     * 0 means the note is located before the first character of the line.
     * 1 means the note is located after the first character, etc. If the
     * returned number is greater than the length of the line, the note will be
     * placed behind the text as if there were additional spaces.
     *
     * @note When returning multiple InlineNote%s, use InlineNote::index() to
     *       map the InlineNote to this QVector's index.
     *
     * @param line Line number
     * @returns vector of columns where inline notes appear in this line
     */
    virtual QVector<int> inlineNotes(int line) const = 0;

    /**
     * Width to be reserved for the note in the text.
     *
     * Typically, a custom width with the current line height can be returned.
     * If the width depends on the font size, note.font() can be used to obtain
     * the font metrics.
     *
     * Example to reserve a square size for painting:
     * @code
     * return QSize(note.lineHeight(), lineHeight());
     * @endcode
     *
     * @note Do not return heights that are larger than note.lineHeight(),
     *       since the painting code clips to the line height anyways.
     *
     * @param note the InlineNote for which the size is queried
     * @return the required size of the InlineNote
     */
    virtual QSize inlineNoteSize(const InlineNote& note) const = 0;

    /**
     * Paint the note into the line.
     *
     * The method should use the given painter to render the note into the
     * line. The painter is translated such that coordinates 0x0 mark the top
     * left corner of the note. The method should not paint outside rectangle
     * given by the size previously returned by inlineNoteSize().
     *
     * The method is given the height of the line, the metrics of current font
     * and the font which it may use during painting.
     *
     * If wanted, you can use note.underMouse() to e.g. highlight the
     *
     * @param note note to paint, containing location and index
     * @param painter painter prepared for rendering the note
     */
    virtual void paintInlineNote(const InlineNote& note, QPainter& painter) const = 0;

    /**
     * Invoked when a note is activated by the user.
     *
     * This method is called when a user activates a note, i.e. clicks on it.
     * Coordinates of @p pos are in note coordinates, i.e.  relative to the note's
     * top-left corner (same coordinate system as the painter has in paintInlineNote()).
     *
     * The default implementation does nothing.
     *
     * @param note the note which was activated
     * @param buttons the button(s) the note was clicked with
     * @param globalPos the point the note was clicked at in global screen coordinates
     */
    virtual void inlineNoteActivated(const InlineNote& note, Qt::MouseButtons buttons, const QPoint& globalPos);

    /**
     * Invoked when the mouse cursor moves into the @p note when it was outside before.
     *
     * The default implementation does nothing.
     *
     * @param note the note which was activated
     * @param globalPos the location of the mouse cursor in global screen coordinates
     */
    virtual void inlineNoteFocusInEvent(const InlineNote& note, const QPoint& globalPos);

    /**
     * Invoked when the mouse cursor leaves the note.
     *
     * The default implementation does nothing.
     *
     * @param note the note which was deactivated
     */
    virtual void inlineNoteFocusOutEvent(const InlineNote& note);

    /**
     * Invoked when the mouse cursor moves inside the note.
     *
     * The default implementation does nothing.
     *
     * @param note the note which was hovered
     * @param globalPos the location of the mouse cursor in global screen coordinates
     */
    virtual void inlineNoteMouseMoveEvent(const InlineNote& note, const QPoint& globalPos);

Q_SIGNALS:
    /**
     * The provider should emit the signal inlineNotesReset() when almost all inline notes
     * changed.
     */
    void inlineNotesReset();

    /**
     * The provider should emit the signal inlineNotesChanged() whenever one
     * or more InlineNote%s on the line changed.
     */
    void inlineNotesChanged(int line);

private:
    class InlineNoteProviderPrivate * const d = nullptr;
};

}

#endif
