/*
    SPDX-FileCopyrightText: 2008 Erlend Hamberg <ehamberg@gmail.com>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include <vimode/command.h>
#include <vimode/keyparser.h>

#include <QRegExp>

using namespace KateVi;

Command::Command(NormalViMode *parent, const QString &pattern, bool (NormalViMode::*commandMethod)(), unsigned int flags)
{
    m_parent = parent;
    m_pattern = KeyParser::self()->encodeKeySequence(pattern);
    m_flags = flags;
    m_ptr2commandMethod = commandMethod;
}

Command::~Command()
{
}

bool Command::execute() const
{
    return (m_parent->*m_ptr2commandMethod)();
}

bool Command::matches(const QString &pattern) const
{
    if (!(m_flags & REGEX_PATTERN)) {
        return m_pattern.startsWith(pattern);
    } else {
        QRegExp re(m_pattern);
        re.exactMatch(pattern);
        return (re.matchedLength() == pattern.length());
    }
}

bool Command::matchesExact(const QString &pattern) const
{
    if (!(m_flags & REGEX_PATTERN)) {
        return (m_pattern == pattern);
    } else {
        QRegExp re(m_pattern);
        return re.exactMatch(pattern);
    }
}
