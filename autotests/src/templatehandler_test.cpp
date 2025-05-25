/*
    This file is part of the KDE libraries
    SPDX-FileCopyrightText: 2010 Bernhard Beschow <bbeschow@cs.tu-berlin.de>
    SPDX-FileCopyrightText: 2025 Mirian Margiani <mixosaurus+kde@pm.me>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "templatehandler_test.h"

#include <KTextEditor/Cursor>
#include <kateconfig.h>
#include <katedocument.h>
#include <katetemplatehandler.h>
#include <kateview.h>

#include <QStandardPaths>
#include <QString>
#include <QTest>
#include <QTestKeyEvent>

QTEST_MAIN(TemplateHandlerTest)

using namespace KTextEditor;

TemplateHandlerTest::TemplateHandlerTest()
    : QObject()
{
    QStandardPaths::setTestModeEnabled(true);
}

void TemplateHandlerTest::testUndo()
{
    const QString snippet = QStringLiteral(
        "for (${type=\"int\"} ${index=\"i\"} = ; ${index} < ; ++${index})\n"
        "{\n"
        "    ${index}\n"
        "}");

    auto doc = new KTextEditor::DocumentPrivate();
    auto view = static_cast<KTextEditor::ViewPrivate *>(doc->createView(nullptr));

    // fixed indentation options
    doc->config()->setTabWidth(8);
    doc->config()->setIndentationWidth(4);
    doc->config()->setReplaceTabsDyn(true);

    view->insertTemplate(KTextEditor::Cursor(0, 0), snippet);

    const QString result = QStringLiteral(
        "for (int i = ; i < ; ++i)\n"
        "{\n"
        "    i\n"
        "}");
    QCOMPARE(doc->text(), result);

    doc->replaceText(Range(0, 9, 0, 10), QStringLiteral("j"));

    const QString result2 = QStringLiteral(
        "for (int j = ; j < ; ++j)\n"
        "{\n"
        "    j\n"
        "}");
    QCOMPARE(doc->text(), result2);

    doc->undo();

    QCOMPARE(doc->text(), result);

    doc->redo();

    QCOMPARE(doc->text(), result2);

    doc->insertText(Cursor(0, 10), QStringLiteral("j"));
    doc->insertText(Cursor(0, 11), QStringLiteral("j"));

    const QString result3 = QStringLiteral(
        "for (int jjj = ; jjj < ; ++jjj)\n"
        "{\n"
        "    jjj\n"
        "}");
    QCOMPARE(doc->text(), result3);

    doc->undo();

    QCOMPARE(doc->text(), result);

    doc->redo();

    QCOMPARE(doc->text(), result3);

    doc->undo();
    QCOMPARE(doc->text(), result);

    doc->undo();
    QCOMPARE(doc->text(), QString());
}

void TemplateHandlerTest::testEscapes()
{
    QFETCH(QString, input);

    auto doc = new KTextEditor::DocumentPrivate();
    auto view = static_cast<KTextEditor::ViewPrivate *>(doc->createView(nullptr));

    view->insertTemplate({0, 0}, input);
    QTEST(doc->text(), "expectedOutput");
}

void TemplateHandlerTest::testEscapes_data()
{
    QTest::addColumn<QString>("input");
    QTest::addColumn<QString>("expectedOutput");

    QTest::newRow("start") << R"(\${foo})" << R"(${foo})";
    QTest::newRow("middle") << R"(foo \${field} foo)" << R"(foo ${field} foo)";
    QTest::newRow("end") << R"(foo \${field})" << R"(foo ${field})";
    QTest::newRow("mixed") << R"(\${field} ${bar} \${foo})" << R"(${field} bar ${foo})";
    QTest::newRow("defaults") << R"(${bar=1} \${foo=3})" << R"(1 ${foo=3})";
    QTest::newRow("double") << R"(\\\${baz=7})" << R"(\${baz=7})";
    QTest::newRow("odd") << R"(\\\\\\\\\${baz=7})" << R"(\\\\${baz=7})";
    QTest::newRow("even") << R"(\\\\\\\\${baz=7})" << R"(\\\\7)";
    QTest::newRow("many_slashes") << R"(\\\\\\\${foo=1} \\\\${bar=2})" << R"(\\\${foo=1} \\2)";
    QTest::newRow("free_slashes1") << R"(\\\ \\${foo=1})" << R"(\\\ \1)";
    QTest::newRow("free_slashes2") << R"(\\\ ${foo} \\\\)" << R"(\\\ foo \\\\)";
    QTest::newRow("multiple_fields") << R"(\${field} ${bar} \\\${foo=3} \\${baz=7})" << R"(${field} bar \${foo=3} \7)";
    QTest::newRow("nested") << R"(\${${field}=x})" << R"(${field=x})";
    QTest::newRow("nested_default") << R"(\${${field=1}=x})" << R"(${1=x})";
    QTest::newRow("adjacent") << R"(${f1}\${f2}\\\${f3}${f4})" << R"(f1${f2}\${f3}f4)";
}

void TemplateHandlerTest::testSimpleMirror()
{
    QFETCH(QString, text);

    auto doc = new KTextEditor::DocumentPrivate();
    auto view = static_cast<KTextEditor::ViewPrivate *>(doc->createView(nullptr));
    view->insertTemplate({0, 0}, text);

    QCOMPARE(doc->text(), QString(text).replace(QStringLiteral("${foo}"), QStringLiteral("foo")));

    doc->insertText({0, 0}, QStringLiteral("xx"));
    QCOMPARE(doc->text(), QString(text).replace(QStringLiteral("${foo}"), QStringLiteral("xxfoo")));

    doc->removeText(KTextEditor::Range({0, 0}, {0, 2}));
    QCOMPARE(doc->text(), QString(text).replace(QStringLiteral("${foo}"), QStringLiteral("foo")));

    delete doc;
}

void TemplateHandlerTest::testSimpleMirror_data()
{
    QTest::addColumn<QString>("text");

    QTest::newRow("one") << QStringLiteral("${foo}");
    QTest::newRow("several") << QStringLiteral("${foo} ${foo} Foo ${foo}");
}

void TemplateHandlerTest::testAlignC()
{
    QFETCH(QString, input);
    QFETCH(QString, expected);

    auto doc = new KTextEditor::DocumentPrivate();
    doc->setHighlightingMode(QStringLiteral("C"));
    auto view = static_cast<KTextEditor::ViewPrivate *>(doc->createView(nullptr));
    view->insertTemplate({0, 0}, input);

    QCOMPARE(doc->text(), expected);

    delete doc;
}

void TemplateHandlerTest::testAlignC_data()
{
    QTest::addColumn<QString>("input");
    QTest::addColumn<QString>("expected");

    QTest::newRow("one") << QStringLiteral("/* ${foo} */") << QStringLiteral("/* foo */");
    QTest::newRow("simple") << QStringLiteral("/**\n* ${foo}\n*/") << QStringLiteral("/**\n * foo\n */");
    QTest::newRow("complex") << QStringLiteral("/**\n* @brief: ${...}\n* \n*/") << QStringLiteral("/**\n * @brief: ...\n * \n */");
}

void TemplateHandlerTest::testAdjacentRanges()
{
    auto doc = new KTextEditor::DocumentPrivate();
    auto view = static_cast<KTextEditor::ViewPrivate *>(doc->createView(nullptr));

    auto reset = [&view, &doc](const QString &snippet, const QString &expected) {
        view->selectAll();
        view->keyDelete();

        view->insertTemplate({0, 0}, snippet);
        QCOMPARE(doc->text(), expected);
    };

    reset(QStringLiteral("${foo} ${foo}"), QStringLiteral("foo foo"));
    doc->removeText(KTextEditor::Range({0, 3}, {0, 4}));
    QCOMPARE(doc->text(), QStringLiteral("foofoo"));
    doc->insertText({0, 1}, QStringLiteral("x"));
    QCOMPARE(doc->text(), QStringLiteral("fxoofxoo"));
    doc->insertText({0, 4}, QStringLiteral("y"));
    QCOMPARE(doc->text(), QStringLiteral("fxooyfxooy"));
    doc->removeText(KTextEditor::Range({0, 4}, {0, 5}));
    QCOMPARE(doc->text(), QStringLiteral("fxoofxoo"));

    reset(QStringLiteral("${foo}${bar}${baz} ${foo}/${bar}/${baz}"), QStringLiteral("foobarbaz foo/bar/baz"));
    doc->insertText({0, 0}, QStringLiteral("x"));
    QCOMPARE(doc->text(), QStringLiteral("xfoobarbaz xfoo/bar/baz"));
    doc->insertText({0, 4}, QStringLiteral("x"));
    QCOMPARE(doc->text(), QStringLiteral("xfooxbarbaz xfoox/bar/baz"));
    doc->insertText({0, 8}, QStringLiteral("x"));
    QCOMPARE(doc->text(), QStringLiteral("xfooxbarxbaz xfoox/barx/baz"));
    doc->insertText({0, 12}, QStringLiteral("x"));
    QCOMPARE(doc->text(), QStringLiteral("xfooxbarxbazx xfoox/barx/bazx"));
    doc->removeText(KTextEditor::Range({0, 4}, {0, 5}));
    QCOMPARE(doc->text(), QStringLiteral("xfoobarxbazx xfoo/barx/bazx"));
    doc->insertText({0, 4}, QStringLiteral("y"));
    QCOMPARE(doc->text(), QStringLiteral("xfooybarxbazx xfooy/barx/bazx"));
    doc->removeText(KTextEditor::Range({0, 8}, {0, 9}));
    QCOMPARE(doc->text(), QStringLiteral("xfooybarbazx xfooy/bar/bazx"));
    doc->insertText({0, 8}, QStringLiteral("y"));
    QCOMPARE(doc->text(), QStringLiteral("xfooybarybazx xfooy/bary/bazx"));

    reset(QStringLiteral("${foo} ${bar} / ${foo} ${bar}"), QStringLiteral("foo bar / foo bar"));
    doc->removeText(KTextEditor::Range({0, 2}, {0, 5}));
    QCOMPARE(doc->text(), QStringLiteral("foar / fo ar"));
    doc->insertText({0, 2}, QStringLiteral("x"));
    QCOMPARE(doc->text(), QStringLiteral("foxar / fox ar"));
    doc->insertText({0, 5}, QStringLiteral("x"));
    QCOMPARE(doc->text(), QStringLiteral("foxarx / fox arx"));

    reset(QStringLiteral("${foo} ${bar} / ${foo} ${bar}"), QStringLiteral("foo bar / foo bar"));
    doc->removeText(KTextEditor::Range({0, 2}, {0, 7}));
    QCOMPARE(doc->text(), QStringLiteral("fo / fo "));
    doc->insertText({0, 2}, QStringLiteral("x"));
    QCOMPARE(doc->text(), QStringLiteral("fox / fox "));
    doc->insertText({0, 4}, QStringLiteral("x"));
    QCOMPARE(doc->text(), QStringLiteral("fox x/ fox "));

    reset(QStringLiteral("${foo}${bar} / ${foo} ${bar}"), QStringLiteral("foobar / foo bar"));
    doc->insertText({0, 3}, QStringLiteral("1"));
    doc->insertText({0, 4}, QStringLiteral("2"));
    QCOMPARE(doc->text(), QStringLiteral("foo12bar / foo12 bar"));
    doc->insertText({0, 8}, QStringLiteral("x"));
    doc->insertText({0, 9}, QStringLiteral("y"));
    QCOMPARE(doc->text(), QStringLiteral("foo12barxy / foo12 barxy"));

    delete doc;
}

void TemplateHandlerTest::testAdjacentRanges2()
{
    auto doc = new KTextEditor::DocumentPrivate();
    auto view = static_cast<KTextEditor::ViewPrivate *>(doc->createView(nullptr));

    auto reset = [&view, &doc]() {
        view->selectAll();
        view->keyDelete();

        view->insertTemplate({0, 0}, QStringLiteral("${foo}${foo='12345'}${foo}"));

        QCOMPARE(doc->text(), QStringLiteral("123451234512345"));
        QCOMPARE(view->cursorPosition().column(), 5);
        QCOMPARE(view->selectionText(), QStringLiteral("12345"));
        QCOMPARE(view->selectionRange().start().column(), 5);
    };

    reset();

    view->clearSelection();
    view->setCursorPosition({0, 5});
    QTest::keyClick(view->focusProxy(), 'X');
    QCOMPARE(doc->text(), QStringLiteral("X12345X12345X12345"));
    QCOMPARE(view->cursorPosition().column(), 7);

    reset();

    QTest::keyClick(view->focusProxy(), Qt::Key_A);
    QCOMPARE(doc->text(), QStringLiteral("aaa"));
    QCOMPARE(view->cursorPosition().column(), 2);

    // QTest::keyClick(view->focusProxy(), Qt::Key_Backspace);
    view->backspace();
    QCOMPARE(doc->text(), QStringLiteral(""));
    QCOMPARE(view->cursorPosition().column(), 0);

    QTest::keyClick(view->focusProxy(), Qt::Key_A);
    QCOMPARE(doc->text(), QStringLiteral("aaa"));
    QCOMPARE(view->cursorPosition().column(), 2);

    QTest::keyClick(view->focusProxy(), Qt::Key_B);
    QCOMPARE(doc->text(), QStringLiteral("ababab"));
    QCOMPARE(view->cursorPosition().column(), 4);

    reset();

    view->clearSelection();
    view->setCursorPosition({0, 8});
    // QTest::keyClick(view->focusProxy(), Qt::Key_Backspace);
    view->backspace();
    QCOMPARE(doc->text(), QStringLiteral("124512451245"));
    QCOMPARE(view->cursorPosition().column(), 6);

    reset();

    view->setSelection({{0, 7}, {0, 8}});
    view->setCursorPosition({0, 7});
    // QTest::keyClick(view->focusProxy(), Qt::Key_Backspace);
    view->backspace();
    QCOMPARE(doc->text(), QStringLiteral("124512451245"));
    QCOMPARE(view->cursorPosition().column(), 6);

    reset();

    view->clearSelection();
    view->setCursorPosition({0, 10});
    QTest::keyClick(view->focusProxy(), 'X');
    QCOMPARE(doc->text(), QStringLiteral("12345X12345X12345X"));
    QCOMPARE(view->cursorPosition().column(), 12);

    delete doc;
}

void TemplateHandlerTest::testAdjacentRanges3()
{
    auto doc = new KTextEditor::DocumentPrivate();
    auto view = static_cast<KTextEditor::ViewPrivate *>(doc->createView(nullptr));

    view->insertTemplate({0, 0}, QStringLiteral("${foo}${foo='foo'}${foo}"));

    QCOMPARE(doc->text(), QStringLiteral("foofoofoo"));
    QCOMPARE(view->cursorPosition().column(), 3);
    QCOMPARE(view->selectionText(), QStringLiteral("foo"));
    QCOMPARE(view->selectionRange().start().column(), 3);

    view->clearSelection();
    view->setCursorPosition({0, 4});
    QCOMPARE(doc->text(), QStringLiteral("foofoofoo"));
    QCOMPARE(view->cursorPosition().column(), 4);

    QTest::keyClick(view->focusProxy(), 'X');
    QCOMPARE(doc->text(), QStringLiteral("fXoofXoofXoo"));
    QCOMPARE(view->cursorPosition().column(), 6);

    // QTest::keyClick(view->focusProxy(), Qt::Key_Backspace);
    view->backspace();
    QCOMPARE(doc->text(), QStringLiteral("foofoofoo"));
    QCOMPARE(view->cursorPosition().column(), 4);

    QTest::keyClick(view->focusProxy(), 'Z');
    QCOMPARE(doc->text(), QStringLiteral("fZoofZoofZoo"));
    QCOMPARE(view->cursorPosition().column(), 6);

    delete doc;
}

void TemplateHandlerTest::testTab()
{
    QFETCH(QString, tpl);
    QFETCH(int, cursor);

    auto doc = new KTextEditor::DocumentPrivate();
    auto view = static_cast<KTextEditor::ViewPrivate *>(doc->createView(nullptr));

    view->insertTemplate({0, 0}, tpl);
    view->setCursorPosition({0, cursor});

    QTest::keyClick(view->focusProxy(), Qt::Key_Tab);
    QEXPECT_FAIL("jump_to_cursor_last", "Regression in KateTemplateHandler::jump", Continue);
    QEXPECT_FAIL("jump_to_cursor_last2", "Regression in KateTemplateHandler::jump", Continue);
    QTEST(view->cursorPosition().column(), "expected_cursor");

    QTest::keyClick(view->focusProxy(), Qt::Key_Tab, Qt::ShiftModifier);
    QTest::keyClick(view->focusProxy(), Qt::Key_Tab);
    QEXPECT_FAIL("jump_to_cursor_last", "Regression in KateTemplateHandler::jump", Continue);
    QEXPECT_FAIL("jump_to_cursor_last2", "Regression in KateTemplateHandler::jump", Continue);
    QTEST(view->cursorPosition().column(), "expected_cursor");

    QTest::keyClick(view->focusProxy(), Qt::Key_Backtab);
    QTest::keyClick(view->focusProxy(), Qt::Key_Tab);
    QTEST(view->cursorPosition().column(), "expected_cursor");

    QEXPECT_FAIL("jump_to_cursor_last", "Regression in KateTemplateHandler::jump", Continue);
    QEXPECT_FAIL("jump_to_cursor_last2", "Regression in KateTemplateHandler::jump", Continue);
    QTest::keyClick(view->focusProxy(), Qt::Key_A);
    QTEST(doc->text(), "expected_edited_result");

    delete doc;
}

void TemplateHandlerTest::testTab_data()
{
    QTest::addColumn<QString>("tpl");
    QTest::addColumn<int>("cursor");
    QTest::addColumn<int>("expected_cursor");
    QTest::addColumn<QString>("expected_edited_result");

    QTest::newRow("simple_start") << "${foo} ${bar}" << 0 << 4 << "foo a";
    QTest::newRow("simple_mid") << "${foo} ${bar}" << 2 << 4 << "foo a";
    QTest::newRow("simple_end") << "${foo} ${bar}" << 3 << 4 << "foo a";
    QTest::newRow("adjacent_start") << "${foo}${bar} / ${foo} ${bar}" << 0 << 3 << "fooa / foo a";
    QTest::newRow("adjacent_mid_1st") << "${foo}${bar}${baz} / ${foo} ${bar} ${baz}" << 2 << 3 << "fooabaz / foo a baz";
    QTest::newRow("adjacent_mid_2nd") << "${foo}${bar}${baz} / ${foo} ${bar} ${baz}" << 4 << 6 << "foobara / foo bar a";
    QTest::newRow("adjacent_mixed_start") << "${foo} ${bar}${baz} / ${foo} ${bar} ${baz}" << 5 << 7 << "foo bara / foo bar a";
    QTest::newRow("adjacent_mixed_end") << "${foo}${bar} ${baz} / ${foo} ${bar} ${baz}" << 5 << 7 << "foobar a / foo bar a";
    QTest::newRow("adjacent_repeat") << "${foo}${foo='foo'}${foo}" << 3 << 3 << "aaa";
    QTest::newRow("wrap_start") << "${foo} ${bar}" << 4 << 0 << "a bar";
    QTest::newRow("wrap_mid") << "${foo} ${bar}" << 5 << 0 << "a bar";
    QTest::newRow("wrap_end") << "${foo} ${bar}" << 6 << 0 << "a bar";
    QTest::newRow("non_editable_start") << "${foo} ${foo}" << 0 << 0 << "a a";
    QTest::newRow("non_editable_mid") << "${foo} ${foo}" << 2 << 0 << "a a";
    QTest::newRow("non_editable_end") << "${foo} ${foo}" << 3 << 0 << "a a";
    QTest::newRow("skip_non_editable") << "${foo} ${foo} ${bar}" << 0 << 8 << "foo foo a";
    QTest::newRow("skip_non_editable_at_end") << "${foo} ${bar} ${foo}" << 4 << 0 << "a bar a";
    QTest::newRow("jump_to_cursor") << "${foo} ${cursor}" << 0 << 4 << "foo a";
    QTest::newRow("jump_to_cursor_last") << "${foo} ${cursor} ${bar}" << 0 << 5 << "foo  a";
    QTest::newRow("jump_to_cursor_last2") << "${foo} ${cursor} ${bar}" << 5 << 4 << "foo a bar";
    QTest::newRow("reference_default") << "${foo} ${bar} ${baz=bar}" << 0 << 4 << "foo a bar";
}

void TemplateHandlerTest::testExit()
{
    auto doc = new KTextEditor::DocumentPrivate();
    auto view = static_cast<KTextEditor::ViewPrivate *>(doc->createView(nullptr));

    auto reset = [&view, &doc](const QString &snippet, const QString &expected) {
        view->selectAll();
        view->keyDelete();

        view->insertTemplate({0, 0}, snippet);
        QCOMPARE(doc->text(), expected);
    };

    auto finish = [&view, &doc](const QString &expected) {
        // required to process the deleteLater() used to exit the template handler
        QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
        QApplication::processEvents();

        // go to the first field and verify it's not mirrored any more (i.e. the handler exited)
        view->setCursorPosition({0, 0});
        QTest::keyClick(view->focusProxy(), Qt::Key_A);
        QCOMPARE(doc->text(), QStringLiteral("a") + expected);
    };

    // Test: insert at ${cursor}
    reset(QStringLiteral("${foo} ${bar} ${cursor} ${foo}"), QStringLiteral("foo bar  foo"));
    view->setCursorPosition({0, 0});

    // - check it jumps to the cursor
    QTest::keyClick(view->focusProxy(), Qt::Key_Tab);
    QCOMPARE(view->cursorPosition().column(), 4);
    QTest::keyClick(view->focusProxy(), Qt::Key_Tab);
    QCOMPARE(view->cursorPosition().column(), 8);

    // - insert an "a" at ${cursor}
    QTest::keyClick(view->focusProxy(), Qt::Key_A);
    // check it was inserted
    QCOMPARE(doc->text(), QStringLiteral("foo bar a foo"));

    finish(QStringLiteral("foo bar a foo"));

    // Test: press escape
    reset(QStringLiteral("${foo} ${bar} ${cursor} ${foo}"), QStringLiteral("foo bar  foo"));

    // - ensure fields work as expected
    view->setCursorPosition({0, 0});
    view->clearSelection();
    QTest::keyClick(view->focusProxy(), Qt::Key_A);
    QCOMPARE(doc->text(), QStringLiteral("afoo bar  afoo"));

    // - finish editing
    QTest::keyClick(view->focusProxy(), Qt::Key_Escape);
    finish(QStringLiteral("afoo bar  afoo"));

    // Test: press alt+return
    reset(QStringLiteral("${foo} ${bar} ${cursor} ${foo}"), QStringLiteral("foo bar  foo"));

    // - ensure fields work as expected
    view->setCursorPosition({0, 0});
    view->clearSelection();
    QTest::keyClick(view->focusProxy(), Qt::Key_A);
    QCOMPARE(doc->text(), QStringLiteral("afoo bar  afoo"));

    // - finish editing
    QTest::keyClick(view->focusProxy(), Qt::Key_Return, Qt::AltModifier);
    finish(QStringLiteral("afoo bar  afoo"));

    delete doc;
}

void TemplateHandlerTest::testDefaultMirror()
{
    auto doc = new KTextEditor::DocumentPrivate();
    auto view = static_cast<KTextEditor::ViewPrivate *>(doc->createView(nullptr));

    view->insertTemplate({0, 0},
                         QStringLiteral("${foo=uppercase(\"hi\")} ${bar=3} ${foo}"),
                         QStringLiteral("function uppercase(x) { return x.toUpperCase(); }"));
    QCOMPARE(doc->text(), QStringLiteral("HI 3 HI"));
    doc->insertText({0, 0}, QStringLiteral("xy@"));
    QCOMPARE(doc->text(), QStringLiteral("xy@HI 3 xy@HI"));

    delete doc;
}

void TemplateHandlerTest::testFunctionMirror()
{
    auto doc = new KTextEditor::DocumentPrivate();
    auto view = static_cast<KTextEditor::ViewPrivate *>(doc->createView(nullptr));

    view->insertTemplate({0, 0}, QStringLiteral("${foo} hi ${uppercase(foo)}"), QStringLiteral("function uppercase(x) { return x.toUpperCase(); }"));
    QCOMPARE(doc->text(), QStringLiteral("foo hi FOO"));
    doc->insertText({0, 0}, QStringLiteral("xy@"));
    QCOMPARE(doc->text(), QStringLiteral("xy@foo hi XY@FOO"));

    delete doc;
}

void TemplateHandlerTest::testAutoSelection()
{
    auto doc = new KTextEditor::DocumentPrivate();
    auto view = static_cast<KTextEditor::ViewPrivate *>(doc->createView(nullptr));

    view->insertTemplate({0, 0}, QStringLiteral("${foo} ${bar} ${bar} ${cursor} ${baz}"));
    QCOMPARE(doc->text(), QStringLiteral("foo bar bar  baz"));
    QCOMPARE(view->selectionText(), QStringLiteral("foo"));

    QTest::keyClick(view->focusProxy(), Qt::Key_Tab);
    QCOMPARE(view->selectionText(), QStringLiteral("bar"));

    QTest::keyClick(view->focusProxy(), Qt::Key_Tab);
    // QCOMPARE(view->selectionText(), QStringLiteral("baz"));
    QVERIFY(view->selectionRange().isEmpty());

    QTest::keyClick(view->focusProxy(), Qt::Key_Tab);
    QCOMPARE(view->selectionText(), QStringLiteral("baz"));
    // QVERIFY(view->selectionRange().isEmpty());

    QTest::keyClick(view->focusProxy(), Qt::Key_Tab);
    QCOMPARE(view->selectionText(), QStringLiteral("foo"));
    QTest::keyClick(view->focusProxy(), Qt::Key_A);
    QCOMPARE(doc->text(), QStringLiteral("a bar bar  baz"));

    QTest::keyClick(view->focusProxy(), Qt::Key_Tab);
    QTest::keyClick(view->focusProxy(), Qt::Key_Tab);
    QTest::keyClick(view->focusProxy(), Qt::Key_Tab);
    QTest::keyClick(view->focusProxy(), Qt::Key_Tab);
    QVERIFY(view->selectionRange().isEmpty());
}

void TemplateHandlerTest::testNotEditableFields()
{
    QFETCH(QString, input);
    QFETCH(int, change_offset);

    auto doc = new KTextEditor::DocumentPrivate();
    auto view = static_cast<KTextEditor::ViewPrivate *>(doc->createView(nullptr));
    view->insertTemplate({0, 0}, input);

    doc->insertText({0, change_offset}, QStringLiteral("xxx"));
    QTEST(doc->text(), "expected");
}

void TemplateHandlerTest::testNotEditableFields_data()
{
    QTest::addColumn<QString>("input");
    QTest::addColumn<int>("change_offset");
    QTest::addColumn<QString>("expected");

    QTest::newRow("mirror") << QStringLiteral("${foo} ${foo}") << 6 << "foo foxxxo";
}

void TemplateHandlerTest::testCanRetrieveSelection()
{
    auto doc = new KTextEditor::DocumentPrivate();
    auto view = static_cast<KTextEditor::ViewPrivate *>(doc->createView(nullptr));
    view->insertText(QStringLiteral("hi world"));
    view->setSelection(KTextEditor::Range(0, 1, 0, 4));
    view->insertTemplate({0, 1}, QStringLiteral("xx${foo=sel()}xx"), QStringLiteral("function sel() { return view.selectedText(); }"));
    QCOMPARE(doc->text(), QStringLiteral("hxxi wxxorld"));
}

void TemplateHandlerTest::testDefaults_data()
{
    auto uppercase = QStringLiteral("function uppercase(x) { return x.toUpperCase(); }");

    QTest::addColumn<QString>("input");
    QTest::addColumn<QString>("expected");
    QTest::addColumn<QString>("function");

    using S = QString;
    QTest::newRow("empty") << S() << S() << S();
    QTest::newRow("foo") << QStringLiteral("${foo}") << QStringLiteral("foo") << S();
    QTest::newRow("foo=3") << QStringLiteral("${foo=3}") << QStringLiteral("3") << S();
    QTest::newRow("${foo=3+5}") << QStringLiteral("${foo=3+5}") << QStringLiteral("8") << S();
    QTest::newRow("string") << QStringLiteral("${foo=\"3+5\"}") << QStringLiteral("3+5") << S();
    QTest::newRow("backslash") << QStringLiteral(R"(${foo='c:\\folder'})") << QStringLiteral(R"(c:\folder)") << S();
    QTest::newRow("string_single_quote") << QStringLiteral("${foo='3+5'}") << QStringLiteral("3+5") << S();
    QTest::newRow("string_mirror") << QStringLiteral("${foo=\"Bar\"} ${foo}") << QStringLiteral("Bar Bar") << S();
    QTest::newRow("mirror_before_default") << QStringLiteral("${foo} ${foo='bar'}") << QStringLiteral("bar bar") << S();
    QTest::newRow("duplicate_default") << QStringLiteral("${foo=1} ${foo=2}") << QStringLiteral("1 1") << S();
    QTest::newRow("func_simple") << QStringLiteral("${foo=myfunc()}") << QStringLiteral("hi") << QStringLiteral("function myfunc() { return 'hi'; }");
    QTest::newRow("func_fixed") << QStringLiteral("${myfunc()}") << QStringLiteral("hi") << QStringLiteral("function myfunc() { return 'hi'; }");
    QTest::newRow("func_constant_arg") << QStringLiteral("${foo=uppercase(\"Foo\")}") << QStringLiteral("FOO") << uppercase;
    QTest::newRow("func_constant_arg_mirror") << QStringLiteral("${foo=uppercase(\"hi\")} ${bar=3} ${foo}") << QStringLiteral("HI 3 HI") << uppercase;
    QTest::newRow("cursor") << QStringLiteral("${foo} ${cursor}") << QStringLiteral("foo ") << S();
    QTest::newRow("only_cursor") << QStringLiteral("${cursor}") << QStringLiteral("") << S();
    QTest::newRow("only_cursor_stuff") << QStringLiteral("fdas ${cursor} asdf") << QStringLiteral("fdas  asdf") << S();
    QTest::newRow("reference_default") << QStringLiteral("${a='foo'} ${b=a}") << QStringLiteral("foo foo") << S();
    QTest::newRow("reference_plain") << QStringLiteral("${a} ${b=a}") << QStringLiteral("a a") << S();
    QTest::newRow("reference_unevaluated1") << QStringLiteral("${a=b} ${b='foo'}") << QStringLiteral("ReferenceError: b is not defined foo") << S();
    QTest::newRow("reference_unevaluated2") << QStringLiteral("${a=b} ${b}") << QStringLiteral("ReferenceError: b is not defined b") << S();
    QTest::newRow("reference_unevaluated3") << QStringLiteral("${a=fields.b} ${b}") << QStringLiteral("undefined b") << S();
    QTest::newRow("reference_empty") << QStringLiteral("${foo} ${bar=foo}") << QStringLiteral("foo foo") << S();
    QTest::newRow("reference_error") << QStringLiteral("${a=b}") << QStringLiteral("ReferenceError: b is not defined") << S();
    QTest::newRow("reference_func") << QStringLiteral("${foo} ${uppercase(foo)}") << QStringLiteral("foo FOO") << uppercase;
    QTest::newRow("reference_func_default") << QStringLiteral("${foo} ${bar=uppercase(foo)}") << QStringLiteral("foo FOO") << uppercase;
    QTest::newRow("reference_func_repeat") << QStringLiteral("${foo} ${uppercase(foo)} ${uppercase(foo)}") << QStringLiteral("foo FOO FOO") << uppercase;
    QTest::newRow("reference_func_body1") << QStringLiteral("${foo} ${myfunc()}") << QStringLiteral("foo ReferenceError: foo is not defined")
                                          << QStringLiteral("function myfunc() { return foo; }");
    QTest::newRow("reference_func_body2") << QStringLiteral("${foo} ${myfunc()}") << QStringLiteral("foo foo")
                                          << QStringLiteral("function myfunc() { return fields.foo; }");
    QTest::newRow("reference_self") << QStringLiteral("${foo=uppercase(foo)}") << QStringLiteral("FOO") << uppercase;
    QTest::newRow("invalid_js1") << QStringLiteral("${foo=break}") << QStringLiteral("SyntaxError: Break outside of loop") << S();
    QTest::newRow("invalid_js2") << QStringLiteral("${foo=#break#}") << QStringLiteral("undefined") << S();
    QTest::newRow("invalid_js_identifier") << QStringLiteral("${foo.bar}") << QStringLiteral("foo.bar") << S();
    QTest::newRow("invalid_js_identifier_in_env1") << QStringLiteral("${foo.bar} ${uppercase('x')}") << QStringLiteral("foo.bar X") << uppercase;
    QTest::newRow("invalid_js_identifier_in_env2") << QStringLiteral("${foo.bar} ${baz=foo.bar}")
                                                   << QStringLiteral("foo.bar ReferenceError: foo is not defined") << S();
    QTest::newRow("invalid_js_identifier_in_env3") << QStringLiteral("${foo.bar} ${baz=fields['foo.bar']}") << QStringLiteral("foo.bar foo.bar") << S();
    QTest::newRow("invalid_js_identifier_in_env4") << QStringLiteral("${volatile} ${foo=volatile}")
                                                   << QStringLiteral("volatile SyntaxError: access volatile through the fields map") << S();
    QTest::newRow("field_name_global") << QStringLiteral("${document='myfield'} ${foo=document.encoding()} ${bar=fields.document}")
                                       << QStringLiteral("myfield UTF-8 myfield") << uppercase;
}

void TemplateHandlerTest::testDefaults()
{
    auto doc = new KTextEditor::DocumentPrivate();
    auto view = static_cast<KTextEditor::ViewPrivate *>(doc->createView(nullptr));

    QFETCH(QString, input);
    QFETCH(QString, function);

    view->insertTemplate(KTextEditor::Cursor(0, 0), input, function);
    QTEST(doc->text(), "expected");

    view->selectAll();
    view->keyDelete();
    QCOMPARE(doc->text(), QString());

    delete doc;
}

void TemplateHandlerTest::testLinesRemoved()
{
    auto doc = new KTextEditor::DocumentPrivate();
    auto view = static_cast<KTextEditor::ViewPrivate *>(doc->createView(nullptr));

    view->insertTemplate({0, 0}, QStringLiteral("${foo}\n${bar}\n${bax}\n${baz}"));
    QCOMPARE(doc->text(), QStringLiteral("foo\nbar\nbax\nbaz"));
    QCOMPARE(view->selectionText(), QStringLiteral("foo"));

    view->killLine();
    view->killLine();
    QVERIFY(view->selectionRange().isEmpty());

    QTest::keyClick(view->focusProxy(), Qt::Key_Tab);
    QCOMPARE(view->selectionText(), QStringLiteral("baz"));

    QTest::keyClick(view->focusProxy(), Qt::Key_Tab);
    QCOMPARE(view->selectionText(), QStringLiteral("bax"));
}

void TemplateHandlerTest::testLinesRemoved2()
{
    // BUG: 434093
    auto doc = new KTextEditor::DocumentPrivate();
    auto view = static_cast<KTextEditor::ViewPrivate *>(doc->createView(nullptr));

    view->insertTemplate({0, 0}, QStringLiteral(R"(@book{${bibkey},
  author    = {${author}},
  editor    = {${editor}},
  title     = {${title}},
  series    = {${series}},
  number    = {${number}},
  publisher = {${publisher}},
  year      = {${year}},
  pagetotal = {${pagetotal}},
  file      = {:${file}:PDF},
  url       = {${url}},
  edition   = {${edition}},
  abstract  = {${abstract}},
  groups    = {${groups}},
}

${cursor}
)"));
    QCOMPARE(view->selectionText(), QStringLiteral("bibkey"));

    QTest::keyClick(view->focusProxy(), Qt::Key_Tab);
    QCOMPARE(view->selectionText(), QStringLiteral("author"));

    view->killLine();
    view->killLine();
    QVERIFY(view->selectionRange().isEmpty());

    QTest::keyClick(view->focusProxy(), Qt::Key_Tab);
    QCOMPARE(view->selectionText(), QStringLiteral("title"));

    QTest::keyClick(view->focusProxy(), Qt::Key_Tab);
    QCOMPARE(view->selectionText(), QStringLiteral("series"));
}

void TemplateHandlerTest::testReadOnly()
{
    auto doc = new KTextEditor::DocumentPrivate();
    doc->setReadWrite(false);

    auto view = static_cast<KTextEditor::ViewPrivate *>(doc->createView(nullptr));

    bool ok = view->insertTemplate({0, 0}, QStringLiteral("${foo} ${foo}"));

    // no text should have been inserted because the document is read-only
    QCOMPARE(ok, false);
    QCOMPARE(doc->text(), QStringLiteral(""));

    // input should have no effect
    view->setCursorPosition({0, 0});
    QTest::keyClick(view->focusProxy(), Qt::Key_A);
    QCOMPARE(doc->text(), QStringLiteral(""));
}

void TemplateHandlerTest::testMultipleViews()
{
    auto doc = new KTextEditor::DocumentPrivate();

    auto view1 = static_cast<KTextEditor::ViewPrivate *>(doc->createView(nullptr));
    view1->insertTemplate({0, 0}, QStringLiteral("${foo} ${foo}"));
    QCOMPARE(doc->text(), QStringLiteral("foo foo"));

    auto view2 = static_cast<KTextEditor::ViewPrivate *>(doc->createView(nullptr));
    view2->setCursorPosition({0, 1});
    QTest::keyClick(view2->focusProxy(), 'X');
    QCOMPARE(doc->text(), QStringLiteral("fXoo fXoo"));
}

#include "moc_templatehandler_test.cpp"
