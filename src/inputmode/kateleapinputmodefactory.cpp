/*
    SPDX-FileCopyrightText: KDE Developers

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "kateleapinputmodefactory.h"
#include "kateleapinputmode.h"

#include <KConfig>
#include <KLocalizedString>

KateLeapInputModeFactory::KateLeapInputModeFactory()
    : KateAbstractInputModeFactory()
{
}

KateAbstractInputMode *KateLeapInputModeFactory::createInputMode(KateViewInternal *viewInternal)
{
    return new KateLeapInputMode(viewInternal);
}

KateConfigPage *KateLeapInputModeFactory::createConfigPage(QWidget *parent)
{
    return nullptr; // return new KateVi::ConfigTab(parent, m_viGlobal->mappings());
}

KTextEditor::View::InputMode KateLeapInputModeFactory::inputMode()
{
    return KTextEditor::View::LeapInputMode;
}

QString KateLeapInputModeFactory::name()
{
    return i18n("Leap Input Mode");
}
