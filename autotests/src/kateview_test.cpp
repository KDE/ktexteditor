/* This file is part of the KDE libraries
   Copyright (C) 2010 Milian Wolff <mail@milianw.de>

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

#include "kateview_test.h"
#include "moc_kateview_test.cpp"

#include <kateglobal.h>
#include <katedocument.h>
#include <kateview.h>
#include <ktexteditor/movingcursor.h>
#include <kateconfig.h>
#include <katebuffer.h>
#include <ktexteditor/message.h>

#include <QtTestWidgets>
#include <QTemporaryFile>

using namespace KTextEditor;

QTEST_MAIN(KateViewTest)

KateViewTest::KateViewTest()
    : QObject()
{
    KTextEditor::EditorPrivate::enableUnitTestMode();
}

KateViewTest::~KateViewTest()
{
}

void KateViewTest::testCoordinatesToCursor()
{
    KTextEditor::DocumentPrivate doc(false, false);
    doc.setText("Hi World!\nHi\n");

    KTextEditor::View* view1 = static_cast<KTextEditor::View*>(doc.createView(nullptr));
    view1->resize(400, 300);
    view1->show();

    QCOMPARE(view1->coordinatesToCursor(view1->cursorToCoordinate(KTextEditor::Cursor(0, 2))),
             KTextEditor::Cursor(0, 2));
    QCOMPARE(view1->coordinatesToCursor(view1->cursorToCoordinate(KTextEditor::Cursor(1, 1))),
             KTextEditor::Cursor(1, 1));
    // behind end of line should give an invalid cursor
    QCOMPARE(view1->coordinatesToCursor(view1->cursorToCoordinate(KTextEditor::Cursor(1, 5))),
             KTextEditor::Cursor::invalid());
    QCOMPARE(view1->cursorToCoordinate(KTextEditor::Cursor(3, 1)), QPoint(-1, -1));

    // check consistency between cursorToCoordinate(view->cursorPosition() and cursorPositionCoordinates()
    // random position
    view1->setCursorPosition(KTextEditor::Cursor(0, 3));
    QCOMPARE(view1->coordinatesToCursor(view1->cursorToCoordinate(view1->cursorPosition())),
             KTextEditor::Cursor(0, 3));
    QCOMPARE(view1->coordinatesToCursor(view1->cursorPositionCoordinates()), KTextEditor::Cursor(0, 3));
    // end of line
    view1->setCursorPosition(KTextEditor::Cursor(0, 9));
    QCOMPARE(view1->coordinatesToCursor(view1->cursorToCoordinate(KTextEditor::Cursor(0, 9))),
             KTextEditor::Cursor(0, 9));
    QCOMPARE(view1->coordinatesToCursor(view1->cursorPositionCoordinates()), KTextEditor::Cursor(0, 9));
    // empty line
    view1->setCursorPosition(KTextEditor::Cursor(2, 0));
    QCOMPARE(view1->coordinatesToCursor(view1->cursorToCoordinate(KTextEditor::Cursor(2, 0))),
             KTextEditor::Cursor(2, 0));
    QCOMPARE(view1->coordinatesToCursor(view1->cursorPositionCoordinates()), KTextEditor::Cursor(2, 0));

    // same test again, but with message widget on top visible
    KTextEditor::Message *message = new KTextEditor::Message("Jo World!", KTextEditor::Message::Information);
    doc.postMessage(message);

    // wait 500ms until show animation is finished, so the message widget is visible
    QTest::qWait(500);

    QCOMPARE(view1->coordinatesToCursor(view1->cursorToCoordinate(KTextEditor::Cursor(0, 2))),
             KTextEditor::Cursor(0, 2));
    QCOMPARE(view1->coordinatesToCursor(view1->cursorToCoordinate(KTextEditor::Cursor(1, 1))),
             KTextEditor::Cursor(1, 1));
    // behind end of line should give an invalid cursor
    QCOMPARE(view1->coordinatesToCursor(view1->cursorToCoordinate(KTextEditor::Cursor(1, 5))),
             KTextEditor::Cursor::invalid());
    QCOMPARE(view1->cursorToCoordinate(KTextEditor::Cursor(3, 1)), QPoint(-1, -1));
}

void KateViewTest::testCursorToCoordinates()
{
    KTextEditor::DocumentPrivate doc(false, false);
    doc.setText("int a;");

    KTextEditor::ViewPrivate *view = new KTextEditor::ViewPrivate(&doc, nullptr);
    view->config()->setDynWordWrap(true);
    view->show();

    // don't crash, see https://bugs.kde.org/show_bug.cgi?id=337863
    view->cursorToCoordinate(Cursor(0, 0));
    view->cursorToCoordinate(Cursor(1, 0));
    view->cursorToCoordinate(Cursor(-1, 0));
}

void KateViewTest::testReloadMultipleViews()
{
    QTemporaryFile file("XXXXXX.cpp");
    file.open();
    QTextStream stream(&file);
    const QString line = "const char* foo = \"asdf\"\n";
    for (int i = 0; i < 200; ++i) {
        stream << line;
    }
    stream << flush;
    file.close();

    KTextEditor::DocumentPrivate doc;
    QVERIFY(doc.openUrl(QUrl::fromLocalFile(file.fileName())));
    QCOMPARE(doc.highlightingMode(), QString("C++"));

    KTextEditor::ViewPrivate *view1 = new KTextEditor::ViewPrivate(&doc, nullptr);
    KTextEditor::ViewPrivate *view2 = new KTextEditor::ViewPrivate(&doc, nullptr);
    view1->show();
    view2->show();
    QCOMPARE(doc.views().count(), 2);

    QVERIFY(doc.documentReload());
}

void KateViewTest::testTabCursorOnReload()
{
    // testcase for https://bugs.kde.org/show_bug.cgi?id=258480
    QTemporaryFile file("XXXXXX.cpp");
    file.open();
    QTextStream stream(&file);
    stream << "\tfoo\n";
    file.close();

    KTextEditor::DocumentPrivate doc;
    QVERIFY(doc.openUrl(QUrl::fromLocalFile(file.fileName())));

    KTextEditor::ViewPrivate *view = new KTextEditor::ViewPrivate(&doc, nullptr);
    const KTextEditor::Cursor cursor(0, 4);
    view->setCursorPosition(cursor);
    QCOMPARE(view->cursorPosition(), cursor);
    QVERIFY(doc.documentReload());
    QCOMPARE(view->cursorPosition(), cursor);
}

void KateViewTest::testLowerCaseBlockSelection()
{
    // testcase for https://bugs.kde.org/show_bug.cgi?id=258480
    KTextEditor::DocumentPrivate doc;
    doc.setText("nY\nnYY\n");

    KTextEditor::ViewPrivate *view1 = new KTextEditor::ViewPrivate(&doc, nullptr);
    view1->setBlockSelection(true);
    view1->setSelection(Range(0, 1, 1, 3));
    view1->lowercase();

    QCOMPARE(doc.text(), QString("ny\nnyy\n"));
}

namespace
{
    QWidget *findViewInternal(KTextEditor::View* view)
    {
        foreach (QObject* child, view->children()) {
            if (child->metaObject()->className() == QByteArrayLiteral("KateViewInternal")) {
                return qobject_cast<QWidget*>(child);
            }
        }
        return nullptr;
    }
}

void KateViewTest::testSelection()
{
    // see also: https://bugs.kde.org/show_bug.cgi?id=277422
    // wrong behavior before:
    // Open file with text
    // click at end of some line (A) and drag to right, i.e. without selecting anything
    // click somewhere else (B)
    // shift click to another place (C)
    // => expected: selection from B to C
    // => actual: selection from A to C

    QTemporaryFile file("XXXXXX.txt");
    file.open();
    QTextStream stream(&file);
    stream << "A\n"
           << "B\n"
           << "C";
    stream << flush;
    file.close();

    KTextEditor::DocumentPrivate doc;
    QVERIFY(doc.openUrl(QUrl::fromLocalFile(file.fileName())));

    KTextEditor::ViewPrivate *view = new KTextEditor::ViewPrivate(&doc, nullptr);
    view->resize(100, 200);
    view->show();


    QObject *internalView = findViewInternal(view);
    QVERIFY(internalView);

    const QPoint afterA = view->cursorToCoordinate(Cursor(0, 1));
    const QPoint afterB = view->cursorToCoordinate(Cursor(1, 1));
    const QPoint afterC = view->cursorToCoordinate(Cursor(2, 1));

    // click after A
    QCoreApplication::sendEvent(internalView, new QMouseEvent(QEvent::MouseButtonPress, afterA,
                                Qt::LeftButton, Qt::LeftButton,
                                Qt::NoModifier));

    QCoreApplication::sendEvent(internalView, new QMouseEvent(QEvent::MouseButtonRelease, afterA,
                                Qt::LeftButton, Qt::LeftButton,
                                Qt::NoModifier));
    QCOMPARE(view->cursorPosition(), Cursor(0, 1));
    // drag to right
    QCoreApplication::sendEvent(internalView, new QMouseEvent(QEvent::MouseButtonPress, afterA,
                                Qt::LeftButton, Qt::LeftButton,
                                Qt::NoModifier));

    QCoreApplication::sendEvent(internalView, new QMouseEvent(QEvent::MouseMove, afterA + QPoint(50, 0),
                                Qt::LeftButton, Qt::LeftButton,
                                Qt::NoModifier));

    QCoreApplication::sendEvent(internalView, new QMouseEvent(QEvent::MouseButtonRelease, afterA + QPoint(50, 0),
                                Qt::LeftButton, Qt::LeftButton,
                                Qt::NoModifier));

    QCOMPARE(view->cursorPosition(), Cursor(0, 1));
    QVERIFY(!view->selection());

    // click after C
    QCoreApplication::sendEvent(internalView, new QMouseEvent(QEvent::MouseButtonPress, afterC,
                                Qt::LeftButton, Qt::LeftButton,
                                Qt::NoModifier));

    QCoreApplication::sendEvent(internalView, new QMouseEvent(QEvent::MouseButtonRelease, afterC,
                                Qt::LeftButton, Qt::LeftButton,
                                Qt::NoModifier));

    QCOMPARE(view->cursorPosition(), Cursor(2, 1));
    // shift+click after B
    QCoreApplication::sendEvent(internalView, new QMouseEvent(QEvent::MouseButtonPress, afterB,
                                Qt::LeftButton, Qt::LeftButton,
                                Qt::ShiftModifier));

    QCoreApplication::sendEvent(internalView, new QMouseEvent(QEvent::MouseButtonRelease, afterB,
                                Qt::LeftButton, Qt::LeftButton,
                                Qt::ShiftModifier));

    QCOMPARE(view->cursorPosition(), Cursor(1, 1));
    QCOMPARE(view->selectionRange(), Range(1, 1, 2, 1));
}

void KateViewTest::testKillline()
{
    KTextEditor::DocumentPrivate doc;
    doc.insertLines(0, { "foo", "bar", "baz" });

    KTextEditor::ViewPrivate *view = new KTextEditor::ViewPrivate(&doc, nullptr);

    view->setCursorPositionInternal(KTextEditor::Cursor(1, 2));
    view->killLine();

    QCOMPARE(doc.text(), QLatin1String("foo\nbaz\n"));

    doc.clear();
    QVERIFY(doc.isEmpty());

    doc.insertLines(0, { "foo", "bar", "baz", "xxx" });

    view->setCursorPositionInternal(KTextEditor::Cursor(1, 2));
    view->shiftDown();
    view->killLine();

    QCOMPARE(doc.text(), QLatin1String("foo\nxxx\n"));
}

void KateViewTest::testFoldFirstLine()
{
    QTemporaryFile file("XXXXXX.cpp");
    file.open();
    QTextStream stream(&file);
    stream << "/**\n"
           << " * foo\n"
           << " */\n"
           << "\n"
           << "int main() {}\n";
    file.close();

    KTextEditor::DocumentPrivate doc;
    QVERIFY(doc.openUrl(QUrl::fromLocalFile(file.fileName())));
    QCOMPARE(doc.highlightingMode(), QString("C++"));

    KTextEditor::ViewPrivate *view = new KTextEditor::ViewPrivate(&doc, nullptr);
    view->config()->setFoldFirstLine(false);
    view->setCursorPosition({4, 0});

    // initially, nothing is folded
    QVERIFY(view->textFolding().isLineVisible(1));

    // now change the config, and expect the header to be folded
    view->config()->setFoldFirstLine(true);
    qint64 foldedRangeId = 0;
    QVERIFY(!view->textFolding().isLineVisible(1, &foldedRangeId));

    // now unfold the range
    QVERIFY(view->textFolding().unfoldRange(foldedRangeId));
    QVERIFY(view->textFolding().isLineVisible(1));

    // and save the file, we do not expect the folding to change then
    doc.setModified(true);
    doc.saveFile();
    QVERIFY(view->textFolding().isLineVisible(1));

    // now reload the document, nothing should change
    doc.setModified(false);
    QVERIFY(doc.documentReload());
    QVERIFY(view->textFolding().isLineVisible(1));
}

// test for bug https://bugs.kde.org/374163
void KateViewTest::testDragAndDrop()
{
    KTextEditor::DocumentPrivate doc(false, false);
    doc.setText("line0\n"
                "line1\n"
                "line2\n"
                "\n"
                "line4");

    KTextEditor::View* view = static_cast<KTextEditor::View*>(doc.createView(nullptr));
    view->show();
    view->resize(400, 300);

    QWidget *internalView = findViewInternal(view);
    QVERIFY(internalView);

    // select "line1\n"
    view->setSelection(Range(1, 0, 2, 0));
    QCOMPARE(view->selectionRange(), Range(1, 0, 2, 0));

    QVERIFY(QTest::qWaitForWindowExposed(view));
    const QPoint startDragPos = internalView->mapFrom(view, view->cursorToCoordinate(KTextEditor::Cursor(1, 2)));
    const QPoint endDragPos = internalView->mapFrom(view, view->cursorToCoordinate(KTextEditor::Cursor(3, 0)));
    const QPoint gStartDragPos = internalView->mapToGlobal(startDragPos);
    const QPoint gEndDragPos = internalView->mapToGlobal(endDragPos);

    // now drag and drop selected text to Cursor(3, 0)
    QMouseEvent pressEvent(QEvent::MouseButtonPress, startDragPos, gStartDragPos,
                                Qt::LeftButton, Qt::LeftButton, Qt::NoModifier);
    QCoreApplication::sendEvent(internalView, &pressEvent);

    // ugly workaround: Drag & Drop has own blocking event queue. Therefore, we need a single-shot timer to
    // break out of the blocking event queue, see (*)
    QTimer::singleShot(50, [&](){
        QMouseEvent moveEvent(QEvent::MouseMove, endDragPos + QPoint(5, 0), gEndDragPos + QPoint(5, 0),
                            Qt::LeftButton, Qt::LeftButton, Qt::NoModifier);
        QMouseEvent releaseEvent(QEvent::MouseButtonRelease, endDragPos, gEndDragPos,
                            Qt::LeftButton, Qt::NoButton, Qt::NoModifier);
        QCoreApplication::sendEvent(internalView, &moveEvent);
        QCoreApplication::sendEvent(internalView, &releaseEvent);
    });

    // (*) this somehow blocks...
    QMouseEvent moveEvent1(QEvent::MouseMove, endDragPos + QPoint(10, 0), gEndDragPos + QPoint(10, 0),
                           Qt::LeftButton, Qt::LeftButton, Qt::NoModifier);
    QCoreApplication::sendEvent(internalView, &moveEvent1);

    QTest::qWait(100);

    // final tests of dragged text
    QCOMPARE(doc.text(), QString("line0\n"
                                 "line2\n"
                                 "line1\n"
                                 "\n"
                                 "line4"));

    QCOMPARE(view->cursorPosition(), KTextEditor::Cursor(3, 0));
    QCOMPARE(view->selectionRange(), Range(2, 0, 3, 0));
}

// kate: indent-mode cstyle; indent-width 4; replace-tabs on;
