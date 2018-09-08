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

#include "completion.h"
#include "katepartdebug.h"

using namespace KateVi;

Completion::Completion(const QString &completedText, bool removeTail, CompletionType completionType)
    : m_completedText(completedText),
      m_removeTail(removeTail),
      m_completionType(completionType)
{
    if (m_completionType == FunctionWithArgs || m_completionType == FunctionWithoutArgs) {
        qCDebug(LOG_KTE) << "Completing a function while not removing tail currently unsupported; will remove tail instead";
        m_removeTail = true;
    }
}

QString Completion::completedText() const
{
    return m_completedText;
}

bool Completion::removeTail() const
{
    return m_removeTail;
}

Completion::CompletionType Completion::completionType() const
{
    return m_completionType;
}
