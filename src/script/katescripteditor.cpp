/*
    SPDX-FileCopyrightText: 2017 Dominik Haumann <dhaumann@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "katescripteditor.h"

#include <QApplication>
#include <QClipboard>
#include <QJSEngine>

KateScriptEditor::KateScriptEditor(QJSEngine *engine, QObject *parent)
    : QObject(parent)
    , m_engine(engine)
{
}

QString KateScriptEditor::clipboardText() const
{
    return QApplication::clipboard()->text();
}

const QVector<KTextEditor::EditorPrivate::ClipboardEntry> KateScriptEditor::clipboardHistory() const
{
    return KTextEditor::EditorPrivate::self()->clipboardHistory();
}

void KateScriptEditor::setClipboardText(const QString &text, const QString &fileName)
{
    KTextEditor::EditorPrivate::self()->copyToClipboard(text, fileName);
}
