/*
    SPDX-FileCopyrightText: 2007 Sebastian Pipping <webmaster@hartwork.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef KATE_WILDCARD_MATCHER_H
#define KATE_WILDCARD_MATCHER_H

#include <ktexteditor_export.h>

class QString;

namespace KateWildcardMatcher
{
/**
 * Matches a string against a given wildcard.
 * The wildcard supports '*' (".*" in regex) and '?' ("." in regex), not more.
 *
 * @param candidate       Text to match
 * @param wildcard        Wildcard to use
 * @param caseSensitive   Case-sensitivity flag
 * @return                True for an exact match, false otherwise
 */
KTEXTEDITOR_EXPORT bool exactMatch(const QString &candidate, const QString &wildcard, bool caseSensitive = true);

}

#endif // KATE_WILDCARD_MATCHER_H
