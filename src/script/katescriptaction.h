/*
    SPDX-FileCopyrightText: 2010-2018 Dominik Haumann <dhaumann@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef KATE_SCRIPT_ACTION_H
#define KATE_SCRIPT_ACTION_H

#include "katecommandlinescript.h"

#include <KActionMenu>

#include <QAction>

namespace KTextEditor
{
class ViewPrivate;
}

/**
 * KateScriptAction is an action that executes a commandline-script
 * if triggered. It is shown in Tools > Scripts.
 */
class KateScriptAction : public QAction
{
    Q_OBJECT

public:
    KateScriptAction(const QString &cmd, const QJsonObject &action, KTextEditor::ViewPrivate *view);

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
    ~KateScriptActionMenu() override;

    void cleanup();

public Q_SLOTS:
    void repopulate();

private:
    KTextEditor::ViewPrivate *m_view;
    QList<QMenu *> m_menus;
    QList<QAction *> m_actions;
};

#endif
