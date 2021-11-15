/*
    SPDX-FileCopyrightText: KDE Developers

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "searcher.h"
#include "globalstate.h"
#include "history.h"
#include "katedocument.h"
#include "kateview.h"
#include "ktexteditor/range.h"
#include <vimode/inputmodemanager.h>
#include <vimode/modes/modebase.h>

using namespace KateVi;

Searcher::Searcher(InputModeManager *manager)
    : m_viInputModeManager(manager)
    , m_view(manager->view())
{
}

const QString Searcher::getLastSearchPattern() const
{
    return m_lastSearchConfig.pattern;
}

void Searcher::setLastSearchParams(const SearchParams &searchParams)
{
    m_lastSearchConfig = searchParams;
}

bool Searcher::lastSearchWrapped() const
{
    return m_lastSearchWrapped;
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
    Range match = findPatternForMotion(m_lastSearchConfig, m_view->cursorPosition(), count);

    if (!match.valid) {
        return match;
    }
    if (!m_lastSearchConfig.shouldPlaceCursorAtEndOfMatch) {
        return Range(match.startLine, match.startColumn, ExclusiveMotion);
    }
    return Range(match.endLine, match.endColumn - 1, ExclusiveMotion);
}

Range Searcher::motionFindPrev(int count)
{
    SearchParams lastSearchReversed = m_lastSearchConfig;
    lastSearchReversed.isBackwards = !lastSearchReversed.isBackwards;
    Range match = findPatternForMotion(lastSearchReversed, m_view->cursorPosition(), count);

    if (!match.valid) {
        return match;
    }
    if (!m_lastSearchConfig.shouldPlaceCursorAtEndOfMatch) {
        return Range(match.startLine, match.startColumn, ExclusiveMotion);
    }
    return Range(match.endLine, match.endColumn - 1, ExclusiveMotion);
}

Range Searcher::findPatternForMotion(const SearchParams &searchParams, const KTextEditor::Cursor startFrom, int count)
{
    if (searchParams.pattern.isEmpty()) {
        return Range::invalid();
    }

    KTextEditor::Range match = findPatternWorker(searchParams, startFrom, count);
    return Range(match.start(), match.end(), ExclusiveMotion);
}

Range Searcher::findWordForMotion(const QString &word, bool backwards, const KTextEditor::Cursor startFrom, int count)
{
    m_lastSearchConfig.isBackwards = backwards;
    m_lastSearchConfig.isCaseSensitive = false;
    m_lastSearchConfig.shouldPlaceCursorAtEndOfMatch = false;

    m_viInputModeManager->globalState()->searchHistory()->append(QStringLiteral("\\<%1\\>").arg(word));
    QString pattern = QStringLiteral("\\b%1\\b").arg(word);
    m_lastSearchConfig.pattern = pattern;

    return findPatternForMotion(m_lastSearchConfig, startFrom, count);
}

KTextEditor::Range Searcher::findPattern(const SearchParams &searchParams, const KTextEditor::Cursor startFrom, int count, bool addToSearchHistory)
{
    if (addToSearchHistory) {
        m_viInputModeManager->globalState()->searchHistory()->append(searchParams.pattern);
        m_lastSearchConfig = searchParams;
    }

    return findPatternWorker(searchParams, startFrom, count);
}

KTextEditor::Range Searcher::findPatternWorker(const SearchParams &searchParams, const KTextEditor::Cursor startFrom, int count)
{
    KTextEditor::Cursor searchBegin = startFrom;
    KTextEditor::SearchOptions flags = KTextEditor::Regex;
    m_lastSearchWrapped = false;

    const QString &pattern = searchParams.pattern;

    if (searchParams.isBackwards) {
        flags |= KTextEditor::Backwards;
    }
    if (!searchParams.isCaseSensitive) {
        flags |= KTextEditor::CaseInsensitive;
    }
    KTextEditor::Range finalMatch;
    for (int i = 0; i < count; i++) {
        if (!searchParams.isBackwards) {
            const KTextEditor::Range matchRange =
                m_view->doc()
                    ->searchText(KTextEditor::Range(KTextEditor::Cursor(searchBegin.line(), searchBegin.column() + 1), m_view->doc()->documentEnd()),
                                 pattern,
                                 flags)
                    .first();

            if (matchRange.isValid()) {
                finalMatch = matchRange;
            } else {
                // Wrap around.
                const KTextEditor::Range wrappedMatchRange =
                    m_view->doc()->searchText(KTextEditor::Range(m_view->doc()->documentRange().start(), m_view->doc()->documentEnd()), pattern, flags).first();
                if (wrappedMatchRange.isValid()) {
                    finalMatch = wrappedMatchRange;
                    m_lastSearchWrapped = true;
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
                QVector<KTextEditor::Range> matchesUnfiltered =
                    m_view->doc()->searchText(KTextEditor::Range(newSearchBegin, m_view->doc()->documentRange().start()), pattern, flags);

                if (matchesUnfiltered.size() == 1 && !matchesUnfiltered.first().isValid()) {
                    break;
                }

                // After sorting, the last element in matchesUnfiltered is the last match position.
                std::sort(matchesUnfiltered.begin(), matchesUnfiltered.end());

                QVector<KTextEditor::Range> filteredMatches;
                for (KTextEditor::Range unfilteredMatch : std::as_const(matchesUnfiltered)) {
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
                const KTextEditor::Range wrappedMatchRange =
                    m_view->doc()->searchText(KTextEditor::Range(m_view->doc()->documentEnd(), m_view->doc()->documentRange().start()), pattern, flags).first();

                if (wrappedMatchRange.isValid()) {
                    finalMatch = wrappedMatchRange;
                    m_lastSearchWrapped = true;
                } else {
                    return KTextEditor::Range::invalid();
                }
            }
        }
        searchBegin = finalMatch.start();
    }
    return finalMatch;
}
