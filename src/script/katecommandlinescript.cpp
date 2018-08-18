/*  This file is part of the KDE libraries and the Kate part.
 *
 *  Copyright (C) 2009-2018 Dominik Haumann <dhaumann@kde.org>
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

#include "katecommandlinescript.h"

#include <QJSValue>
#include <QJSEngine>

#include <KLocalizedString>
#include <KShell>

#include "katedocument.h"
#include "kateview.h"
#include "katecmd.h"
#include "katepartdebug.h"

KateCommandLineScript::KateCommandLineScript(const QString &url, const KateCommandLineScriptHeader &header)
    : KateScript(url)
    , KTextEditor::Command(header.functions())
    , m_commandHeader(header)
{
}

KateCommandLineScript::~KateCommandLineScript()
{
}

const KateCommandLineScriptHeader &KateCommandLineScript::commandHeader()
{
    return m_commandHeader;
}

bool KateCommandLineScript::callFunction(const QString &cmd, const QStringList args, QString &errorMessage)
{
    clearExceptions();
    QJSValue command = function(cmd);
    if (!command.isCallable()) {
        errorMessage = i18n("Function '%1' not found in script: %2", cmd, url());
        return false;
    }

    // add the arguments that we are going to pass to the function
    QJSValueList arguments;
    for (const QString &arg : args) {
        arguments << QJSValue(arg);
    }

    QJSValue result = command.call(arguments);
    // error during the calling?
    if (result.isError()) {
       errorMessage = backtrace(result, i18n("Error calling %1", cmd));
       return false;
   }

    return true;
}

bool KateCommandLineScript::exec(KTextEditor::View *view, const QString &cmd, QString &msg,
                                 const KTextEditor::Range &range)
{
    if (range.isValid())
        view->setSelection(range);

    KShell::Errors errorCode;
    QStringList args(KShell::splitArgs(cmd, KShell::NoOptions, &errorCode));

    if (errorCode != KShell::NoError) {
        msg = i18n("Bad quoting in call: %1. Please escape single quotes with a backslash.", cmd);
        return false;
    }

    QString _cmd(args.first());
    args.removeFirst();

    if (!view) {
        msg = i18n("Could not access view");
        return false;
    }

    if (setView(qobject_cast<KTextEditor::ViewPrivate *>(view))) {
        // setView fails if the script cannot be loaded
        // balance edit stack in any case!
        qobject_cast<KTextEditor::ViewPrivate *>(view)->doc()->pushEditState();
        bool success = callFunction(_cmd, args, msg);
        qobject_cast<KTextEditor::ViewPrivate *>(view)->doc()->popEditState();
        return success;
    }

    return false;
}

bool KateCommandLineScript::supportsRange(const QString &)
{
    return true;
}

bool KateCommandLineScript::help(KTextEditor::View *view, const QString &cmd, QString &msg)
{
    if (!setView(qobject_cast<KTextEditor::ViewPrivate *>(view))) {
        // setView fails, if the script cannot be loaded
        return false;
    }

    clearExceptions();
    QJSValue helpFunction = function(QStringLiteral("help"));
    if (!helpFunction.isCallable()) {
        return false;
    }

    // add the arguments that we are going to pass to the function
    QJSValueList arguments;
    arguments << QJSValue(cmd);

    QJSValue result = helpFunction.call(arguments);

    // error during the calling?
    if (result.isError()) {
        msg = backtrace(result, i18n("Error calling 'help %1'", cmd));
        return false;
    }

    if (result.isUndefined() || !result.isString()) {
        qCDebug(LOG_KTE) << i18n("No help specified for command '%1' in script %2", cmd, url());
        return false;
    }
    msg = result.toString();

    return !msg.isEmpty();
}

