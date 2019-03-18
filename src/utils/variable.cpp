/* This file is part of the KDE project
   Copyright (C) 2019 Dominik Haumann <dhaumann@kde.org>

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
#include "variable.h"

namespace KTextEditor
{

Variable::Variable(const QString& name, const QString& description, Variable::ExpandFunction func)
    : m_name(name)
    , m_description(description)
    , m_function(func)
{}

bool Variable::isValid() const
{
    return (!m_name.isEmpty()) && (m_function != nullptr);
}

QString Variable::name() const
{
    return m_name;
}

QString Variable::description() const
{
    return m_description;
}

QString Variable::evaluate(const QStringView& prefix, KTextEditor::View * view) const
{
    return isValid() ? m_function(prefix, view) : QString();
}

}
