// This file is part of the KDE libraries
// Copyright (C) 2008 Paul Giannaros <paul@giannaros.org>
// Copyright (C) 2009-2018 Dominik Haumann <dhaumann@kde.org>
// Copyright (C) 2010 Joseph Wenninger <jowenn@kde.org>
//
// This library is free software; you can redistribute it and/or
// modify it under the terms of the GNU Library General Public
// License as published by the Free Software Foundation; either
// version 2 of the License, or (at your option) version 3.
//
// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// Library General Public License for more details.
//
// You should have received a copy of the GNU Library General Public License
// along with this library; see the file COPYING.LIB.  If not, write to
// the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
// Boston, MA 02110-1301, USA.

#ifndef KATE_SCRIPT_MANAGER_H
#define KATE_SCRIPT_MANAGER_H

#include <KTextEditor/Command>
#include <ktexteditor/cursor.h>

#include <QVector>

#include "katescript.h"
#include "kateindentscript.h"
#include "katecommandlinescript.h"

class QString;

/**
 * Manage the scripts on disks -- find them and query them.
 * Provides access to loaded scripts too.
 */
class KateScriptManager : public KTextEditor::Command
{
    Q_OBJECT

    KateScriptManager();
    static KateScriptManager *m_instance;

public:
    ~KateScriptManager() override;

    /** Get all scripts available in the command line */
    const QVector<KateCommandLineScript *> &commandLineScripts()
    {
        return m_commandLineScripts;
    }

    /**
     * Get an indentation script for the given language -- if there is more than
     * one result, this will return the script with the highest priority. If
     * both have the same priority, an arbitrary script will be returned.
     * If no scripts are found, 0 is returned.
     */
    KateIndentScript *indenter(const QString &language);

    //
    // KTextEditor::Command
    //
public:
    /**
     * execute command
     * @param view view to use for execution
     * @param cmd cmd string
     * @param errorMsg error to return if no success
     * @return success
     */
    bool exec(KTextEditor::View *view, const QString &cmd, QString &errorMsg, const KTextEditor::Range &) override;

    /**
     * get help for a command
     * @param view view to use
     * @param cmd cmd name
     * @param msg help message
     * @return help available or not
     */
    bool help(KTextEditor::View *view, const QString &cmd, QString &msg) override;

    static KateScriptManager *self()
    {
        if (m_instance == nullptr) {
            m_instance = new KateScriptManager();
        }
        return m_instance;
    }

    //
    // Helper methods
    //
public:
    /**
     * Collect all scripts.
     */
    void collect();

public:
    KateIndentScript *indentationScript(const QString &scriptname)
    {
        return m_indentationScriptMap.value(scriptname);
    }

    int indentationScriptCount()
    {
        return m_indentationScripts.size();
    }
    KateIndentScript *indentationScriptByIndex(int index)
    {
        return m_indentationScripts[index];
    }

public:
    /** explicitly reload all scripts */
    void reload();

Q_SIGNALS:
    /** this signal is emitted when all scripts are _deleted_ and reloaded again. */
    void reloaded();

private:
    /** List of all command line scripts */
    QVector<KateCommandLineScript *> m_commandLineScripts;

    /** list of all indentation scripts */
    QList<KateIndentScript *> m_indentationScripts;

    /** hash of all existing indenter scripts, hashes basename -> script */
    QHash<QString, KateIndentScript *> m_indentationScriptMap;

    /** Map of language to indent scripts */
    QHash<QString, QVector<KateIndentScript *> > m_languageToIndenters;
};

#endif

