/*
    SPDX-FileCopyrightText: 2007, 2008 Matthew Woehlke <mw_triad@users.sourceforge.net>
    SPDX-FileCopyrightText: 2001-2003 Christoph Cullmann <cullmann@kde.org>
    SPDX-FileCopyrightText: 2002, 2003 Anders Lund <anders.lund@lund.tdcadsl.dk>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

// BEGIN Includes
#include "kateschema.h"

#include "kateconfig.h"
#include "kateglobal.h"
#include "katehighlight.h"
#include "katepartdebug.h"
#include "katerenderer.h"
#include "kateview.h"

#include <map>

// END

// BEGIN KateSchemaManager
KateSchemaManager::KateSchemaManager()
    : m_config(KTextEditor::EditorPrivate::unitTestMode() ? QString() : QStringLiteral("katethemerc"), KConfig::SimpleConfig) // skip config for unit tests!
{
    // convert old to new theme names once
    // this is needed for the conversion to use KSyntaxHighlighting themes
    // we do this conversion once, use printing theme to remember this
    // we copy once all old config over from kateschemarc to new katethemerc with renaming
    if (!m_config.hasGroup("Printing")) {
        // mapping for renaming
        std::map<QString, QString> renamed;
        renamed.emplace(QStringLiteral("Normal"), KTextEditor::EditorPrivate::self()->hlManager()->repository().defaultTheme(KSyntaxHighlighting::Repository::LightTheme).name());
        renamed.emplace(QStringLiteral("Solarized (light)"), QStringLiteral("Solarized Light"));
        renamed.emplace(QStringLiteral("Solarized (dark)"), QStringLiteral("Solarized Dark"));
        renamed.emplace(QStringLiteral("Vim (dark)"), QStringLiteral("Vim Dark"));

        // open old config, copy over all groups we have, rename some
        KConfig oldConfig(QStringLiteral("kateschemarc"), KConfig::SimpleConfig);
        for (const auto &oldGroupName : oldConfig.groupList()) {
            // get old group
            KConfigGroup oldGroup = oldConfig.group(oldGroupName);

            // create new group with either 1:1 or renamed name
            KConfigGroup newGroup = m_config.group((renamed.find(oldGroupName) == renamed.end()) ? oldGroupName : renamed[oldGroupName]);

            // copy all data to new group
            oldGroup.copyTo(&newGroup);
        }

        // add dummy element to Printing group to have it, like we do in kateschemaconfig.cpp
        KConfigGroup printing = m_config.group("Printing");
        printing.writeEntry("dummy", "prevent-empty-group");
    }
}

KConfigGroup KateSchemaManager::schema(const QString &name)
{
    return m_config.group(name);
}

KateSchema KateSchemaManager::schemaData(const QString &name)
{
    // normal => schema just a name to some config group
    KateSchema schema;
    schema.rawName = name;
    schema.config = m_config.group(name);

    // we might be a theme derived from KSyntaxHighlighting data
    auto &repo = KTextEditor::EditorPrivate::self()->hlManager()->repository();
    schema.theme = repo.theme(name);

    // mark some stuff as undeleteable
    if (schema.theme.isValid()) {
        schema.notDeletable = true;
    } else {
        // KDE default color scheme thema?
        if (schema.rawName == QLatin1String("KDE")) {
            schema.notDeletable = true;
        }
    }

    return schema;
}

bool KateSchemaManager::exists(const QString &name)
{
    // either some config data around of this is a KSyntaxHighlighting theme
    return m_config.hasGroup(name) || KTextEditor::EditorPrivate::self()->hlManager()->repository().theme(name).isValid();
}

static bool schemasCompare(const KateSchema &s1, const KateSchema &s2)
{
    return s1.translatedName().localeAwareCompare(s2.translatedName()) < 0;
}

QList<KateSchema> KateSchemaManager::list()
{
    // get KSyntaxHighlighting themes
    QList<KateSchema> schemas;
    QSet<QString> defaultThemes;
    for (const auto &theme : KTextEditor::EditorPrivate::self()->hlManager()->repository().themes()) {
        schemas.append(schemaData(theme.name()));
        defaultThemes.insert(theme.name());
    }

    // add default KDE color schema
    schemas.append(schemaData(QStringLiteral("KDE")));
    defaultThemes.insert(QStringLiteral("KDE"));

    // get extra themes that got configured in KConfig data
    const auto names = m_config.groupList();
    for (const QString &s : names) {
        if (!defaultThemes.contains(s)) {
            schemas.append(schemaData(s));
        }
    }

    // sort: prio given by default schema and name
    std::sort(schemas.begin(), schemas.end(), schemasCompare);
    return schemas;
}
// END

// BEGIN SCHEMA ACTION -- the 'View->Color theme' menu action
void KateViewSchemaAction::init()
{
    m_group = nullptr;
    m_view = nullptr;
    last = 0;

    connect(menu(), SIGNAL(aboutToShow()), this, SLOT(slotAboutToShow()));
}

void KateViewSchemaAction::updateMenu(KTextEditor::ViewPrivate *view)
{
    m_view = view;
}

void KateViewSchemaAction::slotAboutToShow()
{
    KTextEditor::ViewPrivate *view = m_view;

    QList<KateSchema> schemas = KTextEditor::EditorPrivate::self()->schemaManager()->list();

    if (!m_group) {
        m_group = new QActionGroup(menu());
        m_group->setExclusive(true);
    }

    for (int z = 0; z < schemas.count(); z++) {
        QString hlName = schemas[z].translatedName();

        if (!names.contains(hlName)) {
            names << hlName;
            QAction *a = menu()->addAction(hlName, this, SLOT(setSchema()));
            a->setData(schemas[z].rawName);
            a->setCheckable(true);
            a->setActionGroup(m_group);
        }
    }

    if (!view) {
        return;
    }

    QString id = view->renderer()->config()->schema();
    const auto menuActions = menu()->actions();
    for (QAction *a : menuActions) {
        a->setChecked(a->data().toString() == id);
    }
}

void KateViewSchemaAction::setSchema()
{
    QAction *action = qobject_cast<QAction *>(sender());

    if (!action) {
        return;
    }
    QString mode = action->data().toString();

    KTextEditor::ViewPrivate *view = m_view;

    if (view) {
        view->renderer()->config()->setSchema(mode);
    }
}
// END SCHEMA ACTION
