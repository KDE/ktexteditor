/*
    This file is part of the KDE libraries
    SPDX-FileCopyrightText: 2010-2018 Dominik Haumann <dhaumann@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef KATE_DOCUMENT_TEST_H
#define KATE_DOCUMENT_TEST_H

#include <QObject>

class KateDocumentTest : public QObject
{
    Q_OBJECT

public:
    KateDocumentTest();
    ~KateDocumentTest();

public Q_SLOTS:
    void initTestCase();

private Q_SLOTS:
    void testWordWrap();
    void testWrapParagraph();
    void testReplaceQStringList();
    void testMovingInterfaceSignals();
    void testSetTextPerformance();
    void testRemoveTextPerformance();
    void testForgivingApiUsage();
    void testRemoveMultipleLines();
    void testInsertNewline();
    void testInsertAfterEOF();
    void testAutoBrackets();
    void testReplaceTabs();
    void testDigest();
    void testModelines();
    void testDefStyleNum();
    void testTypeCharsWithSurrogateAndNewLine();
    void testRemoveComposedCharacters();
    void testAutoReload();
    void testSearch();
    void testMatchingBracket_data();
    void testMatchingBracket();
};

#endif // KATE_DOCUMENT_TEST_H
