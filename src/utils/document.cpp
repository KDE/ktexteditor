/*
    SPDX-FileCopyrightText: 2010 Bernhard Beschow <bbeschow@cs.tu-berlin.de>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "document.h"
#include "katedocument.h"

using namespace KTextEditor;

Document::Document(DocumentPrivate *impl, QObject *parent)
    : KParts::ReadWritePart(parent)
    , d(impl)
{
}

Document::~Document() = default;

namespace KTextEditor
{
/**
 * Private d-pointer type for EditingTransaction
 */
class EditingTransactionPrivate
{
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
    : d(new EditingTransactionPrivate())
{
    // Although it works in release-mode, we usually want a valid document
    Q_ASSERT(document != nullptr);

    // initialize d-pointer
    d->document = qobject_cast<KTextEditor::DocumentPrivate *>(document);
    d->transactionRunning = false;

    // start the editing transaction
    start();
}

void Document::EditingTransaction::start()
{
    if (d->document && !d->transactionRunning) {
        d->document->startEditing();
        d->transactionRunning = true;
    }
}

void Document::EditingTransaction::finish()
{
    if (d->document && d->transactionRunning) {
        d->document->finishEditing();
        d->transactionRunning = false;
    }
}

Document::EditingTransaction::~EditingTransaction()
{
    // finish the editing transaction
    finish();

    // delete our d-pointer
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
