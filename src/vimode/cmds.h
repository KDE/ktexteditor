/*  This file is part of the KDE libraries and the Kate part.
 *
 *  Copyright (C) 2003-2005 Anders Lund <anders@alweb.dk>
 *  Copyright (C) 2001-2010 Christoph Cullmann <cullmann@kde.org>
 *  Copyright (C) 2001 Charles Samuels <charles@kde.org>
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

#ifndef KATEVI_COMMANDS_H
#define KATEVI_COMMANDS_H

#include <KTextEditor/Command>
#include "kateregexpsearch.h"
#include <katesedcmd.h>
#include <vimode/commandinterface.h>
#include "mappings.h"

#include <QStringList>

namespace KTextEditor { class DocumentPrivate; }
class KCompletion;

namespace KateVi
{

/**
 * This KTextEditor::Command provides vi 'ex' commands
 */
class Commands : public KTextEditor::Command, public KateViCommandInterface
{
    Commands()
        : KTextEditor::Command (QStringList() << mappingCommands() << QLatin1String("d") << QLatin1String("delete") << QLatin1String("j") << QLatin1String("c") << QLatin1String("change") << QLatin1String("<") << QLatin1String(">") << QLatin1String("y") << QLatin1String("yank") <<
          QLatin1String("ma") << QLatin1String("mark") << QLatin1String("k"))
    {
        
    }
    static Commands *m_instance;

public:
    ~Commands()
    {
        m_instance = 0;
    }

    /**
     * execute command on given range
     * @param view view to use for execution
     * @param cmd cmd string
     * @param msg message returned from running the command
     * @param rangeStart first line in range
     * @param rangeEnd last line in range
     * @return success
     */
    bool exec(class KTextEditor::View *view, const QString &cmd, QString &msg,
              const KTextEditor::Range &range = KTextEditor::Range(-1, -0, -1, 0)) Q_DECL_OVERRIDE;

    bool supportsRange(const QString &range) Q_DECL_OVERRIDE;

    /** This command does not have help. @see KTextEditor::Command::help */
    bool help(class KTextEditor::View *, const QString &, QString &) Q_DECL_OVERRIDE
    {
        return false;
    }

    /**
     * Reimplement from KTextEditor::Command
     */
    KCompletion *completionObject(KTextEditor::View *, const QString &) Q_DECL_OVERRIDE;

    static Commands *self()
    {
        if (m_instance == 0) {
            m_instance = new Commands();
        }
        return m_instance;
    }
private:
    const QStringList &mappingCommands();
    Mappings::MappingMode modeForMapCommand(const QString &mapCommand);
    bool isMapCommandRecursive(const QString &mapCommand);
};

/**
 * Support vim/sed style search and replace
 * @author Charles Samuels <charles@kde.org>
 **/
class SedReplace : public KateCommands::SedReplace, public KateViCommandInterface
{
    SedReplace() { }
    static SedReplace *m_instance;

public:
    ~SedReplace()
    {
        m_instance = 0;
    }

    static SedReplace *self()
    {
        if (m_instance == 0) {
            m_instance = new SedReplace();
        }
        return m_instance;
    }

protected:
    bool interactiveSedReplace(KTextEditor::ViewPrivate *kateView, QSharedPointer<InteractiveSedReplacer> interactiveSedReplace) Q_DECL_OVERRIDE;
};

}

#endif /* KATEVI_COMMANDS_H */
