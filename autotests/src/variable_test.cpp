/*
    This file is part of the KDE project
    SPDX-FileCopyrightText: 2019 Dominik Haumann <dhaumann@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "variable_test.h"
#include "moc_variable_test.cpp"

#include <katedocument.h>
#include <kateglobal.h>
#include <ktexteditor/document.h>
#include <ktexteditor/editor.h>
#include <ktexteditor/view.h>

#include <QTest>
#include <QUuid>

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
    auto func = [](const QStringView &, KTextEditor::View *) {
        return QString();
    };

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

    QTest::newRow("World") << "World"
                           << "World"
                           << "World";
    QTest::newRow("Smart World") << "Smart World"
                                 << "Smart World"
                                 << "Smart World";
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
    auto func = [](const QStringView &, KTextEditor::View *view) {
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
    auto func = [](const QStringView &text, KTextEditor::View *) {
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
    auto func = [](const QStringView &, KTextEditor::View *view) {
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

void VariableTest::testBuiltins()
{
    auto editor = KTextEditor::Editor::instance();
    auto doc = editor->createDocument(nullptr);
    doc->openUrl(QUrl::fromLocalFile(QDir::homePath() + QStringLiteral("/kate-v5.tar.gz")));
    doc->setText(QStringLiteral("get an edge in editing\n:-)"));
    auto view = doc->createView(nullptr);
    view->setCursorPosition(KTextEditor::Cursor(1, 2));
    view->show();

    QString out;

    // Test invalid ones:
    editor->expandText(QStringLiteral("%{}"), view, out);
    QCOMPARE(out, QStringLiteral("%{}"));
    editor->expandText(QStringLiteral("%{"), view, out);
    QCOMPARE(out, QStringLiteral("%{"));
    editor->expandText(QStringLiteral("%{{}"), view, out);
    QCOMPARE(out, QStringLiteral("%{{}"));
    editor->expandText(QStringLiteral("%{{}}"), view, out);
    QCOMPARE(out, QStringLiteral("%{{}}"));

    // Document:FileBaseName
    editor->expandText(QStringLiteral("%{Document:FileBaseName}"), view, out);
    QCOMPARE(out, QStringLiteral("kate-v5"));

    // Document:FileExtension
    editor->expandText(QStringLiteral("%{Document:FileExtension}"), view, out);
    QCOMPARE(out, QStringLiteral("tar.gz"));

    // Document:FileName
    editor->expandText(QStringLiteral("%{Document:FileName}"), view, out);
    QCOMPARE(out, QStringLiteral("kate-v5.tar.gz"));

    // Document:FilePath
    editor->expandText(QStringLiteral("%{Document:FilePath}"), view, out);
    QCOMPARE(out, QFileInfo(view->document()->url().toLocalFile()).absoluteFilePath());

    // Document:Text
    editor->expandText(QStringLiteral("%{Document:Text}"), view, out);
    QCOMPARE(out, QStringLiteral("get an edge in editing\n:-)"));

    // Document:Path
    editor->expandText(QStringLiteral("%{Document:Path}"), view, out);
    QCOMPARE(out, QFileInfo(doc->url().toLocalFile()).absolutePath());

    // Document:NativeFilePath
    editor->expandText(QStringLiteral("%{Document:NativeFilePath}"), view, out);
    QCOMPARE(out, QDir::toNativeSeparators(QFileInfo(doc->url().toLocalFile()).absoluteFilePath()));

    // Document:NativePath
    editor->expandText(QStringLiteral("%{Document:NativePath}"), view, out);
    QCOMPARE(out, QDir::toNativeSeparators(QFileInfo(doc->url().toLocalFile()).absolutePath()));

    // Document:NativePath
    editor->expandText(QStringLiteral("%{Document:NativePath}"), view, out);
    QCOMPARE(out, QDir::toNativeSeparators(QFileInfo(doc->url().toLocalFile()).absolutePath()));

    // Document:Cursor:Line
    editor->expandText(QStringLiteral("%{Document:Cursor:Line}"), view, out);
    QCOMPARE(out, QStringLiteral("1"));

    // Document:Cursor:Column
    editor->expandText(QStringLiteral("%{Document:Cursor:Column}"), view, out);
    QCOMPARE(out, QStringLiteral("2"));

    // Document:Cursor:XPos
    editor->expandText(QStringLiteral("%{Document:Cursor:XPos}"), view, out);
    QVERIFY(out.toInt() > 0);

    // Document:Cursor:YPos
    editor->expandText(QStringLiteral("%{Document:Cursor:YPos}"), view, out);
    QVERIFY(out.toInt() > 0);

    view->setSelection(KTextEditor::Range(1, 0, 1, 3));
    // Document:Selection:Text
    editor->expandText(QStringLiteral("%{Document:Selection:Text}"), view, out);
    QCOMPARE(out, QStringLiteral(":-)"));

    // Document:Selection:StartLine
    editor->expandText(QStringLiteral("%{Document:Selection:StartLine}"), view, out);
    QCOMPARE(out, QStringLiteral("1"));

    // Document:Selection:StartColumn
    editor->expandText(QStringLiteral("%{Document:Selection:StartColumn}"), view, out);
    QCOMPARE(out, QStringLiteral("0"));

    // Document:Selection:EndLine
    editor->expandText(QStringLiteral("%{Document:Selection:EndLine}"), view, out);
    QCOMPARE(out, QStringLiteral("1"));

    // Document:Selection:EndColumn
    editor->expandText(QStringLiteral("%{Document:Selection:EndColumn}"), view, out);
    QCOMPARE(out, QStringLiteral("3"));

    // Document:RowCount
    editor->expandText(QStringLiteral("%{Document:RowCount}"), view, out);
    QCOMPARE(out, QStringLiteral("2"));

    // Document:Variable:<variable>, since KF 5.78
    qobject_cast<KTextEditor::DocumentPrivate *>(doc)->setVariable(QStringLiteral("cow-sound"), QStringLiteral("moo"));
    editor->expandText(QStringLiteral("%{Document:Variable:cow-sound}"), view, out);
    QCOMPARE(out, QStringLiteral("moo"));

    // Date:Locale
    editor->expandText(QStringLiteral("%{Date:Locale}"), view, out);
    QVERIFY(!out.isEmpty());

    // Date:ISO
    editor->expandText(QStringLiteral("%{Date:ISO}"), view, out);
    QVERIFY(!out.isEmpty());

    // Date:yyyy-MM-dd
    editor->expandText(QStringLiteral("%{Date:yyyy-MM-dd}"), view, out);
    QVERIFY(QDate::fromString(out, QStringLiteral("yyyy-MM-dd")).isValid());

    // Time:Locale
    editor->expandText(QStringLiteral("%{Time:Locale}"), view, out);
    QVERIFY(!out.isEmpty());

    // Time:ISO
    editor->expandText(QStringLiteral("%{Time:ISO}"), view, out);
    QVERIFY(!out.isEmpty());

    // Time:hh-mm-ss
    editor->expandText(QStringLiteral("%{Time:hh-mm-ss}"), view, out);
    QVERIFY(QTime::fromString(out, QStringLiteral("hh-mm-ss")).isValid());

    // ENV:KTE_ENV_VAR_TEST
    qputenv("KTE_ENV_VAR_TEST", "KTE_ENV_VAR_TEST_VALUE");
    editor->expandText(QStringLiteral("%{ENV:KTE_ENV_VAR_TEST}"), view, out);
    QCOMPARE(out, QStringLiteral("KTE_ENV_VAR_TEST_VALUE"));

    // JS:<code>
    editor->expandText(QStringLiteral("%{JS:3 + %{JS:2 + 1}}"), view, out);
    QCOMPARE(out, QStringLiteral("6"));

    // PercentEncoded: since 5.67
    editor->expandText(QStringLiteral("%{PercentEncoded:{a&b+c=d} \"}"), view, out);
    QCOMPARE(out, QStringLiteral("%7Ba%26b%2Bc%3Dd%7D%20%22"));

    // UUID
    editor->expandText(QStringLiteral("%{UUID}"), view, out);
    QCOMPARE(out.count(QLatin1Char('-')), 4);
}

// kate: indent-mode cstyle; indent-width 4; replace-tabs on;
