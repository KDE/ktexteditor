/*  This file is part of the KDE libraries and the Kate part.
 *
 *  Copyright (C) 2008 Erlend Hamberg <ehamberg@gmail.com>
 *  Copyright (C) 2011 Svyatoslav Kuzmich <svatoslav1@gmail.com>
 *  Copyright (C) 2012 - 2013 Simon St James <kdedevel@etotheipiplusone.com>
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

#ifndef KATE_VI_GLOBAL_H_INCLUDED
#define KATE_VI_GLOBAL_H_INCLUDED

#include <QMap>
#include <QHash>
#include <QList>
#include <QPair>

#include <KSharedConfig>

#include "katevimodebase.h"
#include <ktexteditor_export.h>

class QString;
class QChar;
class KConfigGroup;
class KateViInputMode;

namespace KateVi
{
const unsigned int EOL = 99999;
    class History;
    class Macros;
    class Mappings;
}

typedef QPair<QString, OperationMode> KateViRegister;

class KTEXTEDITOR_EXPORT KateViGlobal
{
public:
    KateViGlobal();
    ~KateViGlobal();

    /**
     * The global configuration of katepart for the vi mode, e.g. katevirc
     * @return global shared access to katevirc config
     */
    static KSharedConfigPtr config()
    {
        return KSharedConfig::openConfig(QStringLiteral("katevirc"));
    }

    void writeConfig(KConfig *config) const;
    void readConfig(const KConfig *config);
    QString getRegisterContent(const QChar &reg) const;
    OperationMode getRegisterFlag(const QChar &reg) const;
    void addToNumberedRegister(const QString &text, OperationMode flag = CharWise);
    void fillRegister(const QChar &reg, const QString &text, OperationMode flag = CharWise);
    const QMap<QChar, KateViRegister> *getRegisters() const
    {
        return &m_registers;
    }

    inline KateVi::Mappings *mappings() { return m_mappings; }

    inline KateVi::History *searchHistory() { return m_searchHistory; }
    inline KateVi::History *commandHistory() { return m_commandHistory; }
    inline KateVi::History *replaceHistory() { return m_replaceHistory; }

    inline KateVi::Macros *macros() { return m_macros; }

private:
    // registers
    QList<KateViRegister> m_numberedRegisters;
    QMap<QChar, KateViRegister> m_registers;
    QChar m_defaultRegister;
    QString m_registerTemp;
    KateViRegister getRegister(const QChar &reg) const;

    KateVi::Mappings *m_mappings;

    KateVi::History *m_searchHistory;
    KateVi::History *m_commandHistory;
    KateVi::History *m_replaceHistory;

    KateVi::Macros *m_macros;
};

#endif
