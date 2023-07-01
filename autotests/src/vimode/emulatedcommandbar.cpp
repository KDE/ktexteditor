/*
    This file is part of the KDE libraries
    SPDX-FileCopyrightText: 2011 Kuzmich Svyatoslav
    SPDX-FileCopyrightText: 2012-2013 Simon St James <kdedevel@etotheipiplusone.com>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "emulatedcommandbar.h"
#include "emulatedcommandbarsetupandteardown.h"
#include "keys.h"
#include "view.h"
#include "vimode/globalstate.h"
#include "vimode/mappings.h"
#include <inputmode/kateviinputmode.h>
#include <katebuffer.h>
#include <kateconfig.h>
#include <katedocument.h>
#include <kateview.h>
#include <vimode/emulatedcommandbar/emulatedcommandbar.h>
#include <vimode/history.h>

#include <QAbstractItemView>
#include <QClipboard>
#include <QCompleter>
#include <QLabel>
#include <QStringListModel>

#include <KActionCollection>
#include <KColorScheme>

QTEST_MAIN(EmulatedCommandBarTest)

using namespace KTextEditor;
using namespace KateVi;

void EmulatedCommandBarTest::EmulatedCommandBarTests()
{
    // Ensure that some preconditions for these tests are setup, and (more importantly)
    // ensure that they are reverted no matter how these tests end.
    EmulatedCommandBarSetUpAndTearDown emulatedCommandBarSetUpAndTearDown(vi_input_mode, kate_view, mainWindow);

    // Verify that we can get a non-null pointer to the emulated command bar.
    EmulatedCommandBar *emulatedCommandBar = vi_input_mode->viModeEmulatedCommandBar();
    QVERIFY(emulatedCommandBar);

    // Should initially be hidden.
    QVERIFY(!emulatedCommandBar->isVisible());

    // Test that "/" invokes the emulated command bar (if we are configured to use it)
    BeginTest("");
    TestPressKey("/");
    QVERIFY(emulatedCommandBar->isVisible());
    QCOMPARE(emulatedCommandTypeIndicator()->text(), QString("/"));
    QVERIFY(emulatedCommandTypeIndicator()->isVisible());
    QVERIFY(emulatedCommandBarTextEdit());
    QVERIFY(emulatedCommandBarTextEdit()->text().isEmpty());
    // Make sure the keypresses end up changing the text.
    QVERIFY(emulatedCommandBarTextEdit()->isVisible());
    TestPressKey("foo");
    QCOMPARE(emulatedCommandBarTextEdit()->text(), QString("foo"));
    // Make sure ctrl-c dismisses it (assuming we allow Vim to steal the ctrl-c shortcut).
    TestPressKey("\\ctrl-c");
    QVERIFY(!emulatedCommandBar->isVisible());

    // Ensure that ESC dismisses it, too.
    BeginTest("");
    TestPressKey("/");
    QVERIFY(emulatedCommandBar->isVisible());
    TestPressKey("\\esc");
    QVERIFY(!emulatedCommandBar->isVisible());
    FinishTest("");

    // Ensure that Ctrl-[ dismisses it, too.
    BeginTest("");
    TestPressKey("/");
    QVERIFY(emulatedCommandBar->isVisible());
    TestPressKey("\\ctrl-[");
    QVERIFY(!emulatedCommandBar->isVisible());
    FinishTest("");

    // Ensure that Enter dismisses it, too.
    BeginTest("");
    TestPressKey("/");
    QVERIFY(emulatedCommandBar->isVisible());
    TestPressKey("\\enter");
    QVERIFY(!emulatedCommandBar->isVisible());
    FinishTest("");

    // Ensure that Return dismisses it, too.
    BeginTest("");
    TestPressKey("/");
    QVERIFY(emulatedCommandBar->isVisible());
    TestPressKey("\\return");
    QVERIFY(!emulatedCommandBar->isVisible());
    FinishTest("");

    // Ensure that text is always initially empty.
    BeginTest("");
    TestPressKey("/a\\enter");
    TestPressKey("/");
    QVERIFY(emulatedCommandBarTextEdit()->text().isEmpty());
    TestPressKey("\\enter");
    FinishTest("");

    // Check backspace works.
    BeginTest("");
    TestPressKey("/foo\\backspace");
    QCOMPARE(emulatedCommandBarTextEdit()->text(), QString("fo"));
    TestPressKey("\\enter");
    FinishTest("");

    // Check ctrl-h works.
    BeginTest("");
    TestPressKey("/bar\\ctrl-h");
    QCOMPARE(emulatedCommandBarTextEdit()->text(), QString("ba"));
    TestPressKey("\\enter");
    FinishTest("");

    // ctrl-h should dismiss bar when empty.
    BeginTest("");
    TestPressKey("/\\ctrl-h");
    QVERIFY(!emulatedCommandBar->isVisible());
    FinishTest("");

    // ctrl-h should not dismiss bar when there is stuff to the left of cursor.
    BeginTest("");
    TestPressKey("/a\\ctrl-h");
    QVERIFY(emulatedCommandBar->isVisible());
    TestPressKey("\\enter");
    FinishTest("");

    // ctrl-h should not dismiss bar when bar is not empty, even if there is nothing to the left of cursor.
    BeginTest("");
    TestPressKey("/a\\left\\ctrl-h");
    QVERIFY(emulatedCommandBar->isVisible());
    TestPressKey("\\enter");
    FinishTest("");

    // Same for backspace.
    BeginTest("");
    TestPressKey("/bar\\backspace");
    QCOMPARE(emulatedCommandBarTextEdit()->text(), QString("ba"));
    TestPressKey("\\enter");
    FinishTest("");
    BeginTest("");
    TestPressKey("/\\backspace");
    QVERIFY(!emulatedCommandBar->isVisible());
    FinishTest("");
    BeginTest("");
    TestPressKey("/a\\backspace");
    QVERIFY(emulatedCommandBar->isVisible());
    TestPressKey("\\enter");
    FinishTest("");
    BeginTest("");
    TestPressKey("/a\\left\\backspace");
    QVERIFY(emulatedCommandBar->isVisible());
    TestPressKey("\\enter");
    FinishTest("");

    // Check ctrl-b works.
    BeginTest("");
    TestPressKey("/bar foo xyz\\ctrl-bX");
    QCOMPARE(emulatedCommandBarTextEdit()->text(), QString("Xbar foo xyz"));
    TestPressKey("\\enter");
    FinishTest("");

    // Check ctrl-e works.
    BeginTest("");
    TestPressKey("/bar foo xyz\\ctrl-b\\ctrl-eX");
    QCOMPARE(emulatedCommandBarTextEdit()->text(), QString("bar foo xyzX"));
    TestPressKey("\\enter");
    FinishTest("");

    // Check ctrl-w works.
    BeginTest("");
    TestPressKey("/foo bar\\ctrl-w");
    QCOMPARE(emulatedCommandBarTextEdit()->text(), QString("foo "));
    TestPressKey("\\enter");
    FinishTest("");

    // Check ctrl-w works on empty command bar.
    BeginTest("");
    TestPressKey("/\\ctrl-w");
    QCOMPARE(emulatedCommandBarTextEdit()->text(), QString(""));
    TestPressKey("\\enter");
    FinishTest("");

    // Check ctrl-w works in middle of word.
    BeginTest("");
    TestPressKey("/foo bar\\left\\left\\ctrl-w");
    QCOMPARE(emulatedCommandBarTextEdit()->text(), QString("foo ar"));
    TestPressKey("\\enter");
    FinishTest("");

    // Check ctrl-w leaves the cursor in the right place when in the middle of word.
    BeginTest("");
    TestPressKey("/foo bar\\left\\left\\ctrl-wX");
    QCOMPARE(emulatedCommandBarTextEdit()->text(), QString("foo Xar"));
    TestPressKey("\\enter");
    FinishTest("");

    // Check ctrl-w works when at the beginning of the text.
    BeginTest("");
    TestPressKey("/foo\\left\\left\\left\\ctrl-w");
    QCOMPARE(emulatedCommandBarTextEdit()->text(), QString("foo"));
    TestPressKey("\\enter");
    FinishTest("");

    // Check ctrl-w works when the character to the left is a space.
    BeginTest("");
    TestPressKey("/foo bar   \\ctrl-w");
    QCOMPARE(emulatedCommandBarTextEdit()->text(), QString("foo "));
    TestPressKey("\\enter");
    FinishTest("");

    // Check ctrl-w works when all characters to the left of the cursor are spaces.
    BeginTest("");
    TestPressKey("/   \\ctrl-w");
    QCOMPARE(emulatedCommandBarTextEdit()->text(), QString(""));
    TestPressKey("\\enter");
    FinishTest("");

    // Check ctrl-w works when all characters to the left of the cursor are non-spaces.
    BeginTest("");
    TestPressKey("/foo\\ctrl-w");
    QCOMPARE(emulatedCommandBarTextEdit()->text(), QString(""));
    TestPressKey("\\enter");
    FinishTest("");

    // Check ctrl-w does not continue to delete subsequent alphanumerics if the characters to the left of the cursor
    // are non-space, non-alphanumerics.
    BeginTest("");
    TestPressKey("/foo!!!\\ctrl-w");
    QCOMPARE(emulatedCommandBarTextEdit()->text(), QString("foo"));
    TestPressKey("\\enter");
    FinishTest("");
    // Check ctrl-w does not continue to delete subsequent alphanumerics if the characters to the left of the cursor
    // are non-space, non-alphanumerics.
    BeginTest("");
    TestPressKey("/foo!!!\\ctrl-w");
    QCOMPARE(emulatedCommandBarTextEdit()->text(), QString("foo"));
    TestPressKey("\\enter");
    FinishTest("");

    // Check ctrl-w deletes underscores and alphanumerics to the left of the cursor, but stops when it reaches a
    // character that is none of these.
    BeginTest("");
    TestPressKey("/foo!!!_d1\\ctrl-w");
    QCOMPARE(emulatedCommandBarTextEdit()->text(), QString("foo!!!"));
    TestPressKey("\\enter");
    FinishTest("");

    // Check ctrl-w doesn't swallow the spaces preceding the block of non-word chars.
    BeginTest("");
    TestPressKey("/foo !!!\\ctrl-w");
    QCOMPARE(emulatedCommandBarTextEdit()->text(), QString("foo "));
    TestPressKey("\\enter");
    FinishTest("");

    // Check ctrl-w doesn't swallow the spaces preceding the word.
    BeginTest("");
    TestPressKey("/foo 1d_\\ctrl-w");
    QCOMPARE(emulatedCommandBarTextEdit()->text(), QString("foo "));
    TestPressKey("\\enter");
    FinishTest("");

    // Check there is a "waiting for register" indicator, initially hidden.
    BeginTest("");
    TestPressKey("/");
    QLabel *waitingForRegisterIndicator = emulatedCommandBar->findChild<QLabel *>("waitingforregisterindicator");
    QVERIFY(waitingForRegisterIndicator);
    QVERIFY(!waitingForRegisterIndicator->isVisible());
    QCOMPARE(waitingForRegisterIndicator->text(), QString("\""));
    TestPressKey("\\enter");
    FinishTest("");

    // Test that ctrl-r causes it to become visible.  It is displayed to the right of the text edit.
    BeginTest("");
    TestPressKey("/\\ctrl-r");
    QVERIFY(waitingForRegisterIndicator->isVisible());
    QVERIFY(waitingForRegisterIndicator->x() >= emulatedCommandBarTextEdit()->x() + emulatedCommandBarTextEdit()->width());
    TestPressKey("\\ctrl-c");
    TestPressKey("\\ctrl-c");
    FinishTest("");

    // The first ctrl-c after ctrl-r (when no register entered) hides the waiting for register
    // indicator, but not the bar.
    BeginTest("");
    TestPressKey("/\\ctrl-r");
    QVERIFY(waitingForRegisterIndicator->isVisible());
    TestPressKey("\\ctrl-c");
    QVERIFY(!waitingForRegisterIndicator->isVisible());
    QVERIFY(emulatedCommandBar->isVisible());
    TestPressKey("\\ctrl-c"); // Dismiss the bar.
    FinishTest("");

    // The first ctrl-c after ctrl-r (when no register entered) aborts waiting for register.
    BeginTest("foo");
    TestPressKey("\"cyiw/\\ctrl-r\\ctrl-ca");
    QCOMPARE(emulatedCommandBarTextEdit()->text(), QString("a"));
    TestPressKey("\\ctrl-c"); // Dismiss the bar.
    FinishTest("foo");

    // Same as above, but for ctrl-[ instead of ctrl-c.
    BeginTest("");
    TestPressKey("/\\ctrl-r");
    QVERIFY(waitingForRegisterIndicator->isVisible());
    TestPressKey("\\ctrl-[");
    QVERIFY(!waitingForRegisterIndicator->isVisible());
    QVERIFY(emulatedCommandBar->isVisible());
    TestPressKey("\\ctrl-c"); // Dismiss the bar.
    FinishTest("");
    BeginTest("foo");
    TestPressKey("\"cyiw/\\ctrl-r\\ctrl-[a");
    QCOMPARE(emulatedCommandBarTextEdit()->text(), QString("a"));
    TestPressKey("\\ctrl-c"); // Dismiss the bar.
    FinishTest("foo");

    // Check ctrl-r works with registers, and hides the "waiting for register" indicator.
    BeginTest("xyz");
    TestPressKey("\"ayiw/foo\\ctrl-ra");
    QCOMPARE(emulatedCommandBarTextEdit()->text(), QString("fooxyz"));
    QVERIFY(!waitingForRegisterIndicator->isVisible());
    TestPressKey("\\enter");
    FinishTest("xyz");

    // Check ctrl-r inserts text at the current cursor position.
    BeginTest("xyz");
    TestPressKey("\"ayiw/foo\\left\\ctrl-ra");
    QCOMPARE(emulatedCommandBarTextEdit()->text(), QString("foxyzo"));
    TestPressKey("\\enter");
    FinishTest("xyz");

    // Check ctrl-r ctrl-w inserts word under the cursor, and hides the "waiting for register" indicator.
    BeginTest("foo bar xyz");
    TestPressKey("w/\\left\\ctrl-r\\ctrl-w");
    QCOMPARE(emulatedCommandBarTextEdit()->text(), QString("bar"));
    QVERIFY(!waitingForRegisterIndicator->isVisible());
    TestPressKey("\\enter");
    FinishTest("foo bar xyz");

    // Check ctrl-r ctrl-w doesn't insert the contents of register w!
    BeginTest("foo baz xyz");
    TestPressKey("\"wyiww/\\left\\ctrl-r\\ctrl-w");
    QCOMPARE(emulatedCommandBarTextEdit()->text(), QString("baz"));
    TestPressKey("\\enter");
    FinishTest("foo baz xyz");

    // Check ctrl-r ctrl-w inserts at the current cursor position.
    BeginTest("foo nose xyz");
    TestPressKey("w/bar\\left\\ctrl-r\\ctrl-w");
    QCOMPARE(emulatedCommandBarTextEdit()->text(), QString("banoser"));
    TestPressKey("\\enter");
    FinishTest("foo nose xyz");

    // Cursor position is at the end of the inserted text after ctrl-r ctrl-w.
    BeginTest("foo nose xyz");
    TestPressKey("w/bar\\left\\ctrl-r\\ctrl-wX");
    QCOMPARE(emulatedCommandBarTextEdit()->text(), QString("banoseXr"));
    TestPressKey("\\enter");
    FinishTest("foo nose xyz");

    // Cursor position is at the end of the inserted register contents after ctrl-r.
    BeginTest("xyz");
    TestPressKey("\"ayiw/foo\\left\\ctrl-raX");
    QCOMPARE(emulatedCommandBarTextEdit()->text(), QString("foxyzXo"));
    TestPressKey("\\enter");
    FinishTest("xyz");

    // Insert clipboard contents on ctrl-r +.  We implicitly need to test the ability to handle
    // shift key key events when waiting for register (they should be ignored).
    BeginTest("xyz");
    QApplication::clipboard()->setText("vimodetestclipboardtext");
    TestPressKey("/\\ctrl-r");
    QKeyEvent *shiftKeyDown = new QKeyEvent(QEvent::KeyPress, Qt::Key_Shift, Qt::NoModifier);
    QApplication::postEvent(emulatedCommandBarTextEdit(), shiftKeyDown);
    QApplication::sendPostedEvents();
    TestPressKey("+");
    QKeyEvent *shiftKeyUp = new QKeyEvent(QEvent::KeyPress, Qt::Key_Shift, Qt::NoModifier);
    QApplication::postEvent(emulatedCommandBarTextEdit(), shiftKeyUp);
    QApplication::sendPostedEvents();
    QCOMPARE(emulatedCommandBarTextEdit()->text(), QString("vimodetestclipboardtext"));
    TestPressKey("\\enter");
    FinishTest("xyz");

    // Similarly, test that we can press "ctrl" after ctrl-r without it being taken for a register.
    BeginTest("wordundercursor");
    TestPressKey("/\\ctrl-r");
    QKeyEvent *ctrlKeyDown = new QKeyEvent(QEvent::KeyPress, Qt::Key_Control, Qt::NoModifier);
    QApplication::postEvent(emulatedCommandBarTextEdit(), ctrlKeyDown);
    QApplication::sendPostedEvents();
    QKeyEvent *ctrlKeyUp = new QKeyEvent(QEvent::KeyRelease, Qt::Key_Control, Qt::NoModifier);
    QApplication::postEvent(emulatedCommandBarTextEdit(), ctrlKeyUp);
    QApplication::sendPostedEvents();
    QVERIFY(waitingForRegisterIndicator->isVisible());
    TestPressKey("\\ctrl-w");
    QCOMPARE(emulatedCommandBarTextEdit()->text(), QString("wordundercursor"));
    TestPressKey("\\ctrl-c"); // Dismiss the bar.
    FinishTest("wordundercursor");

    // Begin tests for ctrl-g, which is almost identical to ctrl-r save that the contents, when added,
    // are escaped for searching.
    // Normal register contents/ word under cursor are added as normal.
    BeginTest("wordinregisterb wordundercursor");
    TestPressKey("\"byiw");
    TestPressKey("/\\ctrl-g");
    QVERIFY(waitingForRegisterIndicator->isVisible());
    QVERIFY(waitingForRegisterIndicator->x() >= emulatedCommandBarTextEdit()->x() + emulatedCommandBarTextEdit()->width());
    TestPressKey("b");
    QCOMPARE(emulatedCommandBarTextEdit()->text(), QString("wordinregisterb"));
    QVERIFY(!waitingForRegisterIndicator->isVisible());
    TestPressKey("\\ctrl-c\\ctrl-cw/\\ctrl-g\\ctrl-w");
    QCOMPARE(emulatedCommandBarTextEdit()->text(), QString("wordundercursor"));
    QVERIFY(!waitingForRegisterIndicator->isVisible());
    TestPressKey("\\ctrl-c");
    TestPressKey("\\ctrl-c");
    FinishTest("wordinregisterb wordundercursor");

    // \'s must be escaped when inserted via ctrl-g.
    DoTest("foo a\\b\\\\c\\\\\\d", "wYb/\\ctrl-g0\\enterrX", "foo X\\b\\\\c\\\\\\d");
    // $'s must be escaped when inserted via ctrl-g.
    DoTest("foo a$b", "wYb/\\ctrl-g0\\enterrX", "foo X$b");
    DoTest("foo a$b$c", "wYb/\\ctrl-g0\\enterrX", "foo X$b$c");
    DoTest("foo a\\$b\\$c", "wYb/\\ctrl-g0\\enterrX", "foo X\\$b\\$c");
    // ^'s must be escaped when inserted via ctrl-g.
    DoTest("foo a^b", "wYb/\\ctrl-g0\\enterrX", "foo X^b");
    DoTest("foo a^b^c", "wYb/\\ctrl-g0\\enterrX", "foo X^b^c");
    DoTest("foo a\\^b\\^c", "wYb/\\ctrl-g0\\enterrX", "foo X\\^b\\^c");
    // .'s must be escaped when inserted via ctrl-g.
    DoTest("foo axb a.b", "wwYgg/\\ctrl-g0\\enterrX", "foo axb X.b");
    DoTest("foo a\\xb Na\\.b", "fNlYgg/\\ctrl-g0\\enterrX", "foo a\\xb NX\\.b");
    // *'s must be escaped when inserted via ctrl-g
    DoTest("foo axxxxb ax*b", "wwYgg/\\ctrl-g0\\enterrX", "foo axxxxb Xx*b");
    DoTest("foo a\\xxxxb Na\\x*X", "fNlYgg/\\ctrl-g0\\enterrX", "foo a\\xxxxb NX\\x*X");
    // /'s must be escaped when inserted via ctrl-g.
    DoTest("foo a a/b", "wwYgg/\\ctrl-g0\\enterrX", "foo a X/b");
    DoTest("foo a a/b/c", "wwYgg/\\ctrl-g0\\enterrX", "foo a X/b/c");
    DoTest("foo a a\\/b\\/c", "wwYgg/\\ctrl-g0\\enterrX", "foo a X\\/b\\/c");
    // ['s and ]'s must be escaped when inserted via ctrl-g.
    DoTest("foo axb a[xyz]b", "wwYgg/\\ctrl-g0\\enterrX", "foo axb X[xyz]b");
    DoTest("foo a[b", "wYb/\\ctrl-g0\\enterrX", "foo X[b");
    DoTest("foo a[b[c", "wYb/\\ctrl-g0\\enterrX", "foo X[b[c");
    DoTest("foo a\\[b\\[c", "wYb/\\ctrl-g0\\enterrX", "foo X\\[b\\[c");
    DoTest("foo a]b", "wYb/\\ctrl-g0\\enterrX", "foo X]b");
    DoTest("foo a]b]c", "wYb/\\ctrl-g0\\enterrX", "foo X]b]c");
    DoTest("foo a\\]b\\]c", "wYb/\\ctrl-g0\\enterrX", "foo X\\]b\\]c");
    // Test that expressions involving {'s and }'s work when inserted via ctrl-g.
    DoTest("foo {", "wYgg/\\ctrl-g0\\enterrX", "foo X");
    DoTest("foo }", "wYgg/\\ctrl-g0\\enterrX", "foo X");
    DoTest("foo aaaaa \\aaaaa a\\{5}", "WWWYgg/\\ctrl-g0\\enterrX", "foo aaaaa \\aaaaa X\\{5}");
    DoTest("foo }", "wYgg/\\ctrl-g0\\enterrX", "foo X");
    // Transform newlines into "\\n" when inserted via ctrl-g.
    DoTest(" \nfoo\nfoo\nxyz\nbar\n123", "jjvjjllygg/\\ctrl-g0\\enterrX", " \nfoo\nXoo\nxyz\nbar\n123");
    DoTest(" \nfoo\nfoo\nxyz\nbar\n123", "jjvjjllygg/\\ctrl-g0/e\\enterrX", " \nfoo\nfoo\nxyz\nbaX\n123");
    // Don't do any escaping for ctrl-r, though.
    BeginTest("foo .*$^\\/");
    TestPressKey("wY/\\ctrl-r0");
    QCOMPARE(emulatedCommandBarTextEdit()->text(), QString(".*$^\\/"));
    TestPressKey("\\ctrl-c");
    TestPressKey("\\ctrl-c");
    FinishTest("foo .*$^\\/");
    // Ensure that the flag that says "next register insertion should be escaped for searching"
    // is cleared if we do ctrl-g but then abort with ctrl-c.
    DoTest("foo a$b", "/\\ctrl-g\\ctrl-c\\ctrl-cwYgg/\\ctrl-r0\\enterrX", "Xoo a$b");

    // Ensure that we actually perform a search while typing.
    BeginTest("abcd");
    TestPressKey("/c");
    verifyCursorAt(Cursor(0, 2));
    TestPressKey("\\enter");
    FinishTest("abcd");

    // Ensure that the search is from the cursor.
    BeginTest("acbcd");
    TestPressKey("ll/c");
    verifyCursorAt(Cursor(0, 3));
    TestPressKey("\\enter");
    FinishTest("acbcd");

    // Reset the cursor to the original position on Ctrl-C
    BeginTest("acbcd");
    TestPressKey("ll/c\\ctrl-crX");
    FinishTest("acXcd");

    // Reset the cursor to the original position on Ctrl-[
    BeginTest("acbcd");
    TestPressKey("ll/c\\ctrl-[rX");
    FinishTest("acXcd");

    // Reset the cursor to the original position on ESC
    BeginTest("acbcd");
    TestPressKey("ll/c\\escrX");
    FinishTest("acXcd");

    // *Do not* reset the cursor to the original position on Enter.
    BeginTest("acbcd");
    TestPressKey("ll/c\\enterrX");
    FinishTest("acbXd");

    // *Do not* reset the cursor to the original position on Return.
    BeginTest("acbcd");
    TestPressKey("ll/c\\returnrX");
    FinishTest("acbXd");

    // Should work with mappings.
    clearAllMappings();
    vi_global->mappings()->add(Mappings::NormalModeMapping, "'testmapping", "/c<enter>rX", Mappings::Recursive);
    BeginTest("acbcd");
    TestPressKey("'testmapping");
    FinishTest("aXbcd");
    clearAllMappings();
    // Don't send keys that were part of a mapping to the emulated command bar.
    vi_global->mappings()->add(Mappings::NormalModeMapping, "H", "/a", Mappings::Recursive);
    BeginTest("foo a aH");
    TestPressKey("H\\enterrX");
    FinishTest("foo X aH");
    clearAllMappings();

    // Incremental searching from the original position.
    BeginTest("foo bar foop fool food");
    TestPressKey("ll/foo");
    verifyCursorAt(Cursor(0, 8));
    TestPressKey("l");
    verifyCursorAt(Cursor(0, 13));
    TestPressKey("\\backspace");
    verifyCursorAt(Cursor(0, 8));
    TestPressKey("\\enter");
    FinishTest("foo bar foop fool food");

    // End up back at the start if no match found
    BeginTest("foo bar foop fool food");
    TestPressKey("ll/fool");
    verifyCursorAt(Cursor(0, 13));
    TestPressKey("\\backspacex");
    verifyCursorAt(Cursor(0, 2));
    TestPressKey("\\enter");
    FinishTest("foo bar foop fool food");

    // Wrap around if no match found.
    BeginTest("afoom bar foop fool food");
    TestPressKey("lll/foom");
    verifyCursorAt(Cursor(0, 1));
    TestPressKey("\\enter");
    FinishTest("afoom bar foop fool food");

    // SmartCase: match case-insensitively if the search text is all lower-case.
    DoTest("foo BaR", "ll/bar\\enterrX", "foo XaR");

    // SmartCase: match case-sensitively if the search text is mixed case.
    DoTest("foo BaR bAr", "ll/bAr\\enterrX", "foo BaR XAr");

    // Assume regex by default.
    DoTest("foo bwibblear", "ll/b.*ar\\enterrX", "foo Xwibblear");

    // Set the last search pattern.
    DoTest("foo bar", "ll/bar\\enterggnrX", "foo Xar");

    // Make sure the last search pattern is a regex, too.
    DoTest("foo bwibblear", "ll/b.*ar\\enterggnrX", "foo Xwibblear");

    // 'n' should search case-insensitively if the original search was case-insensitive.
    DoTest("foo bAR", "ll/bar\\enterggnrX", "foo XAR");

    // 'n' should search case-sensitively if the original search was case-sensitive.
    DoTest("foo bar bAR", "ll/bAR\\enterggnrX", "foo bar XAR");

    // 'N' should search case-insensitively if the original search was case-insensitive.
    DoTest("foo bAR xyz", "ll/bar\\enter$NrX", "foo XAR xyz");

    // 'N' should search case-sensitively if the original search was case-sensitive.
    DoTest("foo bAR bar", "ll/bAR\\enter$NrX", "foo XAR bar");

    // Don't forget to set the last search to case-insensitive.
    DoTest("foo bAR bar", "ll/bAR\\enter^/bar\\enter^nrX", "foo XAR bar");

    // Usage of \C for manually specifying case sensitivity.
    // Strip occurrences of "\C" from the pattern to find.
    DoTest("foo bar", "/\\\\Cba\\\\Cr\\enterrX", "foo Xar");
    // Be careful about escaping, though!
    DoTest("foo \\Cba\\Cr", "/\\\\\\\\Cb\\\\Ca\\\\\\\\C\\\\C\\\\Cr\\enterrX", "foo XCba\\Cr");
    // The item added to the search history should contain all the original \C's.
    clearSearchHistory();
    BeginTest("foo \\Cba\\Cr");
    TestPressKey("/\\\\\\\\Cb\\\\Ca\\\\\\\\C\\\\C\\\\Cr\\enterrX");
    QCOMPARE(searchHistory().first(), QString("\\\\Cb\\Ca\\\\C\\C\\Cr"));
    FinishTest("foo XCba\\Cr");
    // If there is an escaped C, assume case sensitivity.
    DoTest("foo bAr BAr bar", "/ba\\\\Cr\\enterrX", "foo bAr BAr Xar");
    // The last search pattern should be the last search with escaped C's stripped.
    DoTest("foo \\Cbar\nfoo \\Cbar", "/\\\\\\\\Cba\\\\C\\\\Cr\\enterggjnrX", "foo \\Cbar\nfoo XCbar");
    // If the last search pattern had an escaped "\C", then the next search should be case-sensitive.
    DoTest("foo bar\nfoo bAr BAr bar", "/ba\\\\Cr\\enterggjnrX", "foo bar\nfoo bAr BAr Xar");

    // Don't set the last search parameters if we abort, though.
    DoTest("foo bar xyz", "/bar\\enter/xyz\\ctrl-cggnrX", "foo Xar xyz");
    DoTest("foo bar bAr", "/bar\\enter/bA\\ctrl-cggnrX", "foo Xar bAr");
    DoTest("foo bar bar", "/bar\\enter?ba\\ctrl-cggnrX", "foo Xar bar");

    // Don't let ":" trample all over the search parameters, either.
    DoTest("foo bar xyz foo", "/bar\\entergg*:yank\\enterggnrX", "foo bar xyz Xoo");

    // Some mirror tests for "?"

    // Test that "?" summons the search bar, with empty text and with the "?" indicator.
    QVERIFY(!emulatedCommandBar->isVisible());
    BeginTest("");
    TestPressKey("?");
    QVERIFY(emulatedCommandBar->isVisible());
    QCOMPARE(emulatedCommandTypeIndicator()->text(), QString("?"));
    QVERIFY(emulatedCommandTypeIndicator()->isVisible());
    QVERIFY(emulatedCommandBarTextEdit());
    QVERIFY(emulatedCommandBarTextEdit()->text().isEmpty());
    TestPressKey("\\enter");
    FinishTest("");

    // Search backwards.
    DoTest("foo foo bar foo foo", "ww?foo\\enterrX", "foo Xoo bar foo foo");

    // Reset cursor if we find nothing.
    BeginTest("foo foo bar foo foo");
    TestPressKey("ww?foo");
    verifyCursorAt(Cursor(0, 4));
    TestPressKey("d");
    verifyCursorAt(Cursor(0, 8));
    TestPressKey("\\enter");
    FinishTest("foo foo bar foo foo");

    // Wrap to the end if we find nothing.
    DoTest("foo foo bar xyz xyz", "ww?xyz\\enterrX", "foo foo bar xyz Xyz");

    // Specify that the last was backwards when using '?'
    DoTest("foo foo bar foo foo", "ww?foo\\enter^wwnrX", "foo Xoo bar foo foo");

    // ... and make sure we do  the equivalent with "/"
    BeginTest("foo foo bar foo foo");
    TestPressKey("ww?foo\\enter^ww/foo");
    QCOMPARE(emulatedCommandTypeIndicator()->text(), QString("/"));
    TestPressKey("\\enter^wwnrX");
    FinishTest("foo foo bar Xoo foo");

    // If we are at the beginning of a word, that word is not the first match in a search
    // for that word.
    DoTest("foo foo foo", "w/foo\\enterrX", "foo foo Xoo");
    DoTest("foo foo foo", "w?foo\\enterrX", "Xoo foo foo");
    // When searching backwards, ensure we can find a match whose range includes the starting cursor position,
    // if we allow it to wrap around.
    DoTest("foo foofoofoo bar", "wlll?foofoofoo\\enterrX", "foo Xoofoofoo bar");
    // When searching backwards, ensure we can find a match whose range includes the starting cursor position,
    // even if we don't allow it to wrap around.
    DoTest("foo foofoofoo foofoofoo", "wlll?foofoofoo\\enterrX", "foo Xoofoofoo foofoofoo");
    // The same, but where we the match ends at the end of the line or document.
    DoTest("foo foofoofoo\nfoofoofoo", "wlll?foofoofoo\\enterrX", "foo Xoofoofoo\nfoofoofoo");
    DoTest("foo foofoofoo", "wlll?foofoofoo\\enterrX", "foo Xoofoofoo");

    // Searching forwards for just "/" repeats last search.
    DoTest("foo bar", "/bar\\entergg//\\enterrX", "foo Xar");
    // The "last search" can be one initiated via e.g. "*".
    DoTest("foo bar foo", "/bar\\entergg*gg//\\enterrX", "foo bar Xoo");
    // Searching backwards for just "?" repeats last search.
    DoTest("foo bar bar", "/bar\\entergg??\\enterrX", "foo bar Xar");
    // Search forwards treats "?" as a literal.
    DoTest("foo ?ba?r", "/?ba?r\\enterrX", "foo Xba?r");
    // As always, be careful with escaping!
    DoTest("foo ?ba\\?r", "/?ba\\\\\\\\\\\\?r\\enterrX", "foo Xba\\?r");
    // Searching forwards for just "?" finds literal question marks.
    DoTest("foo ??", "/?\\enterrX", "foo X?");
    // Searching backwards for just "/" finds literal forward slashes.
    DoTest("foo //", "?/\\enterrX", "foo /X");
    // Searching forwards, stuff after (and including) an unescaped "/" is ignored.
    DoTest("foo ba bar bar/xyz", "/bar/xyz\\enterrX", "foo ba Xar bar/xyz");
    // Needs to be unescaped, though!
    DoTest("foo bar bar/xyz", "/bar\\\\/xyz\\enterrX", "foo bar Xar/xyz");
    DoTest("foo bar bar\\/xyz", "/bar\\\\\\\\/xyz\\enterrX", "foo bar Xar\\/xyz");
    // Searching backwards, stuff after (and including) an unescaped "?" is ignored.
    DoTest("foo bar bar?xyz bar ba", "?bar?xyz\\enterrX", "foo bar bar?xyz Xar ba");
    // Needs to be unescaped, though!
    DoTest("foo bar bar?xyz bar ba", "?bar\\\\?xyz\\enterrX", "foo bar Xar?xyz bar ba");
    DoTest("foo bar bar\\?xyz bar ba", "?bar\\\\\\\\?xyz\\enterrX", "foo bar Xar\\?xyz bar ba");
    // If, in a forward search, the first character after the first unescaped "/" is an e, then
    // we place the cursor at the end of the word.
    DoTest("foo ba bar bar/eyz", "/bar/e\\enterrX", "foo ba baX bar/eyz");
    // Needs to be unescaped, though!
    DoTest("foo bar bar/eyz", "/bar\\\\/e\\enterrX", "foo bar Xar/eyz");
    DoTest("foo bar bar\\/xyz", "/bar\\\\\\\\/e\\enterrX", "foo bar barX/xyz");
    // If, in a backward search, the first character after the first unescaped "?" is an e, then
    // we place the cursor at the end of the word.
    DoTest("foo bar bar?eyz bar ba", "?bar?e\\enterrX", "foo bar bar?eyz baX ba");
    // Needs to be unescaped, though!
    DoTest("foo bar bar?eyz bar ba", "?bar\\\\?e\\enterrX", "foo bar Xar?eyz bar ba");
    DoTest("foo bar bar\\?eyz bar ba", "?bar\\\\\\\\?e\\enterrX", "foo bar barX?eyz bar ba");
    // Quick check that repeating the last search and placing the cursor at the end of the match works.
    DoTest("foo bar bar", "/bar\\entergg//e\\enterrX", "foo baX bar");
    DoTest("foo bar bar", "?bar\\entergg??e\\enterrX", "foo bar baX");
    // When repeating a change, don't try to convert from Vim to Qt regex again.
    DoTest("foo bar()", "/bar()\\entergg//e\\enterrX", "foo bar(X");
    DoTest("foo bar()", "?bar()\\entergg??e\\enterrX", "foo bar(X");
    // If the last search said that we should place the cursor at the end of the match, then
    // do this with n & N.
    DoTest("foo bar bar foo", "/bar/e\\enterggnrX", "foo baX bar foo");
    DoTest("foo bar bar foo", "/bar/e\\enterggNrX", "foo bar baX foo");
    // Don't do this if that search was aborted, though.
    DoTest("foo bar bar foo", "/bar\\enter/bar/e\\ctrl-cggnrX", "foo Xar bar foo");
    DoTest("foo bar bar foo", "/bar\\enter/bar/e\\ctrl-cggNrX", "foo bar Xar foo");
    // "#" and "*" reset the "place cursor at the end of the match" to false.
    DoTest("foo bar bar foo", "/bar/e\\enterggw*nrX", "foo Xar bar foo");
    DoTest("foo bar bar foo", "/bar/e\\enterggw#nrX", "foo Xar bar foo");

    // "/" and "?" should be usable as motions.
    DoTest("foo bar", "ld/bar\\enter", "fbar");
    // They are not linewise.
    DoTest("foo bar\nxyz", "ld/yz\\enter", "fyz");
    DoTest("foo bar\nxyz", "jld?oo\\enter", "fyz");
    // Should be usable in Visual Mode without aborting Visual Mode.
    DoTest("foo bar", "lv/bar\\enterd", "far");
    // Same for ?.
    DoTest("foo bar", "$hd?oo\\enter", "far");
    DoTest("foo bar", "$hv?oo\\enterd", "fr");
    DoTest("foo bar", "lv?bar\\enterd", "far");
    // If we abort the "/" / "?" motion, the command should be aborted, too.
    DoTest("foo bar", "d/bar\\esc", "foo bar");
    DoTest("foo bar", "d/bar\\ctrl-c", "foo bar");
    DoTest("foo bar", "d/bar\\ctrl-[", "foo bar");
    // We should be able to repeat a command using "/" or "?" as the motion.
    DoTest("foo bar bar bar", "d/bar\\enter.", "bar bar");
    // The "synthetic" Enter keypress should not be logged as part of the command to be repeated.
    DoTest("foo bar bar bar\nxyz", "d/bar\\enter.rX", "Xar bar\nxyz");
    // Counting.
    DoTest("foo bar bar bar", "2/bar\\enterrX", "foo bar Xar bar");
    // Counting with wraparound.
    DoTest("foo bar bar bar", "4/bar\\enterrX", "foo Xar bar bar");
    // Counting in Visual Mode.
    DoTest("foo bar bar bar", "v2/bar\\enterd", "ar bar");
    // Should update the selection in Visual Mode as we search.
    BeginTest("foo bar bbc");
    TestPressKey("vl/b");
    QCOMPARE(kate_view->selectionText(), QString("foo b"));
    TestPressKey("b");
    QCOMPARE(kate_view->selectionText(), QString("foo bar b"));
    TestPressKey("\\ctrl-h");
    QCOMPARE(kate_view->selectionText(), QString("foo b"));
    TestPressKey("notexists");
    QCOMPARE(kate_view->selectionText(), QString("fo"));
    TestPressKey("\\enter"); // Dismiss bar.
    QCOMPARE(kate_view->selectionText(), QString("fo"));
    FinishTest("foo bar bbc");
    BeginTest("foo\nxyz\nbar\nbbc");
    TestPressKey("Vj/b");
    QCOMPARE(kate_view->selectionText(), QString("foo\nxyz\nbar"));
    TestPressKey("b");
    QCOMPARE(kate_view->selectionText(), QString("foo\nxyz\nbar\nbbc"));
    TestPressKey("\\ctrl-h");
    QCOMPARE(kate_view->selectionText(), QString("foo\nxyz\nbar"));
    TestPressKey("notexists");
    QCOMPARE(kate_view->selectionText(), QString("foo\nxyz"));
    TestPressKey("\\ctrl-c"); // Dismiss bar.
    FinishTest("foo\nxyz\nbar\nbbc");
    // Dismissing the search bar in visual mode should leave original selection.
    BeginTest("foo bar bbc");
    TestPressKey("vl/\\ctrl-c");
    QCOMPARE(kate_view->selectionText(), QString("fo"));
    FinishTest("foo bar bbc");
    BeginTest("foo bar bbc");
    TestPressKey("vl?\\ctrl-c");
    QCOMPARE(kate_view->selectionText(), QString("fo"));
    FinishTest("foo bar bbc");
    BeginTest("foo bar bbc");
    TestPressKey("vl/b\\ctrl-c");
    QCOMPARE(kate_view->selectionText(), QString("fo"));
    FinishTest("foo bar bbc");
    BeginTest("foo\nbar\nbbc");
    TestPressKey("Vl/b\\ctrl-c");
    QCOMPARE(kate_view->selectionText(), QString("foo"));
    FinishTest("foo\nbar\nbbc");

    // Search-highlighting tests.
    const QColor searchHighlightColour = kate_view->renderer()->config()->searchHighlightColor();
    BeginTest("foo bar xyz");
    // Sanity test.
    const QVector<Kate::TextRange *> rangesInitial = rangesOnFirstLine();
    Q_ASSERT(rangesInitial.isEmpty() && "Assumptions about ranges are wrong - this test is invalid and may need updating!");
    FinishTest("foo bar xyz");

    // Test highlighting single character match.
    BeginTest("foo bar xyz");
    TestPressKey("/b");
    QCOMPARE(rangesOnFirstLine().size(), rangesInitial.size() + 1);
    QCOMPARE(rangesOnFirstLine().first()->attribute()->background().color(), searchHighlightColour);
    QCOMPARE(rangesOnFirstLine().first()->start().line(), 0);
    QCOMPARE(rangesOnFirstLine().first()->start().column(), 4);
    QCOMPARE(rangesOnFirstLine().first()->end().line(), 0);
    QCOMPARE(rangesOnFirstLine().first()->end().column(), 5);
    TestPressKey("\\enter");
    FinishTest("foo bar xyz");

    // Test highlighting two character match.
    BeginTest("foo bar xyz");
    TestPressKey("/ba");
    QCOMPARE(rangesOnFirstLine().size(), rangesInitial.size() + 1);
    QCOMPARE(rangesOnFirstLine().first()->start().line(), 0);
    QCOMPARE(rangesOnFirstLine().first()->start().column(), 4);
    QCOMPARE(rangesOnFirstLine().first()->end().line(), 0);
    QCOMPARE(rangesOnFirstLine().first()->end().column(), 6);
    TestPressKey("\\enter");
    FinishTest("foo bar xyz");

    // Test no highlighting if no longer a match.
    BeginTest("foo bar xyz");
    TestPressKey("/baz");
    QCOMPARE(rangesOnFirstLine().size(), rangesInitial.size());
    TestPressKey("\\enter");
    FinishTest("foo bar xyz");

    // Test highlighting on wraparound.
    BeginTest(" foo bar xyz");
    TestPressKey("ww/foo");
    QCOMPARE(rangesOnFirstLine().size(), rangesInitial.size() + 1);
    QCOMPARE(rangesOnFirstLine().first()->start().line(), 0);
    QCOMPARE(rangesOnFirstLine().first()->start().column(), 1);
    QCOMPARE(rangesOnFirstLine().first()->end().line(), 0);
    QCOMPARE(rangesOnFirstLine().first()->end().column(), 4);
    TestPressKey("\\enter");
    FinishTest(" foo bar xyz");

    // Test highlighting backwards
    BeginTest("foo bar xyz");
    TestPressKey("$?ba");
    QCOMPARE(rangesOnFirstLine().size(), rangesInitial.size() + 1);
    QCOMPARE(rangesOnFirstLine().first()->start().line(), 0);
    QCOMPARE(rangesOnFirstLine().first()->start().column(), 4);
    QCOMPARE(rangesOnFirstLine().first()->end().line(), 0);
    QCOMPARE(rangesOnFirstLine().first()->end().column(), 6);
    TestPressKey("\\enter");
    FinishTest("foo bar xyz");

    // Test no highlighting when no match is found searching backwards
    BeginTest("foo bar xyz");
    TestPressKey("$?baz");
    QCOMPARE(rangesOnFirstLine().size(), rangesInitial.size());
    TestPressKey("\\enter");
    FinishTest("foo bar xyz");

    // Test highlight when wrapping around after searching backwards.
    BeginTest("foo bar xyz");
    TestPressKey("w?xyz");
    QCOMPARE(rangesOnFirstLine().size(), rangesInitial.size() + 1);
    QCOMPARE(rangesOnFirstLine().first()->start().line(), 0);
    QCOMPARE(rangesOnFirstLine().first()->start().column(), 8);
    QCOMPARE(rangesOnFirstLine().first()->end().line(), 0);
    QCOMPARE(rangesOnFirstLine().first()->end().column(), 11);
    TestPressKey("\\enter");
    FinishTest("foo bar xyz");

    // Test no highlighting when bar is dismissed.
    DoTest("foo bar xyz", "/bar\\ctrl-c", "foo bar xyz");
    QCOMPARE(rangesOnFirstLine().size(), rangesInitial.size());
    DoTest("foo bar xyz", ":set-nohls\\enter/bar\\enter", "foo bar xyz");
    QCOMPARE(rangesOnFirstLine().size(), rangesInitial.size());
    DoTest("foo bar xyz", "/bar\\ctrl-[", "foo bar xyz");
    QCOMPARE(rangesOnFirstLine().size(), rangesInitial.size());
    DoTest("foo bar xyz", ":set-nohls\\enter/bar\\return", "foo bar xyz");
    QCOMPARE(rangesOnFirstLine().size(), rangesInitial.size());
    DoTest("foo bar xyz", "/bar\\esc", "foo bar xyz");
    QCOMPARE(rangesOnFirstLine().size(), rangesInitial.size());

    // Update colour on config change.
    BeginTest("foo bar xyz");
    TestPressKey("/xyz");
    const QColor newSearchHighlightColour = QColor(255, 0, 0);
    kate_view->renderer()->config()->setSearchHighlightColor(newSearchHighlightColour);
    QCOMPARE(rangesOnFirstLine().size(), rangesInitial.size() + 1);
    QCOMPARE(rangesOnFirstLine().first()->attribute()->background().color(), newSearchHighlightColour);
    TestPressKey("\\enter");
    FinishTest("foo bar xyz");

    // Set the background colour appropriately.
    KColorScheme currentColorScheme(QPalette::Normal);
    const QColor normalBackgroundColour = QPalette().brush(QPalette::Base).color();
    const QColor matchBackgroundColour = currentColorScheme.background(KColorScheme::PositiveBackground).color();
    const QColor noMatchBackgroundColour = currentColorScheme.background(KColorScheme::NegativeBackground).color();
    BeginTest("foo bar xyz");
    TestPressKey("/xyz");
    verifyTextEditBackgroundColour(matchBackgroundColour);
    TestPressKey("a");
    verifyTextEditBackgroundColour(noMatchBackgroundColour);
    TestPressKey("\\ctrl-w");
    verifyTextEditBackgroundColour(normalBackgroundColour);
    TestPressKey("/xyz\\enter/");
    verifyTextEditBackgroundColour(normalBackgroundColour);
    TestPressKey("\\enter");
    FinishTest("foo bar xyz");

    // Escape regex's in a Vim-ish style.
    // Unescaped ( and ) are always literals.
    DoTest("foo bar( xyz", "/bar(\\enterrX", "foo Xar( xyz");
    DoTest("foo bar) xyz", "/bar)\\enterrX", "foo Xar) xyz");
    // + is literal, unless it is already escaped.
    DoTest("foo bar+ xyz", "/bar+ \\enterrX", "foo Xar+ xyz");
    DoTest("  foo+AAAAbar", "/foo+A\\\\+bar\\enterrX", "  Xoo+AAAAbar");
    DoTest("  foo++++bar", "/foo+\\\\+bar\\enterrX", "  Xoo++++bar");
    DoTest("  foo++++bar", "/+\\enterrX", "  fooX+++bar");
    // An escaped "\" is a literal, of course.
    DoTest("foo x\\y", "/x\\\\\\\\y\\enterrX", "foo X\\y");
    // ( and ), if escaped, are not literals.
    DoTest("foo  barbarxyz", "/ \\\\(bar\\\\)\\\\+xyz\\enterrX", "foo Xbarbarxyz");
    // Handle escaping correctly if we have an escaped and unescaped bracket next to each other.
    DoTest("foo  x(A)y", "/x(\\\\(.\\\\))y\\enterrX", "foo  X(A)y");
    // |, if unescaped, is literal.
    DoTest("foo |bar", "/|\\enterrX", "foo Xbar");
    // |, if escaped, is not a literal.
    DoTest("foo xfoo\\y xbary", "/x\\\\(foo\\\\|bar\\\\)y\\enterrX", "foo xfoo\\y Xbary");
    // A single [ is a literal.
    DoTest("foo bar[", "/bar[\\enterrX", "foo Xar[");
    // A single ] is a literal.
    DoTest("foo bar]", "/bar]\\enterrX", "foo Xar]");
    // A matching [ and ] are *not* literals.
    DoTest("foo xbcay", "/x[abc]\\\\+y\\enterrX", "foo Xbcay");
    DoTest("foo xbcay", "/[abc]\\\\+y\\enterrX", "foo xXcay");
    DoTest("foo xbaadcdcy", "/x[ab]\\\\+[cd]\\\\+y\\enterrX", "foo Xbaadcdcy");
    // Need to be an unescaped match, though.
    DoTest("foo xbcay", "/x[abc\\\\]\\\\+y\\enterrX", "Xoo xbcay");
    DoTest("foo xbcay", "/x\\\\[abc]\\\\+y\\enterrX", "Xoo xbcay");
    DoTest("foo x[abc]]]]]y", "/x\\\\[abc]\\\\+y\\enterrX", "foo X[abc]]]]]y");
    // An escaped '[' between matching unescaped '[' and ']' is treated as a literal '['
    DoTest("foo xb[cay", "/x[a\\\\[bc]\\\\+y\\enterrX", "foo Xb[cay");
    // An escaped ']' between matching unescaped '[' and ']' is treated as a literal ']'
    DoTest("foo xb]cay", "/x[a\\\\]bc]\\\\+y\\enterrX", "foo Xb]cay");
    // An escaped '[' not between other square brackets is a literal.
    DoTest("foo xb[cay", "/xb\\\\[\\enterrX", "foo Xb[cay");
    DoTest("foo xb[cay", "/\\\\[ca\\enterrX", "foo xbXcay");
    // An escaped ']' not between other square brackets is a literal.
    DoTest("foo xb]cay", "/xb\\\\]\\enterrX", "foo Xb]cay");
    DoTest("foo xb]cay", "/\\\\]ca\\enterrX", "foo xbXcay");
    // An unescaped '[' not between other square brackets is a literal.
    DoTest("foo xbaba[y", "/x[ab]\\\\+[y\\enterrX", "foo Xbaba[y");
    DoTest("foo xbaba[dcdcy", "/x[ab]\\\\+[[cd]\\\\+y\\enterrX", "foo Xbaba[dcdcy");
    // An unescaped ']' not between other square brackets is a literal.
    DoTest("foo xbaba]y", "/x[ab]\\\\+]y\\enterrX", "foo Xbaba]y");
    DoTest("foo xbaba]dcdcy", "/x[ab]\\\\+][cd]\\\\+y\\enterrX", "foo Xbaba]dcdcy");
    // Be more clever about how we identify escaping: the presence of a preceding
    // backslash is not always sufficient!
    DoTest("foo x\\babay", "/x\\\\\\\\[ab]\\\\+y\\enterrX", "foo X\\babay");
    DoTest("foo x\\[abc]]]]y", "/x\\\\\\\\\\\\[abc]\\\\+y\\enterrX", "foo X\\[abc]]]]y");
    DoTest("foo xa\\b\\c\\y", "/x[abc\\\\\\\\]\\\\+y\\enterrX", "foo Xa\\b\\c\\y");
    DoTest("foo x[abc\\]]]]y", "/x[abc\\\\\\\\\\\\]\\\\+y\\enterrX", "foo X[abc\\]]]]y");
    DoTest("foo xa[\\b\\[y", "/x[ab\\\\\\\\[]\\\\+y\\enterrX", "foo Xa[\\b\\[y");
    DoTest("foo x\\[y", "/x\\\\\\\\[y\\enterrX", "foo X\\[y");
    DoTest("foo x\\]y", "/x\\\\\\\\]y\\enterrX", "foo X\\]y");
    DoTest("foo x\\+y", "/x\\\\\\\\+y\\enterrX", "foo X\\+y");
    // A dot is not a literal, nor is a star.
    DoTest("foo bar", "/o.*b\\enterrX", "fXo bar");
    // Escaped dots and stars are literals, though.
    DoTest("foo xay x.y", "/x\\\\.y\\enterrX", "foo xay X.y");
    DoTest("foo xaaaay xa*y", "/xa\\\\*y\\enterrX", "foo xaaaay Xa*y");
    // Unescaped curly braces are literals.
    DoTest("foo x{}y", "/x{}y\\enterrX", "foo X{}y");
    // Escaped curly brackets are quantifers.
    DoTest("foo xaaaaay", "/xa\\\\{5\\\\}y\\enterrX", "foo Xaaaaay");
    // Matching curly brackets where only the first is escaped are also quantifiers.
    DoTest("foo xaaaaaybbbz", "/xa\\\\{5}yb\\\\{3}z\\enterrX", "foo Xaaaaaybbbz");
    // Make sure it really is escaped, though!
    DoTest("foo xa\\{5}", "/xa\\\\\\\\{5}\\enterrX", "foo Xa\\{5}");
    // Don't crash if the first character is a }
    DoTest("foo aaaaay", "/{\\enterrX", "Xoo aaaaay");
    // Vim's '\<' and '\>' map, roughly, to Qt's '\b'
    DoTest("foo xbar barx bar", "/bar\\\\>\\enterrX", "foo xXar barx bar");
    DoTest("foo xbar barx bar", "/\\\\<bar\\enterrX", "foo xbar Xarx bar");
    DoTest("foo xbar barx bar ", "/\\\\<bar\\\\>\\enterrX", "foo xbar barx Xar ");
    DoTest("foo xbar barx bar", "/\\\\<bar\\\\>\\enterrX", "foo xbar barx Xar");
    DoTest("foo xbar barx\nbar", "/\\\\<bar\\\\>\\enterrX", "foo xbar barx\nXar");
    // Escaped "^" and "$" are treated as literals.
    DoTest("foo x^$y", "/x\\\\^\\\\$y\\enterrX", "foo X^$y");
    // Ensure that it is the escaped version of the pattern that is recorded as the last search pattern.
    DoTest("foo bar( xyz", "/bar(\\enterggnrX", "foo Xar( xyz");

    // Don't log keypresses sent to the emulated command bar as commands to be repeated via "."!
    DoTest("foo", "/diw\\enterciwbar\\ctrl-c.", "bar");

    // Don't leave Visual mode on aborting a search.
    DoTest("foo bar", "vw/\\ctrl-cd", "ar");
    DoTest("foo bar", "vw/\\ctrl-[d", "ar");

    // Don't crash on leaving Visual Mode on aborting a search. This is perhaps the most opaque regression
    // test ever; what it's testing for is the situation where the synthetic keypress issue by the emulated
    // command bar on the "ctrl-[" is sent to the key mapper.  This in turn converts it into a weird character
    // which is then, upon not being recognised as part of a mapping, sent back around the keypress processing,
    // where it ends up being sent to the emulated command bar's text edit, which in turn issues a "text changed"
    // event where the text is still empty, which tries to move the cursor to (-1, -1), which causes a crash deep
    // within Kate. So, in a nutshell: this test ensures that the keymapper never handles the synthetic keypress :)
    DoTest("", "ifoo\\ctrl-cv/\\ctrl-[", "foo");

    // History auto-completion tests.
    clearSearchHistory();
    QVERIFY(searchHistory().isEmpty());
    vi_global->searchHistory()->append("foo");
    vi_global->searchHistory()->append("bar");
    QCOMPARE(searchHistory(),
             QStringList() << "foo"
                           << "bar");
    clearSearchHistory();
    QVERIFY(searchHistory().isEmpty());

    // Ensure current search bar text is added to the history if we press enter.
    DoTest("foo bar", "/bar\\enter", "foo bar");
    DoTest("foo bar", "/xyz\\enter", "foo bar");
    QCOMPARE(searchHistory(),
             QStringList() << "bar"
                           << "xyz");
    // Interesting - Vim adds the search bar text to the history even if we abort via e.g. ctrl-c, ctrl-[, etc.
    clearSearchHistory();
    DoTest("foo bar", "/baz\\ctrl-[", "foo bar");
    QCOMPARE(searchHistory(), QStringList() << "baz");
    clearSearchHistory();
    DoTest("foo bar", "/foo\\esc", "foo bar");
    QCOMPARE(searchHistory(), QStringList() << "foo");
    clearSearchHistory();
    DoTest("foo bar", "/nose\\ctrl-c", "foo bar");
    QCOMPARE(searchHistory(), QStringList() << "nose");

    clearSearchHistory();
    vi_global->searchHistory()->append("foo");
    vi_global->searchHistory()->append("bar");
    QVERIFY(emulatedCommandBarCompleter() != nullptr);
    BeginTest("foo bar");
    TestPressKey("/\\ctrl-p");
    verifyCommandBarCompletionVisible();
    // Make sure the completion appears in roughly the correct place: this is a little fragile :/
    const QPoint completerRectTopLeft = emulatedCommandBarCompleter()->popup()->mapToGlobal(emulatedCommandBarCompleter()->popup()->rect().topLeft());
    const QPoint barEditBottomLeft = emulatedCommandBarTextEdit()->mapToGlobal(emulatedCommandBarTextEdit()->rect().bottomLeft());
    QCOMPARE(completerRectTopLeft.x(), barEditBottomLeft.x());
    QVERIFY(qAbs(completerRectTopLeft.y() - barEditBottomLeft.y()) <= 1);
    // Will activate the current completion item, activating the search, and dismissing the bar.
    TestPressKey("\\enter");
    QVERIFY(!emulatedCommandBarCompleter()->popup()->isVisible());
    // Close the command bar.
    FinishTest("foo bar");

    // Don't show completion with an empty search bar.
    clearSearchHistory();
    vi_global->searchHistory()->append("foo");
    BeginTest("foo bar");
    TestPressKey("/");
    QVERIFY(!emulatedCommandBarCompleter()->popup()->isVisible());
    TestPressKey("\\enter");
    FinishTest("foo bar");

    // Don't auto-complete, either.
    clearSearchHistory();
    vi_global->searchHistory()->append("foo");
    BeginTest("foo bar");
    TestPressKey("/f");
    QVERIFY(!emulatedCommandBarCompleter()->popup()->isVisible());
    TestPressKey("\\enter");
    FinishTest("foo bar");

    clearSearchHistory();
    vi_global->searchHistory()->append("xyz");
    vi_global->searchHistory()->append("bar");
    QVERIFY(emulatedCommandBarCompleter() != nullptr);
    BeginTest("foo bar");
    TestPressKey("/\\ctrl-p");
    QCOMPARE(emulatedCommandBarCompleter()->currentCompletion(), QString("bar"));
    TestPressKey("\\enter"); // Dismiss bar.
    FinishTest("foo bar");

    clearSearchHistory();
    vi_global->searchHistory()->append("xyz");
    vi_global->searchHistory()->append("bar");
    vi_global->searchHistory()->append("foo");
    QVERIFY(emulatedCommandBarCompleter() != nullptr);
    BeginTest("foo bar");
    TestPressKey("/\\ctrl-p");
    QCOMPARE(emulatedCommandBarTextEdit()->text(), QString("foo"));
    QCOMPARE(emulatedCommandBarCompleter()->currentCompletion(), QString("foo"));
    QCOMPARE(emulatedCommandBarCompleter()->popup()->currentIndex().row(), 0);
    TestPressKey("\\ctrl-p");
    QCOMPARE(emulatedCommandBarTextEdit()->text(), QString("bar"));
    QCOMPARE(emulatedCommandBarCompleter()->currentCompletion(), QString("bar"));
    QCOMPARE(emulatedCommandBarCompleter()->popup()->currentIndex().row(), 1);
    TestPressKey("\\ctrl-p");
    QCOMPARE(emulatedCommandBarTextEdit()->text(), QString("xyz"));
    QCOMPARE(emulatedCommandBarCompleter()->currentCompletion(), QString("xyz"));
    QCOMPARE(emulatedCommandBarCompleter()->popup()->currentIndex().row(), 2);
    TestPressKey("\\ctrl-p");
    QCOMPARE(emulatedCommandBarTextEdit()->text(), QString("foo"));
    QCOMPARE(emulatedCommandBarCompleter()->currentCompletion(), QString("foo")); // Wrap-around
    QCOMPARE(emulatedCommandBarCompleter()->popup()->currentIndex().row(), 0);
    TestPressKey("\\enter"); // Dismiss bar.
    FinishTest("foo bar");

    clearSearchHistory();
    vi_global->searchHistory()->append("xyz");
    vi_global->searchHistory()->append("bar");
    vi_global->searchHistory()->append("foo");
    QVERIFY(emulatedCommandBarCompleter() != nullptr);
    BeginTest("foo bar");
    TestPressKey("/\\ctrl-n");
    verifyCommandBarCompletionVisible();
    QCOMPARE(emulatedCommandBarTextEdit()->text(), QString("xyz"));
    QCOMPARE(emulatedCommandBarCompleter()->currentCompletion(), QString("xyz"));
    QCOMPARE(emulatedCommandBarCompleter()->popup()->currentIndex().row(), 2);
    TestPressKey("\\ctrl-n");
    QCOMPARE(emulatedCommandBarTextEdit()->text(), QString("bar"));
    QCOMPARE(emulatedCommandBarCompleter()->currentCompletion(), QString("bar"));
    QCOMPARE(emulatedCommandBarCompleter()->popup()->currentIndex().row(), 1);
    TestPressKey("\\ctrl-n");
    QCOMPARE(emulatedCommandBarTextEdit()->text(), QString("foo"));
    QCOMPARE(emulatedCommandBarCompleter()->currentCompletion(), QString("foo"));
    QCOMPARE(emulatedCommandBarCompleter()->popup()->currentIndex().row(), 0);
    TestPressKey("\\ctrl-n");
    QCOMPARE(emulatedCommandBarTextEdit()->text(), QString("xyz"));
    QCOMPARE(emulatedCommandBarCompleter()->currentCompletion(), QString("xyz")); // Wrap-around.
    QCOMPARE(emulatedCommandBarCompleter()->popup()->currentIndex().row(), 2);
    TestPressKey("\\enter"); // Dismiss bar.
    FinishTest("foo bar");

    clearSearchHistory();
    vi_global->searchHistory()->append("xyz");
    vi_global->searchHistory()->append("bar");
    vi_global->searchHistory()->append("foo");
    BeginTest("foo bar");
    TestPressKey("/\\ctrl-n\\ctrl-n");
    QCOMPARE(emulatedCommandBarTextEdit()->text(), QString("bar"));
    TestPressKey("\\enter"); // Dismiss bar.
    FinishTest("foo bar");

    // If we add something to the history, remove any earliest occurrences (this is what Vim appears to do)
    // and append to the end.
    clearSearchHistory();
    vi_global->searchHistory()->append("bar");
    vi_global->searchHistory()->append("xyz");
    vi_global->searchHistory()->append("foo");
    vi_global->searchHistory()->append("xyz");
    QCOMPARE(searchHistory(),
             QStringList() << "bar"
                           << "foo"
                           << "xyz");

    // Push out older entries if we have too many search items in the history.
    const int HISTORY_SIZE_LIMIT = 100;
    clearSearchHistory();
    for (int i = 1; i <= HISTORY_SIZE_LIMIT; i++) {
        vi_global->searchHistory()->append(QString("searchhistoryitem %1").arg(i));
    }
    QCOMPARE(searchHistory().size(), HISTORY_SIZE_LIMIT);
    QCOMPARE(searchHistory().first(), QString("searchhistoryitem 1"));
    QCOMPARE(searchHistory().last(), QString("searchhistoryitem 100"));
    vi_global->searchHistory()->append(QString("searchhistoryitem %1").arg(HISTORY_SIZE_LIMIT + 1));
    QCOMPARE(searchHistory().size(), HISTORY_SIZE_LIMIT);
    QCOMPARE(searchHistory().first(), QString("searchhistoryitem 2"));
    QCOMPARE(searchHistory().last(), QString("searchhistoryitem %1").arg(HISTORY_SIZE_LIMIT + 1));

    // Don't add empty searches to the history.
    clearSearchHistory();
    DoTest("foo bar", "/\\enter", "foo bar");
    QVERIFY(searchHistory().isEmpty());

    // "*" and "#" should add the relevant word to the search history, enclosed between \< and \>
    clearSearchHistory();
    BeginTest("foo bar");
    TestPressKey("*");
    QVERIFY(!searchHistory().isEmpty());
    QCOMPARE(searchHistory().last(), QString("\\<foo\\>"));
    TestPressKey("w#");
    QCOMPARE(searchHistory().size(), 2);
    QCOMPARE(searchHistory().last(), QString("\\<bar\\>"));

    // Auto-complete words from the document on ctrl-space.
    // Test that we can actually find a single word and add it to the list of completions.
    BeginTest("foo");
    TestPressKey("/\\ctrl- ");
    verifyCommandBarCompletionVisible();
    QCOMPARE(emulatedCommandBarCompleter()->currentCompletion(), QString("foo"));
    TestPressKey("\\enter\\enter"); // Dismiss completion, then bar.
    FinishTest("foo");

    // Count digits and underscores as being part of a word.
    BeginTest("foo_12");
    TestPressKey("/\\ctrl- ");
    verifyCommandBarCompletionVisible();
    QCOMPARE(emulatedCommandBarCompleter()->currentCompletion(), QString("foo_12"));
    TestPressKey("\\enter\\enter"); // Dismiss completion, then bar.
    FinishTest("foo_12");

    // This feels a bit better to me, usability-wise: in the special case of completion from document, where
    // the completion list is manually summoned, allow one to press Enter without the bar being dismissed
    // (just dismiss the completion list instead).
    BeginTest("foo_12");
    TestPressKey("/\\ctrl- \\ctrl-p\\enter");
    QVERIFY(emulatedCommandBar->isVisible());
    QVERIFY(!emulatedCommandBarCompleter()->popup()->isVisible());
    TestPressKey("\\enter"); // Dismiss bar.
    FinishTest("foo_12");

    // Check that we can find multiple words on one line.
    BeginTest("bar (foo) [xyz]");
    TestPressKey("/\\ctrl- ");
    QStringListModel *completerStringListModel = dynamic_cast<QStringListModel *>(emulatedCommandBarCompleter()->model());
    Q_ASSERT(completerStringListModel);
    QCOMPARE(completerStringListModel->stringList(),
             QStringList() << "bar"
                           << "foo"
                           << "xyz");
    TestPressKey("\\enter\\enter"); // Dismiss completion, then bar.
    FinishTest("bar (foo) [xyz]");

    // Check that we arrange the found words in case-insensitive sorted order.
    BeginTest("D c e a b f");
    TestPressKey("/\\ctrl- ");
    verifyCommandBarCompletionsMatches(QStringList() << "a"
                                                     << "b"
                                                     << "c"
                                                     << "D"
                                                     << "e"
                                                     << "f");
    TestPressKey("\\enter\\enter"); // Dismiss completion, then bar.
    FinishTest("D c e a b f");

    // Check that we don't include the same word multiple times.
    BeginTest("foo bar bar bar foo");
    TestPressKey("/\\ctrl- ");
    verifyCommandBarCompletionsMatches(QStringList() << "bar"
                                                     << "foo");
    TestPressKey("\\enter\\enter"); // Dismiss completion, then bar.
    FinishTest("foo bar bar bar foo");

    // Check that we search only a narrow portion of the document, around the cursor (4096 lines either
    // side, say).
    QStringList manyLines;
    for (int i = 1; i < 2 * 4096 + 3; i++) {
        // Pad the digits so that when sorted alphabetically, they are also sorted numerically.
        manyLines << QString("word%1").arg(i, 5, 10, QChar('0'));
    }
    QStringList allButFirstAndLastOfManyLines = manyLines;
    allButFirstAndLastOfManyLines.removeFirst();
    allButFirstAndLastOfManyLines.removeLast();

    BeginTest(manyLines.join("\n"));
    TestPressKey("4097j/\\ctrl- ");
    verifyCommandBarCompletionsMatches(allButFirstAndLastOfManyLines);
    TestPressKey("\\enter\\enter"); // Dismiss completion, then bar.
    FinishTest(manyLines.join("\n"));

    // "The current word" means the word before the cursor in the command bar, and includes numbers
    // and underscores. Make sure also that the completion prefix is set when the completion is first invoked.
    BeginTest("foo fee foa_11 foa_11b");
    // Write "bar(foa112$nose" and position cursor before the "2", then invoke completion.
    TestPressKey("/bar(foa_112$nose\\left\\left\\left\\left\\left\\left\\ctrl- ");
    verifyCommandBarCompletionsMatches(QStringList() << "foa_11"
                                                     << "foa_11b");
    TestPressKey("\\enter\\enter"); // Dismiss completion, then bar.
    FinishTest("foo fee foa_11 foa_11b");

    // But don't count "-" as being part of the current word.
    BeginTest("foo_12");
    TestPressKey("/bar-foo\\ctrl- ");
    verifyCommandBarCompletionVisible();
    QCOMPARE(emulatedCommandBarCompleter()->currentCompletion(), QString("foo_12"));
    TestPressKey("\\enter\\enter"); // Dismiss completion, then bar.
    FinishTest("foo_12");

    // Be case insensitive.
    BeginTest("foo Fo12 fOo13 FO45");
    TestPressKey("/fo\\ctrl- ");
    verifyCommandBarCompletionsMatches(QStringList() << "Fo12"
                                                     << "FO45"
                                                     << "foo"
                                                     << "fOo13");
    TestPressKey("\\enter\\enter"); // Dismiss completion, then bar.
    FinishTest("foo Fo12 fOo13 FO45");

    // Feed the current word to complete to the completer as we type/ edit.
    BeginTest("foo fee foa foab");
    TestPressKey("/xyz|f\\ctrl- o");
    verifyCommandBarCompletionsMatches(QStringList() << "foa"
                                                     << "foab"
                                                     << "foo");
    TestPressKey("a");
    verifyCommandBarCompletionsMatches(QStringList() << "foa"
                                                     << "foab");
    TestPressKey("\\ctrl-h");
    verifyCommandBarCompletionsMatches(QStringList() << "foa"
                                                     << "foab"
                                                     << "foo");
    TestPressKey("o");
    verifyCommandBarCompletionsMatches(QStringList() << "foo");
    TestPressKey("\\enter\\enter"); // Dismiss completion, then bar.
    FinishTest("foo fee foa foab");

    // Upon selecting a completion with an empty command bar, add the completed text to the command bar.
    BeginTest("foo fee fob foables");
    TestPressKey("/\\ctrl- foa\\ctrl-p");
    QCOMPARE(emulatedCommandBarTextEdit()->text(), QString("foables"));
    verifyCommandBarCompletionVisible();
    TestPressKey("\\enter\\enter"); // Dismiss completion, then bar.
    FinishTest("foo fee fob foables");

    // If bar is non-empty, replace the word under the cursor.
    BeginTest("foo fee foa foab");
    TestPressKey("/xyz|f$nose\\left\\left\\left\\left\\left\\ctrl- oa\\ctrl-p");
    QCOMPARE(emulatedCommandBarTextEdit()->text(), QString("xyz|foab$nose"));
    TestPressKey("\\enter\\enter"); // Dismiss completion, then bar.
    FinishTest("foo fee foa foab");

    // Place the cursor at the end of the completed text.
    BeginTest("foo fee foa foab");
    TestPressKey("/xyz|f$nose\\left\\left\\left\\left\\left\\ctrl- oa\\ctrl-p\\enterX");
    QCOMPARE(emulatedCommandBarTextEdit()->text(), QString("xyz|foabX$nose"));
    TestPressKey("\\ctrl-c"); // Dismiss completion, then bar.
    FinishTest("foo fee foa foab");

    // If we're completing from history, though, the entire text gets set, and the completion prefix
    // is the beginning of the entire text, not the current word before the cursor.
    clearSearchHistory();
    vi_global->searchHistory()->append("foo(bar");
    BeginTest("");
    TestPressKey("/foo(b\\ctrl-p");
    verifyCommandBarCompletionVisible();
    verifyCommandBarCompletionsMatches(QStringList() << "foo(bar");
    QCOMPARE(emulatedCommandBarTextEdit()->text(), QString("foo(bar"));
    TestPressKey("\\enter"); // Dismiss bar.
    FinishTest("");

    // If we're completing from history and we abort the completion via ctrl-c or ctrl-[, we revert the whole
    // text to the last manually typed text.
    clearSearchHistory();
    vi_global->searchHistory()->append("foo(b|ar");
    BeginTest("");
    TestPressKey("/foo(b\\ctrl-p");
    verifyCommandBarCompletionVisible();
    verifyCommandBarCompletionsMatches(QStringList() << "foo(b|ar");
    QCOMPARE(emulatedCommandBarTextEdit()->text(), QString("foo(b|ar"));
    TestPressKey("\\ctrl-c"); // Dismiss completion.
    QCOMPARE(emulatedCommandBarTextEdit()->text(), QString("foo(b"));
    TestPressKey("\\enter"); // Dismiss bar.
    FinishTest("");

    // Scroll completion list if necessary so that currently selected completion is visible.
    BeginTest("a b c d e f g h i j k l m n o p q r s t u v w x y z");
    TestPressKey("/\\ctrl- ");
    const int lastItemRow = 25;
    const QRect initialLastCompletionItemRect =
        emulatedCommandBarCompleter()->popup()->visualRect(emulatedCommandBarCompleter()->popup()->model()->index(lastItemRow, 0));
    QVERIFY(!emulatedCommandBarCompleter()->popup()->rect().contains(
        initialLastCompletionItemRect)); // If this fails, then we have an error in the test setup: initially, the last item in the list should be outside of
                                         // the bounds of the popup.
    TestPressKey("\\ctrl-n");
    QCOMPARE(emulatedCommandBarCompleter()->currentCompletion(), QString("z"));
    const QRect lastCompletionItemRect =
        emulatedCommandBarCompleter()->popup()->visualRect(emulatedCommandBarCompleter()->popup()->model()->index(lastItemRow, 0));
    QVERIFY(emulatedCommandBarCompleter()->popup()->rect().contains(lastCompletionItemRect));
    TestPressKey("\\enter\\enter"); // Dismiss completion, then bar.
    FinishTest("a b c d e f g h i j k l m n o p q r s t u v w x y z");

    // Ensure that the completion list changes size appropriately as the number of candidate completions changes.
    BeginTest("a ab abc");
    TestPressKey("/\\ctrl- ");
    const int initialPopupHeight = emulatedCommandBarCompleter()->popup()->height();
    TestPressKey("ab");
    const int popupHeightAfterEliminatingOne = emulatedCommandBarCompleter()->popup()->height();
    QVERIFY(popupHeightAfterEliminatingOne < initialPopupHeight);
    TestPressKey("\\enter\\enter"); // Dismiss completion, then bar.
    FinishTest("a ab abc");

    // Ensure that the completion list disappears when no candidate completions are found, but re-appears
    // when some are found.
    BeginTest("a ab abc");
    TestPressKey("/\\ctrl- ");
    TestPressKey("abd");
    QVERIFY(!emulatedCommandBarCompleter()->popup()->isVisible());
    TestPressKey("\\ctrl-h");
    verifyCommandBarCompletionVisible();
    TestPressKey("\\enter\\enter"); // Dismiss completion, then bar.
    FinishTest("a ab abc");

    // ctrl-c and ctrl-[ when the completion list is visible should dismiss the completion list, but *not*
    // the emulated command bar. TODO - same goes for ESC, but this is harder as KateViewInternal dismisses it
    // itself.
    BeginTest("a ab abc");
    TestPressKey("/\\ctrl- \\ctrl-cdiw");
    QVERIFY(!emulatedCommandBarCompleter()->popup()->isVisible());
    QVERIFY(emulatedCommandBar->isVisible());
    TestPressKey("\\enter"); // Dismiss bar.
    TestPressKey("/\\ctrl- \\ctrl-[diw");
    QVERIFY(!emulatedCommandBarCompleter()->popup()->isVisible());
    QVERIFY(emulatedCommandBar->isVisible());
    TestPressKey("\\enter"); // Dismiss bar.
    FinishTest("a ab abc");

    // If we implicitly choose an element from the summoned completion list (by highlighting it, then
    // continuing to edit the text), the completion box should not re-appear unless explicitly summoned
    // again, even if the current word has a valid completion.
    BeginTest("a ab abc");
    TestPressKey("/\\ctrl- \\ctrl-p");
    TestPressKey(".a");
    QVERIFY(!emulatedCommandBarCompleter()->popup()->isVisible());
    TestPressKey("\\enter"); // Dismiss bar.
    FinishTest("a ab abc");

    // If we dismiss the summoned completion list via ctrl-c or ctrl-[, it should not re-appear unless explicitly summoned
    // again, even if the current word has a valid completion.
    BeginTest("a ab abc");
    TestPressKey("/\\ctrl- \\ctrl-c");
    TestPressKey(".a");
    QVERIFY(!emulatedCommandBarCompleter()->popup()->isVisible());
    TestPressKey("\\enter");
    TestPressKey("/\\ctrl- \\ctrl-[");
    TestPressKey(".a");
    QVERIFY(!emulatedCommandBarCompleter()->popup()->isVisible());
    TestPressKey("\\enter"); // Dismiss bar.
    FinishTest("a ab abc");

    // If we select a completion from an empty bar, but then dismiss it via ctrl-c or ctrl-[, then we
    // should restore the empty text.
    BeginTest("foo");
    TestPressKey("/\\ctrl- \\ctrl-p");
    QCOMPARE(emulatedCommandBarTextEdit()->text(), QString("foo"));
    TestPressKey("\\ctrl-c");
    QVERIFY(!emulatedCommandBarCompleter()->popup()->isVisible());
    QVERIFY(emulatedCommandBar->isVisible());
    QCOMPARE(emulatedCommandBarTextEdit()->text(), QString(""));
    TestPressKey("\\enter"); // Dismiss bar.
    FinishTest("foo");
    BeginTest("foo");
    TestPressKey("/\\ctrl- \\ctrl-p");
    QCOMPARE(emulatedCommandBarTextEdit()->text(), QString("foo"));
    TestPressKey("\\ctrl-[");
    QVERIFY(!emulatedCommandBarCompleter()->popup()->isVisible());
    QVERIFY(emulatedCommandBar->isVisible());
    QCOMPARE(emulatedCommandBarTextEdit()->text(), QString(""));
    TestPressKey("\\enter"); // Dismiss bar.
    FinishTest("foo");

    // If we select a completion but then dismiss it via ctrl-c or ctrl-[, then we
    // should restore the last manually typed word.
    BeginTest("fooabc");
    TestPressKey("/f\\ctrl- o\\ctrl-p");
    QCOMPARE(emulatedCommandBarTextEdit()->text(), QString("fooabc"));
    TestPressKey("\\ctrl-c");
    QVERIFY(!emulatedCommandBarCompleter()->popup()->isVisible());
    QVERIFY(emulatedCommandBar->isVisible());
    QCOMPARE(emulatedCommandBarTextEdit()->text(), QString("fo"));
    TestPressKey("\\enter"); // Dismiss bar.
    FinishTest("fooabc");

    // If we select a completion but then dismiss it via ctrl-c or ctrl-[, then we
    // should restore the word currently being typed to the last manually typed word.
    BeginTest("fooabc");
    TestPressKey("/ab\\ctrl- |fo\\ctrl-p");
    TestPressKey("\\ctrl-c");
    QCOMPARE(emulatedCommandBarTextEdit()->text(), QString("ab|fo"));
    TestPressKey("\\enter"); // Dismiss bar.
    FinishTest("fooabc");

    // Set the completion prefix for the search history completion as soon as it is shown.
    clearSearchHistory();
    vi_global->searchHistory()->append("foo(bar");
    vi_global->searchHistory()->append("xyz");
    BeginTest("");
    TestPressKey("/f\\ctrl-p");
    verifyCommandBarCompletionVisible();
    verifyCommandBarCompletionsMatches(QStringList() << "foo(bar");
    TestPressKey("\\enter"); // Dismiss bar.
    FinishTest("");

    // Command Mode (:) tests.
    // ":" should summon the command bar, with ":" as the label.
    BeginTest("");
    TestPressKey(":");
    QVERIFY(emulatedCommandBar->isVisible());
    QCOMPARE(emulatedCommandTypeIndicator()->text(), QString(":"));
    QVERIFY(emulatedCommandTypeIndicator()->isVisible());
    QVERIFY(emulatedCommandBarTextEdit());
    QVERIFY(emulatedCommandBarTextEdit()->text().isEmpty());
    TestPressKey("\\esc");
    FinishTest("");

    // If we have a selection, it should be encoded as a range in the text edit.
    BeginTest("d\nb\na\nc");
    TestPressKey("Vjjj:");
    QCOMPARE(emulatedCommandBarTextEdit()->text(), QString("'<,'>"));
    TestPressKey("\\esc");
    FinishTest("d\nb\na\nc");

    // If we have a count, it should be encoded as a range in the text edit.
    BeginTest("d\nb\na\nc");
    TestPressKey("7:");
    QCOMPARE(emulatedCommandBarTextEdit()->text(), QString(".,.+6"));
    TestPressKey("\\esc");
    FinishTest("d\nb\na\nc");

    // Don't go doing an incremental search when we press keys!
    BeginTest("foo bar xyz");
    TestPressKey(":bar");
    QCOMPARE(rangesOnFirstLine().size(), rangesInitial.size());
    TestPressKey("\\esc");
    FinishTest("foo bar xyz");

    // Execute the command on Enter.
    DoTest("d\nb\na\nc", "Vjjj:sort\\enter", "a\nb\nc\nd");

    // Don't crash if we call a non-existent command with a range.
    DoTest("123", ":42nonexistentcommand\\enter", "123");

    // Bar background should always be normal for command bar.
    BeginTest("foo");
    TestPressKey("/foo\\enter:");
    verifyTextEditBackgroundColour(normalBackgroundColour);
    TestPressKey("\\ctrl-c/bar\\enter:");
    verifyTextEditBackgroundColour(normalBackgroundColour);
    TestPressKey("\\esc");
    FinishTest("foo");

    const int commandResponseMessageTimeOutMSOverride = QString::fromLatin1(qgetenv("KATE_VIMODE_TEST_COMMANDRESPONSEMESSAGETIMEOUTMS")).toInt();
    const long commandResponseMessageTimeOutMS = (commandResponseMessageTimeOutMSOverride > 0) ? commandResponseMessageTimeOutMSOverride : 2000;
    {
        // If there is any output from the command, show it in a label for a short amount of time
        // (make sure the bar type indicator is hidden, here, as it looks messy).
        emulatedCommandBar->setCommandResponseMessageTimeout(commandResponseMessageTimeOutMS);
        BeginTest("foo bar xyz");
        const QDateTime timeJustBeforeCommandExecuted = QDateTime::currentDateTime();
        TestPressKey(":commandthatdoesnotexist\\enter");
        QVERIFY(emulatedCommandBar->isVisible());
        QVERIFY(commandResponseMessageDisplay());
        QVERIFY(commandResponseMessageDisplay()->isVisible());
        QVERIFY(!emulatedCommandBarTextEdit()->isVisible());
        QVERIFY(!emulatedCommandTypeIndicator()->isVisible());
        // Be a bit vague about the exact message, due to i18n, etc.
        QVERIFY(commandResponseMessageDisplay()->text().contains("commandthatdoesnotexist"));
        waitForEmulatedCommandBarToHide(4 * commandResponseMessageTimeOutMS);
        QVERIFY(timeJustBeforeCommandExecuted.msecsTo(QDateTime::currentDateTime())
                >= commandResponseMessageTimeOutMS - 500); // "- 500" because coarse timers can fire up to 500ms *prematurely*.
        QVERIFY(!emulatedCommandBar->isVisible());
        // Piggy-back on this test, as the bug we're about to test for would actually make setting
        // up the conditions again in a separate test impossible ;)
        // When we next summon the bar, the response message should be invisible; the editor visible & editable;
        // and the bar type indicator visible again.
        TestPressKey("/");
        QVERIFY(!commandResponseMessageDisplay()->isVisible());
        QVERIFY(emulatedCommandBarTextEdit()->isVisible());
        QVERIFY(emulatedCommandBarTextEdit()->isEnabled());
        QVERIFY(emulatedCommandBar->isVisible());
        TestPressKey("\\esc"); // Dismiss the bar.
        FinishTest("foo bar xyz");
    }

    {
        // Show the same message twice in a row.
        BeginTest("foo bar xyz");
        TestPressKey(":othercommandthatdoesnotexist\\enter");
        QDateTime startWaitingForMessageToHide = QDateTime::currentDateTime();
        waitForEmulatedCommandBarToHide(4 * commandResponseMessageTimeOutMS);
        TestPressKey(":othercommandthatdoesnotexist\\enter");
        QVERIFY(commandResponseMessageDisplay()->isVisible());
        // Wait for it to disappear again, as a courtesy for the next test.
        waitForEmulatedCommandBarToHide(4 * commandResponseMessageTimeOutMS);
    }

    {
        // Emulated command bar should not steal keypresses when it is merely showing the results of an executed command.
        BeginTest("foo bar");
        TestPressKey(":commandthatdoesnotexist\\enterrX");
        Q_ASSERT_X(commandResponseMessageDisplay()->isVisible(), "running test", "Need to increase timeJustBeforeCommandExecuted!");
        FinishTest("Xoo bar");
    }

    {
        // Don't send the synthetic "enter" keypress (for making search-as-a-motion work) when we finally hide.
        BeginTest("foo bar\nbar");
        TestPressKey(":commandthatdoesnotexist\\enter");
        Q_ASSERT_X(commandResponseMessageDisplay()->isVisible(), "running test", "Need to increase timeJustBeforeCommandExecuted!");
        waitForEmulatedCommandBarToHide(commandResponseMessageTimeOutMS * 4);
        TestPressKey("rX");
        FinishTest("Xoo bar\nbar");
    }

    {
        // The timeout should be cancelled when we invoke the command bar again.
        BeginTest("");
        TestPressKey(":commandthatdoesnotexist\\enter");
        TestPressKey(":");
        // Wait ample time for the timeout to fire.  Do not use waitForEmulatedCommandBarToHide for this!
        QTRY_VERIFY_WITH_TIMEOUT(emulatedCommandBar->isVisible(), commandResponseMessageTimeOutMS * 2);
        TestPressKey("\\esc"); // Dismiss the bar.
        FinishTest("");
    }

    {
        // The timeout should not cause kate_view to regain focus if we have manually taken it away.
        qDebug() << " NOTE: this test is weirdly fragile, so if it starts failing, comment it out and e-mail me:  it may well be more trouble that it's worth.";
        BeginTest("");
        TestPressKey(":commandthatdoesnotexist\\enter");
        // Wait for any focus changes to take effect.
        QApplication::processEvents();
        QLineEdit *dummyToFocus = new QLineEdit(QString("Sausage"), mainWindow);
        // Take focus away from kate_view by giving it to dummyToFocus.
        QApplication::setActiveWindow(mainWindow);
        kate_view->setFocus();
        mainWindowLayout->addWidget(dummyToFocus);
        dummyToFocus->show();
        dummyToFocus->setEnabled(true);
        dummyToFocus->setFocus();
        // Allow dummyToFocus to receive focus.
        QTRY_VERIFY(dummyToFocus->hasFocus());
        // Wait ample time for the timeout to fire.  Do not use waitForEmulatedCommandBarToHide for this -
        // the bar never actually hides in this instance, and I think it would take some deep changes in
        // Kate to make it do so (the KateCommandLineBar as the same issue).
        QTest::qWait(commandResponseMessageTimeOutMS * 2);
        QVERIFY(dummyToFocus->hasFocus());
        QVERIFY(emulatedCommandBar->isVisible());
        mainWindowLayout->removeWidget(dummyToFocus);
        // Restore focus to the kate_view.
        kate_view->setFocus();
        QTRY_VERIFY(kate_view->hasFocus());
        // *Now* wait for the command bar to disappear - giving kate_view focus should trigger it.
        waitForEmulatedCommandBarToHide(commandResponseMessageTimeOutMS * 4);
        FinishTest("");
    }

    {
        // No completion should be shown when the bar is first shown: this gives us an opportunity
        // to invoke command history via ctrl-p and ctrl-n.
        BeginTest("");
        TestPressKey(":");
        QVERIFY(!emulatedCommandBarCompleter()->popup()->isVisible());
        TestPressKey("\\ctrl-c"); // Dismiss bar
        FinishTest("");
    }

    {
        // Should be able to switch to completion from document, even when we have a completion from commands.
        BeginTest("soggy1 soggy2");
        TestPressKey(":so");
        verifyCommandBarCompletionContains(QStringList() << "sort");
        TestPressKey("\\ctrl- ");
        verifyCommandBarCompletionsMatches(QStringList() << "soggy1"
                                                         << "soggy2");
        TestPressKey("\\ctrl-c"); // Dismiss completer
        TestPressKey("\\ctrl-c"); // Dismiss bar
        FinishTest("soggy1 soggy2");
    }

    {
        // If we dismiss the command completion then change the text, it should summon the completion
        // again.
        BeginTest("");
        TestPressKey(":so");
        TestPressKey("\\ctrl-c"); // Dismiss completer
        TestPressKey("r");
        verifyCommandBarCompletionVisible();
        verifyCommandBarCompletionContains(QStringList() << "sort");
        TestPressKey("\\ctrl-c"); // Dismiss completer
        TestPressKey("\\ctrl-c"); // Dismiss bar
        FinishTest("");
    }

    {
        // Completion should be dismissed when we are showing command response text.
        BeginTest("");
        TestPressKey(":set-au\\enter");
        QVERIFY(commandResponseMessageDisplay()->isVisible());
        QVERIFY(!emulatedCommandBarCompleter()->popup()->isVisible());
        waitForEmulatedCommandBarToHide(commandResponseMessageTimeOutMS * 4);
        FinishTest("");
    }

    // If we abort completion via ctrl-c or ctrl-[, we should revert the current word to the last
    // manually entered word.
    BeginTest("");
    TestPressKey(":se\\ctrl-p");
    verifyCommandBarCompletionVisible();
    QVERIFY(emulatedCommandBarTextEdit()->text() != "se");
    TestPressKey("\\ctrl-c"); // Dismiss completer
    QCOMPARE(emulatedCommandBarTextEdit()->text(), QString("se"));
    TestPressKey("\\ctrl-c"); // Dismiss bar
    FinishTest("");

    // In practice, it's annoying if, as we enter ":s/se", completions pop up after the "se":
    // for now, only summon completion if we are on the first word in the text.
    BeginTest("");
    TestPressKey(":s/se");
    QVERIFY(!emulatedCommandBarCompleter()->popup()->isVisible());
    TestPressKey("\\ctrl-c"); // Dismiss bar
    FinishTest("");
    BeginTest("");
    TestPressKey(":.,.+7s/se");
    QVERIFY(!emulatedCommandBarCompleter()->popup()->isVisible());
    TestPressKey("\\ctrl-c"); // Dismiss bar
    FinishTest("");

    // Don't blank the text if we activate command history completion with no command history.
    BeginTest("");
    clearCommandHistory();
    TestPressKey(":s/se\\ctrl-p");
    QVERIFY(!emulatedCommandBarCompleter()->popup()->isVisible());
    QCOMPARE(emulatedCommandBarTextEdit()->text(), QString("s/se"));
    TestPressKey("\\ctrl-c"); // Dismiss bar
    FinishTest("");

    // On completion, only update the command in front of the cursor.
    BeginTest("");
    clearCommandHistory();
    TestPressKey(":.,.+6s/se\\left\\left\\leftet-auto-in\\ctrl-p");
    QCOMPARE(emulatedCommandBarTextEdit()->text(), QString(".,.+6set-auto-indent/se"));
    TestPressKey("\\ctrl-c"); // Dismiss completer.
    TestPressKey("\\ctrl-c"); // Dismiss bar
    FinishTest("");

    // On completion, place the cursor after the new command.
    BeginTest("");
    clearCommandHistory();
    TestPressKey(":.,.+6s/fo\\left\\left\\leftet-auto-in\\ctrl-pX");
    QCOMPARE(emulatedCommandBarTextEdit()->text(), QString(".,.+6set-auto-indentX/fo"));
    TestPressKey("\\ctrl-c"); // Dismiss completer.
    TestPressKey("\\ctrl-c"); // Dismiss bar
    FinishTest("");

    // "The current word", for Commands, can contain "-".
    BeginTest("");
    TestPressKey(":set-\\ctrl-p");
    verifyCommandBarCompletionVisible();
    QVERIFY(emulatedCommandBarTextEdit()->text() != "set-");
    QVERIFY(emulatedCommandBarCompleter()->currentCompletion().startsWith(QLatin1String("set-")));
    QCOMPARE(emulatedCommandBarTextEdit()->text(), emulatedCommandBarCompleter()->currentCompletion());
    TestPressKey("\\ctrl-c"); // Dismiss completion.
    TestPressKey("\\ctrl-c"); // Dismiss bar.
    FinishTest("");

    {
        // Don't switch from word-from-document to command-completion just because we press a key, though!
        BeginTest("soggy1 soggy2");
        TestPressKey(":\\ctrl- s");
        TestPressKey("o");
        verifyCommandBarCompletionVisible();
        verifyCommandBarCompletionsMatches(QStringList() << "soggy1"
                                                         << "soggy2");
        TestPressKey("\\ctrl-c"); // Dismiss completer
        TestPressKey("\\ctrl-c"); // Dismiss bar
        FinishTest("soggy1 soggy2");
    }

    {
        // If we're in a place where there is no command completion allowed, don't go hiding the word
        // completion as we type.
        BeginTest("soggy1 soggy2");
        TestPressKey(":s/s\\ctrl- o");
        verifyCommandBarCompletionVisible();
        verifyCommandBarCompletionsMatches(QStringList() << "soggy1"
                                                         << "soggy2");
        TestPressKey("\\ctrl-c"); // Dismiss completer
        TestPressKey("\\ctrl-c"); // Dismiss bar
        FinishTest("soggy1 soggy2");
    }

    {
        // Don't show command completion before we start typing a command: we want ctrl-p/n
        // to go through command history instead (we'll test for that second part later).
        BeginTest("soggy1 soggy2");
        TestPressKey(":");
        QVERIFY(!emulatedCommandBarCompleter()->popup()->isVisible());
        TestPressKey("\\ctrl-cvl:");
        QVERIFY(!emulatedCommandBarCompleter()->popup()->isVisible());
        TestPressKey("\\ctrl-c"); // Dismiss bar
        FinishTest("soggy1 soggy2");
    }

    {
        // Aborting ":" should leave us in normal mode with no selection.
        BeginTest("foo bar");
        TestPressKey("vw:\\ctrl-[");
        QVERIFY(kate_view->selectionText().isEmpty());
        TestPressKey("wdiw");
        BeginTest("foo ");
    }

    // Command history tests.
    clearCommandHistory();
    QVERIFY(commandHistory().isEmpty());
    vi_global->commandHistory()->append("foo");
    vi_global->commandHistory()->append("bar");
    QCOMPARE(commandHistory(),
             QStringList() << "foo"
                           << "bar");
    clearCommandHistory();
    QVERIFY(commandHistory().isEmpty());

    // If we add something to the history, remove any earliest occurrences (this is what Vim appears to do)
    // and append to the end.
    clearCommandHistory();
    vi_global->commandHistory()->append("bar");
    vi_global->commandHistory()->append("xyz");
    vi_global->commandHistory()->append("foo");
    vi_global->commandHistory()->append("xyz");
    QCOMPARE(commandHistory(),
             QStringList() << "bar"
                           << "foo"
                           << "xyz");

    // Push out older entries if we have too many command items in the history.
    clearCommandHistory();
    for (int i = 1; i <= HISTORY_SIZE_LIMIT; i++) {
        vi_global->commandHistory()->append(QString("commandhistoryitem %1").arg(i));
    }
    QCOMPARE(commandHistory().size(), HISTORY_SIZE_LIMIT);
    QCOMPARE(commandHistory().first(), QString("commandhistoryitem 1"));
    QCOMPARE(commandHistory().last(), QString("commandhistoryitem 100"));
    vi_global->commandHistory()->append(QString("commandhistoryitem %1").arg(HISTORY_SIZE_LIMIT + 1));
    QCOMPARE(commandHistory().size(), HISTORY_SIZE_LIMIT);
    QCOMPARE(commandHistory().first(), QString("commandhistoryitem 2"));
    QCOMPARE(commandHistory().last(), QString("commandhistoryitem %1").arg(HISTORY_SIZE_LIMIT + 1));

    // Don't add empty commands to the history.
    clearCommandHistory();
    DoTest("foo bar", ":\\enter", "foo bar");
    QVERIFY(commandHistory().isEmpty());

    clearCommandHistory();
    BeginTest("");
    TestPressKey(":sort\\enter");
    QCOMPARE(commandHistory(), QStringList() << "sort");
    TestPressKey(":yank\\enter");
    QCOMPARE(commandHistory(),
             QStringList() << "sort"
                           << "yank");
    // Add to history immediately: don't wait for the command response display to timeout.
    TestPressKey(":commandthatdoesnotexist\\enter");
    QCOMPARE(commandHistory(),
             QStringList() << "sort"
                           << "yank"
                           << "commandthatdoesnotexist");
    // Vim adds aborted commands to the history too, oddly.
    TestPressKey(":abortedcommand\\ctrl-c");
    QCOMPARE(commandHistory(),
             QStringList() << "sort"
                           << "yank"
                           << "commandthatdoesnotexist"
                           << "abortedcommand");
    // Only add for commands, not searches!
    TestPressKey("/donotaddme\\enter?donotaddmeeither\\enter/donotaddme\\ctrl-c?donotaddmeeither\\ctrl-c");
    QCOMPARE(commandHistory(),
             QStringList() << "sort"
                           << "yank"
                           << "commandthatdoesnotexist"
                           << "abortedcommand");
    FinishTest("");

    // Commands should not be added to the search history!
    clearCommandHistory();
    clearSearchHistory();
    BeginTest("");
    TestPressKey(":sort\\enter");
    QVERIFY(searchHistory().isEmpty());
    FinishTest("");

    // With an empty command bar, ctrl-p / ctrl-n should go through history.
    clearCommandHistory();
    vi_global->commandHistory()->append("command1");
    vi_global->commandHistory()->append("command2");
    BeginTest("");
    TestPressKey(":\\ctrl-p");
    verifyCommandBarCompletionVisible();
    QCOMPARE(emulatedCommandBarCompleter()->currentCompletion(), QString("command2"));
    QCOMPARE(emulatedCommandBarTextEdit()->text(), emulatedCommandBarCompleter()->currentCompletion());
    TestPressKey("\\ctrl-c"); // Dismiss completer
    TestPressKey("\\ctrl-c"); // Dismiss bar
    FinishTest("");
    clearCommandHistory();
    vi_global->commandHistory()->append("command1");
    vi_global->commandHistory()->append("command2");
    BeginTest("");
    TestPressKey(":\\ctrl-n");
    verifyCommandBarCompletionVisible();
    QCOMPARE(emulatedCommandBarCompleter()->currentCompletion(), QString("command1"));
    QCOMPARE(emulatedCommandBarTextEdit()->text(), emulatedCommandBarCompleter()->currentCompletion());
    TestPressKey("\\ctrl-c"); // Dismiss completer
    TestPressKey("\\ctrl-c"); // Dismiss bar
    FinishTest("");

    // If we're at a place where command completions are not allowed, ctrl-p/n should go through history.
    clearCommandHistory();
    vi_global->commandHistory()->append("s/command1");
    vi_global->commandHistory()->append("s/command2");
    BeginTest("");
    TestPressKey(":s/\\ctrl-p");
    verifyCommandBarCompletionVisible();
    QCOMPARE(emulatedCommandBarCompleter()->currentCompletion(), QString("s/command2"));
    QCOMPARE(emulatedCommandBarTextEdit()->text(), emulatedCommandBarCompleter()->currentCompletion());
    TestPressKey("\\ctrl-c"); // Dismiss completer
    TestPressKey("\\ctrl-c"); // Dismiss bar
    FinishTest("");
    clearCommandHistory();
    vi_global->commandHistory()->append("s/command1");
    vi_global->commandHistory()->append("s/command2");
    BeginTest("");
    TestPressKey(":s/\\ctrl-n");
    verifyCommandBarCompletionVisible();
    QCOMPARE(emulatedCommandBarCompleter()->currentCompletion(), QString("s/command1"));
    QCOMPARE(emulatedCommandBarTextEdit()->text(), emulatedCommandBarCompleter()->currentCompletion());
    TestPressKey("\\ctrl-c"); // Dismiss completer
    TestPressKey("\\ctrl-c"); // Dismiss bar
    FinishTest("");

    // Cancelling word-from-document completion should revert the whole text to what it was before.
    BeginTest("sausage bacon");
    TestPressKey(":s/b\\ctrl- \\ctrl-p");
    QCOMPARE(emulatedCommandBarTextEdit()->text(), QString("s/bacon"));
    verifyCommandBarCompletionVisible();
    TestPressKey("\\ctrl-c"); // Dismiss completer
    QCOMPARE(emulatedCommandBarTextEdit()->text(), QString("s/b"));
    TestPressKey("\\ctrl-c"); // Dismiss bar
    FinishTest("sausage bacon");

    // "Replace" history tests.
    clearReplaceHistory();
    QVERIFY(replaceHistory().isEmpty());
    vi_global->replaceHistory()->append("foo");
    vi_global->replaceHistory()->append("bar");
    QCOMPARE(replaceHistory(),
             QStringList() << "foo"
                           << "bar");
    clearReplaceHistory();
    QVERIFY(replaceHistory().isEmpty());

    // If we add something to the history, remove any earliest occurrences (this is what Vim appears to do)
    // and append to the end.
    clearReplaceHistory();
    vi_global->replaceHistory()->append("bar");
    vi_global->replaceHistory()->append("xyz");
    vi_global->replaceHistory()->append("foo");
    vi_global->replaceHistory()->append("xyz");
    QCOMPARE(replaceHistory(),
             QStringList() << "bar"
                           << "foo"
                           << "xyz");

    // Push out older entries if we have too many replace items in the history.
    clearReplaceHistory();
    for (int i = 1; i <= HISTORY_SIZE_LIMIT; i++) {
        vi_global->replaceHistory()->append(QString("replacehistoryitem %1").arg(i));
    }
    QCOMPARE(replaceHistory().size(), HISTORY_SIZE_LIMIT);
    QCOMPARE(replaceHistory().first(), QString("replacehistoryitem 1"));
    QCOMPARE(replaceHistory().last(), QString("replacehistoryitem 100"));
    vi_global->replaceHistory()->append(QString("replacehistoryitem %1").arg(HISTORY_SIZE_LIMIT + 1));
    QCOMPARE(replaceHistory().size(), HISTORY_SIZE_LIMIT);
    QCOMPARE(replaceHistory().first(), QString("replacehistoryitem 2"));
    QCOMPARE(replaceHistory().last(), QString("replacehistoryitem %1").arg(HISTORY_SIZE_LIMIT + 1));

    // Don't add empty replaces to the history.
    clearReplaceHistory();
    vi_global->replaceHistory()->append("");
    QVERIFY(replaceHistory().isEmpty());

    // Some misc SedReplace tests.
    DoTest("x\\/y", ":s/\\\\//replace/g\\enter", "x\\replacey");
    DoTest("x\\/y", ":s/\\\\\\\\\\\\//replace/g\\enter", "xreplacey");
    DoTest("x\\/y", ":s:/:replace:g\\enter", "x\\replacey");
    DoTest("foo\nbar\nxyz", ":%delete\\enter", "");
    DoTest("foo\nbar\nxyz\nbaz", "jVj:delete\\enter", "foo\nbaz");
    DoTest("foo\nbar\nxyz\nbaz", "j2:delete\\enter", "foo\nbaz");

    // Test that 0 is accepted as a line index (and treated as 1) in a range specifier
    DoTest("bar\nbar\nbar", ":0,$s/bar/foo/g\\enter", "foo\nfoo\nfoo");
    DoTest("bar\nbar\nbar", ":1,$s/bar/foo/g\\enter", "foo\nfoo\nfoo");
    DoTest("bar\nbar\nbar", ":0,2s/bar/foo/g\\enter", "foo\nfoo\nbar");

    // On ctrl-d, delete the "search" term in a s/search/replace/xx
    BeginTest("foo bar");
    TestPressKey(":s/x\\\\\\\\\\\\/yz/rep\\\\\\\\\\\\/lace/g\\ctrl-d");
    QCOMPARE(emulatedCommandBarTextEdit()->text(), QString("s//rep\\\\\\/lace/g"));
    TestPressKey("\\ctrl-c"); // Dismiss bar.
    FinishTest("foo bar");
    // Move cursor to position of deleted search term.
    BeginTest("foo bar");
    TestPressKey(":s/x\\\\\\\\\\\\/yz/rep\\\\\\\\\\\\/lace/g\\ctrl-dX");
    QCOMPARE(emulatedCommandBarTextEdit()->text(), QString("s/X/rep\\\\\\/lace/g"));
    TestPressKey("\\ctrl-c"); // Dismiss bar.
    FinishTest("foo bar");
    // Do nothing on ctrl-d in search mode.
    BeginTest("foo bar");
    TestPressKey("/s/search/replace/g\\ctrl-d");
    QCOMPARE(emulatedCommandBarTextEdit()->text(), QString("s/search/replace/g"));
    TestPressKey("\\ctrl-c?s/searchbackwards/replace/g\\ctrl-d");
    QCOMPARE(emulatedCommandBarTextEdit()->text(), QString("s/searchbackwards/replace/g"));
    TestPressKey("\\ctrl-c"); // Dismiss bar.
    FinishTest("foo bar");
    // On ctrl-f, delete "replace" term in a s/search/replace/xx
    BeginTest("foo bar");
    TestPressKey(":s/a\\\\\\\\\\\\/bc/rep\\\\\\\\\\\\/lace/g\\ctrl-f");
    QCOMPARE(emulatedCommandBarTextEdit()->text(), QString("s/a\\\\\\/bc//g"));
    TestPressKey("\\ctrl-c"); // Dismiss bar.
    FinishTest("foo bar");
    // Move cursor to position of deleted replace term.
    BeginTest("foo bar");
    TestPressKey(":s:a/bc:replace:g\\ctrl-fX");
    QCOMPARE(emulatedCommandBarTextEdit()->text(), QString("s:a/bc:X:g"));
    TestPressKey("\\ctrl-c"); // Dismiss bar.
    FinishTest("foo bar");
    // Do nothing on ctrl-d in search mode.
    BeginTest("foo bar");
    TestPressKey("/s/search/replace/g\\ctrl-f");
    QCOMPARE(emulatedCommandBarTextEdit()->text(), QString("s/search/replace/g"));
    TestPressKey("\\ctrl-c?s/searchbackwards/replace/g\\ctrl-f");
    QCOMPARE(emulatedCommandBarTextEdit()->text(), QString("s/searchbackwards/replace/g"));
    TestPressKey("\\ctrl-c"); // Dismiss bar.
    FinishTest("foo bar");
    // Do nothing on ctrl-d / ctrl-f if the current expression is not a sed expression.
    BeginTest("foo bar");
    TestPressKey(":s/notasedreplaceexpression::gi\\ctrl-f\\ctrl-dX");
    QCOMPARE(emulatedCommandBarTextEdit()->text(), QString("s/notasedreplaceexpression::giX"));
    TestPressKey("\\ctrl-c"); // Dismiss bar.
    FinishTest("foo bar");
    // Need to convert Vim-style regex's to Qt one's in Sed Replace.
    DoTest("foo xbacba(boo)|[y", ":s/x[abc]\\\\+(boo)|[y/boo/g\\enter", "foo boo");
    DoTest("foo xbacba(boo)|[y\nfoo xbacba(boo)|[y", "Vj:s/x[abc]\\\\+(boo)|[y/boo/g\\enter", "foo boo\nfoo boo");
    // Just convert the search term, please :)
    DoTest("foo xbacba(boo)|[y", ":s/x[abc]\\\\+(boo)|[y/boo()/g\\enter", "foo boo()");
    // With an empty search expression, ctrl-d should still position the cursor correctly.
    BeginTest("foo bar");
    TestPressKey(":s//replace/g\\ctrl-dX");
    QCOMPARE(emulatedCommandBarTextEdit()->text(), QString("s/X/replace/g"));
    TestPressKey("\\ctrl-c"); // Dismiss bar.
    TestPressKey(":s::replace:g\\ctrl-dX");
    QCOMPARE(emulatedCommandBarTextEdit()->text(), QString("s:X:replace:g"));
    TestPressKey("\\ctrl-c"); // Dismiss bar.
    FinishTest("foo bar");
    // With an empty replace expression, ctrl-f should still position the cursor correctly.
    BeginTest("foo bar");
    TestPressKey(":s/sear\\\\/ch//g\\ctrl-fX");
    QCOMPARE(emulatedCommandBarTextEdit()->text(), QString("s/sear\\/ch/X/g"));
    TestPressKey("\\ctrl-c"); // Dismiss bar.
    TestPressKey(":s:sear\\\\:ch::g\\ctrl-fX");
    QCOMPARE(emulatedCommandBarTextEdit()->text(), QString("s:sear\\:ch:X:g"));
    TestPressKey("\\ctrl-c"); // Dismiss bar.
    FinishTest("foo bar");
    // With both empty search *and* replace expressions, ctrl-f should still position the cursor correctly.
    BeginTest("foo bar");
    TestPressKey(":s///g\\ctrl-fX");
    QCOMPARE(emulatedCommandBarTextEdit()->text(), QString("s//X/g"));
    TestPressKey("\\ctrl-c"); // Dismiss bar.
    TestPressKey(":s:::g\\ctrl-fX");
    QCOMPARE(emulatedCommandBarTextEdit()->text(), QString("s::X:g"));
    TestPressKey("\\ctrl-c"); // Dismiss bar.
    FinishTest("foo bar");
    // Should be able to undo ctrl-f or ctrl-d.
    BeginTest("foo bar");
    TestPressKey(":s/find/replace/g\\ctrl-d");
    emulatedCommandBarTextEdit()->undo();
    QCOMPARE(emulatedCommandBarTextEdit()->text(), QString("s/find/replace/g"));
    TestPressKey("\\ctrl-f");
    emulatedCommandBarTextEdit()->undo();
    QCOMPARE(emulatedCommandBarTextEdit()->text(), QString("s/find/replace/g"));
    TestPressKey("\\ctrl-c"); // Dismiss bar.
    FinishTest("foo bar");
    // ctrl-f / ctrl-d should cleanly finish sed find/ replace history completion.
    clearReplaceHistory();
    clearSearchHistory();
    vi_global->searchHistory()->append("searchxyz");
    vi_global->replaceHistory()->append("replacexyz");
    TestPressKey(":s///g\\ctrl-d\\ctrl-p");
    QVERIFY(emulatedCommandBarCompleter()->popup()->isVisible());
    TestPressKey("\\ctrl-f");
    QCOMPARE(emulatedCommandBarTextEdit()->text(), QString("s/searchxyz//g"));
    QVERIFY(!emulatedCommandBarCompleter()->popup()->isVisible());
    TestPressKey("\\ctrl-p");
    QVERIFY(emulatedCommandBarCompleter()->popup()->isVisible());
    TestPressKey("\\ctrl-d");
    QCOMPARE(emulatedCommandBarTextEdit()->text(), QString("s//replacexyz/g"));
    QVERIFY(!emulatedCommandBarCompleter()->popup()->isVisible());
    TestPressKey("\\ctrl-c"); // Dismiss bar.
    FinishTest("foo bar");
    // Don't hang if we execute a sed replace with empty search term.
    DoTest("foo bar", ":s//replace/g\\enter", "foo bar");

    // ctrl-f & ctrl-d should work even when there is a range expression at the beginning of the sed replace.
    BeginTest("foo bar");
    TestPressKey(":'<,'>s/search/replace/g\\ctrl-d");
    QCOMPARE(emulatedCommandBarTextEdit()->text(), QString("'<,'>s//replace/g"));
    TestPressKey("\\ctrl-c:.,.+6s/search/replace/g\\ctrl-f");
    QCOMPARE(emulatedCommandBarTextEdit()->text(), QString(".,.+6s/search//g"));
    TestPressKey("\\ctrl-c:%s/search/replace/g\\ctrl-f");
    QCOMPARE(emulatedCommandBarTextEdit()->text(), QString("%s/search//g"));
    // Place the cursor in the right place even when there is a range expression.
    TestPressKey("\\ctrl-c:.,.+6s/search/replace/g\\ctrl-fX");
    QCOMPARE(emulatedCommandBarTextEdit()->text(), QString(".,.+6s/search/X/g"));
    TestPressKey("\\ctrl-c:%s/search/replace/g\\ctrl-fX");
    QCOMPARE(emulatedCommandBarTextEdit()->text(), QString("%s/search/X/g"));
    TestPressKey("\\ctrl-c"); // Dismiss bar.
    FinishTest("foo bar");
    // Don't crash on ctrl-f/d if we have an empty command.
    DoTest("", ":\\ctrl-f\\ctrl-d\\ctrl-c", "");
    // Parser regression test: Don't crash on ctrl-f/d with ".,.+".
    DoTest("", ":.,.+\\ctrl-f\\ctrl-d\\ctrl-c", "");

    // Command-completion should be invoked on the command being typed even when preceded by a range expression.
    BeginTest("");
    TestPressKey(":0,'>so");
    verifyCommandBarCompletionVisible();
    TestPressKey("\\ctrl-c"); // Dismiss completer
    TestPressKey("\\ctrl-c"); // Dismiss bar.
    FinishTest("");

    // Command-completion should ignore the range expression.
    BeginTest("");
    TestPressKey(":.,.+6so");
    verifyCommandBarCompletionVisible();
    TestPressKey("\\ctrl-c"); // Dismiss completer
    TestPressKey("\\ctrl-c"); // Dismiss bar.
    FinishTest("");

    // A sed-replace should immediately add the search term to the search history.
    clearSearchHistory();
    BeginTest("");
    TestPressKey(":s/search/replace/g\\enter");
    QCOMPARE(searchHistory(), QStringList() << "search");
    FinishTest("");

    // An aborted sed-replace should not add the search term to the search history.
    clearSearchHistory();
    BeginTest("");
    TestPressKey(":s/search/replace/g\\ctrl-c");
    QCOMPARE(searchHistory(), QStringList());
    FinishTest("");

    // A non-sed-replace should leave the search history unchanged.
    clearSearchHistory();
    BeginTest("");
    TestPressKey(":s,search/replace/g\\enter");
    QCOMPARE(searchHistory(), QStringList());
    FinishTest("");

    // A sed-replace should immediately add the replace term to the replace history.
    clearReplaceHistory();
    BeginTest("");
    TestPressKey(":s/search/replace/g\\enter");
    QCOMPARE(replaceHistory(), QStringList() << "replace");
    clearReplaceHistory();
    TestPressKey(":'<,'>s/search/replace1/g\\enter");
    QCOMPARE(replaceHistory(), QStringList() << "replace1");
    FinishTest("");

    // An aborted sed-replace should not add the replace term to the replace history.
    clearReplaceHistory();
    BeginTest("");
    TestPressKey(":s/search/replace/g\\ctrl-c");
    QCOMPARE(replaceHistory(), QStringList());
    FinishTest("");

    // A non-sed-replace should leave the replace history unchanged.
    clearReplaceHistory();
    BeginTest("");
    TestPressKey(":s,search/replace/g\\enter");
    QCOMPARE(replaceHistory(), QStringList());
    FinishTest("");

    // Misc tests for sed replace.  These are for the *generic* Kate sed replace; they should all
    // use EmulatedCommandBarTests' built-in command execution stuff (\\:<commandtoexecute>\\\) rather than
    // invoking a EmulatedCommandBar and potentially doing some Vim-specific transforms to
    // the command.
    DoTest("foo foo foo", "\\:s/foo/bar/\\", "bar foo foo");
    DoTest("foo foo xyz foo", "\\:s/foo/bar/g\\", "bar bar xyz bar");
    DoTest("foofooxyzfoo", "\\:s/foo/bar/g\\", "barbarxyzbar");
    DoTest("foofooxyzfoo", "\\:s/foo/b/g\\", "bbxyzb");
    DoTest("ffxyzf", "\\:s/f/b/g\\", "bbxyzb");
    DoTest("ffxyzf", "\\:s/f/bar/g\\", "barbarxyzbar");
    DoTest("foo Foo fOO FOO foo", "\\:s/foo/bar/\\", "bar Foo fOO FOO foo");
    DoTest("Foo foo fOO FOO foo", "\\:s/foo/bar/\\", "Foo bar fOO FOO foo");
    DoTest("Foo foo fOO FOO foo", "\\:s/foo/bar/g\\", "Foo bar fOO FOO bar");
    DoTest("foo Foo fOO FOO foo", "\\:s/foo/bar/i\\", "bar Foo fOO FOO foo");
    DoTest("Foo foo fOO FOO foo", "\\:s/foo/bar/i\\", "bar foo fOO FOO foo");
    DoTest("Foo foo fOO FOO foo", "\\:s/foo/bar/gi\\", "bar bar bar bar bar");
    DoTest("Foo foo fOO FOO foo", "\\:s/foo/bar/ig\\", "bar bar bar bar bar");
    // There are some oddities to do with how EmulatedCommandBarTest's "execute command directly" stuff works with selected ranges:
    // basically, we need to do our selection in Visual mode, then exit back to Normal mode before running the
    // command.
    DoTest("foo foo\nbar foo foo\nxyz foo foo\nfoo bar foo", "jVj\\esc\\:'<,'>s/foo/bar/\\", "foo foo\nbar bar foo\nxyz bar foo\nfoo bar foo");
    DoTest("foo foo\nbar foo foo\nxyz foo foo\nfoo bar foo", "jVj\\esc\\:'<,'>s/foo/bar/g\\", "foo foo\nbar bar bar\nxyz bar bar\nfoo bar foo");
    DoTest("Foo foo fOO FOO foo", "\\:s/foo/barfoo/g\\", "Foo barfoo fOO FOO barfoo");
    DoTest("Foo foo fOO FOO foo", "\\:s/foo/foobar/g\\", "Foo foobar fOO FOO foobar");
    DoTest("axyzb", "\\:s/a(.*)b/d\\\\1f/\\", "dxyzf");
    DoTest("ayxzzyxzfddeefdb", "\\:s/a([xyz]+)([def]+)b/<\\\\1|\\\\2>/\\", "<yxzzyxz|fddeefd>");
    DoTest("foo", "\\:s/.*//g\\", "");
    DoTest("foo", "\\:s/.*/f/g\\", "f");
    DoTest("foo/bar", "\\:s/foo\\\\/bar/123\\\\/xyz/g\\", "123/xyz");
    DoTest("foo:bar", "\\:s:foo\\\\:bar:123\\\\:xyz:g\\", "123:xyz");
    const bool oldReplaceTabsDyn = kate_document->config()->replaceTabsDyn();
    kate_document->config()->setReplaceTabsDyn(false);
    DoTest("foo\tbar", "\\:s/foo\\\\tbar/replace/g\\", "replace");
    DoTest("foo\tbar", "\\:s/foo\\\\tbar/rep\\\\tlace/g\\", "rep\tlace");
    kate_document->config()->setReplaceTabsDyn(oldReplaceTabsDyn);
    DoTest("foo", "\\:s/foo/replaceline1\\\\nreplaceline2/g\\", "replaceline1\nreplaceline2");
    DoTest("foofoo", "\\:s/foo/replaceline1\\\\nreplaceline2/g\\", "replaceline1\nreplaceline2replaceline1\nreplaceline2");
    DoTest("foofoo\nfoo", "\\:s/foo/replaceline1\\\\nreplaceline2/g\\", "replaceline1\nreplaceline2replaceline1\nreplaceline2\nfoo");
    DoTest("fooafoob\nfooc\nfood",
           "Vj\\esc\\:'<,'>s/foo/replaceline1\\\\nreplaceline2/g\\",
           "replaceline1\nreplaceline2areplaceline1\nreplaceline2b\nreplaceline1\nreplaceline2c\nfood");
    DoTest("fooafoob\nfooc\nfood",
           "Vj\\esc\\:'<,'>s/foo/replaceline1\\\\nreplaceline2/\\",
           "replaceline1\nreplaceline2afoob\nreplaceline1\nreplaceline2c\nfood");
    DoTest("fooafoob\nfooc\nfood",
           "Vj\\esc\\:'<,'>s/foo/replaceline1\\\\nreplaceline2\\\\nreplaceline3/g\\",
           "replaceline1\nreplaceline2\nreplaceline3areplaceline1\nreplaceline2\nreplaceline3b\nreplaceline1\nreplaceline2\nreplaceline3c\nfood");
    DoTest("foofoo", "\\:s/foo/replace\\\\nfoo/g\\", "replace\nfooreplace\nfoo");
    DoTest("foofoo", "\\:s/foo/replacefoo\\\\nfoo/g\\", "replacefoo\nfooreplacefoo\nfoo");
    DoTest("foofoo", "\\:s/foo/replacefoo\\\\n/g\\", "replacefoo\nreplacefoo\n");
    DoTest("ff", "\\:s/f/f\\\\nf/g\\", "f\nff\nf");
    DoTest("ff", "\\:s/f/f\\\\n/g\\", "f\nf\n");
    DoTest("foo\nbar", "\\:s/foo\\\\n//g\\", "bar");
    DoTest("foo\n\n\nbar", "\\:s/foo(\\\\n)*bar//g\\", "");
    DoTest("foo\n\n\nbar", "\\:s/foo(\\\\n*)bar/123\\\\1456/g\\", "123\n\n\n456");
    DoTest("xAbCy", "\\:s/x(.)(.)(.)y/\\\\L\\\\1\\\\U\\\\2\\\\3/g\\", "aBC");
    DoTest("foo", "\\:s/foo/\\\\a/g\\", "\x07");
    // End "generic" (i.e. not involving any Vi mode tricks/ transformations) sed replace tests: the remaining
    // ones should go via the EmulatedCommandBar.
    BeginTest("foo foo\nxyz\nfoo");
    TestPressKey(":%s/foo/bar/g\\enter");
    verifyShowsNumberOfReplacementsAcrossNumberOfLines(3, 2);
    FinishTest("bar bar\nxyz\nbar");

    // ctrl-p on the first character of the search term in a sed-replace should
    // invoke search history completion.
    clearSearchHistory();
    vi_global->searchHistory()->append("search");
    BeginTest("");
    TestPressKey(":s/search/replace/g\\ctrl-b\\right\\right\\ctrl-p");
    verifyCommandBarCompletionVisible();
    TestPressKey("\\ctrl-c"); // Dismiss completer
    TestPressKey("\\ctrl-c"); // Dismiss bar.
    TestPressKey(":'<,'>s/search/replace/g\\ctrl-b\\right\\right\\right\\right\\right\\right\\right\\ctrl-p");
    verifyCommandBarCompletionVisible();
    TestPressKey("\\ctrl-c"); // Dismiss completer
    TestPressKey("\\ctrl-c"); // Dismiss bar.
    FinishTest("");

    // ctrl-p on the last character of the search term in a sed-replace should
    // invoke search history completion.
    clearSearchHistory();
    vi_global->searchHistory()->append("xyz");
    BeginTest("");
    TestPressKey(":s/xyz/replace/g\\ctrl-b\\right\\right\\right\\right\\ctrl-p");
    verifyCommandBarCompletionVisible();
    TestPressKey("\\ctrl-c"); // Dismiss completer
    TestPressKey("\\ctrl-c"); // Dismiss bar.
    QVERIFY(!emulatedCommandBar->isVisible());
    TestPressKey(":'<,'>s/xyz/replace/g\\ctrl-b\\right\\right\\right\\right\\right\\right\\right\\right\\right\\ctrl-p");
    verifyCommandBarCompletionVisible();
    TestPressKey("\\ctrl-c"); // Dismiss completer
    TestPressKey("\\ctrl-c"); // Dismiss bar.
    FinishTest("");

    // ctrl-p on some arbitrary character of the search term in a sed-replace should
    // invoke search history completion.
    clearSearchHistory();
    vi_global->searchHistory()->append("xyzaaaaaa");
    BeginTest("");
    TestPressKey(":s/xyzaaaaaa/replace/g\\ctrl-b\\right\\right\\right\\right\\right\\right\\right\\ctrl-p");
    verifyCommandBarCompletionVisible();
    TestPressKey("\\ctrl-c"); // Dismiss completer
    TestPressKey("\\ctrl-c"); // Dismiss bar.
    TestPressKey(":'<,'>s/xyzaaaaaa/replace/g\\ctrl-b\\right\\right\\right\\right\\right\\right\\right\\right\\right\\right\\right\\right\\ctrl-p");
    verifyCommandBarCompletionVisible();
    TestPressKey("\\ctrl-c"); // Dismiss completer
    TestPressKey("\\ctrl-c"); // Dismiss bar.
    FinishTest("");

    // ctrl-p on some character *after" the search term should
    // *not* invoke search history completion.
    // Note: in s/xyz/replace/g, the "/" after the "z" is counted as part of the find term;
    // this allows us to do xyz<ctrl-p> and get completions.
    clearSearchHistory();
    clearCommandHistory();
    clearReplaceHistory();
    vi_global->searchHistory()->append("xyz");
    BeginTest("");
    TestPressKey(":s/xyz/replace/g\\ctrl-b\\right\\right\\right\\right\\right\\right\\ctrl-p");
    QVERIFY(!emulatedCommandBarCompleter()->popup()->isVisible());
    TestPressKey("\\ctrl-c"); // Dismiss completer
    TestPressKey("\\ctrl-c"); // Dismiss bar.
    clearSearchHistory();
    clearCommandHistory();
    TestPressKey(":'<,'>s/xyz/replace/g\\ctrl-b\\right\\right\\right\\right\\right\\right\\right\\right\\right\\right\\right\\right\\ctrl-p");
    QVERIFY(!emulatedCommandBarCompleter()->popup()->isVisible());
    TestPressKey("\\ctrl-c"); // Dismiss completer
    TestPressKey("\\ctrl-c"); // Dismiss bar.

    // Make sure it's the search history we're invoking.
    clearSearchHistory();
    vi_global->searchHistory()->append("xyzaaaaaa");
    BeginTest("");
    TestPressKey(":s//replace/g\\ctrl-b\\right\\right\\ctrl-p");
    verifyCommandBarCompletionVisible();
    verifyCommandBarCompletionsMatches(QStringList() << "xyzaaaaaa");
    TestPressKey("\\ctrl-c"); // Dismiss completer
    TestPressKey("\\ctrl-c"); // Dismiss bar.
    TestPressKey(":.,.+6s//replace/g\\ctrl-b\\right\\right\\right\\right\\right\\right\\right\\ctrl-p");
    verifyCommandBarCompletionsMatches(QStringList() << "xyzaaaaaa");
    TestPressKey("\\ctrl-c"); // Dismiss completer
    TestPressKey("\\ctrl-c"); // Dismiss bar.
    FinishTest("");

    // (Search history should be reversed).
    clearSearchHistory();
    vi_global->searchHistory()->append("xyzaaaaaa");
    vi_global->searchHistory()->append("abc");
    vi_global->searchHistory()->append("def");
    BeginTest("");
    TestPressKey(":s//replace/g\\ctrl-b\\right\\right\\ctrl-p");
    verifyCommandBarCompletionVisible();
    verifyCommandBarCompletionsMatches(QStringList() << "def"
                                                     << "abc"
                                                     << "xyzaaaaaa");
    TestPressKey("\\ctrl-c"); // Dismiss completer
    TestPressKey("\\ctrl-c"); // Dismiss bar.
    FinishTest("");

    // Completion prefix is the current find term.
    clearSearchHistory();
    vi_global->searchHistory()->append("xy:zaaaaaa");
    vi_global->searchHistory()->append("abc");
    vi_global->searchHistory()->append("def");
    vi_global->searchHistory()->append("xy:zbaaaaa");
    vi_global->searchHistory()->append("xy:zcaaaaa");
    BeginTest("");
    TestPressKey(":s//replace/g\\ctrl-dxy:z\\ctrl-p");
    verifyCommandBarCompletionVisible();
    verifyCommandBarCompletionsMatches(QStringList() << "xy:zcaaaaa"
                                                     << "xy:zbaaaaa"
                                                     << "xy:zaaaaaa");
    TestPressKey("\\ctrl-c"); // Dismiss completer
    TestPressKey("\\ctrl-c"); // Dismiss bar.
    FinishTest("");

    // Replace entire search term with completion.
    clearSearchHistory();
    vi_global->searchHistory()->append("ab,cd");
    vi_global->searchHistory()->append("ab,xy");
    BeginTest("");
    TestPressKey(":s//replace/g\\ctrl-dab,\\ctrl-p\\ctrl-p");
    QCOMPARE(emulatedCommandBarTextEdit()->text(), QString("s/ab,cd/replace/g"));
    TestPressKey("\\ctrl-c"); // Dismiss completer
    TestPressKey("\\ctrl-c"); // Dismiss bar.
    TestPressKey(":'<,'>s//replace/g\\ctrl-dab,\\ctrl-p\\ctrl-p");
    QCOMPARE(emulatedCommandBarTextEdit()->text(), QString("'<,'>s/ab,cd/replace/g"));
    TestPressKey("\\ctrl-c"); // Dismiss completer
    TestPressKey("\\ctrl-c"); // Dismiss bar.
    FinishTest("");

    // Place the cursor at the end of find term.
    clearSearchHistory();
    vi_global->searchHistory()->append("ab,xy");
    BeginTest("");
    TestPressKey(":s//replace/g\\ctrl-dab,\\ctrl-pX");
    QCOMPARE(emulatedCommandBarTextEdit()->text(), QString("s/ab,xyX/replace/g"));
    TestPressKey("\\ctrl-c"); // Dismiss completer
    TestPressKey("\\ctrl-c"); // Dismiss bar.
    TestPressKey(":.,.+7s//replace/g\\ctrl-dab,\\ctrl-pX");
    QCOMPARE(emulatedCommandBarTextEdit()->text(), QString(".,.+7s/ab,xyX/replace/g"));
    TestPressKey("\\ctrl-c"); // Dismiss completer
    TestPressKey("\\ctrl-c"); // Dismiss bar.
    FinishTest("");

    // Leave find term unchanged if there is no search history.
    clearSearchHistory();
    BeginTest("");
    TestPressKey(":s/nose/replace/g\\ctrl-b\\right\\right\\right\\ctrl-p");
    QCOMPARE(emulatedCommandBarTextEdit()->text(), QString("s/nose/replace/g"));
    TestPressKey("\\ctrl-c"); // Dismiss completer
    TestPressKey("\\ctrl-c"); // Dismiss bar.
    FinishTest("");

    // Leave cursor position unchanged if there is no search history.
    clearSearchHistory();
    BeginTest("");
    TestPressKey(":s/nose/replace/g\\ctrl-b\\right\\right\\right\\ctrl-pX");
    QCOMPARE(emulatedCommandBarTextEdit()->text(), QString("s/nXose/replace/g"));
    TestPressKey("\\ctrl-c"); // Dismiss completer
    TestPressKey("\\ctrl-c"); // Dismiss bar.
    FinishTest("");

    // ctrl-p on the first character of the replace term in a sed-replace should
    // invoke replace history completion.
    clearSearchHistory();
    clearReplaceHistory();
    clearCommandHistory();
    vi_global->replaceHistory()->append("replace");
    BeginTest("");
    TestPressKey(":s/search/replace/g\\left\\left\\left\\left\\left\\left\\left\\left\\left\\ctrl-p");
    verifyCommandBarCompletionVisible();
    TestPressKey("\\ctrl-c"); // Dismiss completer
    TestPressKey("\\ctrl-c"); // Dismiss bar.
    TestPressKey(":'<,'>s/search/replace/g\\left\\left\\left\\left\\left\\left\\left\\left\\left\\ctrl-p");
    verifyCommandBarCompletionVisible();
    TestPressKey("\\ctrl-c"); // Dismiss completer
    TestPressKey("\\ctrl-c"); // Dismiss bar.
    FinishTest("");

    // ctrl-p on the last character of the replace term in a sed-replace should
    // invoke replace history completion.
    clearReplaceHistory();
    vi_global->replaceHistory()->append("replace");
    BeginTest("");
    TestPressKey(":s/xyz/replace/g\\left\\left\\ctrl-p");
    verifyCommandBarCompletionVisible();
    TestPressKey("\\ctrl-c"); // Dismiss completer
    TestPressKey("\\ctrl-c"); // Dismiss bar.
    TestPressKey(":'<,'>s/xyz/replace/g\\left\\left\\ctrl-p");
    verifyCommandBarCompletionVisible();
    TestPressKey("\\ctrl-c"); // Dismiss completer
    TestPressKey("\\ctrl-c"); // Dismiss bar.
    FinishTest("");

    // ctrl-p on some arbitrary character of the search term in a sed-replace should
    // invoke search history completion.
    clearReplaceHistory();
    vi_global->replaceHistory()->append("replaceaaaaaa");
    BeginTest("");
    TestPressKey(":s/xyzaaaaaa/replace/g\\left\\left\\left\\left\\ctrl-p");
    verifyCommandBarCompletionVisible();
    TestPressKey("\\ctrl-c"); // Dismiss completer
    TestPressKey("\\ctrl-c"); // Dismiss bar.
    TestPressKey(":'<,'>s/xyzaaaaaa/replace/g\\left\\left\\left\\left\\ctrl-p");
    verifyCommandBarCompletionVisible();
    TestPressKey("\\ctrl-c"); // Dismiss completer
    TestPressKey("\\ctrl-c"); // Dismiss bar.
    FinishTest("");

    // ctrl-p on some character *after" the replace term should
    // *not* invoke replace history completion.
    // Note: in s/xyz/replace/g, the "/" after the "e" is counted as part of the replace term;
    // this allows us to do replace<ctrl-p> and get completions.
    clearSearchHistory();
    clearCommandHistory();
    clearReplaceHistory();
    vi_global->replaceHistory()->append("xyz");
    BeginTest("");
    TestPressKey(":s/xyz/replace/g\\left\\ctrl-p");
    QVERIFY(!emulatedCommandBarCompleter()->popup()->isVisible());
    TestPressKey("\\ctrl-c"); // Dismiss completer
    TestPressKey("\\ctrl-c"); // Dismiss bar.
    clearSearchHistory();
    clearCommandHistory();
    TestPressKey(":'<,'>s/xyz/replace/g\\left\\ctrl-p");
    QVERIFY(!emulatedCommandBarCompleter()->popup()->isVisible());
    TestPressKey("\\ctrl-c"); // Dismiss completer
    TestPressKey("\\ctrl-c"); // Dismiss bar.

    // (Replace history should be reversed).
    clearReplaceHistory();
    vi_global->replaceHistory()->append("xyzaaaaaa");
    vi_global->replaceHistory()->append("abc");
    vi_global->replaceHistory()->append("def");
    BeginTest("");
    TestPressKey(":s/search//g\\left\\left\\ctrl-p");
    verifyCommandBarCompletionVisible();
    verifyCommandBarCompletionsMatches(QStringList() << "def"
                                                     << "abc"
                                                     << "xyzaaaaaa");
    TestPressKey("\\ctrl-c"); // Dismiss completer
    TestPressKey("\\ctrl-c"); // Dismiss bar.
    FinishTest("");

    // Completion prefix is the current replace term.
    clearReplaceHistory();
    vi_global->replaceHistory()->append("xy:zaaaaaa");
    vi_global->replaceHistory()->append("abc");
    vi_global->replaceHistory()->append("def");
    vi_global->replaceHistory()->append("xy:zbaaaaa");
    vi_global->replaceHistory()->append("xy:zcaaaaa");
    BeginTest("");
    TestPressKey(":'<,'>s/replace/search/g\\ctrl-fxy:z\\ctrl-p");
    verifyCommandBarCompletionVisible();
    verifyCommandBarCompletionsMatches(QStringList() << "xy:zcaaaaa"
                                                     << "xy:zbaaaaa"
                                                     << "xy:zaaaaaa");
    TestPressKey("\\ctrl-c"); // Dismiss completer
    TestPressKey("\\ctrl-c"); // Dismiss bar.
    FinishTest("");

    // Replace entire search term with completion.
    clearReplaceHistory();
    clearSearchHistory();
    vi_global->replaceHistory()->append("ab,cd");
    vi_global->replaceHistory()->append("ab,xy");
    BeginTest("");
    TestPressKey(":s/search//g\\ctrl-fab,\\ctrl-p\\ctrl-p");
    QCOMPARE(emulatedCommandBarTextEdit()->text(), QString("s/search/ab,cd/g"));
    TestPressKey("\\ctrl-c"); // Dismiss completer
    TestPressKey("\\ctrl-c"); // Dismiss bar.
    TestPressKey(":'<,'>s/search//g\\ctrl-fab,\\ctrl-p\\ctrl-p");
    QCOMPARE(emulatedCommandBarTextEdit()->text(), QString("'<,'>s/search/ab,cd/g"));
    TestPressKey("\\ctrl-c"); // Dismiss completer
    TestPressKey("\\ctrl-c"); // Dismiss bar.
    FinishTest("");

    // Place the cursor at the end of replace term.
    clearReplaceHistory();
    vi_global->replaceHistory()->append("ab,xy");
    BeginTest("");
    TestPressKey(":s/search//g\\ctrl-fab,\\ctrl-pX");
    QCOMPARE(emulatedCommandBarTextEdit()->text(), QString("s/search/ab,xyX/g"));
    TestPressKey("\\ctrl-c"); // Dismiss completer
    TestPressKey("\\ctrl-c"); // Dismiss bar.
    TestPressKey(":.,.+7s/search//g\\ctrl-fab,\\ctrl-pX");
    QCOMPARE(emulatedCommandBarTextEdit()->text(), QString(".,.+7s/search/ab,xyX/g"));
    TestPressKey("\\ctrl-c"); // Dismiss completer
    TestPressKey("\\ctrl-c"); // Dismiss bar.
    FinishTest("");

    // Leave replace term unchanged if there is no replace history.
    clearReplaceHistory();
    BeginTest("");
    TestPressKey(":s/nose/replace/g\\left\\left\\left\\ctrl-p");
    QCOMPARE(emulatedCommandBarTextEdit()->text(), QString("s/nose/replace/g"));
    TestPressKey("\\ctrl-c"); // Dismiss completer
    TestPressKey("\\ctrl-c"); // Dismiss bar.
    FinishTest("");

    // Leave cursor position unchanged if there is no replace history.
    clearSearchHistory();
    BeginTest("");
    TestPressKey(":s/nose/replace/g\\left\\left\\left\\left\\ctrl-pX");
    QCOMPARE(emulatedCommandBarTextEdit()->text(), QString("s/nose/replaXce/g"));
    TestPressKey("\\ctrl-c"); // Dismiss completer
    TestPressKey("\\ctrl-c"); // Dismiss bar.
    FinishTest("");

    // Invoke replacement history even when the "find" term is empty.
    BeginTest("");
    clearReplaceHistory();
    clearSearchHistory();
    vi_global->replaceHistory()->append("ab,xy");
    vi_global->searchHistory()->append("whoops");
    TestPressKey(":s///g\\ctrl-f\\ctrl-p");
    QCOMPARE(emulatedCommandBarTextEdit()->text(), QString("s//ab,xy/g"));
    TestPressKey("\\ctrl-c"); // Dismiss completer
    TestPressKey("\\ctrl-c"); // Dismiss bar.
    FinishTest("");

    // Move the cursor back to the last manual edit point when aborting completion.
    BeginTest("");
    clearSearchHistory();
    vi_global->searchHistory()->append("xyzaaaaa");
    TestPressKey(":s/xyz/replace/g\\ctrl-b\\right\\right\\right\\right\\righta\\ctrl-p\\ctrl-[X");
    QCOMPARE(emulatedCommandBarTextEdit()->text(), QString("s/xyzaX/replace/g"));
    TestPressKey("\\ctrl-c"); // Dismiss completer
    TestPressKey("\\ctrl-c"); // Dismiss bar.
    FinishTest("");

    // Don't blank the "find" term if there is no search history that begins with the
    // current "find" term.
    BeginTest("");
    clearSearchHistory();
    vi_global->searchHistory()->append("doesnothavexyzasaprefix");
    TestPressKey(":s//replace/g\\ctrl-dxyz\\ctrl-p");
    QCOMPARE(emulatedCommandBarTextEdit()->text(), QString("s/xyz/replace/g"));
    TestPressKey("\\ctrl-c"); // Dismiss completer
    TestPressKey("\\ctrl-c"); // Dismiss bar.
    FinishTest("");

    // Escape the delimiter if it occurs in a search history term - searching for it likely won't
    // work, but at least it won't crash!
    BeginTest("");
    clearSearchHistory();
    vi_global->searchHistory()->append("search");
    vi_global->searchHistory()->append("aa/aa\\/a");
    vi_global->searchHistory()->append("ss/ss");
    TestPressKey(":s//replace/g\\ctrl-d\\ctrl-p");
    QCOMPARE(emulatedCommandBarTextEdit()->text(), QString("s/ss\\/ss/replace/g"));
    TestPressKey("\\ctrl-p");
    QCOMPARE(emulatedCommandBarTextEdit()->text(), QString("s/aa\\/aa\\/a/replace/g"));
    TestPressKey("\\ctrl-p");
    QCOMPARE(emulatedCommandBarTextEdit()->text(), QString("s/search/replace/g"));
    TestPressKey("\\ctrl-c"); // Dismiss completer
    TestPressKey("\\ctrl-c"); // Dismiss bar.
    clearSearchHistory(); // Now do the same, but with a different delimiter.
    vi_global->searchHistory()->append("search");
    vi_global->searchHistory()->append("aa:aa\\:a");
    vi_global->searchHistory()->append("ss:ss");
    TestPressKey(":s::replace:g\\ctrl-d\\ctrl-p");
    QCOMPARE(emulatedCommandBarTextEdit()->text(), QString("s:ss\\:ss:replace:g"));
    TestPressKey("\\ctrl-p");
    QCOMPARE(emulatedCommandBarTextEdit()->text(), QString("s:aa\\:aa\\:a:replace:g"));
    TestPressKey("\\ctrl-p");
    QCOMPARE(emulatedCommandBarTextEdit()->text(), QString("s:search:replace:g"));
    TestPressKey("\\ctrl-c"); // Dismiss completer
    TestPressKey("\\ctrl-c"); // Dismiss bar.
    FinishTest("");

    // Remove \C if occurs in search history.
    BeginTest("");
    clearSearchHistory();
    vi_global->searchHistory()->append("s\\Cear\\\\Cch");
    TestPressKey(":s::replace:g\\ctrl-d\\ctrl-p");
    QCOMPARE(emulatedCommandBarTextEdit()->text(), QString("s:sear\\\\Cch:replace:g"));
    TestPressKey("\\ctrl-c"); // Dismiss completer
    TestPressKey("\\ctrl-c"); // Dismiss bar.
    FinishTest("");

    // Don't blank the "replace" term if there is no search history that begins with the
    // current "replace" term.
    BeginTest("");
    clearReplaceHistory();
    vi_global->replaceHistory()->append("doesnothavexyzasaprefix");
    TestPressKey(":s/search//g\\ctrl-fxyz\\ctrl-p");
    QCOMPARE(emulatedCommandBarTextEdit()->text(), QString("s/search/xyz/g"));
    TestPressKey("\\ctrl-c"); // Dismiss completer
    TestPressKey("\\ctrl-c"); // Dismiss bar.
    FinishTest("");

    // Escape the delimiter if it occurs in a replace history term - searching for it likely won't
    // work, but at least it won't crash!
    BeginTest("");
    clearReplaceHistory();
    vi_global->replaceHistory()->append("replace");
    vi_global->replaceHistory()->append("aa/aa\\/a");
    vi_global->replaceHistory()->append("ss/ss");
    TestPressKey(":s/search//g\\ctrl-f\\ctrl-p");
    QCOMPARE(emulatedCommandBarTextEdit()->text(), QString("s/search/ss\\/ss/g"));
    TestPressKey("\\ctrl-p");
    QCOMPARE(emulatedCommandBarTextEdit()->text(), QString("s/search/aa\\/aa\\/a/g"));
    TestPressKey("\\ctrl-p");
    QCOMPARE(emulatedCommandBarTextEdit()->text(), QString("s/search/replace/g"));
    TestPressKey("\\ctrl-c"); // Dismiss completer
    TestPressKey("\\ctrl-c"); // Dismiss bar.
    clearReplaceHistory(); // Now do the same, but with a different delimiter.
    vi_global->replaceHistory()->append("replace");
    vi_global->replaceHistory()->append("aa:aa\\:a");
    vi_global->replaceHistory()->append("ss:ss");
    TestPressKey(":s:search::g\\ctrl-f\\ctrl-p");
    QCOMPARE(emulatedCommandBarTextEdit()->text(), QString("s:search:ss\\:ss:g"));
    TestPressKey("\\ctrl-p");
    QCOMPARE(emulatedCommandBarTextEdit()->text(), QString("s:search:aa\\:aa\\:a:g"));
    TestPressKey("\\ctrl-p");
    QCOMPARE(emulatedCommandBarTextEdit()->text(), QString("s:search:replace:g"));
    TestPressKey("\\ctrl-c"); // Dismiss completer
    TestPressKey("\\ctrl-c"); // Dismiss bar.
    FinishTest("");

    // In search mode, don't blank current text on completion if there is no item in the search history which
    // has the current text as a prefix.
    BeginTest("");
    clearSearchHistory();
    vi_global->searchHistory()->append("doesnothavexyzasaprefix");
    TestPressKey("/xyz\\ctrl-p");
    QCOMPARE(emulatedCommandBarTextEdit()->text(), QString("xyz"));
    TestPressKey("\\ctrl-c"); // Dismiss completer
    TestPressKey("\\ctrl-c"); // Dismiss bar.
    FinishTest("");

    // Don't dismiss the command completion just because the cursor ends up *temporarily* at a place where
    // command completion is disallowed when cycling through completions.
    BeginTest("");
    TestPressKey(":set/se\\left\\left\\left-\\ctrl-p");
    verifyCommandBarCompletionVisible();
    TestPressKey("\\ctrl-c"); // Dismiss completer
    TestPressKey("\\ctrl-c"); // Dismiss bar.
    FinishTest("");

    // Don't expand mappings meant for Normal mode in the emulated command bar.
    clearAllMappings();
    vi_global->mappings()->add(Mappings::NormalModeMapping, "foo", "xyz", Mappings::NonRecursive);
    DoTest("bar foo xyz", "/foo\\enterrX", "bar Xoo xyz");
    clearAllMappings();

    // Incremental search and replace.
    QLabel *interactiveSedReplaceLabel = emulatedCommandBar->findChild<QLabel *>("interactivesedreplace");
    QVERIFY(interactiveSedReplaceLabel);

    BeginTest("foo");
    TestPressKey(":s/foo/bar/c\\enter");
    QVERIFY(interactiveSedReplaceLabel->isVisible());
    QVERIFY(!commandResponseMessageDisplay()->isVisible());
    QVERIFY(!emulatedCommandBarTextEdit()->isVisible());
    QVERIFY(!emulatedCommandTypeIndicator()->isVisible());
    TestPressKey("\\ctrl-c"); // Dismiss search and replace.
    QVERIFY(!emulatedCommandBar->isVisible());
    FinishTest("foo");

    // Clear the flag that stops the command response from being shown after an incremental search and
    // replace, and also make sure that the edit and bar type indicator are not forcibly hidden.
    BeginTest("foo");
    TestPressKey(":s/foo/bar/c\\enter\\ctrl-c");
    TestPressKey(":s/foo/bar/");
    QVERIFY(emulatedCommandBarTextEdit()->isVisible());
    QVERIFY(emulatedCommandTypeIndicator()->isVisible());
    TestPressKey("\\enter");
    QVERIFY(commandResponseMessageDisplay()->isVisible());
    FinishTest("bar");

    // Hide the incremental search and replace label when we show the bar.
    BeginTest("foo");
    TestPressKey(":s/foo/bar/c\\enter\\ctrl-c");
    TestPressKey(":");
    QVERIFY(!interactiveSedReplaceLabel->isVisible());
    TestPressKey("\\ctrl-c");
    FinishTest("foo");

    // The "c" marker can be anywhere in the three chars following the delimiter.
    BeginTest("foo");
    TestPressKey(":s/foo/bar/cgi\\enter");
    QVERIFY(interactiveSedReplaceLabel->isVisible());
    TestPressKey("\\ctrl-c");
    FinishTest("foo");
    BeginTest("foo");
    TestPressKey(":s/foo/bar/igc\\enter");
    QVERIFY(interactiveSedReplaceLabel->isVisible());
    TestPressKey("\\ctrl-c");
    FinishTest("foo");
    BeginTest("foo");
    TestPressKey(":s/foo/bar/icg\\enter");
    QVERIFY(interactiveSedReplaceLabel->isVisible());
    TestPressKey("\\ctrl-c");
    FinishTest("foo");
    BeginTest("foo");
    TestPressKey(":s/foo/bar/ic\\enter");
    QVERIFY(interactiveSedReplaceLabel->isVisible());
    TestPressKey("\\ctrl-c");
    FinishTest("foo");
    BeginTest("foo");
    TestPressKey(":s/foo/bar/ci\\enter");
    QVERIFY(interactiveSedReplaceLabel->isVisible());
    TestPressKey("\\ctrl-c");
    FinishTest("foo");

    // Emulated command bar is still active during an incremental search and replace.
    BeginTest("foo");
    TestPressKey(":s/foo/bar/c\\enter");
    TestPressKey("idef\\esc");
    FinishTest("foo");

    // Emulated command bar text is not edited during an incremental search and replace.
    BeginTest("foo");
    TestPressKey(":s/foo/bar/c\\enter");
    TestPressKey("def");
    QCOMPARE(emulatedCommandBarTextEdit()->text(), QString("s/foo/bar/c"));
    TestPressKey("\\ctrl-c");
    FinishTest("foo");

    // Pressing "n" when there is only a single  change we can make aborts incremental search
    // and replace.
    BeginTest("foo");
    TestPressKey(":s/foo/bar/c\\enter");
    TestPressKey("n");
    QVERIFY(!interactiveSedReplaceLabel->isVisible());
    TestPressKey("ixyz\\esc");
    FinishTest("xyzfoo");

    // Pressing "n" when there is only a single  change we can make aborts incremental search
    // and replace, and shows the no replacements on no lines.
    BeginTest("foo");
    TestPressKey(":s/foo/bar/c\\enter");
    TestPressKey("n");
    QVERIFY(commandResponseMessageDisplay()->isVisible());
    verifyShowsNumberOfReplacementsAcrossNumberOfLines(0, 0);
    FinishTest("foo");

    // First possible match is highlighted when we start an incremental search and replace, and
    // cleared if we press 'n'.
    BeginTest(" xyz  123 foo bar");
    TestPressKey(":s/foo/bar/gc\\enter");
    QCOMPARE(rangesOnFirstLine().size(), rangesInitial.size() + 1);
    QCOMPARE(rangesOnFirstLine().first()->start().line(), 0);
    QCOMPARE(rangesOnFirstLine().first()->start().column(), 10);
    QCOMPARE(rangesOnFirstLine().first()->end().line(), 0);
    QCOMPARE(rangesOnFirstLine().first()->end().column(), 13);
    TestPressKey("n");
    QCOMPARE(rangesOnFirstLine().size(), rangesInitial.size());
    FinishTest(" xyz  123 foo bar");

    // Second possible match highlighted if we start incremental search and replace and press 'n',
    // cleared if we press 'n' again.
    BeginTest(" xyz  123 foo foo bar");
    TestPressKey(":s/foo/bar/gc\\enter");
    TestPressKey("n");
    QCOMPARE(rangesOnFirstLine().size(), rangesInitial.size() + 1);
    QCOMPARE(rangesOnFirstLine().first()->start().line(), 0);
    QCOMPARE(rangesOnFirstLine().first()->start().column(), 14);
    QCOMPARE(rangesOnFirstLine().first()->end().line(), 0);
    QCOMPARE(rangesOnFirstLine().first()->end().column(), 17);
    TestPressKey("n");
    QCOMPARE(rangesOnFirstLine().size(), rangesInitial.size());
    FinishTest(" xyz  123 foo foo bar");

    // Perform replacement if we press 'y' on the first match.
    BeginTest(" xyz  foo 123 foo bar");
    TestPressKey(":s/foo/bar/gc\\enter");
    TestPressKey("y");
    TestPressKey("\\ctrl-c");
    FinishTest(" xyz  bar 123 foo bar");

    // Replacement uses grouping, etc.
    BeginTest(" xyz  def 123 foo bar");
    TestPressKey(":s/d\\\\(e\\\\)\\\\(f\\\\)/x\\\\1\\\\U\\\\2/gc\\enter");
    TestPressKey("y");
    TestPressKey("\\ctrl-c");
    FinishTest(" xyz  xeF 123 foo bar");

    // On replacement, highlight next match.
    BeginTest(" xyz  foo 123 foo bar");
    TestPressKey(":s/foo/bar/cg\\enter");
    TestPressKey("y");
    QCOMPARE(rangesOnFirstLine().size(), rangesInitial.size() + 1);
    QCOMPARE(rangesOnFirstLine().first()->start().line(), 0);
    QCOMPARE(rangesOnFirstLine().first()->start().column(), 14);
    QCOMPARE(rangesOnFirstLine().first()->end().line(), 0);
    QCOMPARE(rangesOnFirstLine().first()->end().column(), 17);
    TestPressKey("\\ctrl-c");
    FinishTest(" xyz  bar 123 foo bar");

    // On replacement, if there is no further match, abort incremental search and replace.
    BeginTest(" xyz  foo 123 foa bar");
    TestPressKey(":s/foo/bar/cg\\enter");
    TestPressKey("y");
    QVERIFY(commandResponseMessageDisplay()->isVisible());
    TestPressKey("ggidone\\esc");
    FinishTest("done xyz  bar 123 foa bar");

    // After replacement, the next match is sought after the end of the replacement text.
    BeginTest("foofoo");
    TestPressKey(":s/foo/barfoo/cg\\enter");
    TestPressKey("y");
    QCOMPARE(rangesOnFirstLine().size(), rangesInitial.size() + 1);
    QCOMPARE(rangesOnFirstLine().first()->start().line(), 0);
    QCOMPARE(rangesOnFirstLine().first()->start().column(), 6);
    QCOMPARE(rangesOnFirstLine().first()->end().line(), 0);
    QCOMPARE(rangesOnFirstLine().first()->end().column(), 9);
    TestPressKey("\\ctrl-c");
    FinishTest("barfoofoo");
    BeginTest("xffy");
    TestPressKey(":s/f/bf/cg\\enter");
    TestPressKey("yy");
    FinishTest("xbfbfy");

    // Make sure the incremental search bar label contains the "instruction" keypresses.
    const QString interactiveSedReplaceShortcuts = "(y/n/a/q/l)";
    BeginTest("foofoo");
    TestPressKey(":s/foo/barfoo/cg\\enter");
    QVERIFY(interactiveSedReplaceLabel->text().contains(interactiveSedReplaceShortcuts));
    TestPressKey("\\ctrl-c");
    FinishTest("foofoo");

    // Make sure the incremental search bar label contains a reference to the text we're going to
    // replace with.
    // We're going to be a bit vague about the precise text due to localization issues.
    BeginTest("fabababbbar");
    TestPressKey(":s/f\\\\([ab]\\\\+\\\\)/1\\\\U\\\\12/c\\enter");
    QVERIFY(interactiveSedReplaceLabel->text().contains("1ABABABBBA2"));
    TestPressKey("\\ctrl-c");
    FinishTest("fabababbbar");

    // Replace newlines in the "replace?" message with "\\n"
    BeginTest("foo");
    TestPressKey(":s/foo/bar\\\\nxyz\\\\n123/c\\enter");
    QVERIFY(interactiveSedReplaceLabel->text().contains("bar\\nxyz\\n123"));
    TestPressKey("\\ctrl-c");
    FinishTest("foo");

    // Update the "confirm replace?" message on pressing "y".
    BeginTest("fabababbbar fabbb");
    TestPressKey(":s/f\\\\([ab]\\\\+\\\\)/1\\\\U\\\\12/gc\\enter");
    TestPressKey("y");
    QVERIFY(interactiveSedReplaceLabel->text().contains("1ABBB2"));
    QVERIFY(interactiveSedReplaceLabel->text().contains(interactiveSedReplaceShortcuts));
    TestPressKey("\\ctrl-c");
    FinishTest("1ABABABBBA2r fabbb");

    // Update the "confirm replace?" message on pressing "n".
    BeginTest("fabababbbar fabab");
    TestPressKey(":s/f\\\\([ab]\\\\+\\\\)/1\\\\U\\\\12/gc\\enter");
    TestPressKey("n");
    QVERIFY(interactiveSedReplaceLabel->text().contains("1ABAB2"));
    QVERIFY(interactiveSedReplaceLabel->text().contains(interactiveSedReplaceShortcuts));
    TestPressKey("\\ctrl-c");
    FinishTest("fabababbbar fabab");

    // Cursor is placed at the beginning of first match.
    BeginTest("  foo foo foo");
    TestPressKey(":s/foo/bar/c\\enter");
    verifyCursorAt(Cursor(0, 2));
    TestPressKey("\\ctrl-c");
    FinishTest("  foo foo foo");

    // "y" and "n" update the cursor pos.
    BeginTest("  foo   foo foo");
    TestPressKey(":s/foo/bar/cg\\enter");
    TestPressKey("y");
    verifyCursorAt(Cursor(0, 8));
    TestPressKey("n");
    verifyCursorAt(Cursor(0, 12));
    TestPressKey("\\ctrl-c");
    FinishTest("  bar   foo foo");

    // If we end due to a "y" or "n" on the final match, leave the cursor at the beginning of the final match.
    BeginTest("  foo");
    TestPressKey(":s/foo/bar/c\\enter");
    TestPressKey("y");
    verifyCursorAt(Cursor(0, 2));
    FinishTest("  bar");
    BeginTest("  foo");
    TestPressKey(":s/foo/bar/c\\enter");
    TestPressKey("n");
    verifyCursorAt(Cursor(0, 2));
    FinishTest("  foo");

    // Respect ranges.
    BeginTest("foo foo\nfoo foo\nfoo foo\nfoo foo\n");
    TestPressKey("jVj:s/foo/bar/gc\\enter");
    TestPressKey("ynny");
    QVERIFY(commandResponseMessageDisplay()->isVisible());
    TestPressKey("ggidone \\ctrl-c");
    FinishTest("done foo foo\nbar foo\nfoo bar\nfoo foo\n");
    BeginTest("foo foo\nfoo foo\nfoo foo\nfoo foo\n");
    TestPressKey("jVj:s/foo/bar/gc\\enter");
    TestPressKey("nyyn");
    QVERIFY(commandResponseMessageDisplay()->isVisible());
    TestPressKey("ggidone \\ctrl-c");
    FinishTest("done foo foo\nfoo bar\nbar foo\nfoo foo\n");
    BeginTest("foo foo\nfoo foo\nfoo foo\nfoo foo\n");
    TestPressKey("j:s/foo/bar/gc\\enter");
    TestPressKey("ny");
    QVERIFY(commandResponseMessageDisplay()->isVisible());
    TestPressKey("ggidone \\ctrl-c");
    FinishTest("done foo foo\nfoo bar\nfoo foo\nfoo foo\n");
    BeginTest("foo foo\nfoo foo\nfoo foo\nfoo foo\n");
    TestPressKey("j:s/foo/bar/gc\\enter");
    TestPressKey("yn");
    QVERIFY(commandResponseMessageDisplay()->isVisible());
    TestPressKey("ggidone \\ctrl-c");
    FinishTest("done foo foo\nbar foo\nfoo foo\nfoo foo\n");

    // If no initial match can be found, abort and show a "no replacements" message.
    // The cursor position should remain unnchanged.
    BeginTest("fab");
    TestPressKey("l:s/fee/bar/c\\enter");
    QVERIFY(commandResponseMessageDisplay()->isVisible());
    verifyShowsNumberOfReplacementsAcrossNumberOfLines(0, 0);
    QVERIFY(!interactiveSedReplaceLabel->isVisible());
    TestPressKey("rX");
    BeginTest("fXb");

    // Case-sensitive by default.
    BeginTest("foo Foo FOo foo foO");
    TestPressKey(":s/foo/bar/cg\\enter");
    TestPressKey("yyggidone\\esc");
    FinishTest("donebar Foo FOo bar foO");

    // Case-insensitive if "i" flag is used.
    BeginTest("foo Foo FOo foo foO");
    TestPressKey(":s/foo/bar/icg\\enter");
    TestPressKey("yyyyyggidone\\esc");
    FinishTest("donebar bar bar bar bar");

    // Only one replacement per-line unless "g" flag is used.
    BeginTest("boo foo 123 foo\nxyz foo foo\nfoo foo foo\nxyz\nfoo foo\nfoo 123 foo");
    TestPressKey("jVjjj:s/foo/bar/c\\enter");
    TestPressKey("yynggidone\\esc");
    FinishTest("doneboo foo 123 foo\nxyz bar foo\nbar foo foo\nxyz\nfoo foo\nfoo 123 foo");
    BeginTest("boo foo 123 foo\nxyz foo foo\nfoo foo foo\nxyz\nfoo foo\nfoo 123 foo");
    TestPressKey("jVjjj:s/foo/bar/c\\enter");
    TestPressKey("nnyggidone\\esc");
    FinishTest("doneboo foo 123 foo\nxyz foo foo\nfoo foo foo\nxyz\nbar foo\nfoo 123 foo");

    // If replacement contains new lines, adjust the end line down.
    BeginTest("foo\nfoo1\nfoo2\nfoo3");
    TestPressKey("jVj:s/foo/bar\\\\n/gc\\enter");
    TestPressKey("yyggidone\\esc");
    FinishTest("donefoo\nbar\n1\nbar\n2\nfoo3");
    BeginTest("foo\nfoo1\nfoo2\nfoo3");
    TestPressKey("jVj:s/foo/bar\\\\nboo\\\\n/gc\\enter");
    TestPressKey("yyggidone\\esc");
    FinishTest("donefoo\nbar\nboo\n1\nbar\nboo\n2\nfoo3");

    // With "g" and a replacement that involves multiple lines, resume search from the end of the last line added.
    BeginTest("foofoo");
    TestPressKey(":s/foo/bar\\\\n/gc\\enter");
    TestPressKey("yyggidone\\esc");
    FinishTest("donebar\nbar\n");
    BeginTest("foofoo");
    TestPressKey(":s/foo/bar\\\\nxyz\\\\nfoo/gc\\enter");
    TestPressKey("yyggidone\\esc");
    FinishTest("donebar\nxyz\nfoobar\nxyz\nfoo");

    // Without "g" and with a replacement that involves multiple lines, resume search from the line after the line just added.
    BeginTest("foofoo1\nfoo2\nfoo3");
    TestPressKey("Vj:s/foo/bar\\\\nxyz\\\\nfoo/c\\enter");
    TestPressKey("yyggidone\\esc");
    FinishTest("donebar\nxyz\nfoofoo1\nbar\nxyz\nfoo2\nfoo3");

    // Regression test: handle 'g' when it occurs before 'i' and 'c'.
    BeginTest("foo fOo");
    TestPressKey(":s/foo/bar/gci\\enter");
    TestPressKey("yyggidone\\esc");
    FinishTest("donebar bar");

    // When the search terms swallows several lines, move the endline up accordingly.
    BeginTest("foo\nfoo1\nfoo\nfoo2\nfoo\nfoo3");
    TestPressKey("V3j:s/foo\\\\nfoo/bar/cg\\enter");
    TestPressKey("yyggidone\\esc");
    FinishTest("donebar1\nbar2\nfoo\nfoo3");
    BeginTest("foo\nfoo\nfoo1\nfoo\nfoo\nfoo2\nfoo\nfoo\nfoo3");
    TestPressKey("V5j:s/foo\\\\nfoo\\\\nfoo/bar/cg\\enter");
    TestPressKey("yyggidone\\esc");
    FinishTest("donebar1\nbar2\nfoo\nfoo\nfoo3");
    // Make sure we still adjust endline down if the replacement text has '\n's.
    BeginTest("foo\nfoo\nfoo1\nfoo\nfoo\nfoo2\nfoo\nfoo\nfoo3");
    TestPressKey("V5j:s/foo\\\\nfoo\\\\nfoo/bar\\\\n/cg\\enter");
    TestPressKey("yyggidone\\esc");
    FinishTest("donebar\n1\nbar\n2\nfoo\nfoo\nfoo3");

    // Status reports.
    BeginTest("foo");
    TestPressKey(":s/foo/bar/c\\enter");
    TestPressKey("y");
    verifyShowsNumberOfReplacementsAcrossNumberOfLines(1, 1);
    FinishTest("bar");
    BeginTest("foo foo foo");
    TestPressKey(":s/foo/bar/gc\\enter");
    TestPressKey("yyy");
    verifyShowsNumberOfReplacementsAcrossNumberOfLines(3, 1);
    FinishTest("bar bar bar");
    BeginTest("foo foo foo");
    TestPressKey(":s/foo/bar/gc\\enter");
    TestPressKey("yny");
    verifyShowsNumberOfReplacementsAcrossNumberOfLines(2, 1);
    FinishTest("bar foo bar");
    BeginTest("foo\nfoo");
    TestPressKey(":%s/foo/bar/gc\\enter");
    TestPressKey("yy");
    verifyShowsNumberOfReplacementsAcrossNumberOfLines(2, 2);
    FinishTest("bar\nbar");
    BeginTest("foo foo\nfoo foo\nfoo foo");
    TestPressKey(":%s/foo/bar/gc\\enter");
    TestPressKey("yynnyy");
    verifyShowsNumberOfReplacementsAcrossNumberOfLines(4, 2);
    FinishTest("bar bar\nfoo foo\nbar bar");
    BeginTest("foofoo");
    TestPressKey(":s/foo/bar\\\\nxyz/gc\\enter");
    TestPressKey("yy");
    verifyShowsNumberOfReplacementsAcrossNumberOfLines(2, 1);
    FinishTest("bar\nxyzbar\nxyz");
    BeginTest("foofoofoo");
    TestPressKey(":s/foo/bar\\\\nxyz\\\\nboo/gc\\enter");
    TestPressKey("yyy");
    verifyShowsNumberOfReplacementsAcrossNumberOfLines(3, 1);
    FinishTest("bar\nxyz\nboobar\nxyz\nboobar\nxyz\nboo");
    // Tricky one: how many lines are "touched" if a single replacement
    // swallows multiple lines? I'm going to say the number of lines swallowed.
    BeginTest("foo\nfoo\nfoo");
    TestPressKey(":s/foo\\\\nfoo\\\\nfoo/bar/c\\enter");
    TestPressKey("y");
    verifyShowsNumberOfReplacementsAcrossNumberOfLines(1, 3);
    FinishTest("bar");
    BeginTest("foo\nfoo\nfoo\n");
    TestPressKey(":s/foo\\\\nfoo\\\\nfoo\\\\n/bar/c\\enter");
    TestPressKey("y");
    verifyShowsNumberOfReplacementsAcrossNumberOfLines(1, 4);
    FinishTest("bar");

    // "Undo" undoes last replacement.
    BeginTest("foo foo foo foo");
    TestPressKey(":s/foo/bar/cg\\enter");
    TestPressKey("nyynu");
    FinishTest("foo bar foo foo");

    // "l" does the current replacement then exits.
    BeginTest("foo foo foo foo foo foo");
    TestPressKey(":s/foo/bar/cg\\enter");
    TestPressKey("nnl");
    verifyShowsNumberOfReplacementsAcrossNumberOfLines(1, 1);
    FinishTest("foo foo bar foo foo foo");

    // "q" just exits.
    BeginTest("foo foo foo foo foo foo");
    TestPressKey(":s/foo/bar/cg\\enter");
    TestPressKey("yyq");
    verifyShowsNumberOfReplacementsAcrossNumberOfLines(2, 1);
    FinishTest("bar bar foo foo foo foo");

    // "a" replaces all remaining, then exits.
    BeginTest("foo foo foo foo foo foo");
    TestPressKey(":s/foo/bar/cg\\enter");
    TestPressKey("nna");
    verifyShowsNumberOfReplacementsAcrossNumberOfLines(4, 1);
    FinishTest("foo foo bar bar bar bar");

    // The results of "a" can be undone in one go.
    BeginTest("foo foo foo foo foo foo");
    TestPressKey(":s/foo/bar/cg\\enter");
    TestPressKey("ya");
    verifyShowsNumberOfReplacementsAcrossNumberOfLines(6, 1);
    TestPressKey("u");
    FinishTest("bar foo foo foo foo foo");
#if 0
    // XXX - as of Qt 5.5, simply replaying the correct QKeyEvents does *not* cause shortcuts
    // to be triggered, so these tests cannot pass.
    // It's possible that a solution involving QTestLib will be workable in the future, though.

  {
    // Test the test suite: ensure that shortcuts are still being sent and received correctly.
    // The test shortcut chosen should be one that does not conflict with built-in Kate ones.
    FailsIfSlotNotCalled failsIfActionNotTriggered;
    QAction *dummyAction = kate_view->actionCollection()->addAction("Woo");
    dummyAction->setShortcut(QKeySequence("Ctrl+]"));
    QVERIFY(connect(dummyAction, SIGNAL(triggered()), &failsIfActionNotTriggered, SLOT(slot())));
    DoTest("foo", "\\ctrl-]", "foo");
    // Processing shortcuts seems to require events to be processed.
    while (QApplication::hasPendingEvents())
    {
      QApplication::processEvents();
    }
    delete dummyAction;
  }
  {
    // Test that shortcuts involving ctrl+<digit> work correctly.
    FailsIfSlotNotCalled failsIfActionNotTriggered;
    QAction *dummyAction = kate_view->actionCollection()->addAction("Woo");
    dummyAction->setShortcut(QKeySequence("Ctrl+1"));
    QVERIFY(connect(dummyAction, SIGNAL(triggered()), &failsIfActionNotTriggered, SLOT(slot())));
    DoTest("foo", "\\ctrl-1", "foo");
    // Processing shortcuts seems to require events to be processed.
    while (QApplication::hasPendingEvents())
    {
      QApplication::processEvents();
    }
    delete dummyAction;
  }
  {
    // Test that shortcuts involving alt+<digit> work correctly.
    FailsIfSlotNotCalled failsIfActionNotTriggered;
    QAction *dummyAction = kate_view->actionCollection()->addAction("Woo");
    dummyAction->setShortcut(QKeySequence("Alt+1"));
    QVERIFY(connect(dummyAction, SIGNAL(triggered()), &failsIfActionNotTriggered, SLOT(slot())));
    DoTest("foo", "\\alt-1", "foo");
    // Processing shortcuts seems to require events to be processed.
    while (QApplication::hasPendingEvents())
    {
      QApplication::processEvents();
    }
    delete dummyAction;
  }
#endif

    // Find the "Print" action for later use.
    QAction *printAction = nullptr;
    const auto viewActions = kate_view->actionCollection()->actions();
    for (QAction *action : viewActions) {
        if (action->shortcut() == QKeySequence("Ctrl+p")) {
            printAction = action;
            break;
        }
    }

    // Test that we don't inadvertantly trigger shortcuts in kate_view when typing them in the
    // emulated command bar.  Requires the above test for shortcuts to be sent and received correctly
    // to pass.
    {
        QVERIFY(mainWindow->isActiveWindow());
        QVERIFY(printAction);
        FailsIfSlotCalled failsIfActionTriggered("The kate_view shortcut should not be triggered by typing it in emulated  command bar!");
        // Don't invoke Print on failure, as this hangs instead of failing.
        // disconnect(printAction, SIGNAL(triggered(bool)), kate_document, SLOT(print()));
        connect(printAction, &QAction::triggered, &failsIfActionTriggered, &FailsIfSlotCalled::slot);
        DoTest("foo bar foo bar", "/bar\\enterggd/\\ctrl-p\\enter.", "bar");
        // Processing shortcuts seems to require events to be processed.
        QApplication::processEvents();
    }

    // Test that the interactive search replace does not handle general keypresses like ctrl-p ("invoke
    // completion in emulated command bar").
    // Unfortunately, "ctrl-p" in kate_view, which is what will be triggered if this
    // test succeeds, hangs due to showing the print dialog, so we need to temporarily
    // block the Print action.
    clearCommandHistory();
    if (printAction) {
        printAction->blockSignals(true);
    }
    vi_global->commandHistory()->append("s/foo/bar/caa");
    BeginTest("foo");
    TestPressKey(":s/foo/bar/c\\ctrl-b\\enter\\ctrl-p");
    QVERIFY(!emulatedCommandBarCompleter()->popup()->isVisible());
    TestPressKey("\\ctrl-c");
    if (printAction) {
        printAction->blockSignals(false);
    }
    FinishTest("foo");

    // The interactive sed replace command is added to the history straight away.
    clearCommandHistory();
    BeginTest("foo");
    TestPressKey(":s/foo/bar/c\\enter");
    QCOMPARE(commandHistory(), QStringList() << "s/foo/bar/c");
    TestPressKey("\\ctrl-c");
    FinishTest("foo");
    clearCommandHistory();
    BeginTest("foo");
    TestPressKey(":s/notfound/bar/c\\enter");
    QCOMPARE(commandHistory(), QStringList() << "s/notfound/bar/c");
    TestPressKey("\\ctrl-c");
    FinishTest("foo");

    // Should be usable in mappings.
    clearAllMappings();
    vi_global->mappings()->add(Mappings::NormalModeMapping, "H", ":s/foo/bar/gc<enter>nnyyl", Mappings::Recursive);
    DoTest("foo foo foo foo foo foo", "H", "foo foo bar bar bar foo");
    clearAllMappings();
    vi_global->mappings()->add(Mappings::NormalModeMapping, "H", ":s/foo/bar/gc<enter>nna", Mappings::Recursive);
    DoTest("foo foo foo foo foo foo", "H", "foo foo bar bar bar bar");
    clearAllMappings();
    vi_global->mappings()->add(Mappings::NormalModeMapping, "H", ":s/foo/bar/gc<enter>nnyqggidone<esc>", Mappings::Recursive);
    DoTest("foo foo foo foo foo foo", "H", "donefoo foo bar foo foo foo");

#ifndef Q_OS_MACOS
    // Don't swallow "Ctrl+<key>" meant for the text edit.
    // On MacOS the QKeySequence for undo is defined as "Ctrl+Z" as well, however it's actually Cmd+Z...
    if (QKeySequence::keyBindings(QKeySequence::Undo).contains(QKeySequence("Ctrl+Z"))) {
        DoTest("foo bar", "/bar\\ctrl-z\\enterrX", "Xoo bar");
    } else {
        qWarning() << "Skipped test: Ctrl+Z is not Undo on this platform";
    }
#endif

    // Don't give invalid cursor position to updateCursor in Visual Mode: it will cause a crash!
    DoTest("xyz\nfoo\nbar\n123", "/foo\\\\nbar\\\\n\\enterggv//e\\enter\\ctrl-crX", "xyz\nfoo\nbaX\n123");
    DoTest("\nfooxyz\nbar;\n", "/foo.*\\\\n.*;\\enterggv//e\\enter\\ctrl-crX", "\nfooxyz\nbarX\n");
}

QCompleter *EmulatedCommandBarTest::emulatedCommandBarCompleter()
{
    return vi_input_mode->viModeEmulatedCommandBar()->findChild<QCompleter *>("completer");
}

void EmulatedCommandBarTest::verifyCommandBarCompletionVisible()
{
    if (!emulatedCommandBarCompleter()->popup()->isVisible()) {
        qDebug() << "Emulated command bar completer not visible.";
        QStringListModel *completionModel = qobject_cast<QStringListModel *>(emulatedCommandBarCompleter()->model());
        Q_ASSERT(completionModel);
        const QStringList allAvailableCompletions = completionModel->stringList();
        qDebug() << " Completion list: " << allAvailableCompletions;
        qDebug() << " Completion prefix: " << emulatedCommandBarCompleter()->completionPrefix();
        bool candidateCompletionFound = false;
        for (const QString &availableCompletion : allAvailableCompletions) {
            if (availableCompletion.startsWith(emulatedCommandBarCompleter()->completionPrefix())) {
                candidateCompletionFound = true;
                break;
            }
        }
        if (candidateCompletionFound) {
            qDebug() << " The current completion prefix is a prefix of one of the available completions, so either complete() was not called, or the popup was "
                        "manually hidden since then";
        } else {
            qDebug() << " The current completion prefix is not a prefix of one of the available completions; this may or may not be why it is not visible";
        }
    }
    QVERIFY(emulatedCommandBarCompleter()->popup()->isVisible());
}

void EmulatedCommandBarTest::verifyCommandBarCompletionsMatches(const QStringList &expectedCompletionList)
{
    verifyCommandBarCompletionVisible();
    QStringList actualCompletionList;
    for (int i = 0; emulatedCommandBarCompleter()->setCurrentRow(i); i++) {
        actualCompletionList << emulatedCommandBarCompleter()->currentCompletion();
    }
    if (expectedCompletionList != actualCompletionList) {
        qDebug() << "Actual completions:\n " << actualCompletionList << "\n\ndo not match expected:\n" << expectedCompletionList;
    }

    QCOMPARE(actualCompletionList, expectedCompletionList);
}

void EmulatedCommandBarTest::verifyCommandBarCompletionContains(const QStringList &expectedCompletionList)
{
    verifyCommandBarCompletionVisible();
    QStringList actualCompletionList;

    for (int i = 0; emulatedCommandBarCompleter()->setCurrentRow(i); i++) {
        actualCompletionList << emulatedCommandBarCompleter()->currentCompletion();
    }

    for (const QString &expectedCompletion : expectedCompletionList) {
        if (!actualCompletionList.contains(expectedCompletion)) {
            qDebug() << "Whoops: " << actualCompletionList << " does not contain " << expectedCompletion;
        }
        QVERIFY(actualCompletionList.contains(expectedCompletion));
    }
}

QLabel *EmulatedCommandBarTest::emulatedCommandTypeIndicator()
{
    return emulatedCommandBar()->findChild<QLabel *>("bartypeindicator");
}

void EmulatedCommandBarTest::verifyCursorAt(const Cursor &expectedCursorPos)
{
    QCOMPARE(kate_view->cursorPosition().line(), expectedCursorPos.line());
    QCOMPARE(kate_view->cursorPosition().column(), expectedCursorPos.column());
}

void EmulatedCommandBarTest::clearSearchHistory()
{
    vi_global->searchHistory()->clear();
}

QStringList EmulatedCommandBarTest::searchHistory()
{
    return vi_global->searchHistory()->items();
}

void EmulatedCommandBarTest::clearCommandHistory()
{
    vi_global->commandHistory()->clear();
}

QStringList EmulatedCommandBarTest::commandHistory()
{
    return vi_global->commandHistory()->items();
}

void EmulatedCommandBarTest::clearReplaceHistory()
{
    vi_global->replaceHistory()->clear();
}

QStringList EmulatedCommandBarTest::replaceHistory()
{
    return vi_global->replaceHistory()->items();
}

QVector<Kate::TextRange *> EmulatedCommandBarTest::rangesOnFirstLine()
{
    return kate_document->buffer().rangesForLine(0, kate_view, true);
}

void EmulatedCommandBarTest::verifyTextEditBackgroundColour(const QColor &expectedBackgroundColour)
{
    QCOMPARE(emulatedCommandBarTextEdit()->palette().brush(QPalette::Base).color(), expectedBackgroundColour);
}

QLabel *EmulatedCommandBarTest::commandResponseMessageDisplay()
{
    QLabel *commandResponseMessageDisplay = emulatedCommandBar()->findChild<QLabel *>("commandresponsemessage");
    Q_ASSERT(commandResponseMessageDisplay);
    return commandResponseMessageDisplay;
}

void EmulatedCommandBarTest::waitForEmulatedCommandBarToHide(long int timeout)
{
    QTRY_VERIFY_WITH_TIMEOUT(!emulatedCommandBar()->isVisible(), timeout);
}

void EmulatedCommandBarTest::verifyShowsNumberOfReplacementsAcrossNumberOfLines(int numReplacements, int acrossNumLines)
{
    QVERIFY(commandResponseMessageDisplay()->isVisible());
    QVERIFY(!emulatedCommandTypeIndicator()->isVisible());
    const QString commandMessageResponseText = commandResponseMessageDisplay()->text();
    const QString expectedNumReplacementsAsString = QString::number(numReplacements);
    const QString expectedAcrossNumLinesAsString = QString::number(acrossNumLines);
    // Be a bit vague about the actual contents due to e.g. localization.
    // TODO - see if we can insist that en_US is available on the Kate Jenkins server and
    // insist that we use it ... ?
    static const QRegularExpression numReplacementsMessageRegex("^.*(\\d+).*(\\d+).*$");
    const auto match = numReplacementsMessageRegex.match(commandMessageResponseText);
    QVERIFY(match.hasMatch());
    const QString actualNumReplacementsAsString = match.captured(1);
    const QString actualAcrossNumLinesAsString = match.captured(2);
    QCOMPARE(actualNumReplacementsAsString, expectedNumReplacementsAsString);
    QCOMPARE(actualAcrossNumLinesAsString, expectedAcrossNumLinesAsString);
}

FailsIfSlotNotCalled::FailsIfSlotNotCalled()
    : QObject()
{
}

FailsIfSlotNotCalled::~FailsIfSlotNotCalled()
{
    QVERIFY(m_slotWasCalled);
}

void FailsIfSlotNotCalled::slot()
{
    m_slotWasCalled = true;
}

FailsIfSlotCalled::FailsIfSlotCalled(const QString &failureMessage)
    : QObject()
    , m_failureMessage(failureMessage)
{
}

void FailsIfSlotCalled::slot()
{
    QFAIL(qPrintable(m_failureMessage.toLatin1()));
}

#include "moc_emulatedcommandbar.cpp"
