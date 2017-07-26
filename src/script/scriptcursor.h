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

#ifndef KTEXTEDITOR_SCRIPTCURSOR_H
#define KTEXTEDITOR_SCRIPTCURSOR_H

#include <QtQml/QJSEngine>
#include <QtQml/QJSValue>

#include "ktexteditor/cursor.h"

inline QJSValue cursorToScriptValue(QJSEngine *engine, const KTextEditor::Cursor &cursor)
{
    QString code = QStringLiteral("new Cursor(%1, %2);").arg(cursor.line()).arg(cursor.column());
    QJSValue result = engine->evaluate(code);
    Q_ASSERT(!result.isError());
    return result;
}

inline KTextEditor::Cursor cursorFromScriptValue(const QJSValue &obj)
{
    KTextEditor::Cursor cursor;
    QJSValue line = obj.property(QStringLiteral("line"));
    QJSValue column = obj.property(QStringLiteral("column"));
    Q_ASSERT(!line.isError() && !column.isError());
    cursor.setPosition(line.toInt(), column.toInt());
    return cursor;
}

#endif
