/*  This file is part of the Kate project.
 *
 *  Copyright (C) 2011 Dominik Haumann <dhaumann@kde.org>
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

#ifndef KATE_MODIFIED_UNDO_H
#define KATE_MODIFIED_UNDO_H

#include "kateundo.h"

class KateModifiedInsertText : public KateEditInsertTextUndo
{
public:
    KateModifiedInsertText(KTextEditor::DocumentPrivate *document, int line, int col, const QString &text);

    /**
     * @copydoc KateUndo::undo()
     */
    void undo() override;

    /**
     * @copydoc KateUndo::redo()
     */
    void redo() override;

    void updateUndoSavedOnDiskFlag(QBitArray &lines) override;
    void updateRedoSavedOnDiskFlag(QBitArray &lines) override;
};

class KateModifiedRemoveText : public KateEditRemoveTextUndo
{
public:
    KateModifiedRemoveText(KTextEditor::DocumentPrivate *document, int line, int col, const QString &text);

    /**
     * @copydoc KateUndo::undo()
     */
    void undo() override;

    /**
     * @copydoc KateUndo::redo()
     */
    void redo() override;

    void updateUndoSavedOnDiskFlag(QBitArray &lines) override;
    void updateRedoSavedOnDiskFlag(QBitArray &lines) override;
};

class KateModifiedWrapLine : public KateEditWrapLineUndo
{
public:
    KateModifiedWrapLine(KTextEditor::DocumentPrivate *document, int line, int col, int len, bool newLine);

    /**
     * @copydoc KateUndo::undo()
     */
    void undo() override;

    /**
     * @copydoc KateUndo::redo()
     */
    void redo() override;

    void updateUndoSavedOnDiskFlag(QBitArray &lines) override;
    void updateRedoSavedOnDiskFlag(QBitArray &lines) override;
};

class KateModifiedUnWrapLine : public KateEditUnWrapLineUndo
{
public:
    KateModifiedUnWrapLine(KTextEditor::DocumentPrivate *document, int line, int col, int len, bool removeLine);

    /**
     * @copydoc KateUndo::undo()
     */
    void undo() override;

    /**
     * @copydoc KateUndo::redo()
     */
    void redo() override;

    void updateUndoSavedOnDiskFlag(QBitArray &lines) override;
    void updateRedoSavedOnDiskFlag(QBitArray &lines) override;
};

class KateModifiedInsertLine : public KateEditInsertLineUndo
{
public:
    KateModifiedInsertLine(KTextEditor::DocumentPrivate *document, int line, const QString &text);

    /**
     * @copydoc KateUndo::undo()
     */
    void undo() override;

    /**
     * @copydoc KateUndo::redo()
     */
    void redo() override;

    void updateRedoSavedOnDiskFlag(QBitArray &lines) override;
};

class KateModifiedRemoveLine : public KateEditRemoveLineUndo
{
public:
    KateModifiedRemoveLine(KTextEditor::DocumentPrivate *document, int line, const QString &text);

    /**
     * @copydoc KateUndo::undo()
     */
    void undo() override;

    /**
     * @copydoc KateUndo::redo()
     */
    void redo() override;

    void updateUndoSavedOnDiskFlag(QBitArray &lines) override;
};

#endif // KATE_MODIFIED_UNDO_H

