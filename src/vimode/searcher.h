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

#ifndef KATEVI_SEARCHER_H
#define KATEVI_SEARCHER_H

#include <vimode/range.h>

#include <QString>

namespace KTextEditor
{
class Cursor;
class ViewPrivate;
class Range;
}

namespace KateVi
{
class InputModeManager;

class Searcher
{
public:
    explicit Searcher(InputModeManager *viInputModeManager);
    ~Searcher();

    /** Command part **/
    void findNext();
    void findPrevious();

    /** Simple searchers **/
    Range motionFindNext(int count = 1);
    Range motionFindPrev(int count = 1);
    Range findWordForMotion(const QString &pattern, bool backwards, const KTextEditor::Cursor &startFrom, int count);

    /** Extended searcher for Emulated Command Bar. **/
    struct SearchParams {
        QString pattern;
        bool isBackwards = false;
        bool isCaseSensitive = false;
        bool shouldPlaceCursorAtEndOfMatch = false;
    };
    KTextEditor::Range findPattern(const SearchParams &searchParams, const KTextEditor::Cursor &startFrom, int count, bool addToSearchHistory = true);

    const QString getLastSearchPattern() const;
    void setLastSearchParams(const SearchParams &searchParams);

private:
    Range findPatternForMotion(const SearchParams &searchParams, const KTextEditor::Cursor &startFrom, int count = 1) const;
    KTextEditor::Range findPatternWorker(const SearchParams &searchParams, const KTextEditor::Cursor &startFrom, int count) const;

private:
    InputModeManager *m_viInputModeManager;
    KTextEditor::ViewPrivate *m_view;

    SearchParams m_lastSearchConfig;
};
}

#endif // KATEVI_SEARCHER_H
