/*
    This file is part of the KDE libraries
    SPDX-FileCopyrightText: 2010-2018 Dominik Haumann <dhaumann@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "movingrange_test.h"
#include "moc_movingrange_test.cpp"

#include <katebuffer.h>
#include <katedocument.h>
#include <kateview.h>
#include <ktexteditor/movingrange.h>
#include <ktexteditor/movingrangefeedback.h>

#include <QStandardPaths>
#include <QTest>

using namespace KTextEditor;

QTEST_MAIN(MovingRangeTest)

class RangeFeedback : public MovingRangeFeedback
{
public:
    RangeFeedback(bool deleteOnInvalid = false)
        : MovingRangeFeedback()
        , m_deleteOnInvalidation(deleteOnInvalid)
    {
        reset();
    }

    void rangeEmpty(MovingRange * /*range*/) override
    {
        m_rangeEmptyCalled = true;
    }

    void rangeInvalid(MovingRange *range) override
    {
        if (m_deleteOnInvalidation) {
            delete range;
        }
        m_rangeInvalidCalled = true;
    }

    void mouseEnteredRange(MovingRange * /*range*/, View * /*view*/) override
    {
        m_mouseEnteredRangeCalled = true;
    }

    void mouseExitedRange(MovingRange * /*range*/, View * /*view*/) override
    {
        m_mouseExitedRangeCalled = true;
    }

    void caretEnteredRange(MovingRange * /*range*/, View * /*view*/) override
    {
        m_caretEnteredRangeCalled = true;
    }

    void caretExitedRange(MovingRange * /*range*/, View * /*view*/) override
    {
        m_caretExitedRangeCalled = true;
    }

    //
    // Test functions to reset feedback watcher
    //
public:
    void reset()
    {
        m_rangeEmptyCalled = false;
        m_rangeInvalidCalled = false;
        m_mouseEnteredRangeCalled = false;
        m_mouseExitedRangeCalled = false;
        m_caretEnteredRangeCalled = false;
        m_caretExitedRangeCalled = false;
    }

    void verifyReset()
    {
        QVERIFY(!m_rangeEmptyCalled);
        QVERIFY(!m_rangeInvalidCalled);
        QVERIFY(!m_mouseEnteredRangeCalled);
        QVERIFY(!m_mouseExitedRangeCalled);
        QVERIFY(!m_caretEnteredRangeCalled);
        QVERIFY(!m_caretExitedRangeCalled);
    }

    bool rangeEmptyCalled() const
    {
        return m_rangeEmptyCalled;
    }
    bool rangeInvalidCalled() const
    {
        return m_rangeInvalidCalled;
    }
    bool mouseEnteredRangeCalled() const
    {
        return m_mouseEnteredRangeCalled;
    }
    bool mouseExitedRangeCalled() const
    {
        return m_mouseExitedRangeCalled;
    }
    bool caretEnteredRangeCalled() const
    {
        return m_caretEnteredRangeCalled;
    }
    bool caretExitedRangeCalled() const
    {
        return m_caretExitedRangeCalled;
    }

private:
    const bool m_deleteOnInvalidation;
    bool m_rangeEmptyCalled;
    bool m_rangeInvalidCalled;
    bool m_mouseEnteredRangeCalled;
    bool m_mouseExitedRangeCalled;
    bool m_caretEnteredRangeCalled;
    bool m_caretExitedRangeCalled;
};

MovingRangeTest::MovingRangeTest()
    : QObject()
{
    QStandardPaths::setTestModeEnabled(true);
}

MovingRangeTest::~MovingRangeTest()
{
}

// tests:
// - RangeFeedback::rangeEmpty
void MovingRangeTest::testFeedbackEmptyRange()
{
    KTextEditor::DocumentPrivate doc;
    // the range created below will span the 'x' characters
    QString text(
        QStringLiteral("..xxxx\n"
                       "xxxx.."));
    doc.setText(text);

    // create range feedback
    RangeFeedback rf;

    // allow empty
    std::unique_ptr<MovingRange> range(
        doc.newMovingRange(Range(Cursor(0, 2), Cursor(1, 4)), KTextEditor::MovingRange::DoNotExpand, KTextEditor::MovingRange::AllowEmpty));
    range->setFeedback(&rf);
    rf.verifyReset();

    // remove exact range
    doc.removeText(range->toRange());
    QVERIFY(rf.rangeEmptyCalled());
    QVERIFY(!rf.rangeInvalidCalled());
    QVERIFY(!rf.mouseEnteredRangeCalled());
    QVERIFY(!rf.mouseExitedRangeCalled());
    QVERIFY(!rf.caretEnteredRangeCalled());
    QVERIFY(!rf.caretExitedRangeCalled());

    // clear document: should call rangeInvalid
    rf.reset();
    rf.verifyReset();
    doc.clear();
    QVERIFY(rf.rangeInvalidCalled());
    QVERIFY(!rf.rangeEmptyCalled());
    QVERIFY(!rf.mouseEnteredRangeCalled());
    QVERIFY(!rf.mouseExitedRangeCalled());
    QVERIFY(!rf.caretEnteredRangeCalled());
    QVERIFY(!rf.caretExitedRangeCalled());

    // setText: should behave just like clear document: call rangeInvalid again
    doc.setText(text);
    range->setRange(Range(Cursor(0, 2), Cursor(1, 4)));
    rf.reset();
    rf.verifyReset();
    doc.setText(QStringLiteral("--yyyy\nyyyy--"));
    QVERIFY(rf.rangeInvalidCalled());
    QVERIFY(!rf.rangeEmptyCalled());
    QVERIFY(!rf.mouseEnteredRangeCalled());
    QVERIFY(!rf.mouseExitedRangeCalled());
    QVERIFY(!rf.caretEnteredRangeCalled());
    QVERIFY(!rf.caretExitedRangeCalled());

    // now remove entire document range. In this case, emptyRange should be called
    // instead of rangeInvalid
    doc.setText(text);
    range->setRange(Range(Cursor(0, 2), Cursor(1, 4)));
    rf.reset();
    rf.verifyReset();
    doc.removeText(doc.documentRange());
    QVERIFY(rf.rangeEmptyCalled());
    QVERIFY(!rf.rangeInvalidCalled());
    QVERIFY(!rf.mouseEnteredRangeCalled());
    QVERIFY(!rf.mouseExitedRangeCalled());
    QVERIFY(!rf.caretEnteredRangeCalled());
    QVERIFY(!rf.caretExitedRangeCalled());
}

// tests:
// - RangeFeedback::rangeInvalid
void MovingRangeTest::testFeedbackInvalidRange()
{
    KTextEditor::DocumentPrivate doc;
    // the range created below will span the 'x' characters
    QString text(
        QStringLiteral("..xxxx\n"
                       "xxxx.."));
    doc.setText(text);

    // create range feedback
    RangeFeedback rf;

    // allow empty
    MovingRange *range =
        doc.newMovingRange(Range(Cursor(0, 2), Cursor(1, 4)), KTextEditor::MovingRange::DoNotExpand, KTextEditor::MovingRange::InvalidateIfEmpty);
    range->setFeedback(&rf);
    rf.verifyReset();

    // remove exact range
    doc.removeText(range->toRange());
    QVERIFY(!rf.rangeEmptyCalled());
    QVERIFY(rf.rangeInvalidCalled());
    QVERIFY(!rf.mouseEnteredRangeCalled());
    QVERIFY(!rf.mouseExitedRangeCalled());
    QVERIFY(!rf.caretEnteredRangeCalled());
    QVERIFY(!rf.caretExitedRangeCalled());

    // clear document: should call rangeInvalid again
    doc.setText(text);
    range->setRange(Range(Cursor(0, 2), Cursor(1, 4)));
    rf.reset();
    rf.verifyReset();
    doc.clear();
    QVERIFY(rf.rangeInvalidCalled());
    QVERIFY(!rf.rangeEmptyCalled());
    QVERIFY(!rf.mouseEnteredRangeCalled());
    QVERIFY(!rf.mouseExitedRangeCalled());
    QVERIFY(!rf.caretEnteredRangeCalled());
    QVERIFY(!rf.caretExitedRangeCalled());

    // setText: should behave just like clear document: call rangeInvalid again
    doc.setText(text);
    range->setRange(Range(Cursor(0, 2), Cursor(1, 4)));
    rf.reset();
    rf.verifyReset();
    doc.setText(QStringLiteral("--yyyy\nyyyy--"));
    QVERIFY(rf.rangeInvalidCalled());
    QVERIFY(!rf.rangeEmptyCalled());
    QVERIFY(!rf.mouseEnteredRangeCalled());
    QVERIFY(!rf.mouseExitedRangeCalled());
    QVERIFY(!rf.caretEnteredRangeCalled());
    QVERIFY(!rf.caretExitedRangeCalled());

    // now remove entire document range. Call rangeInvalid again
    doc.setText(text);
    range->setRange(Range(Cursor(0, 2), Cursor(1, 4)));
    rf.reset();
    rf.verifyReset();
    doc.removeText(doc.documentRange());
    QVERIFY(rf.rangeInvalidCalled());
    QVERIFY(!rf.rangeEmptyCalled());
    QVERIFY(!rf.mouseEnteredRangeCalled());
    QVERIFY(!rf.mouseExitedRangeCalled());
    QVERIFY(!rf.caretEnteredRangeCalled());
    QVERIFY(!rf.caretExitedRangeCalled());
    delete range;
}

// tests:
// - RangeFeedback::caretEnteredRange
// - RangeFeedback::caretExitedRange
void MovingRangeTest::testFeedbackCaret()
{
    KTextEditor::DocumentPrivate doc;
    // we only use 'x' characters here to have uniform letter sizes for cursorUp/Down movements
    QString text(
        QStringLiteral("xxxxxx\n"
                       "xxxxxx"));
    doc.setText(text);

    KTextEditor::ViewPrivate *view = static_cast<KTextEditor::ViewPrivate *>(doc.createView(nullptr));

    // create range feedback
    RangeFeedback rf;

    // first test: with ExpandLeft | ExpandRight
    {
        view->setCursorPosition(Cursor(1, 6));

        MovingRange *range = doc.newMovingRange(Range(Cursor(0, 2), Cursor(1, 4)),
                                                KTextEditor::MovingRange::ExpandLeft | KTextEditor::MovingRange::ExpandRight,
                                                KTextEditor::MovingRange::InvalidateIfEmpty);
        rf.reset();
        range->setFeedback(&rf);
        rf.verifyReset();

        // left
        view->cursorLeft();
        QCOMPARE(view->cursorPosition(), Cursor(1, 5));
        QVERIFY(!rf.caretEnteredRangeCalled());
        QVERIFY(!rf.caretExitedRangeCalled());

        view->cursorLeft();
        QCOMPARE(view->cursorPosition(), Cursor(1, 4));
        QVERIFY(rf.caretEnteredRangeCalled()); // ExpandRight: include cursor already now
        QVERIFY(!rf.caretExitedRangeCalled());

        rf.reset();
        view->cursorLeft();
        QCOMPARE(view->cursorPosition(), Cursor(1, 3));
        QVERIFY(!rf.caretEnteredRangeCalled());
        QVERIFY(!rf.caretExitedRangeCalled());

        rf.reset();
        view->up();
        QCOMPARE(view->cursorPosition(), Cursor(0, 3));
        QVERIFY(!rf.caretEnteredRangeCalled());
        QVERIFY(!rf.caretExitedRangeCalled());

        rf.reset();
        view->cursorLeft();
        QCOMPARE(view->cursorPosition(), Cursor(0, 2));
        QVERIFY(!rf.caretEnteredRangeCalled());
        QVERIFY(!rf.caretExitedRangeCalled());

        rf.reset();
        view->cursorLeft();
        QCOMPARE(view->cursorPosition(), Cursor(0, 1)); // ExpandLeft: now we left it, not before
        QVERIFY(!rf.caretEnteredRangeCalled());
        QVERIFY(rf.caretExitedRangeCalled());

        delete range;
    }

    // second test: with DoNotExpand
    {
        view->setCursorPosition(Cursor(1, 6));

        MovingRange *range =
            doc.newMovingRange(Range(Cursor(0, 2), Cursor(1, 4)), KTextEditor::MovingRange::DoNotExpand, KTextEditor::MovingRange::InvalidateIfEmpty);
        rf.reset();
        range->setFeedback(&rf);
        rf.verifyReset();

        // left
        view->cursorLeft();
        QCOMPARE(view->cursorPosition(), Cursor(1, 5));
        QVERIFY(!rf.caretEnteredRangeCalled());
        QVERIFY(!rf.caretExitedRangeCalled());

        view->cursorLeft();
        QCOMPARE(view->cursorPosition(), Cursor(1, 4));
        QVERIFY(!rf.caretEnteredRangeCalled()); // DoNotExpand: does not include cursor
        QVERIFY(!rf.caretExitedRangeCalled());

        rf.reset();
        view->cursorLeft();
        QCOMPARE(view->cursorPosition(), Cursor(1, 3));
        QVERIFY(rf.caretEnteredRangeCalled());
        QVERIFY(!rf.caretExitedRangeCalled());

        rf.reset();
        view->up();
        QCOMPARE(view->cursorPosition(), Cursor(0, 3));
        QVERIFY(!rf.caretEnteredRangeCalled());
        QVERIFY(!rf.caretExitedRangeCalled());

        rf.reset();
        view->cursorLeft();
        QCOMPARE(view->cursorPosition(), Cursor(0, 2));
        QVERIFY(!rf.caretEnteredRangeCalled());
        QVERIFY(rf.caretExitedRangeCalled()); // DoNotExpand: that's why we leave already now

        rf.reset();
        view->cursorLeft();
        QCOMPARE(view->cursorPosition(), Cursor(0, 1));
        QVERIFY(!rf.caretEnteredRangeCalled());
        QVERIFY(!rf.caretExitedRangeCalled());

        delete range;
    }
}

// tests:
// - RangeFeedback::mouseEnteredRange
// - RangeFeedback::mouseExitedRange
void MovingRangeTest::testFeedbackMouse()
{
    // mouse move only on X11
    if (qApp->platformName() != QLatin1String("xcb")) {
        QSKIP("mouse moving only on X11");
    }

    KTextEditor::DocumentPrivate doc;
    // the range created below will span the 'x' characters
    QString text(
        QStringLiteral("..xxxx\n"
                       "xxxx.."));
    doc.setText(text);

    KTextEditor::ViewPrivate *view = static_cast<KTextEditor::ViewPrivate *>(doc.createView(nullptr));
    view->setCursorPosition(Cursor(1, 6));
    view->show();
    view->resize(200, 100);

    // create range feedback
    RangeFeedback rf;
    QVERIFY(!rf.mouseEnteredRangeCalled());
    QVERIFY(!rf.mouseExitedRangeCalled());

    // allow empty
    MovingRange *range = doc.newMovingRange(Range(Cursor(0, 2), Cursor(1, 4)),
                                            KTextEditor::MovingRange::ExpandLeft | KTextEditor::MovingRange::ExpandRight,
                                            KTextEditor::MovingRange::InvalidateIfEmpty);
    range->setFeedback(&rf);
    rf.verifyReset();

    // left (nothing)
    QTest::mouseMove(view, view->cursorToCoordinate(Cursor(0, 0)) + QPoint(0, 5));
    QTest::qWait(200); // process mouse events. do not move mouse manually
    QVERIFY(!rf.mouseEnteredRangeCalled());
    QVERIFY(!rf.mouseExitedRangeCalled());

    // middle (enter)
    rf.reset();
    QTest::mouseMove(view, view->cursorToCoordinate(Cursor(0, 3)) + QPoint(0, 5));
    QTest::qWait(200); // process mouse events. do not move mouse manually
    QVERIFY(rf.mouseEnteredRangeCalled());
    QVERIFY(!rf.mouseExitedRangeCalled());

    // right (exit)
    rf.reset();
    QTest::mouseMove(view, view->cursorToCoordinate(Cursor(1, 6)) + QPoint(10, 5));
    QTest::qWait(200); // process mouse events. do not move mouse manually
    QVERIFY(!rf.mouseEnteredRangeCalled());
    QVERIFY(rf.mouseExitedRangeCalled());
    delete range;
}

void MovingRangeTest::testLineRemoved()
{
    KTextEditor::DocumentPrivate doc;
    // the range created below will span the 'x' characters
    QString text(
        QStringLiteral("abcd\n"
                       "efgh\n"
                       "\n"
                       "hijk"));
    doc.setText(text);

    KTextEditor::ViewPrivate *view = static_cast<KTextEditor::ViewPrivate *>(doc.createView(nullptr));
    view->setCursorPosition(Cursor(1, 3));
    view->show();
    view->resize(200, 100);
    constexpr auto expand = KTextEditor::MovingRange::ExpandLeft | KTextEditor::MovingRange::ExpandRight;
    MovingRange *range = doc.newMovingRange({1, 1, 1, 2}, expand, KTextEditor::MovingRange::InvalidateIfEmpty);
    MovingRange *range2 = doc.newMovingRange({1, 3, 1, 4}, expand, KTextEditor::MovingRange::InvalidateIfEmpty);
    KTextEditor::Attribute::Ptr attr(new KTextEditor::Attribute);
    attr->setForeground(Qt::red);
    range->setAttribute(attr);
    doc.removeLine(1);
    delete range;
    delete range2;

    // shouldn't crash
    auto r = doc.buffer().rangesForLine(1, view, true);
    QVERIFY(r.isEmpty());
}

void MovingRangeTest::testLineWrapOrUnwrapUpdateRangeForLineCache()
{
    KTextEditor::DocumentPrivate doc;
    doc.setText(
        QStringLiteral("abcd\n"
                       "efgh\n"
                       "hijk\n"));

    // add range to line 2, it shall be in rangeForLine for the right lines after each update!
    // must be single line range to be in the cache!
    auto range = doc.newMovingRange({2, 1, 2, 3},
                                    KTextEditor::MovingRange::ExpandLeft | KTextEditor::MovingRange::ExpandRight,
                                    KTextEditor::MovingRange::InvalidateIfEmpty);

    // range shall be in the lookup cache for line 2
    QVERIFY(doc.buffer().rangesForLine(0, nullptr, false).isEmpty());
    QVERIFY(doc.buffer().rangesForLine(1, nullptr, false).isEmpty());
    QVERIFY(doc.buffer().rangesForLine(2, nullptr, false).contains(range));

    // wrap line 1 => range should move to line 3
    doc.editWrapLine(1, 1);
    QVERIFY(doc.buffer().rangesForLine(0, nullptr, false).isEmpty());
    QVERIFY(doc.buffer().rangesForLine(1, nullptr, false).isEmpty());
    QVERIFY(doc.buffer().rangesForLine(2, nullptr, false).isEmpty());
    QVERIFY(doc.buffer().rangesForLine(3, nullptr, false).contains(range));

    // unwrap line 1 => range should back move to line 2
    doc.editUnWrapLine(1);
    QVERIFY(doc.buffer().rangesForLine(0, nullptr, false).isEmpty());
    QVERIFY(doc.buffer().rangesForLine(1, nullptr, false).isEmpty());
    QVERIFY(doc.buffer().rangesForLine(2, nullptr, false).contains(range));
}

void MovingRangeTest::testMultiline()
{
    KTextEditor::DocumentPrivate doc;
    doc.setText(
        QStringLiteral("abcd\n"
                       "efgh\n"
                       "hijk\n"));

    // add range to line 1-2
    auto range = doc.newMovingRange({1, 0, 2, 3},
                                    KTextEditor::MovingRange::ExpandLeft | KTextEditor::MovingRange::ExpandRight,
                                    KTextEditor::MovingRange::InvalidateIfEmpty);
    QVERIFY(doc.buffer().rangesForLine(1, nullptr, false).contains(range));
    QVERIFY(doc.buffer().rangesForLine(2, nullptr, false).contains(range));
}

void MovingRangeTest::testMultiblock()
{
    KTextEditor::DocumentPrivate doc;
    // add enough text so that we have at least 3 blocks
    QStringList text(200, QStringLiteral("asdf"));
    doc.setText(text);

    auto range = doc.newMovingRange({1, 0, 170, 3},
                                    KTextEditor::MovingRange::ExpandLeft | KTextEditor::MovingRange::ExpandRight,
                                    KTextEditor::MovingRange::InvalidateIfEmpty);

    QVERIFY(doc.buffer().hasMultlineRange(range));
    // check that the range is returned for each line it contains
    for (int i = range->toLineRange().start(); i <= range->toLineRange().end(); ++i) {
        QVERIFY(doc.buffer().rangesForLine(i, nullptr, false).contains(range));
    }

    // invalidate and check
    range->setRange(KTextEditor::Range::invalid());
    QVERIFY(!doc.buffer().hasMultlineRange(range));
    for (auto i : {10, 50, 100, 150}) {
        QVERIFY(!doc.buffer().rangesForLine(i, nullptr, false).contains(range));
    }

    // check that the range is returned for each line it contains after setRange
    range->setRange({1, 0, 170, 3});
    QVERIFY(doc.buffer().hasMultlineRange(range));
    for (int i = range->toLineRange().start(); i <= range->toLineRange().end(); ++i) {
        QVERIFY(doc.buffer().rangesForLine(i, nullptr, false).contains(range));
    }

    // delete the range and check
    delete range;
    QVERIFY(!doc.buffer().hasMultlineRange(range));
    for (auto i : {10, 50, 100, 150}) {
        QVERIFY(!doc.buffer().rangesForLine(i, nullptr, false).contains(range));
    }

    // check that range becomes multi block on split block
    range = doc.newMovingRange({197, 0, 199, 3},
                               KTextEditor::MovingRange::ExpandLeft | KTextEditor::MovingRange::ExpandRight,
                               KTextEditor::MovingRange::InvalidateIfEmpty);

    // add enough lines to trigger a splitBlock
    doc.editStart();
    doc.insertLines(198, QStringList(128, QStringLiteral("asdfg")));
    doc.editEnd();

    QVERIFY(doc.buffer().hasMultlineRange(range));
    QCOMPARE(range->toRange(), KTextEditor::Range(197, 0, 327, 3));
    for (int i = range->toLineRange().start(); i <= range->toLineRange().end(); ++i) {
        QVERIFY(doc.buffer().rangesForLine(i, nullptr, false).contains(range));
    }

    doc.editStart();
    doc.removeText({200, 0, 299, 5});
    doc.editEnd();
    QVERIFY(doc.buffer().rangesForLine(198, nullptr, false).contains(range));
    QVERIFY(!doc.buffer().hasMultlineRange(range));

    delete range;
}

void MovingRangeTest::benchCursorsInsertionRemoval()
{
    constexpr int NUM_LINES = 10000;

    KTextEditor::DocumentPrivate doc;
    const QStringList lines(NUM_LINES, QStringLiteral("This is a very long line with some random text"));
    doc.setText(lines);
    QCOMPARE(doc.lines(), NUM_LINES);

    QBENCHMARK {
        std::vector<KTextEditor::MovingRange *> ranges;

        for (int i = 0; i < NUM_LINES; ++i) {
            auto r = doc.newMovingRange({i, 2, i, 2 + 4});
            auto r1 = doc.newMovingRange({i, 2 + 5, i, 2 + 5 + 4});
            ranges.push_back(r);
            ranges.push_back(r1);
        }

        // remove them
        qDeleteAll(ranges);
    }
}

void MovingRangeTest::benchCheckValidity_data()
{
    QTest::addColumn<MovingRange::EmptyBehavior>("emptyBehaviour");

    QTest::addRow("AllowEmpty") << MovingRange::AllowEmpty;
    QTest::addRow("InvalidateIfEmpty") << MovingRange::InvalidateIfEmpty;
}

void MovingRangeTest::benchCheckValidity()
{
    /** NOTE:
     * Atm this test runs very slow because of the way ranges are handled
     * when removing text. When removing text, we move the ranges up into the previous
     * line if needed. And since we are removing the text the ranges become empty. So far
     * that is okay. The problem is the ranges start accumulating into the previous textblock
     * and if we remove a lot of text, we accumulate a lot of ranges meaning we do a lot of
     * checkValidity on TextRanges that were emptied long ago! This can probably be optimized by
     * skipping checkValidity on ranges that were already empty.
     *
     * Also, note that this issue disappears if EmptyBehaviour is InvalidateIfEmpty
     *
     * See TextBlock::unwrapLine
     */
    QFETCH(MovingRange::EmptyBehavior, emptyBehaviour);

    // use a larger number to see the differenc between the two empty behaviours
    constexpr int NUM_LINES = 100;

    KTextEditor::DocumentPrivate doc;
    const QStringList lines(NUM_LINES, QStringLiteral("This is a very long line with some random text"));
    doc.setText(lines);
    QCOMPARE(doc.lines(), NUM_LINES);

    // create NUM_LINES * 2 ranges (and NUM_LINES * 4 cursors)
    std::vector<KTextEditor::MovingRange *> ranges;
    for (int i = 0; i < NUM_LINES; ++i) {
        auto r = doc.newMovingRange({i, 2, i, 2 + 4}, MovingRange::DoNotExpand, emptyBehaviour);
        auto r1 = doc.newMovingRange({i, 2 + 5, i, 2 + 5 + 4}, MovingRange::DoNotExpand, emptyBehaviour);
        ranges.push_back(r);
        ranges.push_back(r1);
    }

    // trigger text removal, we will be iterating the cursors of every block for each line
    QBENCHMARK {
        doc.removeText(doc.documentRange());
    }

    // remove them
    qDeleteAll(ranges);
}

void MovingRangeTest::benchRangeForLine()
{
    constexpr int NUM_LINES = 100;

    KTextEditor::DocumentPrivate doc;
    const QStringList lines(NUM_LINES, QStringLiteral("This is a very long line with some random text"));
    doc.setText(lines);
    QCOMPARE(doc.lines(), NUM_LINES);

    // create NUM_LINES * 2 ranges (and NUM_LINES * 4 cursors)
    std::vector<KTextEditor::MovingRange *> ranges;
    for (int i = 0; i < NUM_LINES; ++i) {
        auto r = doc.newMovingRange({i, 2, i, 2 + 4});
        auto r1 = doc.newMovingRange({i, 2 + 5, i, 2 + 5 + 4});
        ranges.push_back(r);
        ranges.push_back(r1);
    }

    QBENCHMARK {
        for (int i = 0; i < NUM_LINES; ++i) {
            doc.buffer().rangesForLine(i, nullptr, false);
        }
    }

    qDeleteAll(ranges);
}

void MovingRangeTest::testMultiblockRangeWithLineUnwrapping()
{
    KTextEditor::DocumentPrivate doc;
    const QStringList lines(130, QStringLiteral("text"));
    doc.setText(lines);
    QCOMPARE(doc.lines(), 130);

    auto range = doc.newMovingRange(doc.documentRange());
    while (true) {
        auto rs = doc.searchText(range->toRange(), QStringLiteral("\n"), KTextEditor::SearchOption::EscapeSequences);
        if (rs.empty() || !rs.first().isValid()) {
            break;
        }
        doc.replaceText(rs.first(), QString());
        auto x = range->toRange();
        x.setStart(rs.first().start());
        range->setRange(x);
    }

    delete range;
    doc.buffer().rangesForLine(0, nullptr, false);
}

void MovingRangeTest::testRangeSurvivesDocument()
{
    KTextEditor::MovingRange *range;
    {
        KTextEditor::DocumentPrivate doc;
        doc.setText(QStringLiteral("abc"));
        range = doc.newMovingRange({0, 0, 0, 2});
        QVERIFY(range);
    }
    // ensure range is invalid
    QCOMPARE(range->toRange(), KTextEditor::Range::invalid());
    QVERIFY(!range->toLineRange().isValid());
    QVERIFY(!range->document());
    // try to modify this range, shouldn't crash
    range->setRange({1, 2, 3, 4});
    range->setAttribute(KTextEditor::Attribute::Ptr(new KTextEditor::Attribute));
    RangeFeedback rf;
    range->setFeedback(&rf);
    range->setZDepth(1);
    // range remains invalid as there is no document its bound to
    QCOMPARE(range->toRange(), KTextEditor::Range(-1, -1, -1, -1));
    QVERIFY(!range->attribute());
    QVERIFY(!range->feedback());
    delete range;
}

void MovingRangeTest::testRangeWithDynAttrNoCrash()
{
    KTextEditor::DocumentPrivate doc;
    doc.setText(QStringLiteral("abc\ndef\nghi"));
    KTextEditor::ViewPrivate *view = static_cast<KTextEditor::ViewPrivate *>(doc.createView(nullptr));
    std::unique_ptr<KTextEditor::MovingRange> range(doc.newMovingRange({0, 0, 0, 2}, MovingRange::DoNotExpand));
    auto attr = KTextEditor::Attribute::Ptr(new KTextEditor::Attribute());
    auto dattr = KTextEditor::Attribute::Ptr(new KTextEditor::Attribute());
    attr->setDynamicAttribute(KTextEditor::Attribute::ActivateCaretIn, dattr);
    range->setAttribute(attr);

    view->setCursorPosition({0, 0});
    // cursor moves
    view->cursorRight();
    // range is deleted for some reason
    range.reset();
    // cursor moves
    view->cursorRight();
    // no crashes
}

void MovingRangeTest::testNoCrashIfFeedbackWasClearedBeforeDtor()
{
    KTextEditor::DocumentPrivate doc;
    doc.setText(QStringLiteral("abc\ndef\nghi"));
    KTextEditor::ViewPrivate *view = static_cast<KTextEditor::ViewPrivate *>(doc.createView(nullptr));
    std::unique_ptr<KTextEditor::MovingRange> range(doc.newMovingRange({0, 0, 0, 2}, MovingRange::DoNotExpand));
    RangeFeedback rf;
    range->setFeedback(&rf);

    view->setCursorPosition({0, 0});
    // cursor moves
    view->cursorRight();
    // feedback is cleared
    range->setFeedback(nullptr);
    range.reset();
    // cursor moves
    view->cursorRight();
    // no crash
}

void MovingRangeTest::testNoCrashIfDynAttrWasClearedBeforeDtor()
{
    KTextEditor::DocumentPrivate doc;
    doc.setText(QStringLiteral("abc\ndef\nghi"));
    KTextEditor::ViewPrivate *view = static_cast<KTextEditor::ViewPrivate *>(doc.createView(nullptr));
    std::unique_ptr<KTextEditor::MovingRange> range(doc.newMovingRange({0, 0, 0, 2}, MovingRange::DoNotExpand));
    auto attr = KTextEditor::Attribute::Ptr(new KTextEditor::Attribute());
    auto dattr = KTextEditor::Attribute::Ptr(new KTextEditor::Attribute());
    attr->setDynamicAttribute(KTextEditor::Attribute::ActivateCaretIn, dattr);
    range->setAttribute(attr);

    view->setCursorPosition({0, 0});
    // cursor moves
    view->cursorRight();
    // attr is cleared
    range->setAttribute({});
    range.reset();
    // cursor moves
    view->cursorRight();
    // no crash
}

void MovingRangeTest::testNoCrashWithMultiblockRange()
{
    KTextEditor::DocumentPrivate doc;
    KTextEditor::ViewPrivate *view = static_cast<KTextEditor::ViewPrivate *>(doc.createView(nullptr));
    const QStringList lines(130, QStringLiteral("text"));
    doc.setText(lines);
    QCOMPARE(doc.lines(), 130);
    // clang-format off
    std::unique_ptr<KTextEditor::MovingRange> range(doc.newMovingRange({
        63, 1, // in block 0 last line
        64, 2 // in block 1 first line
    }));
    // expect that its multiline
    QVERIFY(doc.buffer().hasMultlineRange(range.get()));
    // clang-format on
    // place the cursor on col 0 of first line block-1
    view->setCursorPosition({64, 0});
    // trigger a line unwrap
    view->backspace();
    // expect that its no longer multiline
    QVERIFY(!doc.buffer().hasMultlineRange(range.get()));

    // delete the range
    range.reset();
    // move the cursor, we shouldnt crash
    view->cursorLeft();
}
