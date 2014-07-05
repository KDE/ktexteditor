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

#include "mappings.h"

#include "katepartdebug.h"
#include "katevikeyparser.h"
#include "kateviinputmode.h"
#include "kateviinputmodemanager.h"
#include "kateviemulatedcommandbar.h"

#include <KConfigGroup>

using namespace KateVi;

Mappings::Mappings()
{
}

Mappings::~Mappings()
{
}

void Mappings::readConfig(const KConfigGroup &config)
{
    readMappings(config, QLatin1String("Normal"), NormalModeMapping);
    readMappings(config, QLatin1String("Visual"), VisualModeMapping);
    readMappings(config, QLatin1String("Insert"), InsertModeMapping);
    readMappings(config, QLatin1String("Command"), CommandModeMapping);
}

void Mappings::writeConfig(KConfigGroup &config) const
{
    writeMappings(config, QLatin1String("Normal"), NormalModeMapping);
    writeMappings(config, QLatin1String("Visual"), VisualModeMapping);
    writeMappings(config, QLatin1String("Insert"), InsertModeMapping);
    writeMappings(config, QLatin1String("Command"), CommandModeMapping);
}

void Mappings::writeMappings(KConfigGroup &config, const QString &mappingModeName, MappingMode mappingMode) const
{
    config.writeEntry(mappingModeName + QLatin1String(" Mode Mapping Keys"), getAll(mappingMode, true));
    QStringList l;
    QList<bool> recursives;
    foreach (const QString &s, getAll(mappingMode)) {
        l << KateViKeyParser::self()->decodeKeySequence(get(mappingMode, s));
        recursives << isRecursive(mappingMode, s);
    }
    config.writeEntry(mappingModeName + QLatin1String(" Mode Mappings"), l);
    config.writeEntry(mappingModeName + QLatin1String(" Mode Mappings Recursion"), recursives);
}

void Mappings::readMappings(const KConfigGroup &config, const QString &mappingModeName, MappingMode mappingMode)
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
            add(mappingMode, keys.at(i), mappings.at(i), recursion);
            qCDebug(LOG_PART) <<  + " mapping " << keys.at(i) << " -> " << mappings.at(i);
        }
    } else {
        qCDebug(LOG_PART) << "Error when reading mappings from " << mappingModeName << " config: number of keys != number of values";
    }
}

void Mappings::add(MappingMode mode, const QString &from, const QString &to, MappingRecursion recursion)
{
    const QString encodedMapping = KateViKeyParser::self()->encodeKeySequence(from);
    const QString encodedTo = KateViKeyParser::self()->encodeKeySequence(to);
    const Mapping mapping(encodedTo, recursion == Recursive);
    if (!from.isEmpty()) {
        m_mappings[mode][encodedMapping] = mapping;
    }
}

void Mappings::remove(MappingMode mode, const QString &from)
{
    m_mappings[mode].remove(from);
}

void Mappings::clear(MappingMode mode)
{
    m_mappings[mode].clear();
}

QString Mappings::get(MappingMode mode, const QString &from, bool decode) const
{
    if (!m_mappings[mode].contains(from)) {
        return QString();
    }

    QString ret = m_mappings[mode][from].first;

    if (decode) {
        return KateViKeyParser::self()->decodeKeySequence(ret);
    }

    return ret;
}

QStringList Mappings::getAll(MappingMode mode, bool decode) const
{
    const QHash <QString, Mapping> mappingsForMode = m_mappings[mode];

    QStringList mappings;
    foreach (const QString &mapping, mappingsForMode.keys()) {
        if (decode) {
            mappings << KateViKeyParser::self()->decodeKeySequence(mapping);
        } else {
            mappings << mapping;
        }
    }

    return mappings;
}

bool Mappings::isRecursive(MappingMode mode, const QString &from) const
{
    if (!m_mappings[mode].contains(from)) {
        return false;
    }

    return m_mappings[mode][from].second;
}

Mappings::MappingMode Mappings::mappingModeForCurrentViMode(KateViInputMode *viInputMode)
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
