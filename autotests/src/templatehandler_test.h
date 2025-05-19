/*
    This file is part of the KDE libraries
    SPDX-FileCopyrightText: 2010 Bernhard Beschow <bbeschow@cs.tu-berlin.de>
    SPDX-FileCopyrightText: 2014 Sven Brauch <svenbrauch@gmail.com>
    SPDX-FileCopyrightText: 2025 Mirian Margiani <mixosaurus+kde@pm.me>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef KATE_TEMPLATEHANDLER_TEST_H
#define KATE_TEMPLATEHANDLER_TEST_H

#include <QObject>

/**
 * Template handler tests.
 *
 * Notes on keyboard input in tests:
 *
 * 1. Key clicks must be posted to the view's focus proxy because it is the actual
 *    receiver of key events: https://doc.qt.io/qt-6/qwidget.html#setFocusProxy
 *    Key events are caught in KateViewInternal::eventFilter() and handled in
 *    KateViewInternal::keyPressEvent().
 *
 *    - view: KTextEditor::ViewPrivate
 *    - view->focusProxy(): KateViewInternal
 *
 *    - do:    QTest::keyClick(view->focusProxy(), 'A');
 *    - don't: QTest::keyClick(view, 'A');
 *
 * 2. Backspace is not handled in KateViewInternal::keyPressEvent() for some
 *    reason. The call to view()->backspace() is commented out. Instead of triggering
 *    the key event, simply call backspace() directly.
 *
 *    - do:    view->backspace();
 *    - don't: QTest::keyClick(view->focusProxy(), Qt::Key_Backspace);
 *
 * 3. Don't forget to clear the current selection via "view->clearSelection()"
 *    when testing keyboard input in a freshly inserted template. Simply calling
 *    "view->setCursorPosition({0, 0})" will not clear the selection and will
 *    lead to unexpected results when inserting text via QTest::keyClick().
 *
 */
class TemplateHandlerTest : public QObject
{
    Q_OBJECT

public:
    TemplateHandlerTest();

private Q_SLOTS:
    void testUndo();

    void testSimpleMirror();
    void testSimpleMirror_data();

    void testAlignC();
    void testAlignC_data();

    void testDefaults();
    void testDefaults_data();

    void testDefaultMirror();

    void testFunctionMirror();

    void testNotEditableFields();
    void testNotEditableFields_data();

    void testAdjacentRanges();
    void testAdjacentRanges2();
    void testAdjacentRanges3();

    void testTab();
    void testTab_data();

    void testExitAtCursor();

    void testAutoSelection();

    void testCanRetrieveSelection();

    void testEscapes();
    void testEscapes_data();
    void testLinesRemoved();
    void testLinesRemoved2();
};

#endif
