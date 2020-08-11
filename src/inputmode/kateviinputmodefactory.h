/*
    SPDX-FileCopyrightText: KDE Developers

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef KATE_VI_INPUT_MODE_FACTORY_H
#define KATE_VI_INPUT_MODE_FACTORY_H

#include "kateabstractinputmodefactory.h"

namespace KateVi
{
class GlobalState;
}
class KateViInputMode;

class KateViInputModeFactory : public KateAbstractInputModeFactory
{
    friend KateViInputMode;

public:
    KateViInputModeFactory();

    ~KateViInputModeFactory() override;
    KateAbstractInputMode *createInputMode(KateViewInternal *viewInternal) override;

    QString name() override;
    KTextEditor::View::InputMode inputMode() override;

    KateConfigPage *createConfigPage(QWidget *) override;

private:
    KateVi::GlobalState *m_viGlobal;
};

#endif
