/*  This file is part of the KDE libraries and the Kate part.
 *
 *  Copyright (C) 2001-2010 Christoph Cullmann <cullmann@kde.org>
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

#ifndef KATE_MODEMANAGER_H
#define KATE_MODEMANAGER_H

#include <QStringList>
#include <QPointer>
#include <QHash>

#include <KLocalizedString>

#include "katedialogs.h"

namespace KTextEditor { class DocumentPrivate; }

class KateFileType
{
public:
    int number = -1;
    QString name;
    QString section;
    QStringList wildcards;
    QStringList mimetypes;
    int priority = 0;
    QString varLine;
    QString hl;
    bool hlGenerated = false;
    QString version;
    QString indenter;

    QString translatedName;
    QString translatedSection;

    QString nameTranslated() const
    {
        return translatedName.isEmpty() ? name : translatedName;
    }

    QString sectionTranslated() const
    {
        return translatedSection.isEmpty() ? section : translatedSection;
    }

    KateFileType()

    {}
};

class KateModeManager
{
public:
    KateModeManager();
    ~KateModeManager();

    KateModeManager(const KateModeManager &) = delete;
    KateModeManager& operator=(const KateModeManager &) = delete;

    /**
     * File Type Config changed, update all docs (which will take care of views/renderers)
     */
    void update();

    void save(const QList<KateFileType *> &v);

    /**
     * get the right fileType for the given document
     * -1 if none found !
     */
    QString fileType(KTextEditor::DocumentPrivate *doc, const QString &fileToReadFrom);

    /**
     * Don't store the pointer somewhere longer times, won't be valid after the next update()
     */
    const KateFileType &fileType(const QString &name) const;

    /**
     * Don't modify
     */
    const QList<KateFileType *> &list() const
    {
        return m_types;
    }

private:
    QString wildcardsFind(const QString &fileName);

private:
    QList<KateFileType *> m_types;
    QHash<QString, KateFileType *> m_name2Type;

};

#endif

