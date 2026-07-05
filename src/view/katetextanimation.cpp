/*
    SPDX-FileCopyrightText: 2013-2018 Dominik Haumann <dhaumann@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "katetextanimation.h"

#include "katerenderer.h"
#include "kateview.h"
#include <ktexteditor/document.h>

#include <QPainter>
#include <QPointF>
#include <QRect>
#include <QSizeF>
#include <QTimeLine>

KateTextAnimation::KateTextAnimation(KTextEditor::Range range, KTextEditor::Attribute::Ptr attribute, KTextEditor::ViewPrivate *view)
    : QObject(view)
    , m_range(range)
    , m_text(view->document()->text(range))
    , m_attribute(std::move(attribute))
    , m_view(view)
    , m_timeLine(new QTimeLine(250, this))
    , m_value(0.0)
{
    connect(m_timeLine, &QTimeLine::valueChanged, this, &KateTextAnimation::nextFrame);
    connect(m_timeLine, &QTimeLine::finished, this, &KateTextAnimation::deleteLater);

    m_timeLine->setEasingCurve(QEasingCurve::SineCurve);
    m_timeLine->start();

    QObject::connect(view, &KTextEditor::View::destroyed, m_timeLine, &QTimeLine::stop);
}

KateTextAnimation::~KateTextAnimation()
{
    // stop the animation, if still running
    if (m_timeLine->state() == QTimeLine::Running) {
        m_timeLine->stop();

        // remove all artifacts
        nextFrame(0.0);
    }
}

void KateTextAnimation::draw(QPainter &painter)
{
    // could happen in corner cases: timeLine emitted finished(), but this object
    // is not yet deleted. Therefore, draw() might be called in paintEvent().
    if (m_timeLine->state() == QTimeLine::NotRunning) {
        return;
    }

    // fill & paint text
    painter.fillRect(m_drawRect, m_attribute->background());
    painter.setPen(m_attribute->foreground().color());
    painter.setFont(m_drawFont);
    painter.drawText(m_drawRect, m_text);
}

void KateTextAnimation::nextFrame(qreal value)
{
    m_value = value;

    // update rect for draw

    // not on screen, avoid work
    const QPoint pixelPos = m_view->editorWidget()->mapFromParent(m_view->cursorToCoordinate(m_range.start()));
    if (pixelPos.x() != -1 && pixelPos.y() != -1) {
        // compute center of animation with rendere font metrics
        const QPointF center =
            QRectF(pixelPos.x(), pixelPos.y(), m_view->renderer()->currentFontMetrics().boundingRect(m_text).width(), m_view->renderer()->lineHeight())
                .center();

        // scale font with animation
        m_drawFont = m_view->renderer()->currentFont();
        m_drawFont.setBold(m_attribute->fontBold());
        m_drawFont.setPointSizeF(m_drawFont.pointSizeF() * (1.0 + 0.5 * m_value));

        // compute new rect size depending on new font metrics
        m_drawRect = QFontMetricsF(m_drawFont).boundingRect(m_text);

        // move new rect to old center
        m_drawRect.moveCenter(center);

        // ensure we will cleanup the right rect
        m_unitedRectForViewUpdate = m_unitedRectForViewUpdate.united(m_drawRect);
    }

    // trigger repaint, ensure we align to full pixels for the request
    m_view->editorWidget()->update(m_unitedRectForViewUpdate.toAlignedRect());
}
