/* This file is part of the KDE project
   Copyright (C) 2017 Allan Sandfeld Jensen <kde@carewolf.com>

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

#ifndef KTEXTEDITOR_SCRIPTRANGE_H
#define KTEXTEDITOR_SCRIPTRANGE_H

#include <QJSEngine>
#include <QJSValue>

#include "ktexteditor/range.h"

inline QJSValue rangeToScriptValue(QJSEngine *engine, const KTextEditor::Range &range)
{
    QString code = QStringLiteral("new Range(%1, %2, %3, %4);").arg(range.start().line())
                   .arg(range.start().column())
                   .arg(range.end().line())
                   .arg(range.end().column());
    QJSValue result = engine->evaluate(code);
    Q_ASSERT(!result.isError());
    return result;
}

inline KTextEditor::Range rangeFromScriptValue(const QJSValue &obj)
{
    KTextEditor::Range range;
    range.setRange(KTextEditor::Cursor(obj.property(QStringLiteral("start")).property(QStringLiteral("line")).toInt(),
                                       obj.property(QStringLiteral("start")).property(QStringLiteral("column")).toInt()),
                   KTextEditor::Cursor(obj.property(QStringLiteral("end")).property(QStringLiteral("line")).toInt(),
                                       obj.property(QStringLiteral("end")).property(QStringLiteral("column")).toInt()));
    return range;
}

#endif
