/* This file is part of the KDE libraries
 *  Copyright (C) 2003-2005 Hamish Rodda <rodda@kde.org>
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

#include "attribute.h"
#include "kateextendedattribute.h"

using namespace KTextEditor;

class KTextEditor::AttributePrivate
{
public:
    AttributePrivate()
    {
        dynamicAttributes.append(Attribute::Ptr());
        dynamicAttributes.append(Attribute::Ptr());
    }

    QList<Attribute::Ptr> dynamicAttributes;
};

Attribute::Attribute()
    : d(new AttributePrivate())
{
}

Attribute::Attribute(const QString &name, DefaultStyle style)
    : d(new AttributePrivate())
{
    setName(name);
    setDefaultStyle(style);
}

Attribute::Attribute(const Attribute &a)
    : QTextCharFormat(a)
    , QSharedData()
    , d(new AttributePrivate())
{
    d->dynamicAttributes = a.d->dynamicAttributes;
}

Attribute::~Attribute()
{
    delete d;
}

Attribute &Attribute::operator+=(const Attribute &a)
{
    merge(a);

    for (int i = 0; i < a.d->dynamicAttributes.count(); ++i)
        if (i < d->dynamicAttributes.count()) {
            if (a.d->dynamicAttributes[i]) {
                d->dynamicAttributes[i] = a.d->dynamicAttributes[i];
            }
        } else {
            d->dynamicAttributes.append(a.d->dynamicAttributes[i]);
        }

    return *this;
}

Attribute::Ptr Attribute::dynamicAttribute(ActivationType type) const
{
    if (type < 0 || type >= d->dynamicAttributes.count()) {
        return Ptr();
    }

    return d->dynamicAttributes[type];
}

void Attribute::setDynamicAttribute(ActivationType type, Attribute::Ptr attribute)
{
    if (type < 0 || type > ActivateCaretIn) {
        return;
    }

    d->dynamicAttributes[type] = attribute;
}

QString Attribute::name() const
{
    return stringProperty(AttributeName);
}

void Attribute::setName(const QString &name)
{
    setProperty(AttributeName, name);
}

DefaultStyle Attribute::defaultStyle() const
{
    return static_cast<DefaultStyle> (intProperty(AttributeDefaultStyleIndex));
}

void Attribute::setDefaultStyle(DefaultStyle style)
{
    setProperty(AttributeDefaultStyleIndex, QVariant(static_cast<int>(style)));
}

bool Attribute::skipSpellChecking() const
{
    return boolProperty(Spellchecking);
}

void Attribute::setSkipSpellChecking(bool skipspellchecking)
{
    setProperty(Spellchecking, QVariant(skipspellchecking));
}

QBrush Attribute::outline() const
{
    if (hasProperty(Outline)) {
        return property(Outline).value<QBrush>();
    }

    return QBrush();
}

void Attribute::setOutline(const QBrush &brush)
{
    setProperty(Outline, brush);
}

QBrush Attribute::selectedForeground() const
{
    if (hasProperty(SelectedForeground)) {
        return property(SelectedForeground).value<QBrush>();
    }

    return QBrush();
}

void Attribute::setSelectedForeground(const QBrush &foreground)
{
    setProperty(SelectedForeground, foreground);
}

bool Attribute::backgroundFillWhitespace() const
{
    if (hasProperty(BackgroundFillWhitespace)) {
        return boolProperty(BackgroundFillWhitespace);
    }

    return true;
}

void Attribute::setBackgroundFillWhitespace(bool fillWhitespace)
{
    setProperty(BackgroundFillWhitespace, fillWhitespace);
}

QBrush Attribute::selectedBackground() const
{
    if (hasProperty(SelectedBackground)) {
        return property(SelectedBackground).value<QBrush>();
    }

    return QBrush();
}

void Attribute::setSelectedBackground(const QBrush &brush)
{
    setProperty(SelectedBackground, brush);
}

void Attribute::clear()
{
    QTextCharFormat::operator=(QTextCharFormat());

    d->dynamicAttributes.clear();
    d->dynamicAttributes.append(Ptr());
    d->dynamicAttributes.append(Ptr());
}

bool Attribute::fontBold() const
{
    return fontWeight() == QFont::Bold;
}

void Attribute::setFontBold(bool bold)
{
    setFontWeight(bold ? QFont::Bold : QFont::Normal);
}

bool Attribute::hasAnyProperty() const
{
    return !properties().isEmpty();
}

Attribute &KTextEditor::Attribute::operator =(const Attribute &a)
{
    QTextCharFormat::operator=(a);
    Q_ASSERT(static_cast<QTextCharFormat>(*this) == a);

    d->dynamicAttributes = a.d->dynamicAttributes;

    return *this;
}

