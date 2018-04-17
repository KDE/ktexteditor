/*  This file is part of the KDE libraries and the Kate part.
 *
 *  Copyright (C) 2013-2016 Simon St James <kdedevel@etotheipiplusone.com>
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

#ifndef KATEVI_EMULATED_COMMAND_BAR_INTERACTIVESEDREPLACEMODE_H
#define KATEVI_EMULATED_COMMAND_BAR_INTERACTIVESEDREPLACEMODE_H

#include "activemode.h"

#include "../cmds.h"

#include <QSharedPointer>

class QKeyEvent;
class QLabel;

namespace KateVi
{
class EmulatedCommandBar;
class MatchHighlighter;

class InteractiveSedReplaceMode : public ActiveMode
{
public:
    InteractiveSedReplaceMode(EmulatedCommandBar* emulatedCommandBar, MatchHighlighter* matchHighlighter, InputModeManager* viInputModeManager, KTextEditor::ViewPrivate* view);
    ~InteractiveSedReplaceMode() Q_DECL_OVERRIDE
    {
    }
    void activate(QSharedPointer<SedReplace::InteractiveSedReplacer> interactiveSedReplace);
    bool isActive() const
    {
        return m_isActive;
    }
    bool handleKeyPress(const QKeyEvent* keyEvent) Q_DECL_OVERRIDE;
    void deactivate(bool wasAborted) Q_DECL_OVERRIDE;
    QWidget *label();
private:
    void updateInteractiveSedReplaceLabelText();
    void finishInteractiveSedReplace();
    QSharedPointer<SedReplace::InteractiveSedReplacer> m_interactiveSedReplacer;
    bool m_isActive;
    QLabel *m_interactiveSedReplaceLabel;
};
}

#endif
