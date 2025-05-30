/*
    This file is part of the KDE libraries
    SPDX-FileCopyrightText: 2024 Waqar Ahmed <waqar.17a@gmail.com

    SPDX-License-Identifier: LGPL-2.0-or-later
*/
#include "swapfiletest.h"

#include <katedocument.h>
#include <kateundomanager.h>
#include <kateview.h>

#include <QFileInfo>
#include <QProcess>
#include <QTest>

QTEST_MAIN(SwapFileTest)

QString SwapFileTest::createFile(const QByteArray &content)
{
    const QString path = QDir(m_testDir->path()).absoluteFilePath(QStringLiteral("file"));
    QFile file(path);
    if (file.open(QFile::WriteOnly)) {
        file.write(content.data(), content.size());
        file.flush();
        file.close();
        return file.fileName();
    }
    return QString();
}

void SwapFileTest::initTestCase()
{
    // fresh clean dir per test
    m_testDir.reset(new QTemporaryDir());
}

void SwapFileTest::testSwapFileIsCreatedAndDestroyed()
{
    QVERIFY(m_testDir->isValid());
    QString file = createFile("This is a test file");
    auto doc = new KTextEditor::DocumentPrivate();
    doc->openUrl(QUrl::fromLocalFile(file));
    auto view = new KTextEditor::ViewPrivate(doc, nullptr);
    view->setCursorPosition({0, 5});
    view->keyReturn();

    QFileInfo fi(file);
    QString fileName = fi.fileName();
    QString swapFileName = QStringLiteral(".%1.kate-swp").arg(fileName);
    QTRY_VERIFY(fi.absoluteDir().exists(swapFileName));

    doc->undo();
    delete doc;
    QTRY_VERIFY(!fi.absoluteDir().exists(swapFileName));
}

#include "moc_swapfiletest.cpp"
