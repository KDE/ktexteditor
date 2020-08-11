/*
    SPDX-FileCopyrightText: 2013 Simon St James <kdedevel@etotheipiplusone.com>

    SPDX-License-Identifier: LGPL-2.0-or-later
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
    void setViGlobal(GlobalState *g)
    {
        m_viGlobal = g;
    }
    void setViInputModeManager(InputModeManager *m)
    {
        m_viInputModeManager = m;
    }

protected:
    GlobalState *m_viGlobal;
    InputModeManager *m_viInputModeManager;
};

}

#endif /* KATEVI_COMMAND_INTERFACE_H */
