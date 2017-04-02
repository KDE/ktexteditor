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

#ifndef KATE_SECURE_TEXTBUFFER_P_H
#define KATE_SECURE_TEXTBUFFER_P_H

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

    /**
     * We support Prepare action for temporary file creation
     * and Move action for moving final file to its destination
     */
    enum ActionMode {
        Prepare = 1,
        Move = 2
    };

    SecureTextBuffer() {}

    ~SecureTextBuffer() {}

    /**
     * Common helper methods
     */
    static void setOwner(const QString &filename, const uint ownerId, const uint groupId);
    static void syncToDisk(const int fd);

private:
    static const qint64 bufferLength = 4096;

    /**
     * Creates temporary file based on given target file path.
     * Temporary file is set to not be deleted on object destroy
     * so KTextEditor can save contents in it.
     */
    static QString prepareTempFileInternal(const QString &targetFile, const uint ownerId);

    /**
     * Move file to its given destination and set owner.
     */
    static bool moveFileInternal(const QString &sourceFile, const QString &targetFile, const uint ownerId, const uint groupId);

public Q_SLOTS:
    /**
     * KAuth action to perform both prepare or move work based on given parameters.
     * We keep this code in one method to prevent multiple KAuth user queries during one save action.
     */
    static ActionReply savefile(const QVariantMap &args);

};

#endif
