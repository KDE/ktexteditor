/* This file is part of the KDE libraries
   Copyright (C) 2017-18 Friedrich W. H. Kossebau <kossebau@kde.org>

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

#include "kateannotationitemdelegate.h"

#include "kateviewinternal.h"

#include <KLocalizedString>

#include <QPainter>
#include <QToolTip>
#include <QFontMetricsF>

#include <math.h>


KateAnnotationItemDelegate::KateAnnotationItemDelegate(KateViewInternal *internalView, QObject *parent)
    : KTextEditor::AbstractAnnotationItemDelegate(parent)
    , m_internalView(internalView)
    , m_view(internalView->m_view)
    , m_cachedDataContentFontMetrics(QFont())
{
}

KateAnnotationItemDelegate::~KateAnnotationItemDelegate() = default;

void KateAnnotationItemDelegate::paint(QPainter *painter, const KTextEditor::StyleOptionAnnotationItem &option,
                                       KTextEditor::AnnotationModel *model, int line) const
{
    Q_ASSERT(painter);
    Q_ASSERT(model);
    if (!painter || !model) {
        return;
    }
    // TODO: also test line for validity for sake of completeness?

    painter->save();

    const int margin = 3;

    const QVariant background = model->data(line, Qt::BackgroundRole);
    // Fill the background
    if (background.isValid()) {
        painter->fillRect(option.rect, background.value<QBrush>());
    }

    const QVariant foreground = model->data(line, Qt::ForegroundRole);
    // Set the pen for drawing the foreground
    if (foreground.isValid() && foreground.canConvert<QPen>()) {
        painter->setPen(foreground.value<QPen>());
    }

    // Draw a border around all adjacent entries that have the same text as the currently hovered one
    if ((option.state & QStyle::State_MouseOver) &&
        (option.annotationItemGroupingPosition & KTextEditor::StyleOptionAnnotationItem::InGroup)) {
        // Use floating point coordinates to support scaled rendering
        QRectF rect(option.rect);
        rect.adjust(0.5, 0.5, -0.5, -0.5);

        // draw left and right highlight borders
        painter->drawLine(rect.topLeft(), rect.bottomLeft());
        painter->drawLine(rect.topRight(), rect.bottomRight());

        if ((option.annotationItemGroupingPosition & KTextEditor::StyleOptionAnnotationItem::GroupBegin) &&
            (option.wrappedLine == 0)) {
            painter->drawLine(rect.topLeft(), rect.topRight());
        }

        if ((option.annotationItemGroupingPosition & KTextEditor::StyleOptionAnnotationItem::GroupEnd) &&
            (option.wrappedLine == (option.wrappedLineCount-1))) {
            painter->drawLine(rect.bottomLeft(), rect.bottomRight());
        }
    }
    // reset pen
    if (foreground.isValid()) {
        QPen pen = painter->pen();
        pen.setWidth(1);
        painter->setPen(pen);
    }

    // Now draw the normal text
    const QVariant text = model->data(line, Qt::DisplayRole);
    if ((option.wrappedLine == 0) && text.isValid() && text.canConvert<QString>()) {
        painter->drawText(option.rect.x() + margin, option.rect.y(),
                          option.rect.width() - 2*margin, option.rect.height(),
                          Qt::AlignLeft | Qt::AlignVCenter, text.toString());
    }

    painter->restore();
}

bool KateAnnotationItemDelegate::helpEvent(QHelpEvent *event, KTextEditor::View *view,
                                           const KTextEditor::StyleOptionAnnotationItem &option,
                                           KTextEditor::AnnotationModel *model, int line)
{
    Q_UNUSED(option);

    if (!model || event->type() != QEvent::ToolTip) {
        return false;
    }

    const QVariant data = model->data(line, Qt::ToolTipRole);
    if (!data.isValid()) {
        return false;
    }

    const QString toolTipText = data.toString();
    if (toolTipText.isEmpty()) {
        return false;
    }

    QToolTip::showText(event->globalPos(), toolTipText, view, option.rect);

    return true;
}

void KateAnnotationItemDelegate::hideTooltip(KTextEditor::View *view)
{
    Q_UNUSED(view);
    QToolTip::hideText();
}

QSize KateAnnotationItemDelegate::sizeHint(const KTextEditor::StyleOptionAnnotationItem &option,
                                           KTextEditor::AnnotationModel *model, int line) const
{
    Q_ASSERT(model);
    if (!model) {
        return QSize(0, 0);
    }

    // recalculate m_maxCharWidth if needed
    if (m_maxCharWidth == 0.0 || (option.contentFontMetrics != m_cachedDataContentFontMetrics)) {
        m_maxCharWidth = 0.0;
        // based on old code written when just a hash was shown, could see an update
        // Loop to determine the widest numeric character in the current font.
        for (char c = '0'; c <= '9'; ++c) {
            const qreal charWidth = ceil(option.contentFontMetrics.width(QLatin1Char(c)));
            m_maxCharWidth = qMax(m_maxCharWidth, charWidth);
        }

        m_cachedDataContentFontMetrics = option.contentFontMetrics;
    }

    const QString annotationText = model->data(line, Qt::DisplayRole).toString();
    return QSize(annotationText.length() * m_maxCharWidth + 8, option.contentFontMetrics.height());
}
