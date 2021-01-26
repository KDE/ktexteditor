/*
    This file is part of the Kate project.

    SPDX-FileCopyrightText: 2021 Waqar Ahmed <waqar.17a@gmail.com>
    SPDX-License-Identifier: LGPL-2.0-or-later
*/
#include "camelcursortest.h"

#include <katedocument.h>
#include <kateview.h>

#include <QTest>

QTEST_MAIN(CamelCursorTest)

CamelCursorTest::CamelCursorTest(QObject *parent)
    : QObject(parent)
{
    doc = new KTextEditor::DocumentPrivate(false, false);
    view = static_cast<KTextEditor::ViewPrivate *>(doc->createView(nullptr));
}

CamelCursorTest::~CamelCursorTest()
{
    delete doc;
}

void CamelCursorTest::testWordMovementSingleRow_data()
{
    /**
     * NOTE: If you are here to fix a bug, try not to add a new function please.
     * Instead consider adding a row here with the label as your bug-number.
     *
     * The other test cases are small because if this works, they will work automaitcally
     * since the core functionality is correct.
     */

    // The text for document
    QTest::addColumn<QString>("text");
    // The number of Ctrl + Right/Left movements
    QTest::addColumn<int>("movements");
    // The expected positions where your cursor is supposed to land
    QTest::addColumn<QVector<int>>("colPos");

    // clang-format off
    // row name                                         text                                no of mov       expected positions
    QTest::addRow("KateView")             << QStringLiteral("KateView")                     << 2 << QVector<int>{4, 8};
    QTest::addRow("Q_LOGGING_CATEGORY")   << QStringLiteral("Q_LOGGING_CATEGORY();")        << 4 << QVector<int>{2, 10, 18, 21};
    QTest::addRow("Q_L11GING_CATEG0RY")   << QStringLiteral("Q_L11GING_CATEG0RY();")        << 7 << QVector<int>{2, 5, 10, 14, 16, 18, 21};
    QTest::addRow("snake_case_name")      << QStringLiteral("int snake_case_name = 123;")   << 7 << QVector<int>{4, 10, 15, 20, 22, 25, 26};
    QTest::addRow("bad___SNAKE_case__")   << QStringLiteral("int bad___SNAKE_case__ = 11;") << 7 << QVector<int>{4, 10, 16, 23, 25, 27, 28};
    QTest::addRow("QApplication")         << QStringLiteral("QApplication app;")            << 4 << QVector<int>{1, 13, 16, 17};
    QTest::addRow("ABCDead")              << QStringLiteral("ABCDead")                      << 2 << QVector<int>{3, 7};
    QTest::addRow("SE_CheckBoxIndicator") << QStringLiteral("QStyle::SE_CheckBoxIndicator") << 7 << QVector<int>{1, 6, 8, 11, 16, 19, 28};
    QTest::addRow("SE_CHECKBoxIndicator") << QStringLiteral("QStyle::SE_CHECKBoxIndicator") << 7 << QVector<int>{1, 6, 8, 11, 16, 19, 28};
    QTest::addRow("SE_CHECKBOXINDICATOR") << QStringLiteral("QStyle::SE_CHECKBOXINDICATOR") << 5 << QVector<int>{1, 6, 8, 11, 28};
    QTest::addRow("abc0_asd")             << QStringLiteral("int abc0_asd")                 << 3 << QVector<int>{4, 9, 12};
    QTest::addRow("abc120_aSD")           << QStringLiteral("int abc120_aSD")               << 4 << QVector<int>{4, 11, 12, 14};
    // clang-format on
}

void CamelCursorTest::testWordMovementSingleRow()
{
    QFETCH(QString, text);
    QFETCH(int, movements);
    QFETCH(QVector<int>, colPos);

    doc->setText(text);
    view->setCursorPosition({0, 0});

    for (int i = 0; i < movements; ++i) {
        view->wordRight();
        QCOMPARE(view->cursorPosition(), KTextEditor::Cursor(0, colPos.at(i)));
    }

    // reverse jumping. Reverse everything but last
    // element. Last element will be set to 0 as that
    // is our start pos
    std::reverse(colPos.begin(), colPos.end() - 1);
    colPos.back() = 0;

    for (int i = 0; i < movements; ++i) {
        view->wordLeft();
        QCOMPARE(view->cursorPosition(), KTextEditor::Cursor(0, colPos.at(i)));
    }

    QCOMPARE(view->cursorPosition(), KTextEditor::Cursor(0, 0));
}

void CamelCursorTest::testRtlWordMovement()
{
    doc->setText(QStringLiteral("اردو کا جملہ"));
    view->setCursorPosition({0, 0});

    // for RTL we move left
    auto pos = QVector<int>{5, 8, 12};
    for (int i = 0; i < 3; ++i) {
        view->wordLeft();
        QCOMPARE(view->cursorPosition().column(), pos.at(i));
    }

    // now reverse to original pos
    pos = QVector<int>{8, 5, 0};
    for (int i = 0; i < 3; ++i) {
        view->wordRight();
        QCOMPARE(view->cursorPosition().column(), pos.at(i));
    }
}

void CamelCursorTest::testWordMovementMultipleRow_data()
{
    QTest::addColumn<QString>("text");
    QTest::addColumn<int>("movements");
    QTest::addColumn<QVector<KTextEditor::Cursor>>("expect_cursor");

    using C = KTextEditor::Cursor;
    QTest::addRow("2 lines") << QStringLiteral("KateView\nnextLine") << 4 << QVector<C>{C(0, 4), C(0, 8), C(1, 0), C(1, 4)};
    QTest::addRow("2 line caps") << QStringLiteral("Kate_VIEW\nNextLINE") << 4 << QVector<C>{C(0, 5), C(0, 9), C(1, 0), C(1, 4)};
    QTest::addRow("4 lines") << QStringLiteral("Kate\nView\nNext\nLINE") << 7 << QVector<C>{C(0, 4), C(1, 0), C(1, 4), C(2, 0), C(2, 4), C(3, 0), C(3, 4)};
}

void CamelCursorTest::testWordMovementMultipleRow()
{
    QFETCH(QString, text);
    QFETCH(int, movements);
    QFETCH(QVector<KTextEditor::Cursor>, expect_cursor);

    doc->setText(text);
    view->setCursorPosition({0, 0});

    for (int i = 0; i < movements; ++i) {
        view->wordRight();
        QCOMPARE(view->cursorPosition(), expect_cursor.at(i));
    }

    std::reverse(expect_cursor.begin(), expect_cursor.end() - 1);
    expect_cursor.back() = KTextEditor::Cursor(0, 0);

    for (int i = 0; i < movements; ++i) {
        view->wordLeft();
        QCOMPARE(view->cursorPosition(), expect_cursor.at(i));
    }
}

void CamelCursorTest::testDeletionRight()
{
    doc->setText("SomeWord");
    view->setCursorPosition({0, 0});

    view->deleteWordRight();
    QCOMPARE(doc->text(), QStringLiteral("Word"));
    view->deleteWordRight();
    QCOMPARE(doc->text(), QString());

    doc->setText("Some Word");
    view->setCursorPosition({0, 0});

    view->deleteWordRight();
    QCOMPARE(doc->text(), QStringLiteral("Word"));
    view->deleteWordRight();
    QCOMPARE(doc->text(), QString());

    doc->setText("Some_WORD");
    view->setCursorPosition({0, 0});

    view->deleteWordRight();
    QCOMPARE(doc->text(), QStringLiteral("WORD"));
    view->deleteWordRight();
    QCOMPARE(doc->text(), QString());

    doc->setText("Some      WORD");
    view->setCursorPosition({0, 0});

    view->deleteWordRight();
    QCOMPARE(doc->text(), QStringLiteral("WORD"));
    view->deleteWordRight();
    QCOMPARE(doc->text(), QString());
}

void CamelCursorTest::testDeletionLeft()
{
    doc->setText("SomeWord");
    view->setCursorPosition({0, 8});
    view->deleteWordLeft();
    QCOMPARE(doc->text(), QStringLiteral("Some"));
    view->deleteWordLeft();
    QCOMPARE(doc->text(), QString());

    doc->setText("Some Word");
    view->setCursorPosition({0, 9});
    view->deleteWordLeft();
    QCOMPARE(doc->text(), QStringLiteral("Some "));
    view->deleteWordLeft();
    QCOMPARE(doc->text(), QString());

    doc->setText("Some_WORD");
    view->setCursorPosition({0, 9});
    view->deleteWordLeft();
    QCOMPARE(doc->text(), QStringLiteral("Some_"));
    view->deleteWordLeft();
    QCOMPARE(doc->text(), QString());

    doc->setText("Some   WORD");
    view->setCursorPosition({0, 11});
    view->deleteWordLeft();
    QCOMPARE(doc->text(), QStringLiteral("Some   "));
    view->deleteWordLeft();
    QCOMPARE(doc->text(), QString());
}

void CamelCursorTest::testSelectionRight()
{
    doc->setText("HelloWorld");
    view->setCursorPosition({0, 0});
    view->shiftWordRight();
    QCOMPARE(view->selectionText(), QStringLiteral("Hello"));
    QCOMPARE(view->selectionRange(), KTextEditor::Range(0, 0, 0, 5));

    doc->setText("Hello\nWorld");
    view->setCursorPosition({0, 0});
    view->shiftWordRight();
    view->shiftWordRight();
    QCOMPARE(view->selectionText(), QStringLiteral("Hello\n"));
    QCOMPARE(view->selectionRange(), KTextEditor::Range(0, 0, 1, 0));
}

void CamelCursorTest::testSelectionLeft()
{
    doc->setText("HelloWorld");
    view->setCursorPosition({0, 10});
    view->shiftWordLeft();
    QCOMPARE(view->selectionText(), QStringLiteral("World"));
    QCOMPARE(view->selectionRange(), KTextEditor::Range(0, 5, 0, 10));

    doc->setText("Hello\nWorld");
    view->setCursorPosition({1, 0});
    view->shiftWordLeft();
    view->shiftWordLeft();
    QCOMPARE(view->selectionText(), QStringLiteral("Hello\n"));
    QCOMPARE(view->selectionRange(), KTextEditor::Range(0, 0, 1, 0));
}
