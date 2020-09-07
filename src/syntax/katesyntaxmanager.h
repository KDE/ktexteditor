/*
    SPDX-FileCopyrightText: 2001, 2002 Joseph Wenninger <jowenn@kde.org>
    SPDX-FileCopyrightText: 2001 Christoph Cullmann <cullmann@kde.org>
    SPDX-FileCopyrightText: 1999 Jochen Wilhelmy <digisnap@cs.tu-berlin.de>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#pragma once

#include "kateextendedattribute.h"
#include "katetextline.h"

#include <KActionMenu>
#include <KConfig>
#include <KSyntaxHighlighting/Definition>
#include <KSyntaxHighlighting/Repository>
#include <KSyntaxHighlighting/Theme>

#include <QDate>
#include <QHash>
#include <QObject>
#include <QPointer>
#include <QStringList>

#include <memory>

class KateHighlighting;

class KateHlManager : public QObject
{
    Q_OBJECT

public:
    KateHlManager() = default;

    static KateHlManager *self();

    KateHighlighting *getHl(int n);
    int nameFind(const QString &name);

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

    /**
     * Get repository (const).
     * @return repository
     */
    const KSyntaxHighlighting::Repository &repository() const
    {
        return m_repository;
    }

    /**
     * Sorted list of KSyntaxHighlighting themes.
     * @return list sorted by translated names for e.g. menus/...
     */
    QVector<KSyntaxHighlighting::Theme> sortedThemes() const;

private:
    /**
     * Syntax highlighting definitions.
     */
    KSyntaxHighlighting::Repository m_repository;

    /**
     * All loaded highlightings.
     */
    QHash<QString, std::shared_ptr<KateHighlighting>> m_hlDict;
};
