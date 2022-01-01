/*
    SPDX-FileCopyrightText: 2010-2018 Dominik Haumann <dhaumann@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/
#include "kateswapdiffcreator.h"
#include "katedocument.h"
#include "katepartdebug.h"
#include "kateswapfile.h"

#include <KIO/JobUiDelegate>
#include <KIO/OpenUrlJob>
#include <KLocalizedString>
#include <KMessageBox>

#include <QDir>
#include <QTextCodec>

// BEGIN SwapDiffCreator
SwapDiffCreator::SwapDiffCreator(Kate::SwapFile *swapFile)
    : QObject(swapFile)
    , m_swapFile(swapFile)
{
}

void SwapDiffCreator::viewDiff()
{
    QString path = m_swapFile->fileName();
    if (path.isNull()) {
        return;
    }

    QFile swp(path);
    if (!swp.open(QIODevice::ReadOnly)) {
        qCWarning(LOG_KTE) << "Can't open swap file";
        return;
    }

    // create all needed tempfiles
    m_originalFile.setFileTemplate(QDir::tempPath() + QLatin1String("/katepart_XXXXXX.original"));
    m_recoveredFile.setFileTemplate(QDir::tempPath() + QLatin1String("/katepart_XXXXXX.recovered"));
    m_diffFile.setFileTemplate(QDir::tempPath() + QLatin1String("/katepart_XXXXXX.diff"));

    if (!m_originalFile.open() || !m_recoveredFile.open() || !m_diffFile.open()) {
        qCWarning(LOG_KTE) << "Can't open temporary files needed for diffing";
        return;
    }

    // truncate files, just in case
    m_originalFile.resize(0);
    m_recoveredFile.resize(0);
    m_diffFile.resize(0);

    // create a document with the recovered data
    KTextEditor::DocumentPrivate recoverDoc;
    recoverDoc.setText(m_swapFile->document()->text());

    // store original text in a file as utf-8 and close it
    {
        QTextStream stream(&m_originalFile);
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
        stream.setCodec(QTextCodec::codecForName("UTF-8"));
#endif
        stream << recoverDoc.text();
    }
    m_originalFile.close();

    // recover data
    QDataStream stream(&swp);
    recoverDoc.swapFile()->recover(stream, false);

    // store recovered text in a file as utf-8 and close it
    {
        QTextStream stream(&m_recoveredFile);
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
        stream.setCodec(QTextCodec::codecForName("UTF-8"));
#endif
        stream << recoverDoc.text();
    }
    m_recoveredFile.close();

    // create a process for diff
    m_proc.setProcessChannelMode(QProcess::MergedChannels);

    connect(&m_proc, &QProcess::readyRead, this, &SwapDiffCreator::slotDataAvailable, Qt::UniqueConnection);
    connect(&m_proc, &QProcess::finished, this, &SwapDiffCreator::slotDiffFinished, Qt::UniqueConnection);

    // try to start diff process, if we can't be started be done with error
    m_proc.start(QStringLiteral("diff"), QStringList() << QStringLiteral("-u") << m_originalFile.fileName() << m_recoveredFile.fileName());
    if (!m_proc.waitForStarted()) {
        KMessageBox::sorry(nullptr,
                           i18n("The diff command could not be started. Please make sure that "
                                "diff(1) is installed and in your PATH."),
                           i18n("Error Creating Diff"));
        deleteLater();
        return;
    }

    // process is up and running, we can write data to it
    QTextStream ts(&m_proc);
    int lineCount = recoverDoc.lines();
    for (int line = 0; line < lineCount; ++line) {
        ts << recoverDoc.line(line) << '\n';
    }
    ts.flush();
    m_proc.closeWriteChannel();
}

void SwapDiffCreator::slotDataAvailable()
{
    // collect diff output
    m_diffFile.write(m_proc.readAll());
}

void SwapDiffCreator::slotDiffFinished()
{
    // collect last diff output, if any
    m_diffFile.write(m_proc.readAll());

    // get the exit status to check whether diff command run successfully
    const QProcess::ExitStatus es = m_proc.exitStatus();

    // check exit status
    if (es != QProcess::NormalExit) {
        KMessageBox::sorry(nullptr,
                           i18n("The diff command failed. Please make sure that "
                                "diff(1) is installed and in your PATH."),
                           i18n("Error Creating Diff"));
        deleteLater();
        return;
    }

    // sanity check: is there any diff content?
    if (m_diffFile.size() == 0) {
        KMessageBox::information(nullptr, i18n("The files are identical."), i18n("Diff Output"));
        deleteLater();
        return;
    }

    // close diffFile and avoid removal, KIO::OpenUrlJob will do that later!
    m_diffFile.close();
    m_diffFile.setAutoRemove(false);

    KIO::OpenUrlJob *job = new KIO::OpenUrlJob(QUrl::fromLocalFile(m_diffFile.fileName()), QStringLiteral("text/x-patch"));
    job->setUiDelegate(new KIO::JobUiDelegate(KJobUiDelegate::AutoHandlingEnabled, m_swapFile->document()->activeView()));
    job->setDeleteTemporaryFile(true); // delete the file, once the client exits
    job->start();

    deleteLater();
}

// END SwapDiffCreator
