/*
    This file is part of the KDE project
    SPDX-FileCopyrightText: 2001, 2003 Peter Kelly <pmk@post.com>
    SPDX-FileCopyrightText: 2003, 2004 Stephan Kulow <coolo@kde.org>
    SPDX-FileCopyrightText: 2004 Dirk Mueller <mueller@kde.org>
    SPDX-FileCopyrightText: 2006, 2007 Leo Savernik <l.savernik@aon.at>
    SPDX-FileCopyrightText: 2010 Milian Wolff <mail@milianw.de>
    SPDX-FileCopyrightText: 2013 Gerald Senarclens de Grancy <oss@senarclens.eu>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

// BEGIN Includes

#include "kateconfig.h"
#include "katedocument.h"
#include "kateglobal.h"
#include "kateview.h"

#include <QCryptographicHash>
#include <QDir>
#include <QFileInfo>
#include <QJSEngine>
#include <QMainWindow>
#include <QProcess>
#include <QStandardPaths>
#include <QTest>

#include <iostream>

#include "script_test_base.h"
#include "testutils.h"

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
    m_msgHandler = qInstallMessageHandler(noDebugMessageOutput);
    m_toplevel = new QMainWindow();
    m_document = new KTextEditor::DocumentPrivate(true, false, m_toplevel);
    m_view = static_cast<KTextEditor::ViewPrivate *>(m_document->widget());
    m_view->config()->setValue(KateViewConfig::AutoBrackets, false);
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
            QJSValue result = m_env->engine()->evaluate(QString::fromLatin1(scriptFile.readAll()), scriptFile.fileName());
            QVERIFY2(!result.isError(), (result.toString() + QLatin1String(" in file ") + scriptFile.fileName()).toUtf8().constData());
        }
    }

    const QDir testDir(testDataPath + m_section + QLatin1Char('/') + script + QLatin1Char('/'));
    if (!testDir.exists()) {
        QSKIP(qPrintable(QString(testDir.path() + QLatin1String(" does not exist"))), SkipAll);
    }
    const auto testList = testDir.entryInfoList(QDir::Dirs | QDir::NoDotAndDotDot, QDir::Name);
    for (const auto &info : testList) {
        QTest::newRow(info.baseName().toUtf8().constData()) << info.absoluteFilePath();
    }
}

/**
 * helper to compare files
 * @param refFile reference file
 * @param outFile output file
 */
inline bool filesEqual(const QString &refFile, const QString &outFile)
{
    /**
     * quick compare, all fine, if no diffs!
     * use text mode + text streams to avoid unix/windows mismatches
     */
    QFile ref(refFile);
    QFile out(outFile);
    ref.open(QIODevice::ReadOnly | QIODevice::Text);
    out.open(QIODevice::ReadOnly | QIODevice::Text);
    QTextStream refIn(&ref);
    QTextStream outIn(&out);
    const QString refContent = refIn.readAll();
    const QString outContent = outIn.readAll();
    const bool equalResults = refContent == outContent;
    if (equalResults) {
        return true;
    }

    /**
     * elaborate diff output, if possible
     */
    static const QString diffExecutable = QStandardPaths::findExecutable(QStringLiteral("diff"));
    if (!diffExecutable.isEmpty()) {
        QProcess proc;
        proc.setProcessChannelMode(QProcess::ForwardedChannels);
        proc.start(diffExecutable, {QStringLiteral("-u"), refFile, outFile});
        proc.waitForFinished();
    }

    /**
     * else: trivial output of mismatching characters, e.g. for windows testing without diff
     */
    else {
        qDebug() << "'diff' executable is not in the PATH, no difference output";
    }

    // there were diffs
    return false;
}

void ScriptTestBase::runTest(const ExpectedFailures &failures)
{
    if (!QFile::exists(testDataPath + m_section)) {
        QSKIP(qPrintable(QString(testDataPath + m_section + QLatin1String(" does not exist"))), SkipAll);
    }

    QFETCH(QString, testcase);

    m_toplevel->resize(800, 600); // restore size

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
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    stream.setCodec("UTF8");
#endif
    QString code = stream.readAll();
    sourceFile.close();

    // Execute script
    QJSValue result = m_env->engine()->evaluate(code, testcase + QLatin1String("/input.js"), 1);
    QVERIFY2(!result.isError(), result.toString().toUtf8().constData());

    const QString fileExpected = testcase + QLatin1String("/expected");
    const QString fileActual = testcase + QLatin1String("/actual");

    url.setPath(fileActual);
    m_document->saveAs(url);
    m_document->closeUrl();

    for (const Failure &failure : failures) {
        QEXPECT_FAIL(failure.first, failure.second, Abort);
    }

    // compare files, expected fail will invert this verify
    QVERIFY(filesEqual(fileExpected, fileActual));
}
