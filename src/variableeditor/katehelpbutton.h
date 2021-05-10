/*
    SPDX-FileCopyrightText: 2011-2018 Dominik Haumann <dhaumann@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef KATE_HELP_BUTTON_H
#define KATE_HELP_BUTTON_H

#include <QToolButton>

class KateHelpButton : public QToolButton
{
    Q_OBJECT

public:
    enum IconState { IconColored = 0, IconHidden };

    void setSection(const QString &section);

public:
    explicit KateHelpButton(QWidget *parent = nullptr);

public Q_SLOTS:
    void setIconState(IconState state);
    void invokeHelp();

private:
    QString m_section;
};

#endif
