/*
    This file is part of the Kate project.
    SPDX-FileCopyrightText: 2010 Christoph Cullmann <cullmann@kde.org>
    SPDX-FileCopyrightText: 2010-2018 Dominik Haumann <dhaumann@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "katetextbuffertest.h"
#include "katetextbuffer.h"
#include "katetextcursor.h"
#include "katetextfolding.h"
#include <kateglobal.h>

QTEST_MAIN(KateTextBufferTest)

KateTextBufferTest::KateTextBufferTest()
    : QObject()
{
    KTextEditor::EditorPrivate::enableUnitTestMode();
}

KateTextBufferTest::~KateTextBufferTest()
{
}

void KateTextBufferTest::basicBufferTest()
{
    // construct an empty text buffer
    Kate::TextBuffer buffer(nullptr, 1);

    // one line per default
    QVERIFY(buffer.lines() == 1);
    QVERIFY(buffer.text() == QString());

    // FIXME: use QTestLib macros for checking the correct state
    // start editing
    buffer.startEditing();

    // end editing
    buffer.finishEditing();
}

void KateTextBufferTest::wrapLineTest()
{
    // construct an empty text buffer
    Kate::TextBuffer buffer(nullptr, 1);

    // wrap first empty line -> we should have two empty lines
    buffer.startEditing();
    buffer.wrapLine(KTextEditor::Cursor(0, 0));
    buffer.finishEditing();
    buffer.debugPrint(QLatin1String("Two empty lines"));
    QVERIFY(buffer.text() == QLatin1String("\n"));

    // unwrap again -> only one empty line
    buffer.startEditing();
    buffer.unwrapLine(1);
    buffer.finishEditing();

    // print debug
    buffer.debugPrint(QLatin1String("Empty Buffer"));
    QVERIFY(buffer.text() == QString());
}

void KateTextBufferTest::insertRemoveTextTest()
{
    // construct an empty text buffer
    Kate::TextBuffer buffer(nullptr, 1);

    // wrap first line
    buffer.startEditing();
    buffer.wrapLine(KTextEditor::Cursor(0, 0));
    buffer.finishEditing();
    buffer.debugPrint(QLatin1String("Two empty lines"));
    QVERIFY(buffer.text() == QLatin1String("\n"));

    // remember second line
    Kate::TextLine second = buffer.line(1);

    // unwrap second line
    buffer.startEditing();
    buffer.unwrapLine(1);
    buffer.finishEditing();
    buffer.debugPrint(QLatin1String("One empty line"));
    QVERIFY(buffer.text() == QString());

    // second text line should be still there
    // const QString &secondText = second->text ();
    // QVERIFY (secondText == "")

    // insert text
    buffer.startEditing();
    buffer.insertText(KTextEditor::Cursor(0, 0), QLatin1String("testremovetext"));
    buffer.finishEditing();
    buffer.debugPrint(QLatin1String("One line"));
    QVERIFY(buffer.text() == QLatin1String("testremovetext"));

    // remove text
    buffer.startEditing();
    buffer.removeText(KTextEditor::Range(KTextEditor::Cursor(0, 4), KTextEditor::Cursor(0, 10)));
    buffer.finishEditing();
    buffer.debugPrint(QLatin1String("One line"));
    QVERIFY(buffer.text() == QLatin1String("testtext"));

    // wrap text
    buffer.startEditing();
    buffer.wrapLine(KTextEditor::Cursor(0, 2));
    buffer.finishEditing();
    buffer.debugPrint(QLatin1String("Two line"));
    QVERIFY(buffer.text() == QLatin1String("te\nsttext"));

    // unwrap text
    buffer.startEditing();
    buffer.unwrapLine(1);
    buffer.finishEditing();
    buffer.debugPrint(QLatin1String("One line"));
    QVERIFY(buffer.text() == QLatin1String("testtext"));
}

void KateTextBufferTest::cursorTest()
{
    // last buffer content, for consistence checks
    QString lastBufferContent;

    // test with different block sizes
    for (int i = 1; i <= 4; ++i) {
        // construct an empty text buffer
        Kate::TextBuffer buffer(nullptr, i);

        // wrap first line
        buffer.startEditing();
        buffer.insertText(KTextEditor::Cursor(0, 0), QLatin1String("sfdfjdsklfjlsdfjlsdkfjskldfjklsdfjklsdjkfl"));
        buffer.wrapLine(KTextEditor::Cursor(0, 8));
        buffer.wrapLine(KTextEditor::Cursor(1, 8));
        buffer.wrapLine(KTextEditor::Cursor(2, 8));
        buffer.finishEditing();
        buffer.debugPrint(QLatin1String("Cursor buffer"));

        // construct cursor
        Kate::TextCursor *cursor1 = new Kate::TextCursor(buffer, KTextEditor::Cursor(0, 0), Kate::TextCursor::MoveOnInsert);
        QVERIFY(cursor1->toCursor() == KTextEditor::Cursor(0, 0));
        // printf ("cursor %d, %d\n", cursor1->line(), cursor1->column());

        // Kate::TextCursor *cursor2 = new Kate::TextCursor (buffer, KTextEditor::Cursor (1, 8), Kate::TextCursor::MoveOnInsert);
        // printf ("cursor %d, %d\n", cursor2->line(), cursor2->column());

        // Kate::TextCursor *cursor3 = new Kate::TextCursor (buffer, KTextEditor::Cursor (0, 123), Kate::TextCursor::MoveOnInsert);
        // printf ("cursor %d, %d\n", cursor3->line(), cursor3->column());

        // Kate::TextCursor *cursor4 = new Kate::TextCursor (buffer, KTextEditor::Cursor (1323, 1), Kate::TextCursor::MoveOnInsert);
        // printf ("cursor %d, %d\n", cursor4->line(), cursor4->column());

        // insert text
        buffer.startEditing();
        buffer.insertText(KTextEditor::Cursor(0, 0), QLatin1String("hallo"));
        buffer.finishEditing();
        buffer.debugPrint(QLatin1String("Cursor buffer"));

        // printf ("cursor %d, %d\n", cursor1->line(), cursor1->column());
        QVERIFY(cursor1->toCursor() == KTextEditor::Cursor(0, 5));

        // remove text
        buffer.startEditing();
        buffer.removeText(KTextEditor::Range(KTextEditor::Cursor(0, 4), KTextEditor::Cursor(0, 10)));
        buffer.finishEditing();
        buffer.debugPrint(QLatin1String("Cursor buffer"));

        // printf ("cursor %d, %d\n", cursor1->line(), cursor1->column());
        QVERIFY(cursor1->toCursor() == KTextEditor::Cursor(0, 4));

        // wrap line
        buffer.startEditing();
        buffer.wrapLine(KTextEditor::Cursor(0, 3));
        buffer.finishEditing();
        buffer.debugPrint(QLatin1String("Cursor buffer"));

        // printf ("cursor %d, %d\n", cursor1->line(), cursor1->column());
        QVERIFY(cursor1->toCursor() == KTextEditor::Cursor(1, 1));

        // unwrap line
        buffer.startEditing();
        buffer.unwrapLine(1);
        buffer.finishEditing();
        buffer.debugPrint(QLatin1String("Cursor buffer"));

        // printf ("cursor %d, %d\n", cursor1->line(), cursor1->column());
        QVERIFY(cursor1->toCursor() == KTextEditor::Cursor(0, 4));

        // verify content
        if (i > 1) {
            QVERIFY(lastBufferContent == buffer.text());
        }

        // remember content
        lastBufferContent = buffer.text();
    }
}

void KateTextBufferTest::foldingTest()
{
    // construct an empty text buffer & folding info
    Kate::TextBuffer buffer(nullptr, 1);
    Kate::TextFolding folding(buffer);

    // insert some text
    buffer.startEditing();
    for (int i = 0; i < 100; ++i) {
        buffer.insertText(KTextEditor::Cursor(i, 0), QLatin1String("1234567890"));
        if (i < 99) {
            buffer.wrapLine(KTextEditor::Cursor(i, 10));
        }
    }
    buffer.finishEditing();
    QVERIFY(buffer.lines() == 100);

    // starting with empty folding!
    folding.debugPrint(QLatin1String("Empty Folding"));
    QVERIFY(folding.debugDump() == QLatin1String("tree  - folded "));

    // check visibility
    QVERIFY(folding.isLineVisible(0));
    QVERIFY(folding.isLineVisible(99));

    // all visible
    QVERIFY(folding.visibleLines() == 100);

    // we shall be able to insert new range
    QVERIFY(folding.newFoldingRange(KTextEditor::Range(KTextEditor::Cursor(5, 0), KTextEditor::Cursor(10, 0))) == 0);

    // we shall have now exactly one range toplevel, that is not folded!
    folding.debugPrint(QLatin1String("One Toplevel Fold"));
    QVERIFY(folding.debugDump() == QLatin1String("tree [5:0  10:0] - folded "));

    // fold the range!
    QVERIFY(folding.foldRange(0));

    folding.debugPrint(QLatin1String("One Toplevel Fold - Folded"));
    QVERIFY(folding.debugDump() == QLatin1String("tree [5:0 f 10:0] - folded [5:0 f 10:0]"));

    // check visibility
    QVERIFY(folding.isLineVisible(5));
    for (int i = 6; i <= 10; ++i) {
        QVERIFY(!folding.isLineVisible(i));
    }
    QVERIFY(folding.isLineVisible(11));

    // 5 lines are hidden
    QVERIFY(folding.visibleLines() == (100 - 5));

    // check line mapping
    QVERIFY(folding.visibleLineToLine(5) == 5);
    for (int i = 6; i <= 50; ++i) {
        QVERIFY(folding.visibleLineToLine(i) == (i + 5));
    }

    // there shall be one range starting at 5
    QVector<QPair<qint64, Kate::TextFolding::FoldingRangeFlags>> forLine = folding.foldingRangesStartingOnLine(5);
    QVERIFY(forLine.size() == 1);
    QVERIFY(forLine[0].first == 0);
    QVERIFY(forLine[0].second & Kate::TextFolding::Folded);

    // we shall be able to insert new range
    QVERIFY(folding.newFoldingRange(KTextEditor::Range(KTextEditor::Cursor(20, 0), KTextEditor::Cursor(30, 0)), Kate::TextFolding::Folded) == 1);

    // we shall have now exactly two range toplevel
    folding.debugPrint(QLatin1String("Two Toplevel Folds"));
    QVERIFY(folding.debugDump() == QLatin1String("tree [5:0 f 10:0] [20:0 f 30:0] - folded [5:0 f 10:0] [20:0 f 30:0]"));

    // check visibility
    QVERIFY(folding.isLineVisible(5));
    for (int i = 6; i <= 10; ++i) {
        QVERIFY(!folding.isLineVisible(i));
    }
    QVERIFY(folding.isLineVisible(11));

    QVERIFY(folding.isLineVisible(20));
    for (int i = 21; i <= 30; ++i) {
        QVERIFY(!folding.isLineVisible(i));
    }
    QVERIFY(folding.isLineVisible(31));

    // 15 lines are hidden
    QVERIFY(folding.visibleLines() == (100 - 5 - 10));

    // check line mapping
    QVERIFY(folding.visibleLineToLine(5) == 5);
    for (int i = 6; i <= 15; ++i) {
        QVERIFY(folding.visibleLineToLine(i) == (i + 5));
    }
    for (int i = 16; i <= 50; ++i) {
        QVERIFY(folding.visibleLineToLine(i) == (i + 15));
    }

    // check line mapping
    QVERIFY(folding.lineToVisibleLine(5) == 5);
    for (int i = 11; i <= 20; ++i) {
        QVERIFY(folding.lineToVisibleLine(i) == (i - 5));
    }
    for (int i = 31; i <= 40; ++i) {
        QVERIFY(folding.lineToVisibleLine(i) == (i - 15));
    }

    // there shall be one range starting at 20
    forLine = folding.foldingRangesStartingOnLine(20);
    QVERIFY(forLine.size() == 1);
    QVERIFY(forLine[0].first == 1);
    QVERIFY(forLine[0].second & Kate::TextFolding::Folded);

    // this shall fail to be inserted, as it badly overlaps with the first range!
    QVERIFY(folding.newFoldingRange(KTextEditor::Range(KTextEditor::Cursor(6, 0), KTextEditor::Cursor(15, 0)), Kate::TextFolding::Folded) == -1);

    // this shall fail to be inserted, as it badly overlaps with the second range!
    QVERIFY(folding.newFoldingRange(KTextEditor::Range(KTextEditor::Cursor(15, 0), KTextEditor::Cursor(25, 0)), Kate::TextFolding::Folded) == -1);

    // we shall still have now exactly two range toplevel
    folding.debugPrint(QLatin1String("Still Two Toplevel Folds"));
    QVERIFY(folding.debugDump() == QLatin1String("tree [5:0 f 10:0] [20:0 f 30:0] - folded [5:0 f 10:0] [20:0 f 30:0]"));

    // still 15 lines are hidden
    QVERIFY(folding.visibleLines() == (100 - 5 - 10));

    // we shall be able to insert new range, should lead to nested folds!
    QVERIFY(folding.newFoldingRange(KTextEditor::Range(KTextEditor::Cursor(15, 0), KTextEditor::Cursor(35, 0)), Kate::TextFolding::Folded) == 2);

    // we shall have now exactly two range toplevel and one embedded fold
    folding.debugPrint(QLatin1String("Two Toplevel Folds, One Nested Fold"));
    QVERIFY(folding.debugDump() == QLatin1String("tree [5:0 f 10:0] [15:0 f [20:0 f 30:0] 35:0] - folded [5:0 f 10:0] [15:0 f 35:0]"));

    // 25 lines are hidden
    QVERIFY(folding.visibleLines() == (100 - 5 - 20));

    // check line mapping
    QVERIFY(folding.lineToVisibleLine(5) == 5);
    for (int i = 11; i <= 15; ++i) {
        QVERIFY(folding.lineToVisibleLine(i) == (i - 5));
    }

    // special case: hidden lines, should fall ack to last visible one!
    for (int i = 16; i <= 35; ++i) {
        QVERIFY(folding.lineToVisibleLine(i) == 10);
    }

    for (int i = 36; i <= 40; ++i) {
        QVERIFY(folding.lineToVisibleLine(i) == (i - 25));
    }

    // we shall be able to insert new range, should lead to nested folds!
    QVERIFY(folding.newFoldingRange(KTextEditor::Range(KTextEditor::Cursor(0, 0), KTextEditor::Cursor(50, 0)), Kate::TextFolding::Folded) == 3);

    // we shall have now exactly one range toplevel and many embedded fold
    folding.debugPrint(QLatin1String("One Toplevel + Embedded Folds"));
    QVERIFY(folding.debugDump() == QLatin1String("tree [0:0 f [5:0 f 10:0] [15:0 f [20:0 f 30:0] 35:0] 50:0] - folded [0:0 f 50:0]"));

    // there shall still be one range starting at 20
    forLine = folding.foldingRangesStartingOnLine(20);
    QVERIFY(forLine.size() == 1);
    QVERIFY(forLine[0].first == 1);
    QVERIFY(forLine[0].second & Kate::TextFolding::Folded);

    // add more regions starting at 20
    QVERIFY(folding.newFoldingRange(KTextEditor::Range(KTextEditor::Cursor(20, 5), KTextEditor::Cursor(24, 0)), Kate::TextFolding::Folded) == 4);
    QVERIFY(folding.newFoldingRange(KTextEditor::Range(KTextEditor::Cursor(20, 3), KTextEditor::Cursor(25, 0)), Kate::TextFolding::Folded) == 5);
    folding.debugPrint(QLatin1String("More ranges at 20"));

    // there shall still be three ranges starting at 20
    forLine = folding.foldingRangesStartingOnLine(20);
    QVERIFY(forLine.size() == 3);
    QVERIFY(forLine[0].first == 1);
    QVERIFY(forLine[0].second & Kate::TextFolding::Folded);
    QVERIFY(forLine[1].first == 5);
    QVERIFY(forLine[1].second & Kate::TextFolding::Folded);
    QVERIFY(forLine[2].first == 4);
    QVERIFY(forLine[2].second & Kate::TextFolding::Folded);

    // 50 lines are hidden
    QVERIFY(folding.visibleLines() == (100 - 50));

    // save state
    QJsonDocument folds = folding.exportFoldingRanges();
    QString textDump = folding.debugDump();

    // clear folds
    folding.clear();
    QVERIFY(folding.debugDump() == QLatin1String("tree  - folded "));

    // restore state
    folding.importFoldingRanges(folds);
    QVERIFY(folding.debugDump() == textDump);
}

void KateTextBufferTest::nestedFoldingTest()
{
    // construct an empty text buffer & folding info
    Kate::TextBuffer buffer(nullptr, 1);
    Kate::TextFolding folding(buffer);

    // insert two nested folds in 5 lines
    buffer.startEditing();
    for (int i = 0; i < 4; ++i) {
        buffer.wrapLine(KTextEditor::Cursor(0, 0));
    }
    buffer.finishEditing();

    QVERIFY(buffer.lines() == 5);

    // folding for line 1
    QVERIFY(folding.newFoldingRange(KTextEditor::Range(KTextEditor::Cursor(0, 0), KTextEditor::Cursor(3, 0)), Kate::TextFolding::Folded) == 0);
    QVERIFY(folding.newFoldingRange(KTextEditor::Range(KTextEditor::Cursor(1, 0), KTextEditor::Cursor(2, 0)), Kate::TextFolding::Folded) == 1);

    QVERIFY(folding.foldRange(1));
    QVERIFY(folding.foldRange(0));

    QVERIFY(folding.unfoldRange(0));
    QVERIFY(folding.unfoldRange(1));
}

void KateTextBufferTest::saveFileInUnwritableFolder()
{
    // create temp dir and get file name inside
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    const QString folder_name = dir.path();
    const QString file_path = folder_name + QLatin1String("/foo");

    QFile f(file_path);
    QVERIFY(f.open(QIODevice::WriteOnly | QIODevice::Truncate));
    f.write("1234567890");
    QVERIFY(f.flush());
    f.close();

    QFile::setPermissions(folder_name, QFile::ExeOwner);

    Kate::TextBuffer buffer(nullptr, 1);
    buffer.setTextCodec(QTextCodec::codecForName("UTF-8"));
    buffer.setFallbackTextCodec(QTextCodec::codecForName("UTF-8"));
    bool a;
    bool b;
    int c;
    buffer.load(file_path, a, b, c, true);
    buffer.clear();
    buffer.startEditing();
    buffer.insertText(KTextEditor::Cursor(0, 0), QLatin1String("ABC"));
    buffer.finishEditing();
    qDebug() << buffer.text();
    buffer.save(file_path);

    f.open(QIODevice::ReadOnly);
    QCOMPARE(f.readAll(), QByteArray("ABC"));
    f.close();

    QFile::setPermissions(folder_name, QFile::WriteOwner | QFile::ExeOwner);
    QVERIFY(f.remove());
    QVERIFY(dir.remove());
}

#if HAVE_KAUTH
void KateTextBufferTest::saveFileWithElevatedPrivileges()
{
    // create temp dir and get file name inside
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    const QString file_path = dir.path() + QLatin1String("/foo");

    QFile f(file_path);
    QVERIFY(f.open(QIODevice::WriteOnly | QIODevice::Truncate));
    f.write("1234567890");
    QVERIFY(f.flush());
    f.close();

    Kate::TextBuffer buffer(nullptr, 1, true);
    buffer.setTextCodec(QTextCodec::codecForName("UTF-8"));
    buffer.setFallbackTextCodec(QTextCodec::codecForName("UTF-8"));
    bool a;
    bool b;
    int c;
    buffer.load(file_path, a, b, c, true);
    buffer.clear();
    buffer.startEditing();
    buffer.insertText(KTextEditor::Cursor(0, 0), QLatin1String("ABC"));
    buffer.finishEditing();
    qDebug() << buffer.text();
    buffer.save(file_path);

    f.open(QIODevice::ReadOnly);
    QCOMPARE(f.readAll(), QByteArray("ABC"));
    f.close();

    QVERIFY(f.remove());
    QVERIFY(dir.remove());
}
#endif

#include "moc_katetextbuffertest.cpp"
