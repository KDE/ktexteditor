/*
    SPDX-FileCopyrightText: 2017 Allan Sandfeld Jensen <kde@carewolf.com>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef KTEXTEDITOR_SCRIPTRANGE_H
#define KTEXTEDITOR_SCRIPTRANGE_H

#include <QJSEngine>
#include <QJSValue>

#include "ktexteditor/range.h"

inline QJSValue rangeToScriptValue(QJSEngine *engine, const KTextEditor::Range &range)
{
    QString code =
        QStringLiteral("new Range(%1, %2, %3, %4);").arg(range.start().line()).arg(range.start().column()).arg(range.end().line()).arg(range.end().column());
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
