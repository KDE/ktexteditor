/*
    SPDX-FileCopyrightText: 2001-2003 Christoph Cullmann <cullmann@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

// BEGIN Includes
#include "katehighlightmenu.h"

#include "kateconfig.h"
#include "katedocument.h"
#include "kateglobal.h"
#include "katepartdebug.h"
#include "katesyntaxmanager.h"
#include "kateview.h"
#include <KLocalizedString>
// END Includes

KateHighlightingMenu::~KateHighlightingMenu()
{
    qDeleteAll(subMenus);
}

void KateHighlightingMenu::init()
{
    m_doc = nullptr;

    connect(menu(), SIGNAL(aboutToShow()), this, SLOT(slotAboutToShow()));
    m_actionGroup = new QActionGroup(menu());
}

void KateHighlightingMenu::updateMenu(KTextEditor::DocumentPrivate *doc)
{
    m_doc = doc;
}

void KateHighlightingMenu::slotAboutToShow()
{
    for (const auto &hl : KateHlManager::self()->modeList()) {
        QString hlName = hl.translatedName();
        QString hlSection = hl.translatedSection();
        if (hlName == QLatin1String("None"))
            hlName = i18n("None");

        if (!hl.isHidden() && !hlName.isEmpty()) {
            if (!hlSection.isEmpty() && !names.contains(hlName)) {
                if (!subMenusName.contains(hlSection)) {
                    subMenusName << hlSection;
                    QMenu *qmenu = new QMenu(QLatin1Char('&') + hlSection);
                    subMenus.append(qmenu);
                    menu()->addMenu(qmenu);
                }

                int m = subMenusName.indexOf(hlSection);
                names << hlName;
                QAction *a = subMenus.at(m)->addAction(QLatin1Char('&') + hlName, this, SLOT(setHl()));
                m_actionGroup->addAction(a);
                a->setData(hl.name());
                a->setCheckable(true);
                subActions.append(a);
            } else if (!names.contains(hlName)) {
                names << hlName;
                QAction *a = menu()->addAction(QLatin1Char('&') + hlName, this, SLOT(setHl()));
                m_actionGroup->addAction(a);
                a->setData(hl.name());
                a->setCheckable(true);
                subActions.append(a);
            }
        }
    }

    if (!m_doc) {
        return;
    }
    QString mode = m_doc->highlightingMode();
    for (int i = 0; i < subActions.count(); i++) {
        subActions[i]->setChecked(subActions[i]->data().toString() == mode);
    }
}

void KateHighlightingMenu::setHl()
{
    if (!m_doc || !sender()) {
        return;
    }
    QAction *action = qobject_cast<QAction *>(sender());
    if (!action) {
        return;
    }
    QString mode = action->data().toString();
    m_doc->setHighlightingMode(mode);

    // use change, honor this
    m_doc->setDontChangeHlOnSave();
}
