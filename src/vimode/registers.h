/*
    SPDX-FileCopyrightText: KDE Developers

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef KATEVI_REGISTERS_H
#define KATEVI_REGISTERS_H

#include "definitions.h"

#include <QChar>
#include <QList>
#include <QString>
#include <map>

using namespace Qt::StringLiterals;

class KConfigGroup;

namespace KateVi
{
constexpr QChar BlackHoleRegister = '_'_L1;
constexpr QChar SmallDeleteRegister = '-'_L1;
constexpr QChar ZeroRegister = '0'_L1;
constexpr QChar PrependNumberedRegister = '!'_L1;
constexpr QChar FirstNumberedRegister = '1'_L1;
constexpr QChar LastNumberedRegister = '9'_L1;
constexpr QChar SystemSelectionRegister = '*'_L1;
constexpr QChar SystemClipboardRegister = '+'_L1;
constexpr QChar UnnamedRegister = '"'_L1;
constexpr QChar InsertStoppedRegister = '^'_L1;

constexpr auto SpecialRegisters = std::array{BlackHoleRegister,
                                             SmallDeleteRegister,
                                             ZeroRegister,
                                             PrependNumberedRegister,
                                             SystemSelectionRegister,
                                             SystemClipboardRegister,
                                             UnnamedRegister,
                                             InsertStoppedRegister};

class Registers
{
public:
    void writeConfig(KConfigGroup &config) const;
    void readConfig(const KConfigGroup &config);

    void setInsertStopped(const QString &text);

    void set(const QChar &reg, const QString &text, OperationMode flag = CharWise);
    QString getContent(const QChar &reg) const;
    OperationMode getFlag(const QChar &reg) const;

    static bool isValidRegister(const QChar &reg);

private:
    struct Register {
        QString contents;
        OperationMode opMode;
    };

private:
    void setNumberedRegister(const QChar &reg, const QString &text, OperationMode flag = CharWise);
    Register getRegister(const QChar &reg) const;

private:
    QList<Register> m_numbered;
    std::map<QChar, Register> m_registers;
    QChar m_default;
};

}

#endif // KATEVI_REGISTERS_H
