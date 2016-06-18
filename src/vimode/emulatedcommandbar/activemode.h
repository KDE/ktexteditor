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

#ifndef KATEVI_EMULATED_COMMAND_BAR_ACTIVEMODE_H
#define KATEVI_EMULATED_COMMAND_BAR_ACTIVEMODE_H

#include "completer.h"

class QKeyEvent;
class QString;
class QWidget;

namespace KTextEditor
{
    class Cursor;
    class Range;
    class ViewPrivate;
}

namespace KateVi
{
class EmulatedCommandBar;
struct CompletionStartParams;
class MatchHighlighter;
class InputModeManager;

class ActiveMode
{
public:
    ActiveMode(EmulatedCommandBar* emulatedCommandBar, MatchHighlighter* matchHighlighter, InputModeManager* viInputModeManager, KTextEditor::ViewPrivate* view)
    : m_emulatedCommandBar(emulatedCommandBar),
      m_viInputModeManager(viInputModeManager),
      m_view(view),
      m_matchHighligher(matchHighlighter)
    {
    }
    virtual ~ActiveMode() = 0;
    virtual bool handleKeyPress(const QKeyEvent *keyEvent) = 0;
    virtual void editTextChanged(const QString &newText)
    {
        Q_UNUSED(newText);
    }
    virtual KateVi::CompletionStartParams completionInvoked(Completer::CompletionInvocation invocationType);
    virtual void completionChosen()
    {
    }
    virtual void deactivate(bool wasAborted) = 0;
    void setViInputModeManager(InputModeManager *viInputModeManager);
protected:
    // Helper methods.
    void hideAllWidgetsExcept(QWidget* widgetToKeepVisible);
    void updateMatchHighlight(const KTextEditor::Range &matchRange);
    void close(bool wasAborted);
    void closeWithStatusMessage(const QString& exitStatusMessage);
    void startCompletion(const CompletionStartParams& completionStartParams);
    void moveCursorTo(const KTextEditor::Cursor &cursorPos);
    EmulatedCommandBar *emulatedCommandBar();
    KTextEditor::ViewPrivate* view();
    InputModeManager* viInputModeManager();
private:
    EmulatedCommandBar *m_emulatedCommandBar = nullptr;
    InputModeManager* m_viInputModeManager = nullptr;
    KTextEditor::ViewPrivate* m_view = nullptr;
    MatchHighlighter *m_matchHighligher = nullptr;
};
}
#endif
