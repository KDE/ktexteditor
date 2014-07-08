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

#ifndef KATEVI_SEARCHER_H
#define KATEVI_SEARCHER_H

#include "katevirange.h"

#include <QString>

class KateViInputModeManager;

namespace KTextEditor {
    class Cursor;
    class ViewPrivate;
    class Range;
}

namespace KateVi
{

class Searcher
{
public:
    explicit Searcher(KateViInputModeManager *viInputModeManager);
    ~Searcher();

    /** Command part **/
    void findNext();
    void findPrevious();

    /** Simple searchers **/
    KateViRange motionFindNext(int count = 1);
    KateViRange motionFindPrev(int count = 1);
    KateViRange findWordForMotion(const QString &pattern, bool backwards, const KTextEditor::Cursor &startFrom, int count);

    /** Extended searcher for ECB **/
    KTextEditor::Range findPattern(const QString &pattern, bool backwards, bool caseSensitive, bool placedCursorAtEndOfmatch, const KTextEditor::Cursor &startFrom, int count);

    const QString getLastSearchPattern() const;

private:
    KateViRange findPatternForMotion(const QString &pattern, bool backwards, bool caseSensitive, const KTextEditor::Cursor &startFrom, int count = 1) const;
    KTextEditor::Range findPatternWorker(const QString &pattern, bool backwards, bool caseSensitive, const KTextEditor::Cursor &startFrom, int count) const;

private:
    KateViInputModeManager *m_viInputModeManager;
    KTextEditor::ViewPrivate *m_view;

    QString m_lastSearchPattern;
    bool m_lastSearchBackwards;
    bool m_lastSearchCaseSensitive;
    bool m_lastSearchPlacedCursorAtEndOfMatch;
};
}

#endif // KATEVI_SEARCHER_H
