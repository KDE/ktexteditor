/*
    This file is part of the KDE libraries
    SPDX-FileCopyrightText: 2024 Waqar Ahmed <waqar.17a@gmail.com

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef SWAP_FILE_TEST_H
#define SWAP_FILE_TEST_H

#include <memory>

#include <QObject>
#include <QTemporaryDir>

class SwapFileTest : public QObject
{
    Q_OBJECT

public Q_SLOTS:
    void initTestCase();

private Q_SLOTS:
    void testSwapFileIsCreatedAndDestroyed();

private:
    QString createFile(const QByteArray &content);

private:
    std::unique_ptr<QTemporaryDir> m_testDir;
};

#endif
