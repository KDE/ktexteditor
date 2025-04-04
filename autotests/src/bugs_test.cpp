/*
    This file is part of the KDE libraries
    SPDX-FileCopyrightText: 2015 Zoe Clifford <zoeacacia@gmail.com>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "bugs_test.h"

#include <katebuffer.h>
#include <kateconfig.h>
#include <katedocument.h>
#include <kateview.h>
#include <kmainwindow.h>
#include <ktexteditor/documentcursor.h>

#include <QStandardPaths>
#include <QTest>

#include "testutils.h"

QTEST_MAIN(BugTest)

using namespace KTextEditor;

BugTest::BugTest()
    : QObject()
{
}

BugTest::~BugTest()
{
}

void BugTest::initTestCase()
{
    QStandardPaths::setTestModeEnabled(true);
}

void BugTest::cleanupTestCase()
{
}

void BugTest::testBug205447DeleteSurrogates()
{
    // set up document and view and open test file
    KTextEditor::DocumentPrivate doc;
    KTextEditor::ViewPrivate *view = static_cast<KTextEditor::ViewPrivate *>(doc.createView(nullptr));
    const QUrl url = QUrl::fromLocalFile(QLatin1String(TEST_DATA_DIR "bug205447.txt"));
    doc.setEncoding(QStringLiteral("UTF-8"));
    QVERIFY(doc.openUrl(url));

    // test delete
    // get UTF-32 representation of original line (before any deletes)
    QList<uint> line = doc.line(0).toUcs4();
    QVERIFY(line.size() == 23);
    // delete from start of line
    view->setCursorPosition(Cursor(0, 0));
    QVERIFY(DocumentCursor(&doc, view->cursorPosition()).isValidTextPosition());
    for (int i = 0; i < line.size(); i++) {
        // get the current line, after `i` delete presses, and convert it to utf32
        // then ensure it's the expected substring of the original line
        QList<uint> current = doc.line(0).toUcs4();
        QCOMPARE(current, line.mid(i));

        // press the delete key and verify that the new text position isn't invalid
        view->keyDelete();
        QVERIFY(DocumentCursor(&doc, view->cursorPosition()).isValidTextPosition());
    }
    QCOMPARE(doc.lineLength(0), 0);
}

void BugTest::testBug205447BackspaceSurrogates()
{
    // set up document and view and open test file
    KTextEditor::DocumentPrivate doc;
    KTextEditor::ViewPrivate *view = static_cast<KTextEditor::ViewPrivate *>(doc.createView(nullptr));
    const QUrl url = QUrl::fromLocalFile(QLatin1String(TEST_DATA_DIR "bug205447.txt"));
    doc.setEncoding(QStringLiteral("UTF-8"));
    QVERIFY(doc.openUrl(url));

    // test backspace
    // get UTF-32 representation of original line (before any backspaces)
    QList<uint> line = doc.line(0).toUcs4();
    QVERIFY(line.size() == 23);
    // backspace from end of line
    view->setCursorPosition(Cursor(0, doc.line(0).size()));
    QVERIFY(DocumentCursor(&doc, view->cursorPosition()).isValidTextPosition());
    for (int i = 0; i < line.size(); i++) {
        // get the current line, after `i` delete presses, and convert it to utf32
        // then ensure it's the expected substring of the original line
        QList<uint> current = doc.line(0).toUcs4();
        QCOMPARE(current, line.mid(0, line.size() - i));

        // press the backspace key and verify that the new text position isn't invalid
        view->backspace();
        QVERIFY(DocumentCursor(&doc, view->cursorPosition()).isValidTextPosition());
    }
    QCOMPARE(doc.lineLength(0), 0);
}

void BugTest::testBug286887CtrlShiftLeft()
{
    KTextEditor::DocumentPrivate doc(false, false);

    // view must be visible...
    KTextEditor::ViewPrivate *view = static_cast<KTextEditor::ViewPrivate *>(doc.createView(nullptr));
    view->show();
    view->resize(400, 300);

    // enable block mode, then set cursor after last character, then shift+left
    doc.clear();
    view->setBlockSelection(true);
    view->setCursorPosition(Cursor(0, 2));
    view->shiftCursorLeft();

    // enable block mode, then set cursor after last character, then delete word left
    doc.clear();
    view->setBlockSelection(true);
    view->setCursorPosition(Cursor(0, 2));
    view->deleteWordLeft();

    // disable wrap-cursor, then set cursor after last character, then shift+left
    doc.clear();
    view->setBlockSelection(false);
    view->setCursorPosition(Cursor(0, 2));
    view->shiftCursorLeft();

    // disable wrap-cursor, then set cursor after last character, then delete word left
    doc.clear();
    view->setCursorPosition(Cursor(0, 2));
    view->deleteWordLeft();
}

void BugTest::bug313759TryCrash()
{
    // set up document and view
    KMainWindow *toplevel = new KMainWindow();
    KTextEditor::DocumentPrivate *doc = new KTextEditor::DocumentPrivate(true, false, toplevel);
    KTextEditor::ViewPrivate *view = static_cast<KTextEditor::ViewPrivate *>(doc->createView(nullptr));
    bool outputWasCustomised = false;
    TestScriptEnv *env = new TestScriptEnv(doc, outputWasCustomised);
    const QUrl url = QUrl::fromLocalFile(QLatin1String(TEST_DATA_DIR "bug313759.txt"));
    doc->openUrl(url);

    // load moveLinesDown and moveLinesUp
    QFile scriptFile(QLatin1String(JS_DATA_DIR "commands/utils.js"));
    QVERIFY(scriptFile.exists());
    QVERIFY(scriptFile.open(QFile::ReadOnly));
    QJSValue result = env->engine()->evaluate(QString::fromLatin1(scriptFile.readAll()), scriptFile.fileName());
    QVERIFY2(!result.isError(), result.toString().toUtf8().constData());

    // enable on the fly spell checking
    doc->onTheFlySpellCheckingEnabled(true);

    // view must be visible...
    view->show();
    view->resize(900, 800);
    view->setCursorPosition(Cursor(0, 0));
    doc->editStart();

    // evaluate test-script
    qDebug() << "attempting crash by moving lines w/ otf spell checking enabled";
    QFile sourceFile(QLatin1String(TEST_DATA_DIR "bug313759.js"));
    QVERIFY(sourceFile.open(QFile::ReadOnly));
    QTextStream stream(&sourceFile);
    QString code = stream.readAll();
    sourceFile.close();
    // execute script
    result = env->engine()->evaluate(code, QLatin1String(TEST_DATA_DIR "bug313759.txt"), 1);
    QVERIFY2(!result.isError(), result.toString().toUtf8().constData());

    doc->editEnd();
    qDebug() << "PASS (no crash)";
}

void BugTest::bug313769TryCrash()
{
    KTextEditor::DocumentPrivate doc(false, false);
    const QUrl url = QUrl::fromLocalFile(QLatin1String(TEST_DATA_DIR "bug313769.cpp"));
    doc.openUrl(url);
    doc.discardDataRecovery();
    doc.setHighlightingMode(QStringLiteral("C++"));
    doc.buffer().ensureHighlighted(doc.lines());

    // view must be visible...
    KTextEditor::ViewPrivate *view = static_cast<KTextEditor::ViewPrivate *>(doc.createView(nullptr));
    view->show();
    view->resize(900, 800);
    view->config()->setDynWordWrap(true);
    view->setSelection(Range(2, 0, 74, 0));
    view->setCursorPosition(Cursor(74, 0));

    doc.editStart();
    QString text = doc.line(1);
    doc.insertLine(74, text);
    doc.removeLine(1);
    view->setCursorPosition(Cursor(1, 0));
    doc.editEnd();

    // fold toplevel nodes
    for (int line = 0; line < doc.lines(); ++line) {
        if (view->textFolding().isLineVisible(line)) {
            view->foldLine(line);
        }
    }
    doc.buffer().ensureHighlighted(doc.lines());

    view->setCursorPosition(Cursor(0, 0));

    doc.undo();
    doc.redo();
    doc.undo();
}

void BugTest::bug317111TryCrash()
{
    // set up document
    KTextEditor::DocumentPrivate doc(false, false);
    const QUrl url = QUrl::fromLocalFile(QLatin1String(TEST_DATA_DIR "bug313769.cpp"));
    QVERIFY(doc.openUrl(url));

    // try to crash with invalid line/column for defStyleNum
    doc.defStyleNum(10000000, 0);
    doc.defStyleNum(0, 10000000);
}

#include "moc_bugs_test.cpp"
