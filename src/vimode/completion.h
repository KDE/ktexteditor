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

#ifndef KATEVI_COMPLETION_H
#define KATEVI_COMPLETION_H

#include <QList>
#include <QString>

namespace KateVi
{
class Completion
{
public:
    enum CompletionType { PlainText, FunctionWithoutArgs, FunctionWithArgs };

    explicit Completion(const QString &completedText, bool removeTail, CompletionType completionType);
    QString completedText() const;
    bool removeTail() const;
    CompletionType completionType() const;

private:
    QString m_completedText;
    bool m_removeTail;
    CompletionType m_completionType;
};

typedef QList<Completion> CompletionList;
}

#endif // KATEVI_COMPLETION_H
