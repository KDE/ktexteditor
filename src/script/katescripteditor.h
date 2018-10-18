/* This file is part of the KDE libraries.
 *
 * Copyright (C) 2017 Dominik Haumann <dhaumann@kde.org>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this library; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */
#ifndef KATE_SCRIPT_EDITOR_H
#define KATE_SCRIPT_EDITOR_H

#include <QObject>
#include <QJSValue>

#include <ktexteditor_export.h>

namespace KTextEditor { class ViewPrivate; }
class QJSEngine;

/**
 * This class wraps the global editor instance KateGlobal, exposing some
 * helper methods such as the clipboard history etc.
 */
class KTEXTEDITOR_EXPORT KateScriptEditor : public QObject
{
    Q_OBJECT

public:
    explicit KateScriptEditor(QJSEngine *engine, QObject *parent = nullptr);

    Q_INVOKABLE QString clipboardText() const;
    Q_INVOKABLE QStringList clipboardHistory() const;
    Q_INVOKABLE void setClipboardText(const QString &text);

private:
    QJSEngine *m_engine = nullptr;
};

#endif
