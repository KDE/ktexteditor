/*  This file is part of the KDE libraries
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

#ifndef KATEVI_REGISTERS_H
#define KATEVI_REGISTERS_H

#include "definitions.h"

#include <QChar>
#include <QList>
#include <QMap>
#include <QString>

class KConfigGroup;

namespace KateVi
{
const QChar BlackHoleRegister = QLatin1Char('_');
const QChar SmallDeleteRegister = QLatin1Char('-');
const QChar ZeroRegister = QLatin1Char('0');
const QChar FirstNumberedRegister = QLatin1Char('1');
const QChar LastNumberedRegister = QLatin1Char('9');
const QChar SystemSelectionRegister = QLatin1Char('*');
const QChar SystemClipboardRegister = QLatin1Char('+');
const QChar UnnamedRegister = QLatin1Char('"');
const QChar InsertStoppedRegister = QLatin1Char('^');

class Registers
{
public:
    explicit Registers();
    ~Registers();

    void writeConfig(KConfigGroup &config) const;
    void readConfig(const KConfigGroup &config);

    void setInsertStopped(const QString &text);

    void set(const QChar &reg, const QString &text, OperationMode flag = CharWise);
    QString getContent(const QChar &reg) const;
    OperationMode getFlag(const QChar &reg) const;

private:
    typedef QPair<QString, OperationMode> Register;

private:
    void setNumberedRegister(const QString &text, OperationMode flag = CharWise);
    Register getRegister(const QChar &reg) const;

private:
    QList<Register> m_numbered;
    QMap<QChar, Register> m_registers;
    QChar m_default;
};

}

#endif // KATEVI_REGISTERS_H
