/*  This file is part of the KDE libraries and the Kate part.
 *
 *  Copyright (C) 2009-2018 Dominik Haumann <dhaumann@kde.org>
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

#ifndef KATE_COMMANDLINE_SCRIPT_H
#define KATE_COMMANDLINE_SCRIPT_H

#include "katescript.h"
#include "kateview.h"

#include <KTextEditor/Command>

#include <QJsonArray>

class KateCommandLineScriptHeader
{
public:
    KateCommandLineScriptHeader()
    {}

    inline void setFunctions(const QStringList &functions)
    {
        m_functions = functions;
    }
    inline const QStringList &functions() const
    {
        return m_functions;
    }

    inline void setActions(const QJsonArray &actions)
    {
        m_actions = actions;
    }
    inline const QJsonArray &actions() const
    {
        return m_actions;
    }

private:
    QStringList m_functions; ///< the functions the script contains
    QJsonArray m_actions; ///< the action for this script
};

/**
 * A specialized class for scripts that are of type ScriptType::Indentation.
 */
class KateCommandLineScript : public KateScript, public KTextEditor::Command
{
public:
    KateCommandLineScript(const QString &url, const KateCommandLineScriptHeader &header);
    ~KateCommandLineScript() override;

    const KateCommandLineScriptHeader &commandHeader();

    bool callFunction(const QString &cmd, const QStringList args, QString &errorMessage);

    //
    // KTextEditor::Command interface
    //
public:
    bool help(KTextEditor::View *view, const QString &cmd, QString &msg) override;
    bool exec(KTextEditor::View *view, const QString &cmd, QString &msg, const KTextEditor::Range &range = KTextEditor::Range::invalid()) override;
    bool supportsRange(const QString &cmd) override;

private:
    KateCommandLineScriptHeader m_commandHeader;
};

#endif
