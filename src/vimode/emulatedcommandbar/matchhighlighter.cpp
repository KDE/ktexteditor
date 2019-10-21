/*  SPDX-License-Identifier: LGPL-2.0-or-later

    Copyright (C) 2013-2016 Simon St James <kdedevel@etotheipiplusone.com>

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

#include "matchhighlighter.h"

#include "kateconfig.h"
#include "katedocument.h"
#include "kateview.h"

using namespace KateVi;

MatchHighlighter::MatchHighlighter(KTextEditor::ViewPrivate *view)
    : m_view(view)
{
    updateMatchHighlightAttrib();
    m_highlightedMatch = m_view->doc()->newMovingRange(KTextEditor::Range::invalid(), Kate::TextRange::DoNotExpand);
    m_highlightedMatch->setView(m_view); // Show only in this view.
    m_highlightedMatch->setAttributeOnlyForViews(true);
    // Use z depth defined in moving ranges interface.
    m_highlightedMatch->setZDepth(-10000.0);
    m_highlightedMatch->setAttribute(m_highlightMatchAttribute);
    connect(m_view, SIGNAL(configChanged()), this, SLOT(updateMatchHighlightAttrib()));
}

MatchHighlighter::~MatchHighlighter()
{
    delete m_highlightedMatch;
}

void MatchHighlighter::updateMatchHighlight(const KTextEditor::Range &matchRange)
{
    // Note that if matchRange is invalid, the highlight will not be shown, so we
    // don't need to check for that explicitly.
    m_highlightedMatch->setRange(matchRange);
}

void MatchHighlighter::updateMatchHighlightAttrib()
{
    const QColor &matchColour = m_view->renderer()->config()->searchHighlightColor();
    if (!m_highlightMatchAttribute) {
        m_highlightMatchAttribute = new KTextEditor::Attribute;
    }
    m_highlightMatchAttribute->setBackground(matchColour);
    KTextEditor::Attribute::Ptr mouseInAttribute(new KTextEditor::Attribute());
    m_highlightMatchAttribute->setDynamicAttribute(KTextEditor::Attribute::ActivateMouseIn, mouseInAttribute);
    m_highlightMatchAttribute->dynamicAttribute(KTextEditor::Attribute::ActivateMouseIn)->setBackground(matchColour);
}
