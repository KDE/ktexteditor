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

#ifndef KATE_SCRIPTHELPERS_H
#define KATE_SCRIPTHELPERS_H

#include <QObject>
#include <QJSValue>
#include <ktexteditor_export.h>

class QJSEngine;

namespace Kate
{
/** Top-level script functions */
namespace Script
{

/** read complete file contents, helper */
KTEXTEDITOR_EXPORT bool readFile(const QString &sourceUrl, QString &sourceCode);

} // namespace Script

class KTEXTEDITOR_EXPORT ScriptHelper : public QObject {
    Q_OBJECT
    QJSEngine *m_engine;
public:
    explicit ScriptHelper(QJSEngine *engine) : m_engine(engine) {}
    Q_INVOKABLE QString read(const QString &file);
    Q_INVOKABLE void require(const QString &file);
    Q_INVOKABLE void debug(const QString &msg);
    Q_INVOKABLE QString _i18n(const QString &msg);
    Q_INVOKABLE QString _i18nc(const QString &textContext, const QString &text);
    Q_INVOKABLE QString _i18np(const QString &trSingular, const QString &trPlural, int number);
    Q_INVOKABLE QString _i18ncp(const QString &trContext, const QString &trSingular,
                                const QString &trPlural, int number = 0);
};

} // namespace Kate

#endif

