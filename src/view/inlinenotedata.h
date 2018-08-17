/* This file is part of the KDE libraries

   Copyright 2018 Sven Brauch <mail@svenbrauch.de>
   Copyright 2018 Michal Srb <michalsrb@gmail.com>
   Copyright 2018 Dominik Haumann <dhaumann@kde.org>

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
#ifndef KATE_INLINENOTE_DATA_H
#define KATE_INLINENOTE_DATA_H

#include <ktexteditor/cursor.h>
#include <QFont>

namespace KTextEditor {
    class InlineNoteProvider;
    class View;
}

/**
 * Internal data container for KTextEditor::InlineNote interface.
 */
class KateInlineNoteData
{
public:
    KateInlineNoteData() = default;
    KateInlineNoteData(KTextEditor::InlineNoteProvider* provider,
                       const KTextEditor::View* view,
                       const KTextEditor::Cursor& position,
                       int index,
                       bool underMouse,
                       const QFont& font,
                       int lineHeight);

    KTextEditor::InlineNoteProvider* m_provider = nullptr;
    const KTextEditor::View* m_view = nullptr;
    KTextEditor::Cursor m_position = KTextEditor::Cursor::invalid();
    int m_index = -1;
    bool m_underMouse = false;
    QFont m_font;
    int m_lineHeight = -1;
};

#endif
