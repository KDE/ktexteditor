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

#ifndef KATEVI_JUMPS_H
#define KATEVI_JUMPS_H

#include <ktexteditor/cursor.h>

#include <KConfigGroup>

#include <QVector>

namespace KateVi
{
class Jumps
{
public:
    explicit Jumps() = default;
    ~Jumps() = default;

    Jumps(const Jumps &) = delete;
    Jumps &operator=(const Jumps &) = delete;

    void add(const KTextEditor::Cursor &cursor);
    KTextEditor::Cursor next(const KTextEditor::Cursor &cursor);
    KTextEditor::Cursor prev(const KTextEditor::Cursor &cursor);

    void writeSessionConfig(KConfigGroup &config) const;
    void readSessionConfig(const KConfigGroup &config);

private:
    void printJumpList() const;

private:
    QVector<KTextEditor::Cursor> m_jumps;
    QVector<KTextEditor::Cursor>::iterator m_current = m_jumps.begin();
};

}

#endif // KATEVI_JUMPS_H
