/*
    SPDX-FileCopyrightText: 2017 Allan Sandfeld Jensen <kde@carewolf.com>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef KTEXTEDITOR_SCRIPTCURSOR_H
#define KTEXTEDITOR_SCRIPTCURSOR_H

#include <QJSEngine>
#include <QJSValue>

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
