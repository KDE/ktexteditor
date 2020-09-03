/*
    SPDX-FileCopyrightText: 2001-2003 Christoph Cullmann <cullmann@kde.org>
    SPDX-FileCopyrightText: 2002, 2003 Anders Lund <anders.lund@lund.tdcadsl.dk>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef KATE_SCHEMA_H
#define KATE_SCHEMA_H

#include <KSyntaxHighlighting/Theme>

#include <KActionMenu>
#include <KConfig>
#include <KConfigGroup>
#include <KLocalizedString>

#include <QPointer>
#include <QStringList>

namespace KTextEditor
{
class ViewPrivate;
}
class QActionGroup;

class KateSchema
{
public:
    QString rawName;
    KSyntaxHighlighting::Theme theme;
    KConfigGroup config;

    /**
     * construct translated name for shipped schemas
     */
    QString translatedName() const
    {
        // for KSyntaxHighlighting themes, we can have some proper translated names
        return theme.isValid() ? theme.translatedName() : rawName;
    }
};

class KateSchemaManager
{
public:
    KateSchemaManager();

    /**
     * Config
     */
    KConfig &config()
    {
        return m_config;
    }

    /**
     * return kconfiggroup for the given schema
     */
    KConfigGroup schema(const QString &name);

    /**
     * return schema data for on schema
     */
    KateSchema schemaData(const QString &name);

    /**
     * does the given schema exist?
     */
    bool exists(const QString &name);

    /**
     * Constructs list of schemas atm known in config object
     */
    QList<KateSchema> list();

private:
    KConfig m_config;
};

class KateViewSchemaAction : public KActionMenu
{
    Q_OBJECT

public:
    KateViewSchemaAction(const QString &text, QObject *parent)
        : KActionMenu(text, parent)
    {
        init();
        setDelayed(false);
    }

    void updateMenu(KTextEditor::ViewPrivate *view);

private:
    void init();

    QPointer<KTextEditor::ViewPrivate> m_view;
    QStringList names;
    QActionGroup *m_group;
    int last;

public Q_SLOTS:
    void slotAboutToShow();

private Q_SLOTS:
    void setSchema();
};

#endif
