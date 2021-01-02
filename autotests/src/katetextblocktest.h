/*
    This file is part of the Kate project.

    SPDX-FileCopyrightText: 2021 Waqar Ahmed <waqar.17a@gmail.com>
    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef KATETEXTBLOCKTEST_H
#define KATETEXTBLOCKTEST_H

#include <QObject>
#include <QTest>

class KateTextBlockTest : public QObject
{
    Q_OBJECT
public:
    explicit KateTextBlockTest(QObject *parent = nullptr);

private Q_SLOTS:
    void basicTest();
    void testWrap();
    void testInsertRemoveText();
    void testSplitMergeBlocks();
    void testTextRanges();
};

#endif // KATETEXTBLOCKTEST_H
