/*
    SPDX-FileCopyrightText: 2007 David Nolden <david.nolden.kdevelop@art-master.de>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "katecompletiondelegate.h"

#include <ktexteditor/codecompletionmodel.h>

#include "katedocument.h"
#include "katehighlight.h"
#include "katepartdebug.h"
#include "katetextline.h"
#include "kateview.h"

#include "katecompletionmodel.h"
#include "katecompletiontree.h"
#include "katecompletionwidget.h"

KateCompletionDelegate::KateCompletionDelegate(ExpandingWidgetModel *model, KateCompletionWidget *parent)
    : ExpandingDelegate(model, parent)
{
}

void KateCompletionDelegate::adjustStyle(const QModelIndex &index, QStyleOptionViewItem &option) const
{
    if (index.column() == 0) {
        // We always want to use the match-color if available, also for highlighted items.
        ///@todo Only do this for the "current" item, for others the model is asked for the match color.
        uint color = model()->matchColor(index);
        if (color != 0) {
            QColor match(color);

            for (int a = 0; a <= 2; a++) {
                option.palette.setColor((QPalette::ColorGroup)a, QPalette::Highlight, match);
            }
        }
    }
}

QVector<QTextLayout::FormatRange> KateCompletionDelegate::createHighlighting(const QModelIndex &index, QStyleOptionViewItem &) const
{
    QVariant highlight = model()->data(index, KTextEditor::CodeCompletionModel::HighlightingMethod);

    // TODO: config enable specifying no highlight as default
    int highlightMethod = KTextEditor::CodeCompletionModel::InternalHighlighting;
    if (highlight.canConvert(QVariant::Int)) {
        highlightMethod = highlight.toInt();
    }

    if (highlightMethod & KTextEditor::CodeCompletionModel::CustomHighlighting) {
        m_currentColumnStart = 0;
        return highlightingFromVariantList(model()->data(index, KTextEditor::CodeCompletionModel::CustomHighlight).toList());
    }

    return {};
}
