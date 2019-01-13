/*  This file is part of the KDE libraries and the Kate part.
 *
 *  Copyright (C) 2008 Erlend Hamberg <ehamberg@gmail.com>
 *  Copyright (C) 2008 Evgeniy Ivanov <powerfox@kde.ru>
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

#ifndef KATEVI_KEY_PARSER_H
#define KATEVI_KEY_PARSER_H

#include <QChar>
#include <QString>
#include <QHash>
#include <ktexteditor_export.h>

class QKeyEvent;

namespace KateVi
{

/**
 * for encoding keypresses w/ modifiers into an internal QChar representation and back again to a
 * descriptive text string
 */
class KTEXTEDITOR_EXPORT KeyParser
{
private:
    KeyParser();

public:
    static KeyParser *self();
    ~KeyParser()
    {
        m_instance = nullptr;
    }
    KeyParser(const KeyParser &) = delete;
    KeyParser& operator=(const KeyParser &) = delete;

    const QString encodeKeySequence(const QString &keys) const;
    const QString decodeKeySequence(const QString &keys) const;
    QString qt2vi(int key) const;
    int vi2qt(const QString &keypress) const;
    int encoded2qt(const QString &keypress) const;
    const QChar KeyEventToQChar(const QKeyEvent &keyEvent);

private:
    void initKeyTables();

    QHash<int, QString> m_qt2katevi;
    QHash<QString, int> m_katevi2qt;
    QHash<QString, int> m_nameToKeyCode;
    QHash<int, QString> m_keyCodeToName;

    static KeyParser *m_instance;
};

}

#endif /* KATEVI_KEY_PARSER_H */
