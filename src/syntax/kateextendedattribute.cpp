/* This file is part of the KDE libraries
   Copyright (C) 2003, 2004 Anders Lund <anders@alweb.dk>
   Copyright (C) 2003, 2005 Hamish Rodda <rodda@kde.org>
   Copyright (C) 2001,2002 Joseph Wenninger <jowenn@kde.org>
   Copyright (C) 2001 Christoph Cullmann <cullmann@kde.org>
   Copyright (C) 1999 Jochen Wilhelmy <digisnap@cs.tu-berlin.de>
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

#include "kateextendedattribute.h"

KateExtendedAttribute::KateExtendedAttribute(const QString &name, int defaultStyleIndex)
{
    setName(name);
    setDefaultStyleIndex(defaultStyleIndex);
    setPerformSpellchecking(true);
}

QString KateExtendedAttribute::name() const
{
    return stringProperty(AttributeName);
}

void KateExtendedAttribute::setName(const QString &name)
{
    setProperty(AttributeName, name);
}

bool KateExtendedAttribute::isDefaultStyle() const
{
    return hasProperty(AttributeDefaultStyleIndex);
}

int KateExtendedAttribute::defaultStyleIndex() const
{
    return intProperty(AttributeDefaultStyleIndex);
}

void KateExtendedAttribute::setDefaultStyleIndex(int index)
{
    setProperty(AttributeDefaultStyleIndex, QVariant(index));
}

bool KateExtendedAttribute::performSpellchecking() const
{
    return boolProperty(Spellchecking);
}

void KateExtendedAttribute::setPerformSpellchecking(bool spellchecking)
{
    setProperty(Spellchecking, QVariant(spellchecking));
}
