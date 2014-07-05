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

#ifndef KATEVI_MAPPINGS_H
#define KATEVI_MAPPINGS_H

#include <ktexteditor_export.h>

#include <QString>
#include <QPair>
#include <QMap>
#include <QHash>

class KConfigGroup;
class KateViInputMode;

namespace KateVi
{

class KTEXTEDITOR_EXPORT Mappings
{
public:
    enum MappingRecursion { Recursive, NonRecursive };
    enum MappingMode { NormalModeMapping, VisualModeMapping, InsertModeMapping, CommandModeMapping };

public:

    explicit Mappings();
    ~Mappings();

    void writeConfig(KConfigGroup &config) const;
    void readConfig(const KConfigGroup &config);

    void add(MappingMode mode, const QString &from, const QString &to, MappingRecursion recursion);
    void remove(MappingMode mode, const QString &from);
    void clear(MappingMode mode);

    QString get(MappingMode mode, const QString &from, bool decode = false) const;
    QStringList getAll(MappingMode mode, bool decode = false) const;
    bool isRecursive(MappingMode mode, const QString &from) const;

public:
    /**
     * Returns CommandModeMapping if the emulated command bar is active, else the mapping mode
     * corresponding to the current Vi mode.
     */
    static MappingMode mappingModeForCurrentViMode(KateViInputMode *viInputMode);

private:
    void writeMappings(KConfigGroup &config, const QString &mappingModeName, MappingMode mappingMode) const;
    void readMappings(const KConfigGroup &config, const QString &mappingModeName, MappingMode mappingMode);

private:
    typedef QPair<QString, bool> Mapping;
    typedef QHash<QString, Mapping> MappingList;

    QMap<MappingMode, MappingList> m_mappings;
};

}

#endif // KATEVI_MAPPINGS_H
