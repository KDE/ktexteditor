/*
    This file is part of the Kate project.

    SPDX-FileCopyrightText: 2021 Waqar Ahmed <waqar.17a@gmail.com>
    SPDX-License-Identifier: LGPL-2.0-or-later
*/
#include "katetextblocktest.h"

#include <katetextblock.h>
#include <katetextbuffer.h>
#include <katetextline.h>
#include <katetextrange.h>

QTEST_MAIN(KateTextBlockTest)

using namespace Kate;

KateTextBlockTest::KateTextBlockTest(QObject *parent)
    : QObject(parent)
{
}

void KateTextBlockTest::basicTest()
{
    TextBuffer buf(nullptr);
    TextBlock block(&buf, 0);

    block.appendLine(QStringLiteral("First line"));
    QVERIFY(block.lines() == 1);

    block.appendLine(QStringLiteral("Second line"));
    QVERIFY(block.lines() == 2);
    QVERIFY(block.startLine() == 0);
    QVERIFY(block.line(block.startLine())->text() == QStringLiteral("First line"));
    QVERIFY(block.line(block.startLine() + 1)->text() == QStringLiteral("Second line"));

    QString text;
    block.text(text);
    QVERIFY(text == QStringLiteral("First line\nSecond line"));

    block.clearLines();
    QVERIFY(block.lines() == 0);
    text.clear();
    block.text(text);
    QVERIFY(text.isEmpty());
}

void KateTextBlockTest::testWrap()
{
    TextBuffer buf(nullptr);
    TextBlock block(&buf, 0);

    block.appendLine(QStringLiteral("First line"));
    QVERIFY(block.lines() == 1);

    int blockIdx = 0;
    block.wrapLine(KTextEditor::Cursor(0, 5), blockIdx);
    QString text;
    block.text(text);
    QVERIFY(text == QStringLiteral("First\n line"));
    text.clear();

    block.unwrapLine(1, nullptr, blockIdx);
    block.text(text);
    QVERIFY(text == QStringLiteral("First line"));
    text.clear();

    block.clearLines();
}

void KateTextBlockTest::testInsertRemoveText()
{
    TextBuffer buf(nullptr);
    TextBlock block(&buf, 0);

    block.appendLine(QStringLiteral("First line"));
    QVERIFY(block.lines() == 1);

    block.insertText(KTextEditor::Cursor(0, 1), QStringLiteral(" "));
    QString text;
    block.text(text);
    QVERIFY(text == QStringLiteral("F irst line"));
    text.clear();

    block.removeText(KTextEditor::Range(0, 1, 0, 2), text);
    QVERIFY(text == QStringLiteral(" "));
    text.clear();
    block.text(text);
    QVERIFY(text == QStringLiteral("First line"));

    block.clearLines();
}

void KateTextBlockTest::testSplitMergeBlocks()
{
    TextBuffer buf(nullptr);
    TextBlock block(&buf, 0);

    block.appendLine(QStringLiteral("First line"));
    QVERIFY(block.lines() == 1);
    block.appendLine(QStringLiteral("Second line"));
    QVERIFY(block.lines() == 2);

    // split
    auto newBlock = block.splitBlock(1);
    QVERIFY(newBlock);

    QString text;
    newBlock->text(text);
    QVERIFY(text == QStringLiteral("\nSecond line"));
    text.clear();
    block.text(text);
    QVERIFY(text == QStringLiteral("First line"));
    text.clear();

    // merge
    block.mergeBlock(newBlock);
    newBlock->text(text);
    QVERIFY(text == QStringLiteral("\nSecond line\nFirst line"));
    text.clear();

    newBlock->clearLines();
    delete newBlock;
    block.clearLines();
}

void KateTextBlockTest::testTextRanges()
{
    TextBuffer buf(nullptr);
    TextBlock block(&buf, 0);

    block.appendLine(QStringLiteral("First line"));
    QVERIFY(block.lines() == 1);

    // range over char 'i' in "First"
    Kate::TextRange range(buf, KTextEditor::Range(0, 1, 0, 2), KTextEditor::MovingRange::InsertBehavior::ExpandRight);

    // update
    block.updateRange(&range);

    QVERIFY(block.containsRange(&range));

    auto ranges = block.rangesForLine(0, nullptr, false);
    QVERIFY(ranges.size() == 1);
    QVERIFY((*ranges.first()).start() == range.start());
    QVERIFY((*ranges.first()).end() == range.end());

    // try getting ranges with attributes
    QVERIFY(block.rangesForLine(0, nullptr, true).isEmpty());

    // remove
    block.removeRange(&range);
    QVERIFY(block.cachedRangesForLine(0).isEmpty());

    block.clearLines();
}
