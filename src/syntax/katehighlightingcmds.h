/*  This file is part of the KDE libraries and the Kate part.
 *
 *  Copyright (C) 2014 Christoph Rüßler <christoph.ruessler@mailbox.org>
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

#ifndef KATE_HIGHLIGHTING_CMDS_H
#define KATE_HIGHLIGHTING_CMDS_H

#include <KTextEditor/Command>

namespace KateCommands
{

class Highlighting : public KTextEditor::Command
{
    Highlighting()
        : KTextEditor::Command({ QStringLiteral("reload-highlighting"), QStringLiteral("edit-highlighting") })
    {
    }

    static Highlighting* m_instance;

public:
    ~Highlighting() override
    {
        m_instance = nullptr;
    }
    
    static Highlighting *self()
    {
        if (m_instance == nullptr) {
            m_instance = new Highlighting();
        }
        return m_instance;
    }
    
    /**
     * execute command
     * @param view view to use for execution
     * @param cmd cmd string
     * @param errorMsg error to return if no success
     * @return success
     */
    bool exec(class KTextEditor::View *view, const QString &cmd, QString &errorMsg,
              const KTextEditor::Range &range = KTextEditor::Range::invalid()) override;
    
    /** This command does not have help. @see KTextEditor::Command::help */
    bool help(class KTextEditor::View *, const QString &, QString &) override;
};

}

#endif
