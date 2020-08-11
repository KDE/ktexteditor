/*
    SPDX-FileCopyrightText: 2017 Dominik Haumann <dhaumann@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "katescripteditor.h"

#include "kateglobal.h"

#include <QApplication>
#include <QClipboard>
#include <QJSEngine>

using KTextEditor::EditorPrivate;

KateScriptEditor::KateScriptEditor(QJSEngine *engine, QObject *parent)
    : QObject(parent)
    , m_engine(engine)
{
}

QString KateScriptEditor::clipboardText() const
{
    return QApplication::clipboard()->text();
}

QStringList KateScriptEditor::clipboardHistory() const
{
    return KTextEditor::EditorPrivate::self()->clipboardHistory();
}

void KateScriptEditor::setClipboardText(const QString &text)
{
    KTextEditor::EditorPrivate::self()->copyToClipboard(text);
}
