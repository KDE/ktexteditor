/*
    This file is part of the KDE libraries
    SPDX-FileCopyrightText: 2010 Bernhard Beschow <bbeschow@cs.tu-berlin.de>
    SPDX-FileCopyrightText: 2009-2018 Dominik Haumann <dhaumann@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "undomanager_test.h"

#include <katedocument.h>
#include <kateglobal.h>
#include <kateundomanager.h>
#include <kateview.h>

#include <QtTestWidgets>

QTEST_MAIN(UndoManagerTest)

using namespace KTextEditor;

UndoManagerTest::UndoManagerTest()
    : QObject()
{
    KTextEditor::EditorPrivate::enableUnitTestMode();
}

void UndoManagerTest::testUndoRedoCount()
{
    KTextEditor::DocumentPrivate doc;
    KateUndoManager *undoManager = doc.undoManager();

    // no undo/redo items at the beginning
    QCOMPARE(undoManager->undoCount(), 0u);
    QCOMPARE(undoManager->redoCount(), 0u);

    doc.insertText(Cursor(0, 0), QLatin1String("a"));

    // create one insert-group
    QCOMPARE(undoManager->undoCount(), 1u);
    QCOMPARE(undoManager->redoCount(), 0u);

    doc.undo();

    // move insert-group to redo items
    QCOMPARE(undoManager->undoCount(), 0u);
    QCOMPARE(undoManager->redoCount(), 1u);

    doc.redo();

    // move insert-group back to undo items
    QCOMPARE(undoManager->undoCount(), 1u);
    QCOMPARE(undoManager->redoCount(), 0u);

    doc.insertText(Cursor(0, 1), QLatin1String("b"));

    // merge "b" into insert-group
    QCOMPARE(undoManager->undoCount(), 1u);
    QCOMPARE(undoManager->redoCount(), 0u);

    doc.removeText(Range(0, 1, 0, 2));

    // create an additional remove-group
    QCOMPARE(undoManager->undoCount(), 2u);
    QCOMPARE(undoManager->redoCount(), 0u);

    doc.undo();

    // move remove-group to redo items
    QCOMPARE(undoManager->undoCount(), 1u);
    QCOMPARE(undoManager->redoCount(), 1u);

    doc.insertText(Cursor(0, 1), QLatin1String("b"));

    // merge "b" into insert-group
    // and remove remove-group
    QCOMPARE(undoManager->undoCount(), 1u);
    QCOMPARE(undoManager->redoCount(), 0u);
}

void UndoManagerTest::testSafePoint()
{
    KTextEditor::DocumentPrivate doc;
    KateUndoManager *undoManager = doc.undoManager();

    doc.insertText(Cursor(0, 0), QLatin1String("a"));

    // create one undo group
    QCOMPARE(undoManager->undoCount(), 1u);
    QCOMPARE(undoManager->redoCount(), 0u);

    undoManager->undoSafePoint();
    doc.insertText(Cursor(0, 1), QLatin1String("b"));

    // create a second undo group (don't merge)
    QCOMPARE(undoManager->undoCount(), 2u);

    doc.undo();

    // move second undo group to redo items
    QCOMPARE(undoManager->undoCount(), 1u);
    QCOMPARE(undoManager->redoCount(), 1u);

    doc.insertText(Cursor(0, 1), QLatin1String("b"));

    // create a second undo group again, (don't merge)
    QCOMPARE(undoManager->undoCount(), 2u);
    QCOMPARE(undoManager->redoCount(), 0u);

    doc.editStart();
    doc.insertText(Cursor(0, 2), QLatin1String("c"));
    undoManager->undoSafePoint();
    doc.insertText(Cursor(0, 3), QLatin1String("d"));
    doc.editEnd();

    // merge both edits into second undo group
    QCOMPARE(undoManager->undoCount(), 2u);
    QCOMPARE(undoManager->redoCount(), 0u);

    doc.insertText(Cursor(0, 4), QLatin1String("e"));

    // create a third undo group (don't merge)
    QCOMPARE(undoManager->undoCount(), 3u);
    QCOMPARE(undoManager->redoCount(), 0u);
}

void UndoManagerTest::testCursorPosition()
{
    KTextEditor::DocumentPrivate doc;
    KTextEditor::ViewPrivate *view = static_cast<KTextEditor::ViewPrivate *>(doc.createView(nullptr));

    doc.setText(
        QLatin1String("aaaa bbbb cccc\n"
                      "dddd  ffff"));
    view->setCursorPosition(KTextEditor::Cursor(1, 5));

    doc.typeChars(view, QLatin1String("eeee"));

    // cursor position: "dddd eeee| ffff"
    QCOMPARE(view->cursorPosition(), KTextEditor::Cursor(1, 9));

    // undo once to remove "eeee", cursor position: "dddd | ffff"
    doc.undo();
    QCOMPARE(view->cursorPosition(), KTextEditor::Cursor(1, 5));

    // redo once to insert "eeee" again. cursor position: "dddd eeee| ffff"
    doc.redo();
    QCOMPARE(view->cursorPosition(), KTextEditor::Cursor(1, 9));

    delete view;
}

void UndoManagerTest::testSelectionUndo()
{
    KTextEditor::DocumentPrivate doc;
    KTextEditor::ViewPrivate *view = static_cast<KTextEditor::ViewPrivate *>(doc.createView(nullptr));

    doc.setText(
        QLatin1String("aaaa bbbb cccc\n"
                      "dddd eeee ffff"));
    view->setCursorPosition(KTextEditor::Cursor(1, 9));
    KTextEditor::Range selectionRange(KTextEditor::Cursor(0, 5), KTextEditor::Cursor(1, 9));
    view->setSelection(selectionRange);

    doc.typeChars(view, QLatin1String(QLatin1String("eeee")));

    // cursor position: "aaaa eeee| ffff", no selection anymore
    QCOMPARE(view->cursorPosition(), KTextEditor::Cursor(0, 9));
    QCOMPARE(view->selection(), false);

    // undo to remove "eeee" and add selection and text again
    doc.undo();
    QCOMPARE(view->cursorPosition(), KTextEditor::Cursor(1, 9));
    QCOMPARE(view->selection(), true);
    QCOMPARE(view->selectionRange(), selectionRange);

    // redo to insert "eeee" again and remove selection
    // cursor position: "aaaa eeee| ffff", no selection anymore
    doc.redo();
    QCOMPARE(view->cursorPosition(), KTextEditor::Cursor(0, 9));
    QCOMPARE(view->selection(), false);

    delete view;
}

void UndoManagerTest::testUndoWordWrapBug301367()
{
    KTextEditor::DocumentPrivate doc;
    doc.setWordWrap(true);
    doc.setWordWrapAt(20);
    KTextEditor::ViewPrivate *view = static_cast<KTextEditor::ViewPrivate *>(doc.createView(nullptr));

    QString text = QString::fromLatin1(
        "1234 1234 1234 1234\n"
        "1234 1234 1234 1234");

    doc.setText(text);
    view->setCursorPosition(KTextEditor::Cursor(0, 0));

    doc.typeChars(view, QLatin1String("           "));

    while (doc.undoCount() > 1) {
        doc.undo();
    }

    // test must be exactly the same as before
    QCOMPARE(doc.text(), text);

    while (doc.redoCount() > 1) {
        doc.redo();
    }

    while (doc.undoCount() > 1) {
        doc.undo();
    }

    // test must be exactly the same as before
    QCOMPARE(doc.text(), text);

    delete view;
}

void UndoManagerTest::testUndoIndentBug373009()
{
    KTextEditor::DocumentPrivate doc;
    KTextEditor::ViewPrivate *view = static_cast<KTextEditor::ViewPrivate *>(doc.createView(nullptr));

    doc.setMode("C");

    QString text = QString::fromLatin1(
        "    while (whatever) printf (\"please fix indentation.\\n\");\n"
        "    return 0;");
    doc.setText(text);

    // position cursor right before return
    view->setCursorPosition(KTextEditor::Cursor(1, 4));

    QCOMPARE(view->cursorPosition(), KTextEditor::Cursor(1, 4));
    QCOMPARE(doc.characterAt(view->cursorPosition()), 'r');

    view->keyReturn();

    QCOMPARE(doc.undoCount(), 2);

    // After indent we should be able to revert with
    // one undo operation
    doc.undo();
    QCOMPARE(doc.text(), text);

    delete view;
}

#include "moc_undomanager_test.cpp"
