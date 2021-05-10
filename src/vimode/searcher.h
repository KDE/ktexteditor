/*
    SPDX-FileCopyrightText: KDE Developers

    SPDX-License-Identifier: LGPL-2.0-or-later
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
    bool lastSearchWrapped() const;
    void setLastSearchParams(const SearchParams &searchParams);

private:
    Range findPatternForMotion(const SearchParams &searchParams, const KTextEditor::Cursor &startFrom, int count = 1);
    KTextEditor::Range findPatternWorker(const SearchParams &searchParams, const KTextEditor::Cursor &startFrom, int count);

private:
    InputModeManager *m_viInputModeManager;
    KTextEditor::ViewPrivate *m_view;

    SearchParams m_lastSearchConfig;
    bool m_lastSearchWrapped;
};
}

#endif // KATEVI_SEARCHER_H
