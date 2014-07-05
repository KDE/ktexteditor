/*  This file is part of the KDE libraries and the Kate part.
 *
 *  Copyright (C) 2008-2011 Erlend Hamberg <ehamberg@gmail.com>
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

#include <stdio.h>

#include "kateviglobal.h"
#include "katevikeyparser.h"
#include "kateviemulatedcommandbar.h"
#include "katepartdebug.h"
#include "kateviinputmode.h"
#include "history.h"
#include "macros.h"
#include "mappings.h"

#include <kconfiggroup.h>
#include <ktexteditor/movingcursor.h>
#include <QApplication>
#include <QClipboard>

using namespace KateVi;

KateViGlobal::KateViGlobal()
{
    m_searchHistory = new History();
    m_replaceHistory = new History();
    m_commandHistory = new History();
    m_macros = new Macros();
    m_mappings = new Mappings();

    // read global settings
    readConfig(config().data());
}

KateViGlobal::~KateViGlobal()
{
    // write global settings
    writeConfig(config().data());
    config().data()->sync();

    delete m_searchHistory;
    delete m_replaceHistory;
    delete m_commandHistory;
    delete m_macros;
    delete m_mappings;
}

void KateViGlobal::writeConfig(KConfig *configFile) const
{
    // FIXME: use own groups instead of one big group!
    KConfigGroup config(configFile, "Kate Vi Input Mode Settings");
    m_macros->writeConfig(config);
    m_mappings->writeConfig(config);

    if (m_registers.isEmpty()) {
        return;
    }

    QStringList names, contents;
    QList<int> flags;
    QMap<QChar, KateViRegister>::const_iterator i;
    for (i = m_registers.constBegin(); i != m_registers.constEnd(); ++i) {
        if (i.value().first.length() <= 1000) {
            names << i.key();
            contents << i.value().first;
            flags << int(i.value().second);
        } else {
            qCDebug(LOG_PART) << "Did not save contents of register " << i.key() << ": contents too long ("
            << i.value().first.length() << " characters)";
        }
    }

    config.writeEntry("ViRegisterNames", names);
    config.writeEntry("ViRegisterContents", contents);
    config.writeEntry("ViRegisterFlags", flags);
}

void KateViGlobal::readConfig(const KConfig *configFile)
{
    // FIXME: use own groups instead of one big group!
    const KConfigGroup config(configFile, "Kate Vi Input Mode Settings");
    
    m_macros->readConfig(config);
    m_mappings->readConfig(config);

    QStringList names = config.readEntry("ViRegisterNames", QStringList());
    QStringList contents = config.readEntry("ViRegisterContents", QStringList());
    QList<int> flags = config.readEntry("ViRegisterFlags", QList<int>());

    // sanity check
    if (names.size() == contents.size() && contents.size() == flags.size()) {
        for (int i = 0; i < names.size(); i++) {
            if (!names.at(i).isEmpty()) {
                fillRegister(names.at(i).at(0), contents.at(i), (OperationMode)(flags.at(i)));
            }
        }
    }
}

KateViRegister KateViGlobal::getRegister(const QChar &reg) const
{
    KateViRegister regPair;
    QChar _reg = (reg != QLatin1Char('"') ? reg : m_defaultRegister);

    if (_reg >= QLatin1Char('1') && _reg <= QLatin1Char('9')) {   // numbered register
        int index = QString(_reg).toInt() - 1;
        if (m_numberedRegisters.size() > index) {
            regPair = m_numberedRegisters.at(index);
        }
    } else if (_reg == QLatin1Char('+')) {   // system clipboard register
        QString regContent = QApplication::clipboard()->text(QClipboard::Clipboard);
        regPair = KateViRegister(regContent, CharWise);
    } else if (_reg == QLatin1Char('*')) {   // system selection register
        QString regContent = QApplication::clipboard()->text(QClipboard::Selection);
        regPair = KateViRegister(regContent, CharWise);
    } else { // regular, named register
        if (m_registers.contains(_reg)) {
            regPair = m_registers.value(_reg);
        }
    }

    return regPair;
}

QString KateViGlobal::getRegisterContent(const QChar &reg) const
{
    return getRegister(reg).first;
}

OperationMode KateViGlobal::getRegisterFlag(const QChar &reg) const
{
    return getRegister(reg).second;
}

void KateViGlobal::addToNumberedRegister(const QString &text, OperationMode flag)
{
    if (m_numberedRegisters.size() == 9) {
        m_numberedRegisters.removeLast();
    }

    // register 0 is used for the last yank command, so insert at position 1
    m_numberedRegisters.prepend(KateViRegister(text, flag));

    qCDebug(LOG_PART) << "Register 1-9:";
    for (int i = 0; i < m_numberedRegisters.size(); i++) {
        qCDebug(LOG_PART) << "\t Register " << i + 1 << ": " << m_numberedRegisters.at(i);
    }
}

void KateViGlobal::fillRegister(const QChar &reg, const QString &text, OperationMode flag)
{
    // the specified register is the "black hole register", don't do anything
    if (reg == QLatin1Char('_')) {
        return;
    }

    if (reg >= QLatin1Char('1') && reg <= QLatin1Char('9')) {   // "kill ring" registers
        addToNumberedRegister(text);
    } else if (reg == QLatin1Char('+')) {   // system clipboard register
        QApplication::clipboard()->setText(text,  QClipboard::Clipboard);
    } else if (reg == QLatin1Char('*')) {   // system selection register
        QApplication::clipboard()->setText(text, QClipboard::Selection);
    } else {
        m_registers.insert(reg, KateViRegister(text, flag));
    }

    qCDebug(LOG_PART) << "Register " << reg << " set to " << getRegisterContent(reg);

    if (reg == QLatin1Char('0') || reg == QLatin1Char('1') || reg == QLatin1Char('-')) {
        m_defaultRegister = reg;
        qCDebug(LOG_PART) << "Register " << '"' << " set to point to \"" << reg;
    }
}
