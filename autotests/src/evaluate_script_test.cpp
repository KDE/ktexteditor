/*
    This file is part of the KDE libraries
    SPDX-FileCopyrightText: 2025 Thomas Friedrichsmeier <thomas.friedrichsmeier@kdemail.net>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "evaluate_script_test.h"

#include <KTextEditor/Cursor>
#include <KTextEditor/View>
#include <katedocument.h>
#include <kateglobal.h>

#include <QTest>

QTEST_MAIN(EvaluateScriptTest)

using namespace KTextEditor;

EvaluateScriptTest::EvaluateScriptTest()
    : QObject()
{
    QStandardPaths::setTestModeEnabled(true);
}

void EvaluateScriptTest::testUndo()
{
    const QString content = QStringLiteral(
        "for (int my_x = 1; my_x < 5; ++my_x)\n"
        "{\n"
        "    // another mention of my_x\n"
        "}");

    auto doc = new KTextEditor::DocumentPrivate();
    auto view = doc->createView(nullptr);
    doc->setText(content);

    const QString script = QStringLiteral(
        "for (let l = 0; l < document.lines(); ++l) {\n"
        "    let line = document.line(l).replace(/my_x/g, 'my_y');\n"
        "    document.removeLine(l);\n"
        "    document.insertLine(l, line);\n"
        "}");

    bool res = view->evaluateScript(script);

    // verify that script worked as expecsted
    QVERIFY(res);
    QCOMPARE(doc->text(), QString(content).replace(QStringLiteral("my_x"), QStringLiteral("my_y")));

    // a single undo should suffice
    doc->undo();
    QCOMPARE(doc->text(), content);
}

void EvaluateScriptTest::testError()
{
    auto doc = new KTextEditor::DocumentPrivate();
    auto view = doc->createView(nullptr);

    bool res = view->evaluateScript(QStringLiteral("syntaxerror){"));
    QVERIFY(!res);
}

void EvaluateScriptTest::testSelection()
{
    auto doc = new KTextEditor::DocumentPrivate();
    auto view = doc->createView(nullptr);
    const QString content = QStringLiteral("one shoe three\n");
    doc->setText(content);
    view->setSelection(KTextEditor::Range(0, 4, 0, 8));
    QCOMPARE(view->selectionText(), QStringLiteral("shoe"));

    const QString script = QStringLiteral(
        "require('range.js')\n"
        "let pos = view.selection().start;\n"
        "view.removeSelectedText();\n"
        "document.insertText(pos, 'two');\n"
        "view.setSelection(new Range(0, 8, 0, 14));\n");

    bool res = view->evaluateScript(script);
    QVERIFY(res);

    // did we replace the correct thing (based on current selection)?
    QCOMPARE(doc->text(), QString(content).replace(QStringLiteral("shoe"), QStringLiteral("two")));

    // did we select text, successfully?
    QCOMPARE(view->selectionText(), QStringLiteral("three"));
}

void EvaluateScriptTest::testReturn()
{
    auto doc = new KTextEditor::DocumentPrivate();
    auto view = doc->createView(nullptr);
    doc->setText(QStringLiteral("line 1\nline 2\n"));
    auto cursor = KTextEditor::Cursor(1, 3);
    view->setCursorPosition(cursor);

    QVariant result;
    bool success = view->evaluateScript(QStringLiteral("return view.cursorPosition()"), &result);
    QVERIFY(success);
    auto map = result.toMap();
    QCOMPARE(map.size(), 2);
    QCOMPARE(map.value(QStringLiteral("line")).toInt(), cursor.line());
    QCOMPARE(map.value(QStringLiteral("column")).toInt(), cursor.column());

    success = view->evaluateScript(QStringLiteral("return ['a', 'b', 'c']"), &result);
    QVERIFY(success);
    auto list = result.toList();
    QCOMPARE(list.size(), 3);
    QCOMPARE(list.value(2).toString(), QStringLiteral("c"));
}

#include "moc_evaluate_script_test.cpp"
