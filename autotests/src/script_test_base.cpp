/**
 * This file is part of the KDE project
 *
 * Copyright (C) 2001,2003 Peter Kelly (pmk@post.com)
 * Copyright (C) 2003,2004 Stephan Kulow (coolo@kde.org)
 * Copyright (C) 2004 Dirk Mueller ( mueller@kde.org )
 * Copyright 2006, 2007 Leo Savernik (l.savernik@aon.at)
 * Copyright (C) 2010 Milian Wolff <mail@milianw.de>
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

//BEGIN Includes

#include "kateview.h"
#include "katedocument.h"
#include "kateglobal.h"

#include <QProcess>
#include <QDirIterator>
#include <QMainWindow>
#include <QScriptEngine>
#include <QCryptographicHash>
#include <QTest>

#include "testutils.h"

#include "script_test_base.h"

const QString testDataPath(QLatin1String(TEST_DATA_DIR));

QtMessageHandler ScriptTestBase::m_msgHandler = nullptr;
void noDebugMessageOutput(QtMsgType type, const QMessageLogContext &context, const QString &msg)
{
    switch (type) {
    case QtDebugMsg:
        break;
    default:
        ScriptTestBase::m_msgHandler(type, context, msg);
    }
}

void ScriptTestBase::initTestCase()
{
    KTextEditor::EditorPrivate::enableUnitTestMode();
    m_msgHandler =  qInstallMessageHandler(noDebugMessageOutput);
    m_toplevel = new QMainWindow();
    m_document = new KTextEditor::DocumentPrivate(true, false, m_toplevel);
    m_view = static_cast<KTextEditor::ViewPrivate *>(m_document->widget());
    m_env = new TestScriptEnv(m_document, m_outputWasCustomised);
}

void ScriptTestBase::cleanupTestCase()
{
    qInstallMessageHandler(m_msgHandler);
}

void ScriptTestBase::getTestData(const QString &script)
{
    QTest::addColumn<QString>("testcase");

    // make sure the script files are valid
    if (!m_script_dir.isEmpty()) {
        QFile scriptFile(QLatin1String(JS_DATA_DIR) + m_script_dir + QLatin1Char('/') + script + QLatin1String(".js"));
        if (scriptFile.exists()) {
            QVERIFY(scriptFile.open(QFile::ReadOnly));
            QScriptValue result = m_env->engine()->evaluate(QString::fromLatin1(scriptFile.readAll()), scriptFile.fileName());
            QVERIFY2(!result.isError(), qPrintable(QString(result.toString() + QLatin1String("\nat ")
                                               + m_env->engine()->uncaughtExceptionBacktrace().join(QLatin1String("\n")))));
        }
    }

    const QString testDir(testDataPath + m_section + QLatin1Char('/') + script + QLatin1Char('/'));
    if (!QFile::exists(testDir)) {
        QSKIP(qPrintable(QString(testDir + QLatin1String(" does not exist"))), SkipAll);
    }
    QDirIterator contents(testDir);
    while (contents.hasNext()) {
        QString entry = contents.next();
        if (entry.endsWith(QLatin1Char('.'))) {
            continue;
        }
        QFileInfo info(entry);
        if (!info.isDir()) {
            continue;
        }
        QTest::newRow(info.baseName().toLocal8Bit().constData()) << info.absoluteFilePath();
    }
}

void ScriptTestBase::runTest(const ExpectedFailures &failures)
{
    if (!QFile::exists(testDataPath + m_section)) {
        QSKIP(qPrintable(QString(testDataPath + m_section + QLatin1String(" does not exist"))), SkipAll);
    }

    QFETCH(QString, testcase);

    m_toplevel->resize(800, 600);  // restore size

    // load page
    QUrl url;
    url.setScheme(QLatin1String("file"));
    url.setPath(testcase + QLatin1String("/origin"));
    m_document->openUrl(url);

    // evaluate test-script
    QFile sourceFile(testcase + QLatin1String("/input.js"));
    if (!sourceFile.open(QFile::ReadOnly)) {
        QFAIL(qPrintable(QString::fromLatin1("Failed to open file: %1").arg(sourceFile.fileName())));
    }

    QTextStream stream(&sourceFile);
    stream.setCodec("UTF8");
    QString code = stream.readAll();
    sourceFile.close();

    // Execute script
    QScriptValue result = m_env->engine()->evaluate(code, testcase + QLatin1String("/input.js"), 1);
    QVERIFY2(!result.isError(), result.toString().toUtf8().constData());

    const QString fileExpected = testcase + QLatin1String("/expected");
    const QString fileActual = testcase + QLatin1String("/actual");

    url.setPath(fileActual);
    m_document->saveAs(url);

    const QByteArray actualChecksum = m_document->checksum();
    const QByteArray expectedChecksum = digestForFile(fileExpected);

    if (actualChecksum != expectedChecksum) {
        // diff actual and expected
        QProcess diff;
        QStringList args(QStringList() << QLatin1String("-u") << fileExpected << fileActual);
        diff.start(QLatin1String("diff"), args);
        diff.waitForFinished();

        QByteArray out = diff.readAllStandardOutput();
        QByteArray err = diff.readAllStandardError();

        if (!err.isEmpty()) {
            qWarning() << err;
        }

        if (diff.exitCode() != EXIT_SUCCESS) {
            QTextStream(stdout) << out << endl;
        }

        for (const Failure &failure : failures) {
            QEXPECT_FAIL(failure.first, failure.second, Abort);
        }

        QCOMPARE(diff.exitCode(), EXIT_SUCCESS);
    }

    m_document->closeUrl();
}

QByteArray ScriptTestBase::digestForFile(const QString &file)
{
    QByteArray digest;

    QFile f(file);
    if (f.open(QIODevice::ReadOnly)) {
        // init the hash with the git header
        QCryptographicHash crypto(QCryptographicHash::Sha1);
        const QString header = QString(QLatin1String("blob %1")).arg(f.size());
        crypto.addData(header.toLatin1() + '\0');

        while (!f.atEnd()) {
            crypto.addData(f.read(256 * 1024));
        }

        digest = crypto.result();
    }

    return digest;
}
