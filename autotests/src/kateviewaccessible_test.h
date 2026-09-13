/*
    SPDX-FileCopyrightText: 2026 Fan Chung <kde@fanchung.net>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef KATEVIEWACCESSIBLE_TEST_H
#define KATEVIEWACCESSIBLE_TEST_H

#include <QObject>

class KateViewAccessibleTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void initTestCase();
    void testTextAtOffset();
};

#endif
