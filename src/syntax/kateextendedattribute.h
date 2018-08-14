/* This file is part of the KDE libraries
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

#ifndef KATEEXTENDEDATTRIBUTE_H
#define KATEEXTENDEDATTRIBUTE_H

#include <ktexteditor/attribute.h>

/**
 * Custom property types, which may or may not be supported by implementations.
 * Internally used
 */
enum CustomProperties {
        /// Draws an outline around the text
        Outline = QTextFormat::UserProperty,
        /// Changes the brush used to paint the text when it is selected
        SelectedForeground,
        /// Changes the brush used to paint the background when it is selected
        SelectedBackground,
        /// Determines whether background color is drawn over whitespace. Defaults to true.
        BackgroundFillWhitespace,
        /// Defined to allow storage of dynamic effect information
        AttributeDynamicEffect = 0x10A00,
        /// Defined for internal usage of KTextEditor implementations
        AttributeInternalProperty = 0x10E00,
        AttributeName = AttributeInternalProperty,
        AttributeDefaultStyleIndex,
        Spellchecking,
        /// Defined to allow 3rd party code to create their own custom attributes - you may use values at or above this property.
        AttributeUserProperty = 0x110000
};

typedef QVector<KTextEditor::Attribute::Ptr> KateAttributeList;

#endif
