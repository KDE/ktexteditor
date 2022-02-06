/*
    SPDX-FileCopyrightText: 2007 David Nolden <david.nolden.kdevelop@art-master.de>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef KATECOMPLETIONDELEGATE_H
#define KATECOMPLETIONDELEGATE_H

#include "expandingtree/expandingdelegate.h"

class KateCompletionWidget;

class KateCompletionDelegate : public ExpandingDelegate
{
public:
    explicit KateCompletionDelegate(ExpandingWidgetModel *model, KateCompletionWidget *parent);

protected:
    void adjustStyle(const QModelIndex &index, QStyleOptionViewItem &option) const override;
    QVector<QTextLayout::FormatRange> createHighlighting(const QModelIndex &index, QStyleOptionViewItem &option) const override;

    bool editorEvent(QEvent *, QAbstractItemModel *, const QStyleOptionViewItem &, const QModelIndex &) override
    {
        return false;
    }

    QSize sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const override
    {
        return QItemDelegate::sizeHint(option, index);
    }
};

#endif
