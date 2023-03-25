/*
    SPDX-FileCopyrightText: 2011 Dominik Haumann <dhaumann@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef KATE_MODIFIED_UNDO_H
#define KATE_MODIFIED_UNDO_H

#include "kateundo.h"

class KateModifiedInsertText : public KateEditInsertTextUndo
{
public:
    KateModifiedInsertText(KTextEditor::DocumentPrivate *document, int line, int col, const QString &text);

    void undo(KTextEditor::DocumentPrivate *document) override;
    void redo(KTextEditor::DocumentPrivate *document) override;

    void updateUndoSavedOnDiskFlag(QBitArray &lines) override;
    void updateRedoSavedOnDiskFlag(QBitArray &lines) override;
};

class KateModifiedRemoveText : public KateEditRemoveTextUndo
{
public:
    KateModifiedRemoveText(KTextEditor::DocumentPrivate *document, int line, int col, const QString &text);

    void undo(KTextEditor::DocumentPrivate *document) override;
    void redo(KTextEditor::DocumentPrivate *document) override;

    void updateUndoSavedOnDiskFlag(QBitArray &lines) override;
    void updateRedoSavedOnDiskFlag(QBitArray &lines) override;
};

class KateModifiedWrapLine : public KateEditWrapLineUndo
{
public:
    KateModifiedWrapLine(KTextEditor::DocumentPrivate *document, int line, int col, int len, bool newLine);

    void undo(KTextEditor::DocumentPrivate *document) override;
    void redo(KTextEditor::DocumentPrivate *document) override;

    void updateUndoSavedOnDiskFlag(QBitArray &lines) override;
    void updateRedoSavedOnDiskFlag(QBitArray &lines) override;
};

class KateModifiedUnWrapLine : public KateEditUnWrapLineUndo
{
public:
    KateModifiedUnWrapLine(KTextEditor::DocumentPrivate *document, int line, int col, int len, bool removeLine);

    void undo(KTextEditor::DocumentPrivate *document) override;
    void redo(KTextEditor::DocumentPrivate *document) override;

    void updateUndoSavedOnDiskFlag(QBitArray &lines) override;
    void updateRedoSavedOnDiskFlag(QBitArray &lines) override;
};

class KateModifiedInsertLine : public KateEditInsertLineUndo
{
public:
    KateModifiedInsertLine(int line, const QString &text);

    void undo(KTextEditor::DocumentPrivate *document) override;

    /**
     * @copydoc KateUndo::redo()
     */
    void redo(KTextEditor::DocumentPrivate *document) override;

    void updateRedoSavedOnDiskFlag(QBitArray &lines) override;
};

class KateModifiedRemoveLine : public KateEditRemoveLineUndo
{
public:
    KateModifiedRemoveLine(KTextEditor::DocumentPrivate *document, int line, const QString &text);

    void undo(KTextEditor::DocumentPrivate *document) override;
    void redo(KTextEditor::DocumentPrivate *document) override;

    void updateUndoSavedOnDiskFlag(QBitArray &lines) override;
};

#endif // KATE_MODIFIED_UNDO_H
