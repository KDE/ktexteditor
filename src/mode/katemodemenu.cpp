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

//BEGIN Includes
#include "katemodemenu.h"

#include "katedocument.h"
#include "kateconfig.h"
#include "kateview.h"
#include "kateglobal.h"
#include "katesyntaxmanager.h"
#include "katepartdebug.h"
//END Includes

void KateModeMenu::init()
{
    m_doc = nullptr;

    connect(menu(), SIGNAL(triggered(QAction*)), this, SLOT(setType(QAction*)));

    connect(menu(), SIGNAL(aboutToShow()), this, SLOT(slotAboutToShow()));

    m_actionGroup = new QActionGroup(menu());
}

KateModeMenu::~KateModeMenu()
{
    qDeleteAll(subMenus);
}

void KateModeMenu::updateMenu(KTextEditor::Document *doc)
{
    m_doc = static_cast<KTextEditor::DocumentPrivate *>(doc);
}

void KateModeMenu::slotAboutToShow()
{
    KTextEditor::DocumentPrivate *doc = m_doc;
    int count = KTextEditor::EditorPrivate::self()->modeManager()->list().count();

    for (int z = 0; z < count; z++) {
        QString nameRaw = KTextEditor::EditorPrivate::self()->modeManager()->list().at(z)->name;
        QString hlName = KTextEditor::EditorPrivate::self()->modeManager()->list().at(z)->nameTranslated();
        QString hlSection = KTextEditor::EditorPrivate::self()->modeManager()->list().at(z)->sectionTranslated();

        if (!hlSection.isEmpty() && !names.contains(hlName)) {
            if (!subMenusName.contains(hlSection)) {
                subMenusName << hlSection;
                QMenu *qmenu = new QMenu(hlSection);
                connect(qmenu, SIGNAL(triggered(QAction*)), this, SLOT(setType(QAction*)));
                subMenus.append(qmenu);
                menu()->addMenu(qmenu);
            }

            int m = subMenusName.indexOf(hlSection);
            names << hlName;
            QAction *action = subMenus.at(m)->addAction(hlName);
            m_actionGroup->addAction(action);
            action->setCheckable(true);
            action->setData(nameRaw);
        } else if (!names.contains(hlName)) {
            names << hlName;

            disconnect(menu(), SIGNAL(triggered(QAction*)), this, SLOT(setType(QAction*)));
            connect(menu(), SIGNAL(triggered(QAction*)), this, SLOT(setType(QAction*)));

            QAction *action = menu()->addAction(hlName);
            m_actionGroup->addAction(action);
            action->setCheckable(true);
            action->setData(nameRaw);
        }
    }

    if (!doc) {
        return;
    }

    for (int i = 0; i < subMenus.count(); i++) {
        QList<QAction *> actions = subMenus.at(i)->actions();
        for (int j = 0; j < actions.count(); ++j) {
            actions[ j ]->setChecked(false);
        }
    }

    QList<QAction *> actions = menu()->actions();
    for (int i = 0; i < actions.count(); ++i) {
        actions[ i ]->setChecked(false);
    }

    if (doc->fileType().isEmpty() || doc->fileType() == QLatin1String("Normal")) {
        for (int i = 0; i < actions.count(); ++i) {
            if (actions[ i ]->data().toString() == QLatin1String("Normal")) {
                actions[ i ]->setChecked(true);
            }
        }
    } else {
        if (!doc->fileType().isEmpty()) {
            const KateFileType &t = KTextEditor::EditorPrivate::self()->modeManager()->fileType(doc->fileType());
            int i = subMenusName.indexOf(t.section);
            if (i >= 0 && subMenus.at(i)) {
                QList<QAction *> actions = subMenus.at(i)->actions();
                for (int j = 0; j < actions.count(); ++j) {
                    if (actions[ j ]->data().toString() == doc->fileType()) {
                        actions[ j ]->setChecked(true);
                    }
                }
            } else {
                QList<QAction *> actions = menu()->actions();
                for (int j = 0; j < actions.count(); ++j) {
                    if (actions[ j ]->data().toString().isEmpty()) {
                        actions[ j ]->setChecked(true);
                    }
                }
            }
        }
    }
}

void KateModeMenu::setType(QAction *action)
{
    KTextEditor::DocumentPrivate *doc = m_doc;

    if (doc) {
        doc->updateFileType(action->data().toString(), true);
    }
}

