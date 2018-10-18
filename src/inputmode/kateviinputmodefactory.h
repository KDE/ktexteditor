/*  This file is part of the KDE libraries and the Kate part.
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Library General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Library General Public License for more details.
 *
 *  You should have received a copy of the GNU Library General Public License
 *  along with this library; see the file COPYING.LIB.  If not, write to
 *  the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 *  Boston, MA 02110-1301, USA.
 */

#ifndef KATE_VI_INPUT_MODE_FACTORY_H
#define KATE_VI_INPUT_MODE_FACTORY_H

#include "kateabstractinputmodefactory.h"

namespace KateVi { class GlobalState; }
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
