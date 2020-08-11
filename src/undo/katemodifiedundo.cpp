/*
    SPDX-FileCopyrightText: 2011 Dominik Haumann <dhaumann@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "katemodifiedundo.h"

#include "katedocument.h"
#include "kateundomanager.h"

#include <ktexteditor/cursor.h>
#include <ktexteditor/view.h>

KateModifiedInsertText::KateModifiedInsertText(KTextEditor::DocumentPrivate *document, int line, int col, const QString &text)
    : KateEditInsertTextUndo(document, line, col, text)
{
    setFlag(RedoLine1Modified);
    Kate::TextLine tl = document->plainKateTextLine(line);
    Q_ASSERT(tl);
    if (tl->markedAsModified()) {
        setFlag(UndoLine1Modified);
    } else {
        setFlag(UndoLine1Saved);
    }
}

KateModifiedRemoveText::KateModifiedRemoveText(KTextEditor::DocumentPrivate *document, int line, int col, const QString &text)
    : KateEditRemoveTextUndo(document, line, col, text)
{
    setFlag(RedoLine1Modified);
    Kate::TextLine tl = document->plainKateTextLine(line);
    Q_ASSERT(tl);
    if (tl->markedAsModified()) {
        setFlag(UndoLine1Modified);
    } else {
        setFlag(UndoLine1Saved);
    }
}

KateModifiedWrapLine::KateModifiedWrapLine(KTextEditor::DocumentPrivate *document, int line, int col, int len, bool newLine)
    : KateEditWrapLineUndo(document, line, col, len, newLine)
{
    Kate::TextLine tl = document->plainKateTextLine(line);
    Q_ASSERT(tl);
    if (len > 0 || tl->markedAsModified()) {
        setFlag(RedoLine1Modified);
    } else if (tl->markedAsSavedOnDisk()) {
        setFlag(RedoLine1Saved);
    }

    if (col > 0 || len == 0 || tl->markedAsModified()) {
        setFlag(RedoLine2Modified);
    } else if (tl->markedAsSavedOnDisk()) {
        setFlag(RedoLine2Saved);
    }

    if (tl->markedAsModified()) {
        setFlag(UndoLine1Modified);
    } else if ((len > 0 && col > 0) || tl->markedAsSavedOnDisk()) {
        setFlag(UndoLine1Saved);
    }
}

KateModifiedUnWrapLine::KateModifiedUnWrapLine(KTextEditor::DocumentPrivate *document, int line, int col, int len, bool removeLine)
    : KateEditUnWrapLineUndo(document, line, col, len, removeLine)
{
    Kate::TextLine tl = document->plainKateTextLine(line);
    Kate::TextLine nextLine = document->plainKateTextLine(line + 1);
    Q_ASSERT(tl);
    Q_ASSERT(nextLine);

    const int len1 = tl->length();
    const int len2 = nextLine->length();

    if (len1 > 0 && len2 > 0) {
        setFlag(RedoLine1Modified);

        if (tl->markedAsModified()) {
            setFlag(UndoLine1Modified);
        } else {
            setFlag(UndoLine1Saved);
        }

        if (nextLine->markedAsModified()) {
            setFlag(UndoLine2Modified);
        } else {
            setFlag(UndoLine2Saved);
        }
    } else if (len1 == 0) {
        if (nextLine->markedAsModified()) {
            setFlag(RedoLine1Modified);
        } else if (nextLine->markedAsSavedOnDisk()) {
            setFlag(RedoLine1Saved);
        }

        if (tl->markedAsModified()) {
            setFlag(UndoLine1Modified);
        } else {
            setFlag(UndoLine1Saved);
        }

        if (nextLine->markedAsModified()) {
            setFlag(UndoLine2Modified);
        } else if (nextLine->markedAsSavedOnDisk()) {
            setFlag(UndoLine2Saved);
        }
    } else { // len2 == 0
        if (nextLine->markedAsModified()) {
            setFlag(RedoLine1Modified);
        } else if (nextLine->markedAsSavedOnDisk()) {
            setFlag(RedoLine1Saved);
        }

        if (tl->markedAsModified()) {
            setFlag(UndoLine1Modified);
        } else if (tl->markedAsSavedOnDisk()) {
            setFlag(UndoLine1Saved);
        }

        if (nextLine->markedAsModified()) {
            setFlag(UndoLine2Modified);
        } else {
            setFlag(UndoLine2Saved);
        }
    }
}

KateModifiedInsertLine::KateModifiedInsertLine(KTextEditor::DocumentPrivate *document, int line, const QString &text)
    : KateEditInsertLineUndo(document, line, text)
{
    setFlag(RedoLine1Modified);
}

KateModifiedRemoveLine::KateModifiedRemoveLine(KTextEditor::DocumentPrivate *document, int line, const QString &text)
    : KateEditRemoveLineUndo(document, line, text)
{
    Kate::TextLine tl = document->plainKateTextLine(line);
    Q_ASSERT(tl);
    if (tl->markedAsModified()) {
        setFlag(UndoLine1Modified);
    } else {
        setFlag(UndoLine1Saved);
    }
}

void KateModifiedInsertText::undo()
{
    KateEditInsertTextUndo::undo();

    KTextEditor::DocumentPrivate *doc = document();
    Kate::TextLine tl = doc->plainKateTextLine(line());
    Q_ASSERT(tl);

    tl->markAsModified(isFlagSet(UndoLine1Modified));
    tl->markAsSavedOnDisk(isFlagSet(UndoLine1Saved));
}

void KateModifiedRemoveText::undo()
{
    KateEditRemoveTextUndo::undo();

    KTextEditor::DocumentPrivate *doc = document();
    Kate::TextLine tl = doc->plainKateTextLine(line());
    Q_ASSERT(tl);

    tl->markAsModified(isFlagSet(UndoLine1Modified));
    tl->markAsSavedOnDisk(isFlagSet(UndoLine1Saved));
}

void KateModifiedWrapLine::undo()
{
    KateEditWrapLineUndo::undo();

    KTextEditor::DocumentPrivate *doc = document();
    Kate::TextLine tl = doc->plainKateTextLine(line());
    Q_ASSERT(tl);

    tl->markAsModified(isFlagSet(UndoLine1Modified));
    tl->markAsSavedOnDisk(isFlagSet(UndoLine1Saved));
}

void KateModifiedUnWrapLine::undo()
{
    KateEditUnWrapLineUndo::undo();

    KTextEditor::DocumentPrivate *doc = document();
    Kate::TextLine tl = doc->plainKateTextLine(line());
    Q_ASSERT(tl);

    tl->markAsModified(isFlagSet(UndoLine1Modified));
    tl->markAsSavedOnDisk(isFlagSet(UndoLine1Saved));

    Kate::TextLine nextLine = doc->plainKateTextLine(line() + 1);
    Q_ASSERT(nextLine);
    nextLine->markAsModified(isFlagSet(UndoLine2Modified));
    nextLine->markAsSavedOnDisk(isFlagSet(UndoLine2Saved));
}

void KateModifiedInsertLine::undo()
{
    KateEditInsertLineUndo::undo();

    // no line modification needed, since the line is removed
}

void KateModifiedRemoveLine::undo()
{
    KateEditRemoveLineUndo::undo();

    KTextEditor::DocumentPrivate *doc = document();
    Kate::TextLine tl = doc->plainKateTextLine(line());
    Q_ASSERT(tl);

    tl->markAsModified(isFlagSet(UndoLine1Modified));
    tl->markAsSavedOnDisk(isFlagSet(UndoLine1Saved));
}

void KateModifiedRemoveText::redo()
{
    KateEditRemoveTextUndo::redo();

    KTextEditor::DocumentPrivate *doc = document();
    Kate::TextLine tl = doc->plainKateTextLine(line());
    Q_ASSERT(tl);

    tl->markAsModified(isFlagSet(RedoLine1Modified));
    tl->markAsSavedOnDisk(isFlagSet(RedoLine1Saved));
}

void KateModifiedInsertText::redo()
{
    KateEditInsertTextUndo::redo();

    KTextEditor::DocumentPrivate *doc = document();
    Kate::TextLine tl = doc->plainKateTextLine(line());
    Q_ASSERT(tl);

    tl->markAsModified(isFlagSet(RedoLine1Modified));
    tl->markAsSavedOnDisk(isFlagSet(RedoLine1Saved));
}

void KateModifiedUnWrapLine::redo()
{
    KateEditUnWrapLineUndo::redo();

    KTextEditor::DocumentPrivate *doc = document();
    Kate::TextLine tl = doc->plainKateTextLine(line());
    Q_ASSERT(tl);

    tl->markAsModified(isFlagSet(RedoLine1Modified));
    tl->markAsSavedOnDisk(isFlagSet(RedoLine1Saved));
}

void KateModifiedWrapLine::redo()
{
    KateEditWrapLineUndo::redo();

    KTextEditor::DocumentPrivate *doc = document();
    Kate::TextLine tl = doc->plainKateTextLine(line());
    Q_ASSERT(tl);

    tl->markAsModified(isFlagSet(RedoLine1Modified));
    tl->markAsSavedOnDisk(isFlagSet(RedoLine1Saved));

    Kate::TextLine nextLine = doc->plainKateTextLine(line() + 1);
    Q_ASSERT(nextLine);

    nextLine->markAsModified(isFlagSet(RedoLine2Modified));
    nextLine->markAsSavedOnDisk(isFlagSet(RedoLine2Saved));
}

void KateModifiedRemoveLine::redo()
{
    KateEditRemoveLineUndo::redo();

    // no line modification needed, since the line is removed
}

void KateModifiedInsertLine::redo()
{
    KateEditInsertLineUndo::redo();

    KTextEditor::DocumentPrivate *doc = document();
    Kate::TextLine tl = doc->plainKateTextLine(line());
    Q_ASSERT(tl);

    tl->markAsModified(isFlagSet(RedoLine1Modified));
    tl->markAsSavedOnDisk(isFlagSet(RedoLine1Saved));
}

void KateModifiedInsertText::updateRedoSavedOnDiskFlag(QBitArray &lines)
{
    if (line() >= lines.size()) {
        lines.resize(line() + 1);
    }

    if (!lines.testBit(line())) {
        lines.setBit(line());

        unsetFlag(RedoLine1Modified);
        setFlag(RedoLine1Saved);
    }
}

void KateModifiedInsertText::updateUndoSavedOnDiskFlag(QBitArray &lines)
{
    if (line() >= lines.size()) {
        lines.resize(line() + 1);
    }

    if (!lines.testBit(line())) {
        lines.setBit(line());

        unsetFlag(UndoLine1Modified);
        setFlag(UndoLine1Saved);
    }
}

void KateModifiedRemoveText::updateRedoSavedOnDiskFlag(QBitArray &lines)
{
    if (line() >= lines.size()) {
        lines.resize(line() + 1);
    }

    if (!lines.testBit(line())) {
        lines.setBit(line());

        unsetFlag(RedoLine1Modified);
        setFlag(RedoLine1Saved);
    }
}

void KateModifiedRemoveText::updateUndoSavedOnDiskFlag(QBitArray &lines)
{
    if (line() >= lines.size()) {
        lines.resize(line() + 1);
    }

    if (!lines.testBit(line())) {
        lines.setBit(line());

        unsetFlag(UndoLine1Modified);
        setFlag(UndoLine1Saved);
    }
}

void KateModifiedWrapLine::updateRedoSavedOnDiskFlag(QBitArray &lines)
{
    if (line() + 1 >= lines.size()) {
        lines.resize(line() + 2);
    }

    if (isFlagSet(RedoLine1Modified) && !lines.testBit(line())) {
        lines.setBit(line());

        unsetFlag(RedoLine1Modified);
        setFlag(RedoLine1Saved);
    }

    if (isFlagSet(RedoLine2Modified) && !lines.testBit(line() + 1)) {
        lines.setBit(line() + 1);

        unsetFlag(RedoLine2Modified);
        setFlag(RedoLine2Saved);
    }
}

void KateModifiedWrapLine::updateUndoSavedOnDiskFlag(QBitArray &lines)
{
    if (line() >= lines.size()) {
        lines.resize(line() + 1);
    }

    if (isFlagSet(UndoLine1Modified) && !lines.testBit(line())) {
        lines.setBit(line());

        unsetFlag(UndoLine1Modified);
        setFlag(UndoLine1Saved);
    }
}

void KateModifiedUnWrapLine::updateRedoSavedOnDiskFlag(QBitArray &lines)
{
    if (line() >= lines.size()) {
        lines.resize(line() + 1);
    }

    if (isFlagSet(RedoLine1Modified) && !lines.testBit(line())) {
        lines.setBit(line());

        unsetFlag(RedoLine1Modified);
        setFlag(RedoLine1Saved);
    }
}

void KateModifiedUnWrapLine::updateUndoSavedOnDiskFlag(QBitArray &lines)
{
    if (line() + 1 >= lines.size()) {
        lines.resize(line() + 2);
    }

    if (isFlagSet(UndoLine1Modified) && !lines.testBit(line())) {
        lines.setBit(line());

        unsetFlag(UndoLine1Modified);
        setFlag(UndoLine1Saved);
    }

    if (isFlagSet(UndoLine2Modified) && !lines.testBit(line() + 1)) {
        lines.setBit(line() + 1);

        unsetFlag(UndoLine2Modified);
        setFlag(UndoLine2Saved);
    }
}

void KateModifiedInsertLine::updateRedoSavedOnDiskFlag(QBitArray &lines)
{
    if (line() >= lines.size()) {
        lines.resize(line() + 1);
    }

    if (!lines.testBit(line())) {
        lines.setBit(line());

        unsetFlag(RedoLine1Modified);
        setFlag(RedoLine1Saved);
    }
}

void KateModifiedRemoveLine::updateUndoSavedOnDiskFlag(QBitArray &lines)
{
    if (line() >= lines.size()) {
        lines.resize(line() + 1);
    }

    if (!lines.testBit(line())) {
        lines.setBit(line());

        unsetFlag(UndoLine1Modified);
        setFlag(UndoLine1Saved);
    }
}
