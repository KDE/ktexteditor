/*
    SPDX-FileCopyrightText: 2002-2010 Anders Lund <anders@alweb.dk>

    Rewritten based on code of:
    SPDX-FileCopyrightText: 2002 Michael Goffioul <kdeprint@swing.be>

    SPDX-License-Identifier: LGPL-2.0-or-later
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
 * Launches print dialog for specified @doc and optional @view
 * @returns true if document was successfully printed
 */
bool print(KTextEditor::DocumentPrivate *doc, KTextEditor::ViewPrivate *view = nullptr);

/**
 * Launches print preview dialog for specified @doc and optional @view
 * @returns true if document was printed
 */
bool printPreview(KTextEditor::DocumentPrivate *doc, KTextEditor::ViewPrivate *view = nullptr);
}

#endif
