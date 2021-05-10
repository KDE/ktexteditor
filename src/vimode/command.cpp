/*
    SPDX-FileCopyrightText: 2008 Erlend Hamberg <ehamberg@gmail.com>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include <vimode/command.h>
#include <vimode/keyparser.h>

#include <QRegularExpression>

using namespace KateVi;

Command::Command(NormalViMode *parent, const QString &pattern, bool (NormalViMode::*commandMethod)(), unsigned int flags)
{
    m_parent = parent;
    m_pattern = KeyParser::self()->encodeKeySequence(pattern);
    m_flags = flags;
    m_ptr2commandMethod = commandMethod;
}

Command::~Command() = default;

bool Command::execute() const
{
    return (m_parent->*m_ptr2commandMethod)();
}

bool Command::matches(const QString &pattern) const
{
    if (!(m_flags & REGEX_PATTERN)) {
        return m_pattern.startsWith(pattern);
    } else {
        const QRegularExpression re(m_pattern, QRegularExpression::UseUnicodePropertiesOption);
        const auto match = re.match(pattern, 0, QRegularExpression::PartialPreferFirstMatch);
        // Partial matching could lead to a complete match, in that case hasPartialMatch() will return false, and hasMatch() will return true
        return match.hasPartialMatch() || match.hasMatch();
    }
}

bool Command::matchesExact(const QString &pattern) const
{
    if (!(m_flags & REGEX_PATTERN)) {
        return (m_pattern == pattern);
    } else {
        const QRegularExpression re(QRegularExpression::anchoredPattern(m_pattern), QRegularExpression::UseUnicodePropertiesOption);
        return re.match(pattern).hasMatch();
    }
}
