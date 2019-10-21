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

#ifndef KATEVI_LASTCHANGERECORDER_H
#define KATEVI_LASTCHANGERECORDER_H

#include "completion.h"

#include <QKeyEvent>
#include <QList>
#include <QString>

namespace KateVi
{
class InputModeManager;

/**
 * In e.g. Insert mode, Qt seems to feed each keypress through twice; once as a ShortcutOverride (even if the key
 * doesn't actually appear to be a ShortcutOverride) and then, whether the "ShortcutOverride" was accepted or not,
 * again as a KeyPress.  We don't want to store both, so this helper helps to decide what to do.
 */
bool isRepeatOfLastShortcutOverrideAsKeyPress(const QKeyEvent &currentKeyPress, const QList<QKeyEvent> &keyEventLog);

class LastChangeRecorder
{
public:
    explicit LastChangeRecorder(InputModeManager *viInputModeManager);
    ~LastChangeRecorder();

    void record(const QKeyEvent &event);
    void dropLast();
    void clear();

    QString encodedChanges() const;

    void replay(const QString &commands, const CompletionList &completions);
    bool isReplaying() const;

private:
    InputModeManager *m_viInputModeManager;

    QList<QKeyEvent> m_changeLog;

    bool m_isReplaying;
};
}

#endif // KATEVI_LASTCHANGERECORDER_H
