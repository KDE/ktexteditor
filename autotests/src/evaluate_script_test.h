/*
    This file is part of the KDE libraries
    SPDX-FileCopyrightText: 2025 Thomas Friedrichsmeier <thomas.friedrichsmeier@kdemail.net>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef EVALUATE_SCRIPT_TEST_H
#define EVALUATE_SCRIPT_TEST_H

#include <QObject>

class EvaluateScriptTest : public QObject
{
    Q_OBJECT

public:
    EvaluateScriptTest();

private Q_SLOTS:
    void testUndo();
    void testError();
    void testSelection();
    void testReturn();
};

#endif
