/* This file is part of the KDE libraries
   Copyright (C) 2001-2003 Christoph Cullmann <cullmann@kde.org>
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

//BEGIN Includes
#include "katehighlightmenu.h"

#include "katedocument.h"
#include "kateconfig.h"
#include "kateview.h"
#include "kateglobal.h"
#include "katesyntaxmanager.h"
#include "katepartdebug.h"
//END Includes

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
        QString hlSection = hl.section();

        if (!hl.isHidden()) {
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

