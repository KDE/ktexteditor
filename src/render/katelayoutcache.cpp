/*
    SPDX-FileCopyrightText: 2005 Hamish Rodda <rodda@kde.org>
    SPDX-FileCopyrightText: 2008-2018 Dominik Haumann <dhaumann@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "katelayoutcache.h"

#include <QtAlgorithms>

#include "katebuffer.h"
#include "katedocument.h"
#include "katepartdebug.h"
#include "katerenderer.h"
#include "kateview.h"

namespace
{
bool enableLayoutCache = false;

bool lessThan(const KateLineLayoutMap::LineLayoutPair &lhs, const KateLineLayoutMap::LineLayoutPair &rhs)
{
    return lhs.first < rhs.first;
}

}

// BEGIN KateLineLayoutMap

void KateLineLayoutMap::clear()
{
    m_lineLayouts.clear();
}

bool KateLineLayoutMap::contains(int i) const
{
    return std::binary_search(m_lineLayouts.cbegin(), m_lineLayouts.cend(), LineLayoutPair(i, KateLineLayoutPtr()), lessThan);
}

void KateLineLayoutMap::insert(int realLine, const KateLineLayoutPtr &lineLayoutPtr)
{
    LineLayoutMap::iterator it = std::lower_bound(m_lineLayouts.begin(), m_lineLayouts.end(), LineLayoutPair(realLine, KateLineLayoutPtr()), lessThan);
    if (it != m_lineLayouts.end() && (*it) == LineLayoutPair(realLine, KateLineLayoutPtr())) {
        (*it).second = lineLayoutPtr;
    } else {
        it = std::upper_bound(m_lineLayouts.begin(), m_lineLayouts.end(), LineLayoutPair(realLine, KateLineLayoutPtr()), lessThan);
        m_lineLayouts.insert(it, LineLayoutPair(realLine, lineLayoutPtr));
    }
}

void KateLineLayoutMap::viewWidthIncreased()
{
    LineLayoutMap::iterator it = m_lineLayouts.begin();
    for (; it != m_lineLayouts.end(); ++it) {
        if ((*it).second->isValid() && (*it).second->viewLineCount() > 1) {
            (*it).second->invalidateLayout();
        }
    }
}

void KateLineLayoutMap::viewWidthDecreased(int newWidth)
{
    LineLayoutMap::iterator it = m_lineLayouts.begin();
    for (; it != m_lineLayouts.end(); ++it) {
        if ((*it).second->isValid() && ((*it).second->viewLineCount() > 1 || (*it).second->width() > newWidth)) {
            (*it).second->invalidateLayout();
        }
    }
}

void KateLineLayoutMap::relayoutLines(int startRealLine, int endRealLine)
{
    LineLayoutMap::iterator start = std::lower_bound(m_lineLayouts.begin(), m_lineLayouts.end(), LineLayoutPair(startRealLine, KateLineLayoutPtr()), lessThan);
    LineLayoutMap::iterator end = std::upper_bound(start, m_lineLayouts.end(), LineLayoutPair(endRealLine, KateLineLayoutPtr()), lessThan);

    while (start != end) {
        (*start).second->setLayoutDirty();
        ++start;
    }
}

void KateLineLayoutMap::slotEditDone(int fromLine, int toLine, int shiftAmount)
{
    LineLayoutMap::iterator start = std::lower_bound(m_lineLayouts.begin(), m_lineLayouts.end(), LineLayoutPair(fromLine, KateLineLayoutPtr()), lessThan);
    LineLayoutMap::iterator end = std::upper_bound(start, m_lineLayouts.end(), LineLayoutPair(toLine, KateLineLayoutPtr()), lessThan);
    LineLayoutMap::iterator it;

    if (shiftAmount != 0) {
        for (it = end; it != m_lineLayouts.end(); ++it) {
            (*it).first += shiftAmount;
            (*it).second->setLine((*it).second->line() + shiftAmount);
        }

        for (it = start; it != end; ++it) {
            (*it).second->clear();
        }

        m_lineLayouts.erase(start, end);
    } else {
        for (it = start; it != end; ++it) {
            (*it).second->setLayoutDirty();
        }
    }
}

KateLineLayoutPtr &KateLineLayoutMap::operator[](int i)
{
    const LineLayoutMap::iterator it = std::lower_bound(m_lineLayouts.begin(), m_lineLayouts.end(), LineLayoutPair(i, KateLineLayoutPtr()), lessThan);
    Q_ASSERT(it != m_lineLayouts.end());
    return (*it).second;
}
// END KateLineLayoutMap

KateLayoutCache::KateLayoutCache(KateRenderer *renderer, QObject *parent)
    : QObject(parent)
    , m_renderer(renderer)
    , m_startPos(-1, -1)
    , m_viewWidth(0)
    , m_wrap(false)
    , m_acceptDirtyLayouts(false)
{
    Q_ASSERT(m_renderer);

    // connect to all possible editing primitives
    connect(&m_renderer->doc()->buffer(), &KateBuffer::lineWrapped, this, &KateLayoutCache::wrapLine);
    connect(&m_renderer->doc()->buffer(), &KateBuffer::lineUnwrapped, this, &KateLayoutCache::unwrapLine);
    connect(&m_renderer->doc()->buffer(), &KateBuffer::textInserted, this, &KateLayoutCache::insertText);
    connect(&m_renderer->doc()->buffer(), &KateBuffer::textRemoved, this, &KateLayoutCache::removeText);
}

void KateLayoutCache::updateViewCache(const KTextEditor::Cursor startPos, int newViewLineCount, int viewLinesScrolled)
{
    // qCDebug(LOG_KTE) << startPos << " nvlc " << newViewLineCount << " vls " << viewLinesScrolled;

    int oldViewLineCount = m_textLayouts.size();
    if (newViewLineCount == -1) {
        newViewLineCount = oldViewLineCount;
    }

    enableLayoutCache = true;

    int realLine;
    if (newViewLineCount == -1) {
        realLine = m_renderer->folding().visibleLineToLine(m_renderer->folding().lineToVisibleLine(startPos.line()));
    } else {
        realLine = m_renderer->folding().visibleLineToLine(startPos.line());
    }

    // compute the correct view line
    int _viewLine = 0;
    if (wrap()) {
        if (KateLineLayoutPtr l = line(realLine)) {
            Q_ASSERT(l->isValid());
            Q_ASSERT(l->length() >= startPos.column() || m_renderer->view()->wrapCursor());
            bool found = false;
            for (; _viewLine < l->viewLineCount(); ++_viewLine) {
                const KateTextLayout &t = l->viewLine(_viewLine);
                if (t.startCol() >= startPos.column() || _viewLine == l->viewLineCount() - 1) {
                    found = true;
                    break;
                }
            }
            Q_ASSERT(found);
        }
    }

    m_startPos = startPos;

    // Move the text layouts if we've just scrolled...
    if (viewLinesScrolled != 0) {
        // loop backwards if we've just scrolled up...
        bool forwards = viewLinesScrolled >= 0 ? true : false;
        for (int z = forwards ? 0 : m_textLayouts.size() - 1; forwards ? ((size_t)z < m_textLayouts.size()) : (z >= 0); forwards ? z++ : z--) {
            int oldZ = z + viewLinesScrolled;
            if (oldZ >= 0 && (size_t)oldZ < m_textLayouts.size()) {
                m_textLayouts[z] = m_textLayouts[oldZ];
            }
        }
    }

    // Resize functionality
    if (newViewLineCount > oldViewLineCount) {
        m_textLayouts.reserve(newViewLineCount);
    } else if (newViewLineCount < oldViewLineCount) {
        m_textLayouts.resize(newViewLineCount);
    }

    KateLineLayoutPtr l = line(realLine);
    for (int i = 0; i < newViewLineCount; ++i) {
        if (!l) {
            if ((size_t)i < m_textLayouts.size()) {
                if (m_textLayouts[i].isValid()) {
                    m_textLayouts[i] = KateTextLayout::invalid();
                }
            } else {
                m_textLayouts.push_back(KateTextLayout::invalid());
            }
            continue;
        }

        Q_ASSERT(l->isValid());
        Q_ASSERT(_viewLine < l->viewLineCount());

        if ((size_t)i < m_textLayouts.size()) {
            bool dirty = false;
            if (m_textLayouts[i].line() != realLine || m_textLayouts[i].viewLine() != _viewLine
                || (!m_textLayouts[i].isValid() && l->viewLine(_viewLine).isValid())) {
                dirty = true;
            }
            m_textLayouts[i] = l->viewLine(_viewLine);
            if (dirty) {
                m_textLayouts[i].setDirty(true);
            }

        } else {
            m_textLayouts.push_back(l->viewLine(_viewLine));
        }

        // qCDebug(LOG_KTE) << "Laid out line " << realLine << " (" << l << "), viewLine " << _viewLine << " (" << m_textLayouts[i].kateLineLayout().data() <<
        // ")"; m_textLayouts[i].debugOutput();

        _viewLine++;

        if (_viewLine > l->viewLineCount() - 1) {
            int virtualLine = l->virtualLine() + 1;
            realLine = m_renderer->folding().visibleLineToLine(virtualLine);
            _viewLine = 0;
            if (realLine < m_renderer->doc()->lines()) {
                l = line(realLine, virtualLine);
            } else {
                l = nullptr;
            }
        }
    }

    enableLayoutCache = false;
}

KateLineLayoutPtr KateLayoutCache::line(int realLine, int virtualLine)
{
    if (m_lineLayouts.contains(realLine)) {
        KateLineLayoutPtr l = m_lineLayouts[realLine];

        // ensure line is OK
        Q_ASSERT(l->line() == realLine);
        Q_ASSERT(realLine < m_renderer->doc()->buffer().lines());

        if (virtualLine != -1) {
            l->setVirtualLine(virtualLine);
        }

        if (!l->isValid()) {
            l->setUsePlainTextLine(acceptDirtyLayouts());
            l->textLine(!acceptDirtyLayouts());
            m_renderer->layoutLine(l, wrap() ? m_viewWidth : -1, enableLayoutCache);
        } else if (l->isLayoutDirty() && !acceptDirtyLayouts()) {
            // reset textline
            l->setUsePlainTextLine(false);
            l->textLine(true);
            m_renderer->layoutLine(l, wrap() ? m_viewWidth : -1, enableLayoutCache);
        }

        Q_ASSERT(l->isValid() && (!l->isLayoutDirty() || acceptDirtyLayouts()));

        return l;
    }

    if (realLine < 0 || realLine >= m_renderer->doc()->lines()) {
        return KateLineLayoutPtr();
    }

    KateLineLayoutPtr l(new KateLineLayout(*m_renderer));
    l->setLine(realLine, virtualLine);

    // Mark it dirty, because it may not have the syntax highlighting applied
    // mark this here, to allow layoutLine to use plainLines...
    if (acceptDirtyLayouts()) {
        l->setUsePlainTextLine(true);
    }

    m_renderer->layoutLine(l, wrap() ? m_viewWidth : -1, enableLayoutCache);
    Q_ASSERT(l->isValid());

    if (acceptDirtyLayouts()) {
        l->setLayoutDirty(true);
    }

    m_lineLayouts.insert(realLine, l);
    return l;
}

KateLineLayoutPtr KateLayoutCache::line(const KTextEditor::Cursor realCursor)
{
    return line(realCursor.line());
}

KateTextLayout KateLayoutCache::textLayout(const KTextEditor::Cursor realCursor)
{
    return line(realCursor.line())->viewLine(viewLine(realCursor));
}

KateTextLayout KateLayoutCache::textLayout(uint realLine, int _viewLine)
{
    return line(realLine)->viewLine(_viewLine);
}

KateTextLayout &KateLayoutCache::viewLine(int _viewLine)
{
    Q_ASSERT(_viewLine >= 0 && (size_t)_viewLine < m_textLayouts.size());
    return m_textLayouts[_viewLine];
}

int KateLayoutCache::viewCacheLineCount() const
{
    return m_textLayouts.size();
}

KTextEditor::Cursor KateLayoutCache::viewCacheStart() const
{
    return !m_textLayouts.empty() ? m_textLayouts.front().start() : KTextEditor::Cursor();
}

KTextEditor::Cursor KateLayoutCache::viewCacheEnd() const
{
    return !m_textLayouts.empty() ? m_textLayouts.back().end() : KTextEditor::Cursor();
}

int KateLayoutCache::viewWidth() const
{
    return m_viewWidth;
}

/**
 * This returns the view line upon which realCursor is situated.
 * The view line is the number of lines in the view from the first line
 * The supplied cursor should be in real lines.
 */
int KateLayoutCache::viewLine(const KTextEditor::Cursor realCursor)
{
    /**
     * Make sure cursor column and line is valid
     */
    if (realCursor.column() < 0 || realCursor.line() < 0 || realCursor.line() > m_renderer->doc()->lines()) {
        return 0;
    }

    KateLineLayoutPtr thisLine = line(realCursor.line());

    for (int i = 0; i < thisLine->viewLineCount(); ++i) {
        const KateTextLayout &l = thisLine->viewLine(i);
        if (realCursor.column() >= l.startCol() && realCursor.column() < l.endCol()) {
            return i;
        }
    }

    return thisLine->viewLineCount() - 1;
}

int KateLayoutCache::displayViewLine(const KTextEditor::Cursor virtualCursor, bool limitToVisible)
{
    if (!virtualCursor.isValid()) {
        return -1;
    }

    KTextEditor::Cursor work = viewCacheStart();

    // only try this with valid lines!
    if (work.isValid()) {
        work.setLine(m_renderer->folding().lineToVisibleLine(work.line()));
    }

    if (!work.isValid()) {
        return virtualCursor.line();
    }

    int limit = m_textLayouts.size();

    // Efficient non-word-wrapped path
    if (!m_renderer->view()->dynWordWrap()) {
        int ret = virtualCursor.line() - work.line();
        if (limitToVisible && (ret < 0)) {
            return -1;
        } else if (limitToVisible && (ret > limit)) {
            return -2;
        } else {
            return ret;
        }
    }

    if (work == virtualCursor) {
        return 0;
    }

    int ret = -(int)viewLine(viewCacheStart());
    bool forwards = (work < virtualCursor);

    // FIXME switch to using ranges? faster?
    if (forwards) {
        while (work.line() != virtualCursor.line()) {
            ret += viewLineCount(m_renderer->folding().visibleLineToLine(work.line()));
            work.setLine(work.line() + 1);
            if (limitToVisible && ret > limit) {
                return -2;
            }
        }
    } else {
        while (work.line() != virtualCursor.line()) {
            work.setLine(work.line() - 1);
            ret -= viewLineCount(m_renderer->folding().visibleLineToLine(work.line()));
            if (limitToVisible && ret < 0) {
                return -1;
            }
        }
    }

    // final difference
    KTextEditor::Cursor realCursor = virtualCursor;
    realCursor.setLine(m_renderer->folding().visibleLineToLine(realCursor.line()));
    if (realCursor.column() == -1) {
        realCursor.setColumn(m_renderer->doc()->lineLength(realCursor.line()));
    }
    ret += viewLine(realCursor);

    if (limitToVisible && (ret < 0 || ret > limit)) {
        return -1;
    }

    return ret;
}

int KateLayoutCache::lastViewLine(int realLine)
{
    if (!m_renderer->view()->dynWordWrap()) {
        return 0;
    }

    KateLineLayoutPtr l = line(realLine);
    Q_ASSERT(l);
    return l->viewLineCount() - 1;
}

int KateLayoutCache::viewLineCount(int realLine)
{
    return lastViewLine(realLine) + 1;
}

void KateLayoutCache::viewCacheDebugOutput() const
{
    qCDebug(LOG_KTE) << "Printing values for " << m_textLayouts.size() << " lines:";
    for (const KateTextLayout &t : std::as_const(m_textLayouts)) {
        if (t.isValid()) {
            t.debugOutput();
        } else {
            qCDebug(LOG_KTE) << "Line Invalid.";
        }
    }
}

void KateLayoutCache::wrapLine(const KTextEditor::Cursor position)
{
    m_lineLayouts.slotEditDone(position.line(), position.line() + 1, 1);
}

void KateLayoutCache::unwrapLine(int line)
{
    m_lineLayouts.slotEditDone(line - 1, line, -1);
}

void KateLayoutCache::insertText(const KTextEditor::Cursor position, const QString &)
{
    m_lineLayouts.slotEditDone(position.line(), position.line(), 0);
}

void KateLayoutCache::removeText(KTextEditor::Range range)
{
    m_lineLayouts.slotEditDone(range.start().line(), range.start().line(), 0);
}

void KateLayoutCache::clear()
{
    m_textLayouts.clear();
    m_lineLayouts.clear();
    m_startPos = KTextEditor::Cursor(-1, -1);
}

void KateLayoutCache::setViewWidth(int width)
{
    bool wider = width > m_viewWidth;

    m_viewWidth = width;

    m_lineLayouts.clear();
    m_startPos = KTextEditor::Cursor(-1, -1);

    // Only get rid of layouts that we have to
    if (wider) {
        m_lineLayouts.viewWidthIncreased();
    } else {
        m_lineLayouts.viewWidthDecreased(width);
    }
}

bool KateLayoutCache::wrap() const
{
    return m_wrap;
}

void KateLayoutCache::setWrap(bool wrap)
{
    m_wrap = wrap;
    clear();
}

void KateLayoutCache::relayoutLines(int startRealLine, int endRealLine)
{
    if (startRealLine > endRealLine) {
        qCWarning(LOG_KTE) << "start" << startRealLine << "before end" << endRealLine;
    }

    m_lineLayouts.relayoutLines(startRealLine, endRealLine);
}

bool KateLayoutCache::acceptDirtyLayouts() const
{
    return m_acceptDirtyLayouts;
}

void KateLayoutCache::setAcceptDirtyLayouts(bool accept)
{
    m_acceptDirtyLayouts = accept;
}
