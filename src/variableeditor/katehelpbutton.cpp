/*
    SPDX-FileCopyrightText: 2011-2018 Dominik Haumann <dhaumann@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "katehelpbutton.h"

#include <QIcon>

#include <KHelpClient>
#include <KLocalizedString>

KateHelpButton::KateHelpButton(QWidget *parent)
    : QToolButton(parent)
{
    setAutoRaise(true);
    setIconState(IconColored);
    setToolTip(i18n("Kate Handbook."));

    connect(this, &KateHelpButton::clicked, this, &KateHelpButton::invokeHelp);
}

void KateHelpButton::setIconState(IconState state)
{
    if (state == IconHidden) {
        setIcon(QIcon());
    } else {
        setIcon(QIcon::fromTheme(QStringLiteral("help-contents")));
    }

    update();
}

void KateHelpButton::invokeHelp()
{
    KHelpClient::invokeHelp(m_section, QStringLiteral("kate"));
}

void KateHelpButton::setSection(const QString &section)
{
    m_section = section;
}
