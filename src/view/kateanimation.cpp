/*  SPDX-License-Identifier: LGPL-2.0-or-later

    Copyright (C) 2013 Dominik Haumann <dhaumann@kde.org>

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

#include "kateanimation.h"

#include "katefadeeffect.h"
#include "kateglobal.h"

#include <KMessageWidget>

#include <QStyle>
#include <QTimer>

KateAnimation::KateAnimation(KMessageWidget *widget, EffectType effect)
    : QObject(widget)
    , m_widget(widget)
    , m_fadeEffect(nullptr)
{
    Q_ASSERT(m_widget != nullptr);

    // create wanted effect
    if (effect == FadeEffect) {
        m_fadeEffect = new KateFadeEffect(widget);

        connect(m_fadeEffect, SIGNAL(hideAnimationFinished()), this, SIGNAL(widgetHidden()));
        connect(m_fadeEffect, SIGNAL(showAnimationFinished()), this, SIGNAL(widgetShown()));
    } else {
        connect(m_widget, SIGNAL(hideAnimationFinished()), this, SIGNAL(widgetHidden()));
        connect(m_widget, SIGNAL(showAnimationFinished()), this, SIGNAL(widgetShown()));
    }
}

bool KateAnimation::isHideAnimationRunning() const
{
    return m_fadeEffect ? m_fadeEffect->isHideAnimationRunning() : m_widget->isHideAnimationRunning();
}

bool KateAnimation::isShowAnimationRunning() const
{
    return m_fadeEffect ? m_fadeEffect->isShowAnimationRunning() : m_widget->isShowAnimationRunning();
}

void KateAnimation::show()
{
    Q_ASSERT(m_widget != nullptr);

    // show according to effects config
    if (m_widget->style()->styleHint(QStyle::SH_Widget_Animate, nullptr, m_widget)) {
        // launch show effect
        // NOTE: use a singleShot timer to avoid resizing issues when showing the message widget the first time (bug #316666)
        if (m_fadeEffect) {
            QTimer::singleShot(0, m_fadeEffect, SLOT(fadeIn()));
        } else {
            QTimer::singleShot(0, m_widget, SLOT(animatedShow()));
        }
    } else {
        m_widget->show();
        emit widgetShown();
    }
}

void KateAnimation::hide()
{
    Q_ASSERT(m_widget != nullptr);

    // hide according to effects config
    if (m_widget->style()->styleHint(QStyle::SH_Widget_Animate, nullptr, m_widget) || KTextEditor::EditorPrivate::unitTestMode() // due to timing issues in the unit test
    ) {
        // hide depending on effect
        if (m_fadeEffect) {
            m_fadeEffect->fadeOut();
        } else {
            m_widget->animatedHide();
        }
    } else {
        m_widget->hide();
        emit widgetHidden();
    }
}
