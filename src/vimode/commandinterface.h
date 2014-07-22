/*  This file is part of the KDE libraries and the Kate part.
 *
 *  Copyright (C) 2013 Simon St James <kdedevel@etotheipiplusone.com>
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

#ifndef KATEVI_COMMAND_INTERFACE_H
#define KATEVI_COMMAND_INTERFACE_H

namespace KateVi
{
class GlobalState;
class InputModeManager;

class KateViCommandInterface
{
public:
    void setViGlobal(GlobalState *g)  { m_viGlobal = g; }
    void setViInputModeManager(InputModeManager *m)  { m_viInputModeManager = m; }

protected:
    GlobalState *m_viGlobal;
    InputModeManager *m_viInputModeManager;
};

}

#endif /* KATEVI_COMMAND_INTERFACE_H */
