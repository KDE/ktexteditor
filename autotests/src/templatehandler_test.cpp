/*
    This file is part of the KDE libraries
    SPDX-FileCopyrightText: 2010 Bernhard Beschow <bbeschow@cs.tu-berlin.de>

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
    auto doc = new KTextEditor::DocumentPrivate();
    auto view = static_cast<KTextEditor::ViewPrivate *>(doc->createView(nullptr));
    view->insertTemplate({0, 0}, QStringLiteral("\\${field} ${bar} \\${foo=3} \\\\${baz=7}"));
    QCOMPARE(doc->text(), QStringLiteral("${field} bar ${foo=3} \\${baz=7}"));
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

    QEXPECT_FAIL("", "TBD", Continue);
    reset(QStringLiteral("${foo}${bar}${baz} ${foo}/${bar}/${baz}"), QStringLiteral("foobarbaz foo/bar/baz"));
    doc->insertText({0, 0}, QStringLiteral("x"));
    QEXPECT_FAIL("", "TBD", Continue);
    QCOMPARE(doc->text(), QStringLiteral("xfoobarbaz xfoo/bar/baz"));
    doc->insertText({0, 4}, QStringLiteral("x"));
    QEXPECT_FAIL("", "TBD", Continue);
    QCOMPARE(doc->text(), QStringLiteral("xfooxbarbaz xfoox/bar/baz"));
    doc->insertText({0, 8}, QStringLiteral("x"));
    QEXPECT_FAIL("", "TBD", Continue);
    QCOMPARE(doc->text(), QStringLiteral("xfooxbarxbaz xfoox/barx/baz"));
    doc->insertText({0, 12}, QStringLiteral("x"));
    QEXPECT_FAIL("", "TBD", Continue);
    QCOMPARE(doc->text(), QStringLiteral("xfooxbarbazx xfoox/barx/bazx"));
    doc->removeText(KTextEditor::Range({0, 4}, {0, 5}));
    QEXPECT_FAIL("", "TBD", Continue);
    QCOMPARE(doc->text(), QStringLiteral("xfoobarxbaz xfoo/barx/baz"));
    doc->insertText({0, 4}, QStringLiteral("y"));
    QEXPECT_FAIL("", "TBD", Continue);
    QCOMPARE(doc->text(), QStringLiteral("xfooybarxbaz xfooy/barx/baz"));
    doc->removeText(KTextEditor::Range({0, 8}, {0, 9}));
    QEXPECT_FAIL("", "TBD", Continue);
    QCOMPARE(doc->text(), QStringLiteral("xfooybarbaz xfooy/bar/baz"));
    doc->insertText({0, 8}, QStringLiteral("y"));
    QEXPECT_FAIL("", "TBD", Continue);
    QCOMPARE(doc->text(), QStringLiteral("xfooybarybaz xfooy/bary/baz"));

    reset(QStringLiteral("${foo} ${bar} / ${foo} ${bar}"), QStringLiteral("foo bar / foo bar"));
    doc->removeText(KTextEditor::Range({0, 2}, {0, 5}));
    QEXPECT_FAIL("", "TBD", Continue);
    QCOMPARE(doc->text(), QStringLiteral("foar / fo ar"));
    doc->insertText({0, 2}, QStringLiteral("x"));
    QEXPECT_FAIL("", "TBD", Continue);
    QCOMPARE(doc->text(), QStringLiteral("foxar / fox ar"));
    doc->insertText({0, 5}, QStringLiteral("x"));
    QEXPECT_FAIL("", "TBD", Continue);
    QCOMPARE(doc->text(), QStringLiteral("foxar / fox arx"));

    reset(QStringLiteral("${foo} ${bar} / ${foo} ${bar}"), QStringLiteral("foo bar / foo bar"));
    doc->removeText(KTextEditor::Range({0, 2}, {0, 7}));
    QEXPECT_FAIL("", "TBD", Continue);
    QCOMPARE(doc->text(), QStringLiteral("fo / fo "));
    doc->insertText({0, 2}, QStringLiteral("x"));
    QEXPECT_FAIL("", "TBD", Continue);
    QCOMPARE(doc->text(), QStringLiteral("fox / fox "));
    doc->insertText({0, 4}, QStringLiteral("x"));
    QEXPECT_FAIL("", "TBD", Continue);
    QCOMPARE(doc->text(), QStringLiteral("fox x/ fox "));

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

    // no idea why the event needs to be posted to the focus proxy
    QTest::keyClick(view->focusProxy(), Qt::Key_Tab);
    QEXPECT_FAIL("adjacent_mid_1st", "TBD", Continue);
    QEXPECT_FAIL("adjacent_mid_2nd", "TBD", Continue);
    QEXPECT_FAIL("adjacent_mixed_start", "TBD", Continue);
    QEXPECT_FAIL("adjacent_mixed_end", "TBD", Continue);
    QTEST(view->cursorPosition().column(), "expected_cursor");

    QTest::keyClick(view->focusProxy(), Qt::Key_Tab, Qt::ShiftModifier);
    QTest::keyClick(view->focusProxy(), Qt::Key_Tab);
    QEXPECT_FAIL("adjacent_mid_1st", "TBD", Continue);
    QEXPECT_FAIL("adjacent_mid_2nd", "TBD", Continue);
    QEXPECT_FAIL("adjacent_mixed_start", "TBD", Continue);
    QEXPECT_FAIL("adjacent_mixed_end", "TBD", Continue);
    QTEST(view->cursorPosition().column(), "expected_cursor");

    QTest::keyClick(view->focusProxy(), Qt::Key_A);
    QEXPECT_FAIL("adjacent_start", "TBD", Continue);
    QEXPECT_FAIL("adjacent_mid_1st", "TBD", Continue);
    QEXPECT_FAIL("adjacent_mid_2nd", "TBD", Continue);
    QEXPECT_FAIL("adjacent_mixed_start", "TBD", Continue);
    QEXPECT_FAIL("adjacent_mixed_end", "TBD", Continue);
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
    QTest::newRow("adjacent_start") << "${foo}${bar}" << 0 << 0 << "fooa";
    QTest::newRow("adjacent_mid_1st") << "${foo}${bar}${baz}" << 2 << 3 << "fooabaz";
    QTest::newRow("adjacent_mid_2nd") << "${foo}${bar}${baz}" << 4 << 6 << "foobara";
    QTest::newRow("adjacent_mixed_start") << "${foo} ${bar}${baz}" << 5 << 7 << "foo bara";
    QTest::newRow("adjacent_mixed_end") << "${foo}${bar} ${baz}" << 5 << 7 << "foobar a";
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

void TemplateHandlerTest::testExitAtCursor()
{
    auto doc = new KTextEditor::DocumentPrivate();
    auto view = static_cast<KTextEditor::ViewPrivate *>(doc->createView(nullptr));

    view->insertTemplate({0, 0}, QStringLiteral("${foo} ${bar} ${cursor} ${foo}"));
    view->setCursorPosition({0, 0});

    // check it jumps to the cursor
    QTest::keyClick(view->focusProxy(), Qt::Key_Tab);
    QCOMPARE(view->cursorPosition().column(), 4);
    QTest::keyClick(view->focusProxy(), Qt::Key_Tab);
    QCOMPARE(view->cursorPosition().column(), 8);

    // insert an a at cursor position
    QTest::keyClick(view->focusProxy(), Qt::Key_A);
    // check it was inserted
    QCOMPARE(doc->text(), QStringLiteral("foo bar a foo"));

    // required to process the deleteLater() used to exit the template handler
    QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
    QApplication::processEvents();

    // go to the first field and verify it's not mirrored any more (i.e. the handler exited)
    view->setCursorPosition({0, 0});
    QTest::keyClick(view->focusProxy(), Qt::Key_A);
    QCOMPARE(doc->text(), QStringLiteral("afoo bar a foo"));

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
    QCOMPARE(view->selectionText(), QStringLiteral("baz"));

    QTest::keyClick(view->focusProxy(), Qt::Key_Tab);
    QVERIFY(view->selectionRange().isEmpty());

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
    QTest::newRow("string_single_quote") << QStringLiteral("${foo='3+5'}") << QStringLiteral("3+5") << S();
    QTest::newRow("string_mirror") << QStringLiteral("${foo=\"Bar\"} ${foo}") << QStringLiteral("Bar Bar") << S();
    QTest::newRow("func_simple") << QStringLiteral("${foo=myfunc()}") << QStringLiteral("hi") << QStringLiteral("function myfunc() { return 'hi'; }");
    QTest::newRow("func_fixed") << QStringLiteral("${myfunc()}") << QStringLiteral("hi") << QStringLiteral("function myfunc() { return 'hi'; }");
    QTest::newRow("func_constant_arg") << QStringLiteral("${foo=uppercase(\"Foo\")}") << QStringLiteral("FOO")
                                       << QStringLiteral("function uppercase(x) { return x.toUpperCase(); }");
    QTest::newRow("func_constant_arg_mirror") << QStringLiteral("${foo=uppercase(\"hi\")} ${bar=3} ${foo}") << QStringLiteral("HI 3 HI")
                                              << QStringLiteral("function uppercase(x) { return x.toUpperCase(); }");
    QTest::newRow("cursor") << QStringLiteral("${foo} ${cursor}") << QStringLiteral("foo ") << S();
    QTest::newRow("only_cursor") << QStringLiteral("${cursor}") << QStringLiteral("") << S();
    QTest::newRow("only_cursor_stuff") << QStringLiteral("fdas ${cursor} asdf") << QStringLiteral("fdas  asdf") << S();
    QTest::newRow("reference_default") << QStringLiteral("${a='foo'} ${b=a}") << QStringLiteral("foo foo") << S();
    QTest::newRow("reference_plain") << QStringLiteral("${a} ${b=a}") << QStringLiteral("a a") << S();
    QTest::newRow("reference_unevaluated") << QStringLiteral("${a=b} ${b='foo'}") << QStringLiteral("${b='foo'} foo") << S();
    QTest::newRow("reference_error") << QStringLiteral("${a=b}") << QStringLiteral("ReferenceError: b is not defined") << S();
    QTest::newRow("reference_func") << QStringLiteral("${foo} ${uppercase(foo)}") << QStringLiteral("foo FOO")
                                    << QStringLiteral("function uppercase(x) { return x.toUpperCase(); }");
    QTest::newRow("reference_func_default") << QStringLiteral("${foo} ${bar=uppercase(foo)}") << QStringLiteral("foo FOO")
                                            << QStringLiteral("function uppercase(x) { return x.toUpperCase(); }");
    QTest::newRow("reference_self") << QStringLiteral("${foo=uppercase(foo)}") << QStringLiteral("FOO")
                                    << QStringLiteral("function uppercase(x) { return x.toUpperCase(); }");
    QTest::newRow("reference_func_body1") << QStringLiteral("${foo} ${myfunc()}") << QStringLiteral("foo ReferenceError: foo is not defined")
                                          << QStringLiteral("function myfunc() { return foo; }");
    QTest::newRow("reference_func_body2") << QStringLiteral("${foo} ${myfunc()}") << QStringLiteral("foo foo")
                                          << QStringLiteral("function myfunc() { return fields.foo; }");
    QTest::newRow("invalid_js1") << QStringLiteral("${foo=break}") << QStringLiteral("SyntaxError: Break outside of loop") << S();
    QTest::newRow("invalid_js2") << QStringLiteral("${foo=#break#}") << QStringLiteral("undefined") << S();
    QTest::newRow("invalid_js_identifier") << QStringLiteral("${foo.bar}") << QStringLiteral("foo.bar") << S();
    QTest::newRow("invalid_js_identifier_in_env1") << QStringLiteral("${foo.bar} ${uppercase('x')}") << QStringLiteral("foo.bar X") << uppercase;
    QTest::newRow("invalid_js_identifier_in_env2") << QStringLiteral("${foo.bar} ${baz=foo.bar}")
                                                   << QStringLiteral("foo.bar ReferenceError: foo is not defined") << S();
    QTest::newRow("invalid_js_identifier_in_env3") << QStringLiteral("${foo.bar} ${baz=fields['foo.bar']}") << QStringLiteral("foo.bar foo.bar") << S();
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

#include "moc_templatehandler_test.cpp"
