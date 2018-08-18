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
#ifndef KTEXTEDITOR_INLINENOTEINTERFACE_H
#define KTEXTEDITOR_INLINENOTEINTERFACE_H

#include <QString>

#include <ktexteditor_export.h>

#include <ktexteditor/cursor.h>
#include <ktexteditor/view.h>

class QPainter;

namespace KTextEditor
{

class InlineNoteProvider;

/**
 * @brief Inline notes interface for rendering notes in the text.
 *
 * @ingroup kte_group_view_extensions
 *
 * @section inlinenote_introduction Introduction
 *
 * The inline notes interface provides a way to render arbitrary things in
 * the text. The text layout of the line is adapted to create space for the
 * note. Possible applications include showing a name of a function parameter
 * in a function call or rendering a square with a color preview next to CSS
 * color property.
 *
 * \image html inlinenote.png "Inline note showing a CSS color preview"
 *
 * To register as inline note provider, call registerInlineNoteProvider() with
 * an instance that inherits InlineNoteProvider. Finally, make sure you remove
 * your inline note provider by calling unregisterInlineNoteProvider().
 *
 * @section inlinenote_access Accessing the InlineNoteInterface
 *
 * The InlineNoteInterface is an extension interface for a
 * View, i.e. the View inherits the interface. Use qobject_cast to access the
 * interface:
 * @code
 * // view is of type KTextEditor::View*
 * auto iface = qobject_cast<KTextEditor::InlineNoteInterface*>(view);
 *
 * if (iface) {
 *     // the implementation supports the interface
 *     // myProvider inherits KTextEditor::InlineNoteProvider
 *     iface->registerInlineNoteProvider(myProvider);
 * } else {
 *     // the implementation does not support the interface
 * }
 * @endcode
 *
 * @see InlineNoteProvider
 * @see InlineNote
 *
 * @author Sven Brauch, Michal Srb
 * @since 5.50
 */
class KTEXTEDITOR_EXPORT InlineNoteInterface
{
public:
    InlineNoteInterface();
    virtual ~InlineNoteInterface();

    /**
     * Register the inline note provider @p provider.
     *
     * Whenever a line is painted, the @p provider will be queried for notes
     * that should be painted in it. When the provider is about to be
     * destroyed, make sure to call unregisterInlineNoteProvider() to avoid a
     * dangling pointer.
     *
     * @param provider inline note provider
     * @see unregisterInlineNoteProvider(), InlineNoteProvider
     */
    virtual void registerInlineNoteProvider(KTextEditor::InlineNoteProvider *provider) = 0;

    /**
     * Unregister the inline note provider @p provider.
     *
     * @param provider inline note provider to unregister
     * @see registerInlineNoteProvider(), InlineNoteProvider
     */
    virtual void unregisterInlineNoteProvider(KTextEditor::InlineNoteProvider *provider) = 0;
};

}

Q_DECLARE_INTERFACE(KTextEditor::InlineNoteInterface, "org.kde.KTextEditor.InlineNoteInterface")

#endif
