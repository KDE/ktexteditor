/*  SPDX-License-Identifier: LGPL-2.0-or-later

    Copyright (C) KDE Developers

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

#ifndef KATE_ABSTRACT_INPUT_MODE_FACTORY_H
#define KATE_ABSTRACT_INPUT_MODE_FACTORY_H

class KateAbstractInputMode;
class KateViewInternal;

class KConfig;
class KateConfigPage;

#include "ktexteditor/view.h"
#include <QString>
class QWidget;

class KateAbstractInputModeFactory
{
public:
    KateAbstractInputModeFactory();

    virtual ~KateAbstractInputModeFactory();
    virtual KateAbstractInputMode *createInputMode(KateViewInternal *viewInternal) = 0;

    virtual QString name() = 0;
    virtual KTextEditor::View::InputMode inputMode() = 0;

    virtual KateConfigPage *createConfigPage(QWidget *) = 0;
};

#endif
