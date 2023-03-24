/*
    SPDX-FileCopyrightText: KDE Developers

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef KATEVI_MAPPINGS_H
#define KATEVI_MAPPINGS_H

#include <QHash>
#include <ktexteditor_export.h>

class KConfigGroup;
class KateViInputMode;

namespace KateVi
{
class KTEXTEDITOR_EXPORT Mappings
{
public:
    enum MappingRecursion { Recursive, NonRecursive };

    enum MappingMode { NormalModeMapping = 0, VisualModeMapping, InsertModeMapping, CommandModeMapping };

public:
    void writeConfig(KConfigGroup &config) const;
    void readConfig(const KConfigGroup &config);

    void add(MappingMode mode, const QString &from, const QString &to, MappingRecursion recursion);
    void remove(MappingMode mode, const QString &from);
    void clear(MappingMode mode);

    QString get(MappingMode mode, const QString &from, bool decode = false, bool includeTemporary = false) const;
    QStringList getAll(MappingMode mode, bool decode = false, bool includeTemporary = false) const;
    bool isRecursive(MappingMode mode, const QString &from) const;

    void setLeader(const QChar &leader);

public:
    /**
     * Returns CommandModeMapping if the emulated command bar is active, else the mapping mode
     * corresponding to the current Vi mode.
     */
    static MappingMode mappingModeForCurrentViMode(KateViInputMode *viInputMode);

private:
    KTEXTEDITOR_NO_EXPORT
    void writeMappings(KConfigGroup &config, const QString &mappingModeName, MappingMode mappingMode) const;
    KTEXTEDITOR_NO_EXPORT
    void readMappings(const KConfigGroup &config, const QString &mappingModeName, MappingMode mappingMode);

private:
    typedef struct {
        // The real value of the mapping.
        QString encoded;

        // True if it's recursive, false otherwise.
        bool recursive;

        // True if this mapping should not be read/written in the config.
        // Used for temporary mapping (e.g. mappings with <leader>).
        bool temporary;
    } Mapping;
    typedef QHash<QString, Mapping> MappingList;

    MappingList m_mappings[4];
    QChar m_leader;
};

}

#endif // KATEVI_MAPPINGS_H
