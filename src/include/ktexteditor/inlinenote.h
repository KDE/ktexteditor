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
class KateInlineNoteData;
namespace KTextEditor { class InlineNoteProvider; }

namespace KTextEditor {

/**
 * Describes an inline note.
 *
 * This class contains all the information required to deal with a particular
 * inline note. It is instantiated and populated with information internally by
 * KTextEditor based on the list of notes returned by InlineNoteProvider::inlineNotes(),
 * and then passed back to the user of the API.
 *
 * @note Users of the InlineNoteInterface API should never create a InlineNote
 *       themselves. Maybe it helps to think of a InlineNote as if it were a
 *       QModelIndex. Only the internal KTextEditor implementation creates them.
 *
 * @since 5.50
 */
class KTEXTEDITOR_EXPORT InlineNote
{
public:
    /**
     * Constructs an inline note. User code never calls this constructor,
     * since notes are created internally only from the columns returned by
     * InlineNoteProvider::inlineNotes(), and then passed around as handles
     * grouping useful information.
     */
    InlineNote(const KateInlineNoteData & data);

    /**
     * Returns the width of this note in pixels.
     */
    qreal width() const;

    /**
     * The provider which created this note
     */
    InlineNoteProvider* provider() const;

    /**
     * The View this note is shown in.
     */
    const KTextEditor::View* view() const;

    /**
     * The cursor position of this note.
     */
    KTextEditor::Cursor position() const;

    /**
     * The index of this note, i.e. its index in the vector returned by
     * the provider for a given line
     */
    int index() const;

    /**
     * Returns whether the mouse cursor is currently over this note.
     * @note This flag is useful when in InlineNoteProvider::paintInlineNote().
     */
    bool underMouse() const;

    /**
     * The font of the text surrounding this note.
     * This can be used to obtain the QFontMetrics or similar font information.
     */
    QFont font() const;

    /**
     * The height of the line containing this note
     */
    int lineHeight() const;

private:
    // Internal implementation data structure.
    const KateInlineNoteData & d;
};

}

#endif
