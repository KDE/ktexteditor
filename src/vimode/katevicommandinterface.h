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

#ifndef _KATE_VI_COMMAND_INTERFACE_
#define _KATE_VI_COMMAND_INTERFACE_

namespace KateVi {
class GlobalState;
}

class KateViInputModeManager;

class KateViCommandInterface {
public:
    void setViGlobal(KateVi::GlobalState *g)  { m_viGlobal = g; }
    void setViInputModeManager(KateViInputModeManager *m)  { m_viInputModeManager = m; }

protected:
    KateVi::GlobalState *m_viGlobal;
    KateViInputModeManager *m_viInputModeManager;
};

#endif