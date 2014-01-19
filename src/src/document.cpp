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

using namespace KTextEditor;

Document::Document(DocumentPrivate *impl, QObject *parent)
    : KParts::ReadWritePart(parent)
    , d (impl)
{
}

Document::~Document()
{
}

bool Document::openingError() const
{
    return false; // FIXME KF 5; d->openingError;
}

QString Document::openingErrorMessage() const
{
    return QString (); //FIXME KF5 d->openingErrorMessage;
}

void Document::setOpeningError(bool errors)
{
    // FIXME KF 5; d->openingError = errors;
}

void Document::setOpeningErrorMessage(const QString &message)
{
    // FIXME KF 5; d->openingErrorMessage = message;
}

bool Document::cursorInText(const Cursor &cursor)
{
    if ((cursor.line() < 0) || (cursor.line() >= lines())) {
        return false;
    }
    return (cursor.column() >= 0) && (cursor.column() <= lineLength(cursor.line())); // = because new line isn't usually contained in line length
}

bool KTextEditor::Document::replaceText(const Range &range, const QString &text, bool block)
{
    bool success = true;
    startEditing();
    success &= removeText(range, block);
    success &= insertText(range.start(), text, block);
    endEditing();
    return success;
}

bool Document::replaceText(const Range &range, const QStringList &text, bool block)
{
    bool success = true;
    startEditing();
    success &= removeText(range, block);
    success &= insertText(range.start(), text, block);
    endEditing();
    return success;
}

bool Document::isEmpty() const
{
    return documentEnd() == Cursor::start();
}

