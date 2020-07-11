/**
 * This file is part of the KDE project
 *
 * Copyright (C) 2013 Gerald Senarclens de Grancy <oss@senarclens.eu>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this library; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 *
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
#include <QObject>
#include <QString>
#include <QTextStream>
#include <QTimer>
#include <Q_PID>

#include <QtTestWidgets>

#include "script_test_base.h"
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
