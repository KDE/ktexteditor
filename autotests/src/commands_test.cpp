/*
    This file is part of the KDE project
    SPDX-FileCopyrightText: 2013 Gerald Senarclens de Grancy <oss@senarclens.eu>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

// BEGIN Includes
#include "commands_test.h"

#include <QtTestWidgets>

#include "testutils.h"

QTEST_MAIN(CommandsTest)

#define FAILURE(test, comment) qMakePair<const char *, const char *>((test), (comment))

void CommandsTest::initTestCase()
{
    ScriptTestBase::initTestCase();
    m_section = QStringLiteral("commands");
    m_script_dir = QStringLiteral("commands");
}

void CommandsTest::utils_data()
{
    getTestData(QStringLiteral("utils"));
}

void CommandsTest::utils()
{
    runTest(ExpectedFailures());
}
