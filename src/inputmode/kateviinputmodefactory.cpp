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

#include "kateviinputmodefactory.h"
#include "kateviinputmode.h"
#include "vimode/globalstate.h"
#include <vimode/config/configtab.h>

#include <KConfig>
#include <KLocalizedString>

KateViInputModeFactory::KateViInputModeFactory()
    : KateAbstractInputModeFactory()
    , m_viGlobal(new KateVi::GlobalState())
{
}

KateViInputModeFactory::~KateViInputModeFactory()
{
    delete m_viGlobal;
}

KateAbstractInputMode *KateViInputModeFactory::createInputMode(KateViewInternal *viewInternal)
{
    return new KateViInputMode(viewInternal, m_viGlobal);
}

KateConfigPage *KateViInputModeFactory::createConfigPage(QWidget *parent)
{
    return new KateVi::ConfigTab(parent, m_viGlobal->mappings());
}

KTextEditor::View::InputMode KateViInputModeFactory::inputMode()
{
    return KTextEditor::View::ViInputMode;
}

QString KateViInputModeFactory::name()
{
    return i18n("Vi Input Mode");
}
