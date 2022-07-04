/*
    SPDX-FileCopyrightText: KDE Developers

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef KATE_LEAP_INPUT_MODE_FACTORY_H
#define KATE_LEAP_INPUT_MODE_FACTORY_H

#include <memory>

#include "kateabstractinputmodefactory.h"

class KateLeapInputMode;

class KateLeapInputModeFactory : public KateAbstractInputModeFactory
{
    friend KateLeapInputMode;

public:
    KateLeapInputModeFactory();

    KateAbstractInputMode *createInputMode(KateViewInternal *viewInternal) override;

    QString name() override;
    KTextEditor::View::InputMode inputMode() override;

    KateConfigPage *createConfigPage(QWidget *) override;
};

#endif
