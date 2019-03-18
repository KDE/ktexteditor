/* This file is part of the KDE project
 *
 *  Copyright 2019 Dominik Haumann <dhaumann@kde.org>
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Library General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Library General Public License for more details.
 *
 *  You should have received a copy of the GNU Library General Public License
 *  along with this library; see the file COPYING.LIB.  If not, write to
 *  the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 *  Boston, MA 02110-1301, USA.
 */
#include "variable_test.h"
#include "moc_variable_test.cpp"

#include <kateglobal.h>
#include <ktexteditor/editor.h>
#include <ktexteditor/document.h>
#include <ktexteditor/view.h>

#include <QtTest>

using namespace KTextEditor;

QTEST_MAIN(VariableTest)

VariableTest::VariableTest()
    : QObject()
{
    KTextEditor::EditorPrivate::enableUnitTestMode();
}

VariableTest::~VariableTest()
{
}

void VariableTest::testReturnValues()
{
    auto editor = KTextEditor::Editor::instance();

    const QString name = QStringLiteral("Document:");
    auto func = [](const QStringView&, KTextEditor::View*) { return QString(); };

    // exact matches
    QVERIFY(!editor->unregisterVariableMatch(name));
    QVERIFY(editor->registerVariableMatch(name, "Document Text", func));
    QVERIFY(!editor->registerVariableMatch(name, "Document Text", func));
    QVERIFY(editor->unregisterVariableMatch(name));
    QVERIFY(!editor->unregisterVariableMatch(name));

    // prefix matches
    QVERIFY(!editor->unregisterVariablePrefix(name));
    QVERIFY(editor->registerVariablePrefix(name, "Document Text", func));
    QVERIFY(!editor->registerVariablePrefix(name, "Document Text", func));
    QVERIFY(editor->unregisterVariablePrefix(name));
    QVERIFY(!editor->unregisterVariablePrefix(name));
}

void VariableTest::testExactMatch_data()
{
    QTest::addColumn<QString>("text");
    QTest::addColumn<QString>("expected");
    QTest::addColumn<QString>("expectedText");

    QTest::newRow("World") << "World" << "World" << "World";
    QTest::newRow("Smart World") << "Smart World" << "Smart World" << "Smart World";
}

void VariableTest::testExactMatch()
{
    QFETCH(QString, text);
    QFETCH(QString, expected);
    QFETCH(QString, expectedText);

    auto editor = KTextEditor::Editor::instance();
    auto doc = editor->createDocument(nullptr);
    auto view = doc->createView(nullptr);
    doc->setText(text);

    const QString name = QStringLiteral("Doc:Text");
    auto func = [](const QStringView&, KTextEditor::View* view) {
        return view->document()->text();
    };

    QVERIFY(editor->registerVariableMatch(name, "Document Text", func));

    // expandVariable
    QString output;
    QVERIFY(editor->expandVariable(QStringLiteral("Doc:Text"), view, output));
    QCOMPARE(output, expected);

    // expandText
    editor->expandText(QStringLiteral("Hello %{Doc:Text}!"), view, output);
    QCOMPARE(output, QStringLiteral("Hello ") + expectedText + QLatin1Char('!'));

    editor->expandText(QStringLiteral("Hello %{Doc:Text} %{Doc:Text}!"), view, output);
    QCOMPARE(output, QStringLiteral("Hello ") + expectedText + QLatin1Char(' ') + expectedText + QLatin1Char('!'));

    QVERIFY(editor->unregisterVariableMatch("Doc:Text"));

    delete doc;
}

void VariableTest::testPrefixMatch()
{
    auto editor = KTextEditor::Editor::instance();

    const QString prefix = QStringLiteral("Mirror:");
    auto func = [](const QStringView& text, KTextEditor::View*) {
        QStringView rest = text.right(text.size() - 7);
        QString out;
        for (auto it = rest.rbegin(); it != rest.rend(); ++it) {
            out += *it;
        }
        return out;
    };

    QVERIFY(editor->registerVariablePrefix(prefix, "Reverse text", func));

    QString output;
    QVERIFY(editor->expandVariable(QStringLiteral("Mirror:12345"), nullptr, output));
    QCOMPARE(output, QStringLiteral("54321"));

    editor->expandText(QStringLiteral("Countdown: %{Mirror:12345}"), nullptr, output);
    QCOMPARE(output, QStringLiteral("Countdown: 54321"));

    // Test recursive expansion
    editor->expandText(QStringLiteral("Countup: %{Mirror:%{Mirror:12345}}"), nullptr, output);
    QCOMPARE(output, QStringLiteral("Countup: 12345"));

    QVERIFY(editor->unregisterVariablePrefix(prefix));
}

void VariableTest::testRecursiveMatch()
{
    auto editor = KTextEditor::Editor::instance();
    auto doc = editor->createDocument(nullptr);
    auto view = doc->createView(nullptr);
    doc->setText(QStringLiteral("Text"));

    const QString name = QStringLiteral("Doc:Text");
    auto func = [](const QStringView&, KTextEditor::View* view) {
        return view->document()->text();
    };
    QVERIFY(editor->registerVariableMatch(name, "Document Text", func));

    // Test recursive expansion
    doc->setText(QStringLiteral("Text"));
    QString output;
    editor->expandText(QStringLiteral("Hello %{Doc:%{Doc:Text}}!"), view, output);
    QCOMPARE(output, QStringLiteral("Hello Text!"));

    QVERIFY(editor->unregisterVariableMatch(name));
    delete doc;
}

// kate: indent-mode cstyle; indent-width 4; replace-tabs on;
