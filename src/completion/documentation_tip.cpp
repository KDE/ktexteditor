#include "documentation_tip.h"

#include <QDebug>
#include <QHBoxLayout>
#include <QScreen>

DocTip::DocTip(QWidget *parent)
    : QFrame(parent)
    , m_textEdit(new QTextEdit(this))
{
    setFocusPolicy(Qt::NoFocus);
    setWindowFlags(Qt::Tool | Qt::FramelessWindowHint);

    m_textEdit->setFrameStyle(QFrame::Box | QFrame::Raised);

    setFixedWidth(250);
    setFixedHeight(150);

    auto layout = new QHBoxLayout(this);
    layout->setContentsMargins({});
    layout->setSpacing(0);
    setContentsMargins({});
    layout->addWidget(&m_stack);
    m_stack.addWidget(m_textEdit);
}

void DocTip::setText(const QString &s)
{
    m_textEdit->setPlainText(s);
    if (m_stack.currentWidget() != m_textEdit) {
        m_stack.removeWidget(m_stack.currentWidget());
        m_stack.addWidget(m_textEdit);
        m_stack.setCurrentWidget(m_textEdit);
    }
}

void DocTip::setWidget(QWidget *w)
{
    m_stack.removeWidget(m_stack.currentWidget());
    m_stack.addWidget(w);
    m_stack.setCurrentWidget(w);
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
