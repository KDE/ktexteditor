/* This file is part of the KDE libraries

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License version 2 as published by the Free Software Foundation.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02111-13020, USA.
*/

#include "wordcounter.h"
#include "katedocument.h"
#include "kateview.h"

namespace {
    const int MaximumLinesToRecalculate = 100;
}

WordCounter::WordCounter(KTextEditor::ViewPrivate *view)
    : QObject(view)
    , m_wordsInDocument(0)
    , m_wordsInSelection(0)
    , m_charsInDocument(0)
    , m_charsInSelection(0)
    , m_startRecalculationFrom(0)
    , m_document(view->document())
{
    connect(view->doc(), &KTextEditor::DocumentPrivate::textInserted, this, &WordCounter::textInserted);
    connect(view->doc(), &KTextEditor::DocumentPrivate::textRemoved, this, &WordCounter::textRemoved);
    connect(view->doc(), &KTextEditor::DocumentPrivate::loaded, this, &WordCounter::recalculate);
    connect(view, &KTextEditor::View::selectionChanged, this, &WordCounter::selectionChanged);

    m_timer.setInterval(500);
    m_timer.setSingleShot(true);
    connect(&m_timer, &QTimer::timeout, this, &WordCounter::recalculateLines);

    recalculate(m_document);
}

void WordCounter::textInserted(KTextEditor::Document *, const KTextEditor::Range &range)
{
    const int startLine = range.start().line();
    const int endLine = range.end().line();
    int newLines = endLine - startLine;

    if (m_countByLine.count() == 0) { // was empty document before insert
        newLines++;
    }

    if (newLines > 0) {
        m_countByLine.insert(startLine, newLines, -1);
    }

    m_countByLine[endLine] = -1;
    m_timer.start();
}

void WordCounter::textRemoved(KTextEditor::Document *, const KTextEditor::Range &range, const QString &)
{
    const int startLine = range.start().line();
    const int endLine = range.end().line();
    const int removedLines = endLine - startLine;

    if (removedLines > 0) {
        m_countByLine.remove(startLine, removedLines);
    }

    if (!m_countByLine.isEmpty()) {
        m_countByLine[startLine] = -1;
        m_timer.start();
    } else {
        emit changed(0, 0, 0, 0);
    }
}

void WordCounter::recalculate(KTextEditor::Document *)
{
    m_countByLine = QVector<int>(m_document->lines(), -1);
    m_timer.start();
}

void WordCounter::selectionChanged(KTextEditor::View *view)
{
    if (view->selectionRange().isEmpty()) {
        m_wordsInSelection = m_charsInSelection = 0;
        emit changed(m_wordsInDocument, 0, m_charsInDocument, 0);
        return;
    }

    const int firstLine = view->selectionRange().start().line();
    const int lastLine = view->selectionRange().end().line();

    if (firstLine == lastLine || view->blockSelection()) {
        const QString text = view->selectionText();
        m_wordsInSelection = countWords(text);
        m_charsInSelection = text.size();
    } else {
        m_wordsInSelection = m_charsInSelection = 0;

        const KTextEditor::Range firstLineRange(view->selectionRange().start(), firstLine, view->document()->lineLength(firstLine));
        const QString firstLineText = view->document()->text(firstLineRange);
        m_wordsInSelection += countWords(firstLineText);
        m_charsInSelection += firstLineText.size();

        // whole lines
        for (int i = firstLine + 1; i < lastLine; i++) {
            m_wordsInSelection += m_countByLine[i];
            m_charsInSelection += m_document->lineLength(i);
        }

        const KTextEditor::Range lastLineRange(KTextEditor::Cursor(lastLine, 0), view->selectionRange().end());
        const QString lastLineText = view->document()->text(lastLineRange);
        m_wordsInSelection += countWords(lastLineText);
        m_charsInSelection += lastLineText.size();
    }

    emit changed(m_wordsInDocument, m_wordsInSelection, m_charsInDocument, m_charsInSelection);
}

void WordCounter::recalculateLines()
{
    if (m_startRecalculationFrom >= m_countByLine.size()) {
        m_startRecalculationFrom = 0;
    }

    int wordsCount = 0, charsCount = 0;
    int calculated = 0;
    int i = m_startRecalculationFrom;

    // stay in bounds, vector might be empty, even 0 is too large then
    while (i < m_countByLine.size()) {
        if (m_countByLine[i] == -1) {
            m_countByLine[i] = countWords(m_document->line(i));
            if (++calculated > MaximumLinesToRecalculate) {
                m_startRecalculationFrom = i;
                m_timer.start();
                return;
            }
        }

        wordsCount += m_countByLine[i];
        charsCount += m_document->lineLength(i);

        if (++i == m_countByLine.size()) { // array cycle
            i = 0;
        }

        if (i == m_startRecalculationFrom) {
            break;
        }
    }

    m_wordsInDocument = wordsCount;
    m_charsInDocument = charsCount;
    emit changed(m_wordsInDocument, m_wordsInSelection, m_charsInDocument, m_charsInSelection);
}

int WordCounter::countWords(const QString &text) const
{
    int count = 0;
    bool inWord = false;

    for (const QChar &c : text) {
        if (c.isLetterOrNumber()) {
            if (!inWord) {
                inWord = true;
            }
        } else {
            if (inWord) {
                inWord = false;
                count++;
            }
        }
    }

    return (inWord) ? count + 1: count;
}
