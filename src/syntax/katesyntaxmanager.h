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

#include <QHash>
#include <QList>
#include <QMap>
#include <QVector>

#include <QDate>
#include <QObject>
#include <QPointer>
#include <QStringList>

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
