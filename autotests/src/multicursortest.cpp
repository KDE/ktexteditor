/*
    This file is part of the KDE libraries
    SPDX-FileCopyrightText: 2022 Waqar Ahmed <waqar.17a@gmail.com>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "multicursortest.h"

#include <katebuffer.h>
#include <kateconfig.h>
#include <katedocument.h>
#include <kateglobal.h>
#include <kateundomanager.h>
#include <kateview.h>
#include <kateviewinternal.h>
#include <ktexteditor/message.h>
#include <ktexteditor/movingcursor.h>

#include <QTest>

using namespace KTextEditor;

#define CREATE_VIEW_AND_DOC(text, line, col)                                          \
    KTextEditor::DocumentPrivate doc(false, false);                                   \
    doc.setText(text);                                                                \
    KTextEditor::ViewPrivate *view = new KTextEditor::ViewPrivate(&doc, nullptr);     \
    view->setCursorPosition({line, col})

QTEST_MAIN(MulticursorTest)

MulticursorTest::MulticursorTest()
    : QObject()
{
    KTextEditor::EditorPrivate::enableUnitTestMode();
}

MulticursorTest::~MulticursorTest()
{
}

static QWidget *findViewInternal(KTextEditor::View *view)
{
    for (QObject *child : view->children()) {
        if (child->metaObject()->className() == QByteArrayLiteral("KateViewInternal")) {
            return qobject_cast<QWidget *>(child);
        }
    }
    return nullptr;
}

static void clickAtPosition(ViewPrivate *view, QObject *internalView, Cursor pos, Qt::KeyboardModifiers m)
{
    QPoint p = view->cursorToCoordinate(pos);
    auto me = QMouseEvent(QEvent::MouseButtonPress, p, Qt::LeftButton, Qt::LeftButton, m);
    QCoreApplication::sendEvent(internalView, &me);
}

void MulticursorTest::testKillline()
{
    KTextEditor::DocumentPrivate doc;
    doc.insertLines(0, {"foo", "bar", "baz"});
    KTextEditor::ViewPrivate *view = new KTextEditor::ViewPrivate(&doc, nullptr);
    view->setCursorPositionInternal(KTextEditor::Cursor(0, 0));
    view->addSecondaryCursorAt(KTextEditor::Cursor(1, 0));
    view->addSecondaryCursorAt(KTextEditor::Cursor(2, 0));

    view->killLine();

    QCOMPARE(doc.text(), QString());
}

void MulticursorTest::insertRemoveText()
{
    CREATE_VIEW_AND_DOC("foo\nbar\nfoo\n", 0, 0);
    QObject *internalView = findViewInternal(view);
    QVERIFY(internalView);

    { // Same line
        view->addSecondaryCursorAt({0, 1});
        view->addSecondaryCursorAt({0, 2});
        view->addSecondaryCursorAt({0, 3});
        QKeyEvent ke(QKeyEvent::KeyPress, Qt::Key_L, Qt::NoModifier, QStringLiteral("L"));
        QCoreApplication::sendEvent(internalView, &ke);

        QCOMPARE(doc.line(0), QStringLiteral("LfLoLoL"));

        // Removal
        view->backspace();
        QCOMPARE(doc.line(0), QStringLiteral("foo"));

        view->clearSecondaryCursors();
    }

    { // Different lines
        view->addSecondaryCursorAt({1, 0});
        view->addSecondaryCursorAt({2, 0});
        QKeyEvent ke(QKeyEvent::KeyPress, Qt::Key_L, Qt::NoModifier, QStringLiteral("L"));
        QCoreApplication::sendEvent(internalView, &ke);

        QCOMPARE(doc.line(0), QStringLiteral("Lfoo"));
        QCOMPARE(doc.line(1), QStringLiteral("Lbar"));
        QCOMPARE(doc.line(2), QStringLiteral("Lfoo"));

        view->backspace();

        QCOMPARE(doc.line(0), QStringLiteral("foo"));
        QCOMPARE(doc.line(1), QStringLiteral("bar"));
        QCOMPARE(doc.line(2), QStringLiteral("foo"));

        view->clearSecondaryCursors();
    }

    // Three empty lines
    doc.setText(QStringLiteral("\n\n\n"));
    view->setCursorPosition({0, 0});
    view->addSecondaryCursorAt({1, 0});
    view->addSecondaryCursorAt({2, 0});

    // cursors should merge
    view->backspace();
    QCOMPARE(view->secondaryMovingCursors().size(), 0);
    QCOMPARE(view->cursorPosition(), Cursor(0, 0));
}

void MulticursorTest::testUndoRedo()
{
    CREATE_VIEW_AND_DOC("foo\nfoo", 0, 3);

    // single cursor backspace
    view->backspace();
    QCOMPARE(doc.text(), QStringLiteral("fo\nfoo"));
    doc.undoManager()->undoSafePoint();

    // backspace with 2 cursors
    view->addSecondaryCursorAt({1, 3});
    view->backspace();
    QCOMPARE(doc.text(), QStringLiteral("f\nfo"));

    view->doc()->undo();
    QCOMPARE(doc.text(), QStringLiteral("fo\nfoo"));
    QCOMPARE(view->secondaryMovingCursors().size(), 1);
    QCOMPARE(*view->secondaryMovingCursors().at(0), Cursor(1, 3));

    // Another undo, multicursor should be gone
    view->doc()->undo();
    QCOMPARE(doc.text(), QStringLiteral("foo\nfoo"));
    QCOMPARE(view->secondaryMovingCursors().size(), 0);

    // One redo
    view->doc()->redo();
    QCOMPARE(doc.text(), QStringLiteral("fo\nfoo"));

    // Second redo, multicursor should be back
    view->doc()->redo();
    QCOMPARE(doc.text(), QStringLiteral("f\nfo"));
    QCOMPARE(view->secondaryMovingCursors().size(), 1);
    QCOMPARE(*view->secondaryMovingCursors().at(0), Cursor(1, 2));
}

void MulticursorTest::testUndoRedoWithSelection()
{
    CREATE_VIEW_AND_DOC("foo\nfoo", 0, 3);
    view->addSecondaryCursorAt({1, 3});

    // select a word & remove it
    view->shiftWordLeft();
    view->backspace();

    QCOMPARE(doc.text(), QStringLiteral("\n"));
    QCOMPARE(view->cursorPosition(), Cursor(0, 0));
    QCOMPARE(view->secondaryMovingCursors().size(), 1);
    QCOMPARE(*view->secondaryMovingCursors().at(0), Cursor(1, 0));
    QCOMPARE(view->secondarySelections().size(), 0);

    view->doc()->undo();

    QCOMPARE(doc.text(), QStringLiteral("foo\nfoo"));
    QCOMPARE(view->cursorPosition(), Cursor(0, 0));
    QCOMPARE(view->secondaryMovingCursors().size(), 1);
    QCOMPARE(*view->secondaryMovingCursors().at(0), Cursor(1, 0));
    QCOMPARE(view->secondarySelections().size(), 1);
    QCOMPARE(*view->secondarySelections().at(0).range, Range(1, 0, 1, 3));
    QCOMPARE(view->secondarySelections().at(0).anchor, Cursor(1, 3));
}

void MulticursorTest::testCreateMultiCursor()
{
    CREATE_VIEW_AND_DOC("foo\nbar\nfoo\n", 0, 0);

    QObject *internalView = findViewInternal(view);
    QVERIFY(internalView);

    // Alt + click should add a cursor
    clickAtPosition(view, internalView, {1, 0}, Qt::AltModifier);
    QCOMPARE(view->secondaryMovingCursors().size(), 1);
    QCOMPARE(view->secondaryMovingCursors().at(0)->toCursor(), KTextEditor::Cursor(1, 0));

    // Alt + click at the same point should remove the cursor
    clickAtPosition(view, internalView, {1, 0}, Qt::AltModifier);
    QCOMPARE(view->secondaryMovingCursors().size(), 0);

    // Create two cursors using alt+click
    clickAtPosition(view, internalView, {1, 0}, Qt::AltModifier);
    clickAtPosition(view, internalView, {1, 1}, Qt::AltModifier);
    QCOMPARE(view->secondaryMovingCursors().size(), 2);

    // now simple click => show remove all secondary cursors
    clickAtPosition(view, internalView, {0, 0}, Qt::NoModifier);
    QCOMPARE(view->secondaryMovingCursors().size(), 0);
    QCOMPARE(view->cursorPosition(), Cursor(0, 0));
}

void MulticursorTest::testCreateMultiCursorFromSelection()
{
    CREATE_VIEW_AND_DOC("foo\nbar\nfoo", 2, 3);
    view->setSelection(KTextEditor::Range(0, 0, 2, 3));
    view->createMultiCursorsFromSelection();

    const auto &cursors = view->secondaryMovingCursors();
    QCOMPARE(cursors.size(), doc.lines() - 1); // 1 cursor is primary, not included

    int i = 0;
    for (auto *c : cursors) {
        QCOMPARE(c->toCursor(), KTextEditor::Cursor(i, 3));
        i++;
    }
}

void MulticursorTest::moveCharTest()
{
    CREATE_VIEW_AND_DOC("foo\nbar\nfoo\n", 0, 0);

    // Simple left right
    view->addSecondaryCursorAt({1, 0});
    view->cursorRight();
    QCOMPARE(view->cursorPosition(), Cursor(0, 1));
    QCOMPARE(*view->secondaryMovingCursors().at(0), Cursor(1, 1));

    view->cursorLeft();
    QCOMPARE(view->cursorPosition(), Cursor(0, 0));
    QCOMPARE(*view->secondaryMovingCursors().at(0), Cursor(1, 0));

    // Shift pressed
    view->shiftCursorRight();
    QCOMPARE(view->cursorPosition(), Cursor(0, 1));
    QCOMPARE(*view->secondaryMovingCursors().at(0), Cursor(1, 1));
    QCOMPARE(view->secondarySelections().at(0).range->toRange(), Range(1, 0, 1, 1));

    view->shiftCursorLeft();
    QCOMPARE(view->cursorPosition(), Cursor(0, 0));
    QCOMPARE(*view->secondaryMovingCursors().at(0), Cursor(1, 0));
    QCOMPARE(view->secondarySelections().at(0).range->toRange(), Range(1, 0, 1, 0));

    view->clearSecondaryCursors();

    // Selection merge test => merge into primary cursor
    view->setCursorPosition({0, 2}); // fo|o
    view->addSecondaryCursorAt({0, 3}); // foo|
    // Two shift left should result in one cursor
    view->shiftCursorLeft();
    view->shiftCursorLeft();
    QCOMPARE(view->cursorPosition(), Cursor(0, 0));
    QCOMPARE(view->secondaryMovingCursors().size(), 0);
    QCOMPARE(view->selectionRange(), Range(0, 0, 0, 3));

    view->clearSelection();

    // Selection merge test => merge primary into multi => multi becomes primary
    view->setCursorPosition({0, 0}); // fo|o
    view->addSecondaryCursorAt({0, 1}); // foo|
    // Two shift left should result in one cursor
    view->shiftCursorRight();
    view->shiftCursorRight();
    QCOMPARE(view->cursorPosition(), Cursor(0, 3));
    QCOMPARE(view->secondaryMovingCursors().size(), 0);
    QCOMPARE(view->selectionRange(), Range(0, 0, 0, 3));
}

void MulticursorTest::moveCharInFirstOrLastLineTest()
{
    CREATE_VIEW_AND_DOC("foo", 0, 0);
    view->addSecondaryCursorAt({0, 1});
    // |f|oo

    view->cursorLeft();
    QCOMPARE(view->secondaryMovingCursors().size(), 0);
    QCOMPARE(view->cursorPosition(), Cursor(0, 0));

    view->setCursorPosition({0, 2});
    view->addSecondaryCursorAt({0, 3});
    view->cursorRight();
    QCOMPARE(view->secondaryMovingCursors().size(), 0);
    QCOMPARE(view->cursorPosition(), Cursor(0, 3));
}

void MulticursorTest::moveWordTest()
{
    CREATE_VIEW_AND_DOC("foo\nbar\nfoo\n", 0, 0);

    // Simple left right
    view->addSecondaryCursorAt({1, 0});
    view->wordRight();
    QCOMPARE(view->cursorPosition(), Cursor(0, 3));
    QCOMPARE(*view->secondaryMovingCursors().at(0), Cursor(1, 3));

    view->wordLeft();
    QCOMPARE(view->cursorPosition(), Cursor(0, 0));
    QCOMPARE(*view->secondaryMovingCursors().at(0), Cursor(1, 0));

    // Shift pressed
    view->shiftWordRight();
    QCOMPARE(view->cursorPosition(), Cursor(0, 3));
    QCOMPARE(*view->secondaryMovingCursors().at(0), Cursor(1, 3));
    QCOMPARE(view->secondarySelections().at(0).range->toRange(), Range(1, 0, 1, 3));

    view->shiftWordLeft();
    QCOMPARE(view->cursorPosition(), Cursor(0, 0));
    QCOMPARE(*view->secondaryMovingCursors().at(0), Cursor(1, 0));
    QCOMPARE(view->secondarySelections().at(0).range->toRange(), Range(1, 0, 1, 0));

    view->clearSecondaryCursors();

    // Two cursors in same word, => word movement should merge them (sel)
    view->setCursorPosition({0, 0}); // |foo
    view->addSecondaryCursorAt({0, 1}); // f|oo
    view->shiftWordRight(); // foo|
    QCOMPARE(view->cursorPosition(), Cursor(0, 3));
    QCOMPARE(view->secondaryMovingCursors().size(), 0);
    QCOMPARE(view->selectionRange(), Range(0, 0, 0, 3));

    // Three cursors in same word, => word movement should merge them (no sel)
    view->setCursorPosition({0, 3}); // foo|
    view->addSecondaryCursorAt({0, 2}); // fo|o
    view->addSecondaryCursorAt({0, 1}); // f|oo
    view->wordLeft(); // foo|
    QCOMPARE(view->cursorPosition(), Cursor(0, 0));
    QCOMPARE(view->secondaryMovingCursors().size(), 0);
}

void MulticursorTest::homeEndKeyTest()
{
    CREATE_VIEW_AND_DOC("foo\nbar\nfoo\n", 0, 0);

    // Two cursor in same line => home should merge them
    view->addSecondaryCursorAt({0, 1});
    view->home();
    QCOMPARE(view->cursorPosition(), Cursor(0, 0));
    QCOMPARE(view->secondaryMovingCursors().size(), 0);

    // Two cursor in same line => end should merge them
    view->addSecondaryCursorAt({0, 1});
    view->end();
    QCOMPARE(view->cursorPosition(), Cursor(0, 3));
    QCOMPARE(view->secondaryMovingCursors().size(), 0);

    view->addSecondaryCursorAt({1, 0});
    view->end();
    QCOMPARE(view->cursorPosition(), Cursor(0, 3));
    QCOMPARE(*view->secondaryMovingCursors().at(0), Cursor(1, 3));

    view->clearSecondaryCursors();

    view->setCursorPosition({0, 3});
    view->addSecondaryCursorAt({1, 3});
    view->home();
    QCOMPARE(view->cursorPosition(), Cursor(0, 0));
    QCOMPARE(*view->secondaryMovingCursors().at(0), Cursor(1, 0));
}

void MulticursorTest::moveUpDown()
{
    /** TEST UP **/
    CREATE_VIEW_AND_DOC("foo\nbar\nfoo", 0, 0);

    view->addSecondaryCursors({Cursor(1, 0), Cursor(2, 0)});
    QCOMPARE(view->secondaryMovingCursors().size(), 2);

    view->up();
    QCOMPARE(view->secondaryMovingCursors().size(), 1);

    view->up();
    QCOMPARE(view->secondaryMovingCursors().size(), 0);

    /** TEST DOWN **/

    view->addSecondaryCursors({Cursor(1, 0), Cursor(2, 0)});
    QCOMPARE(view->secondaryMovingCursors().size(), 2);

    view->down();
    QCOMPARE(view->secondaryMovingCursors().size(), 2); // last cursor moves to end of line
    QCOMPARE(*view->secondaryMovingCursors().at(1), Cursor(2, 3));

    view->down();
    QCOMPARE(view->secondaryMovingCursors().size(), 1);

    view->down();
    QCOMPARE(view->secondaryMovingCursors().size(), 0);
    QCOMPARE(view->cursorPosition(), Cursor(2, 3));
}

void MulticursorTest::findNextOccurenceTest()
{
    CREATE_VIEW_AND_DOC("foo\nbar\nfoo\nfoo", 0, 0);

    // No selection
    view->findNextOccurunceAndSelect();
    QCOMPARE(view->selectionRange(), Range(0, 0, 0, 3));
    QCOMPARE(view->cursorPosition(), Cursor(0, 3));
    QCOMPARE(view->secondaryMovingCursors().size(), 0);

    view->clearSelection();
    // with selection
    view->setSelection(Range(0, 0, 0, 3));
    view->findNextOccurunceAndSelect();
    QCOMPARE(view->secondaryMovingCursors().size(), 1);
    QCOMPARE(view->secondaryMovingCursors().at(0)->toCursor(), Cursor(0, 3));
    QCOMPARE(view->secondarySelections().at(0).range->toRange(), Range(0, 0, 0, 3));
    // primary cursor has the last selection
    QCOMPARE(view->cursorPosition(), Cursor(2, 3));
    QCOMPARE(view->selectionRange(), Range(2, 0, 2, 3));

    // find another
    view->findNextOccurunceAndSelect();
    QCOMPARE(view->secondaryMovingCursors().size(), 2);
    QCOMPARE(view->secondaryMovingCursors().at(0)->toCursor(), Cursor(0, 3));
    QCOMPARE(view->secondarySelections().at(0).range->toRange(), Range(0, 0, 0, 3));
    QCOMPARE(view->secondaryMovingCursors().at(1)->toCursor(), Cursor(2, 3));
    QCOMPARE(view->secondarySelections().at(1).range->toRange(), Range(2, 0, 2, 3));
    // primary cursor has the last selection
    QCOMPARE(view->cursorPosition(), Cursor(3, 3));
    QCOMPARE(view->selectionRange(), Range(3, 0, 3, 3));

    // Try to find another, there is none so nothing should change
    view->findNextOccurunceAndSelect();
    QCOMPARE(view->cursorPosition(), Cursor(3, 3));
    QCOMPARE(view->selectionRange(), Range(3, 0, 3, 3));
}

void MulticursorTest::findAllOccurenceTest()
{
    CREATE_VIEW_AND_DOC("foo\nbar\nfoo\nfoo", 0, 0);

    // No selection
    view->findAllOccuruncesAndSelect();
    QCOMPARE(view->selectionRange(), Range(0, 0, 0, 3));
    QCOMPARE(view->cursorPosition(), Cursor(0, 3));
    QCOMPARE(view->secondaryMovingCursors().size(), 2);
    // first
    QCOMPARE(view->secondaryMovingCursors().at(0)->toCursor(), Cursor(2, 3));
    QCOMPARE(view->secondarySelections().at(0).range->toRange(), Range(2, 0, 2, 3));
    // second
    QCOMPARE(view->secondaryMovingCursors().at(1)->toCursor(), Cursor(3, 3));
    QCOMPARE(view->secondarySelections().at(1).range->toRange(), Range(3, 0, 3, 3));

    // Try to find another, there is none so nothing should change
    view->findAllOccuruncesAndSelect();
    QCOMPARE(view->cursorPosition(), Cursor(0, 3));
    QCOMPARE(view->selectionRange(), Range(0, 0, 0, 3));
}

// kate: indent-mode cstyle; indent-width 4; replace-tabs on;
