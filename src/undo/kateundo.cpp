/*
    SPDX-FileCopyrightText: 2011 Dominik Haumann <dhaumann@kde.org>
    SPDX-FileCopyrightText: 2009-2010 Bernhard Beschow <bbeschow@cs.tu-berlin.de>
    SPDX-FileCopyrightText: 2002 John Firebaugh <jfirebaugh@kde.org>
    SPDX-FileCopyrightText: 2001 Christoph Cullmann <cullmann@kde.org>
    SPDX-FileCopyrightText: 2001 Joseph Wenninger <jowenn@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "kateundo.h"

#include "katedocument.h"
#include "kateundomanager.h"

#include <ktexteditor/cursor.h>
#include <ktexteditor/view.h>

KateUndo::KateUndo(KTextEditor::DocumentPrivate *document)
    : m_document(document)
{
}

KateEditInsertTextUndo::KateEditInsertTextUndo(KTextEditor::DocumentPrivate *document, int line, int col, const QString &text)
    : KateUndo(document)
    , m_line(line)
    , m_col(col)
    , m_text(text)
{
}

KateEditRemoveTextUndo::KateEditRemoveTextUndo(KTextEditor::DocumentPrivate *document, int line, int col, const QString &text)
    : KateUndo(document)
    , m_line(line)
    , m_col(col)
    , m_text(text)
{
}

KateEditWrapLineUndo::KateEditWrapLineUndo(KTextEditor::DocumentPrivate *document, int line, int col, int len, bool newLine)
    : KateUndo(document)
    , m_line(line)
    , m_col(col)
    , m_len(len)
    , m_newLine(newLine)
{
}

KateEditUnWrapLineUndo::KateEditUnWrapLineUndo(KTextEditor::DocumentPrivate *document, int line, int col, int len, bool removeLine)
    : KateUndo(document)
    , m_line(line)
    , m_col(col)
    , m_len(len)
    , m_removeLine(removeLine)
{
}

KateEditInsertLineUndo::KateEditInsertLineUndo(KTextEditor::DocumentPrivate *document, int line, const QString &text)
    : KateUndo(document)
    , m_line(line)
    , m_text(text)
{
}

KateEditRemoveLineUndo::KateEditRemoveLineUndo(KTextEditor::DocumentPrivate *document, int line, const QString &text)
    : KateUndo(document)
    , m_line(line)
    , m_text(text)
{
}

bool KateUndo::isEmpty() const
{
    return false;
}

bool KateEditInsertTextUndo::isEmpty() const
{
    return len() == 0;
}

bool KateEditRemoveTextUndo::isEmpty() const
{
    return len() == 0;
}

bool KateUndo::mergeWith(const KateUndo * /*undo*/)
{
    return false;
}

bool KateEditInsertTextUndo::mergeWith(const KateUndo *undo)
{
    // we can do a hard cast, we ensure we are only called with the same types on the outside
    Q_ASSERT(type() == undo->type());
    const KateEditInsertTextUndo *u = static_cast<const KateEditInsertTextUndo *>(undo);
    if (m_line == u->m_line && (m_col + len()) == u->m_col) {
        m_text += u->m_text;
        return true;
    }

    return false;
}

bool KateEditRemoveTextUndo::mergeWith(const KateUndo *undo)
{
    // we can do a hard cast, we ensure we are only called with the same types on the outside
    Q_ASSERT(type() == undo->type());
    const KateEditRemoveTextUndo *u = static_cast<const KateEditRemoveTextUndo *>(undo);
    if (m_line == u->m_line && m_col == (u->m_col + u->len())) {
        m_text.prepend(u->m_text);
        m_col = u->m_col;
        return true;
    }

    return false;
}

void KateEditInsertTextUndo::undo()
{
    KTextEditor::DocumentPrivate *doc = document();

    doc->editRemoveText(m_line, m_col, len());
}

void KateEditRemoveTextUndo::undo()
{
    KTextEditor::DocumentPrivate *doc = document();

    doc->editInsertText(m_line, m_col, m_text);
}

void KateEditWrapLineUndo::undo()
{
    KTextEditor::DocumentPrivate *doc = document();

    doc->editUnWrapLine(m_line, m_newLine, m_len);
}

void KateEditUnWrapLineUndo::undo()
{
    KTextEditor::DocumentPrivate *doc = document();

    doc->editWrapLine(m_line, m_col, m_removeLine);
}

void KateEditInsertLineUndo::undo()
{
    KTextEditor::DocumentPrivate *doc = document();

    doc->editRemoveLine(m_line);
}

void KateEditRemoveLineUndo::undo()
{
    KTextEditor::DocumentPrivate *doc = document();

    doc->editInsertLine(m_line, m_text);
}

void KateEditMarkLineAutoWrappedUndo::undo()
{
    KTextEditor::DocumentPrivate *doc = document();

    doc->editMarkLineAutoWrapped(m_line, m_autowrapped);
}

void KateEditRemoveTextUndo::redo()
{
    KTextEditor::DocumentPrivate *doc = document();

    doc->editRemoveText(m_line, m_col, len());
}

void KateEditInsertTextUndo::redo()
{
    KTextEditor::DocumentPrivate *doc = document();

    doc->editInsertText(m_line, m_col, m_text);
}

void KateEditUnWrapLineUndo::redo()
{
    KTextEditor::DocumentPrivate *doc = document();

    doc->editUnWrapLine(m_line, m_removeLine, m_len);
}

void KateEditWrapLineUndo::redo()
{
    KTextEditor::DocumentPrivate *doc = document();

    doc->editWrapLine(m_line, m_col, m_newLine);
}

void KateEditRemoveLineUndo::redo()
{
    KTextEditor::DocumentPrivate *doc = document();

    doc->editRemoveLine(m_line);
}

void KateEditInsertLineUndo::redo()
{
    KTextEditor::DocumentPrivate *doc = document();

    doc->editInsertLine(m_line, m_text);
}

void KateEditMarkLineAutoWrappedUndo::redo()
{
    KTextEditor::DocumentPrivate *doc = document();

    doc->editMarkLineAutoWrapped(m_line, m_autowrapped);
}

KateUndoGroup::KateUndoGroup(KateUndoManager *manager, const KTextEditor::Cursor &cursorPosition, const KTextEditor::Range &selectionRange)
    : m_manager(manager)
    , m_undoSelection(selectionRange)
    , m_redoSelection(-1, -1, -1, -1)
    , m_undoCursor(cursorPosition)
    , m_redoCursor(-1, -1)
{
}

KateUndoGroup::~KateUndoGroup()
{
    qDeleteAll(m_items);
}

void KateUndoGroup::undo(KTextEditor::View *view)
{
    if (m_items.isEmpty()) {
        return;
    }

    m_manager->startUndo();

    for (int i = m_items.size() - 1; i >= 0; --i) {
        m_items[i]->undo();
    }

    if (view != nullptr) {
        if (m_undoSelection.isValid()) {
            view->setSelection(m_undoSelection);
        } else {
            view->removeSelection();
        }

        if (m_undoCursor.isValid()) {
            view->setCursorPosition(m_undoCursor);
        }
    }

    m_manager->endUndo();
}

void KateUndoGroup::redo(KTextEditor::View *view)
{
    if (m_items.isEmpty()) {
        return;
    }

    m_manager->startUndo();

    for (int i = 0; i < m_items.size(); ++i) {
        m_items[i]->redo();
    }

    if (view != nullptr) {
        if (m_redoSelection.isValid()) {
            view->setSelection(m_redoSelection);
        } else {
            view->removeSelection();
        }

        if (m_redoCursor.isValid()) {
            view->setCursorPosition(m_redoCursor);
        }
    }

    m_manager->endUndo();
}

void KateUndoGroup::editEnd(const KTextEditor::Cursor &cursorPosition, const KTextEditor::Range &selectionRange)
{
    m_redoCursor = cursorPosition;
    m_redoSelection = selectionRange;
}

void KateUndoGroup::addItem(KateUndo *u)
{
    // kill empty items
    if (u->isEmpty()) {
        delete u;
        return;
    }

    // try to merge, do that only for equal types, inside mergeWith we do hard casts
    if (!m_items.isEmpty() && m_items.last()->type() == u->type() && m_items.last()->mergeWith(u)) {
        delete u;
        return;
    }

    // default: just add new item unchanged
    m_items.append(u);
}

bool KateUndoGroup::merge(KateUndoGroup *newGroup, bool complex)
{
    if (m_safePoint) {
        return false;
    }

    if (newGroup->isOnlyType(singleType()) || complex) {
        // Take all of its items first -> last
        KateUndo *u = newGroup->m_items.isEmpty() ? nullptr : newGroup->m_items.takeFirst();
        while (u) {
            addItem(u);
            u = newGroup->m_items.isEmpty() ? nullptr : newGroup->m_items.takeFirst();
        }

        if (newGroup->m_safePoint) {
            safePoint();
        }

        m_redoCursor = newGroup->m_redoCursor;
        m_redoSelection = newGroup->m_redoSelection;

        return true;
    }

    return false;
}

void KateUndoGroup::safePoint(bool safePoint)
{
    m_safePoint = safePoint;
}

void KateUndoGroup::flagSavedAsModified()
{
    for (KateUndo *item : std::as_const(m_items)) {
        if (item->isFlagSet(KateUndo::UndoLine1Saved)) {
            item->unsetFlag(KateUndo::UndoLine1Saved);
            item->setFlag(KateUndo::UndoLine1Modified);
        }

        if (item->isFlagSet(KateUndo::UndoLine2Saved)) {
            item->unsetFlag(KateUndo::UndoLine2Saved);
            item->setFlag(KateUndo::UndoLine2Modified);
        }

        if (item->isFlagSet(KateUndo::RedoLine1Saved)) {
            item->unsetFlag(KateUndo::RedoLine1Saved);
            item->setFlag(KateUndo::RedoLine1Modified);
        }

        if (item->isFlagSet(KateUndo::RedoLine2Saved)) {
            item->unsetFlag(KateUndo::RedoLine2Saved);
            item->setFlag(KateUndo::RedoLine2Modified);
        }
    }
}

void KateUndoGroup::markUndoAsSaved(QBitArray &lines)
{
    for (int i = m_items.size() - 1; i >= 0; --i) {
        KateUndo *item = m_items[i];
        item->updateUndoSavedOnDiskFlag(lines);
    }
}

void KateUndoGroup::markRedoAsSaved(QBitArray &lines)
{
    for (int i = m_items.size() - 1; i >= 0; --i) {
        KateUndo *item = m_items[i];
        item->updateRedoSavedOnDiskFlag(lines);
    }
}

KTextEditor::Document *KateUndoGroup::document()
{
    return m_manager->document();
}

KateUndo::UndoType KateUndoGroup::singleType() const
{
    KateUndo::UndoType ret = KateUndo::editInvalid;

    for (const KateUndo *item : m_items) {
        if (ret == KateUndo::editInvalid) {
            ret = item->type();
        } else if (ret != item->type()) {
            return KateUndo::editInvalid;
        }
    }

    return ret;
}

bool KateUndoGroup::isOnlyType(KateUndo::UndoType type) const
{
    if (type == KateUndo::editInvalid) {
        return false;
    }

    return std::all_of(m_items.begin(), m_items.end(), [type](const KateUndo *item) {
        return (item->type() == type);
    });
}
