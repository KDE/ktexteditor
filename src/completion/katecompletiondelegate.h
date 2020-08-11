/*
    SPDX-FileCopyrightText: 2007 David Nolden <david.nolden.kdevelop@art-master.de>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef KATECOMPLETIONDELEGATE_H
#define KATECOMPLETIONDELEGATE_H

#include "expandingtree/expandingdelegate.h"

class KateRenderer;
namespace KTextEditor
{
class DocumentPrivate;
}
class KateCompletionWidget;

class KateCompletionDelegate : public ExpandingDelegate
{
public:
    explicit KateCompletionDelegate(ExpandingWidgetModel *model, KateCompletionWidget *parent);

    KateRenderer *renderer() const;
    KateCompletionWidget *widget() const;
    KTextEditor::DocumentPrivate *document() const;

protected:
    void adjustStyle(const QModelIndex &index, QStyleOptionViewItem &option) const override;
    mutable int m_cachedRow;
    mutable QList<int> m_cachedColumnStarts;
    void heightChanged() const override;
    QVector<QTextLayout::FormatRange> createHighlighting(const QModelIndex &index, QStyleOptionViewItem &option) const override;
};

#endif
