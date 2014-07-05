/*  This file is part of the KDE libraries
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

#ifndef KATEVI_MACROS_H
#define KATEVI_MACROS_H

#include "kateviinputmodemanager.h"
#include "ktexteditor_export.h"

class KConfigGroup;

namespace KateVi
{

class KTEXTEDITOR_EXPORT Macros
{
public:
    explicit Macros();
    ~Macros();

    void writeConfig(KConfigGroup &config) const;
    void readConfig(const KConfigGroup &config);

    void store(const QChar &reg, const QList<QKeyEvent> &macroKeyEventLog, const QList<KateViInputModeManager::Completion> &completions);
    void remove(const QChar &reg);
    void clear();

    QString get(const QChar &reg) const;
    QList<KateViInputModeManager::Completion> getCompletions(const QChar &reg) const;

private:
    int readMacroCompletions(const QChar &reg, const QStringList &encodedMacroCompletions, int macroCompletionIndex);
    QString encodeMacroCompletionForConfig(const KateViInputModeManager::Completion &completionForMacro) const;
    KateViInputModeManager::Completion decodeMacroCompletionFromConfig(const QString &encodedMacroCompletion);

private:
    QHash<QChar, QString> m_macros;
    QHash<QChar, QList<KateViInputModeManager::Completion>> m_completions;
};

}

#endif // KATEVI_MACROS_H
