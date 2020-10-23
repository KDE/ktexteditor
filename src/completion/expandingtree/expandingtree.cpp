/*
    SPDX-FileCopyrightText: 2007 David Nolden <david.nolden.kdevelop@art-master.de>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "expandingtree.h"

#include "expandingwidgetmodel.h"
#include "katepartdebug.h"
#include <QAbstractTextDocumentLayout>
#include <QHeaderView>
#include <QPainter>
#include <QTextLayout>

ExpandingTree::ExpandingTree(QWidget *parent)
    : QTreeView(parent)
{
    m_drawText.documentLayout()->setPaintDevice(this);
    setUniformRowHeights(false);
    header()->setMinimumSectionSize(0);
}

void ExpandingTree::drawRow(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    QTreeView::drawRow(painter, option, index);

    const ExpandingWidgetModel *eModel = qobject_cast<const ExpandingWidgetModel *>(model());
    if (eModel && eModel->isPartiallyExpanded(index)) {
        QRect rect = eModel->partialExpandRect(index);
        if (rect.isValid()) {
            painter->fillRect(rect, QBrush(0xffffffff));

            QAbstractTextDocumentLayout::PaintContext ctx;
            // since arbitrary HTML can be shown use a black on white color scheme here
            ctx.palette = QPalette(Qt::black, Qt::white);
            ctx.clip = QRectF(0, 0, rect.width(), rect.height());
            ;
            painter->setViewTransformEnabled(true);
            painter->translate(rect.left(), rect.top());

            m_drawText.setHtml(eModel->partialExpandText(index));
            m_drawText.setPageSize(QSizeF(rect.width(), rect.height()));
            m_drawText.documentLayout()->draw(painter, ctx);

            painter->translate(-rect.left(), -rect.top());
        }
    }
}

int ExpandingTree::sizeHintForColumn(int column) const
{
    return columnWidth(column);
}
