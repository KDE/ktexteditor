/*  This file is part of the KDE libraries and the Kate part.
 *
 *  Copyright (C) 2008-2011 Erlend Hamberg <ehamberg@gmail.com>
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

#include "globalstate.h"
#include "history.h"
#include "macros.h"
#include "mappings.h"
#include "registers.h"

#include <kconfiggroup.h>

using namespace KateVi;

GlobalState::GlobalState()
{
    m_macros = new Macros();
    m_mappings = new Mappings();
    m_registers = new Registers();
    m_searchHistory = new History();
    m_replaceHistory = new History();
    m_commandHistory = new History();

    readConfig(config().data());
}

GlobalState::~GlobalState()
{
    writeConfig(config().data());
    config().data()->sync();

    delete m_searchHistory;
    delete m_replaceHistory;
    delete m_commandHistory;
    delete m_macros;
    delete m_mappings;
    delete m_registers;
}

void GlobalState::writeConfig(KConfig *configFile) const
{
    // FIXME: use own groups instead of one big group!
    KConfigGroup config(configFile, "Kate Vi Input Mode Settings");
    m_macros->writeConfig(config);
    m_mappings->writeConfig(config);
    m_registers->writeConfig(config);
}

void GlobalState::readConfig(const KConfig *configFile)
{
    // FIXME: use own groups instead of one big group!
    const KConfigGroup config(configFile, "Kate Vi Input Mode Settings");

    m_macros->readConfig(config);
    m_mappings->readConfig(config);
    m_registers->readConfig(config);
}

KSharedConfigPtr GlobalState::config() const
{
    return KSharedConfig::openConfig(QStringLiteral("katevirc"));
}
