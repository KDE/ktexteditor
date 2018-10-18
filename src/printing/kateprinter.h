/*  This file is part of the KDE libraries and the Kate part.
 *
 *  Copyright (C) 2002-2010 Anders Lund <anders@alweb.dk>
 *
 *  Rewritten based on code of Copyright (c) 2002 Michael Goffioul <kdeprint@swing.be>
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

#ifndef KATE_PRINTER_H
#define KATE_PRINTER_H

namespace KTextEditor
{
class DocumentPrivate;
class ViewPrivate;
}

namespace KatePrinter
{
    /**
     * Launches print dialog for specified @view
     * @returns true if document was successfully printed
     */
    bool print(KTextEditor::ViewPrivate *view);

    /**
     * Launches print preview dialog for specified @view
     * @returns true if document was printed
     */
    bool printPreview(KTextEditor::ViewPrivate *view);

    /**
     * Overloaded print function for document
     * Useful when there is no view for the document. Consequently this function
     * cannot print only selected portion of document.
     */
    bool print(KTextEditor::DocumentPrivate *doc);

    /**
     * Overloaded print function for document
     * Useful when there is no view for the document. Consequently this function
     * cannot print only selected portion of document.
     */
    bool printPreview(KTextEditor::DocumentPrivate *doc);
}

#endif
