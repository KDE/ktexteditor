/*
    This file is part of the KDE libraries
    SPDX-FileCopyrightText: 2007 Sebastian Pipping <webmaster@hartwork.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "mode/katewildcardmatcher.h"
#include <QtDebug>
#include <QtGlobal>

bool testCase(const char *candidate, const char *wildcard)
{
    qDebug("\"%s\" / \"%s\"", candidate, wildcard);
    return KateWildcardMatcher::exactMatch(QString(candidate), QString(wildcard));
}

int main()
{
    Q_ASSERT(testCase("abc.txt", "*.txt"));
    Q_ASSERT(!testCase("abc.txt", "*.cpp"));

    Q_ASSERT(testCase("Makefile.am", "*Makefile*"));

    Q_ASSERT(testCase("control", "control"));

    Q_ASSERT(testCase("abcd", "a??d"));

    Q_ASSERT(testCase("a", "?"));
    Q_ASSERT(testCase("a", "*?*"));
    Q_ASSERT(testCase("a", "*"));
    Q_ASSERT(testCase("a", "**"));
    Q_ASSERT(testCase("a", "***"));

    Q_ASSERT(testCase("", "*"));
    Q_ASSERT(testCase("", "**"));
    Q_ASSERT(!testCase("", "?"));

    Q_ASSERT(testCase("ab", "a*"));
    Q_ASSERT(testCase("ab", "*b"));
    Q_ASSERT(testCase("ab", "a?"));
    Q_ASSERT(testCase("ab", "?b"));

    Q_ASSERT(testCase("aXXbXXbYYaYc", "a*b*c"));

    qDebug() << "\nDONE";
    return 0;
}
