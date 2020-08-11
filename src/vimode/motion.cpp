/*
    SPDX-FileCopyrightText: 2008 Erlend Hamberg <ehamberg@gmail.com>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include <vimode/motion.h>

using namespace KateVi;

Motion::Motion(NormalViMode *parent, const QString &pattern, Range (NormalViMode::*commandMethod)(), unsigned int flags)
    : Command(parent, pattern, nullptr, flags)
{
    m_ptr2commandMethod = commandMethod;
}

Range Motion::execute() const
{
    return (m_parent->*m_ptr2commandMethod)();
}
