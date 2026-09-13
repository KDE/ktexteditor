/*
    SPDX-FileCopyrightText: 2026 Fan Chung <kde@fanchung.net>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "kateviewaccessible_test.h"

#include <katedocument.h>
#include <ktexteditor/view.h>

#include <QAccessible>
#include <QStandardPaths>
#include <QTest>

#include <memory>

QTEST_MAIN(KateViewAccessibleTest)

void KateViewAccessibleTest::initTestCase()
{
    QStandardPaths::setTestModeEnabled(true);
}

void KateViewAccessibleTest::testTextAtOffset()
{
    KTextEditor::DocumentPrivate doc(false, false);
    doc.setText(QStringLiteral("Line 1\nLine 2\nLine 3"));

    const std::unique_ptr<KTextEditor::View> view(doc.createView(nullptr));
    QAccessibleInterface *accessibleInterface = QAccessible::queryAccessibleInterface(view->editorWidget());
    QVERIFY(accessibleInterface);
    QAccessibleTextInterface *textInterface = accessibleInterface->textInterface();
    QVERIFY(textInterface);

    struct ExpectedBoundary {
        int offset;
        QString text;
        int startOffset;
        int endOffset;
    };
    const QList<ExpectedBoundary> expectedLines{
        {0, QStringLiteral("Line 1\n"), 0, 7},
        {4, QStringLiteral("Line 1\n"), 0, 7},
        {6, QStringLiteral("Line 1\n"), 0, 7},
        {7, QStringLiteral("Line 2\n"), 7, 14},
        {13, QStringLiteral("Line 2\n"), 7, 14},
        {14, QStringLiteral("Line 3"), 14, 20},
        {19, QStringLiteral("Line 3"), 14, 20},
    };

    for (const ExpectedBoundary &expected : expectedLines) {
        int startOffset = -1;
        int endOffset = -1;
        const QString text = textInterface->textAtOffset(expected.offset, QAccessible::LineBoundary, &startOffset, &endOffset);
        QCOMPARE(text, expected.text);
        QCOMPARE(startOffset, expected.startOffset);
        QCOMPARE(endOffset, expected.endOffset);
    }

    int startOffset = -1;
    int endOffset = -1;
    QCOMPARE(textInterface->textAtOffset(2, QAccessible::WordBoundary, &startOffset, &endOffset), QStringLiteral("Line"));
    QCOMPARE(startOffset, 0);
    QCOMPARE(endOffset, 4);

    doc.setText(QStringLiteral("Line 1\n\nLine 3"));
    QCOMPARE(textInterface->textAtOffset(7, QAccessible::LineBoundary, &startOffset, &endOffset), QStringLiteral("\n"));
    QCOMPARE(startOffset, 7);
    QCOMPARE(endOffset, 8);
}

#include "moc_kateviewaccessible_test.cpp"
