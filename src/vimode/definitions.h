/*  SPDX-License-Identifier: LGPL-2.0-or-later

    Copyright (C) KDE Developers

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to
    the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
    Boston, MA 02110-1301, USA.
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
