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

#ifndef KATE_ANNOTATIONITEMDELEGATE_H
#define KATE_ANNOTATIONITEMDELEGATE_H

#include <ktexteditor/abstractannotationitemdelegate.h>

namespace KTextEditor {
class ViewPrivate;
}
class KateViewInternal;

class KateAnnotationItemDelegate : public KTextEditor::AbstractAnnotationItemDelegate
{
    Q_OBJECT

public:
    explicit KateAnnotationItemDelegate(KateViewInternal *internalView, QObject *parent);
    ~KateAnnotationItemDelegate() override;

public:
    void paint(QPainter *painter, const KTextEditor::StyleOptionAnnotationItem &option,
               KTextEditor::AnnotationModel *model, int line) const override;
    bool helpEvent(QHelpEvent *event, KTextEditor::View *view,
                   const KTextEditor::StyleOptionAnnotationItem &option,
                   KTextEditor::AnnotationModel *model, int line) override;
    void hideTooltip(KTextEditor::View *view) override;
    QSize sizeHint(const KTextEditor::StyleOptionAnnotationItem &option,
                   KTextEditor::AnnotationModel *model, int line) const override;

private:
    KateViewInternal *m_internalView;
    KTextEditor::ViewPrivate *m_view;

    mutable qreal m_maxCharWidth = 0.0;
    mutable QFontMetricsF m_cachedDataContentFontMetrics;
};

#endif
