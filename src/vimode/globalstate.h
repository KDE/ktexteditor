/*  This file is part of the KDE libraries and the Kate part.
 *
 *  Copyright (C) 2008 Erlend Hamberg <ehamberg@gmail.com>
 *  Copyright (C) 2011 Svyatoslav Kuzmich <svatoslav1@gmail.com>
 *  Copyright (C) 2012 - 2013 Simon St James <kdedevel@etotheipiplusone.com>
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

#ifndef KATEVI_GLOBAL_STATE_H
#define KATEVI_GLOBAL_STATE_H

#include <KSharedConfig>
#include <ktexteditor_export.h>

namespace KateVi
{
class History;
class Macros;
class Mappings;
class Registers;

class KTEXTEDITOR_EXPORT GlobalState
{
public:
    explicit GlobalState();
    ~GlobalState();

    void writeConfig(KConfig *config) const;
    void readConfig(const KConfig *config);

    inline KateVi::Macros *macros() const { return m_macros; }
    inline KateVi::Mappings *mappings() const { return m_mappings; }
    inline KateVi::Registers *registers() const { return m_registers; }

    inline KateVi::History *searchHistory() const { return m_searchHistory; }
    inline KateVi::History *commandHistory() const { return m_commandHistory; }
    inline KateVi::History *replaceHistory() const { return m_replaceHistory; }

private:
    KSharedConfigPtr config() const;

private:
    KateVi::Macros *m_macros;
    KateVi::Mappings *m_mappings;
    KateVi::Registers *m_registers;

    KateVi::History *m_searchHistory;
    KateVi::History *m_commandHistory;
    KateVi::History *m_replaceHistory;
};
}

#endif
