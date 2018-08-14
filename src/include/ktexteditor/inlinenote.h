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
#ifndef KTEXTEDITOR_INLINENOTE_H
#define KTEXTEDITOR_INLINENOTE_H

#include <ktexteditor/view.h>
#include <ktexteditor/cursor.h>

class QFont;
namespace KTextEditor { class InlineNoteProvider; }

namespace KTextEditor {

/**
 * Describes an inline note.
 *
 * This structure contains all the information required to deal with
 * a particular inline note. It is instantiated and populated with information
 * internally by KTextEditor based on the list of notes returned by
 * InlineNoteProvider::inlineNotes(), and then passed back to the user of the API.
 *
 * Users of the InlineNoteInterface API should never need to modify instances
 * of this structure.
 *
 * @since 5.50
 */
class KTEXTEDITOR_EXPORT InlineNote
{
public:
    /**
     * Constructs an inline note. User code usually does not need to call this,
     * notes are created from the columns returned by InlineNoteProvider::inlineNotes(int line),
     * and then passed around as handles grouping useful information.
     */
    InlineNote(InlineNoteProvider* provider, const KTextEditor::Cursor& position, int index,
               const KTextEditor::View* view, QFont font, int lineHeight, bool hasFocus);

    /**
     * Constructs an invalid inline note, i.e. isValid() will return false.
     */
    InlineNote();

    /**
     * Returns the column this note appears in.
     */
    int column() const;

    /**
     * Returns the width of this note in pixels.
     */
    qreal width() const;

    /**
     * Tells whether this note is valid, i.e. whether it has a valid provider and location set.
     */
    bool isValid() const;

    /**
     * Equality of notes. Only checks provider, index, and position.
     */
    bool operator==(const InlineNote& other) const;

    /**
     * Transforms the given @p pos from note coordinates to global (screen) coordinates.
     *
     * Useful for showing a popup; to e.g. show a popup at the bottom left corner
     * of a note, show it at @c mapToGlobal({0, noteHeight}).
     */
    QPoint mapToGlobal(const QPoint& pos) const;

    /**
     * The provider which created this note
     */
    InlineNoteProvider* provider() const;

    /**
     * The view this note is shown in
     */
    const KTextEditor::View* view() const;

    /**
     * The position of this note
     */
    KTextEditor::Cursor position() const;

    /**
     * The index of this note, i.e. its index in the vector returned by
     * the provider for a given line
     */
    int index() const;

    /**
     * Whether the mouse cursor is currently over this note; only set
     * when paintInlineNote() is called
     */
    bool hasFocus() const;

    /**
     * The font of the text surrounding this note
     */
    QFont font() const;

    /**
     * The height of the line containing this note
     */
    int lineHeight() const;

private:
    InlineNoteProvider* m_provider = nullptr;
    const KTextEditor::View* m_view = nullptr;
    KTextEditor::Cursor m_position;
    int m_index = -1;
    bool m_hasFocus = false;
    QFont m_font;
    int m_lineHeight = -1;

    // For future use, in case members need to be added.
    // TODO KF6: remove if it turns out this is unneeded.
    class InlineNotePrivate* d = nullptr;
};

}

#endif
