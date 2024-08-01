/*
    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef COMMANDS_TEST_H
#define COMMANDS_TEST_H

#include <QObject>

#include "script_test_base.h"

class CommandsTest : public ScriptTestBase
{
    Q_OBJECT
private Q_SLOTS:
    void initTestCase();

    void utils_data();
    void utils();
};

#endif // COMMANDS_TEST_H
