// This file is part of the KDE libraries
// Copyright (C) 2008 Paul Giannaros <paul@giannaros.org>
// Copyright (C) 2009-2018 Dominik Haumann <dhaumann@kde.org>
//
// This library is free software; you can redistribute it and/or
// modify it under the terms of the GNU Library General Public
// License as published by the Free Software Foundation; either
// version 2 of the License, or (at your option) version 3.
//
// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// Library General Public License for more details.
//
// You should have received a copy of the GNU Library General Public License
// along with this library; see the file COPYING.LIB.  If not, write to
// the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
// Boston, MA 02110-1301, USA.

#include "katescriptdocument.h"

#include "katedocument.h"
#include "kateview.h"
#include "katerenderer.h"
#include "kateconfig.h"
#include "katehighlight.h"
#include "katescript.h"
#include "katepartdebug.h"
#include "scriptcursor.h"
#include "scriptrange.h"

#include <ktexteditor/documentcursor.h>
#include <QJSEngine>

KateScriptDocument::KateScriptDocument(QJSEngine *engine, QObject *parent)
    : QObject(parent), m_document(nullptr), m_engine(engine)
{
}

void KateScriptDocument::setDocument(KTextEditor::DocumentPrivate *document)
{
    m_document = document;
}

KTextEditor::DocumentPrivate *KateScriptDocument::document()
{
    return m_document;
}

int KateScriptDocument::defStyleNum(int line, int column)
{
    return m_document->defStyleNum(line, column);
}

int KateScriptDocument::defStyleNum(const QJSValue &jscursor)
{
    const auto cursor = cursorFromScriptValue(jscursor);
    return defStyleNum(cursor.line(), cursor.column());
}

bool KateScriptDocument::isCode(int line, int column)
{
    const int defaultStyle = defStyleNum(line, column);
    return _isCode(defaultStyle);
}

bool KateScriptDocument::isCode(const QJSValue &jscursor)
{
    const auto cursor = cursorFromScriptValue(jscursor);
    return isCode(cursor.line(), cursor.column());
}

bool KateScriptDocument::isComment(int line, int column)
{
    return m_document->isComment(line, column);
}

bool KateScriptDocument::isComment(const QJSValue &jscursor)
{
    const auto cursor = cursorFromScriptValue(jscursor);
    return isComment(cursor.line(), cursor.column());
}

bool KateScriptDocument::isString(int line, int column)
{
    const int defaultStyle = defStyleNum(line, column);
    return defaultStyle == KTextEditor::dsString;
}

bool KateScriptDocument::isString(const QJSValue &jscursor)
{
    const auto cursor = cursorFromScriptValue(jscursor);
    return isString(cursor.line(), cursor.column());
}

bool KateScriptDocument::isRegionMarker(int line, int column)
{
    const int defaultStyle = defStyleNum(line, column);
    return defaultStyle == KTextEditor::dsRegionMarker;
}

bool KateScriptDocument::isRegionMarker(const QJSValue &jscursor)
{
    const auto cursor = cursorFromScriptValue(jscursor);
    return isRegionMarker(cursor.line(), cursor.column());
}

bool KateScriptDocument::isChar(int line, int column)
{
    const int defaultStyle = defStyleNum(line, column);
    return defaultStyle == KTextEditor::dsChar;
}

bool KateScriptDocument::isChar(const QJSValue &jscursor)
{
    const auto cursor = cursorFromScriptValue(jscursor);
    return isChar(cursor.line(), cursor.column());
}

bool KateScriptDocument::isOthers(int line, int column)
{
    const int defaultStyle = defStyleNum(line, column);
    return defaultStyle == KTextEditor::dsOthers;
}

bool KateScriptDocument::isOthers(const QJSValue &jscursor)
{
    const auto cursor = cursorFromScriptValue(jscursor);
    return isOthers(cursor.line(), cursor.column());
}

int KateScriptDocument::firstVirtualColumn(int line)
{
    const int tabWidth = m_document->config()->tabWidth();
    Kate::TextLine textLine = m_document->plainKateTextLine(line);
    const int firstPos = textLine ? textLine->firstChar() : -1;
    if (!textLine || firstPos == -1) {
        return -1;
    }
    return textLine->indentDepth(tabWidth);
}

int KateScriptDocument::lastVirtualColumn(int line)
{
    const int tabWidth = m_document->config()->tabWidth();
    Kate::TextLine textLine = m_document->plainKateTextLine(line);
    const int lastPos = textLine ? textLine->lastChar() : -1;
    if (!textLine || lastPos == -1) {
        return -1;
    }
    return textLine->toVirtualColumn(lastPos, tabWidth);
}

int KateScriptDocument::toVirtualColumn(int line, int column)
{
    const int tabWidth = m_document->config()->tabWidth();
    Kate::TextLine textLine = m_document->plainKateTextLine(line);
    if (!textLine || column < 0 || column > textLine->length()) {
        return -1;
    }
    return textLine->toVirtualColumn(column, tabWidth);
}

int KateScriptDocument::toVirtualColumn(const QJSValue &jscursor)
{
    const auto cursor = cursorFromScriptValue(jscursor);
    return toVirtualColumn(cursor.line(), cursor.column());
}

QJSValue KateScriptDocument::toVirtualCursor(int line, int column)
{
    const KTextEditor::Cursor cursor(line, toVirtualColumn(line, column));
    return cursorToScriptValue(m_engine, cursor);
}

QJSValue KateScriptDocument::toVirtualCursor(const QJSValue &jscursor)
{
    const auto cursor = cursorFromScriptValue(jscursor);
    return toVirtualCursor(cursor.line(), cursor.column());
}

int KateScriptDocument::fromVirtualColumn(int line, int virtualColumn)
{
    const int tabWidth = m_document->config()->tabWidth();
    Kate::TextLine textLine = m_document->plainKateTextLine(line);
    if (!textLine || virtualColumn < 0 || virtualColumn > textLine->virtualLength(tabWidth)) {
        return -1;
    }
    return textLine->fromVirtualColumn(virtualColumn, tabWidth);
}

int KateScriptDocument::fromVirtualColumn(const QJSValue &jscursor)
{
    const auto cursor = cursorFromScriptValue(jscursor);
    return fromVirtualColumn(cursor.line(), cursor.column());
}

QJSValue KateScriptDocument::fromVirtualCursor(int line, int column)
{
    const KTextEditor::Cursor cursor(line, fromVirtualColumn(line, column));
    return cursorToScriptValue(m_engine, cursor);
}

QJSValue KateScriptDocument::fromVirtualCursor(const QJSValue &jscursor)
{
    const auto cursor = cursorFromScriptValue(jscursor);
    return fromVirtualCursor(cursor.line(), cursor.column());
}

KTextEditor::Cursor KateScriptDocument::rfindInternal(int line, int column, const QString &text, int attribute)
{
    KTextEditor::DocumentCursor cursor(document(), line, column);
    const int start = cursor.line();

    do {
        Kate::TextLine textLine = m_document->plainKateTextLine(cursor.line());
        if (!textLine) {
            break;
        }

        if (cursor.line() != start) {
            cursor.setColumn(textLine->length());
        } else if (column >= textLine->length()) {
            cursor.setColumn(qMax(textLine->length(), 0));
        }

        int foundAt;
        while ((foundAt = textLine->string().leftRef(cursor.column()).lastIndexOf(text, -1, Qt::CaseSensitive)) >= 0) {
            bool hasStyle = true;
            if (attribute != -1) {
                const KTextEditor::DefaultStyle ds = m_document->highlight()->defaultStyleForAttribute(textLine->attribute(foundAt));
                hasStyle = (ds == attribute);
            }

            if (hasStyle) {
                return KTextEditor::Cursor(cursor.line(), foundAt);
            } else {
                cursor.setColumn(foundAt);
            }
        }
    } while (cursor.gotoPreviousLine());

    return KTextEditor::Cursor::invalid();
}

KTextEditor::Cursor KateScriptDocument::rfind(const KTextEditor::Cursor &cursor, const QString &text, int attribute)
{
    return rfindInternal(cursor.line(), cursor.column(), text, attribute);
}

QJSValue KateScriptDocument::rfind(int line, int column, const QString &text, int attribute)
{
    return cursorToScriptValue(m_engine, rfindInternal(line, column, text, attribute));
}

QJSValue KateScriptDocument::rfind(const QJSValue &jscursor, const QString &text, int attribute)
{
    KTextEditor::Cursor cursor = cursorFromScriptValue(jscursor);
    return cursorToScriptValue(m_engine, rfind(cursor, text, attribute));
}

KTextEditor::Cursor KateScriptDocument::anchorInternal(int line, int column, QChar character)
{
    QChar lc;
    QChar rc;
    if (character == QLatin1Char('(') || character == QLatin1Char(')')) {
        lc = QLatin1Char('(');
        rc = QLatin1Char(')');
    } else if (character == QLatin1Char('{') || character == QLatin1Char('}')) {
        lc = QLatin1Char('{');
        rc = QLatin1Char('}');
    } else if (character == QLatin1Char('[') || character == QLatin1Char(']')) {
        lc = QLatin1Char('[');
        rc = QLatin1Char(']');
    } else {
        qCDebug(LOG_KTE) << "invalid anchor character:" << character << " allowed are: (){}[]";
        return KTextEditor::Cursor::invalid();
    }

    // cache line
    Kate::TextLine currentLine = document()->plainKateTextLine(line);
    if (!currentLine)
        return KTextEditor::Cursor::invalid();

    // Move backwards char by char and find the opening character
    int count = 1;
    KTextEditor::DocumentCursor cursor(document(), KTextEditor::Cursor(line, column));
    while (cursor.move(-1, KTextEditor::DocumentCursor::Wrap)) {
        // need to fetch new line?
        if (line != cursor.line()) {
            line = cursor.line();
            currentLine = document()->plainKateTextLine(line);
            if (!currentLine)
                return KTextEditor::Cursor::invalid();
        }

        // get current char
        const QChar ch = currentLine->at(cursor.column());
        if (ch == lc) {
            const KTextEditor::DefaultStyle ds = m_document->highlight()->defaultStyleForAttribute(currentLine->attribute(cursor.column()));
            if (_isCode(ds)) {
                --count;
            }
        } else if (ch == rc) {
            const KTextEditor::DefaultStyle ds = m_document->highlight()->defaultStyleForAttribute(currentLine->attribute(cursor.column()));
            if (_isCode(ds)) {
                ++count;
            }
        }

        if (count == 0) {
            return cursor.toCursor();
        }
    }
    return KTextEditor::Cursor::invalid();
}

KTextEditor::Cursor KateScriptDocument::anchor(const KTextEditor::Cursor &cursor, QChar character)
{
    return anchorInternal(cursor.line(), cursor.column(), character);
}

QJSValue KateScriptDocument::anchor(int line, int column, QChar character)
{
    return cursorToScriptValue(m_engine, anchorInternal(line, column, character));
}

QJSValue KateScriptDocument::anchor(const QJSValue &jscursor, QChar character)
{
    KTextEditor::Cursor cursor = cursorFromScriptValue(jscursor);
    return anchor(cursor.line(), cursor.column(), character);
}

bool KateScriptDocument::startsWith(int line, const QString &pattern, bool skipWhiteSpaces)
{
    Kate::TextLine textLine = m_document->plainKateTextLine(line);

    if (!textLine) {
        return false;
    }

    if (skipWhiteSpaces) {
        return textLine->matchesAt(textLine->firstChar(), pattern);
    }

    return textLine->startsWith(pattern);
}

bool KateScriptDocument::endsWith(int line, const QString &pattern, bool skipWhiteSpaces)
{
    Kate::TextLine textLine = m_document->plainKateTextLine(line);

    if (!textLine) {
        return false;
    }

    if (skipWhiteSpaces) {
        return textLine->matchesAt(textLine->lastChar() - pattern.length() + 1, pattern);
    }

    return textLine->endsWith(pattern);
}

QString KateScriptDocument::fileName()
{
    return m_document->documentName();
}

QString KateScriptDocument::url()
{
    return m_document->url().toString();
}

QString KateScriptDocument::mimeType()
{
    return m_document->mimeType();
}

QString KateScriptDocument::encoding()
{
    return m_document->encoding();
}

QString KateScriptDocument::highlightingMode()
{
    return m_document->highlightingMode();
}

QStringList KateScriptDocument::embeddedHighlightingModes()
{
    return m_document->embeddedHighlightingModes();
}

QString KateScriptDocument::highlightingModeAt(const QJSValue &jspos)
{
    return m_document->highlightingModeAt(cursorFromScriptValue(jspos));
}

bool KateScriptDocument::isModified()
{
    return m_document->isModified();
}

QString KateScriptDocument::text()
{
    return m_document->text();
}

QString KateScriptDocument::text(int fromLine, int fromColumn, int toLine, int toColumn)
{
    const KTextEditor::Range range(fromLine, fromColumn, toLine, toColumn);
    return m_document->text(range);
}

QString KateScriptDocument::text(const QJSValue &jsfrom, const QJSValue &jsto)
{
    const KTextEditor::Cursor from = cursorFromScriptValue(jsfrom);
    const KTextEditor::Cursor to = cursorFromScriptValue(jsto);
    return text(from.line(), from.column(), to.line(), to.column());
}

QString KateScriptDocument::text(const QJSValue &jsrange)
{
    const auto range = rangeFromScriptValue(jsrange);
    return text(range.start().line(), range.start().column(), range.end().line(), range.end().column());
}

QString KateScriptDocument::line(int line)
{
    return m_document->line(line);
}

QString KateScriptDocument::wordAt(int line, int column)
{
    const KTextEditor::Cursor cursor(line, column);
    return m_document->wordAt(cursor);
}

QString KateScriptDocument::wordAt(const QJSValue &jscursor)
{
    const auto cursor = cursorFromScriptValue(jscursor);
    return wordAt(cursor.line(), cursor.column());
}

QJSValue KateScriptDocument::wordRangeAt(int line, int column)
{
    const KTextEditor::Cursor cursor(line, column);
    return rangeToScriptValue(m_engine, m_document->wordRangeAt(cursor));
}

QJSValue KateScriptDocument::wordRangeAt(const QJSValue &jscursor)
{
    const auto cursor = cursorFromScriptValue(jscursor);
    return wordRangeAt(cursor.line(), cursor.column());
}

QString KateScriptDocument::charAt(int line, int column)
{
    const KTextEditor::Cursor cursor(line, column);
    const QChar c = m_document->characterAt(cursor);
    return c.isNull() ? QString() : QString(c);
}

QString KateScriptDocument::charAt(const QJSValue &jscursor)
{
    const auto cursor = cursorFromScriptValue(jscursor);
    return charAt(cursor.line(), cursor.column());
}

QString KateScriptDocument::firstChar(int line)
{
    Kate::TextLine textLine = m_document->plainKateTextLine(line);
    if (!textLine) {
        return QString();
    }
    // check for isNull(), as the returned character then would be "\0"
    const QChar c = textLine->at(textLine->firstChar());
    return c.isNull() ? QString() : QString(c);
}

QString KateScriptDocument::lastChar(int line)
{
    Kate::TextLine textLine = m_document->plainKateTextLine(line);
    if (!textLine) {
        return QString();
    }
    // check for isNull(), as the returned character then would be "\0"
    const QChar c = textLine->at(textLine->lastChar());
    return c.isNull() ? QString() : QString(c);
}

bool KateScriptDocument::isSpace(int line, int column)
{
    const KTextEditor::Cursor cursor(line, column);
    return m_document->characterAt(cursor).isSpace();
}

bool KateScriptDocument::isSpace(const QJSValue &jscursor)
{
    const auto cursor = cursorFromScriptValue(jscursor);
    return isSpace(cursor.line(), cursor.column());
}

bool KateScriptDocument::matchesAt(int line, int column, const QString &s)
{
    Kate::TextLine textLine = m_document->plainKateTextLine(line);
    return textLine ? textLine->matchesAt(column, s) : false;
}

bool KateScriptDocument::matchesAt(const QJSValue &jscursor, const QString &s)
{
    const auto cursor = cursorFromScriptValue(jscursor);
    return matchesAt(cursor.line(), cursor.column(), s);
}

bool KateScriptDocument::setText(const QString &s)
{
    return m_document->setText(s);
}

bool KateScriptDocument::clear()
{
    return m_document->clear();
}

bool KateScriptDocument::truncate(int line, int column)
{
    Kate::TextLine textLine = m_document->plainKateTextLine(line);
    if (!textLine || textLine->text().size() < column) {
        return false;
    }

    return removeText(line, column, line, textLine->text().size() - column);
}

bool KateScriptDocument::truncate(const QJSValue &jscursor)
{
    const auto cursor = cursorFromScriptValue(jscursor);
    return truncate(cursor.line(), cursor.column());
}

bool KateScriptDocument::insertText(int line, int column, const QString &s)
{
    KTextEditor::Cursor cursor(line, column);
    return m_document->insertText(cursor, s);
}

bool KateScriptDocument::insertText(const QJSValue &jscursor, const QString &s)
{
    const auto cursor = cursorFromScriptValue(jscursor);
    return insertText(cursor.line(), cursor.column(), s);
}

bool KateScriptDocument::removeText(int fromLine, int fromColumn, int toLine, int toColumn)
{
    const KTextEditor::Range range(fromLine, fromColumn, toLine, toColumn);
    return m_document->removeText(range);
}

bool KateScriptDocument::removeText(const QJSValue &jsfrom, const QJSValue &jsto)
{
    const KTextEditor::Cursor from = cursorFromScriptValue(jsfrom);
    const KTextEditor::Cursor to = cursorFromScriptValue(jsto);
    return removeText(from.line(), from.column(), to.line(), to.column());
}

bool KateScriptDocument::removeText(const QJSValue &jsrange)
{
    const auto range = rangeFromScriptValue(jsrange);
    return removeText(range.start().line(), range.start().column(), range.end().line(), range.end().column());
}

bool KateScriptDocument::insertLine(int line, const QString &s)
{
    return m_document->insertLine(line, s);
}

bool KateScriptDocument::removeLine(int line)
{
    return m_document->removeLine(line);
}

bool KateScriptDocument::wrapLine(int line, int column)
{
    return m_document->editWrapLine(line, column);
}

bool KateScriptDocument::wrapLine(const QJSValue &jscursor)
{
    const auto cursor = cursorFromScriptValue(jscursor);
    return wrapLine(cursor.line(), cursor.column());
}

void KateScriptDocument::joinLines(int startLine, int endLine)
{
    m_document->joinLines(startLine, endLine);
}

int KateScriptDocument::lines()
{
    return m_document->lines();
}

bool KateScriptDocument::isLineModified(int line)
{
    return m_document->isLineModified(line);
}

bool KateScriptDocument::isLineSaved(int line)
{
    return m_document->isLineSaved(line);
}

bool KateScriptDocument::isLineTouched(int line)
{
    return m_document->isLineTouched(line);
}

int KateScriptDocument::findTouchedLine(int startLine, bool down)
{
    return m_document->findTouchedLine(startLine, down);
}

int KateScriptDocument::length()
{
    return m_document->totalCharacters();
}

int KateScriptDocument::lineLength(int line)
{
    return m_document->lineLength(line);
}

void KateScriptDocument::editBegin()
{
    m_document->editBegin();
}

void KateScriptDocument::editEnd()
{
    m_document->editEnd();
}

bool KateScriptDocument::isValidTextPosition(int line, int column)
{
    return m_document->isValidTextPosition(KTextEditor::Cursor(line, column));
}

bool KateScriptDocument::isValidTextPosition(const QJSValue& cursor)
{
    return m_document->isValidTextPosition(cursorFromScriptValue(cursor));
}

int KateScriptDocument::firstColumn(int line)
{
    Kate::TextLine textLine = m_document->plainKateTextLine(line);
    return textLine ? textLine->firstChar() : -1;
}

int KateScriptDocument::lastColumn(int line)
{
    Kate::TextLine textLine = m_document->plainKateTextLine(line);
    return textLine ? textLine->lastChar() : -1;
}

int KateScriptDocument::prevNonSpaceColumn(int line, int column)
{
    Kate::TextLine textLine = m_document->plainKateTextLine(line);
    if (!textLine) {
        return -1;
    }
    return textLine->previousNonSpaceChar(column);
}

int KateScriptDocument::prevNonSpaceColumn(const QJSValue &jscursor)
{
    const auto cursor = cursorFromScriptValue(jscursor);
    return prevNonSpaceColumn(cursor.line(), cursor.column());
}

int KateScriptDocument::nextNonSpaceColumn(int line, int column)
{
    Kate::TextLine textLine = m_document->plainKateTextLine(line);
    if (!textLine) {
        return -1;
    }
    return textLine->nextNonSpaceChar(column);
}

int KateScriptDocument::nextNonSpaceColumn(const QJSValue &jscursor)
{
    const auto cursor = cursorFromScriptValue(jscursor);
    return nextNonSpaceColumn(cursor.line(), cursor.column());
}

int KateScriptDocument::prevNonEmptyLine(int line)
{
    const int startLine = line;
    for (int currentLine = startLine; currentLine >= 0; --currentLine) {
        Kate::TextLine textLine = m_document->plainKateTextLine(currentLine);
        if (!textLine) {
            return -1;
        }
        if (textLine->firstChar() != -1) {
            return currentLine;
        }
    }
    return -1;
}

int KateScriptDocument::nextNonEmptyLine(int line)
{
    const int startLine = line;
    for (int currentLine = startLine; currentLine < m_document->lines(); ++currentLine) {
        Kate::TextLine textLine = m_document->plainKateTextLine(currentLine);
        if (!textLine) {
            return -1;
        }
        if (textLine->firstChar() != -1) {
            return currentLine;
        }
    }
    return -1;
}

bool KateScriptDocument::isInWord(const QString &character, int attribute)
{
    return m_document->highlight()->isInWord(character.at(0), attribute);
}

bool KateScriptDocument::canBreakAt(const QString &character, int attribute)
{
    return m_document->highlight()->canBreakAt(character.at(0), attribute);
}

bool KateScriptDocument::canComment(int startAttribute, int endAttribute)
{
    return m_document->highlight()->canComment(startAttribute, endAttribute);
}

QString KateScriptDocument::commentMarker(int attribute)
{
    return m_document->highlight()->getCommentSingleLineStart(attribute);
}

QString KateScriptDocument::commentStart(int attribute)
{
    return m_document->highlight()->getCommentStart(attribute);
}

QString KateScriptDocument::commentEnd(int attribute)
{
    return m_document->highlight()->getCommentEnd(attribute);
}

QJSValue KateScriptDocument::documentRange()
{
    return rangeToScriptValue(m_engine, m_document->documentRange());
}

QJSValue KateScriptDocument::documentEnd()
{
    return cursorToScriptValue(m_engine, m_document->documentEnd());
}

int KateScriptDocument::attribute(int line, int column)
{
    Kate::TextLine textLine = m_document->kateTextLine(line);
    if (!textLine) {
        return 0;
    }
    return textLine->attribute(column);
}

int KateScriptDocument::attribute(const QJSValue &jscursor)
{
    const auto cursor = cursorFromScriptValue(jscursor);
    return attribute(cursor.line(), cursor.column());
}

bool KateScriptDocument::isAttribute(int line, int column, int attr)
{
    return attr == attribute(line, column);
}

bool KateScriptDocument::isAttribute(const QJSValue &jscursor, int attr)
{
    const auto cursor = cursorFromScriptValue(jscursor);
    return isAttribute(cursor.line(), cursor.column(), attr);
}

QString KateScriptDocument::attributeName(int line, int column)
{
    return m_document->highlight()->nameForAttrib(document()->plainKateTextLine(line)->attribute(column));
}

QString KateScriptDocument::attributeName(const QJSValue &jscursor)
{
    const auto cursor = cursorFromScriptValue(jscursor);
    return attributeName(cursor.line(), cursor.column());
}

bool KateScriptDocument::isAttributeName(int line, int column, const QString &name)
{
    return name == attributeName(line, column);
}

bool KateScriptDocument::isAttributeName(const QJSValue &jscursor, const QString &name)
{
    const auto cursor = cursorFromScriptValue(jscursor);
    return isAttributeName(cursor.line(), cursor.column(), name);
}

QString KateScriptDocument::variable(const QString &s)
{
    return m_document->variable(s);
}

void KateScriptDocument::setVariable(const QString &s, const QString &v)
{
    m_document->setVariable(s, v);
}

bool KateScriptDocument::_isCode(int defaultStyle)
{
    return (defaultStyle != KTextEditor::dsComment
            && defaultStyle != KTextEditor::dsAlert
            && defaultStyle != KTextEditor::dsString
            && defaultStyle != KTextEditor::dsRegionMarker
            && defaultStyle != KTextEditor::dsChar
            && defaultStyle != KTextEditor::dsOthers);
}

void KateScriptDocument::indent(const QJSValue &jsrange, int change)
{
    const auto range = rangeFromScriptValue(jsrange);
    m_document->indent(range, change);
}
