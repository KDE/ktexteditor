/*
    SPDX-FileCopyrightText: KDE Developers

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef KATEVI_DEFINITIONS_H
#define KATEVI_DEFINITIONS_H

namespace KateVi
{
/**
 * The four vi modes supported by Kate's vi input mode
 */
enum ViMode { NormalMode, InsertMode, VisualMode, VisualLineMode, VisualBlockMode, ReplaceMode };

enum OperationMode { CharWise = 0, LineWise, Block };

const unsigned int EOL = 99999;
}

#endif // KATEVI_DEFINITIONS_H
