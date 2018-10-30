/* This file is part of the KDE project
 *
 *  Copyright (C) 2010 Bernhard Beschow <bbeschow@cs.tu-berlin.de>
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

#include "document.h"
#include "katedocument.h"

using namespace KTextEditor;

Document::Document(DocumentPrivate *impl, QObject *parent)
    : KParts::ReadWritePart(parent)
    , d (impl)
{
}

Document::~Document()
{
}

namespace KTextEditor {

/**
 * Private d-pointer type for EditingTransaction
 */
class EditingTransactionPrivate {
    public:
        /**
         * real document implementation
         */
        DocumentPrivate *document;

        /**
         * Indicator for running editing transaction
         */
        bool transactionRunning;
};

}

Document::EditingTransaction::EditingTransaction(Document *document)
    : d (new EditingTransactionPrivate())
{
    // Although it works in release-mode, we usually want a valid document
    Q_ASSERT(document != nullptr);

    // initialize d-pointer
    d->document = qobject_cast<KTextEditor::DocumentPrivate *> (document);
    d->transactionRunning = false;

    // start the editing transaction
    start();
}

void Document::EditingTransaction::start()
{
    if (d->document && !d->transactionRunning) {
        d->document->startEditing ();
        d->transactionRunning = true;
    }
}

void Document::EditingTransaction::finish()
{
    if (d->document && d->transactionRunning) {
        d->document->finishEditing ();
        d->transactionRunning = false;
    }
}

Document::EditingTransaction::~EditingTransaction()
{
    /**
     * finish the editing transaction
     */
    finish();

    /**
     * delete our d-pointer
     */
    delete d;
}

bool Document::openingError() const
{
    return d->m_openingError;
}

QString Document::openingErrorMessage() const
{
    return d->m_openingErrorMessage;
}

bool KTextEditor::Document::replaceText(const Range &range, const QString &text, bool block)
{
    bool success = true;
    EditingTransaction transaction(this);
    success &= removeText(range, block);
    success &= insertText(range.start(), text, block);
    return success;
}

bool Document::replaceText(const Range &range, const QStringList &text, bool block)
{
    bool success = true;
    EditingTransaction transaction(this);
    success &= removeText(range, block);
    success &= insertText(range.start(), text, block);
    return success;
}

bool Document::isEmpty() const
{
    return documentEnd() == Cursor::start();
}

QVector<KTextEditor::Range> Document::searchText(const KTextEditor::Range &range, const QString &pattern, const SearchOptions options) const
{
    return d->searchText(range, pattern, options);
}
