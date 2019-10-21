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

#ifndef KATEVI_COMPLETIONREPLAYER_H
#define KATEVI_COMPLETIONREPLAYER_H

#include "completion.h"

#include <QStack>

namespace KTextEditor
{
class Cursor;
}

namespace KateVi
{
class InputModeManager;

class CompletionReplayer
{
public:
    explicit CompletionReplayer(InputModeManager *viInputModeManager);
    ~CompletionReplayer();

    void start(const CompletionList &completions);
    void stop();

    void replay();

private:
    Completion nextCompletion();
    int findNextMergeableBracketPos(const KTextEditor::Cursor &startPos) const;

private:
    InputModeManager *m_viInputModeManager;

    QStack<CompletionList> m_CompletionsToReplay;
    QStack<int> m_nextCompletionIndex;
};

}

#endif // KATEVI_COMPLETIONREPLAYER_H
