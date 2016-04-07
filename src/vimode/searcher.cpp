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

#include "searcher.h"
#include <vimode/inputmodemanager.h>
#include <vimode/modes/modebase.h>
#include "kateview.h"
#include "katedocument.h"
#include "ktexteditor/range.h"
#include "globalstate.h"
#include "history.h"

using namespace KateVi;

Searcher::Searcher(InputModeManager *manager)
    : m_viInputModeManager(manager)
    , m_view(manager->view())
    , m_lastSearchBackwards(false)
    , m_lastSearchCaseSensitive(false)
    , m_lastSearchPlacedCursorAtEndOfMatch(false)
{
}

Searcher::~Searcher()
{
}

const QString Searcher::getLastSearchPattern() const
{
    return m_lastSearchPattern;
}

void Searcher::findNext()
{
    const Range r = motionFindPrev();
    if (r.valid) {
        m_viInputModeManager->getCurrentViModeHandler()->goToPos(r);
    }
}

void Searcher::findPrevious()
{
    const Range r = motionFindPrev();
    if (r.valid) {
        m_viInputModeManager->getCurrentViModeHandler()->goToPos(r);
    }
}

Range Searcher::motionFindNext(int count)
{
    Range match = findPatternForMotion(
        m_lastSearchPattern,
        m_lastSearchBackwards,
        m_lastSearchCaseSensitive,
        m_view->cursorPosition(),
        count);

    if (!match.valid) {
        return match;
    }
    if (!m_lastSearchPlacedCursorAtEndOfMatch) {
        return Range(match.startLine, match.startColumn, ExclusiveMotion);
    }
    return Range(match.endLine, match.endColumn - 1, ExclusiveMotion);
}

Range Searcher::motionFindPrev(int count)
{
    Range match = findPatternForMotion(
        m_lastSearchPattern,
        !m_lastSearchBackwards,
        m_lastSearchCaseSensitive,
        m_view->cursorPosition(),
        count);

    if (!match.valid) {
        return match;
    }
    if (!m_lastSearchPlacedCursorAtEndOfMatch) {
        return Range(match.startLine, match.startColumn, ExclusiveMotion);
    }
    return Range(match.endLine, match.endColumn - 1, ExclusiveMotion);
}

Range Searcher::findPatternForMotion(const QString &pattern, bool backwards, bool caseSensitive, const KTextEditor::Cursor &startFrom, int count) const
{
    if (pattern.isEmpty()) {
        return Range::invalid();
    }

    KTextEditor::Range match = findPatternWorker(pattern, backwards, caseSensitive, startFrom, count);
    return Range(match.start(), match.end(), ExclusiveMotion);
}

Range Searcher::findWordForMotion(const QString &word, bool backwards, const KTextEditor::Cursor &startFrom, int count)
{
    m_lastSearchBackwards = backwards;
    m_lastSearchCaseSensitive = false;
    m_lastSearchPlacedCursorAtEndOfMatch = false;

    m_viInputModeManager->globalState()->searchHistory()->append(QStringLiteral("\\<%1\\>").arg(word));
    QString pattern = QStringLiteral("\\b%1\\b").arg(word);
    m_lastSearchPattern = pattern;

    return findPatternForMotion(pattern, backwards, false, startFrom, count);
}

KTextEditor::Range Searcher::findPattern(const QString &pattern, bool backwards, bool caseSensitive, bool placedCursorAtEndOfmatch, const KTextEditor::Cursor &startFrom, int count, bool addToSearchHistory)
{
    m_lastSearchPattern = pattern;
    m_lastSearchBackwards = backwards;
    m_lastSearchCaseSensitive = caseSensitive;
    m_lastSearchPlacedCursorAtEndOfMatch = placedCursorAtEndOfmatch;

    if (addToSearchHistory) {
        m_viInputModeManager->globalState()->searchHistory()->append(pattern);
    }

    return findPatternWorker(pattern, backwards, caseSensitive, startFrom, count);
}

KTextEditor::Range Searcher::findPatternWorker(const QString &pattern, bool backwards, bool caseSensitive, const KTextEditor::Cursor &startFrom, int count) const
{
    KTextEditor::Cursor searchBegin = startFrom;
    KTextEditor::SearchOptions flags = KTextEditor::Regex;

    if (backwards) {
        flags |= KTextEditor::Backwards;
    }
    if (!caseSensitive) {
        flags |= KTextEditor::CaseInsensitive;
    }
    KTextEditor::Range finalMatch;
    for (int i = 0; i < count; i++) {
        if (!backwards) {
            const KTextEditor::Range matchRange = m_view->doc()->searchText(KTextEditor::Range(KTextEditor::Cursor(searchBegin.line(), searchBegin.column() + 1), m_view->doc()->documentEnd()), pattern, flags).first();

            if (matchRange.isValid()) {
                finalMatch = matchRange;
            } else {
                // Wrap around.
                const KTextEditor::Range wrappedMatchRange = m_view->doc()->searchText(KTextEditor::Range(m_view->doc()->documentRange().start(), m_view->doc()->documentEnd()), pattern, flags).first();
                if (wrappedMatchRange.isValid()) {
                    finalMatch = wrappedMatchRange;
                } else {
                    return KTextEditor::Range::invalid();
                }
            }
        } else {
            // Ok - this is trickier: we can't search in the range from doc start to searchBegin, because
            // the match might extend *beyond* searchBegin.
            // We could search through the entire document and then filter out only those matches that are
            // after searchBegin, but it's more efficient to instead search from the start of the
            // document until the beginning of the line after searchBegin, and then filter.
            // Unfortunately, searchText doesn't necessarily turn up all matches (just the first one, sometimes)
            // so we must repeatedly search in such a way that the previous match isn't found, until we either
            // find no matches at all, or the first match that is before searchBegin.
            KTextEditor::Cursor newSearchBegin = KTextEditor::Cursor(searchBegin.line(), m_view->doc()->lineLength(searchBegin.line()));
            KTextEditor::Range bestMatch = KTextEditor::Range::invalid();
            while (true) {
                QVector<KTextEditor::Range> matchesUnfiltered = m_view->doc()->searchText(KTextEditor::Range(newSearchBegin, m_view->doc()->documentRange().start()), pattern, flags);

                if (matchesUnfiltered.size() == 1 && !matchesUnfiltered.first().isValid()) {
                    break;
                }

                // After sorting, the last element in matchesUnfiltered is the last match position.
                qSort(matchesUnfiltered);

                QVector<KTextEditor::Range> filteredMatches;
                foreach (KTextEditor::Range unfilteredMatch, matchesUnfiltered) {
                    if (unfilteredMatch.start() < searchBegin) {
                        filteredMatches.append(unfilteredMatch);
                    }
                }
                if (!filteredMatches.isEmpty()) {
                    // Want the latest matching range that is before searchBegin.
                    bestMatch = filteredMatches.last();
                    break;
                }

                // We found some unfiltered matches, but none were suitable. In case matchesUnfiltered wasn't
                // all matching elements, search again, starting from before the earliest matching range.
                if (filteredMatches.isEmpty()) {
                    newSearchBegin = matchesUnfiltered.first().start();
                }
            }

            KTextEditor::Range matchRange = bestMatch;

            if (matchRange.isValid()) {
                finalMatch = matchRange;
            } else {
                const KTextEditor::Range wrappedMatchRange = m_view->doc()->searchText(KTextEditor::Range(m_view->doc()->documentEnd(), m_view->doc()->documentRange().start()), pattern, flags).first();

                if (wrappedMatchRange.isValid()) {
                    finalMatch = wrappedMatchRange;
                } else {
                    return KTextEditor::Range::invalid();
                }
            }
        }
        searchBegin = finalMatch.start();
    }
    return finalMatch;
}
