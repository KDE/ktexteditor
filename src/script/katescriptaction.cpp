/* This file is part of the KDE libraries
   Copyright (C) 2010 Dominik Haumann <dhaumann kde org>
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

#include "katescriptaction.h"
#include "katecommandlinescript.h"
#include "katescriptmanager.h"
#include "kateview.h"
#include "katedocument.h"
#include "kateglobal.h"
#include "kateviewhelpers.h"
#include "katepartdebug.h"
#include "katecmd.h"
#include "kateabstractinputmode.h"

#include <KXMLGUIFactory>
#include <KActionCollection>

//BEGIN KateScriptAction
KateScriptAction::KateScriptAction(const ScriptActionInfo &info, KTextEditor::ViewPrivate *view)
    : QAction(info.text(), view)
    , m_view(view)
    , m_command(info.command())
    , m_interactive(info.interactive())
{
    if (!info.icon().isEmpty()) {
        setIcon(QIcon::fromTheme(info.icon()));
    }

    connect(this, SIGNAL(triggered(bool)), this, SLOT(exec()));
}

KateScriptAction::~KateScriptAction()
{
}

void KateScriptAction::exec()
{
    if (m_interactive) {
        m_view->currentInputMode()->launchInteractiveCommand(m_command + QLatin1Char(' '));
    } else {
        KTextEditor::Command *p = KateCmd::self()->queryCommand(m_command);
        if (p) {
            QString msg;
            p->exec(m_view, m_command, msg);
        }
    }
}
//END KateScriptAction

//BEGIN KateScriptActionMenu
KateScriptActionMenu::KateScriptActionMenu(KTextEditor::ViewPrivate *view, const QString &text)
    : KActionMenu(QIcon::fromTheme(QLatin1String("code-context")), text, view)
    , m_view(view)
{
    repopulate();

    // on script-reload signal, repopulate script menu
    connect(KTextEditor::EditorPrivate::self()->scriptManager(), SIGNAL(reloaded()),
            this, SLOT(repopulate()));
}

KateScriptActionMenu::~KateScriptActionMenu()
{
    cleanup();
}

void KateScriptActionMenu::cleanup()
{
    // delete menus and actions for real
    qDeleteAll(m_menus);
    m_menus.clear();

    qDeleteAll(m_actions);
    m_actions.clear();
}

void KateScriptActionMenu::repopulate()
{
    // if the view is already hooked into the GUI, first remove it
    // now and add it later, so that the changes we do here take effect
    KXMLGUIFactory *viewFactory = m_view->factory();
    if (viewFactory) {
        viewFactory->removeClient(m_view);
    }

    // remove existing menu actions
    cleanup();

    // now add all command line script commands
    QVector<KateCommandLineScript *> scripts =
        KTextEditor::EditorPrivate::self()->scriptManager()->commandLineScripts();

    QHash<QString, QMenu *> menus;

    foreach (KateCommandLineScript *script, scripts) {

        const QStringList &cmds = script->cmds();
        foreach (const QString &cmd, cmds) {

            ScriptActionInfo info = script->actionInfo(cmd);
            if (!info.isValid()) {
                continue;
            }

            QMenu *m = menu();

            // show in a category submenu?
            if (!info.category().isEmpty()) {
                m = menus[info.category()];
                if (!m) {
                    m = menu()->addMenu(info.category());
                    menus.insert(info.category(), m);
                    m_menus.append(m);
                }
            }

            // create action + add to menu
            QAction *a = new KateScriptAction(info, m_view);
            m->addAction(a);
            m_view->actionCollection()->addAction(QLatin1String("tools_scripts_") + cmd, a);
            if (!info.shortcut().isEmpty()) {
                m_view->actionCollection()->setDefaultShortcut(a, info.shortcut());
            }
            
            m_actions.append(a);
        }
    }

    // finally add the view to the xml factory again, if it initially was there
    if (viewFactory) {
        viewFactory->addClient(m_view);
    }
}

//END KateScriptActionMenu

