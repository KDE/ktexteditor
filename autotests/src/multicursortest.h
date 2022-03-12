/*
    This file is part of the KDE libraries
    SPDX-FileCopyrightText: 2022 Waqar Ahmed <waqar.17a@gmail.com>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef KTE_MULTI_CURSOR_TEST_H
#define KTE_MULTI_CURSOR_TEST_H

#include <QObject>

class MulticursorTest : public QObject
{
    Q_OBJECT

public:
    MulticursorTest();
    ~MulticursorTest() override;

private Q_SLOTS:
    // Creation
    void testCreateMultiCursor();
    void testCreateMultiCursorFromSelection();

    // Text transformations
    void testKillline();

    // Insert & Remove tests
    void insertRemoveText();
    void testUndoRedo();
    void testUndoRedoWithSelection();

    // Movement
    void moveCharTest();
    void moveWordTest();
    void homeEndKeyTest();
};

#endif // KATE_VIEW_TEST_H
