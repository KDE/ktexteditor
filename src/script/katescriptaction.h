/* This file is part of the KDE libraries
   Copyright (C) 2010-2018 Dominik Haumann <dhaumann@kde.org>
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

#ifndef KATE_SCRIPT_ACTION_H
#define KATE_SCRIPT_ACTION_H

#include "katecommandlinescript.h"

#include <KActionMenu>

#include <QAction>

namespace KTextEditor { class ViewPrivate; }

/**
 * KateScriptAction is an action that executes a commandline-script
 * if triggered. It is shown in Tools > Scripts.
 */
class KateScriptAction : public QAction
{
    Q_OBJECT

public:
    KateScriptAction(const QString &cmd, const QJsonObject &action, KTextEditor::ViewPrivate *view);
    virtual ~KateScriptAction();

public Q_SLOTS:
    void exec();

private:
    KTextEditor::ViewPrivate *m_view;
    QString m_command;
    bool m_interactive;
};

/**
 * Tools > Scripts menu
 * This menu is filled with the command line scripts exported
 * via the scripting support.
 */
class KateScriptActionMenu : public KActionMenu
{
    Q_OBJECT

public:
    KateScriptActionMenu(KTextEditor::ViewPrivate *view, const QString &text);
    ~KateScriptActionMenu();

    void cleanup();

public Q_SLOTS:
    void repopulate();

private:
    KTextEditor::ViewPrivate *m_view;
    QList<QMenu *> m_menus;
    QList<QAction *> m_actions;
};

#endif

