/*
   SPDX-FileCopyrightText: 2022 Martin Seher <martin.seher@gmail.com>

   SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "hlsearch.h"
#include "emulatedcommandbarsetupandteardown.h"

#include <inputmode/kateviinputmode.h>
#include <katebuffer.h>
#include <kateconfig.h>
#include <katedocument.h>
#include <kateview.h>
#include <vimode/emulatedcommandbar/emulatedcommandbar.h>

using namespace KTextEditor;

#define TestHighlight(...) TestHighlight_(__LINE__, __FILE__, __VA_ARGS__)

QTEST_MAIN(HlSearchTest)

void HlSearchTest::highlightModeTests()
{
    EmulatedCommandBarSetUpAndTearDown emulatedCommandBarSetUpAndTearDown(vi_input_mode, kate_view, mainWindow);

    setWindowSize();

    const QVector<Kate::TextRange *> rangesInitial = rangesOnLine(0);
    Q_ASSERT(rangesInitial.isEmpty() && "Assumptions about ranges are wrong - this test is invalid and may need updating!");

    const QColor searchHighlightColor = kate_view->renderer()->config()->searchHighlightColor();

    // test commands exist
    {
        QCOMPARE(vi_input_mode->viModeEmulatedCommandBar()->executeCommand("set-hls").size(), 0);
        QCOMPARE(vi_input_mode->viModeEmulatedCommandBar()->executeCommand("set-hlsearch").size(), 0);
        QCOMPARE(vi_input_mode->viModeEmulatedCommandBar()->executeCommand("set-nohls").size(), 0);
        QCOMPARE(vi_input_mode->viModeEmulatedCommandBar()->executeCommand("set-nohlsearch").size(), 0);
        QCOMPARE(vi_input_mode->viModeEmulatedCommandBar()->executeCommand("noh").size(), 0);
        QCOMPARE(vi_input_mode->viModeEmulatedCommandBar()->executeCommand("nohlsearch").size(), 0);
    }
    // test highlight initiated by *
    {
        QString text = "foo bar xyz foo ab bar x";
        BeginTest(text);
        auto vr = kate_view->visibleRange();
        QCOMPARE(vr.end().column(), text.length());

        TestPressKey("w*");
        {
            QVector<Kate::TextRange *> ranges = rangesOnLine(0);
            QCOMPARE(ranges.size(), rangesInitial.size() + 2);

            TestHighlight(*ranges[0], {0, 4}, {0, 7}, searchHighlightColor);
            TestHighlight(*ranges[1], {0, 19}, {0, 22}, searchHighlightColor);
        }
        FinishTest(text);
    }
    // test highlight initiated by #
    {
        QString text = "foo bar xyz foo ab bar x";
        BeginTest(text);
        auto vr = kate_view->visibleRange();
        QCOMPARE(vr.end().column(), text.length());

        TestPressKey("w#");
        {
            QVector<Kate::TextRange *> ranges = rangesOnLine(0);
            QCOMPARE(ranges.size(), rangesInitial.size() + 2);

            TestHighlight(*ranges[0], {0, 4}, {0, 7}, searchHighlightColor);
            TestHighlight(*ranges[1], {0, 19}, {0, 22}, searchHighlightColor);
        }
        FinishTest(text);
    }
    // test highlight initiated by /
    {
        QString text = "foo bar xyz foo ab bar x";
        BeginTest(text);
        auto vr = kate_view->visibleRange();
        QCOMPARE(vr.end().column(), text.length());

        TestPressKey("/bar\\enter");
        {
            QVector<Kate::TextRange *> ranges = rangesOnLine(0);
            QCOMPARE(ranges.size(), rangesInitial.size() + 2);

            TestHighlight(*ranges[0], {0, 4}, {0, 7}, searchHighlightColor);
            TestHighlight(*ranges[1], {0, 19}, {0, 22}, searchHighlightColor);
        }
        FinishTest(text);
    }
    // test highlight initiated by ?
    {
        QString text = "foo bar xyz foo ab bar x";
        BeginTest(text);
        auto vr = kate_view->visibleRange();
        QCOMPARE(vr.end().column(), text.length());

        TestPressKey("?bar\\enter");
        {
            QVector<Kate::TextRange *> ranges = rangesOnLine(0);
            QCOMPARE(ranges.size(), rangesInitial.size() + 2);

            TestHighlight(*ranges[0], {0, 4}, {0, 7}, searchHighlightColor);
            TestHighlight(*ranges[1], {0, 19}, {0, 22}, searchHighlightColor);
        }
        FinishTest(text);
    }
    // test that aborting search removes highlights
    {
        QString text = "foo bar xyz foo ab bar x";
        BeginTest(text);
        auto vr = kate_view->visibleRange();
        QCOMPARE(vr.end().column(), text.length());

        TestPressKey("/bar");
        {
            QVector<Kate::TextRange *> ranges = rangesOnLine(0);
            QCOMPARE(ranges.size(), rangesInitial.size() + 2);

            TestHighlight(*ranges[0], {0, 4}, {0, 7}, searchHighlightColor);
            TestHighlight(*ranges[1], {0, 19}, {0, 22}, searchHighlightColor);
        }
        TestPressKey("\\esc");
        {
            QVector<Kate::TextRange *> ranges = rangesOnLine(0);
            QCOMPARE(ranges.size(), rangesInitial.size());
        }
        FinishTest(text);
    }
    // test empty matches handled
    {
        QString text = "foo bar xyz";
        BeginTest(text);
        auto vr = kate_view->visibleRange();
        QCOMPARE(vr.end().column(), text.length());

        TestPressKey("/\\\\<\\enter");
        {
            QVector<Kate::TextRange *> ranges = rangesOnLine(0);
            QCOMPARE(ranges.size(), rangesInitial.size() + 6);

            TestHighlight(*ranges[0], {0, 0}, {0, 1}, searchHighlightColor);
            TestHighlight(*ranges[3], {0, 7}, {0, 8}, searchHighlightColor);
        }
        FinishTest(text);
    }
    // test that only visible matches are highlighted
    {
        QString text = "foo bar xyz\n\n\n\n\nfoo ab bar x";
        BeginTest(text);
        auto vr = kate_view->visibleRange();
        // ensure that last line is not visible
        QVERIFY(vr.end().line() < 4);

        TestPressKey("/bar\\enter");
        {
            QVector<Kate::TextRange *> ranges = rangesOnLine(0);
            QCOMPARE(ranges.size(), rangesInitial.size() + 1);
            TestHighlight(*ranges[0], {0, 4}, {0, 7}, searchHighlightColor);
            ranges = rangesOnLine(5);
            QCOMPARE(ranges.size(), rangesInitial.size());
        }

        kate_view->bottom();
        {
            QVector<Kate::TextRange *> ranges = rangesOnLine(0);
            QCOMPARE(ranges.size(), rangesInitial.size());
            ranges = rangesOnLine(5);
            QCOMPARE(ranges.size(), rangesInitial.size() + 1);
            TestHighlight(*ranges[0], {5, 7}, {5, 10}, searchHighlightColor);
        }
        FinishTest(text);
    }
    // test highlighting when typing in search triggers a visual range change
    {
        QString text = "foo bar xyz\n\n\n\n\nfoo ab barx";
        BeginTest(text);
        auto vr = kate_view->visibleRange();
        // ensure that last line is not visible
        QVERIFY(vr.end().line() < 4);

        TestPressKey("/barx");
        {
            QVector<Kate::TextRange *> ranges = rangesOnLine(0);
            QCOMPARE(ranges.size(), rangesInitial.size());
            ranges = rangesOnLine(5);
            QCOMPARE(ranges.size(), rangesInitial.size() + 1);
            TestHighlight(*ranges[0], {5, 7}, {5, 11}, searchHighlightColor);
        }
        TestPressKey("\\enter");

        FinishTest(text);
    }
    // test that normal search highlight is deactivated when hls mode is active
    {
        QString text = "foo bar xyz";
        BeginTest(text);

        TestPressKey("/bar");
        {
            QVector<Kate::TextRange *> ranges = rangesOnLine(0);
            QCOMPARE(ranges.size(), rangesInitial.size() + 1);
            TestHighlight(*ranges[0], {0, 4}, {0, 7}, searchHighlightColor);
        }
        TestPressKey("\\enter");
        FinishTest(text);
    }
    // test that :noh turns of current highlight
    {
        QString text = "foo bar xyz\n\n\n\n\nfoo ab bar x";
        BeginTest(text);
        auto vr = kate_view->visibleRange();
        // ensure that last line is not visible
        QVERIFY(vr.end().line() < 4);

        TestPressKey("/bar\\enter");
        {
            QVector<Kate::TextRange *> ranges = rangesOnLine(0);
            QCOMPARE(ranges.size(), rangesInitial.size() + 1);
            TestHighlight(*ranges[0], {0, 4}, {0, 7}, searchHighlightColor);
        }

        TestPressKey(":noh\\enter");
        {
            QVector<Kate::TextRange *> ranges = rangesOnLine(0);
            QCOMPARE(ranges.size(), rangesInitial.size());
        }
        // changing view range should not activate highlighting again
        kate_view->bottom();
        {
            QVector<Kate::TextRange *> ranges = rangesOnLine(5);
            QCOMPARE(ranges.size(), rangesInitial.size());
        }
        FinishTest(text);
    }
    // test that :nohlsearch turns of current highlight
    {
        QString text = "foo bar xyz\n\n\n\n\nfoo ab bar x";
        BeginTest(text);
        auto vr = kate_view->visibleRange();
        // ensure that last line is not visible
        QVERIFY(vr.end().line() < 4);

        TestPressKey("/bar\\enter");
        {
            QVector<Kate::TextRange *> ranges = rangesOnLine(0);
            QCOMPARE(ranges.size(), rangesInitial.size() + 1);
            TestHighlight(*ranges[0], {0, 4}, {0, 7}, searchHighlightColor);
        }

        TestPressKey(":nohlsearch\\enter");
        {
            QVector<Kate::TextRange *> ranges = rangesOnLine(0);
            QCOMPARE(ranges.size(), rangesInitial.size());
        }
        // changing view range should not activate highlighting again
        kate_view->bottom();
        {
            QVector<Kate::TextRange *> ranges = rangesOnLine(5);
            QCOMPARE(ranges.size(), rangesInitial.size());
        }
        FinishTest(text);
    }
    // test that new search activates highlighting after :noh
    {
        QString text = "foo bar xyz foo";
        BeginTest(text);

        TestPressKey("/bar\\enter");
        {
            QVector<Kate::TextRange *> ranges = rangesOnLine(0);
            QCOMPARE(ranges.size(), rangesInitial.size() + 1);
            TestHighlight(*ranges[0], {0, 4}, {0, 7}, searchHighlightColor);
        }

        TestPressKey(":noh\\enter");
        {
            QVector<Kate::TextRange *> ranges = rangesOnLine(0);
            QCOMPARE(ranges.size(), rangesInitial.size());
        }
        TestPressKey("/bar\\enter");
        {
            QVector<Kate::TextRange *> ranges = rangesOnLine(0);
            QCOMPARE(ranges.size(), rangesInitial.size() + 1);
            TestHighlight(*ranges[0], {0, 4}, {0, 7}, searchHighlightColor);
        }
        FinishTest(text);
    }
    // test that word search activates highlighting after :noh
    {
        QString text = "foo bar xyz foo";
        BeginTest(text);

        TestPressKey("/bar\\enter");
        {
            QVector<Kate::TextRange *> ranges = rangesOnLine(0);
            QCOMPARE(ranges.size(), rangesInitial.size() + 1);
            TestHighlight(*ranges[0], {0, 4}, {0, 7}, searchHighlightColor);
        }

        TestPressKey(":noh\\enter");
        {
            QVector<Kate::TextRange *> ranges = rangesOnLine(0);
            QCOMPARE(ranges.size(), rangesInitial.size());
        }
        TestPressKey("*");
        {
            QVector<Kate::TextRange *> ranges = rangesOnLine(0);
            QCOMPARE(ranges.size(), rangesInitial.size() + 1);
            TestHighlight(*ranges[0], {0, 4}, {0, 7}, searchHighlightColor);
        }
        FinishTest(text);
    }
    // test that next match ('n') activates highlighting after :noh
    {
        QString text = "foo bar xyz foo";
        BeginTest(text);

        TestPressKey("/bar\\enter");
        {
            QVector<Kate::TextRange *> ranges = rangesOnLine(0);
            QCOMPARE(ranges.size(), rangesInitial.size() + 1);
            TestHighlight(*ranges[0], {0, 4}, {0, 7}, searchHighlightColor);
        }

        TestPressKey(":noh\\enter");
        {
            QVector<Kate::TextRange *> ranges = rangesOnLine(0);
            QCOMPARE(ranges.size(), rangesInitial.size());
        }
        TestPressKey("n");
        {
            QVector<Kate::TextRange *> ranges = rangesOnLine(0);
            QCOMPARE(ranges.size(), rangesInitial.size() + 1);
            TestHighlight(*ranges[0], {0, 4}, {0, 7}, searchHighlightColor);
        }
        FinishTest(text);
    }
    // test that previous match ('N') activates highlighting after :noh
    {
        QString text = "foo bar xyz foo";
        BeginTest(text);

        TestPressKey("/bar\\enter");
        {
            QVector<Kate::TextRange *> ranges = rangesOnLine(0);
            QCOMPARE(ranges.size(), rangesInitial.size() + 1);
            TestHighlight(*ranges[0], {0, 4}, {0, 7}, searchHighlightColor);
        }

        TestPressKey(":noh\\enter");
        {
            QVector<Kate::TextRange *> ranges = rangesOnLine(0);
            QCOMPARE(ranges.size(), rangesInitial.size());
        }
        TestPressKey("N");
        {
            QVector<Kate::TextRange *> ranges = rangesOnLine(0);
            QCOMPARE(ranges.size(), rangesInitial.size() + 1);
            TestHighlight(*ranges[0], {0, 4}, {0, 7}, searchHighlightColor);
        }
        FinishTest(text);
    }
    // test that :set-nohls turns of highlight
    {
        QString text = "foo bar xyz\n\n\n\n\nfoo ab bar x";
        BeginTest(text);
        auto vr = kate_view->visibleRange();
        // ensure that last line is not visible
        QVERIFY(vr.end().line() < 4);

        TestPressKey("/bar\\enter");
        {
            QVector<Kate::TextRange *> ranges = rangesOnLine(0);
            QCOMPARE(ranges.size(), rangesInitial.size() + 1);
            TestHighlight(*ranges[0], {0, 4}, {0, 7}, searchHighlightColor);
        }

        TestPressKey(":set-nohls\\enter");
        {
            QVector<Kate::TextRange *> ranges = rangesOnLine(0);
            QCOMPARE(ranges.size(), rangesInitial.size());
        }
        // changing view range should not activate highlighting again
        kate_view->bottom();
        {
            QVector<Kate::TextRange *> ranges = rangesOnLine(5);
            QCOMPARE(ranges.size(), rangesInitial.size());
        }
        FinishTest(text);
    }
    // test that :set-nohlsearch turns of highlight
    {
        QString text = "foo bar xyz\n\n\n\n\nfoo ab bar x";
        BeginTest(text);
        auto vr = kate_view->visibleRange();
        // ensure that last line is not visible
        QVERIFY(vr.end().line() < 4);

        TestPressKey("/bar\\enter");
        {
            QVector<Kate::TextRange *> ranges = rangesOnLine(0);
            QCOMPARE(ranges.size(), rangesInitial.size() + 1);
            TestHighlight(*ranges[0], {0, 4}, {0, 7}, searchHighlightColor);
        }

        TestPressKey(":set-nohlsearch\\enter");
        {
            QVector<Kate::TextRange *> ranges = rangesOnLine(0);
            QCOMPARE(ranges.size(), rangesInitial.size());
        }
        // changing view range should not activate highlighting again
        kate_view->bottom();
        {
            QVector<Kate::TextRange *> ranges = rangesOnLine(5);
            QCOMPARE(ranges.size(), rangesInitial.size());
        }
        FinishTest(text);
    }
    // test that new search does not activate highlighting after :set-nohls
    {
        QString text = "foo bar xyz foo";
        BeginTest(text);

        TestPressKey("/bar\\enter");
        {
            QVector<Kate::TextRange *> ranges = rangesOnLine(0);
            QCOMPARE(ranges.size(), rangesInitial.size() + 1);
            TestHighlight(*ranges[0], {0, 4}, {0, 7}, searchHighlightColor);
        }

        TestPressKey(":set-nohls\\enter");
        {
            QVector<Kate::TextRange *> ranges = rangesOnLine(0);
            QCOMPARE(ranges.size(), rangesInitial.size());
        }
        TestPressKey("/bar\\enter");
        {
            QVector<Kate::TextRange *> ranges = rangesOnLine(0);
            QCOMPARE(ranges.size(), rangesInitial.size());
        }
        FinishTest(text);
    }
    // test that word search does not activate highlighting after :set-nohls
    {
        QString text = "foo bar xyz foo";
        BeginTest(text);

        TestPressKey("/bar\\enter");
        {
            QVector<Kate::TextRange *> ranges = rangesOnLine(0);
            QCOMPARE(ranges.size(), rangesInitial.size() + 1);
            TestHighlight(*ranges[0], {0, 4}, {0, 7}, searchHighlightColor);
        }

        TestPressKey(":set-nohls\\enter");
        {
            QVector<Kate::TextRange *> ranges = rangesOnLine(0);
            QCOMPARE(ranges.size(), rangesInitial.size());
        }
        TestPressKey("*");
        {
            QVector<Kate::TextRange *> ranges = rangesOnLine(0);
            QCOMPARE(ranges.size(), rangesInitial.size());
        }
        FinishTest(text);
    }
    // test that highlighting can be activated with :set-hls after :set-nohls
    {
        QString text = "foo bar xyz foo";
        BeginTest(text);

        TestPressKey("/bar\\enter");
        {
            QVector<Kate::TextRange *> ranges = rangesOnLine(0);
            QCOMPARE(ranges.size(), rangesInitial.size() + 1);
            TestHighlight(*ranges[0], {0, 4}, {0, 7}, searchHighlightColor);
        }

        TestPressKey(":set-nohls\\enter");
        {
            QVector<Kate::TextRange *> ranges = rangesOnLine(0);
            QCOMPARE(ranges.size(), rangesInitial.size());
        }
        TestPressKey(":set-hls\\enter");
        {
            QVector<Kate::TextRange *> ranges = rangesOnLine(0);
            QCOMPARE(ranges.size(), rangesInitial.size() + 1);
            TestHighlight(*ranges[0], {0, 4}, {0, 7}, searchHighlightColor);
        }
        FinishTest(text);
    }
    // test that highlighting can be activated with :set-hlsearch after :set-nohls
    {
        QString text = "foo bar xyz foo";
        BeginTest(text);

        TestPressKey("/bar\\enter");
        {
            QVector<Kate::TextRange *> ranges = rangesOnLine(0);
            QCOMPARE(ranges.size(), rangesInitial.size() + 1);
            TestHighlight(*ranges[0], {0, 4}, {0, 7}, searchHighlightColor);
        }

        TestPressKey(":set-nohls\\enter");
        {
            QVector<Kate::TextRange *> ranges = rangesOnLine(0);
            QCOMPARE(ranges.size(), rangesInitial.size());
        }
        TestPressKey(":set-hlsearch\\enter");
        {
            QVector<Kate::TextRange *> ranges = rangesOnLine(0);
            QCOMPARE(ranges.size(), rangesInitial.size() + 1);
            TestHighlight(*ranges[0], {0, 4}, {0, 7}, searchHighlightColor);
        }
        FinishTest(text);
    }
    // test that switching to normal mode turns off highlighting
    {
        QString text = "foo bar xyz foo";
        BeginTest(text);

        TestPressKey("/bar\\enter");
        {
            QVector<Kate::TextRange *> ranges = rangesOnLine(0);
            QCOMPARE(ranges.size(), rangesInitial.size() + 1);
            TestHighlight(*ranges[0], {0, 4}, {0, 7}, searchHighlightColor);
        }

        kate_view->setInputMode(KTextEditor::View::InputMode::NormalInputMode);
        {
            QVector<Kate::TextRange *> ranges = rangesOnLine(0);
            QCOMPARE(ranges.size(), rangesInitial.size());
        }
        kate_view->setInputMode(KTextEditor::View::InputMode::ViInputMode);
        FinishTest(text);
    }
    // test that opening search bar does not hide previous results
    {
        QString text = "foo xbar barx bar";

        BeginTest(text);

        TestPressKey("/bar\\enter");
        {
            QVector<Kate::TextRange *> ranges = rangesOnLine(0);
            QCOMPARE(ranges.size(), rangesInitial.size() + 3);
            TestHighlight(*ranges[0], {0, 5}, {0, 8}, searchHighlightColor);
        }
        TestPressKey("/");
        {
            QVector<Kate::TextRange *> ranges = rangesOnLine(0);
            QCOMPARE(ranges.size(), rangesInitial.size() + 3);
            TestHighlight(*ranges[0], {0, 5}, {0, 8}, searchHighlightColor);
        }
        TestPressKey("\\enter");
        {
            QVector<Kate::TextRange *> ranges = rangesOnLine(0);
            QCOMPARE(ranges.size(), rangesInitial.size() + 3);
            TestHighlight(*ranges[0], {0, 5}, {0, 8}, searchHighlightColor);
        }
        TestPressKey("/");
        {
            QVector<Kate::TextRange *> ranges = rangesOnLine(0);
            QCOMPARE(ranges.size(), rangesInitial.size() + 3);
            TestHighlight(*ranges[0], {0, 5}, {0, 8}, searchHighlightColor);
        }
        TestPressKey("\\esc");
        {
            QVector<Kate::TextRange *> ranges = rangesOnLine(0);
            QCOMPARE(ranges.size(), rangesInitial.size() + 3);
            TestHighlight(*ranges[0], {0, 5}, {0, 8}, searchHighlightColor);
        }
        FinishTest(text);
    }
    // test that deleting all text in search bar removes highlights
    {
        QString text = "foo xbar barx bar";

        BeginTest(text);

        TestPressKey("/bar");
        {
            QVector<Kate::TextRange *> ranges = rangesOnLine(0);
            QCOMPARE(ranges.size(), rangesInitial.size() + 3);
            TestHighlight(*ranges[0], {0, 5}, {0, 8}, searchHighlightColor);
        }
        TestPressKey("\\backspace\\backspace\\backspace");
        {
            QVector<Kate::TextRange *> ranges = rangesOnLine(0);
            QCOMPARE(ranges.size(), rangesInitial.size());
        }
        TestPressKey("\\esc");
        FinishTest(text);
    }
    // test that previously results are shown when current search is aborted
    {
        QString text = "foo xbar barx bar";

        BeginTest(text);

        TestPressKey("/bar\\enter");
        {
            QVector<Kate::TextRange *> ranges = rangesOnLine(0);
            QCOMPARE(ranges.size(), rangesInitial.size() + 3);
            TestHighlight(*ranges[0], {0, 5}, {0, 8}, searchHighlightColor);
        }
        TestPressKey("/rx");
        {
            QVector<Kate::TextRange *> ranges = rangesOnLine(0);
            QCOMPARE(ranges.size(), rangesInitial.size() + 1);
            TestHighlight(*ranges[0], {0, 11}, {0, 13}, searchHighlightColor);
        }
        TestPressKey("\\esc");
        {
            QVector<Kate::TextRange *> ranges = rangesOnLine(0);
            QCOMPARE(ranges.size(), rangesInitial.size() + 3);
            TestHighlight(*ranges[0], {0, 5}, {0, 8}, searchHighlightColor);
        }
        FinishTest(text);
    }
    // test that newly inserted text will be highlighted
    {
        QString text = "foo xbar abcd bar";

        TestPressKey("/xbar\\enter");
        {
            QVector<Kate::TextRange *> ranges = rangesOnLine(0);
            QCOMPARE(ranges.size(), rangesInitial.size() + 1);
            TestHighlight(*ranges[0], {0, 4}, {0, 8}, searchHighlightColor);
        }
        TestPressKey("wwix\\esc");
        {
            QVector<Kate::TextRange *> ranges = rangesOnLine(0);
            QCOMPARE(ranges.size(), rangesInitial.size() + 2);
            TestHighlight(*ranges[0], {0, 4}, {0, 8}, searchHighlightColor);
            TestHighlight(*ranges[1], {0, 14}, {0, 18}, searchHighlightColor);
        }

        BeginTest(text);
        FinishTest(text);
    }
    // test that no endless loop is triggered
    {
        QString text = "foo bar xyz\nabc def\nghi jkl\nmno pqr\nstu vwx\nfoo ab bar x";
        BeginTest(text);
        auto vr = kate_view->visibleRange();
        // ensure that last line is not visible
        QVERIFY(vr.end().line() < 4);

        TestPressKey("/\\\\<\\enter");

        FinishTest(text);
    }
}

QVector<Kate::TextRange *> HlSearchTest::rangesOnLine(int line)
{
    QVector<Kate::TextRange *> arr = kate_document->buffer().rangesForLine(line, kate_view, true);
    std::sort(arr.begin(), arr.end(), [](const Kate::TextRange *l, const Kate::TextRange *r) {
        return l->toRange().start() < r->toRange().start();
    });
    return arr;
}

void HlSearchTest::setWindowSize()
{
    const QFont font = kate_view->renderer()->config()->baseFont();
    QFontMetrics fm(font);
    auto fontHeight = fm.height();

    kate_document->setText("\n\n\n\n");
    for (int i = 250; i > 0; i -= fontHeight) {
        mainWindow->setMaximumHeight(i);
        if (kate_view->visibleRange().end().line() == 3)
            break;
    }
    QVERIFY(kate_view->visibleRange().end().line() == 3);
}

void HlSearchTest::TestHighlight_(int line, const char *file, const Kate::TextRange &r, std::array<int, 2> start, std::array<int, 2> end, const QColor &bg)
{
    QTest::qCompare(r.attribute()->background().color(), bg, "bgcolor", "bgcolor", file, line);
    QTest::qCompare(r.start().line(), start[0], "start_line", "start_line", file, line);
    QTest::qCompare(r.start().column(), start[1], "start_column", "start_column", file, line);
    QTest::qCompare(r.end().line(), end[0], "end_line", "end_line", file, line);
    QTest::qCompare(r.end().column(), end[1], "end_column", "end_column", file, line);
}
