/* This file is part of the KDE libraries
   Copyright (C) 2001,2002 Joseph Wenninger <jowenn@kde.org>
   Copyright (C) 2001 Christoph Cullmann <cullmann@kde.org>
   Copyright (C) 1999 Jochen Wilhelmy <digisnap@cs.tu-berlin.de>

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

#pragma once

#include "katetextline.h"
#include "kateextendedattribute.h"

#include <KSyntaxHighlighting/Definition>
#include <KSyntaxHighlighting/Repository>
#include <KConfig>
#include <KActionMenu>

#include <QVector>
#include <QList>
#include <QHash>
#include <QMap>

#include <QObject>
#include <QStringList>
#include <QPointer>
#include <QDate>

#include <memory>

class KateHighlighting;

class KateHlManager : public QObject
{
    Q_OBJECT

public:
    KateHlManager();

    static KateHlManager *self();

    inline KConfig *getKConfig()
    {
        return &m_config;
    }

    KateHighlighting *getHl(int n);
    int nameFind(const QString &name);

    void getDefaults(const QString &schema, KateAttributeList &, KConfig *cfg = nullptr);
    void setDefaults(const QString &schema, KateAttributeList &, KConfig *cfg = nullptr);

    void reload();

Q_SIGNALS:
    void changed();

//
// methods to get the default style count + names
//
public:
    /**
     * Return the number of default styles.
     */
    static int defaultStyleCount();

    /**
     * Return the name of default style @p n. If @p translateNames is @e true,
     * the default style name is translated.
     */
    static QString defaultStyleName(int n, bool translateNames = false);

    /**
     * Return the index for the default style @p name.
     * @param name @e untranslated default style
     * @see KTextEditor::DefaultStyles)
     */
    static int defaultStyleNameToIndex(const QString &name);

    /**
     * Get the mode list
     * @return mode list
     */
    QVector<KSyntaxHighlighting::Definition> modeList() const
    {
        return m_repository.definitions();
    }

    /**
     * Get repository.
     * @return repository
     */
    KSyntaxHighlighting::Repository &repository()
    {
        return m_repository;
    }

private:
    /**
     * Syntax highlighting definitions.
     */
    KSyntaxHighlighting::Repository m_repository;

    /**
     * All loaded highlightings.
     */
    QHash<QString, std::shared_ptr<KateHighlighting>> m_hlDict;

    /**
     * katesyntaxhighlightingrc config file
     */
    KConfig m_config;
};
