/*
    SPDX-FileCopyrightText: 2009-2010 Bernhard Beschow <bbeschow@cs.tu-berlin.de>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "kateundomanager.h"

#include <ktexteditor/view.h>

#include "katedocument.h"
#include "katemodifiedundo.h"
#include "katepartdebug.h"
#include "kateview.h"

#include <QBitArray>

KateUndoManager::KateUndoManager(KTextEditor::DocumentPrivate *doc)
    : QObject(doc)
    , m_document(doc)
{
    connect(this, &KateUndoManager::undoEnd, this, &KateUndoManager::undoChanged);
    connect(this, &KateUndoManager::redoEnd, this, &KateUndoManager::undoChanged);

    connect(doc, &KTextEditor::DocumentPrivate::viewCreated, this, &KateUndoManager::viewCreated);

    // Before reload save history
    connect(doc, &KTextEditor::DocumentPrivate::aboutToReload, this, [this] {
        savedUndoItems = undoItems;
        savedRedoItems = redoItems;
        undoItems.clear();
        redoItems.clear();
        docChecksumBeforeReload = m_document->checksum();
    });

    // After reload restore it only if checksum of the doc is same
    connect(doc, &KTextEditor::DocumentPrivate::loaded, this, [this](KTextEditor::Document *doc) {
        if (doc && !doc->checksum().isEmpty() && !docChecksumBeforeReload.isEmpty() && doc->checksum() == docChecksumBeforeReload) {
            undoItems = savedUndoItems;
            redoItems = savedRedoItems;
            Q_EMIT undoChanged();
        } else {
            // Else delete everything, we don't want to leak
            qDeleteAll(savedUndoItems);
            qDeleteAll(savedRedoItems);
        }
        docChecksumBeforeReload.clear();
        savedUndoItems.clear();
        savedRedoItems.clear();
    });
}

KateUndoManager::~KateUndoManager()
{
    delete m_editCurrentUndo;

    // cleanup the undo/redo items, very important, truee :/
    qDeleteAll(undoItems);
    undoItems.clear();
    qDeleteAll(redoItems);
    redoItems.clear();
}

KTextEditor::Document *KateUndoManager::document()
{
    return m_document;
}

void KateUndoManager::viewCreated(KTextEditor::Document *, KTextEditor::View *newView) const
{
    connect(newView, &KTextEditor::View::cursorPositionChanged, this, &KateUndoManager::undoCancel);
}

void KateUndoManager::editStart()
{
    if (!m_isActive) {
        return;
    }

    // editStart() and editEnd() must be called in alternating fashion
    Q_ASSERT(m_editCurrentUndo == nullptr); // make sure to enter a clean state

    const KTextEditor::Cursor cursorPosition = activeView() ? activeView()->cursorPosition() : KTextEditor::Cursor::invalid();
    const KTextEditor::Range primarySelectionRange = activeView() ? activeView()->selectionRange() : KTextEditor::Range::invalid();
    QVector<KTextEditor::ViewPrivate::PlainSecondaryCursor> secondaryCursors;
    if (activeView()) {
        secondaryCursors = activeView()->plainSecondaryCursors();
    }

    // new current undo item
    m_editCurrentUndo = new KateUndoGroup(this, cursorPosition, primarySelectionRange, secondaryCursors);

    Q_ASSERT(m_editCurrentUndo != nullptr); // a new undo group must be created by this method
}

void KateUndoManager::editEnd()
{
    if (!m_isActive) {
        return;
    }

    // editStart() and editEnd() must be called in alternating fashion
    Q_ASSERT(m_editCurrentUndo != nullptr); // an undo group must have been created by editStart()

    const KTextEditor::Cursor cursorPosition = activeView() ? activeView()->cursorPosition() : KTextEditor::Cursor::invalid();
    const KTextEditor::Range selectionRange = activeView() ? activeView()->selectionRange() : KTextEditor::Range::invalid();

    QVector<KTextEditor::ViewPrivate::PlainSecondaryCursor> secondaryCursors;
    if (activeView()) {
        secondaryCursors = activeView()->plainSecondaryCursors();
    }

    m_editCurrentUndo->editEnd(cursorPosition, selectionRange, secondaryCursors);

    bool changedUndo = false;

    if (m_editCurrentUndo->isEmpty()) {
        delete m_editCurrentUndo;
    } else if (!undoItems.isEmpty() && undoItems.last()->merge(m_editCurrentUndo, m_undoComplexMerge)) {
        delete m_editCurrentUndo;
    } else {
        undoItems.append(m_editCurrentUndo);
        changedUndo = true;
    }

    m_editCurrentUndo = nullptr;

    if (changedUndo) {
        Q_EMIT undoChanged();
    }

    Q_ASSERT(m_editCurrentUndo == nullptr); // must be 0 after calling this method
}

void KateUndoManager::inputMethodStart()
{
    setActive(false);
    m_document->editStart();
}

void KateUndoManager::inputMethodEnd()
{
    m_document->editEnd();
    setActive(true);
}

void KateUndoManager::startUndo()
{
    setActive(false);
    m_document->editStart();
}

void KateUndoManager::endUndo()
{
    m_document->editEnd();
    setActive(true);
}

void KateUndoManager::slotTextInserted(int line, int col, const QString &s)
{
    if (m_editCurrentUndo != nullptr) { // do we care about notifications?
        addUndoItem(new KateModifiedInsertText(m_document, line, col, s));
    }
}

void KateUndoManager::slotTextRemoved(int line, int col, const QString &s)
{
    if (m_editCurrentUndo != nullptr) { // do we care about notifications?
        addUndoItem(new KateModifiedRemoveText(m_document, line, col, s));
    }
}

void KateUndoManager::slotMarkLineAutoWrapped(int line, bool autowrapped)
{
    if (m_editCurrentUndo != nullptr) { // do we care about notifications?
        addUndoItem(new KateEditMarkLineAutoWrappedUndo(m_document, line, autowrapped));
    }
}

void KateUndoManager::slotLineWrapped(int line, int col, int length, bool newLine)
{
    if (m_editCurrentUndo != nullptr) { // do we care about notifications?
        addUndoItem(new KateModifiedWrapLine(m_document, line, col, length, newLine));
    }
}

void KateUndoManager::slotLineUnWrapped(int line, int col, int length, bool lineRemoved)
{
    if (m_editCurrentUndo != nullptr) { // do we care about notifications?
        addUndoItem(new KateModifiedUnWrapLine(m_document, line, col, length, lineRemoved));
    }
}

void KateUndoManager::slotLineInserted(int line, const QString &s)
{
    if (m_editCurrentUndo != nullptr) { // do we care about notifications?
        addUndoItem(new KateModifiedInsertLine(m_document, line, s));
    }
}

void KateUndoManager::slotLineRemoved(int line, const QString &s)
{
    if (m_editCurrentUndo != nullptr) { // do we care about notifications?
        addUndoItem(new KateModifiedRemoveLine(m_document, line, s));
    }
}

void KateUndoManager::undoCancel()
{
    // Don't worry about this when an edit is in progress
    if (m_document->isEditRunning()) {
        return;
    }

    undoSafePoint();
}

void KateUndoManager::undoSafePoint()
{
    KateUndoGroup *undoGroup = m_editCurrentUndo;

    if (undoGroup == nullptr && !undoItems.isEmpty()) {
        undoGroup = undoItems.last();
    }

    if (undoGroup == nullptr) {
        return;
    }

    undoGroup->safePoint();
}

void KateUndoManager::addUndoItem(KateUndo *undo)
{
    Q_ASSERT(undo != nullptr); // don't add null pointers to our history
    Q_ASSERT(m_editCurrentUndo != nullptr); // make sure there is an undo group for our item

    m_editCurrentUndo->addItem(undo);

    // Clear redo buffer
    qDeleteAll(redoItems);
    redoItems.clear();
}

void KateUndoManager::setActive(bool enabled)
{
    Q_ASSERT(m_editCurrentUndo == nullptr); // must not already be in edit mode
    Q_ASSERT(m_isActive != enabled);

    m_isActive = enabled;

    Q_EMIT isActiveChanged(enabled);
}

uint KateUndoManager::undoCount() const
{
    return undoItems.count();
}

uint KateUndoManager::redoCount() const
{
    return redoItems.count();
}

void KateUndoManager::undo()
{
    Q_ASSERT(m_editCurrentUndo == nullptr); // undo is not supported while we care about notifications (call editEnd() first)

    if (!undoItems.isEmpty()) {
        Q_EMIT undoStart(document());

        undoItems.last()->undo(activeView());
        redoItems.append(undoItems.last());
        undoItems.removeLast();
        updateModified();

        Q_EMIT undoEnd(document());
    }
}

void KateUndoManager::redo()
{
    Q_ASSERT(m_editCurrentUndo == nullptr); // redo is not supported while we care about notifications (call editEnd() first)

    if (!redoItems.isEmpty()) {
        Q_EMIT redoStart(document());

        redoItems.last()->redo(activeView());
        undoItems.append(redoItems.last());
        redoItems.removeLast();
        updateModified();

        Q_EMIT redoEnd(document());
    }
}

void KateUndoManager::updateModified()
{
    /*
    How this works:

      After noticing that there where to many scenarios to take into
      consideration when using 'if's to toggle the "Modified" flag
      I came up with this baby, flexible and repetitive calls are
      minimal.

      A numeric unique pattern is generated by toggling a set of bits,
      each bit symbolizes a different state in the Undo Redo structure.

        undoItems.isEmpty() != null          BIT 1
        redoItems.isEmpty() != null          BIT 2
        docWasSavedWhenUndoWasEmpty == true  BIT 3
        docWasSavedWhenRedoWasEmpty == true  BIT 4
        lastUndoGroupWhenSavedIsLastUndo     BIT 5
        lastUndoGroupWhenSavedIsLastRedo     BIT 6
        lastRedoGroupWhenSavedIsLastUndo     BIT 7
        lastRedoGroupWhenSavedIsLastRedo     BIT 8

      If you find a new pattern, please add it to the patterns array
    */

    unsigned char currentPattern = 0;
    const unsigned char patterns[] = {5, 16, 21, 24, 26, 88, 90, 93, 133, 144, 149, 154, 165};
    const unsigned char patternCount = sizeof(patterns);
    KateUndoGroup *undoLast = nullptr;
    KateUndoGroup *redoLast = nullptr;

    if (undoItems.isEmpty()) {
        currentPattern |= 1;
    } else {
        undoLast = undoItems.last();
    }

    if (redoItems.isEmpty()) {
        currentPattern |= 2;
    } else {
        redoLast = redoItems.last();
    }

    if (docWasSavedWhenUndoWasEmpty) {
        currentPattern |= 4;
    }
    if (docWasSavedWhenRedoWasEmpty) {
        currentPattern |= 8;
    }
    if (lastUndoGroupWhenSaved == undoLast) {
        currentPattern |= 16;
    }
    if (lastUndoGroupWhenSaved == redoLast) {
        currentPattern |= 32;
    }
    if (lastRedoGroupWhenSaved == undoLast) {
        currentPattern |= 64;
    }
    if (lastRedoGroupWhenSaved == redoLast) {
        currentPattern |= 128;
    }

    // This will print out the pattern information

    qCDebug(LOG_KTE) << "Pattern:" << static_cast<unsigned int>(currentPattern);

    for (uint patternIndex = 0; patternIndex < patternCount; ++patternIndex) {
        if (currentPattern == patterns[patternIndex]) {
            // Note: m_document->setModified() calls KateUndoManager::setModified!
            m_document->setModified(false);
            // (dominik) whenever the doc is not modified, succeeding edits
            // should not be merged
            undoSafePoint();
            qCDebug(LOG_KTE) << "setting modified to false!";
            break;
        }
    }
}

void KateUndoManager::clearUndo()
{
    qDeleteAll(undoItems);
    undoItems.clear();

    lastUndoGroupWhenSaved = nullptr;
    docWasSavedWhenUndoWasEmpty = false;

    Q_EMIT undoChanged();
}

void KateUndoManager::clearRedo()
{
    qDeleteAll(redoItems);
    redoItems.clear();

    lastRedoGroupWhenSaved = nullptr;
    docWasSavedWhenRedoWasEmpty = false;

    Q_EMIT undoChanged();
}

void KateUndoManager::setModified(bool modified)
{
    if (!modified) {
        if (!undoItems.isEmpty()) {
            lastUndoGroupWhenSaved = undoItems.last();
        }

        if (!redoItems.isEmpty()) {
            lastRedoGroupWhenSaved = redoItems.last();
        }

        docWasSavedWhenUndoWasEmpty = undoItems.isEmpty();
        docWasSavedWhenRedoWasEmpty = redoItems.isEmpty();
    }
}

void KateUndoManager::updateLineModifications()
{
    // change LineSaved flag of all undo & redo items to LineModified
    for (KateUndoGroup *undoGroup : std::as_const(undoItems)) {
        undoGroup->flagSavedAsModified();
    }

    for (KateUndoGroup *undoGroup : std::as_const(redoItems)) {
        undoGroup->flagSavedAsModified();
    }

    // iterate all undo/redo items to find out, which item sets the flag LineSaved
    QBitArray lines(document()->lines(), false);
    for (int i = undoItems.size() - 1; i >= 0; --i) {
        undoItems[i]->markRedoAsSaved(lines);
    }

    lines.fill(false);
    for (int i = redoItems.size() - 1; i >= 0; --i) {
        redoItems[i]->markUndoAsSaved(lines);
    }
}

void KateUndoManager::setUndoRedoCursorsOfLastGroup(const KTextEditor::Cursor undoCursor, const KTextEditor::Cursor redoCursor)
{
    Q_ASSERT(m_editCurrentUndo == nullptr);
    if (!undoItems.isEmpty()) {
        KateUndoGroup *last = undoItems.last();
        last->setUndoCursor(undoCursor);
        last->setRedoCursor(redoCursor);
    }
}

KTextEditor::Cursor KateUndoManager::lastRedoCursor() const
{
    Q_ASSERT(m_editCurrentUndo == nullptr);
    if (!undoItems.isEmpty()) {
        KateUndoGroup *last = undoItems.last();
        return last->redoCursor();
    }
    return KTextEditor::Cursor::invalid();
}

void KateUndoManager::updateConfig()
{
    Q_EMIT undoChanged();
}

void KateUndoManager::setAllowComplexMerge(bool allow)
{
    m_undoComplexMerge = allow;
}

KTextEditor::ViewPrivate *KateUndoManager::activeView()
{
    return static_cast<KTextEditor::ViewPrivate *>(m_document->activeView());
}

#include "moc_kateundomanager.cpp"
