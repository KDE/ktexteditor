/* This file is part of the KDE libraries
 *   Copyright (C) 2015 Zoe Clifford <zoeacacia@gmail.com>
 *
 *   This library is free software; you can redistribute it and/or
 *   modify it under the terms of the GNU Library General Public
 *   License as published by the Free Software Foundation; either
 *   version 2 of the License, or (at your option) any later version.
 *
 *   This library is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *   Library General Public License for more details.
 *
 *   You should have received a copy of the GNU Library General Public License
 *   along with this library; see the file COPYING.LIB.  If not, write to
 *   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 *   Boston, MA 02110-1301, USA.
 */

#include "bug205447.h"

#include <katedocument.h>
#include <kateglobal.h>
#include <kateview.h>
#include <kmainwindow.h>
#include <ktexteditor/documentcursor.h>

#include <QtScript/QScriptEngine>
#include <QtTestWidgets>

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
    KTextEditor::EditorPrivate::enableUnitTestMode();
}

void BugTest::cleanupTestCase()
{
}

void BugTest::deleteSurrogates()
{
    // set up document and view and open test file
    KTextEditor::DocumentPrivate doc;
    KTextEditor::ViewPrivate *view = static_cast<KTextEditor::ViewPrivate *>(doc.createView(nullptr));
    const QUrl url = QUrl::fromLocalFile(QLatin1String(TEST_DATA_DIR"bug205447.txt"));
    doc.setEncoding(QStringLiteral("UTF-8"));
    QVERIFY(doc.openUrl(url));

    // test delete
    // get UTF-32 representation of original line (before any deletes)
    QVector<uint> line = doc.line(0).toUcs4();
    QVERIFY(line.size() == 23);
    // delete from start of line
    view->setCursorPosition(Cursor(0, 0));
    QVERIFY(DocumentCursor(&doc, view->cursorPosition()).isValidTextPosition());
    for (int i = 0; i < line.size(); i++) {
        // get the current line, after `i` delete presses, and convert it to utf32
        // then ensure it's the expected substring of the original line
        QVector<uint> current = doc.line(0).toUcs4();
        QCOMPARE(current, line.mid(i));

        // press the delete key and verify that the new text position isn't invalid
        view->keyDelete();
        QVERIFY(DocumentCursor(&doc, view->cursorPosition()).isValidTextPosition());
    }
    QCOMPARE(doc.lineLength(0), 0);
}

void BugTest::backspaceSurrogates()
{
    // set up document and view and open test file
    KTextEditor::DocumentPrivate doc;
    KTextEditor::ViewPrivate *view = static_cast<KTextEditor::ViewPrivate *>(doc.createView(nullptr));
    const QUrl url = QUrl::fromLocalFile(QLatin1String(TEST_DATA_DIR"bug205447.txt"));
    doc.setEncoding(QStringLiteral("UTF-8"));
    QVERIFY(doc.openUrl(url));

    // test backspace
    // get UTF-32 representation of original line (before any backspaces)
    QVector<uint> line = doc.line(0).toUcs4();
    QVERIFY(line.size() == 23);
    // backspace from end of line
    view->setCursorPosition(Cursor(0, doc.line(0).size()));
    QVERIFY(DocumentCursor(&doc, view->cursorPosition()).isValidTextPosition());
    for (int i = 0; i < line.size(); i++) {
        // get the current line, after `i` delete presses, and convert it to utf32
        // then ensure it's the expected substring of the original line
        QVector<uint> current = doc.line(0).toUcs4();
        QCOMPARE(current, line.mid(0, line.size()-i));

        // press the backspace key and verify that the new text position isn't invalid
        view->backspace();
        QVERIFY(DocumentCursor(&doc, view->cursorPosition()).isValidTextPosition());
    }
    QCOMPARE(doc.lineLength(0), 0);
}

#include "moc_bug205447.cpp"
