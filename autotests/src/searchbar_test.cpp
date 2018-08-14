/* This file is part of the KDE libraries
   Copyright (C) 2010 Bernhard Beschow <bbeschow@cs.tu-berlin.de>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#include "searchbar_test.h"

#include "ui_searchbarincremental.h"
#include "ui_searchbarpower.h"

#include <katedocument.h>
#include <kateview.h>
#include <kateconfig.h>
#include <kateglobal.h>
#include <katesearchbar.h>
#include <ktexteditor/movingrange.h>
#include <KMessageBox>

#include <QtTestWidgets>
#include <QStringListModel>

QTEST_MAIN(SearchBarTest)

#define testNewRow() (QTest::newRow(QString("line %1").arg(__LINE__).toLatin1().data()))

using namespace KTextEditor;

SearchBarTest::SearchBarTest()
    : QObject()
{
}

SearchBarTest::~SearchBarTest()
{
}

void SearchBarTest::initTestCase()
{
    KTextEditor::EditorPrivate::enableUnitTestMode();
    KMessageBox::saveDontShowAgainYesNo(QLatin1String("DoNotShowAgainContinueSearchDialog"), KMessageBox::Yes);
}

void SearchBarTest::cleanupTestCase()
{
}

void SearchBarTest::testFindNextIncremental()
{
    KTextEditor::DocumentPrivate doc;
    doc.setText("a a a b b");

    KTextEditor::ViewPrivate view(&doc, nullptr);
    KateViewConfig config(&view);

    KateSearchBar bar(false, &view, &config);

    bar.setSearchPattern("b");

    QCOMPARE(view.selectionRange(), Range(0, 6, 0, 7));

    bar.findNext();

    QCOMPARE(view.selectionRange(), Range(0, 8, 0, 9));

    bar.setSearchPattern("a");

    QCOMPARE(view.selectionRange(), Range(0, 0, 0, 1));

    bar.findNext();

    QCOMPARE(view.selectionRange(), Range(0, 2, 0, 3));

    bar.findNext();

    QCOMPARE(view.selectionRange(), Range(0, 4, 0, 5));

    bar.findNext();

    QCOMPARE(view.selectionRange(), Range(0, 0, 0, 1));
}

void SearchBarTest::testSetMatchCaseIncremental()
{
    KTextEditor::DocumentPrivate doc;
    KTextEditor::ViewPrivate view(&doc, nullptr);
    KateViewConfig config(&view);

    doc.setText("a A a");
    KateSearchBar bar(false, &view, &config);

    QVERIFY(!bar.isPower());
    QVERIFY(!view.selection());

    bar.setMatchCase(false);
    bar.setSearchPattern("A");

    QVERIFY(!bar.matchCase());
    QCOMPARE(view.selectionRange(), Range(0, 0, 0, 1));

    bar.setMatchCase(true);

    QVERIFY(bar.matchCase());
    QCOMPARE(view.selectionRange(), Range(0, 2, 0, 3));

    bar.setMatchCase(false);

    QVERIFY(!bar.matchCase());
    QCOMPARE(view.selectionRange(), Range(0, 0, 0, 1));

    bar.setMatchCase(true);

    QVERIFY(bar.matchCase());
    QCOMPARE(view.selectionRange(), Range(0, 2, 0, 3));
}

void SearchBarTest::testSetMatchCasePower()
{
    KTextEditor::DocumentPrivate doc;
    KTextEditor::ViewPrivate view(&doc, nullptr);
    KateViewConfig config(&view);

    doc.setText("a A a");
    view.setCursorPosition(Cursor(0, 0));

    KateSearchBar bar(true, &view, &config);

    QVERIFY(bar.isPower());
    QVERIFY(!view.selection());

    bar.setMatchCase(false);
    bar.setSearchPattern("A");
    bar.findNext();

    QCOMPARE(bar.searchPattern(), QString("A"));
    QVERIFY(!bar.matchCase());
    QCOMPARE(view.selectionRange(), Range(0, 0, 0, 1));

    bar.setMatchCase(true);

    QCOMPARE(view.selectionRange(), Range(0, 0, 0, 1));

    bar.findNext();

    QVERIFY(bar.matchCase());
    QCOMPARE(view.selectionRange(), Range(0, 2, 0, 3));

    bar.setMatchCase(false);

    QVERIFY(!bar.matchCase());
    QCOMPARE(view.selectionRange(), Range(0, 2, 0, 3));

    bar.findNext();

    QCOMPARE(view.selectionRange(), Range(0, 4, 0, 5));
}

void SearchBarTest::testSetSelectionOnlyPower()
{
    KTextEditor::DocumentPrivate doc;
    KTextEditor::ViewPrivate view(&doc, nullptr);
    KateViewConfig config(&view);

    doc.setText("a a a");
    KateSearchBar bar(true, &view, &config);

    bar.setSearchPattern("a");

    QVERIFY(bar.isPower());
    QVERIFY(!view.selection());

    bar.setSelectionOnly(false);
    bar.findNext();

    QVERIFY(!bar.selectionOnly());
    QCOMPARE(view.selectionRange(), Range(0, 0, 0, 1));

    view.setSelection(Range(0, 2, 0, 5));
    bar.setSelectionOnly(true);

    QVERIFY(bar.selectionOnly());

    bar.findNext();

    QCOMPARE(view.selectionRange(), Range(0, 2, 0, 3));
    QVERIFY(bar.selectionOnly());

    bar.setSelectionOnly(false);
    bar.findNext();

    QCOMPARE(view.selectionRange(), Range(0, 4, 0, 5));
    QVERIFY(!bar.selectionOnly());
}

void SearchBarTest::testSetSearchPattern_data()
{
    QTest::addColumn<bool>("power");
    QTest::addColumn<int>("numMatches2");

    testNewRow() << false << 0;
    testNewRow() << true  << 3;
}

void SearchBarTest::testSetSearchPattern()
{
    QFETCH(bool, power);
    QFETCH(int, numMatches2);

    KTextEditor::DocumentPrivate doc;
    KTextEditor::ViewPrivate view(&doc, nullptr);
    KateViewConfig config(&view);

    doc.setText("a a a");

    KateSearchBar bar(power, &view, &config);

    bar.setSearchPattern("a");
    bar.findAll();

    QCOMPARE(bar.m_hlRanges.size(), 3);

    bar.setSearchPattern("a ");

    QCOMPARE(bar.m_hlRanges.size(), numMatches2);

    bar.findAll();

    QCOMPARE(bar.m_hlRanges.size(), 2);
}

void SearchBarTest::testSetSelectionOnly()
{
    KTextEditor::DocumentPrivate doc;
    KTextEditor::ViewPrivate view(&doc, nullptr);
    KateViewConfig config(&view);

    doc.setText("a a a");
    view.setSelection(Range(0, 0, 0, 3));

    KateSearchBar bar(false, &view, &config);

    bar.setSelectionOnly(false);
    bar.setSearchPattern("a");
    bar.findAll();

    QCOMPARE(bar.m_hlRanges.size(), 3);

    bar.setSelectionOnly(true);

    QCOMPARE(bar.m_hlRanges.size(), 3);
}

void SearchBarTest::testFindAll_data()
{
    QTest::addColumn<bool>("power");
    QTest::addColumn<int>("numMatches2");
    QTest::addColumn<int>("numMatches4");

    testNewRow() << false << 0 << 0;
    testNewRow() << true  << 3 << 2;
}

void SearchBarTest::testFindAll()
{
    QFETCH(bool, power);
    QFETCH(int, numMatches2);
    QFETCH(int, numMatches4);

    KTextEditor::DocumentPrivate doc;
    KTextEditor::ViewPrivate view(&doc, nullptr);
    KateViewConfig config(&view);

    doc.setText("a a a");
    KateSearchBar bar(power, &view, &config);

    QCOMPARE(bar.isPower(), power);

    bar.setSearchPattern("a");
    bar.findAll();

    QCOMPARE(bar.m_hlRanges.size(), 3);
    QCOMPARE(bar.m_hlRanges.at(0)->toRange(), Range(0, 0, 0, 1));
    QCOMPARE(bar.m_hlRanges.at(1)->toRange(), Range(0, 2, 0, 3));
    QCOMPARE(bar.m_hlRanges.at(2)->toRange(), Range(0, 4, 0, 5));

    bar.setSearchPattern("a ");

    QCOMPARE(bar.m_hlRanges.size(), numMatches2);

    bar.findAll();

    QCOMPARE(bar.m_hlRanges.size(), 2);

    bar.setSearchPattern("a  ");

    QCOMPARE(bar.m_hlRanges.size(), numMatches4);

    bar.findAll();

    QCOMPARE(bar.m_hlRanges.size(), 0);
}

void SearchBarTest::testReplaceAll()
{
    KTextEditor::DocumentPrivate doc;
    KTextEditor::ViewPrivate view(&doc, nullptr);
    KateViewConfig config(&view);

    doc.setText("a a a");
    KateSearchBar bar(true, &view, &config);

    bar.setSearchPattern("a");
    bar.setReplacementPattern("");
    bar.replaceAll();

    QCOMPARE(bar.m_hlRanges.size(), 3);
    QCOMPARE(bar.m_hlRanges.at(0)->toRange(), Range(0, 0, 0, 0));
    QCOMPARE(bar.m_hlRanges.at(1)->toRange(), Range(0, 1, 0, 1));
    QCOMPARE(bar.m_hlRanges.at(2)->toRange(), Range(0, 2, 0, 2));

    bar.setSearchPattern(" ");
    bar.setReplacementPattern("b");
    bar.replaceAll();

    QCOMPARE(bar.m_hlRanges.size(), 2);
    QCOMPARE(bar.m_hlRanges.at(0)->toRange(), Range(0, 0, 0, 1));
    QCOMPARE(bar.m_hlRanges.at(1)->toRange(), Range(0, 1, 0, 2));
}

void SearchBarTest::testFindSelectionForward_data()
{
    QTest::addColumn<QString>("text");
    QTest::addColumn<bool>("selectionOnly");
    QTest::addColumn<Range>("selectionRange");
    QTest::addColumn<Range>("match");

    testNewRow() << "a a a" << false << Range(0, 0, 0, 1) << Range(0, 0, 0, 2);
    testNewRow() << "a a a" << true  << Range(0, 0, 0, 1) << Range(0, 0, 0, 1);

    testNewRow() << "a a a" << false << Range(0, 0, 0, 2) << Range(0, 2, 0, 4);
    testNewRow() << "a a a" << true  << Range(0, 0, 0, 2) << Range(0, 0, 0, 2);

    testNewRow() << "a a a" << false << Range(0, 0, 0, 3) << Range(0, 0, 0, 2);
    testNewRow() << "a a a" << true  << Range(0, 0, 0, 3) << Range(0, 0, 0, 2);

    testNewRow() << "a a a" << false << Range(0, 2, 0, 4) << Range(0, 0, 0, 2);
    testNewRow() << "a a a" << true  << Range(0, 2, 0, 4) << Range(0, 2, 0, 4);

    testNewRow() << "a a a" << false << Range(0, 3, 0, 4) << Range(0, 0, 0, 2);
    testNewRow() << "a a a" << true  << Range(0, 3, 0, 4) << Range(0, 3, 0, 4);
}

void SearchBarTest::testFindSelectionForward()
{
    QFETCH(QString, text);
    QFETCH(bool, selectionOnly);
    QFETCH(Range, selectionRange);
    QFETCH(Range, match);

    KTextEditor::DocumentPrivate doc;
    KTextEditor::ViewPrivate view(&doc, nullptr);
    KateViewConfig config(&view);

    doc.setText(text);

    view.setSelection(Range(0, 0, 0, 2));

    KateSearchBar bar(true, &view, &config);
    QVERIFY(bar.searchPattern() == QString("a "));

    view.setSelection(selectionRange);
    QCOMPARE(view.selectionRange(), selectionRange);
    bar.setSelectionOnly(selectionOnly);

    bar.findNext();

    QCOMPARE(view.selectionRange(), match);
}

void SearchBarTest::testRemoveWithSelectionForward_data()
{
    QTest::addColumn<Range>("selectionRange");
    QTest::addColumn<Range>("match");

    testNewRow() << Range(0, 0, 0, 1) << Range(0, 0, 0, 2);
    testNewRow() << Range(0, 0, 0, 2) << Range(0, 0, 0, 2);
    testNewRow() << Range(0, 0, 0, 3) << Range(0, 0, 0, 2);
    testNewRow() << Range(0, 2, 0, 4) << Range(0, 0, 0, 2);
    testNewRow() << Range(0, 3, 0, 4) << Range(0, 0, 0, 2);
}

void SearchBarTest::testRemoveWithSelectionForward()
{
    QFETCH(Range, selectionRange);
    QFETCH(Range, match);

    KTextEditor::DocumentPrivate doc;
    KTextEditor::ViewPrivate view(&doc, nullptr);
    KateViewConfig config(&view);

    doc.setText("a a a");
    view.setSelection(selectionRange);

    KateSearchBar bar(true, &view, &config);
    bar.setSearchPattern("a ");
    bar.setSelectionOnly(false);

    bar.replaceNext();

    QCOMPARE(view.selectionRange(), match);
}

void SearchBarTest::testRemoveInSelectionForward_data()
{
    QTest::addColumn<Range>("selectionRange");
    QTest::addColumn<Range>("match");

    testNewRow() << Range(0, 0, 0, 1) << Range(0, 0, 0, 1);
    testNewRow() << Range(0, 0, 0, 2) << Range(0, 0, 0, 0);
    testNewRow() << Range(0, 0, 0, 3) << Range(0, 0, 0, 2);
    testNewRow() << Range(0, 0, 0, 4) << Range(0, 0, 0, 2);
    testNewRow() << Range(0, 2, 0, 4) << Range(0, 2, 0, 2);
    testNewRow() << Range(0, 3, 0, 4) << Range(0, 3, 0, 4);
}

void SearchBarTest::testRemoveInSelectionForward()
{
    QFETCH(Range, selectionRange);
    QFETCH(Range, match);

    KTextEditor::DocumentPrivate doc;
    KTextEditor::ViewPrivate view(&doc, nullptr);
    KateViewConfig config(&view);

    doc.setText("a a a");
    view.setSelection(selectionRange);

    KateSearchBar bar(true, &view, &config);
    bar.setSearchPattern("a ");
    bar.setSelectionOnly(true);

    QVERIFY(bar.replacementPattern().isEmpty());

    bar.replaceNext();

    QCOMPARE(view.selectionRange(), match);
}

void SearchBarTest::testReplaceWithDoubleSelecion_data()
{
    QTest::addColumn<QString>("text");
    QTest::addColumn<Range>("selectionRange");
    QTest::addColumn<QString>("result");
    QTest::addColumn<Range>("match");

//  testNewRow() << "a"  << Range(0, 0, 0, 1) << "aa" << Range(?, ?, ?, ?);
    testNewRow() << "aa" << Range(0, 1, 0, 2) << "aaa" << Range(0, 0, 0, 1);
    testNewRow() << "aa" << Range(0, 0, 0, 1) << "aaa" << Range(0, 2, 0, 3);

//  testNewRow() << "ab"  << Range(0, 0, 0, 1) << "aab"  << Range(?, ?, ?, ?);
    testNewRow() << "aab" << Range(0, 0, 0, 1) << "aaab" << Range(0, 2, 0, 3);
    testNewRow() << "aba" << Range(0, 0, 0, 1) << "aaba" << Range(0, 3, 0, 4);

//  testNewRow() << "ab"   << Range(0, 0, 0, 2) << "abab"   << Range(?, ?, ?, ?);
    testNewRow() << "abab" << Range(0, 0, 0, 2) << "ababab" << Range(0, 4, 0, 6);
    testNewRow() << "abab" << Range(0, 2, 0, 4) << "ababab" << Range(0, 0, 0, 2);
}

void SearchBarTest::testReplaceWithDoubleSelecion()
{
    QFETCH(QString, text);
    QFETCH(Range, selectionRange);
    QFETCH(QString, result);
    QFETCH(Range, match);

    KTextEditor::DocumentPrivate doc;
    KTextEditor::ViewPrivate view(&doc, nullptr);
    KateViewConfig config(&view);

    doc.setText(text);
    view.setSelection(selectionRange);

    KateSearchBar bar(true, &view, &config);

    bar.setSelectionOnly(false);
    bar.setReplacementPattern(bar.searchPattern() + bar.searchPattern());
    bar.replaceNext();

    QCOMPARE(doc.text(), result);
    QCOMPARE(view.selectionRange(), match);
}

void SearchBarTest::testReplaceDollar()
{
    KTextEditor::DocumentPrivate doc;
    KTextEditor::ViewPrivate view(&doc, nullptr);
    KateViewConfig config(&view);

    doc.setText("aaa\nbbb\nccc\n\n\naaa\nbbb\nccc\nddd\n");

    KateSearchBar bar(true, &view, &config);

    bar.setSearchPattern("$");
    bar.setSearchMode(KateSearchBar::MODE_REGEX);
    bar.setReplacementPattern("D");

    bar.replaceAll();

    QCOMPARE(doc.text(), QString("aaaD\nbbbD\ncccD\nD\nD\naaaD\nbbbD\ncccD\ndddD\n"));
}

void SearchBarTest::testSearchHistoryIncremental()
{
    KTextEditor::DocumentPrivate doc;
    KTextEditor::ViewPrivate view(&doc, nullptr);
    KateViewConfig *const config = view.config();
    KTextEditor::EditorPrivate::self()->searchHistoryModel()->setStringList(QStringList());

    doc.setText("foo bar");

    KateSearchBar bar(false, &view, config);

    bar.setSearchPattern("foo");
    bar.findNext();

    QCOMPARE(bar.m_incUi->pattern->findText("foo"), 0);

    bar.setSearchPattern("bar");
    bar.findNext();

    QCOMPARE(bar.m_incUi->pattern->findText("bar"), 0);
    QCOMPARE(bar.m_incUi->pattern->findText("foo"), 1);

    KTextEditor::DocumentPrivate doc2;
    KTextEditor::ViewPrivate view2(&doc2, nullptr);
    KateViewConfig *const config2 = view2.config();
    KateSearchBar bar2(false, &view2, config2);

    QCOMPARE(bar2.m_incUi->pattern->findText("bar"), 0);
    QCOMPARE(bar2.m_incUi->pattern->findText("foo"), 1);

    //testcase for https://bugs.kde.org/show_bug.cgi?id=248305
    bar2.m_incUi->pattern->setCurrentIndex(1);
    QCOMPARE(bar2.searchPattern(), QLatin1String("foo"));
    bar2.findNext();
    QCOMPARE(bar2.searchPattern(), QLatin1String("foo"));
}

void SearchBarTest::testSearchHistoryPower()
{
    KTextEditor::DocumentPrivate doc;
    KTextEditor::ViewPrivate view(&doc, nullptr);
    KateViewConfig *const config = view.config();
    KTextEditor::EditorPrivate::self()->searchHistoryModel()->setStringList(QStringList());

    doc.setText("foo bar");

    KateSearchBar bar(true, &view, config);

    QCOMPARE(bar.m_powerUi->pattern->count(), 0);

    bar.setSearchPattern("foo");
    bar.findNext();

    QCOMPARE(bar.m_powerUi->pattern->findText("foo"), 0);

    bar.findNext();

    QCOMPARE(bar.m_powerUi->pattern->findText("foo"), 0);
    QCOMPARE(bar.m_powerUi->pattern->count(), 1);

    bar.setSearchPattern("bar");
    bar.findNext();

    QCOMPARE(bar.m_powerUi->pattern->findText("bar"), 0);
    QCOMPARE(bar.m_powerUi->pattern->findText("foo"), 1);
    QCOMPARE(bar.m_powerUi->pattern->count(), 2);

    KTextEditor::DocumentPrivate doc2;
    KTextEditor::ViewPrivate view2(&doc2, nullptr);
    KateViewConfig *const config2 = view2.config();
    KateSearchBar bar2(true, &view2, config2);

    QCOMPARE(bar2.m_powerUi->pattern->findText("bar"), 0);
    QCOMPARE(bar2.m_powerUi->pattern->findText("foo"), 1);
}

// Make sure Kate doesn't replace anything outside selection in block mode (see bug 253191)
void SearchBarTest::testReplaceInBlockMode()
{
    KTextEditor::DocumentPrivate doc;
    KTextEditor::ViewPrivate view(&doc, nullptr);
    view.setInputMode(View::NormalInputMode);
    KateViewConfig config(&view);

    doc.setText("111\n111");
    view.setBlockSelection(true);
    view.setSelection(KTextEditor::Range(0, 1, 1, 2));

    KateSearchBar bar(true, &view, &config);

    bar.setSearchPattern("1");
    bar.setReplacementPattern("2");
    bar.replaceAll();

    QCOMPARE(doc.text(), QString("121\n121"));
}


void SearchBarTest::testReplaceManyCapturesBug365124()
{
    KTextEditor::DocumentPrivate doc;
    KTextEditor::ViewPrivate view(&doc, nullptr);
    KateViewConfig config(&view);

    doc.setText("one two three four five six seven eight nine ten eleven twelve thirteen\n");

    KateSearchBar bar(true, &view, &config);

    bar.setSearchPattern("^(.*) (.*) (.*) (.*) (.*) (.*) (.*) (.*) (.*) (.*) (.*) (.*) (.*)$");
    bar.setSearchMode(KateSearchBar::MODE_REGEX);
    bar.setReplacementPattern("\\{1}::\\2::\\3::\\4::\\5::\\6::\\7::\\8::\\9::\\{10}::\\{11}::\\{12}::\\{13}");

    bar.replaceAll();

    QCOMPARE(doc.text(), QString("one::two::three::four::five::six::seven::eight::nine::ten::eleven::twelve::thirteen\n"));
}

void SearchBarTest::testReplaceEscapeSequence_data()
{
    QTest::addColumn<QString>("textBefore");
    QTest::addColumn<QString>("textAfter");
    QTest::addColumn<Cursor>("cursorBefore");
    QTest::addColumn<Cursor>("cursorAfter");

    testNewRow() << QStringLiteral("a\n") << QStringLiteral("a ") << Cursor(1, 0) << Cursor(0, 2);
    testNewRow() << QStringLiteral("a\nb\n") << QStringLiteral("a b ") << Cursor(2, 0) << Cursor(0, 4);
    testNewRow() << QStringLiteral("\n\n\n") << QStringLiteral("   ") << Cursor(3, 0) << Cursor(0, 3);
}

void SearchBarTest::testReplaceEscapeSequence()
{
    QFETCH(QString, textBefore);
    QFETCH(QString, textAfter);
    QFETCH(Cursor, cursorBefore);
    QFETCH(Cursor, cursorAfter);

    // testcase for https://bugs.kde.org/show_bug.cgi?id=381080
    KTextEditor::DocumentPrivate doc;
    KTextEditor::ViewPrivate view(&doc, nullptr);
    KateViewConfig config(&view);

    doc.setText(textBefore);
    view.setCursorPosition(cursorBefore);
    QCOMPARE(view.cursorPosition(), cursorBefore);

    KateSearchBar bar(true, &view, &config);

    bar.setSearchMode(KateSearchBar::MODE_ESCAPE_SEQUENCES);
    bar.setSearchPattern(QStringLiteral("\\n"));
    bar.setReplacementPattern(QStringLiteral(" "));

    bar.replaceAll();

    QCOMPARE(doc.text(), textAfter);
    QCOMPARE(view.cursorPosition(), cursorAfter);
}

#include "moc_searchbar_test.cpp"
