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
}

void KateViGlobal::writeConfig(KConfig *configFile) const
{
    // FIXME: use own groups instead of one big group!
    KConfigGroup config(configFile, "Kate Vi Input Mode Settings");
    
    writeMappingsToConfig(config, QLatin1String("Normal"), NormalModeMapping);
    writeMappingsToConfig(config, QLatin1String("Visual"), VisualModeMapping);
    writeMappingsToConfig(config, QLatin1String("Insert"), InsertModeMapping);
    writeMappingsToConfig(config, QLatin1String("Command"), CommandModeMapping);

    m_macros->writeConfig(config);

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
    
    readMappingsFromConfig(config, QLatin1String("Normal"), NormalModeMapping);
    readMappingsFromConfig(config, QLatin1String("Visual"), VisualModeMapping);
    readMappingsFromConfig(config, QLatin1String("Insert"), InsertModeMapping);
    readMappingsFromConfig(config, QLatin1String("Command"), CommandModeMapping);

    m_macros->readConfig(config);

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

void KateViGlobal::addMapping(MappingMode mode, const QString &from, const QString &to, KateViGlobal::MappingRecursion recursion)
{
    const QString encodedMapping = KateViKeyParser::self()->encodeKeySequence(from);
    const QString encodedTo = KateViKeyParser::self()->encodeKeySequence(to);
    const Mapping mapping(encodedTo, recursion == KateViGlobal::Recursive);
    if (!from.isEmpty()) {
        m_mappingsForMode[mode][encodedMapping] = mapping;
    }
}

void KateViGlobal::removeMapping(MappingMode mode, const QString &from)
{
    m_mappingsForMode[mode].remove(from);
}

const QString KateViGlobal::getMapping(MappingMode mode, const QString &from, bool decode) const
{
    const QString ret = m_mappingsForMode[mode][from].mappedKeyPresses;

    if (decode) {
        return KateViKeyParser::self()->decodeKeySequence(ret);
    }
    return ret;
}

const QStringList KateViGlobal::getMappings(MappingMode mode, bool decode) const
{
    const QHash <QString, Mapping> mappingsForMode = m_mappingsForMode[mode];

    QStringList mappings;
    foreach (const QString mapping, mappingsForMode.keys()) {
        if (decode) {
            mappings << KateViKeyParser::self()->decodeKeySequence(mapping);
        } else {
            mappings << mapping;
        }
    }

    return mappings;
}

bool KateViGlobal::isMappingRecursive(MappingMode mode, const QString &from) const
{
    return m_mappingsForMode[mode][from].isRecursive;
}

KateViGlobal::MappingMode KateViGlobal::mappingModeForCurrentViMode(KateViInputMode *viInputMode)
{
    if (viInputMode->viModeEmulatedCommandBar()->isActive()) {
        return CommandModeMapping;
    }
    const ViMode mode = viInputMode->viInputModeManager()->getCurrentViMode();
    switch (mode) {
    case NormalMode:
        return NormalModeMapping;
    case VisualMode:
    case VisualLineMode:
    case VisualBlockMode:
        return VisualModeMapping;
    case InsertMode:
    case ReplaceMode:
        return InsertModeMapping;
    default:
        Q_ASSERT(false && "unrecognised ViMode!");
        return NormalModeMapping; // Return arbitrary mode to satisfy compiler.
    }
}

void KateViGlobal::clearMappings(MappingMode mode)
{
    m_mappingsForMode[mode].clear();
}

void KateViGlobal::writeMappingsToConfig(KConfigGroup &config, const QString &mappingModeName, MappingMode mappingMode) const
{
    config.writeEntry(mappingModeName + QLatin1String(" Mode Mapping Keys"), getMappings(mappingMode, true));
    QStringList l;
    QList<bool> isRecursive;
    foreach (const QString &s, getMappings(mappingMode)) {
        l << KateViKeyParser::self()->decodeKeySequence(getMapping(mappingMode, s));
        isRecursive << isMappingRecursive(mappingMode, s);
    }
    config.writeEntry(mappingModeName + QLatin1String(" Mode Mappings"), l);
    config.writeEntry(mappingModeName + QLatin1String(" Mode Mappings Recursion"), isRecursive);
}

void KateViGlobal::readMappingsFromConfig(const KConfigGroup &config, const QString &mappingModeName, MappingMode mappingMode)
{
    const QStringList keys = config.readEntry(mappingModeName + QLatin1String(" Mode Mapping Keys"), QStringList());
    const QStringList mappings = config.readEntry(mappingModeName + QLatin1String(" Mode Mappings"), QStringList());
    const QList<bool> isRecursive = config.readEntry(mappingModeName + QLatin1String(" Mode Mappings Recursion"), QList<bool>());

    // sanity check
    if (keys.length() == mappings.length()) {
        for (int i = 0; i < keys.length(); i++) {
            // "Recursion" is a newly-introduced part of the config that some users won't have,
            // so rather than abort (and lose our mappings) if there are not enough entries, simply
            // treat any missing ones as Recursive (for backwards compatibility).
            MappingRecursion recursion = Recursive;
            if (isRecursive.size() > i && !isRecursive.at(i)) {
                recursion = NonRecursive;
            }
            addMapping(mappingMode, keys.at(i), mappings.at(i), recursion);
            qCDebug(LOG_PART) <<  + " mapping " << keys.at(i) << " -> " << mappings.at(i);
        }
    } else {
        qCDebug(LOG_PART) << "Error when reading mappings from " << mappingModeName << " config: number of keys != number of values";
    }
}
