/*
    SPDX-FileCopyrightText: 2010-2018 Dominik Haumann <dhaumann@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/
#ifndef KATE_SWAP_DIFF_CREATOR_H
#define KATE_SWAP_DIFF_CREATOR_H

#include <QProcess>
#include <QTemporaryFile>

namespace Kate
{
class SwapFile;
}

class SwapDiffCreator : public QObject
{
    Q_OBJECT

public:
    explicit SwapDiffCreator(Kate::SwapFile *swapFile);
    ~SwapDiffCreator() override = default;

public Q_SLOTS:
    void viewDiff();

private:
    Kate::SwapFile *const m_swapFile;

protected Q_SLOTS:
    void slotDataAvailable();
    void slotDiffFinished();

private:
    QProcess m_proc;
    QTemporaryFile m_originalFile;
    QTemporaryFile m_recoveredFile;
    QTemporaryFile m_diffFile;
};

#endif // KATE_SWAP_DIFF_CREATOR_H
