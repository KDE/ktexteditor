/*
    This file is part of the KDE project
    SPDX-FileCopyrightText: 2013 Gerald Senarclens de Grancy <oss@senarclens.eu>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

// BEGIN Includes
#include "scripting_test.h"

#include "katecmd.h"
#include "kateconfig.h"
#include "katedocument.h"
#include "kateglobal.h"
#include "kateview.h"
#include <KTextEditor/Command>

#include <kconfig.h>
#include <kconfiggroup.h>
#include <kio/job.h>

#include <QDir>
#include <QEvent>
#include <QFile>
#include <QFileInfo>
#include <QList>
#include <QString>
#include <QTextStream>
#include <QTimer>

#include <QtTestWidgets>

#include "testutils.h"

QTEST_MAIN(ScriptingTest)

#define FAILURE(test, comment) qMakePair<const char *, const char *>((test), (comment))

void ScriptingTest::initTestCase()
{
    ScriptTestBase::initTestCase();
    m_section = "scripting";
    m_script_dir = "";
}

void ScriptingTest::bugs_data()
{
    getTestData("bugs");
}

void ScriptingTest::bugs()
{
    runTest(ExpectedFailures());
}
