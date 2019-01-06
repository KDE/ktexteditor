/*
 * This file is part of the KDE libraries
 *
 * Copyright (C) 2014 Miquel Sabaté Solà <mikisabate@gmail.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 * You should have received a copy of the GNU Library General Public License
 * along with this library; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */


#include <KLocalizedString>
#include <kateconfig.h>
#include <vimode/keymapper.h>
#include <vimode/keyparser.h>
#include <vimode/emulatedcommandbar/emulatedcommandbar.h>
#include <inputmode/kateviinputmode.h>
#include "keys.h"
#include "emulatedcommandbarsetupandteardown.h"
#include "fakecodecompletiontestmodel.h"
#include "vimode/mappings.h"
#include "vimode/globalstate.h"

using namespace KTextEditor;
using KateVi::Mappings;
using KateVi::KeyParser;


QTEST_MAIN(KeysTest)

//BEGIN: KeysTest

void KeysTest::MappingTests()
{
//     QVERIFY(false);
    const int mappingTimeoutMSOverride = QString::fromLocal8Bit(qgetenv("KATE_VIMODE_TEST_MAPPINGTIMEOUTMS")).toInt();
    const int mappingTimeoutMS = (mappingTimeoutMSOverride > 0) ? mappingTimeoutMSOverride : 2000;
    KateViewConfig::global()->setViInputModeStealKeys(true); // For tests involving e.g. <c-a>
    {
        // Check storage and retrieval of mapping recursion.
        clearAllMappings();

        vi_global->mappings()->add(Mappings::NormalModeMapping, "'", "ihello", Mappings::Recursive);
        QVERIFY(vi_global->mappings()->isRecursive(Mappings::NormalModeMapping, "'"));

        vi_global->mappings()->add(Mappings::NormalModeMapping, "a", "ihello", Mappings::NonRecursive);
        QVERIFY(!vi_global->mappings()->isRecursive(Mappings::NormalModeMapping, "a"));
    }

    clearAllMappings();

    vi_global->mappings()->add(Mappings::NormalModeMapping, "'", "<esc>ihello<esc>^aworld<esc>", Mappings::Recursive);
    DoTest("", "'", "hworldello");

    // Ensure that the non-mapping logged keypresses are cleared before we execute a mapping
    vi_global->mappings()->add(Mappings::NormalModeMapping, "'a", "rO", Mappings::Recursive);
    DoTest("X", "'a", "O");

    {
        // Check that '123 is mapped after the timeout, given that we also have mappings that
        // extend it (e.g. '1234, '12345, etc) and which it itself extends ('1, '12, etc).
        clearAllMappings();
        BeginTest("");
        vi_input_mode_manager->keyMapper()->setMappingTimeout(mappingTimeoutMS);;
        QString consectiveDigits;
        for (int i = 1; i < 9; i++) {
            consectiveDigits += QString::number(i);
            vi_global->mappings()->add(Mappings::NormalModeMapping, '\'' + consectiveDigits, "iMapped from " + consectiveDigits + "<esc>", Mappings::Recursive);
        }
        TestPressKey("'123");
        QCOMPARE(kate_document->text(), QString()); // Shouldn't add anything until after the timeout!
        QTest::qWait(2 * mappingTimeoutMS);
        FinishTest("Mapped from 123");
    }

    // Mappings are not "counted": any count entered applies to the first command/ motion in the mapped sequence,
    // and is not used to replay the entire mapped sequence <count> times in a row.
    clearAllMappings();
    vi_global->mappings()->add(Mappings::NormalModeMapping, "'downmapping", "j", Mappings::Recursive);
    vi_global->mappings()->add(Mappings::NormalModeMapping, "'testmapping", "ifoo<esc>ibar<esc>", Mappings::Recursive);
    vi_global->mappings()->add(Mappings::NormalModeMapping, "'testmotionmapping", "lj", Mappings::Recursive);
    DoTest("AAAA\nXXXX\nXXXX\nXXXX\nXXXX\nBBBB\nCCCC\nDDDD", "jd3'downmapping", "AAAA\nBBBB\nCCCC\nDDDD");
    DoTest("", "5'testmapping", "foofoofoofoofobaro");
    DoTest("XXXX\nXXXX\nXXXX\nXXXX", "3'testmotionmappingrO", "XXXX\nXXXO\nXXXX\nXXXX");

    // Regression test for a weird mistake I made: *completely* remove all counting for the
    // first command in the sequence; don't just set it to 1! If it is set to 1, then "%"
    // will mean "go to position 1 percent of the way through the document" rather than
    // go to matching item.
    clearAllMappings();
    vi_global->mappings()->add(Mappings::NormalModeMapping, "gl", "%", Mappings::Recursive);
    DoTest("0\n1\n2\n3\n4\n5\nfoo bar(xyz) baz", "jjjjjjwdgl", "0\n1\n2\n3\n4\n5\nfoo  baz");

    // Test that countable mappings work even when triggered by timeouts.
    clearAllMappings();
    vi_global->mappings()->add(Mappings::NormalModeMapping, "'testmapping", "ljrO", Mappings::Recursive);
    vi_global->mappings()->add(Mappings::NormalModeMapping, "'testmappingdummy", "dummy", Mappings::Recursive);
    BeginTest("XXXX\nXXXX\nXXXX\nXXXX");
    vi_input_mode_manager->keyMapper()->setMappingTimeout(mappingTimeoutMS);;
    TestPressKey("3'testmapping");
    QTest::qWait(2 * mappingTimeoutMS);
    FinishTest("XXXX\nXXXO\nXXXX\nXXXX");

    // Test that telescoping mappings don't interfere with built-in commands. Assumes that gp
    // is implemented and working.
    clearAllMappings();
    vi_global->mappings()->add(Mappings::NormalModeMapping, "gdummy", "idummy", Mappings::Recursive);
    DoTest("hello", "yiwgpx", "hhellollo");

    // Test that we can map a sequence of keys that extends a built-in command and use
    // that sequence without the built-in command firing.
    // Once again, assumes that gp is implemented and working.
    clearAllMappings();
    vi_global->mappings()->add(Mappings::NormalModeMapping, "gpa", "idummy", Mappings::Recursive);
    DoTest("hello", "yiwgpa", "dummyhello");

    // Test that we can map a sequence of keys that extends a built-in command and still
    // have the original built-in command fire if we timeout after entering that command.
    // Once again, assumes that gp is implemented and working.
    clearAllMappings();
    vi_global->mappings()->add(Mappings::NormalModeMapping, "gpa", "idummy", Mappings::Recursive);
    BeginTest("hello");
    vi_input_mode_manager->keyMapper()->setMappingTimeout(mappingTimeoutMS);;
    TestPressKey("yiwgp");
    QTest::qWait(2 * mappingTimeoutMS);
    TestPressKey("x");
    FinishTest("hhellollo");

    // Test that something that starts off as a partial mapping following a command
    // (the "g" in the first "dg" is a partial mapping of "gj"), when extended to something
    // that is definitely not a mapping ("gg"), results in the full command being executed ("dgg").
    clearAllMappings();
    vi_global->mappings()->add(Mappings::NormalModeMapping, "gj", "aj", Mappings::Recursive);
    DoTest("foo\nbar\nxyz", "jjdgg", "");

    // Make sure that a mapped sequence of commands is merged into a single undo-able edit.
    clearAllMappings();
    vi_global->mappings()->add(Mappings::NormalModeMapping, "'a", "ofoo<esc>ofoo<esc>ofoo<esc>", Mappings::Recursive);
    DoTest("bar", "'au", "bar");

    // Make sure that a counted mapping is merged into a single undoable edit.
    clearAllMappings();
    vi_global->mappings()->add(Mappings::NormalModeMapping, "'a", "ofoo<esc>", Mappings::Recursive);
    DoTest("bar", "5'au", "bar");

    // Some test setup for non-recursive mapping g -> gj (cf: bug:314415)
    // Firstly: work out the expected result of gj (this might be fragile as default settings
    // change, etc.).  We use BeginTest & FinishTest for the setup and teardown etc, but this is
    // not an actual test - it's just computing the expected result of the real test!
    const QString multiVirtualLineText = "foo foo foo foo foo foo foo foo foo foo foo foo foo foo foo foo foo foo foo foo foo foo foo foo foo foo foo foo foo foo foo foo foo foo foo foo foo foo foo foo foo foo foo foo foo foo foo foo foo foo foo foo foo foo foo foo foo foo foo foo foo foo foo foo foo foo foo foo foo foo foo foo foo foo foo foo foo foo foo foo";
    ensureKateViewVisible(); // Needs to be visible in order for virtual lines to make sense.
    KateViewConfig::global()->setDynWordWrap(true);
    BeginTest(multiVirtualLineText);
    TestPressKey("gjrX");
    const QString expectedAfterVirtualLineDownAndChange = kate_document->text();
    Q_ASSERT_X(expectedAfterVirtualLineDownAndChange.contains("X") && !expectedAfterVirtualLineDownAndChange.startsWith('X'), "setting up j->gj testcase data", "gj doesn't seem to have worked correctly!");
    FinishTest(expectedAfterVirtualLineDownAndChange);

    // Test that non-recursive mappings are not expanded.
    clearAllMappings();
    vi_global->mappings()->add(Mappings::NormalModeMapping, "j", "gj", Mappings::NonRecursive);
    DoTest(multiVirtualLineText, "jrX", expectedAfterVirtualLineDownAndChange);
    KateViewConfig::global()->setDynWordWrap(false);

    // Test that recursive mappings are expanded.
    clearAllMappings();
    vi_global->mappings()->add(Mappings::NormalModeMapping, "a", "X", Mappings::Recursive);
    vi_global->mappings()->add(Mappings::NormalModeMapping, "X", "rx", Mappings::Recursive);
    DoTest("foo", "la", "fxo");

    // Test that the flag that stops mappings being expanded is reset after the mapping has been executed.
    clearAllMappings();
    vi_global->mappings()->add(Mappings::NormalModeMapping, "j", "gj", Mappings::NonRecursive);
    vi_global->mappings()->add(Mappings::NormalModeMapping, "a", "X", Mappings::Recursive);
    vi_global->mappings()->add(Mappings::NormalModeMapping, "X", "rx", Mappings::Recursive);
    DoTest("foo", "jla", "fxo");

    // Even if we start with a recursive mapping, as soon as we hit one that is not recursive, we should stop
    // expanding.
    clearAllMappings();
    vi_global->mappings()->add(Mappings::NormalModeMapping, "a", "X", Mappings::NonRecursive);
    vi_global->mappings()->add(Mappings::NormalModeMapping, "X", "r.", Mappings::Recursive);
    vi_global->mappings()->add(Mappings::NormalModeMapping, "i", "a", Mappings::Recursive);
    DoTest("foo", "li", "oo");

    // Regression test: Using a mapping may trigger a call to updateSelection(), which can change the mode
    // from VisualLineMode to plain VisualMode.
    clearAllMappings();
    vi_global->mappings()->add(Mappings::VisualModeMapping, "gA", "%", Mappings::NonRecursive);
    DoTest("xyz\nfoo\n{\nbar\n}", "jVjgAdgglP", "foo\n{\nbar\n}\nxyz");
    // Piggy back on the previous test with a regression test for issue where, if gA is mapped to %, vgly
    // will yank one more character than it should.
    DoTest("foo(bar)X", "vgAyp", "ffoo(bar)oo(bar)X");
    // Make sure that a successful mapping does not break the "if we select stuff externally in Normal mode,
    // we should switch to Visual Mode" thing.
    clearAllMappings();
    vi_global->mappings()->add(Mappings::NormalModeMapping, "gA", "%", Mappings::NonRecursive);
    BeginTest("foo bar xyz()");
    TestPressKey("gAr.");
    kate_view->setSelection(Range(0, 1, 0, 4)); // Actually selects "oo " (i.e. without the "b").
    TestPressKey("d");
    FinishTest("fbar xyz(.");

    // Regression tests for BUG:260655
    clearAllMappings();
    vi_global->mappings()->add(Mappings::NormalModeMapping, "a", "f", Mappings::NonRecursive);
    vi_global->mappings()->add(Mappings::NormalModeMapping, "d", "i", Mappings::NonRecursive);
    DoTest("foo dar", "adr.", "foo .ar");
    clearAllMappings();
    vi_global->mappings()->add(Mappings::NormalModeMapping, "a", "F", Mappings::NonRecursive);
    vi_global->mappings()->add(Mappings::NormalModeMapping, "d", "i", Mappings::NonRecursive);
    DoTest("foo dar", "$adr.", "foo .ar");
    clearAllMappings();
    vi_global->mappings()->add(Mappings::NormalModeMapping, "a", "t", Mappings::NonRecursive);
    vi_global->mappings()->add(Mappings::NormalModeMapping, "d", "i", Mappings::NonRecursive);
    DoTest("foo dar", "adr.", "foo.dar");
    clearAllMappings();
    vi_global->mappings()->add(Mappings::NormalModeMapping, "a", "T", Mappings::NonRecursive);
    vi_global->mappings()->add(Mappings::NormalModeMapping, "d", "i", Mappings::NonRecursive);
    DoTest("foo dar", "$adr.", "foo d.r");
    clearAllMappings();
    vi_global->mappings()->add(Mappings::NormalModeMapping, "a", "r", Mappings::NonRecursive);
    vi_global->mappings()->add(Mappings::NormalModeMapping, "d", "i", Mappings::NonRecursive);
    DoTest("foo dar", "ad", "doo dar");
    // Feel free to map the keypress after that, though.
    DoTest("foo dar", "addber\\esc", "berdoo dar");
    // Also, be careful about how we interpret "waiting for find char/ replace char"
    DoTest("foo dar", "ffas", "soo dar");

    // Ignore raw "Ctrl", "Shift", "Meta" and "Alt" keys, which will almost certainly end up being pressed as
    // we try to trigger mappings that contain these keys.
    clearAllMappings();
    {
        // Ctrl.
        vi_global->mappings()->add(Mappings::NormalModeMapping, "<c-a><c-b>", "ictrl<esc>", Mappings::NonRecursive);
        BeginTest("");
        QKeyEvent *ctrlKeyDown = new QKeyEvent(QEvent::KeyPress, Qt::Key_Control, Qt::NoModifier);
        QApplication::postEvent(kate_view->focusProxy(), ctrlKeyDown);
        QApplication::sendPostedEvents();
        TestPressKey("\\ctrl-a");
        ctrlKeyDown = new QKeyEvent(QEvent::KeyPress, Qt::Key_Control, Qt::NoModifier);
        QApplication::postEvent(kate_view->focusProxy(), ctrlKeyDown);
        QApplication::sendPostedEvents();
        TestPressKey("\\ctrl-b");
        FinishTest("ctrl");
    }
    {
        // Shift.
        vi_global->mappings()->add(Mappings::NormalModeMapping, "<c-a>C", "ishift<esc>", Mappings::NonRecursive);
        BeginTest("");
        QKeyEvent *ctrlKeyDown = new QKeyEvent(QEvent::KeyPress, Qt::Key_Control, Qt::NoModifier);
        QApplication::postEvent(kate_view->focusProxy(), ctrlKeyDown);
        QApplication::sendPostedEvents();
        TestPressKey("\\ctrl-a");
        QKeyEvent *shiftKeyDown = new QKeyEvent(QEvent::KeyPress, Qt::Key_Shift, Qt::NoModifier);
        QApplication::postEvent(kate_view->focusProxy(), shiftKeyDown);
        QApplication::sendPostedEvents();
        TestPressKey("C");
        FinishTest("shift");
    }
    {
        // Alt.
        vi_global->mappings()->add(Mappings::NormalModeMapping, "<c-a><a-b>", "ialt<esc>", Mappings::NonRecursive);
        BeginTest("");
        QKeyEvent *ctrlKeyDown = new QKeyEvent(QEvent::KeyPress, Qt::Key_Control, Qt::NoModifier);
        QApplication::postEvent(kate_view->focusProxy(), ctrlKeyDown);
        QApplication::sendPostedEvents();
        TestPressKey("\\ctrl-a");
        QKeyEvent *altKeyDown = new QKeyEvent(QEvent::KeyPress, Qt::Key_Alt, Qt::NoModifier);
        QApplication::postEvent(kate_view->focusProxy(), altKeyDown);
        QApplication::sendPostedEvents();
        TestPressKey("\\alt-b");
        FinishTest("alt");
    }
    {
        // Meta.
        vi_global->mappings()->add(Mappings::NormalModeMapping, "<c-a><m-b>", "imeta<esc>", Mappings::NonRecursive);
        BeginTest("");
        QKeyEvent *ctrlKeyDown = new QKeyEvent(QEvent::KeyPress, Qt::Key_Control, Qt::NoModifier);
        QApplication::postEvent(kate_view->focusProxy(), ctrlKeyDown);
        QApplication::sendPostedEvents();
        TestPressKey("\\ctrl-a");
        QKeyEvent *metaKeyDown = new QKeyEvent(QEvent::KeyPress, Qt::Key_Meta, Qt::NoModifier);
        QApplication::postEvent(kate_view->focusProxy(), metaKeyDown);
        QApplication::sendPostedEvents();
        TestPressKey("\\meta-b");
        FinishTest("meta");
    }
    {
        // Can have mappings in Visual mode, distinct from Normal mode..
        clearAllMappings();
        vi_global->mappings()->add(Mappings::VisualModeMapping, "a", "3l", Mappings::NonRecursive);
        vi_global->mappings()->add(Mappings::NormalModeMapping, "a", "inose<esc>", Mappings::NonRecursive);
        DoTest("0123456", "lvad", "056");

        // The recursion in Visual Mode is distinct from that of  Normal mode.
        clearAllMappings();
        vi_global->mappings()->add(Mappings::VisualModeMapping, "b", "<esc>", Mappings::NonRecursive);
        vi_global->mappings()->add(Mappings::VisualModeMapping, "a", "b", Mappings::NonRecursive);
        vi_global->mappings()->add(Mappings::NormalModeMapping, "a", "b", Mappings::Recursive);
        DoTest("XXX\nXXX", "lvajd", "XXX");
        clearAllMappings();
        vi_global->mappings()->add(Mappings::VisualModeMapping, "b", "<esc>", Mappings::NonRecursive);
        vi_global->mappings()->add(Mappings::VisualModeMapping, "a", "b", Mappings::Recursive);
        vi_global->mappings()->add(Mappings::NormalModeMapping, "a", "b", Mappings::NonRecursive);
        DoTest("XXX\nXXX", "lvajd", "XXX\nXXX");

        // A Visual mode mapping applies to all Visual modes (line, block, etc).
        clearAllMappings();
        vi_global->mappings()->add(Mappings::VisualModeMapping, "a", "2j", Mappings::NonRecursive);
        DoTest("123\n456\n789", "lvad", "19");
        DoTest("123\n456\n789", "l\\ctrl-vad", "13\n46\n79");
        DoTest("123\n456\n789", "lVad", "");
        // Same for recursion.
        clearAllMappings();
        vi_global->mappings()->add(Mappings::VisualModeMapping, "b", "2j", Mappings::NonRecursive);
        vi_global->mappings()->add(Mappings::VisualModeMapping, "a", "b", Mappings::Recursive);
        DoTest("123\n456\n789", "lvad", "19");
        DoTest("123\n456\n789", "l\\ctrl-vad", "13\n46\n79");
        DoTest("123\n456\n789", "lVad", "");

        // Can clear Visual mode mappings.
        clearAllMappings();
        vi_global->mappings()->add(Mappings::VisualModeMapping, "h", "l", Mappings::Recursive);
        vi_global->mappings()->clear(Mappings::VisualModeMapping);
        DoTest("123\n456\n789", "lvhd", "3\n456\n789");
        DoTest("123\n456\n789", "l\\ctrl-vhd", "3\n456\n789");
        DoTest("123\n456\n789", "lVhd", "456\n789");
        vi_global->mappings()->add(Mappings::VisualModeMapping, "h", "l", Mappings::Recursive);
        vi_global->mappings()->clear(Mappings::VisualModeMapping);
        DoTest("123\n456\n789", "lvhd", "3\n456\n789");
        DoTest("123\n456\n789", "l\\ctrl-vhd", "3\n456\n789");
        DoTest("123\n456\n789", "lVhd", "456\n789");
        vi_global->mappings()->add(Mappings::VisualModeMapping, "h", "l", Mappings::Recursive);
        vi_global->mappings()->clear(Mappings::VisualModeMapping);
        DoTest("123\n456\n789", "lvhd", "3\n456\n789");
        DoTest("123\n456\n789", "l\\ctrl-vhd", "3\n456\n789");
        DoTest("123\n456\n789", "lVhd", "456\n789");
    }
    {
        // Can have mappings in Insert mode.
        clearAllMappings();
        vi_global->mappings()->add(Mappings::InsertModeMapping, "a", "xyz", Mappings::NonRecursive);
        vi_global->mappings()->add(Mappings::NormalModeMapping, "a", "inose<esc>", Mappings::NonRecursive);
        DoTest("foo", "ia\\esc", "xyzfoo");

        // Recursion for Insert mode.
        clearAllMappings();
        vi_global->mappings()->add(Mappings::InsertModeMapping, "b", "c", Mappings::NonRecursive);
        vi_global->mappings()->add(Mappings::InsertModeMapping, "a", "b", Mappings::NonRecursive);
        DoTest("", "ia\\esc", "b");
        clearAllMappings();
        vi_global->mappings()->add(Mappings::InsertModeMapping, "b", "c", Mappings::NonRecursive);
        vi_global->mappings()->add(Mappings::InsertModeMapping, "a", "b", Mappings::Recursive);
        DoTest("", "ia\\esc", "c");

        clearAllMappings();
        // Clear mappings for Insert mode.
        vi_global->mappings()->add(Mappings::InsertModeMapping, "a", "b", Mappings::NonRecursive);
        vi_global->mappings()->clear(Mappings::InsertModeMapping);
        DoTest("", "ia\\esc", "a");
    }

    {
        EmulatedCommandBarSetUpAndTearDown vimStyleCommandBarTestsSetUpAndTearDown(vi_input_mode, kate_view, mainWindow);
        // Can have mappings in Emulated Command Bar.
        clearAllMappings();
        vi_global->mappings()->add(Mappings::CommandModeMapping, "a", "xyz", Mappings::NonRecursive);
        DoTest(" a xyz", "/a\\enterrX", " a Xyz");
        // Use mappings from Normal mode as soon as we exit command bar via Enter.
        vi_global->mappings()->add(Mappings::NormalModeMapping, "a", "ixyz<c-c>", Mappings::NonRecursive);
        DoTest(" a xyz", "/a\\entera", " a xyzxyz");
        // Multiple mappings.
        vi_global->mappings()->add(Mappings::CommandModeMapping, "b", "123", Mappings::NonRecursive);
        DoTest("  xyz123", "/ab\\enterrX", "  Xyz123");
        // Recursive mappings.
        vi_global->mappings()->add(Mappings::CommandModeMapping, "b", "a", Mappings::Recursive);
        DoTest("  xyz", "/b\\enterrX", "  Xyz");
        // Can clear all.
        vi_global->mappings()->clear(Mappings::CommandModeMapping);
        DoTest("  ab xyz xyz123", "/ab\\enterrX", "  Xb xyz xyz123");
    }

    // Test that not *both* of the mapping and the mapped keys are logged for repetition via "."
    clearAllMappings();
    vi_global->mappings()->add(Mappings::NormalModeMapping, "ixyz", "iabc", Mappings::NonRecursive);
    vi_global->mappings()->add(Mappings::NormalModeMapping, "gl", "%", Mappings::NonRecursive);
    DoTest("", "ixyz\\esc.", "ababcc");
    DoTest("foo()X\nbarxyz()Y", "cglbaz\\escggj.", "bazX\nbazY");

    // Regression test for a crash when executing a mapping that switches to Normal mode.
    clearAllMappings();
    vi_global->mappings()->add(Mappings::VisualModeMapping, "h", "d", Mappings::Recursive);
    DoTest("foo", "vlh", "o");

    {
        // Test that we can set/ unset mappings from the command-line.
        clearAllMappings();
        DoTest("", "\\:nn foo ibar<esc>\\foo", "bar");

        // "nn" is not recursive.
        clearAllMappings();
        vi_global->mappings()->add(Mappings::NormalModeMapping, "l", "iabc<esc>", Mappings::NonRecursive);
        DoTest("xxx", "\\:nn foo l\\foorX", "xXx");

        // "no" is not recursive.
        clearAllMappings();
        vi_global->mappings()->add(Mappings::NormalModeMapping, "l", "iabc<esc>", Mappings::NonRecursive);
        DoTest("xxx", "\\:no foo l\\foorX", "xXx");

        // "noremap" is not recursive.
        clearAllMappings();
        vi_global->mappings()->add(Mappings::NormalModeMapping, "l", "iabc<esc>", Mappings::NonRecursive);
        DoTest("xxx", "\\:noremap foo l\\foorX", "xXx");

        // "nm" is recursive.
        clearAllMappings();
        vi_global->mappings()->add(Mappings::NormalModeMapping, "l", "iabc<esc>", Mappings::NonRecursive);
        DoTest("xxx", "\\:nm foo l\\foorX", "abXxxx");

        // "nmap" is recursive.
        clearAllMappings();
        vi_global->mappings()->add(Mappings::NormalModeMapping, "l", "iabc<esc>", Mappings::NonRecursive);
        DoTest("xxx", "\\:nmap foo l\\foorX", "abXxxx");

        // Unfortunately, "map" is a reserved word :/
        clearAllMappings();
        vi_global->mappings()->add(Mappings::NormalModeMapping, "l", "iabc<esc>", Mappings::NonRecursive);
        DoTest("xxx", "\\:map foo l\\foorX", "abXxxx", ShouldFail, "'map' is reserved for other stuff in Kate command line");

        // nunmap works in normal mode.
        clearAllMappings();
        vi_global->mappings()->add(Mappings::NormalModeMapping, "w", "ciwabc<esc>", Mappings::NonRecursive);
        vi_global->mappings()->add(Mappings::NormalModeMapping, "b", "ciwxyz<esc>", Mappings::NonRecursive);
        DoTest(" 123 456 789", "\\:nunmap b\\WWwbrX", " 123 Xbc 789");

        // nmap and nunmap whose "from" is a complex encoded expression.
        clearAllMappings();
        BeginTest("123");
        TestPressKey("\\:nmap <c-9> ciwxyz<esc>\\");
        TestPressKey("\\ctrl-9");
        FinishTest("xyz");
        BeginTest("123");
        TestPressKey("\\:nunmap <c-9>\\");
        TestPressKey("\\ctrl-9");
        FinishTest("123");

        // vmap works in Visual mode and is recursive.
        clearAllMappings();
        vi_global->mappings()->add(Mappings::VisualModeMapping, "l", "d", Mappings::NonRecursive);
        DoTest("abco", "\\:vmap foo l\\v\\rightfoogU", "co");

        // vmap does not work in Normal mode.
        clearAllMappings();
        DoTest("xxx", "\\:vmap foo l\\foorX", "xxx\nrX");

        // vm works in Visual mode and is recursive.
        clearAllMappings();
        vi_global->mappings()->add(Mappings::VisualModeMapping, "l", "d", Mappings::NonRecursive);
        DoTest("abco", "\\:vm foo l\\v\\rightfoogU", "co");

        // vn works in Visual mode and is not recursive.
        clearAllMappings();
        vi_global->mappings()->add(Mappings::VisualModeMapping, "l", "d", Mappings::NonRecursive);
        DoTest("abco", "\\:vn foo l\\v\\rightfoogU", "ABCo");

        // vnoremap works in Visual mode and is not recursive.
        clearAllMappings();
        vi_global->mappings()->add(Mappings::VisualModeMapping, "l", "d", Mappings::NonRecursive);
        DoTest("abco", "\\:vnoremap foo l\\v\\rightfoogU", "ABCo");

        // vunmap works in Visual Mode.
        clearAllMappings();
        vi_global->mappings()->add(Mappings::VisualModeMapping, "l", "w", Mappings::NonRecursive);
        vi_global->mappings()->add(Mappings::VisualModeMapping, "gU", "2b", Mappings::NonRecursive);
        DoTest("foo bar xyz", "\\:vunmap gU\\wvlgUd", "foo BAR Xyz");

        // imap works in Insert mode and is recursive.
        clearAllMappings();
        vi_global->mappings()->add(Mappings::InsertModeMapping, "l", "d", Mappings::NonRecursive);
        DoTest("", "\\:imap foo l\\ifoo\\esc", "d");

        // im works in Insert mode and is recursive.
        clearAllMappings();
        vi_global->mappings()->add(Mappings::InsertModeMapping, "l", "d", Mappings::NonRecursive);
        DoTest("", "\\:im foo l\\ifoo\\esc", "d");

        // ino works in Insert mode and is not recursive.
        clearAllMappings();
        vi_global->mappings()->add(Mappings::InsertModeMapping, "l", "d", Mappings::NonRecursive);
        DoTest("", "\\:ino foo l\\ifoo\\esc", "l");

        // inoremap works in Insert mode and is not recursive.
        clearAllMappings();
        vi_global->mappings()->add(Mappings::InsertModeMapping, "l", "d", Mappings::NonRecursive);
        DoTest("", "\\:inoremap foo l\\ifoo\\esc", "l");

        // iunmap works in Insert mode.
        clearAllMappings();
        vi_global->mappings()->add(Mappings::InsertModeMapping, "l", "d", Mappings::NonRecursive);
        vi_global->mappings()->add(Mappings::InsertModeMapping, "m", "e", Mappings::NonRecursive);
        DoTest("", "\\:iunmap l\\ilm\\esc", "le");

        {
            EmulatedCommandBarSetUpAndTearDown vimStyleCommandBarTestsSetUpAndTearDown(vi_input_mode, kate_view, mainWindow);
            // cmap works in emulated command bar and is recursive.
            // NOTE: need to do the cmap call using the direct execution (i.e. \\:cmap blah blah\\), *not* using
            // the emulated command bar (:cmap blah blah\\enter), as this will be subject to mappings, which
            // can interfere with the tests!
            clearAllMappings();
            vi_global->mappings()->add(Mappings::CommandModeMapping, "l", "d", Mappings::NonRecursive);
            DoTest(" l d foo", "\\:cmap foo l\\/foo\\enterrX", " l X foo");

            // cm works in emulated command bar and is recursive.
            clearAllMappings();
            vi_global->mappings()->add(Mappings::CommandModeMapping, "l", "d", Mappings::NonRecursive);
            DoTest(" l d foo", "\\:cm foo l\\/foo\\enterrX", " l X foo");

            // cnoremap works in emulated command bar and is recursive.
            clearAllMappings();
            vi_global->mappings()->add(Mappings::CommandModeMapping, "l", "d", Mappings::NonRecursive);
            DoTest(" l d foo", "\\:cnoremap foo l\\/foo\\enterrX", " X d foo");

            // cno works in emulated command bar and is recursive.
            clearAllMappings();
            vi_global->mappings()->add(Mappings::CommandModeMapping, "l", "d", Mappings::NonRecursive);
            DoTest(" l d foo", "\\:cno foo l\\/foo\\enterrX", " X d foo");

            // cunmap works in emulated command bar.
            clearAllMappings();
            vi_global->mappings()->add(Mappings::CommandModeMapping, "l", "d", Mappings::NonRecursive);
            vi_global->mappings()->add(Mappings::CommandModeMapping, "m", "e", Mappings::NonRecursive);
            DoTest(" de le", "\\:cunmap l\\/lm\\enterrX", " de Xe");
        }

        // Can use <space> to signify a space.
        clearAllMappings();
        DoTest("", "\\:nn h<space> i<space>a<space>b<esc>\\h ", " a b");
    }

    // More recursion tests - don't lose characters from a Recursive mapping if it looks like they might
    // be part of a different mapping (but end up not being so).
    // (Here, the leading "i" in "irecursive<c-c>" could be part of the mapping "ihello<c-c>").
    clearAllMappings();
    vi_global->mappings()->add(Mappings::NormalModeMapping, "'", "ihello<c-c>", Mappings::Recursive);
    vi_global->mappings()->add(Mappings::NormalModeMapping, "ihello<c-c>", "irecursive<c-c>", Mappings::Recursive);
    DoTest("", "'", "recursive");

    // Capslock in insert mode is not handled by Vim nor by KateViewInternal, and ends up
    // being sent to KateViInputModeManager::handleKeypress twice (it could be argued that this is
    // incorrect behaviour on the part of KateViewInternal), which can cause infinite
    // recursion if we are not careful about identifying replayed rejected keypresses.
    BeginTest("foo bar");
    TestPressKey("i");
    QKeyEvent *capslockKeyPress = new QKeyEvent(QEvent::KeyPress, Qt::Key_CapsLock, Qt::NoModifier);
    QApplication::postEvent(kate_view->focusProxy(), capslockKeyPress);
    QApplication::sendPostedEvents();
    FinishTest("foo bar");

    // Mapping the u and the U commands to other keys.
    clearAllMappings();
    vi_global->mappings()->add(Mappings::NormalModeMapping, "t", "u", Mappings::Recursive);
    vi_global->mappings()->add(Mappings::NormalModeMapping, "r", "U", Mappings::Recursive);
    DoTest("", "ihello\\esct", "");
    DoTest("", "ihello\\esctr", "hello");

    // <nop>
    clearAllMappings();
    vi_global->mappings()->add(Mappings::NormalModeMapping, "l", "<nop>", Mappings::Recursive);
    DoTest("Hello", "lrr", "rello");
    clearAllMappings();
    vi_global->mappings()->add(Mappings::InsertModeMapping, "l", "<nop>", Mappings::Recursive);
    DoTest("Hello", "sl\\esc", "ello");
    clearAllMappings();
    vi_global->mappings()->add(Mappings::InsertModeMapping, "l", "<nop>abc", Mappings::Recursive);
    DoTest("Hello", "sl\\esc", "abcello");

    // Clear mappings for subsequent tests.
    clearAllMappings();
}

void KeysTest::LeaderTests()
{
    // Clean slate.
    KateViewConfig::global()->setViInputModeStealKeys(true);
    clearAllMappings();

    // By default the backslash character is the leader. The default leader
    // is picked from the config. If we don't want to mess this from other
    // tests, it's better if we mock the config.
    const QString viTestKConfigFileName = QStringLiteral("vimodetest-leader-katevimoderc");
    KConfig viTestKConfig(viTestKConfigFileName);
    vi_global->mappings()->setLeader(QChar());
    vi_global->readConfig(&viTestKConfig);
    vi_global->mappings()->add(Mappings::NormalModeMapping, "<leader>i", "ii", Mappings::Recursive);
    DoTest("", "\\\\i", "i");

    // We can change the leader and it will work.
    clearAllMappings();
    vi_global->readConfig(&viTestKConfig);
    vi_global->mappings()->setLeader(QChar::fromLatin1(','));
    vi_global->mappings()->add(Mappings::NormalModeMapping, "<leader>i", "ii", Mappings::Recursive);
    DoTest("", ",i", "i");

    // Mixing up the <leader> with its value.
    clearAllMappings();
    vi_global->readConfig(&viTestKConfig);
    vi_global->mappings()->setLeader(QChar::fromLatin1(','));
    vi_global->mappings()->add(Mappings::NormalModeMapping, "<leader>,", "ii", Mappings::Recursive);
    DoTest("", ",,", "i");
    vi_global->mappings()->add(Mappings::NormalModeMapping, ",<leader>", "ii", Mappings::Recursive);
    DoTest("", ",,", "i");

    // It doesn't work outside normal mode.
    clearAllMappings();
    vi_global->readConfig(&viTestKConfig);
    vi_global->mappings()->setLeader(QChar::fromLatin1(','));
    vi_global->mappings()->add(Mappings::InsertModeMapping, "<leader>i", "ii", Mappings::Recursive);
    DoTest("", "i,ii", ",ii");

    // Clear mappings for subsequent tests.
    clearAllMappings();
}

void KeysTest::ParsingTests()
{
    // BUG #298726
    const QChar char_o_diaeresis(246);

    // Test that we can correctly translate finnish key ö
    QKeyEvent *k = new QKeyEvent(QEvent::KeyPress, 214, Qt::NoModifier, 47, 246, 16400, char_o_diaeresis);
    QCOMPARE(KeyParser::self()->KeyEventToQChar(*k), QChar(246));

    // Test that it can be used in mappings
    clearAllMappings();
    vi_global->mappings()->add(Mappings::NormalModeMapping, char_o_diaeresis, "ifoo", Mappings::Recursive);
    DoTest("hello", QString("ll%1bar").arg(char_o_diaeresis), "hefoobarllo");

    // Test that <cr> is parsed like <enter>
    QCOMPARE(KeyParser::self()->vi2qt("cr"), int(Qt::Key_Enter));
    const QString &enter = KeyParser::self()->encodeKeySequence(QLatin1String("<cr>"));
    QCOMPARE(KeyParser::self()->decodeKeySequence(enter), QLatin1String("<cr>"));
}

void KeysTest::AltGr()
{
    QKeyEvent *altGrDown;
    QKeyEvent *altGrUp;

    // Test Alt-gr still works - this isn't quite how things work in "real-life": in real-life, something like
    // Alt-gr+7 would be a "{", but I don't think this can be reproduced without sending raw X11
    // keypresses to Qt, so just duplicate the keypress events we would receive if we pressed
    // Alt-gr+7 (that is: Alt-gr down; "{"; Alt-gr up).
    BeginTest("");
    TestPressKey("i");
    altGrDown = new QKeyEvent(QEvent::KeyPress, Qt::Key_AltGr, Qt::NoModifier);
    QApplication::postEvent(kate_view->focusProxy(), altGrDown);
    QApplication::sendPostedEvents();
    // Not really Alt-gr and 7, but this is the key event that is reported by Qt if we press that.
    QKeyEvent *altGrAnd7 = new QKeyEvent(QEvent::KeyPress, Qt::Key_BraceLeft, Qt::GroupSwitchModifier, "{");
    QApplication::postEvent(kate_view->focusProxy(), altGrAnd7);
    QApplication::sendPostedEvents();
    altGrUp = new QKeyEvent(QEvent::KeyRelease, Qt::Key_AltGr, Qt::NoModifier);
    QApplication::postEvent(kate_view->focusProxy(), altGrUp);
    QApplication::sendPostedEvents();
    TestPressKey("\\ctrl-c");
    FinishTest("{");

    // French Bepo keyabord AltGr + Shift + s = Ù = Unicode(0x00D9);
    const QString ugrave = QString(QChar(0x00D9));
    BeginTest("");
    TestPressKey("i");
    altGrDown = new QKeyEvent(QEvent::KeyPress, Qt::Key_AltGr, Qt::NoModifier);
    QApplication::postEvent(kate_view->focusProxy(), altGrDown);
    QApplication::sendPostedEvents();
    altGrDown = new QKeyEvent(QEvent::KeyPress, Qt::Key_Shift, Qt::ShiftModifier | Qt::GroupSwitchModifier);
    QApplication::postEvent(kate_view->focusProxy(), altGrDown);
    QApplication::sendPostedEvents();
    QKeyEvent *altGrAndUGrave = new QKeyEvent(QEvent::KeyPress, Qt::Key_Ugrave, Qt::ShiftModifier | Qt::GroupSwitchModifier, ugrave);
    qDebug() << QString("%1").arg(altGrAndUGrave->modifiers(), 10, 16);
    QApplication::postEvent(kate_view->focusProxy(), altGrAndUGrave);
    QApplication::sendPostedEvents();
    altGrUp = new QKeyEvent(QEvent::KeyRelease, Qt::Key_AltGr, Qt::NoModifier);
    QApplication::postEvent(kate_view->focusProxy(), altGrUp);
    QApplication::sendPostedEvents();
    FinishTest(ugrave);
}

void KeysTest::MacroTests()
{
    // Update the status on qa.
    const QString macroIsRecordingStatus = QLatin1String("(") + i18n("recording") + QLatin1String(")");
    clearAllMacros();
    BeginTest("");
    QVERIFY(!kate_view->viewModeHuman().contains(macroIsRecordingStatus));
    TestPressKey("qa");
    QVERIFY(kate_view->viewModeHuman().contains(macroIsRecordingStatus));
    TestPressKey("q");
    QVERIFY(!kate_view->viewModeHuman().contains(macroIsRecordingStatus));
    FinishTest("");

    // The closing "q" is not treated as the beginning of a new "begin recording macro" command.
    clearAllMacros();
    BeginTest("foo");
    TestPressKey("qaqa");
    QVERIFY(!kate_view->viewModeHuman().contains(macroIsRecordingStatus));
    TestPressKey("xyz\\esc");
    FinishTest("fxyzoo");

    // Record and playback a single keypress into macro register "a".
    clearAllMacros();
    DoTest("foo bar", "qawqgg@arX", "foo Xar");
    // Two macros - make sure the old one is cleared.
    clearAllMacros();
    DoTest("123 foo bar xyz", "qawqqabqggww@arX", "123 Xoo bar xyz");

    // Update the status on qb.
    clearAllMacros();
    BeginTest("");
    QVERIFY(!kate_view->viewModeHuman().contains(macroIsRecordingStatus));
    TestPressKey("qb");
    QVERIFY(kate_view->viewModeHuman().contains(macroIsRecordingStatus));
    TestPressKey("q");
    QVERIFY(!kate_view->viewModeHuman().contains(macroIsRecordingStatus));
    FinishTest("");

    // Record and playback a single keypress into macro register "b".
    clearAllMacros();
    DoTest("foo bar", "qbwqgg@brX", "foo Xar");

    // More complex macros.
    clearAllMacros();
    DoTest("foo", "qcrXql@c", "XXo");

    // Re-recording a macro should only clear that macro.
    clearAllMacros();
    DoTest("foo 123", "qaraqqbrbqqbrBqw@a", "Boo a23");

    // Empty macro clears it.
    clearAllMacros();
    DoTest("", "qaixyz\\ctrl-cqqaq@a", "xyz");

    // Hold two macros in memory simultanenously so both can be played.
    clearAllMacros();
    DoTest("foo 123", "qaraqqbrbqw@al@b", "boo ab3");

    // Do more complex things, including switching modes and using ctrl codes.
    clearAllMacros();
    DoTest("foo bar", "qainose\\ctrl-c~qw@a", "nosEfoo nosEbar");
    clearAllMacros();
    DoTest("foo bar", "qayiwinose\\ctrl-r0\\ctrl-c~qw@a", "nosefoOfoo nosebaRbar");
    clearAllMacros();
    DoTest("foo bar", "qavldqw@a", "o r");
    // Make sure we can use "q" in insert mode while recording a macro.
    clearAllMacros();
    DoTest("foo bar", "qaiqueequeg\\ctrl-cqw@a", "queequegfoo queequegbar");
    // Can invoke a macro in Visual Mode.
    clearAllMacros();
    DoTest("foo bar", "qa~qvlll@a", "FOO Bar");
    // Invoking a macro in Visual Mode does not exit Visual Mode.
    clearAllMacros();
    DoTest("foo bar", "qallqggv@a~", "FOO bar");;
    // Can record & macros in Visual Mode for playback in Normal Mode.
    clearAllMacros();
    DoTest("foo bar", "vqblq\\ctrl-c@b~", "foO bar");
    // Recording a macro in Visual Mode does not exit Visual Mode.
    clearAllMacros();
    DoTest("foo bar", "vqblql~", "FOO bar");
    // Recognize correctly numbered registers
    clearAllMacros();
    DoTest("foo", "q1iX\\escq@1", "XXfoo");

    {
        // Ensure that we can call emulated command bar searches, and that we don't record
        // synthetic keypresses.
        EmulatedCommandBarSetUpAndTearDown vimStyleCommandBarTestsSetUpAndTearDown(vi_input_mode, kate_view, mainWindow);
        clearAllMacros();
        DoTest("foo bar\nblank line", "qa/bar\\enterqgg@arX", "foo Xar\nblank line");
        // More complex searching stuff.
        clearAllMacros();
        DoTest("foo 123foo123\nbar 123bar123", "qayiw/\\ctrl-r0\\enterrXqggj@a", "foo 123Xoo123\nbar 123Xar123");
    }

    // Expand mappings,  but don't do *both* original keypresses and executed keypresses.
    clearAllMappings();
    vi_global->mappings()->add(Mappings::NormalModeMapping, "'", "ihello<c-c>", Mappings::Recursive);
    clearAllMacros();
    DoTest("", "qa'q@a", "hellhelloo");
    // Actually, just do the mapped keypresses, not the executed mappings (like Vim).
    clearAllMappings();
    vi_global->mappings()->add(Mappings::NormalModeMapping, "'", "ihello<c-c>", Mappings::Recursive);
    clearAllMacros();
    BeginTest("");
    TestPressKey("qa'q");
    vi_global->mappings()->add(Mappings::NormalModeMapping, "'", "igoodbye<c-c>", Mappings::Recursive);
    TestPressKey("@a");
    FinishTest("hellgoodbyeo");
    // Clear the "stop recording macro keypresses because we're executing a mapping" when the mapping has finished
    // executing.
    clearAllMappings();
    vi_global->mappings()->add(Mappings::NormalModeMapping, "'", "ihello<c-c>", Mappings::Recursive);
    clearAllMacros();
    DoTest("", "qa'ixyz\\ctrl-cq@a", "hellxyhellxyzozo");
    // ... make sure that *all* mappings have finished, though: take into account recursion.
    clearAllMappings();
    clearAllMacros();
    vi_global->mappings()->add(Mappings::NormalModeMapping, "'", "ihello<c-c>", Mappings::Recursive);
    vi_global->mappings()->add(Mappings::NormalModeMapping, "ihello<c-c>", "irecursive<c-c>", Mappings::Recursive);
    DoTest("", "qa'q@a", "recursivrecursivee");
    clearAllMappings();
    clearAllMacros();
    vi_global->mappings()->add(Mappings::NormalModeMapping, "'", "ihello<c-c>ixyz<c-c>", Mappings::Recursive);
    vi_global->mappings()->add(Mappings::NormalModeMapping, "ihello<c-c>", "irecursive<c-c>", Mappings::Recursive);
    DoTest("", "qa'q@a", "recursivxyrecursivxyzeze");

    clearAllMappings();
    clearAllMacros();
    // Don't save the trailing "q" with macros, and also test that we can call one macro from another,
    // without one of the macros being repeated.
    DoTest("", "qaixyz\\ctrl-cqqb@aq@b", "xyxyxyzzz");
    clearAllMappings();
    clearAllMacros();
    // More stringent test that macros called from another macro aren't repeated - requires more nesting
    // of macros ('a' calls 'b' calls 'c').
    DoTest("", "qciC\\ctrl-cq"
           "qb@ciB\\ctrl-cq"
           "qa@biA\\ctrl-cq"
           "dd@a", "ABC");
    // Don't crash if we invoke a non-existent macro.
    clearAllMacros();
    DoTest("", "@x", "");
    // Make macros "counted".
    clearAllMacros();
    DoTest("XXXX\nXXXX\nXXXX\nXXXX", "qarOljq3@a", "OXXX\nXOXX\nXXOX\nXXXO");

    // A macro can be undone with one undo.
    clearAllMacros();
    DoTest("foo bar", "qaciwxyz\\ctrl-ci123\\ctrl-cqw@au", "xy123z bar");
    // As can a counted macro.
    clearAllMacros();
    DoTest("XXXX\nXXXX\nXXXX\nXXXX", "qarOljq3@au", "OXXX\nXXXX\nXXXX\nXXXX");

    {
        EmulatedCommandBarSetUpAndTearDown vimStyleCommandBarTestsSetUpAndTearDown(vi_input_mode, kate_view, mainWindow);
        // Make sure we can macro-ise an interactive sed replace.
        clearAllMacros();
        DoTest("foo foo foo foo\nfoo foo foo foo", "qa:s/foo/bar/gc\\enteryynyAdone\\escqggj@a", "bar bar foo bardone\nbar bar foo bardone");
        // Make sure the closing "q" in the interactive sed replace isn't mistaken for a macro's closing "q".
        clearAllMacros();
        DoTest("foo foo foo foo\nfoo foo foo foo", "qa:s/foo/bar/gc\\enteryyqAdone\\escqggj@a", "bar bar foo foodone\nbar bar foo foodone");
        clearAllMacros();
        DoTest("foo foo foo foo\nfoo foo foo foo", "qa:s/foo/bar/gc\\enteryyqqAdone\\escggj@aAdone\\esc", "bar bar foo foodone\nbar bar foo foodone");
    }

    clearAllMappings();
    clearAllMacros();
    // Expand mapping in an executed macro, if the invocation of the macro "@a" is a prefix of a mapping M, and
    // M ends up not being triggered.
    vi_global->mappings()->add(Mappings::NormalModeMapping, "@aaaa", "idummy<esc>", Mappings::Recursive);
    vi_global->mappings()->add(Mappings::NormalModeMapping, "S", "ixyz<esc>", Mappings::Recursive);
    DoTest("", "qaSq@abrX", "Xyxyzz");
    clearAllMappings();

    // Can't play old version of macro while recording new version.
    clearAllMacros();
    DoTest("", "qaiaaa\\ctrl-cqqa@aq", "aaa");

    // Can't play the macro while recording it.
    clearAllMacros();
    DoTest("", "qaiaaa\\ctrl-c@aq", "aaa");

    // "@@" plays back macro "a" if "a" was the last macro we played back.
    clearAllMacros();
    DoTest("", "qaia\\ctrl-cq@adiw@@", "a");
    // "@@" plays back macro "b" if "b" was the last macro we played back.
    clearAllMacros();
    DoTest("", "qbib\\ctrl-cq@bdiw@@", "b");
    // "@@" does nothing if no macro was previously played.
    clearAllMacros();
    DoTest("", "qaia\\ctrl-cq@@", "a");
    // Nitpick: "@@" replays the last played back macro, even if that macro had not been defined
    // when it was first played back.
    clearAllMacros();
    DoTest("", "@aqaia\\ctrl-cq@@", "aa");
    // "@@" is counted.
    clearAllMacros();
    DoTest("", "qaia\\ctrl-cq@adiw5@@", "aaaaa");

    // Test that we can save and restore a single macro.
    const QString viTestKConfigFileName = "vimodetest-katevimoderc";
    {
        clearAllMacros();
        KConfig viTestKConfig(viTestKConfigFileName);
        BeginTest("");
        TestPressKey("qaia\\ctrl-cq");
        vi_global->writeConfig(&viTestKConfig);
        viTestKConfig.sync();
        // Overwrite macro "a", and clear the document.
        TestPressKey("qaidummy\\ctrl-cqdd");
        vi_global->readConfig(&viTestKConfig);
        TestPressKey("@a");
        FinishTest("a");
    }

    {
        // Test that we can save and restore several macros.
        clearAllMacros();
        const QString viTestKConfigFileName = "vimodetest-katevimoderc";
        KConfig viTestKConfig(viTestKConfigFileName);
        BeginTest("");
        TestPressKey("qaia\\ctrl-cqqbib\\ctrl-cq");
        vi_global->writeConfig(&viTestKConfig);
        viTestKConfig.sync();
        // Overwrite macros "a" & "b", and clear the document.
        TestPressKey("qaidummy\\ctrl-cqqbidummy\\ctrl-cqdd");
        vi_global->readConfig(&viTestKConfig);
        TestPressKey("@a@b");
        FinishTest("ba");
    }

    // Ensure that we don't crash when a "repeat change" occurs in a macro we execute.
    clearAllMacros();
    DoTest("", "qqixyz\\ctrl-c.q@qdd", "");
    // Don't record both the "." *and* the last-change keypresses when recording a macro;
    // just record the "."
    clearAllMacros();
    DoTest("", "ixyz\\ctrl-cqq.qddi123\\ctrl-c@q", "121233");

    // Test dealing with auto-completion.
    FakeCodeCompletionTestModel *fakeCodeCompletionModel = new FakeCodeCompletionTestModel(kate_view);
    kate_view->registerCompletionModel(fakeCodeCompletionModel);
    // Completion tests require a visible kate_view.
    ensureKateViewVisible();
    // Want Vim mode to intercept ctrl-p, ctrl-n shortcuts, etc.
    const bool oldStealKeys = KateViewConfig::global()->viInputModeStealKeys();
    KateViewConfig::global()->setViInputModeStealKeys(true);

    // Don't invoke completion via ctrl-space when replaying a macro.
    clearAllMacros();
    fakeCodeCompletionModel->setCompletions({ "completionA", "completionB", "completionC" });
    fakeCodeCompletionModel->setFailTestOnInvocation(false);
    BeginTest("");
    TestPressKey("qqico\\ctrl- \\ctrl-cq");
    fakeCodeCompletionModel->setFailTestOnInvocation(true);
    TestPressKey("@q");
    FinishTest("ccoo");

    // Don't invoke completion via ctrl-p when replaying a macro.
    clearAllMacros();
    fakeCodeCompletionModel->setCompletions({ "completionA", "completionB", "completionC" });
    fakeCodeCompletionModel->setFailTestOnInvocation(false);
    BeginTest("");
    TestPressKey("qqico\\ctrl-p\\ctrl-cq");
    fakeCodeCompletionModel->setFailTestOnInvocation(true);
    TestPressKey("@q");
    FinishTest("ccoo");

    // Don't invoke completion via ctrl-n when replaying a macro.
    clearAllMacros();
    fakeCodeCompletionModel->setCompletions({ "completionA", "completionB", "completionC" });
    fakeCodeCompletionModel->setFailTestOnInvocation(false);
    BeginTest("");
    TestPressKey("qqico\\ctrl-n\\ctrl-cq");
    fakeCodeCompletionModel->setFailTestOnInvocation(true);
    TestPressKey("@q");
    FinishTest("ccoo");

    // An "enter" in insert mode when no completion is activated (so, a newline)
    // is treated as a newline when replayed as a macro, even if completion is
    // active when the "enter" is replayed.
    clearAllMacros();
    fakeCodeCompletionModel->setCompletions(QStringList()); // Prevent any completions.
    fakeCodeCompletionModel->setFailTestOnInvocation(false);
    fakeCodeCompletionModel->clearWasInvoked();
    BeginTest("");
    TestPressKey("qqicompl\\enterX\\ctrl-cqdddd");
    QVERIFY(!fakeCodeCompletionModel->wasInvoked()); // Error in test setup!
    fakeCodeCompletionModel->setCompletions({ "completionA", "completionB", "completionC" });
    fakeCodeCompletionModel->forceInvocationIfDocTextIs("compl");
    fakeCodeCompletionModel->clearWasInvoked();
    TestPressKey("@q");
    QVERIFY(fakeCodeCompletionModel->wasInvoked()); // Error in test setup!
    fakeCodeCompletionModel->doNotForceInvocation();
    FinishTest("compl\nX");
    // Same for "return".
    clearAllMacros();
    fakeCodeCompletionModel->setCompletions(QStringList()); // Prevent any completions.
    fakeCodeCompletionModel->setFailTestOnInvocation(false);
    fakeCodeCompletionModel->clearWasInvoked();
    BeginTest("");
    TestPressKey("qqicompl\\returnX\\ctrl-cqdddd");
    QVERIFY(!fakeCodeCompletionModel->wasInvoked()); // Error in test setup!
    fakeCodeCompletionModel->setCompletions({ "completionA", "completionB", "completionC" });
    fakeCodeCompletionModel->forceInvocationIfDocTextIs("compl");
    fakeCodeCompletionModel->clearWasInvoked();
    TestPressKey("@q");
    QVERIFY(fakeCodeCompletionModel->wasInvoked()); // Error in test setup!
    fakeCodeCompletionModel->doNotForceInvocation();
    FinishTest("compl\nX");

    // If we do a plain-text completion in a macro, this should be repeated when we replay it.
    clearAllMacros();
    BeginTest("");
    fakeCodeCompletionModel->setCompletions({ "completionA", "completionB", "completionC" });
    fakeCodeCompletionModel->setFailTestOnInvocation(false);
    TestPressKey("qqicompl\\ctrl- \\enter\\ctrl-cq");
    kate_document->clear();
    fakeCodeCompletionModel->setFailTestOnInvocation(true);
    TestPressKey("@q");
    FinishTest("completionA");

    // Should replace only the current word when we repeat the completion.
    clearAllMacros();
    BeginTest("compl");
    fakeCodeCompletionModel->setCompletions({ "completionA", "completionB", "completionC" });
    fakeCodeCompletionModel->setFailTestOnInvocation(false);
    TestPressKey("qqfla\\ctrl- \\enter\\ctrl-cq");
    fakeCodeCompletionModel->setFailTestOnInvocation(true);
    kate_document->setText("(compl)");
    TestPressKey("gg@q");
    FinishTest("(completionA)");

    // Tail-clearing completions should be undoable with one undo.
    clearAllMacros();
    BeginTest("compl");
    fakeCodeCompletionModel->setCompletions({ "completionA", "completionB", "completionC" });
    fakeCodeCompletionModel->setFailTestOnInvocation(false);
    TestPressKey("qqfla\\ctrl- \\enter\\ctrl-cq");
    fakeCodeCompletionModel->setFailTestOnInvocation(true);
    kate_document->setText("(compl)");
    TestPressKey("gg@qu");
    FinishTest("(compl)");

    // Should be able to store multiple completions.
    clearAllMacros();
    BeginTest("");
    fakeCodeCompletionModel->setCompletions({ "completionA", "completionB", "completionC" });
    fakeCodeCompletionModel->setFailTestOnInvocation(false);
    TestPressKey("qqicom\\ctrl-p\\enter com\\ctrl-p\\ctrl-p\\enter\\ctrl-cq");
    fakeCodeCompletionModel->setFailTestOnInvocation(true);
    TestPressKey("dd@q");
    FinishTest("completionC completionB");

    // Clear the completions for a macro when we start recording.
    clearAllMacros();
    BeginTest("");
    fakeCodeCompletionModel->setCompletions({ "completionOrig" });
    fakeCodeCompletionModel->setFailTestOnInvocation(false);
    TestPressKey("qqicom\\ctrl- \\enter\\ctrl-cq");
    fakeCodeCompletionModel->setCompletions({ "completionSecond" });
    TestPressKey("ddqqicom\\ctrl- \\enter\\ctrl-cq");
    fakeCodeCompletionModel->setFailTestOnInvocation(true);
    TestPressKey("dd@q");
    FinishTest("completionSecond");

    // Completions are per macro.
    clearAllMacros();
    BeginTest("");
    fakeCodeCompletionModel->setCompletions({ "completionA" });
    fakeCodeCompletionModel->setFailTestOnInvocation(false);
    TestPressKey("qaicom\\ctrl- \\enter\\ctrl-cq");
    fakeCodeCompletionModel->setCompletions({ "completionB" });
    TestPressKey("ddqbicom\\ctrl- \\enter\\ctrl-cq");
    fakeCodeCompletionModel->setFailTestOnInvocation(true);
    TestPressKey("dd@aA\\enter\\ctrl-c@b");
    FinishTest("completionA\ncompletionB");

    // Make sure completions work with recursive macros.
    clearAllMacros();
    BeginTest("");
    fakeCodeCompletionModel->setCompletions({ "completionA1", "completionA2" });
    fakeCodeCompletionModel->setFailTestOnInvocation(false);
    // Record 'a', which calls the (non-yet-existent) macro 'b'.
    TestPressKey("qaicom\\ctrl- \\enter\\ctrl-cA\\enter\\ctrl-c@bA\\enter\\ctrl-cicom\\ctrl- \\ctrl-p\\enter\\ctrl-cq");
    // Clear document and record 'b'.
    fakeCodeCompletionModel->setCompletions({ "completionB" });
    TestPressKey("ggdGqbicom\\ctrl- \\enter\\ctrl-cq");
    TestPressKey("dd@a");
    FinishTest("completionA1\ncompletionB\ncompletionA2");

    // Test that non-tail-removing completions are respected.
    // Note that there is no way (in general) to determine if a completion was
    // non-tail-removing, so we explicitly set the config to false.
    const bool oldRemoveTailOnCompletion = KateViewConfig::global()->wordCompletionRemoveTail();
    KateViewConfig::global()->setWordCompletionRemoveTail(false);
    const bool oldReplaceTabsDyn = kate_document->config()->replaceTabsDyn();
    kate_document->config()->setReplaceTabsDyn(false);
    fakeCodeCompletionModel->setRemoveTailOnComplete(false);
    clearAllMacros();
    BeginTest("compTail");
    fakeCodeCompletionModel->setCompletions({ "completionA", "completionB", "completionC" });
    fakeCodeCompletionModel->setFailTestOnInvocation(false);
    TestPressKey("qqfTi\\ctrl- \\enter\\ctrl-cq");
    fakeCodeCompletionModel->setFailTestOnInvocation(true);
    kate_document->setText("compTail");
    TestPressKey("gg@q");
    FinishTest("completionATail");

    // A "word" consists of letters & numbers, plus "_".
    clearAllMacros();
    BeginTest("(123_compTail");
    fakeCodeCompletionModel->setCompletions({ "123_completionA" });
    fakeCodeCompletionModel->setFailTestOnInvocation(false);
    TestPressKey("qqfTi\\ctrl- \\enter\\ctrl-cq");
    fakeCodeCompletionModel->setFailTestOnInvocation(true);
    kate_document->setText("(123_compTail");
    TestPressKey("gg@q");
    FinishTest("(123_completionATail");

    // Correctly remove word if we are set to remove tail.
    KateViewConfig::global()->setWordCompletionRemoveTail(true);
    clearAllMacros();
    BeginTest("(123_compTail)");
    fakeCodeCompletionModel->setCompletions({ "123_completionA" });
    fakeCodeCompletionModel->setFailTestOnInvocation(false);
    fakeCodeCompletionModel->setRemoveTailOnComplete(true);
    TestPressKey("qqfTi\\ctrl- \\enter\\ctrl-cq");
    fakeCodeCompletionModel->setFailTestOnInvocation(true);
    kate_document->setText("(123_compTail)");
    TestPressKey("gg@q");
    FinishTest("(123_completionA)");

    // Again, a "word" consists of letters & numbers & underscores.
    clearAllMacros();
    BeginTest("(123_compTail_456)");
    fakeCodeCompletionModel->setCompletions({ "123_completionA" });
    fakeCodeCompletionModel->setFailTestOnInvocation(false);
    fakeCodeCompletionModel->setRemoveTailOnComplete(true);
    TestPressKey("qqfTi\\ctrl- \\enter\\ctrl-cq");
    fakeCodeCompletionModel->setFailTestOnInvocation(true);
    kate_document->setText("(123_compTail_456)");
    TestPressKey("gg@q");
    FinishTest("(123_completionA)");

    // Actually, let whether the tail is swallowed or not depend on the value when the
    // completion occurred, not when we replay it.
    clearAllMacros();
    BeginTest("(123_compTail_456)");
    fakeCodeCompletionModel->setCompletions({ "123_completionA" });
    fakeCodeCompletionModel->setFailTestOnInvocation(false);
    fakeCodeCompletionModel->setRemoveTailOnComplete(true);
    KateViewConfig::global()->setWordCompletionRemoveTail(true);
    TestPressKey("qqfTi\\ctrl- \\enter\\ctrl-cq");
    fakeCodeCompletionModel->setFailTestOnInvocation(true);
    KateViewConfig::global()->setWordCompletionRemoveTail(false);
    kate_document->setText("(123_compTail_456)");
    TestPressKey("gg@q");
    FinishTest("(123_completionA)");
    clearAllMacros();
    BeginTest("(123_compTail_456)");
    fakeCodeCompletionModel->setCompletions({ "123_completionA" });
    fakeCodeCompletionModel->setFailTestOnInvocation(false);
    fakeCodeCompletionModel->setRemoveTailOnComplete(false);
    KateViewConfig::global()->setWordCompletionRemoveTail(false);
    TestPressKey("qqfTi\\ctrl- \\enter\\ctrl-cq");
    fakeCodeCompletionModel->setFailTestOnInvocation(true);
    KateViewConfig::global()->setWordCompletionRemoveTail(true);
    kate_document->setText("(123_compTail_456)");
    TestPressKey("gg@q");
    FinishTest("(123_completionATail_456)");

    // Can have remove-tail *and* non-remove-tail completions in one macro.
    clearAllMacros();
    BeginTest("(123_compTail_456)\n(123_compTail_456)");
    fakeCodeCompletionModel->setCompletions({ "123_completionA" });
    fakeCodeCompletionModel->setFailTestOnInvocation(false);
    fakeCodeCompletionModel->setRemoveTailOnComplete(true);
    KateViewConfig::global()->setWordCompletionRemoveTail(true);
    TestPressKey("qqfTi\\ctrl- \\enter\\ctrl-c");
    fakeCodeCompletionModel->setRemoveTailOnComplete(false);
    KateViewConfig::global()->setWordCompletionRemoveTail(false);
    TestPressKey("j^fTi\\ctrl- \\enter\\ctrl-cq");
    kate_document->setText("(123_compTail_456)\n(123_compTail_456)");
    TestPressKey("gg@q");
    FinishTest("(123_completionA)\n(123_completionATail_456)");

    // Can repeat plain-text completions when there is no word to the left of the cursor.
    clearAllMacros();
    BeginTest("");
    fakeCodeCompletionModel->setCompletions({ "123_completionA" });
    fakeCodeCompletionModel->setFailTestOnInvocation(false);
    TestPressKey("qqi\\ctrl- \\enter\\ctrl-cq");
    kate_document->clear();
    TestPressKey("gg@q");
    FinishTest("123_completionA");

    // Shouldn't swallow the letter under the cursor if we're not swallowing tails.
    clearAllMacros();
    BeginTest("");
    fakeCodeCompletionModel->setCompletions({ "123_completionA" });
    fakeCodeCompletionModel->setFailTestOnInvocation(false);
    fakeCodeCompletionModel->setRemoveTailOnComplete(false);
    KateViewConfig::global()->setWordCompletionRemoveTail(false);
    TestPressKey("qqi\\ctrl- \\enter\\ctrl-cq");
    kate_document->setText("oldwordshouldbeuntouched");
    fakeCodeCompletionModel->setFailTestOnInvocation(true);
    TestPressKey("gg@q");
    FinishTest("123_completionAoldwordshouldbeuntouched");

    // ... but do if we are swallowing tails.
    clearAllMacros();
    BeginTest("");
    fakeCodeCompletionModel->setCompletions({ "123_completionA" });
    fakeCodeCompletionModel->setFailTestOnInvocation(false);
    fakeCodeCompletionModel->setRemoveTailOnComplete(true);
    KateViewConfig::global()->setWordCompletionRemoveTail(true);
    TestPressKey("qqi\\ctrl- \\enter\\ctrl-cq");
    kate_document->setText("oldwordshouldbedeleted");
    fakeCodeCompletionModel->setFailTestOnInvocation(true);
    TestPressKey("gg@q");
    FinishTest("123_completionA");

    // Completion of functions.
    // Currently, not removing the tail on function completion is not supported.
    fakeCodeCompletionModel->setRemoveTailOnComplete(true);
    KateViewConfig::global()->setWordCompletionRemoveTail(true);
    // A completed, no argument function "function()" is repeated correctly.
    BeginTest("");
    fakeCodeCompletionModel->setCompletions({ "function()" });
    fakeCodeCompletionModel->setFailTestOnInvocation(false);
    TestPressKey("qqifunc\\ctrl- \\enter\\ctrl-cq");
    fakeCodeCompletionModel->setFailTestOnInvocation(true);
    TestPressKey("dd@q");
    FinishTest("function()");

    // Cursor is placed after the closing bracket when completion a no-arg function.
    BeginTest("");
    fakeCodeCompletionModel->setCompletions({ "function()" });
    fakeCodeCompletionModel->setFailTestOnInvocation(false);
    TestPressKey("qqifunc\\ctrl- \\enter.something();\\ctrl-cq");
    fakeCodeCompletionModel->setFailTestOnInvocation(true);
    TestPressKey("dd@q");
    FinishTest("function().something();");

    // A function taking some arguments, repeated where there is no opening bracket to
    // merge with, is repeated as "function()").
    BeginTest("");
    fakeCodeCompletionModel->setCompletions({ "function(...)" });
    fakeCodeCompletionModel->setFailTestOnInvocation(false);
    TestPressKey("qqifunc\\ctrl- \\enter\\ctrl-cq");
    fakeCodeCompletionModel->setFailTestOnInvocation(true);
    TestPressKey("dd@q");
    FinishTest("function()");

    // A function taking some arguments, repeated where there is no opening bracket to
    // merge with, places the cursor after the opening bracket.
    BeginTest("");
    fakeCodeCompletionModel->setCompletions({ "function(...)" });
    fakeCodeCompletionModel->setFailTestOnInvocation(false);
    TestPressKey("qqifunc\\ctrl- \\enterfirstArg\\ctrl-cq");
    fakeCodeCompletionModel->setFailTestOnInvocation(true);
    TestPressKey("dd@q");
    FinishTest("function(firstArg)");

    // A function taking some arguments, recorded where there was an opening bracket to merge
    // with but repeated where there is no such bracket, is repeated as "function()" and the
    // cursor placed appropriately.
    BeginTest("(<-Mergeable opening bracket)");
    fakeCodeCompletionModel->setCompletions({ "function(...)" });
    fakeCodeCompletionModel->setFailTestOnInvocation(false);
    TestPressKey("qqifunc\\ctrl- \\enterfirstArg\\ctrl-cq");
    fakeCodeCompletionModel->setFailTestOnInvocation(true);
    TestPressKey("dd@q");
    FinishTest("function(firstArg)");

    // A function taking some arguments, recorded where there was no opening bracket to merge
    // with but repeated where there is such a bracket, is repeated as "function" and the
    // cursor moved to after the merged opening bracket.
    BeginTest("");
    fakeCodeCompletionModel->setCompletions({ "function(...)" });
    fakeCodeCompletionModel->setFailTestOnInvocation(false);
    TestPressKey("qqifunc\\ctrl- \\enterfirstArg\\ctrl-cq");
    fakeCodeCompletionModel->setFailTestOnInvocation(true);
    kate_document->setText("(<-firstArg goes here)");
    TestPressKey("gg@q");
    FinishTest("function(firstArg<-firstArg goes here)");

    // A function taking some arguments, recorded where there was an opening bracket to merge
    // with and repeated where there is also such a bracket, is repeated as "function" and the
    // cursor moved to after the merged opening bracket.
    BeginTest("(<-mergeablebracket)");
    fakeCodeCompletionModel->setCompletions({ "function(...)" });
    fakeCodeCompletionModel->setFailTestOnInvocation(false);
    TestPressKey("qqifunc\\ctrl- \\enterfirstArg\\ctrl-cq");
    fakeCodeCompletionModel->setFailTestOnInvocation(true);
    kate_document->setText("(<-firstArg goes here)");
    TestPressKey("gg@q");
    FinishTest("function(firstArg<-firstArg goes here)");

    // The mergeable bracket can be separated by whitespace; the cursor is still placed after the
    // opening bracket.
    BeginTest("");
    fakeCodeCompletionModel->setCompletions({ "function(...)" });
    fakeCodeCompletionModel->setFailTestOnInvocation(false);
    TestPressKey("qqifunc\\ctrl- \\enterfirstArg\\ctrl-cq");
    fakeCodeCompletionModel->setFailTestOnInvocation(true);
    kate_document->setText("   \t (<-firstArg goes here)");
    TestPressKey("gg@q");
    FinishTest("function   \t (firstArg<-firstArg goes here)");

    // Whitespace only, though!
    BeginTest("");
    fakeCodeCompletionModel->setCompletions({ "function(...)" });
    fakeCodeCompletionModel->setFailTestOnInvocation(false);
    TestPressKey("qqifunc\\ctrl- \\enterfirstArg\\ctrl-cq");
    fakeCodeCompletionModel->setFailTestOnInvocation(true);
    kate_document->setText("|   \t ()");
    TestPressKey("gg@q");
    FinishTest("function(firstArg)|   \t ()");

    // The opening bracket can actually be after the current word (with optional whitespace).
    // Note that this wouldn't be the case if we weren't swallowing tails when completion functions,
    // but this is not currently supported.
    BeginTest("function");
    fakeCodeCompletionModel->setCompletions({ "function(...)" });
    fakeCodeCompletionModel->setFailTestOnInvocation(false);
    TestPressKey("qqfta\\ctrl- \\enterfirstArg\\ctrl-cq");
    fakeCodeCompletionModel->setFailTestOnInvocation(true);
    kate_document->setText("functxyz    (<-firstArg goes here)");
    TestPressKey("gg@q");
    FinishTest("function    (firstArg<-firstArg goes here)");

    // Regression test for weird issue with replaying completions when the character to the left of the cursor
    // is not a word char.
    BeginTest("");
    fakeCodeCompletionModel->setCompletions({ "completionA" });
    fakeCodeCompletionModel->setFailTestOnInvocation(false);
    TestPressKey("qqciw\\ctrl- \\enter\\ctrl-cq");
    TestPressKey("ddi.xyz\\enter123\\enter456\\ctrl-cggl"); // Position cursor just after the "."
    TestPressKey("@q");
    FinishTest(".completionA\n123\n456");
    BeginTest("");
    fakeCodeCompletionModel->setCompletions({ "completionA" });
    fakeCodeCompletionModel->setFailTestOnInvocation(false);
    TestPressKey("qqciw\\ctrl- \\enter\\ctrl-cq");
    TestPressKey("ddi.xyz.abc\\enter123\\enter456\\ctrl-cggl"); // Position cursor just after the "."
    TestPressKey("@q");
    FinishTest(".completionA.abc\n123\n456");

    // Functions taking no arguments are never bracket-merged.
    BeginTest("");
    fakeCodeCompletionModel->setCompletions({ "function()" });
    fakeCodeCompletionModel->setFailTestOnInvocation(false);
    TestPressKey("qqifunc\\ctrl- \\enter.something();\\ctrl-cq");
    fakeCodeCompletionModel->setFailTestOnInvocation(true);
    kate_document->setText("(<-don't merge this bracket)");
    TestPressKey("gg@q");
    FinishTest("function().something();(<-don't merge this bracket)");

    // Not-removing-tail when completing functions is not currently supported,
    // so ignore the "do-not-remove-tail" settings when we try this.
    BeginTest("funct");
    fakeCodeCompletionModel->setCompletions({ "function(...)" });
    fakeCodeCompletionModel->setFailTestOnInvocation(false);
    KateViewConfig::global()->setWordCompletionRemoveTail(false);
    TestPressKey("qqfta\\ctrl- \\enterfirstArg\\ctrl-cq");
    kate_document->setText("functxyz");
    fakeCodeCompletionModel->setFailTestOnInvocation(true);
    TestPressKey("gg@q");
    FinishTest("function(firstArg)");
    BeginTest("funct");
    fakeCodeCompletionModel->setCompletions({ "function()" });
    fakeCodeCompletionModel->setFailTestOnInvocation(false);
    KateViewConfig::global()->setWordCompletionRemoveTail(false);
    TestPressKey("qqfta\\ctrl- \\enter\\ctrl-cq");
    kate_document->setText("functxyz");
    fakeCodeCompletionModel->setFailTestOnInvocation(true);
    TestPressKey("gg@q");
    FinishTest("function()");
    KateViewConfig::global()->setWordCompletionRemoveTail(true);

    // Deal with cases where completion ends with ";".
    BeginTest("");
    fakeCodeCompletionModel->setCompletions({ "function();" });
    fakeCodeCompletionModel->setFailTestOnInvocation(false);
    TestPressKey("qqifun\\ctrl- \\enter\\ctrl-cq");
    kate_document->clear();
    fakeCodeCompletionModel->setFailTestOnInvocation(true);
    TestPressKey("gg@q");
    FinishTest("function();");
    BeginTest("");
    fakeCodeCompletionModel->setCompletions({ "function();" });
    fakeCodeCompletionModel->setFailTestOnInvocation(false);
    TestPressKey("qqifun\\ctrl- \\enterX\\ctrl-cq");
    kate_document->clear();
    fakeCodeCompletionModel->setFailTestOnInvocation(true);
    TestPressKey("gg@q");
    FinishTest("function();X");
    BeginTest("");
    fakeCodeCompletionModel->setCompletions({ "function(...);" });
    fakeCodeCompletionModel->setFailTestOnInvocation(false);
    TestPressKey("qqifun\\ctrl- \\enter\\ctrl-cq");
    kate_document->clear();
    fakeCodeCompletionModel->setFailTestOnInvocation(true);
    TestPressKey("gg@q");
    FinishTest("function();");
    BeginTest("");
    fakeCodeCompletionModel->setCompletions({ "function(...);" });
    fakeCodeCompletionModel->setFailTestOnInvocation(false);
    TestPressKey("qqifun\\ctrl- \\enterX\\ctrl-cq");
    kate_document->clear();
    fakeCodeCompletionModel->setFailTestOnInvocation(true);
    TestPressKey("gg@q");
    FinishTest("function(X);");
    // Tests for completions ending in ";" where bracket merging should happen on replay.
    // NB: bracket merging when recording is impossible with completions that end in ";".
    BeginTest("");
    fakeCodeCompletionModel->setCompletions({ "function(...);" });
    fakeCodeCompletionModel->setFailTestOnInvocation(false);
    TestPressKey("qqifun\\ctrl- \\enter\\ctrl-cq");
    kate_document->setText("(<-mergeable bracket");
    fakeCodeCompletionModel->setFailTestOnInvocation(true);
    TestPressKey("gg@q");
    FinishTest("function(<-mergeable bracket");
    BeginTest("");
    fakeCodeCompletionModel->setCompletions({ "function(...);" });
    fakeCodeCompletionModel->setFailTestOnInvocation(false);
    TestPressKey("qqifun\\ctrl- \\enterX\\ctrl-cq");
    kate_document->setText("(<-mergeable bracket");
    fakeCodeCompletionModel->setFailTestOnInvocation(true);
    TestPressKey("gg@q");
    FinishTest("function(X<-mergeable bracket");
    // Don't merge no arg functions.
    BeginTest("");
    fakeCodeCompletionModel->setCompletions({ "function();" });
    fakeCodeCompletionModel->setFailTestOnInvocation(false);
    TestPressKey("qqifun\\ctrl- \\enterX\\ctrl-cq");
    kate_document->setText("(<-mergeable bracket");
    fakeCodeCompletionModel->setFailTestOnInvocation(true);
    TestPressKey("gg@q");
    FinishTest("function();X(<-mergeable bracket");

    {
        const QString viTestKConfigFileName = "vimodetest-katevimoderc";
        KConfig viTestKConfig(viTestKConfigFileName);
        // Test loading and saving of macro completions.
        clearAllMacros();
        BeginTest("funct\nnoa\ncomtail\ncomtail\ncom");
        fakeCodeCompletionModel->setCompletions({ "completionA", "functionwithargs(...)", "noargfunction()" });
        fakeCodeCompletionModel->setFailTestOnInvocation(false);
        // Record 'a'.
        TestPressKey("qafta\\ctrl- \\enterfirstArg\\ctrl-c"); // Function with args.
        TestPressKey("\\enterea\\ctrl- \\enter\\ctrl-c");     // Function no args.
        fakeCodeCompletionModel->setRemoveTailOnComplete(true);
        KateViewConfig::global()->setWordCompletionRemoveTail(true);
        TestPressKey("\\enterfti\\ctrl- \\enter\\ctrl-c");   // Cut off tail.
        fakeCodeCompletionModel->setRemoveTailOnComplete(false);
        KateViewConfig::global()->setWordCompletionRemoveTail(false);
        TestPressKey("\\enterfti\\ctrl- \\enter\\ctrl-cq");   // Don't cut off tail.
        fakeCodeCompletionModel->setRemoveTailOnComplete(true);
        KateViewConfig::global()->setWordCompletionRemoveTail(true);
        // Record 'b'.
        fakeCodeCompletionModel->setCompletions({ "completionB", "semicolonfunctionnoargs();", "semicolonfunctionwithargs(...);" });
        TestPressKey("\\enterqbea\\ctrl- \\enter\\ctrl-cosemicolonfunctionw\\ctrl- \\enterX\\ctrl-cosemicolonfunctionn\\ctrl- \\enterX\\ctrl-cq");
        // Save.
        vi_global->writeConfig(&viTestKConfig);
        viTestKConfig.sync();
        // Overwrite 'a' and 'b' and their completions.
        fakeCodeCompletionModel->setCompletions({ "blah1" });
        kate_document->setText("");
        TestPressKey("ggqaiblah\\ctrl- \\enter\\ctrl-cq");
        TestPressKey("ddqbiblah\\ctrl- \\enter\\ctrl-cq");
        // Reload.
        vi_global->readConfig(&viTestKConfig);
        // Replay reloaded.
        fakeCodeCompletionModel->setFailTestOnInvocation(true);
        kate_document->setText("funct\nnoa\ncomtail\ncomtail\ncom");
        TestPressKey("gg@a\\enter@b");
        FinishTest("functionwithargs(firstArg)\nnoargfunction()\ncompletionA\ncompletionAtail\ncompletionB\nsemicolonfunctionwithargs(X);\nsemicolonfunctionnoargs();X");
    }

    // Check that undo/redo operations work properly with macros.
    {
        clearAllMacros();
        BeginTest("");
        TestPressKey("ihello\\ctrl-cqauq");
        TestPressKey("@a\\enter");
        FinishTest("");
    }
    {
        clearAllMacros();
        BeginTest("");
        TestPressKey("ihello\\ctrl-cui.bye\\ctrl-cu");
        TestPressKey("qa\\ctrl-r\\enterq");
        TestPressKey("@a\\enter");
        FinishTest(".bye");
    }

    // When replaying a last change in the process of replaying a macro, take the next completion
    // event from the last change completions log, rather than the macro completions log.
    // Ensure that the last change completions log is kept up to date even while we're replaying the macro.
    if (false) { // FIXME: test currently fails in newer Qt >= 5.11, but works with Qt 5.10
    clearAllMacros();
    BeginTest("");
    fakeCodeCompletionModel->setCompletions({ "completionMacro", "completionRepeatLastChange" });
    fakeCodeCompletionModel->setFailTestOnInvocation(false);
    TestPressKey("qqicompletionM\\ctrl- \\enter\\ctrl-c");
    TestPressKey("a completionRep\\ctrl- \\enter\\ctrl-c");
    TestPressKey(".q");
    qDebug() << "text: " << kate_document->text();
    kate_document->clear();
    TestPressKey("gg@q");
    FinishTest("completionMacro completionRepeatLastChange completionRepeatLastChange");
    }

    KateViewConfig::global()->setWordCompletionRemoveTail(oldRemoveTailOnCompletion);
    kate_document->config()->setReplaceTabsDyn(oldReplaceTabsDyn);

    kate_view->unregisterCompletionModel(fakeCodeCompletionModel);
    delete fakeCodeCompletionModel;
    fakeCodeCompletionModel = nullptr;
    // Hide the kate_view for subsequent tests.
    kate_view->hide();
    mainWindow->hide();
    KateViewConfig::global()->setViInputModeStealKeys(oldStealKeys);
}

void KeysTest::MarkTests()
{
    // Difference between ` and '
    DoTest("  a\n    b", "jmak'aii", "  a\n    ib");
    DoTest("  a\n    b", "jmak`aii", "  a\ni    b");

    // Last edit markers.
    DoTest("foo", "ean\\escgg`.r.", "foo.");
    DoTest("foo", "ean\\escgg`[r[", "foo[");
    DoTest("foo", "ean\\escgg`]r]", "foo]");
    DoTest("foo bar", "ean\\escgg`]r]", "foon]bar");
    DoTest("", "ibar\\escgg`.r.", "ba.");
    DoTest("", "ibar\\escgggUiw`.r.", ".AR");
    DoTest("", "2ibar\\escgg`]r]", "barba]");
    DoTest("", "2ibar\\escgg`[r[", "[arbar");
    DoTest("", "3ibar\\escgg`.r.", "barbar.ar"); // Vim is weird.
    DoTest("", "abar\\esc.gg`]r]", "barba]");
    DoTest("foo bar", "wgUiwgg`]r]", "foo BA]");
    DoTest("foo bar", "wgUiwgg`.r.", "foo .AR");
    DoTest("foo bar", "gUiwgg`]r.", "FO. bar");
    DoTest("foo bar", "wdiwgg`[r[", "foo[");
    DoTest("foo bar", "wdiwgg`]r]", "foo]");
    DoTest("foo bar", "wdiwgg`.r.", "foo.");
    DoTest("foo bar", "wciwnose\\escgg`.r.", "foo nos.");
    DoTest("foo bar", "wciwnose\\escgg`[r[", "foo [ose");
    DoTest("foo bar", "wciwnose\\escgg`]r]", "foo nos]");
    DoTest("foo", "~ibar\\escgg`[r[", "F[aroo");
    DoTest("foo bar", "lragg`.r.", "f.o bar");
    DoTest("foo bar", "lragg`[r[", "f[o bar");
    DoTest("foo bar", "lragg`]r]", "f]o bar");
    DoTest("", "ifoo\\ctrl-hbar\\esc`[r[", "[obar");
    DoTest("", "ifoo\\ctrl-wbar\\esc`[r[", "[ar");
    DoTest("", "if\\ctrl-hbar\\esc`[r[", "[ar");
    DoTest("", "5ofoo\\escgg`[r[", "\n[oo\nfoo\nfoo\nfoo\nfoo");
    DoTest("", "5ofoo\\escgg`]r]", "\nfoo\nfoo\nfoo\nfoo\nfo]");
    DoTest("", "5ofoo\\escgg`.r.", "\nfoo\nfoo\nfoo\nfoo\n.oo");
    DoTest("foo", "yyp`[r[", "foo\n[oo");
    DoTest("xyz\nfoo", "ja\\returnbar\\esc`[r[", "xyz\n[\nbaroo");
    DoTest("foo", "lrayypgg`[r[", "fao\n[ao");
    DoTest("foo", "l~u`[r[", "[oo");
    DoTest("foo", "l~u`.r.", ".oo");
    DoTest("foo", "l~u`]r]", "]oo");
    DoTest("foo", "lia\\escu`[r[", "[oo");
    DoTest("foo", "lia\\escu`.r.", ".oo");
    DoTest("foo", "lia\\escu`]r]", "]oo");
    DoTest("foo", "l~u~`[r[", "f[o");
    DoTest("foo\nbar\nxyz", "jyypu`[r[", "foo\nbar\n[yz");
    DoTest("foo\nbar\nxyz", "jyypu`.r.", "foo\nbar\n.yz");
    DoTest("foo\nbar\nxyz", "jyypu`]r]", "foo\nbar\n]yz");
    DoTest("foo\nbar\nxyz\n123", "jdju`[r[", "foo\n[ar\nxyz\n123");
    DoTest("foo\nbar\nxyz\n123", "jdju`.r.", "foo\n.ar\nxyz\n123");
    DoTest("foo\nbar\nxyz\n123", "jdju`]r]", "foo\nbar\n]yz\n123");
    DoTest("foo\nbar\nxyz\n123", "jVj~u\\esc`[r[", "foo\n[ar\nxyz\n123", ShouldFail, "Vim is weird.");
}

//END: KeysTest
