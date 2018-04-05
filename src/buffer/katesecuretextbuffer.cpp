/*  This file is part of the KTextEditor project.
 *
 *  Copyright (C) 2017 KDE Developers
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

#include "katesecuretextbuffer_p.h"

#include "config.h"

#ifndef Q_OS_WIN
#include <unistd.h>
#include <errno.h>
#endif

#include <QString>
#include <QFile>
#include <QDir>
#include <QFileInfo>
#include <QTemporaryFile>

KAUTH_HELPER_MAIN("org.kde.ktexteditor.katetextbuffer", SecureTextBuffer)

ActionReply SecureTextBuffer::savefile(const QVariantMap &args)
{
    const QString sourceFile = args[QStringLiteral("sourceFile")].toString();
    const QString targetFile = args[QStringLiteral("targetFile")].toString();
    const QByteArray checksum = args[QStringLiteral("checksum")].toByteArray();
    const uint ownerId = (uint) args[QStringLiteral("ownerId")].toInt();
    const uint groupId = (uint) args[QStringLiteral("groupId")].toInt();

    if (saveFileInternal(sourceFile, targetFile, checksum, ownerId, groupId)) {
        return ActionReply::SuccessReply();
    }

    return ActionReply::HelperErrorReply();
}

bool SecureTextBuffer::saveFileInternal(const QString &sourceFile, const QString &targetFile,
                                        const QByteArray &checksum, const uint ownerId, const uint groupId)
{
    QFileInfo targetFileInfo(targetFile);
    if (!QDir::setCurrent(targetFileInfo.dir().path())) {
        return false;
    }

    // get information about target file
    const QString targetFileName = targetFileInfo.fileName();
    targetFileInfo.setFile(targetFileName);
    const bool newFile = !targetFileInfo.exists();

    // open source and target file
    QFile readFile(sourceFile);
    //TODO use QSaveFile for saving contents and automatic atomic move on commit() when QSaveFile's security problem
    // (default temporary file permissions) is fixed
    //
    // We will first generate temporary filename and then use it relatively to prevent an attacker
    // to trick us to write contents to a different file by changing underlying directory.
    QTemporaryFile tempFile(targetFileName);
    if (!tempFile.open()) {
        return false;
    }
    tempFile.close();
    QString tempFileName = QFileInfo(tempFile).fileName();
    tempFile.setFileName(tempFileName);
    if (!readFile.open(QIODevice::ReadOnly) || !tempFile.open()) {
        return false;
    }
    const int tempFileDescriptor = tempFile.handle();

    // prepare checksum maker
    QCryptographicHash cryptographicHash(checksumAlgorithm);

    // copy contents
    char buffer[bufferLength];
    qint64 read = -1;
    while ((read = readFile.read(buffer, bufferLength)) > 0) {
        cryptographicHash.addData(buffer, read);
        if (tempFile.write(buffer, read) == -1) {
            return false;
        }
    }

    // check that copying was successful and checksum matched
    QByteArray localChecksum = cryptographicHash.result();
    if (read == -1 || localChecksum != checksum || !tempFile.flush()) {
        return false;
    }

    tempFile.close();

    if (newFile) {
        // ensure new file is readable by anyone
        tempFile.setPermissions(tempFile.permissions() | QFile::Permission::ReadGroup | QFile::Permission::ReadOther);
    } else {
        // ensure the same file permissions
        tempFile.setPermissions(targetFileInfo.permissions());
        // ensure file has the same owner and group as before
        setOwner(tempFileDescriptor, ownerId, groupId);
    }

    // rename temporary file to the target file
    if (moveFile(tempFileName, targetFileName)) {
        // temporary file was renamed, there is nothing to remove anymore
        tempFile.setAutoRemove(false);
        return true;
    }
    return false;
}

void SecureTextBuffer::setOwner(const int filedes, const uint ownerId, const uint groupId)
{
#ifndef Q_OS_WIN
    if (ownerId != (uint)-2 && groupId != (uint)-2) {
        const int result = fchown(filedes, ownerId, groupId);
        // set at least correct group if owner cannot be changed
        if (result != 0 && errno == EPERM) {
            fchown(filedes, getuid(), groupId);
        }
    }
#else
    // no-op for windows
#endif
}

bool SecureTextBuffer::moveFile(const QString &sourceFile, const QString &targetFile)
{
#if !defined(Q_OS_WIN) && !defined(Q_OS_ANDROID)
    const int result = std::rename(QFile::encodeName(sourceFile).constData(), QFile::encodeName(targetFile).constData());
    if (result == 0) {
        syncToDisk(QFile(targetFile).handle());
        return true;
    }
    return false;
#else
    // use racy fallback for windows
    QFile::remove(targetFile);
    return QFile::rename(sourceFile, targetFile);
#endif
}

void SecureTextBuffer::syncToDisk(const int fd)
{
#ifndef Q_OS_WIN
#if HAVE_FDATASYNC
    fdatasync(fd);
#else
    fsync(fd);
#endif
#else
    // no-op for windows
#endif
}

