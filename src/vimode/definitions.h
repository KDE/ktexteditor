/*
    SPDX-FileCopyrightText: KDE Developers

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef KATEVI_DEFINITIONS_H
#define KATEVI_DEFINITIONS_H

#include <QChar>
#include <QStringView>

#ifdef Q_OS_MACOS
// From the Qt docs: On macOS, the ControlModifier value corresponds to the Command keys on the
// keyboard, and the MetaModifier value corresponds to the Control keys.
#define CONTROL_MODIFIER Qt::MetaModifier
#define META_MODIFIER Qt::ControlModifier
#else
#define CONTROL_MODIFIER Qt::ControlModifier
#define META_MODIFIER Qt::MetaModifier
#endif

#ifdef Q_OS_WINDOWS
// On windows we don't get Qt::GroupSwitchModifier for AltGr,
// but the combination of Ctrl + Alt modifiers
#define ALTGR_MODIFIER (Qt::ControlModifier | Qt::AltModifier)
#else
#define ALTGR_MODIFIER Qt::GroupSwitchModifier
#endif

namespace KateVi
{
/**
 * The four vi modes supported by Kate's vi input mode
 */
enum ViMode {
    NormalMode,
    InsertMode,
    VisualMode,
    VisualLineMode,
    VisualBlockMode,
    ReplaceMode
};

enum OperationMode {
    CharWise = 0,
    LineWise,
    Block
};

const unsigned int EOL = 99999;

inline bool charInRange(const QChar &ch, const QChar &start, const QChar &end)
{
    return ch >= start && ch <= end;
}

inline bool charInList(const QChar &ch, QStringView values)
{
    return values.contains(ch);
}
}

#endif // KATEVI_DEFINITIONS_H
