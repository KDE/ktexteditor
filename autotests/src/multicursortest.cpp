/*
    This file is part of the KDE libraries
    SPDX-FileCopyrightText: 2022 Waqar Ahmed <waqar.17a@gmail.com>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "multicursortest.h"

#include <QClipboard>
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

template<typename Cont>
bool isSorted(const Cont &c)
{
    return std::is_sorted(c.begin(), c.end());
}

MulticursorTest::MulticursorTest()
    : QObject()
{
    KTextEditor::EditorPrivate::enableUnitTestMode();
}

MulticursorTest::~MulticursorTest()
{
}

static QWidget *findViewInternal(KTextEditor::ViewPrivate *view)
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
    QVERIFY(isSorted(view->secondaryCursors()));

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
        QVERIFY(isSorted(view->secondaryCursors()));
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
        QVERIFY(isSorted(view->secondaryCursors()));

        QCOMPARE(doc.line(0), QStringLiteral("foo"));
        QCOMPARE(doc.line(1), QStringLiteral("bar"));
        QCOMPARE(doc.line(2), QStringLiteral("foo"));

        QVERIFY(isSorted(view->secondaryCursors()));
        view->clearSecondaryCursors();
    }

    // Three empty lines
    doc.setText(QStringLiteral("\n\n\n"));
    view->setCursorPosition({0, 0});
    view->addSecondaryCursorAt({1, 0});
    view->addSecondaryCursorAt({2, 0});
    QVERIFY(isSorted(view->secondaryCursors()));

    // cursors should merge
    view->backspace();
    QCOMPARE(view->secondaryCursors().size(), 0);
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
    QCOMPARE(view->secondaryCursors().size(), 1);
    QCOMPARE(*view->secondaryCursors().at(0).pos, Cursor(1, 3));

    // Another undo, multicursor should be gone
    view->doc()->undo();
    QCOMPARE(doc.text(), QStringLiteral("foo\nfoo"));
    QCOMPARE(view->secondaryCursors().size(), 0);

    // One redo
    view->doc()->redo();
    QCOMPARE(doc.text(), QStringLiteral("fo\nfoo"));

    // Second redo, multicursor should be back
    view->doc()->redo();
    QCOMPARE(doc.text(), QStringLiteral("f\nfo"));
    QCOMPARE(view->secondaryCursors().size(), 1);
    QCOMPARE(*view->secondaryCursors().at(0).pos, Cursor(1, 2));
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
    QCOMPARE(view->secondaryCursors().size(), 1);
    QCOMPARE(*view->secondaryCursors().at(0).pos, Cursor(1, 0));

    view->doc()->undo();

    QCOMPARE(doc.text(), QStringLiteral("foo\nfoo"));
    QCOMPARE(view->cursorPosition(), Cursor(0, 0));
    QCOMPARE(view->secondaryCursors().size(), 1);
    QCOMPARE(*view->secondaryCursors().at(0).pos, Cursor(1, 0));
    QCOMPARE(*view->secondaryCursors().at(0).range, Range(1, 0, 1, 3));
    QCOMPARE(view->secondaryCursors().at(0).anchor, Cursor(1, 3));
}

void MulticursorTest::keyReturnIndentTest()
{
    CREATE_VIEW_AND_DOC("\n\n", 0, 0);
    QCOMPARE(doc.lines(), 3);
    doc.setMode(QStringLiteral("C++"));
    view->config()->setValue(KateViewConfig::AutoBrackets, true);

    view->addSecondaryCursorDown();
    view->addSecondaryCursorDown();
    QCOMPARE(view->secondaryCursors().size(), 2);
    QVERIFY(isSorted(view->secondaryCursors()));

    doc.typeChars(view, QStringLiteral("{"));
    QCOMPARE(doc.text(), QStringLiteral("{}\n{}\n{}"));
    QCOMPARE(view->secondaryCursors().size(), 2);
    QVERIFY(isSorted(view->secondaryCursors()));

    view->keyReturn();
    QCOMPARE(doc.text(), QStringLiteral("{\n    \n}\n{\n    \n}\n{\n    \n}"));
}

void MulticursorTest::wrapSelectionWithCharsTest()
{
    CREATE_VIEW_AND_DOC("foo\nfoo\nfoo", 0, 3);

    view->addSecondaryCursorDown();
    view->addSecondaryCursorDown();
    QCOMPARE(view->secondaryCursors().size(), 2);

    view->shiftWordLeft();
    doc.typeChars(view, QStringLiteral("{"));
    QCOMPARE(doc.text(), QStringLiteral("{foo}\n{foo}\n{foo}"));
}

void MulticursorTest::testCreateMultiCursor()
{
    CREATE_VIEW_AND_DOC("foo\nbar\nfoo\n", 0, 0);

    QObject *internalView = findViewInternal(view);
    QVERIFY(internalView);

    // Alt + click should add a cursor
    clickAtPosition(view, internalView, {1, 0}, Qt::AltModifier);
    QCOMPARE(view->secondaryCursors().size(), 1);
    QCOMPARE(view->secondaryCursors().at(0).cursor(), KTextEditor::Cursor(1, 0));

    // Alt + click at the same point should remove the cursor
    clickAtPosition(view, internalView, {1, 0}, Qt::AltModifier);
    QCOMPARE(view->secondaryCursors().size(), 0);

    // Create two cursors using alt+click
    clickAtPosition(view, internalView, {1, 0}, Qt::AltModifier);
    clickAtPosition(view, internalView, {1, 1}, Qt::AltModifier);
    QCOMPARE(view->secondaryCursors().size(), 2);
    QVERIFY(isSorted(view->secondaryCursors()));

    // now simple click => show remove all secondary cursors
    clickAtPosition(view, internalView, {0, 0}, Qt::NoModifier);
    QCOMPARE(view->secondaryCursors().size(), 0);
    QCOMPARE(view->cursorPosition(), Cursor(0, 0));
}

void MulticursorTest::testCreateMultiCursorFromSelection()
{
    CREATE_VIEW_AND_DOC("foo\nbar\nfoo", 2, 3);
    view->setSelection(KTextEditor::Range(0, 0, 2, 3));
    // move primary cursor to beginning of line, so we can check whether it is moved to end of line
    view->setCursorPosition({view->cursorPosition().line(), 0});
    view->createMultiCursorsFromSelection();
    QVERIFY(isSorted(view->secondaryCursors()));
    QCOMPARE(view->cursorPosition().column(), 3);

    const auto &cursors = view->secondaryCursors();
    QCOMPARE(cursors.size(), doc.lines() - 1); // 1 cursor is primary, not included

    int i = 0;
    for (const auto &c : cursors) {
        QCOMPARE(c.cursor(), KTextEditor::Cursor(i, 3));
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
    QCOMPARE(view->secondaryCursors().at(0).cursor(), Cursor(1, 1));

    view->cursorLeft();
    QCOMPARE(view->cursorPosition(), Cursor(0, 0));
    QCOMPARE(view->secondaryCursors().at(0).cursor(), Cursor(1, 0));

    // Shift pressed
    view->shiftCursorRight();
    QCOMPARE(view->cursorPosition(), Cursor(0, 1));
    QCOMPARE(view->secondaryCursors().at(0).cursor(), Cursor(1, 1));
    QCOMPARE(view->secondaryCursors().at(0).range->toRange(), Range(1, 0, 1, 1));

    view->shiftCursorLeft();
    QCOMPARE(view->cursorPosition(), Cursor(0, 0));
    QCOMPARE(view->secondaryCursors().at(0).cursor(), Cursor(1, 0));
    QCOMPARE(view->secondaryCursors().at(0).range->toRange(), Range(1, 0, 1, 0));

    view->clearSecondaryCursors();

    // Selection merge test => merge into primary cursor
    view->setCursorPosition({0, 2}); // fo|o
    view->addSecondaryCursorAt({0, 3}); // foo|
    // Two shift left should result in one cursor
    view->shiftCursorLeft();
    view->shiftCursorLeft();
    QCOMPARE(view->cursorPosition(), Cursor(0, 0));
    QCOMPARE(view->secondaryCursors().size(), 0);
    QCOMPARE(view->selectionRange(), Range(0, 0, 0, 3));

    view->clearSelection();

    // Selection merge test => merge primary into multi => multi becomes primary
    view->setCursorPosition({0, 0}); // fo|o
    view->addSecondaryCursorAt({0, 1}); // foo|
    // Two shift left should result in one cursor
    view->shiftCursorRight();
    view->shiftCursorRight();
    QCOMPARE(view->cursorPosition(), Cursor(0, 3));
    QCOMPARE(view->secondaryCursors().size(), 0);
    QCOMPARE(view->selectionRange(), Range(0, 0, 0, 3));
}

void MulticursorTest::moveCharInFirstOrLastLineTest()
{
    CREATE_VIEW_AND_DOC("foo", 0, 0);
    view->addSecondaryCursorAt({0, 1});
    // |f|oo

    view->cursorLeft();
    QCOMPARE(view->secondaryCursors().size(), 0);
    QCOMPARE(view->cursorPosition(), Cursor(0, 0));

    view->setCursorPosition({0, 2});
    view->addSecondaryCursorAt({0, 3});
    view->cursorRight();
    QCOMPARE(view->secondaryCursors().size(), 0);
    QCOMPARE(view->cursorPosition(), Cursor(0, 3));
}

void MulticursorTest::moveWordTest()
{
    CREATE_VIEW_AND_DOC("foo\nbar\nfoo\n", 0, 0);

    // Simple left right
    view->addSecondaryCursorAt({1, 0});
    view->wordRight();
    QCOMPARE(view->cursorPosition(), Cursor(0, 3));
    QCOMPARE(*view->secondaryCursors().at(0).pos, Cursor(1, 3));

    view->wordLeft();
    QCOMPARE(view->cursorPosition(), Cursor(0, 0));
    QCOMPARE(*view->secondaryCursors().at(0).pos, Cursor(1, 0));

    // Shift pressed
    view->shiftWordRight();
    QCOMPARE(view->cursorPosition(), Cursor(0, 3));
    QCOMPARE(*view->secondaryCursors().at(0).pos, Cursor(1, 3));
    QCOMPARE(view->secondaryCursors().at(0).range->toRange(), Range(1, 0, 1, 3));

    view->shiftWordLeft();
    QCOMPARE(view->cursorPosition(), Cursor(0, 0));
    QCOMPARE(*view->secondaryCursors().at(0).pos, Cursor(1, 0));
    QCOMPARE(view->secondaryCursors().at(0).range->toRange(), Range(1, 0, 1, 0));

    view->clearSecondaryCursors();

    // Two cursors in same word, => word movement should merge them (sel)
    view->setCursorPosition({0, 0}); // |foo
    view->addSecondaryCursorAt({0, 1}); // f|oo
    view->shiftWordRight(); // foo|
    QCOMPARE(view->cursorPosition(), Cursor(0, 3));
    QCOMPARE(view->secondaryCursors().size(), 0);
    QCOMPARE(view->selectionRange(), Range(0, 0, 0, 3));

    // Three cursors in same word, => word movement should merge them (no sel)
    view->setCursorPosition({0, 3}); // foo|
    view->addSecondaryCursorAt({0, 2}); // fo|o
    view->addSecondaryCursorAt({0, 1}); // f|oo
    view->wordLeft(); // foo|
    QCOMPARE(view->cursorPosition(), Cursor(0, 0));
    QCOMPARE(view->secondaryCursors().size(), 0);
}

void MulticursorTest::homeEndKeyTest()
{
    CREATE_VIEW_AND_DOC("foo\nbar\nfoo\n", 0, 0);

    // Two cursor in same line => home should merge them
    view->addSecondaryCursorAt({0, 1});
    view->home();
    QCOMPARE(view->cursorPosition(), Cursor(0, 0));
    QCOMPARE(view->secondaryCursors().size(), 0);

    // Two cursor in same line => end should merge them
    view->addSecondaryCursorAt({0, 1});
    view->end();
    QCOMPARE(view->cursorPosition(), Cursor(0, 3));
    QCOMPARE(view->secondaryCursors().size(), 0);

    view->addSecondaryCursorAt({1, 0});
    view->end();
    QCOMPARE(view->cursorPosition(), Cursor(0, 3));
    QCOMPARE(*view->secondaryCursors().at(0).pos, Cursor(1, 3));

    view->clearSecondaryCursors();

    view->setCursorPosition({0, 3});
    view->addSecondaryCursorAt({1, 3});
    view->home();
    QCOMPARE(view->cursorPosition(), Cursor(0, 0));
    QCOMPARE(*view->secondaryCursors().at(0).pos, Cursor(1, 0));
}

void MulticursorTest::moveUpDown()
{
    /** TEST UP **/
    CREATE_VIEW_AND_DOC("foo\nbar\nfoo", 0, 0);

    view->setSecondaryCursors({Cursor(1, 0), Cursor(2, 0)});
    QCOMPARE(view->secondaryCursors().size(), 2);
    QVERIFY(isSorted(view->secondaryCursors()));

    view->up();
    QCOMPARE(view->secondaryCursors().size(), 1);

    view->up();
    QCOMPARE(view->secondaryCursors().size(), 0);

    /** TEST DOWN **/

    view->setSecondaryCursors({Cursor(1, 0), Cursor(2, 0)});
    QCOMPARE(view->secondaryCursors().size(), 2);

    view->down();
    QCOMPARE(view->secondaryCursors().size(), 2); // last cursor moves to end of line
    QCOMPARE(*view->secondaryCursors().at(1).pos, Cursor(2, 3));
    QVERIFY(isSorted(view->secondaryCursors()));

    view->down();
    QCOMPARE(view->secondaryCursors().size(), 1);

    view->down();
    QCOMPARE(view->secondaryCursors().size(), 0);
    QCOMPARE(view->cursorPosition(), Cursor(2, 3));
}

void MulticursorTest::testSelectionMerge()
{
    // 8 lines
    {
        // Left movement, cursor at top
        CREATE_VIEW_AND_DOC("foo\nfoo\nfoo\nfoo\nfoo\nfoo\nfoo", 0, 3);

        view->selectAll();
        view->createMultiCursorsFromSelection();
        QVERIFY(isSorted(view->secondaryCursors()));

        QCOMPARE(view->secondaryCursors().size(), 6);

        view->shiftWordLeft();
        QVERIFY(isSorted(view->secondaryCursors()));
        view->shiftWordLeft();
        QVERIFY(isSorted(view->secondaryCursors()));
        view->shiftWordLeft();

        QCOMPARE(view->secondaryCursors().size(), 0);
        QCOMPARE(view->cursorPosition(), Cursor(0, 0));
        QCOMPARE(view->selectionRange(), Range(0, 0, 6, 3));
    }

    {
        // Left movement, cursor at bottom
        CREATE_VIEW_AND_DOC("foo\nfoo\nfoo\nfoo\nfoo\nfoo\nfoo", 6, 3);

        view->selectAll();
        view->createMultiCursorsFromSelection();
        QVERIFY(isSorted(view->secondaryCursors()));

        QCOMPARE(view->secondaryCursors().size(), 6);
        QCOMPARE(view->cursorPosition(), Cursor(6, 3));

        view->shiftWordLeft();
        QVERIFY(isSorted(view->secondaryCursors()));
        view->shiftWordLeft();
        QVERIFY(isSorted(view->secondaryCursors()));
        view->shiftWordLeft();

        QCOMPARE(view->secondaryCursors().size(), 0);
        QCOMPARE(view->cursorPosition(), Cursor(0, 0));
        QCOMPARE(view->selectionRange(), Range(0, 0, 6, 3));
    }

    {
        // Left word movement, cursor in the middle
        CREATE_VIEW_AND_DOC("foo\nfoo\nfoo\nfoo\nfoo\nfoo\nfoo", 3, 3);

        for (int i = 0; i < 10; ++i) {
            view->addSecondaryCursorUp();
            view->addSecondaryCursorDown();
        }

        QCOMPARE(view->secondaryCursors().size(), 6);
        QCOMPARE(view->cursorPosition(), Cursor(3, 3));

        view->shiftWordLeft();
        view->shiftWordLeft();
        view->shiftWordLeft();

        QCOMPARE(view->secondaryCursors().size(), 0);
        QCOMPARE(view->cursorPosition(), Cursor(0, 0));
        QCOMPARE(view->selectionRange(), Range(0, 0, 6, 3));
    }

    {
        // Left word + char movement, cursor in the middle
        CREATE_VIEW_AND_DOC("foo\nfoo\nfoo\nfoo\nfoo\nfoo\nfoo", 3, 3);

        view->addSecondaryCursorUp();
        view->addSecondaryCursorUp();
        view->addSecondaryCursorDown();
        view->addSecondaryCursorDown();
        QVERIFY(isSorted(view->secondaryCursors()));

        QCOMPARE(view->secondaryCursors().size(), 4);
        QCOMPARE(view->cursorPosition(), Cursor(3, 3));

        view->shiftWordLeft();
        view->shiftCursorLeft();
        view->shiftCursorLeft();
        view->shiftCursorLeft();

        QCOMPARE(view->secondaryCursors().size(), 0);
        QCOMPARE(view->cursorPosition(), Cursor(0, 1));
        QCOMPARE(view->selectionRange(), Range(0, 1, 5, 3));
    }

    {
        // Right movement, cursor at bottom line
        CREATE_VIEW_AND_DOC("foo\nfoo\nfoo\nfoo\nfoo\nfoo\nfoo", 6, 0);

        for (int i = 0; i < 10; ++i) {
            view->addSecondaryCursorUp();
        }

        QCOMPARE(view->secondaryCursors().size(), 6);
        QCOMPARE(view->cursorPosition(), Cursor(6, 0));

        view->shiftWordRight();
        QVERIFY(isSorted(view->secondaryCursors()));
        view->shiftWordRight();
        QVERIFY(isSorted(view->secondaryCursors()));
        view->shiftWordRight();

        QCOMPARE(view->secondaryCursors().size(), 0);
        QCOMPARE(view->cursorPosition(), Cursor(6, 3));
        QCOMPARE(view->selectionRange(), Range(0, 0, 6, 3));
    }

    {
        // Right movement, cursor at top line
        CREATE_VIEW_AND_DOC("foo\nfoo\nfoo\nfoo\nfoo\nfoo\nfoo", 0, 0);

        for (int i = 0; i < 10; ++i) {
            view->addSecondaryCursorDown();
        }

        QCOMPARE(view->secondaryCursors().size(), 6);
        QCOMPARE(view->cursorPosition(), Cursor(0, 0));

        view->shiftWordRight();
        view->shiftWordRight();
        view->shiftWordRight();

        QCOMPARE(view->secondaryCursors().size(), 0);
        QCOMPARE(view->cursorPosition(), Cursor(6, 3));
        QCOMPARE(view->selectionRange(), Range(0, 0, 6, 3));
    }

    {
        // Right word + char movement, cursor in the middle
        CREATE_VIEW_AND_DOC("foo\nfoo\nfoo\nfoo\nfoo\nfoo\nfoo", 3, 0);

        view->addSecondaryCursorUp();
        view->addSecondaryCursorUp();
        view->addSecondaryCursorDown();
        view->addSecondaryCursorDown();
        QVERIFY(isSorted(view->secondaryCursors()));

        QCOMPARE(view->secondaryCursors().size(), 4);
        QCOMPARE(view->cursorPosition(), Cursor(3, 0));

        view->shiftWordRight();
        QVERIFY(isSorted(view->secondaryCursors()));
        view->shiftCursorRight();
        QVERIFY(isSorted(view->secondaryCursors()));
        view->shiftCursorRight();
        QVERIFY(isSorted(view->secondaryCursors()));
        view->shiftCursorRight();

        QCOMPARE(view->secondaryCursors().size(), 0);
        QCOMPARE(view->cursorPosition(), Cursor(6, 2));
        QCOMPARE(view->selectionRange(), Range(1, 0, 6, 2));
    }
}

void MulticursorTest::findNextOccurenceTest()
{
    CREATE_VIEW_AND_DOC("foo\nbar\nfoo\nfoo", 0, 0);

    // No selection
    view->findNextOccurunceAndSelect();
    QCOMPARE(view->selectionRange(), Range(0, 0, 0, 3));
    QCOMPARE(view->cursorPosition(), Cursor(0, 3));
    QCOMPARE(view->secondaryCursors().size(), 0);

    view->clearSelection();
    // with selection
    view->setSelection(Range(0, 0, 0, 3));
    view->findNextOccurunceAndSelect();
    QCOMPARE(view->secondaryCursors().size(), 1);
    QCOMPARE(view->secondaryCursors().at(0).cursor(), Cursor(0, 3));
    QCOMPARE(view->secondaryCursors().at(0).range->toRange(), Range(0, 0, 0, 3));
    // primary cursor has the last selection
    QCOMPARE(view->cursorPosition(), Cursor(2, 3));
    QCOMPARE(view->selectionRange(), Range(2, 0, 2, 3));

    // find another
    view->findNextOccurunceAndSelect();
    QCOMPARE(view->secondaryCursors().size(), 2);
    QVERIFY(isSorted(view->secondaryCursors()));
    QCOMPARE(view->secondaryCursors().at(0).cursor(), Cursor(0, 3));
    QCOMPARE(view->secondaryCursors().at(0).range->toRange(), Range(0, 0, 0, 3));
    QCOMPARE(view->secondaryCursors().at(1).cursor(), Cursor(2, 3));
    QCOMPARE(view->secondaryCursors().at(1).range->toRange(), Range(2, 0, 2, 3));
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
    QCOMPARE(view->secondaryCursors().size(), 2);
    // first
    QCOMPARE(view->secondaryCursors().at(0).cursor(), Cursor(2, 3));
    QCOMPARE(view->secondaryCursors().at(0).range->toRange(), Range(2, 0, 2, 3));
    // second
    QCOMPARE(view->secondaryCursors().at(1).cursor(), Cursor(3, 3));
    QCOMPARE(view->secondaryCursors().at(1).range->toRange(), Range(3, 0, 3, 3));

    // Try to find another, there is none so nothing should change
    view->findAllOccuruncesAndSelect();
    QCOMPARE(view->cursorPosition(), Cursor(0, 3));
    QCOMPARE(view->selectionRange(), Range(0, 0, 0, 3));
}

void MulticursorTest::testMultiCopyPaste()
{
    // Create two docs, copy from one to the other
    {
        CREATE_VIEW_AND_DOC("foo\nbar\nfoo\nfoo", 0, 0);
        view->addSecondaryCursorAt({1, 0});
        view->addSecondaryCursorAt({2, 0});
        view->addSecondaryCursorAt({3, 0});
        view->shiftWordRight();
        view->copy();
    }

    // Same number of cursors when pasting => each line gets pasted into matching cursor postion
    {
        KTextEditor::DocumentPrivate doc;
        doc.setText("\n\n\n\n");
        KTextEditor::ViewPrivate *v = new KTextEditor::ViewPrivate(&doc, nullptr);
        v->setCursorPosition({0, 0});
        v->addSecondaryCursorAt({1, 0});
        v->addSecondaryCursorAt({2, 0});
        v->addSecondaryCursorAt({3, 0});
        v->paste();
        QCOMPARE(doc.text(), QStringLiteral("foo\nbar\nfoo\nfoo\n"));

        // Different number of cursors
        v->clear();
        QVERIFY(doc.clear());
        doc.setText(QStringLiteral("\n\n"));
        v->setCursorPosition({0, 0});
        v->addSecondaryCursorAt({1, 0});
        QCOMPARE(v->secondaryCursors().size(), 1);

        v->paste();
        QString text = doc.text();
        QCOMPARE(text, QStringLiteral("foo\nbar\nfoo\nfoo\nfoo\nbar\nfoo\nfoo\n"));
    }
}

void MulticursorTest::testSelectionTextOrdering()
{
    CREATE_VIEW_AND_DOC("foo\nbar\nfoo\nfoo", 0, 0);
    view->addSecondaryCursorAt({1, 0});
    view->addSecondaryCursorAt({2, 0});
    view->shiftWordRight();
    QVERIFY(isSorted(view->secondaryCursors()));

    QString selText = view->selectionText();
    QCOMPARE(selText, QStringLiteral("foo\nbar\nfoo"));

    view->copy();
    QCOMPARE(QApplication::clipboard()->text(QClipboard::Clipboard), selText);
}

void MulticursorTest::testViewClear()
{
    CREATE_VIEW_AND_DOC("foo\nbar", 0, 0);
    view->addSecondaryCursorAt({1, 0});
    QCOMPARE(view->secondaryCursors().size(), 1);
    view->clear();
    QCOMPARE(view->secondaryCursors().size(), 0);
}

void MulticursorTest::testSetGetCursors()
{
    CREATE_VIEW_AND_DOC("foo\nbar\nfoo\nfoo", 0, 0);
    // primary included
    QCOMPARE(view->cursors(), QVector<Cursor>{Cursor(0, 0)});

    const QVector<Cursor> cursors = {{0, 1}, {1, 1}, {2, 1}, {3, 1}};
    view->setCursors(cursors);
    QCOMPARE(view->cursors(), cursors);
    QVERIFY(isSorted(view->cursors()));
    QCOMPARE(view->cursorPosition(), Cursor(0, 1));
    // We have no selection
    QVERIFY(!view->selection());
    QCOMPARE(view->selectionRanges(), QVector<Range>{});
}

void MulticursorTest::testSetGetSelections()
{
    CREATE_VIEW_AND_DOC("foo\nbar\nfoo", 0, 0);
    // primary included
    QCOMPARE(view->cursors(), QVector<Cursor>{Cursor(0, 0)});

    QVector<Cursor> cursors = {{0, 1}, {1, 1}, {2, 1}};
    view->setCursors(cursors);
    QCOMPARE(view->cursors(), cursors);
    QVERIFY(isSorted(view->cursors()));
    view->shiftCursorRight();
    QVERIFY(view->selection());
    cursors = {{0, 2}, {1, 2}, {2, 2}};
    QCOMPARE(view->cursors(), cursors);
    QVector<Range> selections = {Range(0, 1, 0, 2), Range(1, 1, 1, 2), Range(2, 1, 2, 2)};
    QCOMPARE(view->selectionRanges(), selections);
    QVERIFY(isSorted(view->selectionRanges()));
    QCOMPARE(view->selectionRange(), selections.front());
}

// kate: indent-mode cstyle; indent-width 4; replace-tabs on;
