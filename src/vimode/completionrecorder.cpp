/*
 *  This file is part of the KDE libraries
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
 *
 */

#include "completionrecorder.h"
#include <vimode/inputmodemanager.h>
#include "katepartdebug.h"
#include "macrorecorder.h"
#include "lastchangerecorder.h"

#include <QKeyEvent>

using namespace KateVi;

CompletionRecorder::CompletionRecorder(InputModeManager *viInputModeManager)
    : m_viInputModeManager(viInputModeManager)
{
}

CompletionRecorder::~CompletionRecorder()
{
}

void CompletionRecorder::logCompletionEvent(const Completion &completion)
{
    // Ctrl-space is a special code that means: if you're replaying a macro, fetch and execute
    // the next logged completion.
    static const QKeyEvent CompletionEvent(QKeyEvent::KeyPress, Qt::Key_Space, Qt::ControlModifier, QStringLiteral(" "));

    if (m_viInputModeManager->macroRecorder()->isRecording()) {
        m_viInputModeManager->macroRecorder()->record(CompletionEvent);
        m_currentMacroCompletionsLog.append(completion);
    }

    m_viInputModeManager->lastChangeRecorder()->record(CompletionEvent);
    m_currentChangeCompletionsLog.append(completion);
}

void CompletionRecorder::start()
{
    m_currentMacroCompletionsLog.clear();
}

CompletionList CompletionRecorder::stop()
{
    return m_currentMacroCompletionsLog;
}

void CompletionRecorder::clearCurrentChangeCompletionsLog()
{
    m_currentChangeCompletionsLog.clear();
}

CompletionList CompletionRecorder::currentChangeCompletionsLog()
{
    return m_currentChangeCompletionsLog;
}
