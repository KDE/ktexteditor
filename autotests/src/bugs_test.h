/*
    This file is part of the KDE libraries
    SPDX-FileCopyrightText: 2015 Zoe Clifford <zoeacacia@gmail.com>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef KATE_BUGS_TEST_H
#define KATE_BUGS_TEST_H

#include <QObject>

class BugTest : public QObject
{
    Q_OBJECT

public:
    BugTest();
    ~BugTest() override;

private Q_SLOTS:
    void initTestCase();
    void cleanupTestCase();

    void testBug205447DeleteSurrogates();
    void testBug205447BackspaceSurrogates();
    void testBug286887CtrlShiftLeft();
    void bug313759TryCrash();
    void bug313769TryCrash();
    void bug317111TryCrash();
};

#endif
