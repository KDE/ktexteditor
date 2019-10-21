/*  SPDX-License-Identifier: LGPL-2.0-or-later

    Copyright (C) 2017 KDE Developers

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to
    the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
    Boston, MA 02110-1301, USA.
*/

#ifndef KATE_SECURE_TEXTBUFFER_P_H
#define KATE_SECURE_TEXTBUFFER_P_H

#include <QCryptographicHash>
#include <QObject>
#include <QString>

#include <kauth.h>

using namespace KAuth;

/**
 * Class used as KAuth helper binary.
 * It is supposed to be called through KAuth action.
 *
 * It also contains couple of common methods intended to be used
 * directly by TextBuffer as well as from helper binary.
 *
 * This class should only be used by TextBuffer.
 */
class SecureTextBuffer : public QObject
{
    Q_OBJECT

public:
    SecureTextBuffer()
    {
    }

    ~SecureTextBuffer()
    {
    }

    /**
     * Common helper method
     */
    static void setOwner(const int filedes, const uint ownerId, const uint groupId);

    static const QCryptographicHash::Algorithm checksumAlgorithm = QCryptographicHash::Algorithm::Sha512;

private:
    /**
     * Saves file contents using sets permissions.
     */
    static bool saveFileInternal(const QString &sourceFile, const QString &targetFile, const QByteArray &checksum, const uint ownerId, const uint groupId);

    static bool moveFile(const QString &sourceFile, const QString &targetFile);

public Q_SLOTS:
    /**
     * KAuth action to perform both prepare or move work based on given parameters.
     * We keep this code in one method to prevent multiple KAuth user queries during one save action.
     */
    static ActionReply savefile(const QVariantMap &args);
};

#endif
