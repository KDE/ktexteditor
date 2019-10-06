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

#ifndef KATEVI_MACRORECORDER_H
#define KATEVI_MACRORECORDER_H

#include <QChar>
#include <QKeyEvent>
#include <QList>

namespace KateVi
{
class InputModeManager;

class MacroRecorder
{
public:
    explicit MacroRecorder(InputModeManager *viInputModeManager);
    ~MacroRecorder();

    void start(const QChar &macroRegister);
    void stop();

    bool isRecording() const;

    void record(const QKeyEvent &event);
    void dropLast();

    void replay(const QChar &macroRegister);
    bool isReplaying();

private:
    InputModeManager *m_viInputModeManager;

    bool m_isRecording;
    QChar m_register;
    QList<QKeyEvent> m_eventsLog;

    int m_macrosBeingReplayedCount;
    QChar m_lastPlayedMacroRegister;
};
}

#endif // KATEVI_MACRORECORDER_H
