/*
    SPDX-FileCopyrightText: 2010 Bernhard Beschow <bbeschow@cs.tu-berlin.de>
    SPDX-FileCopyrightText: 2007 Sebastian Pipping <webmaster@hartwork.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef _KATE_REGEXPSEARCH_H_
#define _KATE_REGEXPSEARCH_H_

#include <QObject>

#include <ktexteditor/range.h>

#include <ktexteditor_export.h>

namespace KTextEditor
{
class Document;
}

/**
 * Object to help to search for regexp.
 * This should be NO QObject, it is created to often!
 * I measured that, if you create it 20k times to replace for example " " in a document, that takes seconds on a modern machine!
 */
class KTEXTEDITOR_EXPORT KateRegExpSearch
{
public:
    explicit KateRegExpSearch(const KTextEditor::Document *document, Qt::CaseSensitivity caseSensitivity);
    ~KateRegExpSearch();

    //
    // KTextEditor::SearchInterface stuff
    //
public:
    /**
     * Search for the regular expression \p pattern inside the range
     * \p inputRange. If \p backwards is \e true, the search direction will
     * be reversed.
     *
     * \param pattern text to search for
     * \param inputRange Range to search in
     * \param backwards if \e true, the search will be backwards
     * \return Vector of ranges, one for each capture. The first range (index zero)
     *        spans the full match. If the pattern does not match the vector
     *        has length 1 and holds the invalid range (see Range::isValid()).
     * \see KTextEditor::Range, QRegularExpression
     */
    QVector<KTextEditor::Range> search(const QString &pattern, const KTextEditor::Range &inputRange, bool backwards = false);

    /**
     * Returns a modified version of text where escape sequences are resolved, e.g. "\\n" to "\n".
     *
     * \param text text containing escape sequences
     * \return text with resolved escape sequences
     */
    static QString escapePlaintext(const QString &text);

    /**
     * Returns a modified version of text where
     * \li escape sequences are resolved, e.g. "\\n" to "\n",
     * \li references are resolved, e.g. "\\1" to <i>1st entry in capturedTexts</i>, and
     * \li counter sequences are resolved, e.g. "\\#...#" to <i>replacementCounter</i>.
     *
     * \param text text containing escape sequences, references, and counter sequences
     * \param capturedTexts list of substitutes for references
     * \param replacementCounter value for replacement counter
     * \return resolved text
     */
    static QString buildReplacement(const QString &text, const QStringList &capturedTexts, int replacementCounter);

private:
    /**
     * Implementation of escapePlainText() and public buildReplacement().
     *
     * \param text text containing escape sequences and possibly references and counters
     * \param capturedTexts list of substitutes for references
     * \param replacementCounter value for replacement counter (only used when replacementGoodies == true)
     * \param replacementGoodies @c true for buildReplacement(), @c false for escapePlainText()
     * \return resolved text
     */
    static QString buildReplacement(const QString &text, const QStringList &capturedTexts, int replacementCounter, bool replacementGoodies);

private:
    const KTextEditor::Document *const m_document;
    Qt::CaseSensitivity m_caseSensitivity;
    class ReplacementStream;
};

#endif
