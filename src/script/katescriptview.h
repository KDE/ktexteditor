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

#ifndef KATE_SCRIPT_VIEW_H
#define KATE_SCRIPT_VIEW_H

#include <QObject>
#include <QJSValue>

#include <ktexteditor_export.h>

#include <ktexteditor/cursor.h>
#include <ktexteditor/range.h>

namespace KTextEditor { class ViewPrivate; }
class QJSEngine;
/**
 * Thinish wrapping around KTextEditor::ViewPrivate, exposing the methods we want exposed
 * and adding some helper methods.
 *
 * setView _must_ be called before using any other method. This is not checked
 * for the sake of speed.
 */
class KTEXTEDITOR_EXPORT KateScriptView : public QObject
{
    /// Properties are accessible with a nicer syntax from JavaScript
    Q_OBJECT

public:
    explicit KateScriptView(QJSEngine *, QObject *parent = nullptr);
    void setView(KTextEditor::ViewPrivate *view);
    KTextEditor::ViewPrivate *view();

    Q_INVOKABLE QJSValue cursorPosition();
    /**
     * Set the cursor position in the view.
     * @since 4.4
     */
    Q_INVOKABLE void setCursorPosition(int line, int column);
    Q_INVOKABLE void setCursorPosition(const QJSValue &cursor);

    Q_INVOKABLE QJSValue virtualCursorPosition();
    Q_INVOKABLE void setVirtualCursorPosition(int line, int column);
    Q_INVOKABLE void setVirtualCursorPosition(const QJSValue &cursor);

    Q_INVOKABLE QString selectedText();
    Q_INVOKABLE bool hasSelection();
    Q_INVOKABLE QJSValue selection();
    Q_INVOKABLE void setSelection(const QJSValue &range);
    Q_INVOKABLE void removeSelectedText();
    Q_INVOKABLE void selectAll();
    Q_INVOKABLE void clearSelection();

    Q_INVOKABLE void align(const QJSValue &range);

    Q_INVOKABLE QJSValue executeCommand(const QString &command,
                                        const QString &args = QString(),
                                        const QJSValue &jsrange = QJSValue());

private:
    KTextEditor::ViewPrivate *m_view;
    QJSEngine *m_engine;
};

#endif

