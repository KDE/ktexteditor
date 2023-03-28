/*
    This file is part of the KDE project
    SPDX-FileCopyrightText: 2013 Gerald Senarclens de Grancy <oss@senarclens.eu>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

// BEGIN Includes
#include "scripting_test.h"

#include <QTest>

QTEST_MAIN(ScriptingTest)

#define FAILURE(test, comment) qMakePair<const char *, const char *>((test), (comment))

void ScriptingTest::initTestCase()
{
    ScriptTestBase::initTestCase();
    m_section = QStringLiteral("scripting");
    m_script_dir = QString();
}

void ScriptingTest::bugs_data()
{
    getTestData(QStringLiteral("bugs"));
}

void ScriptingTest::bugs()
{
    runTest(ExpectedFailures());
}
