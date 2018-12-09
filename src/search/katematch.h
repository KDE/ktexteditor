/*  This file is part of the KDE libraries and the Kate part.
 *
 *  Copyright (C) 2010 Bernhard Beschow <bbeschow@cs.tu-berlin.de>
 *  Copyright (C) 2007 Sebastian Pipping <webmaster@hartwork.org>
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

#ifndef KATE_MATCH_H
#define KATE_MATCH_H

#include <memory>

#include <ktexteditor/document.h>
#include <ktexteditor/movingrange.h>

namespace KTextEditor { class DocumentPrivate; }

class KateMatch
{
public:
    KateMatch(KTextEditor::DocumentPrivate *document, KTextEditor::SearchOptions options);
    KTextEditor::Range searchText(const KTextEditor::Range &range, const QString &pattern);
    KTextEditor::Range replace(const QString &replacement, bool blockMode, int replacementCounter = 1);
    bool isValid() const;
    bool isEmpty() const;
    KTextEditor::Range range() const;

private:
    /**
     * Resolve references and escape sequences.
     */
    QString buildReplacement(const QString &replacement, bool blockMode, int replacementCounter) const;

private:
    KTextEditor::DocumentPrivate *const m_document;
    const KTextEditor::SearchOptions m_options;
    QVector<KTextEditor::Range> m_resultRanges;

    /**
     * moving range to track replace changes
     * kept for later reuse
     */
    std::unique_ptr<KTextEditor::MovingRange> m_afterReplaceRange;
};

#endif // KATE_MATCH_H

