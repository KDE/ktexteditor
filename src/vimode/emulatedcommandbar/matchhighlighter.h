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

#ifndef KATEVI_EMULATED_COMMAND_BAR_MATCHHIGHLIGHTER_H
#define KATEVI_EMULATED_COMMAND_BAR_MATCHHIGHLIGHTER_H

#include <ktexteditor/attribute.h>

#include <QObject>

namespace KTextEditor
{
    class ViewPrivate;
    class Range;
    class MovingRange;
}

namespace KateVi
{
class MatchHighlighter : public QObject
{
    Q_OBJECT
public:
    MatchHighlighter(KTextEditor::ViewPrivate* view);
    ~MatchHighlighter();
    void updateMatchHighlight(const KTextEditor::Range &matchRange);
private Q_SLOTS:
    void updateMatchHighlightAttrib();
private:
    KTextEditor::ViewPrivate *m_view = nullptr;
    KTextEditor::Attribute::Ptr m_highlightMatchAttribute;
    KTextEditor::MovingRange *m_highlightedMatch;
};
}
#endif
