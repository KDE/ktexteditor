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

#ifndef Q_OS_WIN
#include <unistd.h>
#include <errno.h>
#endif

#include <QString>
#include <QFile>
#include <QTemporaryFile>
#include <QSaveFile>

KAUTH_HELPER_MAIN("org.kde.ktexteditor.katetextbuffer", SecureTextBuffer)

ActionReply SecureTextBuffer::savefile(const QVariantMap &args)
{
    const ActionMode actionMode = static_cast<ActionMode>(args[QLatin1String("actionMode")].toInt());
    const QString targetFile = args[QLatin1String("targetFile")].toString();
    const uint ownerId = (uint) args[QLatin1String("ownerId")].toInt();

    if (actionMode == ActionMode::Prepare) {

        const QString temporaryFile = prepareTempFileInternal(targetFile, ownerId);

        if (temporaryFile.isEmpty()) {
            return ActionReply::HelperErrorReply();
        }

        ActionReply reply;
        reply.addData(QLatin1String("temporaryFile"), temporaryFile);

        return reply;

    }

    if (actionMode == ActionMode::Move) {

        const QString sourceFile = args[QLatin1String("sourceFile")].toString();
        const uint groupId = (uint) args[QLatin1String("groupId")].toInt();

        if (moveFileInternal(sourceFile, targetFile, ownerId, groupId)) {
            return ActionReply::SuccessReply();
        }
    }

    return ActionReply::HelperErrorReply();
}

bool SecureTextBuffer::moveFileInternal(const QString &sourceFile, const QString &targetFile, const uint ownerId, const uint groupId)
{
    const bool newFile = !QFile::exists(targetFile);
    bool atomicRenameSucceeded = false;

    /**
     * There is no atomic rename operation publicly exposed by Qt.
     *
     * We use std::rename for UNIX and for now no-op for windows (triggers fallback).
     *
     * As fallback we are copying source file to destination with the help of QSaveFile
     * to ensure targetFile is overwritten safely.
     */
#ifndef Q_OS_WIN
    const int result = std::rename(QFile::encodeName(sourceFile).constData(), QFile::encodeName(targetFile).constData());
    if (result == 0) {
        syncToDisk(QFile(targetFile).handle());
        atomicRenameSucceeded = true;
    }
#else
    atomicRenameSucceeded = false;
#endif

    if (!atomicRenameSucceeded) {
        // as fallback copy the temporary file to the target with help of QSaveFile
        QFile readFile(sourceFile);
        QSaveFile saveFile(targetFile);
        if (!readFile.open(QIODevice::ReadOnly) || !saveFile.open(QIODevice::WriteOnly)) {
            return false;
        }
        char buffer[bufferLength];
        qint64 read = -1;
        while ((read = readFile.read(buffer, bufferLength)) > 0) {
            if (saveFile.write(buffer, read) == -1) {
                return false;
            }
        }
        if (read == -1 || !saveFile.commit()) {
            return false;
        }
    }

    if (!newFile) {
        // ensure file has the same owner and group as before
        setOwner(targetFile, ownerId, groupId);
    }

    return true;
}

QString SecureTextBuffer::prepareTempFileInternal(const QString &targetFile, const uint ownerId)
{
    QTemporaryFile tempFile(targetFile);
    if (!tempFile.open()) {
        return QString();
    }
    tempFile.setAutoRemove(false);
    setOwner(tempFile.fileName(), ownerId, -1);
    return tempFile.fileName();
}

void SecureTextBuffer::setOwner(const QString &filename, const uint ownerId, const uint groupId)
{
#ifndef Q_OS_WIN
    if (ownerId != (uint)-2 && groupId != (uint)-2) {
        const int result = chown(QFile::encodeName(filename).constData(), ownerId, groupId);
        // set at least correct group if owner cannot be changed
        if (result != 0 && errno == EPERM) {
            chown(QFile::encodeName(filename).constData(), getuid(), groupId);
        }
    }
#else
    // no-op for windows
#endif
}

void SecureTextBuffer::syncToDisk(const int fd)
{
#ifndef Q_OS_WIN
#ifdef HAVE_FDATASYNC
    fdatasync(fd);
#else
    fsync(fd);
#endif
#else
    // no-op for windows
#endif
}