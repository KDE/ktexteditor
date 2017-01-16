/*  This file is part of the Kate project.
 *
 *  Copyright (C) 2013 Dominik Haumann <dhaumann@kde.org>
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Library General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Library General Public License for more details.
 *
 *  You should have received a copy of the GNU Library General Public License
 *  along with this library; see the file COPYING.LIB.  If not, write to
 *  the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 *  Boston, MA 02110-1301, USA.
 */

#include "katesyntaxtest.h"

#include <kateglobal.h>
#include <katebuffer.h>
#include <katedocument.h>
#include <kateview.h>
#include <kateconfig.h>
#include <katetextfolding.h>

#include <QtTestWidgets>
#include <QDirIterator>
#include <QFileInfo>
#include <QProcess>

QTEST_MAIN(KateSyntaxTest)

void KateSyntaxTest::initTestCase()
{
    KTextEditor::EditorPrivate::enableUnitTestMode();
}

void KateSyntaxTest::cleanupTestCase()
{
}

void KateSyntaxTest::testSyntaxHighlighting_data()
{
    QTest::addColumn<QString>("hlTestCase");

    /**
     * check for directories, one dir == one hl
     */
    const QString testDir(QLatin1String(TEST_DATA_DIR) + QLatin1String("/syntax/"));
    QDirIterator contents(testDir);
    while (contents.hasNext()) {
        const QString hlDir = contents.next();
        const QFileInfo info(hlDir);
        if (!info.isDir() || hlDir.contains(QLatin1Char('.'))) {
            continue;
        }

        /**
         * now: get the tests per hl
         */
        QDirIterator contents(hlDir);
        while (contents.hasNext()) {
            const QString hlTestCase = contents.next();
            const QFileInfo info(hlTestCase);
            if (!info.isFile()) {
                continue;
            }

            QTest::newRow(info.absoluteFilePath().toLocal8Bit().constData()) << info.absoluteFilePath();
        }
    }
}


void KateSyntaxTest::testSyntaxHighlighting()
{
    /**
     * get current test case
     */
    QFETCH(QString, hlTestCase);

    /**
     * create a document with a view to be able to export stuff
     */
    KTextEditor::DocumentPrivate doc;
    auto view = static_cast<KTextEditor::ViewPrivate*>(doc.createView(nullptr));

    /**
     * load the test case
     * enforce UTF-8 to avoid locale problems
     */
    QUrl url;
    url.setScheme(QLatin1String("file"));
    url.setPath(hlTestCase);
    doc.setEncoding(QStringLiteral("UTF-8"));
    QVERIFY(doc.openUrl(url));

    /**
     * compute needed dirs
     */
    const QFileInfo info(hlTestCase);
    const QString resultDir(info.absolutePath() + QLatin1String("/results/"));
    const QString currentResult(resultDir + info.fileName() + QLatin1String(".current.html"));
    const QString referenceResult(resultDir + info.fileName() + QLatin1String(".reference.html"));

    /**
     * export the result
     */
    view->exportHtmlToFile(currentResult);

    /**
     * verify the result against reference
     */
    QProcess diff;
    diff.setProcessChannelMode(QProcess::MergedChannels);
    QStringList args;
    args << QLatin1String("-u") << (referenceResult) << (currentResult);
    diff.start(QLatin1String("diff"), args);
    diff.waitForFinished();
    QByteArray out = diff.readAllStandardOutput();
    if (!out.isEmpty()) {
        printf("DIFF:\n");
        QList<QByteArray> outLines = out.split('\n');
        Q_FOREACH(const QByteArray &line, outLines) {
            printf("%s\n", qPrintable(line));
        }
    }
    QCOMPARE(QString::fromLocal8Bit(out), QString());
    QCOMPARE(diff.exitCode(), EXIT_SUCCESS);
}
