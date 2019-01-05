/* This file is part of the KDE libraries
 *
 * Copyright (C) 2010-2018 Dominik Haumann <dhaumann@kde.org>
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

// Undefine this because we don't want our i18n*() method names to be turned
// into i18nd*(). This means any translated string in this file should use
// i18nd("ktexteditor5", "foo") instead of i18n("foo")
#undef TRANSLATION_DOMAIN

#include "katescripthelpers.h"
#include "katescript.h"
#include "katescriptdocument.h"
#include "katescriptview.h"
#include "kateview.h"
#include "katedocument.h"
#include "katepartdebug.h"

#include <iostream>

#include <QJSEngine>
#include <QJSValue>
#include <QFile>
#include <QStandardPaths>

#include <QDebug>

#include <KLocalizedString>

namespace Kate
{
namespace Script
{

bool readFile(const QString &sourceUrl, QString &sourceCode)
{
    sourceCode = QString();

    QFile file(sourceUrl);
    if (!file.open(QIODevice::ReadOnly)) {
        qCDebug(LOG_KTE) << QStringLiteral("Unable to find '%1'").arg(sourceUrl);
        return false;
    } else {
        QTextStream stream(&file);
        stream.setCodec("UTF-8");
        sourceCode = stream.readAll();
        file.close();
    }
    return true;
}

} // namespace Script

QString ScriptHelper::read(const QString &file)
{
    QList<QString> files;
    files << file;
    /**
     * just search for all given files and read them all
     */
    QString fullContent;
    for (int i = 0; i < files.count(); ++i) {
        /**
         * get full name of file
         * skip on errors
         */
        const QString name = files[i];
        QString fullName = QStandardPaths::locate(QStandardPaths::GenericDataLocation,
                                 QLatin1String("katepart5/script/files/") + name);
        if (fullName.isEmpty()) {
            /**
             * retry with resource
             */
            fullName = QLatin1String(":/ktexteditor/script/files/") + name;
            if (!QFile::exists(fullName))
                continue;
        }

        /**
         * try to read complete file
         * skip non-existing files
         */
        QString content;
        if (!Script::readFile(fullName, content)) {
            continue;
        }

        /**
         * concat to full content
         */
        fullContent += content;
    }

    /**
     * return full content
     */
    return fullContent;
}

void ScriptHelper::require(const QString &file)
{
    QStringList files;
    files << file;
    /**
     * just search for all given scripts and eval them
     */
    for (int i = 0; i < files.count(); ++i) {
        /**
         * get full name of file
         * skip on errors
         */
        const QString name = files[i];
        QString fullName = QStandardPaths::locate(QStandardPaths::GenericDataLocation,
                                 QLatin1String("katepart5/script/libraries/") + name);
        if (fullName.isEmpty()) {
            /**
             * retry with resource
             */
            fullName = QLatin1String(":/ktexteditor/script/libraries/") + name;
            if (!QFile::exists(fullName))
                continue;
        }

        /**
         * check include guard
         */
        QJSValue require_guard = m_engine->globalObject().property(QStringLiteral("require_guard"));
        if (require_guard.property(fullName).toBool()) {
            continue;
        }

        /**
         * try to read complete file
         * skip non-existing files
         */
        QString code;
        if (!Script::readFile(fullName, code)) {
            continue;
        }

        /**
         * eval in current script engine
         */
        QJSValue val = m_engine->evaluate(code, fullName);
        if (val.isError()) {
            qWarning() << "error evaluating" << fullName << val.toString() << ", at line" << val.property(QStringLiteral("lineNumber")).toInt();
        }


        /**
         * set include guard
         */
        require_guard.setProperty(fullName, QJSValue(true));
    }
}

void ScriptHelper::debug(const QString &message)
{
    // debug in blue to distance from other debug output if necessary
    std::cerr << "\033[31m" << qPrintable(message) << "\033[0m\n";
}

//BEGIN code adapted from kdelibs/kross/modules/translation.cpp
/// helper function to do the substitution from QtScript > QVariant > real values
//KLocalizedString substituteArguments(const KLocalizedString &kls, const QVariantList &arguments, int max = 99)
//{
//    KLocalizedString ls = kls;
//    int cnt = qMin(arguments.count(), max);   // QString supports max 99
//    for (int i = 0; i < cnt; ++i) {
//        QVariant arg = arguments[i];
//        switch (arg.type()) {
//        case QVariant::Int: ls = ls.subs(arg.toInt()); break;
//        case QVariant::UInt: ls = ls.subs(arg.toUInt()); break;
//        case QVariant::LongLong: ls = ls.subs(arg.toLongLong()); break;
//        case QVariant::ULongLong: ls = ls.subs(arg.toULongLong()); break;
//        case QVariant::Double: ls = ls.subs(arg.toDouble()); break;
//        default: ls = ls.subs(arg.toString()); break;
//        }
//    }
//    return ls;
//}

/// i18n("text", arguments [optional])
QString ScriptHelper::_i18n(const QString &text)
{
    KLocalizedString ls = ki18n(text.toUtf8().constData());
    return ls.toString(); //substituteArguments(ls, args).toString();
}

/// i18nc("context", "text", arguments [optional])
QString ScriptHelper::_i18nc(const QString &textContext, const QString &text)
{
    KLocalizedString ls = ki18nc(textContext.toUtf8().constData(), text.toUtf8().constData());
    return ls.toString(); //substituteArguments(ls, args).toString();
}

/// i18np("singular", "plural", number, arguments [optional])
QString ScriptHelper::_i18np(const QString &trSingular, const QString &trPlural, int number)
{
    KLocalizedString ls = ki18np(trSingular.toUtf8().constData(), trPlural.toUtf8().constData()).subs(number);
    return ls.toString(); //substituteArguments(ls, args, 98).toString();
}

/// i18ncp("context", "singular", "plural", number, arguments [optional])
QString ScriptHelper::_i18ncp(
        const QString &trContext, const QString &trSingular,
        const QString &trPlural, int number)
{
    KLocalizedString ls = ki18ncp(trContext.toUtf8().data(), trSingular.toUtf8().data(), trPlural.toUtf8().data()).subs(number);
    return ls.toString(); // substituteArguments(ls, args, 98).toString();
}

//END code adapted from kdelibs/kross/modules/translation.cpp

} // namespace kate

