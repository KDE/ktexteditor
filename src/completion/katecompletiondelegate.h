/*
    SPDX-FileCopyrightText: 2007 David Nolden <david.nolden.kdevelop@art-master.de>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef KATECOMPLETIONDELEGATE_H
#define KATECOMPLETIONDELEGATE_H

// #include "expandingtree/expandingdelegate.h"
#include <QStyledItemDelegate>
#include <QTextLayout>

class KateCompletionWidget;
class KateCompletionModel;

class KateCompletionDelegate : public QStyledItemDelegate
{
public:
    explicit KateCompletionDelegate(QObject *parent);

    void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const override;

protected:
    //     void adjustStyle(const QModelIndex &index, QStyleOptionViewItem &option) const;
    QVector<QTextLayout::FormatRange> createHighlighting(const QModelIndex &index) const;

    QSize sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const override;

private:
    mutable int m_currentColumnStart = 0; // Text-offset for custom highlighting, will be applied to m_cachedHighlights(Only highlights starting after this will
                                          // be used). Should be zero of the highlighting is not taken from kate.
    QAbstractItemModel *const m_model = nullptr;
};

#endif
