/*  This file is part of the KDE libraries and the Kate part.
 *
 *  Copyright (C) 2008 Erlend Hamberg <ehamberg@gmail.com>
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


#ifndef KATE_VI_REPLACE_MODE_INCLUDED
#define KATE_VI_REPLACE_MODE_INCLUDED


#include <vimode/katevimodebase.h>


/**
 * Commands for the vi replace mode
 */
class KateViReplaceMode : public KateViModeBase
{
public:
    explicit KateViReplaceMode(KateViInputModeManager *viInputModeManager,
                               KTextEditor::ViewPrivate *view,
                               KateViewInternal *viewInternal);
    ~KateViReplaceMode();

    /// Update the track of overwritten characters with the \p s character.
    inline void overwrittenChar(const QChar &s)
    {
        m_overwritten += s;
    }

protected:
    /**
     * Checks if the key is a valid command in replace mode.
     *
     * @returns true if a command was completed and executed, false otherwise.
     */
    bool handleKeypress(const QKeyEvent *e) override;

private:
    /**
     * Replace the character at the current column with a character from
     * the same column but in a different line.
     *
     * @param offset The offset of the line to be picked. This value is
     * relative to the current line.
     * @returns true if the character could be replaced, false otherwise.
     */
    bool commandInsertFromLine(int offset);

    // Auxiliar methods for moving the cursor in replace mode.

    bool commandMoveOneWordLeft();
    bool commandMoveOneWordRight();

    /// It removes a modification in the previous character.
    void backspace();

private:
    /// Keeps track of the characters that have been overwritten so far.
    QString m_overwritten;
};


#endif /* KATE_VI_REPLACE_MODE_INCLUDED */
