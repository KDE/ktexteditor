/*
    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef SCRIPTINGTEST_H
#define SCRIPTINGTEST_H

#include <QObject>

#include "script_test_base.h"

class ScriptingTest : public ScriptTestBase
{
    Q_OBJECT
private Q_SLOTS:
    void initTestCase();

    void bugs_data();
    void bugs();
};

#endif // SCRIPTINGTEST_H
