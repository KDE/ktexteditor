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

#include "katenormalinputmodefactory.h"
#include "katenormalinputmode.h"

#include <KLocalizedString>

KateNormalInputModeFactory::KateNormalInputModeFactory()
    : KateAbstractInputModeFactory()
{

}

KateNormalInputModeFactory::~KateNormalInputModeFactory()
{

}

KateAbstractInputMode *KateNormalInputModeFactory::createInputMode(KateViewInternal *viewInternal)
{
    return new KateNormalInputMode(viewInternal);
}

KateConfigPage *KateNormalInputModeFactory::createConfigPage(QWidget *)
{
    return nullptr;
}

KTextEditor::View::InputMode KateNormalInputModeFactory::inputMode()
{
    return KTextEditor::View::NormalInputMode;
}

QString KateNormalInputModeFactory::name()
{
    return i18n("Normal Mode");
}

