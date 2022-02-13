/*
    SPDX-FileCopyrightText: 2022 Waqar Ahmed <waqar.17a@gmail.com>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/
#include "documentation_tip.h"

#include <KWindowSystem>

#include <QDebug>
#include <QHBoxLayout>
#include <QScreen>

DocTip::DocTip(QWidget *parent)
    : QFrame(parent)
    , m_textView(new QTextBrowser(this))
{
    setFocusPolicy(Qt::NoFocus);
    setWindowFlags(Qt::ToolTip | Qt::BypassGraphicsProxyWidget);

    m_textView->setFrameStyle(QFrame::Box | QFrame::Raised);

    setFixedWidth(250);
    setFixedHeight(150);

    auto layout = new QHBoxLayout(this);
    layout->setContentsMargins({});
    layout->setSpacing(0);
    setContentsMargins({});
    layout->addWidget(&m_stack);
    m_stack.addWidget(m_textView);
}

QWidget *DocTip::currentWidget()
{
    return m_stack.currentWidget();
}

void DocTip::clearWidgets()
{
    for (auto *widget : m_widgets) {
        widget->deleteLater();
    }
    m_widgets.clear();
}

void DocTip::setText(const QString &s)
{
    m_textView->setPlainText(s);
    if (m_stack.currentWidget() != m_textView) {
        m_stack.removeWidget(m_stack.currentWidget());
        m_stack.addWidget(m_textView);
    }
    Q_ASSERT(m_stack.count() == 1);
}

void DocTip::setWidget(QWidget *widget)
{
    if (auto w = m_stack.currentWidget()) {
        if (w != m_textView) {
            m_widgets.push_back(w);
        }
        m_stack.removeWidget(w);
    }

    if (!widget) {
        return;
    }

    m_stack.addWidget(widget);

    Q_ASSERT(m_stack.count() == 1);
}

void DocTip::updatePosition()
{
    auto parent = parentWidget();
    if (!parent) {
        qWarning() << Q_FUNC_INFO << "Unexpected null parent!";
        return;
    }

    const auto screen = parent->screen();
    if (!screen) {
        // No screen, no tooltips
        return;
    }

    const auto screenRight = screen->availableGeometry().right();
    auto parentRight = parent->geometry().right();
    constexpr int Margin = 8;

    const bool wayland = KWindowSystem::isPlatformWayland();

    int x = 0;
    // is the completion widget too far right?
    if ((parentRight + this->width()) > screenRight) {
        // let's hope there is enough available space to the left of parent
        x = (parent->x() - this->width()) - Margin;
    } else {
        // we have enough space on the right
        x = parent->x() + parent->width() + Margin;
    }

    if (x != this->x()) {
        // On way land move() doesn't work, but if hide and then show again
        // window is positioned correctly. This is a hack, and if there is
        // a better solution, we can replace it
        if (wayland) {
            hide();
            move(x, parent->y());
            show();
        } else {
            move(x, parent->y());
        }
    }
}
