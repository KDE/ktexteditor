/*
    SPDX-FileCopyrightText: 2010 Christoph Cullmann <cullmann@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "katetextblock.h"
#include "katetextbuffer.h"
#include "katetextcursor.h"
#include "katetextrange.h"

namespace Kate
{
TextBlock::TextBlock(TextBuffer *buffer, int index)
    : m_buffer(buffer)
    , m_blockIndex(index)
{
    // reserve the block size
    m_lines.reserve(BufferBlockSize);
}

TextBlock::~TextBlock()
{
    // blocks should be empty before they are deleted!
    Q_ASSERT(m_blockSize == 0);
    Q_ASSERT(m_lines.empty());
    Q_ASSERT(m_cursors.empty());

    // it only is a hint for ranges for this block, not the storage of them
}

int TextBlock::startLine() const
{
    return m_buffer->m_startLines[m_blockIndex];
}

TextLine TextBlock::line(int line) const
{
    // right input
    Q_ASSERT(size_t(line) < m_lines.size());
    // get text line, at will bail out on out-of-range
    return m_lines.at(line);
}

void TextBlock::setLineMetaData(int line, const TextLine &textLine)
{
    // right input
    Q_ASSERT(size_t(line) < m_lines.size());

    // set stuff, at will bail out on out-of-range
    const QString originalText = m_lines.at(line).text();
    m_lines.at(line) = textLine;
    m_lines.at(line).text() = originalText;
}

void TextBlock::appendLine(const QString &textOfLine)
{
    m_lines.emplace_back(textOfLine);
    m_blockSize += textOfLine.size();
}

void TextBlock::clearLines()
{
    m_lines.clear();
    m_blockSize = 0;
}

void TextBlock::text(QString &text) const
{
    // combine all lines
    for (const auto &line : m_lines) {
        text.append(line.text());
        text.append(QLatin1Char('\n'));
    }
}

void TextBlock::wrapLine(const KTextEditor::Cursor position, int fixStartLinesStartIndex)
{
    // calc internal line
    const int line = position.line() - startLine();

    // get text, copy, we might invalidate the reference
    const QString text = m_lines.at(line).text();

    // check if valid column
    Q_ASSERT(position.column() >= 0);
    Q_ASSERT(position.column() <= text.size());
    Q_ASSERT(fixStartLinesStartIndex == m_blockIndex);

    // create new line and insert it
    m_lines.insert(m_lines.begin() + line + 1, TextLine());

    // cases for modification:
    // 1. line is wrapped in the middle
    // 2. if empty line is wrapped, mark new line as modified
    // 3. line-to-be-wrapped is already modified
    if (position.column() > 0 || text.size() == 0 || m_lines.at(line).markedAsModified()) {
        m_lines.at(line + 1).markAsModified(true);
    } else if (m_lines.at(line).markedAsSavedOnDisk()) {
        m_lines.at(line + 1).markAsSavedOnDisk(true);
    }

    // perhaps remove some text from previous line and append it
    if (position.column() < text.size()) {
        // text from old line moved first to new one
        m_lines.at(line + 1).text() = text.right(text.size() - position.column());

        // now remove wrapped text from old line
        m_lines.at(line).text().chop(text.size() - position.column());

        // mark line as modified
        m_lines.at(line).markAsModified(true);
    }

    // fix all start lines
    // we need to do this NOW, else the range update will FAIL!
    // bug 313759
    m_buffer->fixStartLines(fixStartLinesStartIndex + 1, 1);

    // notify the text history
    m_buffer->history().wrapLine(position);

    // cursor and range handling below

    // no cursors will leave or join this block

    // no cursors in this block, no work to do..
    if (m_cursors.empty()) {
        return;
    }

    // move all cursors on the line which has the text inserted
    // remember all ranges modified, optimize for the standard case of a few ranges
    QVarLengthArray<TextRange *, 32> changedRanges;
    for (TextCursor *cursor : m_cursors) {
        // skip cursors on lines in front of the wrapped one!
        if (cursor->lineInBlock() < line) {
            continue;
        }

        // either this is simple, line behind the wrapped one
        if (cursor->lineInBlock() > line) {
            // patch line of cursor
            cursor->m_line++;
        }

        // this is the wrapped line
        else {
            // skip cursors with too small column
            if (cursor->column() <= position.column()) {
                if (cursor->column() < position.column() || !cursor->m_moveOnInsert) {
                    continue;
                }
            }

            // move cursor

            // patch line of cursor
            cursor->m_line++;

            // patch column
            cursor->m_column -= position.column();
        }

        // remember range, if any, avoid double insert
        auto range = cursor->kateRange();
        if (range && !range->isValidityCheckRequired()) {
            range->setValidityCheckRequired();
            changedRanges.push_back(range);
        }
    }

    // we might need to invalidate ranges or notify about their changes
    // checkValidity might trigger delete of the range!
    for (TextRange *range : std::as_const(changedRanges)) {
        // ensure that we really invalidate bad ranges!
        range->checkValidity();
    }
}

void TextBlock::unwrapLine(int line, TextBlock *previousBlock, int fixStartLinesStartIndex)
{
    // two possiblities: either first line of this block or later line
    if (line == 0) {
        // we need previous block with at least one line
        Q_ASSERT(previousBlock);
        Q_ASSERT(previousBlock->lines() > 0);

        // move last line of previous block to this one, might result in empty block
        const TextLine oldFirst = m_lines.at(0);
        const int lastLineOfPreviousBlock = previousBlock->lines() - 1;
        m_lines[0] = previousBlock->m_lines.back();
        previousBlock->m_lines.erase(previousBlock->m_lines.begin() + (previousBlock->lines() - 1));

        const int oldSizeOfPreviousLine = m_lines[0].text().size();
        if (oldFirst.length() > 0) {
            // append text
            m_lines[0].text().append(oldFirst.text());

            // mark line as modified, since text was appended
            m_lines[0].markAsModified(true);
        }

        // fix all start lines
        // we need to do this NOW, else the range update will FAIL!
        // bug 313759
        Q_ASSERT(fixStartLinesStartIndex + 1 == m_blockIndex);
        m_buffer->fixStartLines(fixStartLinesStartIndex + 1, -1);

        // notify the text history in advance
        m_buffer->history().unwrapLine(startLine() + line, oldSizeOfPreviousLine);

        // cursor and range handling below

        // no cursors in this block and the previous one, no work to do..
        if (m_cursors.empty() && previousBlock->m_cursors.empty()) {
            return;
        }

        // move all cursors because of the unwrapped line
        // remember all ranges modified, optimize for the standard case of a few ranges
        QVarLengthArray<TextRange *, 32> changedRanges;
        for (TextCursor *cursor : m_cursors) {
            // this is the unwrapped line
            if (cursor->lineInBlock() == 0) {
                // patch column
                cursor->m_column += oldSizeOfPreviousLine;

                // remember range, if any, avoid double insert
                auto range = cursor->kateRange();
                if (range && !range->isValidityCheckRequired()) {
                    range->setValidityCheckRequired();
                    changedRanges.push_back(range);
                }
            }
        }

        // move cursors of the moved line from previous block to this block now
        for (auto it = previousBlock->m_cursors.begin(); it != previousBlock->m_cursors.end();) {
            auto cursor = *it;
            if (cursor->lineInBlock() == lastLineOfPreviousBlock) {
                cursor->m_line = 0;
                cursor->m_block = this;
                m_cursors.insert(cursor);

                // remember range, if any, avoid double insert
                auto range = cursor->kateRange();
                if (range && !range->isValidityCheckRequired()) {
                    range->setValidityCheckRequired();
                    changedRanges.push_back(range);
                }

                // remove from previous block
                it = previousBlock->m_cursors.erase(it);
            } else {
                // keep in previous block
                ++it;
            }
        }

        // fixup the ranges that might be effected, because they moved from last line to this block
        // we might need to invalidate ranges or notify about their changes
        // checkValidity might trigger delete of the range!
        for (TextRange *range : std::as_const(changedRanges)) {
            // afterwards check validity, might delete this range!
            range->checkValidity();
        }

        // be done
        return;
    }

    // easy: just move text to previous line and remove current one
    const int oldSizeOfPreviousLine = m_lines.at(line - 1).length();
    const int sizeOfCurrentLine = m_lines.at(line).length();
    if (sizeOfCurrentLine > 0) {
        m_lines.at(line - 1).text().append(m_lines.at(line).text());
    }

    const bool lineChanged = (oldSizeOfPreviousLine > 0 && m_lines.at(line - 1).markedAsModified())
        || (sizeOfCurrentLine > 0 && (oldSizeOfPreviousLine > 0 || m_lines.at(line).markedAsModified()));
    m_lines.at(line - 1).markAsModified(lineChanged);
    if (oldSizeOfPreviousLine == 0 && m_lines.at(line).markedAsSavedOnDisk()) {
        m_lines.at(line - 1).markAsSavedOnDisk(true);
    }

    m_lines.erase(m_lines.begin() + line);

    // fix all start lines
    // we need to do this NOW, else the range update will FAIL!
    // bug 313759
    m_buffer->fixStartLines(fixStartLinesStartIndex + 1, -1);

    // notify the text history in advance
    m_buffer->history().unwrapLine(startLine() + line, oldSizeOfPreviousLine);

    // cursor and range handling below

    // no cursors in this block, no work to do..
    if (m_cursors.empty()) {
        return;
    }

    // move all cursors because of the unwrapped line
    // remember all ranges modified, optimize for the standard case of a few ranges
    QVarLengthArray<TextRange *, 32> changedRanges;
    for (TextCursor *cursor : m_cursors) {
        // skip cursors in lines in front of removed one
        if (cursor->lineInBlock() < line) {
            continue;
        }

        // this is the unwrapped line
        if (cursor->lineInBlock() == line) {
            // patch column
            cursor->m_column += oldSizeOfPreviousLine;
        }

        // patch line of cursor
        cursor->m_line--;

        // remember range, if any, avoid double insert
        auto range = cursor->kateRange();
        if (range && !range->isValidityCheckRequired()) {
            range->setValidityCheckRequired();
            changedRanges.push_back(range);
        }
    }

    // we might need to invalidate ranges or notify about their changes
    // checkValidity might trigger delete of the range!
    for (TextRange *range : std::as_const(changedRanges)) {
        // ensure that we really invalidate bad ranges!
        range->checkValidity();
    }
}

void TextBlock::insertText(const KTextEditor::Cursor position, const QString &text)
{
    // calc internal line
    int line = position.line() - startLine();

    // get text
    QString &textOfLine = m_lines.at(line).text();
    int oldLength = textOfLine.size();
    m_lines.at(line).markAsModified(true);

    // check if valid column
    Q_ASSERT(position.column() >= 0);
    Q_ASSERT(position.column() <= textOfLine.size());

    // insert text
    textOfLine.insert(position.column(), text);

    // notify the text history
    m_buffer->history().insertText(position, text.size(), oldLength);

    m_blockSize += text.size();

    // cursor and range handling below

    // no cursors in this block, no work to do..
    if (m_cursors.empty()) {
        return;
    }

    // move all cursors on the line which has the text inserted
    // remember all ranges modified, optimize for the standard case of a few ranges
    QVarLengthArray<TextRange *, 32> changedRanges;
    for (TextCursor *cursor : m_cursors) {
        // skip cursors not on this line!
        if (cursor->lineInBlock() != line) {
            continue;
        }

        // skip cursors with too small column
        if (cursor->column() <= position.column()) {
            if (cursor->column() < position.column() || !cursor->m_moveOnInsert) {
                continue;
            }
        }

        // patch column of cursor
        if (cursor->m_column <= oldLength) {
            cursor->m_column += text.size();
        }

        // special handling if cursor behind the real line, e.g. non-wrapping cursor in block selection mode
        else if (cursor->m_column < textOfLine.size()) {
            cursor->m_column = textOfLine.size();
        }

        // remember range, if any, avoid double insert
        // we only need to trigger checkValidity later if the range has feedback or might be invalidated
        auto range = cursor->kateRange();
        if (range && !range->isValidityCheckRequired() && (range->feedback() || range->start().line() == range->end().line())) {
            range->setValidityCheckRequired();
            changedRanges.push_back(range);
        }
    }

    // we might need to invalidate ranges or notify about their changes
    // checkValidity might trigger delete of the range!
    for (TextRange *range : std::as_const(changedRanges)) {
        range->checkValidity();
    }
}

void TextBlock::removeText(KTextEditor::Range range, QString &removedText)
{
    // calc internal line
    int line = range.start().line() - startLine();

    // get text
    QString &textOfLine = m_lines.at(line).text();
    int oldLength = textOfLine.size();

    // check if valid column
    Q_ASSERT(range.start().column() >= 0);
    Q_ASSERT(range.start().column() <= textOfLine.size());
    Q_ASSERT(range.end().column() >= 0);
    Q_ASSERT(range.end().column() <= textOfLine.size());

    // get text which will be removed
    removedText = textOfLine.mid(range.start().column(), range.end().column() - range.start().column());

    // remove text
    textOfLine.remove(range.start().column(), range.end().column() - range.start().column());
    m_lines.at(line).markAsModified(true);

    // notify the text history
    m_buffer->history().removeText(range, oldLength);

    m_blockSize -= removedText.size();

    // cursor and range handling below

    // no cursors in this block, no work to do..
    if (m_cursors.empty()) {
        return;
    }

    // move all cursors on the line which has the text removed
    // remember all ranges modified, optimize for the standard case of a few ranges
    QVarLengthArray<TextRange *, 32> changedRanges;
    for (TextCursor *cursor : m_cursors) {
        // skip cursors not on this line!
        if (cursor->lineInBlock() != line) {
            continue;
        }

        // skip cursors with too small column
        if (cursor->column() <= range.start().column()) {
            continue;
        }

        // patch column of cursor
        if (cursor->column() <= range.end().column()) {
            cursor->m_column = range.start().column();
        } else {
            cursor->m_column -= (range.end().column() - range.start().column());
        }

        // remember range, if any, avoid double insert
        // we only need to trigger checkValidity later if the range has feedback or might be invalidated
        auto range = cursor->kateRange();
        if (range && !range->isValidityCheckRequired() && (range->feedback() || range->start().line() == range->end().line())) {
            range->setValidityCheckRequired();
            changedRanges.push_back(range);
        }
    }

    // we might need to invalidate ranges or notify about their changes
    // checkValidity might trigger delete of the range!
    for (TextRange *range : std::as_const(changedRanges)) {
        range->checkValidity();
    }
}

void TextBlock::debugPrint(int blockIndex) const
{
    // print all blocks
    for (size_t i = 0; i < m_lines.size(); ++i) {
        printf("%4d - %4llu : %4llu : '%s'\n",
               blockIndex,
               (unsigned long long)startLine() + i,
               (unsigned long long)m_lines.at(i).text().size(),
               qPrintable(m_lines.at(i).text()));
    }
}

void TextBlock::splitBlock(int fromLine, TextBlock *newBlock)
{
    // half the block
    const int linesOfNewBlock = lines() - fromLine;

    // move lines
    newBlock->m_lines.reserve(linesOfNewBlock);
    for (size_t i = fromLine; i < m_lines.size(); ++i) {
        auto line = std::move(m_lines[i]);
        m_blockSize -= line.length();
        newBlock->m_blockSize += line.length();
        newBlock->m_lines.push_back(std::move(line));
    }

    m_lines.resize(fromLine);

    // move cursors
    QSet<Kate::TextRange *> ranges;
    for (auto it = m_cursors.begin(); it != m_cursors.end();) {
        auto cursor = *it;
        if (cursor->lineInBlock() >= fromLine) {
            cursor->m_line = cursor->lineInBlock() - fromLine;
            cursor->m_block = newBlock;

            // add to new, remove from current
            newBlock->m_cursors.insert(cursor);
            it = m_cursors.erase(it);
            if (cursor->kateRange()) {
                ranges.insert(cursor->kateRange());
            }
        } else {
            // keep in current
            ++it;
        }
    }

    for (auto range : std::as_const(ranges)) {
        if (range->spansMultipleBlocks()) {
            m_buffer->addMultilineRange(range);
        }
    }
}

void TextBlock::mergeBlock(TextBlock *targetBlock)
{
    // move cursors, do this first, now still lines() count is correct for target
    QSet<Kate::TextRange *> ranges;
    for (TextCursor *cursor : m_cursors) {
        cursor->m_line = cursor->lineInBlock() + targetBlock->lines();
        cursor->m_block = targetBlock;
        targetBlock->m_cursors.insert(cursor);
        if (cursor->kateRange()) {
            ranges.insert(cursor->kateRange());
        }
    }
    m_cursors.clear();

    for (auto range : std::as_const(ranges)) {
        if (!range->spansMultipleBlocks()) {
            m_buffer->removeMultilineRange(range);
        }
    }

    // move lines
    targetBlock->m_lines.reserve(targetBlock->lines() + lines());
    for (size_t i = 0; i < m_lines.size(); ++i) {
        targetBlock->m_lines.push_back(m_lines.at(i));
    }
    targetBlock->m_blockSize += m_blockSize;
    clearLines();
}

void TextBlock::deleteBlockContent()
{
    // kill cursors, if not belonging to a range
    // we can do in-place editing of the current set of cursors as
    // we remove them before deleting
    for (auto it = m_cursors.begin(); it != m_cursors.end();) {
        auto cursor = *it;
        if (!cursor->kateRange()) {
            // remove it and advance to next element
            it = m_cursors.erase(it);

            // delete after cursor is gone from the set
            // else the destructor will modify it!
            delete cursor;
        } else {
            // keep this cursor
            ++it;
        }
    }

    // kill lines
    clearLines();
}

void TextBlock::clearBlockContent(TextBlock *targetBlock)
{
    // move cursors, if not belonging to a range
    // we can do in-place editing of the current set of cursors
    for (auto it = m_cursors.begin(); it != m_cursors.end();) {
        auto cursor = *it;
        if (!cursor->kateRange()) {
            cursor->m_column = 0;
            cursor->m_line = 0;
            cursor->m_block = targetBlock;
            targetBlock->m_cursors.insert(cursor);

            // remove it and advance to next element
            it = m_cursors.erase(it);
        } else {
            // keep this cursor
            ++it;
        }
    }

    // kill lines
    clearLines();
}

QList<TextRange *> TextBlock::rangesForLine(int line, KTextEditor::View *view, bool rangesWithAttributeOnly) const
{
    QList<TextRange *> ranges;
    rangesForLine(line, view, rangesWithAttributeOnly, ranges);
    return ranges;
}

void TextBlock::rangesForLine(const int line, KTextEditor::View *view, bool rangesWithAttributeOnly, QList<TextRange *> &outRanges) const
{
    outRanges.clear();
    const int lineInBlock = line - startLine(); // line number in block
    for (auto cursor : std::as_const(m_cursors)) {
        if (!cursor->kateRange()) {
            continue;
        }
        auto range = cursor->kateRange();
        if (rangesWithAttributeOnly && !range->hasAttribute()) {
            continue;
        }

        // we want ranges for no view, but this one's attribute is only valid for views
        if (!view && range->attributeOnlyForViews()) {
            continue;
        }

        // the range's attribute is not valid for this view
        if (range->view() && range->view() != view) {
            continue;
        }

        // simple case
        if (cursor->lineInBlock() == lineInBlock) {
            outRanges.append(range);
        }
        // if line is in the range, ok
        else if (range->startInternal().lineInternal() <= line && line <= range->endInternal().lineInternal()) {
            outRanges.append(range);
        }
    }
    std::sort(outRanges.begin(), outRanges.end());
    auto it = std::unique(outRanges.begin(), outRanges.end());
    outRanges.erase(it, outRanges.end());
}

void TextBlock::markModifiedLinesAsSaved()
{
    // mark all modified lines as saved
    for (auto &textLine : m_lines) {
        if (textLine.markedAsModified()) {
            textLine.markAsSavedOnDisk(true);
        }
    }
}
}
