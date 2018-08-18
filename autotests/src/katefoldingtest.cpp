/*  This file is part of the Kate project.
 *
 *  Copyright (C) 2013 Dominik Haumann <dhaumann@kde.org>
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

#include "katefoldingtest.h"

#include <kateglobal.h>
#include <katebuffer.h>
#include <katedocument.h>
#include <kateview.h>
#include <kateconfig.h>
#include <katetextfolding.h>

#include <QtTestWidgets>

using namespace KTextEditor;

QTEST_MAIN(KateFoldingTest)

void KateFoldingTest::initTestCase()
{
    KTextEditor::EditorPrivate::enableUnitTestMode();
}

void KateFoldingTest::cleanupTestCase()
{
}

// This is a unit test for bug 311866 (http://bugs.kde.org/show_bug.cgi?id=311866)
// It loads 5 lines of C++ code, places the cursor in line 4, and then folds
// the code.
// Expected behavior: the cursor should be moved so it stays visible
// Buggy behavior: the cursor is hidden, and moving the hidden cursor crashes kate
void KateFoldingTest::testCrash311866()
{
    KTextEditor::DocumentPrivate doc;
    const QUrl url = QUrl::fromLocalFile(QLatin1String(TEST_DATA_DIR"bug311866.cpp"));
    doc.openUrl(url);
    doc.setHighlightingMode("C++");
    doc.buffer().ensureHighlighted(6);

    KTextEditor::ViewPrivate *view = static_cast<KTextEditor::ViewPrivate *>(doc.createView(nullptr));
    view->show();
    view->resize(400, 300);
    view->setCursorPosition(Cursor(3, 0));
    QTest::qWait(100);

    view->slotFoldToplevelNodes();
    doc.buffer().ensureHighlighted(6);

    qDebug() << "!!! Does the next line crash?";
    view->up();
}

// This test makes sure that,
// - if you have selected text
// - that spans a folded range,
// - and the cursor is at the end of the text selection,
// - and you type a char, e.g. 'x',
// then the resulting text is correct, and changing region
// visibility does not mess around with the text cursor.
//
// See https://bugs.kde.org/show_bug.cgi?id=295632
void KateFoldingTest::testBug295632()
{
    KTextEditor::DocumentPrivate doc;
    QString text = "oooossssssss\n"
                   "{\n"
                   "\n"
                   "}\n"
                   "ssssss----------";
    doc.setText(text);

    // view must be visible...
    KTextEditor::ViewPrivate *view = static_cast<KTextEditor::ViewPrivate *>(doc.createView(nullptr));
    view->show();
    view->resize(400, 300);

    qint64 foldId = view->textFolding().newFoldingRange(KTextEditor::Range(1, 0, 3, 1));
    view->textFolding().foldRange(foldId);
    QVERIFY(view->textFolding().isLineVisible(0));
    QVERIFY(view->textFolding().isLineVisible(1));
    QVERIFY(!view->textFolding().isLineVisible(2));
    QVERIFY(!view->textFolding().isLineVisible(3));
    QVERIFY(view->textFolding().isLineVisible(4));

    view->setSelection(Range(Cursor(0, 4), Cursor(4, 6)));
    view->setCursorPosition(Cursor(4, 6));

    QTest::qWait(100);
    doc.typeChars(view, "x");
    QTest::qWait(100);

    QString line = doc.line(0);
    QCOMPARE(line, QString("oooox----------"));
}

// This testcase tests the following:
// - the cursor is first set into the word 'hello'
// - then lines 0-3 are folded.
// - the real text cursor is still in the word 'hello'
// - the important issue is: The display cursor must be in the visible line range
// --> if this test passes, KateViewInternal's m_displayCursor is properly adapted.
void KateFoldingTest::testCrash367466()
{
    KTextEditor::DocumentPrivate doc;
    QString text = "fold begin\n"
                   "\n"
                   "\n"
                   "fold end\n"
                   "hello\n"
                   "world\n";
    doc.setText(text);

    // view must be visible...
    KTextEditor::ViewPrivate *view = static_cast<KTextEditor::ViewPrivate *>(doc.createView(nullptr));
    view->show();
    view->resize(400, 300);
    view->setCursorPosition(KTextEditor::Cursor(5, 2));
    QCOMPARE(view->cursorPosition(), KTextEditor::Cursor(5, 2));
    qint64 foldId = view->textFolding().newFoldingRange(KTextEditor::Range(0, 0, 3, 8));

    view->textFolding().foldRange(foldId);
    QVERIFY(view->textFolding().isLineVisible(0));
    QVERIFY(!view->textFolding().isLineVisible(1));
    QVERIFY(!view->textFolding().isLineVisible(2));
    QVERIFY(!view->textFolding().isLineVisible(3));
    QVERIFY(view->textFolding().isLineVisible(4));
    QVERIFY(view->textFolding().isLineVisible(5));

    QCOMPARE(view->cursorPosition(), KTextEditor::Cursor(5, 2));
    view->up();
    QCOMPARE(view->cursorPosition(), KTextEditor::Cursor(4, 2));
}
