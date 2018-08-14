// This file is part of the KDE libraries
// Copyright (C) 2008 Paul Giannaros <paul@giannaros.org>
// Copyright (C) 2008 Christoph Cullmann <cullmann@kde.org>
//
// This library is free software; you can redistribute it and/or
// modify it under the terms of the GNU Library General Public
// License as published by the Free Software Foundation; either
// version 2 of the License, or (at your option) version 3.
//
// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// Library General Public License for more details.
//
// You should have received a copy of the GNU Library General Public License
// along with this library; see the file COPYING.LIB.  If not, write to
// the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
// Boston, MA 02110-1301, USA.

#include "katescriptview.h"

#include "kateglobal.h"
#include "katedocument.h"
#include "kateview.h"
#include "katerenderer.h"
#include "katescript.h"
#include "scriptcursor.h"
#include "scriptrange.h"

#include <KTextEditor/Command>
#include <KLocalizedString>

#include <QJSEngine>

KateScriptView::KateScriptView(QJSEngine *engine, QObject *parent)
    : QObject(parent), m_view(nullptr), m_engine(engine)
{
}

void KateScriptView::setView(KTextEditor::ViewPrivate *view)
{
    m_view = view;
}

KTextEditor::ViewPrivate *KateScriptView::view()
{
    return m_view;
}

QJSValue KateScriptView::cursorPosition()
{
    return cursorToScriptValue(m_engine, m_view->cursorPosition());
}

void KateScriptView::setCursorPosition(int line, int column)
{
    const KTextEditor::Cursor cursor(line, column);
    m_view->setCursorPosition(cursor);
}

void KateScriptView::setCursorPosition(const QJSValue &jscursor)
{
    const auto cursor = cursorFromScriptValue(jscursor);
    m_view->setCursorPosition(cursor);
}

QJSValue KateScriptView::virtualCursorPosition()
{
    return cursorToScriptValue(m_engine, m_view->cursorPositionVirtual());
}

void KateScriptView::setVirtualCursorPosition(int line, int column)
{
    const KTextEditor::Cursor cursor(line, column);
    m_view->setCursorPositionVisual(cursor);
}

void KateScriptView::setVirtualCursorPosition(const QJSValue &jscursor)
{
    const auto cursor = cursorFromScriptValue(jscursor);
    setVirtualCursorPosition(cursor.line(), cursor.column());
}

QString KateScriptView::selectedText()
{
    return m_view->selectionText();
}

bool KateScriptView::hasSelection()
{
    return m_view->selection();
}

QJSValue KateScriptView::selection()
{
    return rangeToScriptValue(m_engine, m_view->selectionRange());
}

void KateScriptView::setSelection(const QJSValue &jsrange)
{
    m_view->setSelection(rangeFromScriptValue(jsrange));
}

void KateScriptView::removeSelectedText()
{
    m_view->removeSelectedText();
}

void KateScriptView::selectAll()
{
    m_view->selectAll();
}

void KateScriptView::clearSelection()
{
    m_view->clearSelection();
}

void KateScriptView::align(const QJSValue &jsrange)
{
    const auto range = rangeFromScriptValue(jsrange);
    m_view->doc()->align (m_view, range);
}

QJSValue KateScriptView::executeCommand(const QString &command,
                                        const QString &args,
                                        const QJSValue &jsrange)
{
    QString message;
    bool ok = false;

    const auto range = rangeFromScriptValue(jsrange);
    const auto cmd = KTextEditor::EditorPrivate::self()->queryCommand(command);
    if (!cmd) {
        ok = false;
        message = i18n("Command not found: %1", command);
    } else {
        const auto cmdLine = args.isEmpty() ? (command) : (command + QLatin1Char(' ') + args);
        ok = cmd->exec(m_view, cmdLine, message, range);
    }

    QJSValue object;
    object.setProperty(QStringLiteral("ok"), ok);
    object.setProperty(QStringLiteral("status"), message);
    return object;
}
