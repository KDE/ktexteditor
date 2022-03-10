/*
    This file is part of the KDE libraries
    SPDX-FileCopyrightText: 2022 Waqar Ahmed <waqar.17a@gmail.com>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "multicursortest.h"

#include <katebuffer.h>
#include <kateconfig.h>
#include <katedocument.h>
#include <kateglobal.h>
#include <kateview.h>
#include <kateviewinternal.h>
#include <ktexteditor/message.h>
#include <ktexteditor/movingcursor.h>

#include <QTest>

using namespace KTextEditor;

#define CREATE_VIEW_AND_DOC(text, line, col)                                          \
    KTextEditor::DocumentPrivate doc(false, false);                                   \
    doc.setText(text);                                                                \
    KTextEditor::ViewPrivate *view = new KTextEditor::ViewPrivate(&doc, nullptr);     \
    view->setCursorPosition({line, col})

QTEST_MAIN(MulticursorTest)

MulticursorTest::MulticursorTest()
    : QObject()
{
    KTextEditor::EditorPrivate::enableUnitTestMode();
}

MulticursorTest::~MulticursorTest()
{
}

static QWidget *findViewInternal(KTextEditor::View *view)
{
    for (QObject *child : view->children()) {
        if (child->metaObject()->className() == QByteArrayLiteral("KateViewInternal")) {
            return qobject_cast<QWidget *>(child);
        }
    }
    return nullptr;
}

static void clickAtPosition(ViewPrivate *view, QObject *internalView, Cursor pos, Qt::KeyboardModifiers m)
{
    QPoint p = view->cursorToCoordinate(pos);
    auto me = QMouseEvent(QEvent::MouseButtonPress, p, Qt::LeftButton, Qt::LeftButton, m);
    QCoreApplication::sendEvent(internalView, &me);
}

void MulticursorTest::testKillline()
{
    KTextEditor::DocumentPrivate doc;
    doc.insertLines(0, {"foo", "bar", "baz"});
    KTextEditor::ViewPrivate *view = new KTextEditor::ViewPrivate(&doc, nullptr);
    view->setCursorPositionInternal(KTextEditor::Cursor(0, 0));
    view->addSecondaryCursorAt(KTextEditor::Cursor(1, 0));
    view->addSecondaryCursorAt(KTextEditor::Cursor(2, 0));

    view->killLine();

    QCOMPARE(doc.text(), QString());
}

void MulticursorTest::testCreateMultiCursor()
{
    CREATE_VIEW_AND_DOC("foo\nbar\nfoo\n", 0, 0);

    QObject *internalView = findViewInternal(view);
    QVERIFY(internalView);


    { // Alt + click should add a cursor
        clickAtPosition(view, internalView, {1, 0}, Qt::AltModifier);

        const auto &cursors = view->secondaryMovingCursors();
        QCOMPARE(cursors.size(), 1);
        QCOMPARE(cursors.at(0)->toCursor(), KTextEditor::Cursor(1, 0));
    }

    { // Alt + click at the same point should remove the cursor
        clickAtPosition(view, internalView, {1, 0}, Qt::AltModifier);

        const auto &cursors = view->secondaryMovingCursors();
        QCOMPARE(cursors.size(), 0);
    }
}

void MulticursorTest::testCreateMultiCursorFromSelection()
{
    CREATE_VIEW_AND_DOC("foo\nbar\nfoo", 2, 3);
    view->setSelection(KTextEditor::Range(0, 0, 2, 3));
    view->createMultiCursorsFromSelection();

    const auto &cursors = view->secondaryMovingCursors();
    QCOMPARE(cursors.size(), doc.lines() - 1); // 1 cursor is primary, not included

    int i = 0;
    for (auto *c : cursors) {
        QCOMPARE(c->toCursor(), KTextEditor::Cursor(i, 3));
        i++;
    }
}

// kate: indent-mode cstyle; indent-width 4; replace-tabs on;
