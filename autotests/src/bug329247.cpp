/*
    This file is part of the KDE libraries
    SPDX-FileCopyrightText: 2023 Rémi Peuchot <kde.remi@proton.me>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "bug329247.h"

#include <kateconfig.h>
#include <katedocument.h>
#include <kateglobal.h>
#include <kateview.h>

#include <QtTestWidgets>

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

// This bug fires when the end of selection
// is inside the indentation of the last line of the selection

const Cursor start_selection = Cursor(0, 2);

const int end_selection_line = 2;

const QString document = QString::fromUtf8(
    "        AAAAAAAA\n" // <- will be selected
    "        AAAAAAAA\n" // <- will be selected
    "        AAAAAAAA\n" // <- will be selected
    "        BBBBBBBB\n"
    "        BBBBBBBB\n\n");

const QString expected_document_after_1_indent = QString::fromUtf8(
    "            AAAAAAAA\n"
    "            AAAAAAAA\n"
    "            AAAAAAAA\n"
    "        BBBBBBBB\n"
    "        BBBBBBBB\n\n");

const QString expected_document_after_2_indent = QString::fromUtf8(
    "                AAAAAAAA\n"
    "                AAAAAAAA\n"
    "                AAAAAAAA\n"
    "        BBBBBBBB\n"
    "        BBBBBBBB\n\n");

void expectDocument(KTextEditor::DocumentPrivate &document, const QString expected_document)
{
    QCOMPARE(document.text(), expected_document);
}

void BugTest::indentSelection()
{
    KTextEditor::DocumentPrivate doc(false, false);
    KTextEditor::View *view = doc.createView(nullptr);
    view->resize(400, 300);

    // Selection starts before "AAAAA" bloc
    // and ends on the last line of "AAAAA" bloc
    // Notes :
    // - for a wider coverage, all possible end_selection values are tested
    // - end_selection_col = 0 is not tested, it's a specific case where
    //   last line must not be indented (see KateAutoIndent::changeIndent internal comment)
    for (int end_selection_col = 1; end_selection_col < 12; end_selection_col++) {
        doc.setText(document);

        Cursor end_selection(end_selection_line, end_selection_col);
        Range selection(start_selection, end_selection);

        // First indent : select the "AAAAA" bloc and indent it
        // (the first indent works as expected)
        view->setSelection(selection);
        doc.indent(selection, 1);
        expectDocument(doc, expected_document_after_1_indent);

        // The bug is related to the selection being altered by the first indent action.
        // To reproduce it, we indent the selection again.
        selection = view->selectionRange();
        doc.indent(selection, 1);
        expectDocument(doc, expected_document_after_2_indent);
    }
}

#include "moc_bug329247.cpp"
