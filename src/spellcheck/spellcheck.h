/*  This file is part of the KDE libraries and the Kate part.
 *
 *  Copyright (C) 2008-2009 by Michel Ludwig <michel.ludwig@kdemail.net>
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

#ifndef SPELLCHECK_H
#define SPELLCHECK_H

#include <QList>
#include <QObject>
#include <QPair>
#include <QString>

#include <ktexteditor/document.h>
#include <sonnet/backgroundchecker.h>
#include <sonnet/speller.h>

namespace KTextEditor { class DocumentPrivate; }

class KateSpellCheckManager : public QObject
{
    Q_OBJECT

    typedef QPair<KTextEditor::Range, QString> RangeDictionaryPair;

public:
    explicit KateSpellCheckManager(QObject *parent = nullptr);
    virtual ~KateSpellCheckManager();

    QStringList suggestions(const QString &word, const QString &dictionary);

    void ignoreWord(const QString &word, const QString &dictionary);
    void addToDictionary(const QString &word, const QString &dictionary);

    /**
     * 'r2' is a subrange of 'r1', which is extracted from 'r1' and the remaining ranges are returned
     **/
    static QList<KTextEditor::Range> rangeDifference(const KTextEditor::Range &r1, const KTextEditor::Range &r2);

Q_SIGNALS:
    /**
     * These signals are used to propagate the dictionary changes to the
     * BackgroundChecker instance in other components (e.g. onTheFlyChecker).
     */
    void wordAddedToDictionary(const QString &word);
    void wordIgnored(const QString &word);

public:
    QList<QPair<KTextEditor::Range, QString> > spellCheckLanguageRanges(KTextEditor::DocumentPrivate *doc, const KTextEditor::Range &range);

    QList<QPair<KTextEditor::Range, QString> > spellCheckWrtHighlightingRanges(KTextEditor::DocumentPrivate *doc, const KTextEditor::Range &range,
            const QString &dictionary = QString(),
            bool singleLine = false,
            bool returnSingleRange = false);
    QList<QPair<KTextEditor::Range, QString> > spellCheckRanges(KTextEditor::DocumentPrivate *doc, const KTextEditor::Range &range,
            bool singleLine = false);

    void replaceCharactersEncodedIfNecessary(const QString &newWord, KTextEditor::DocumentPrivate *doc, const KTextEditor::Range &replacementRange);

private:
    void trimRange(KTextEditor::DocumentPrivate *doc, KTextEditor::Range &r);
};

#endif

