#include "documentation_tip.h"

#include <QDebug>
#include <QScreen>

DocTip::DocTip(QWidget *parent)
    : QPlainTextEdit(parent)
{
    setFrameStyle(QFrame::NoFrame);
    setFocusPolicy(Qt::NoFocus);
    setWindowFlags(Qt::Tool | Qt::FramelessWindowHint);

    setFixedWidth(200);
    setFixedHeight(150);
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

    // is the completion widget too far right?
    if ((parentRight + this->width()) > screenRight) {
        // let's hope there is enough available space to the left of parent
        const int x = (parent->x() - this->width()) - Margin;
        move(x, parent->y());
    } else {
        // we have enough space on the right
        const int x = parent->x() + parent->width() + Margin;
        move(x, parent->y());
    }
}
