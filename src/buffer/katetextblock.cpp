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
}

void TextBlock::clearLines()
{
    m_lines.clear();
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

        m_buffer->m_blockSizes[m_blockIndex - 1] -= m_lines[0].length() + 1;
        m_buffer->m_blockSizes[m_blockIndex] += m_lines[0].length();

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
        QVarLengthArray<QPair<TextRange *, bool>, 32> changedRanges;
        for (TextCursor *cursor : m_cursors) {
            // this is the unwrapped line
            if (cursor->lineInBlock() == 0) {
                // patch column
                cursor->m_column += oldSizeOfPreviousLine;

                // remember range, if any, avoid double insert
                auto range = cursor->kateRange();
                if (range && !range->isValidityCheckRequired()) {
                    range->setValidityCheckRequired();
                    changedRanges.push_back({range, range->spansMultipleBlocks()});
                }
            }
        }

        // move cursors of the moved line from previous block to this block now
        for (auto it = previousBlock->m_cursors.begin(); it != previousBlock->m_cursors.end();) {
            auto cursor = *it;
            if (cursor->lineInBlock() == lastLineOfPreviousBlock) {
                Kate::TextRange *range = cursor->kateRange();
                // get the value before changing the block
                const bool spansMultipleBlocks = range && range->spansMultipleBlocks();
                cursor->m_line = 0;
                cursor->m_block = this;
                insertCursor(cursor);

                // remember range, if any, avoid double insert
                if (range && !range->isValidityCheckRequired()) {
                    range->setValidityCheckRequired();
                    // the range might not span multiple blocks anymore
                    changedRanges.push_back({range, spansMultipleBlocks});
                }

                // mark the cursor to be removed from previous block
                *it = nullptr;
                it++;
            } else {
                // keep in previous block
                ++it;
            }
        }

        std::erase_if(previousBlock->m_cursors, [](auto *c) {
            return c == nullptr;
        });

        // fixup the ranges that might be effected, because they moved from last line to this block
        // we might need to invalidate ranges or notify about their changes
        // checkValidity might trigger delete of the range!
        for (auto [range, wasMultiblock] : changedRanges) {
            // if the range doesn't span multiple blocks anymore remove it from buffer multiline range cache
            if (!range->spansMultipleBlocks() && wasMultiblock) {
                m_buffer->removeMultilineRange(range);
            }
            // afterwards check validity, might delete this range!
            range->checkValidity();
        }

        // be done
        return;
    }

    m_buffer->m_blockSizes[m_blockIndex] -= 1;

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
    Q_ASSERT(newBlock->m_cursors.empty());
    // move lines
    auto myLinesToMoveBegin = m_lines.begin() + fromLine;
    auto myLinesToMoveEnd = m_lines.end();
    int blockSizeChange = myLinesToMoveEnd - myLinesToMoveBegin;// how many newlines
    std::for_each(myLinesToMoveBegin, myLinesToMoveEnd, [&blockSizeChange] (const TextLine &line) -> void {
        blockSizeChange += line.length();// how many non-newlines
    });
    m_buffer->m_blockSizes[m_blockIndex] -= blockSizeChange;
    m_buffer->m_blockSizes[newBlock->m_blockIndex] += blockSizeChange;
    newBlock->m_lines.insert(newBlock->m_lines.cend(), std::make_move_iterator(myLinesToMoveBegin), std::make_move_iterator(myLinesToMoveEnd));
    m_lines.resize(fromLine);

    // move cursors
    QSet<Kate::TextRange *> ranges;
    for (auto it = m_cursors.begin(); it != m_cursors.end();) {
        auto cursor = *it;
        if (cursor->lineInBlock() >= fromLine) {
            cursor->m_line = cursor->lineInBlock() - fromLine;
            cursor->m_block = newBlock;

            // add to new, remove from current
            newBlock->m_cursors.push_back(cursor);
            it = m_cursors.erase(it);
            if (cursor->kateRange()) {
                ranges.insert(cursor->kateRange());
            }
        } else {
            // keep in current
            ++it;
        }
    }
    Q_ASSERT(std::is_sorted(newBlock->m_cursors.cbegin(), newBlock->m_cursors.cend()));

    for (auto range : std::as_const(ranges)) {
        if (range->spansMultipleBlocks()) {
            m_buffer->addMultilineRange(range);
        }
    }
}

void TextBlock::mergeBlock(TextBlock *targetBlock)
{
    // This function moves everything from *this into *targetBlock.
    // *targetBlock exists before *this with no blocks between.
    // Both this->m_cursors and targetBlock->m_cursors are sorted.

    // Iterating m_cursors backwards to modify TextRange's m_end before m_start.
    std::for_each(m_cursors.crbegin(), m_cursors.crend(), [targetBlockLines = targetBlock->lines(), targetBlock, m_buffer = m_buffer] (TextCursor *cursor) -> void {
        cursor->m_line += targetBlockLines;
        cursor->m_block = targetBlock;
        TextRange *range = cursor->m_range;
        if (range && cursor == &range->m_end && targetBlock == range->m_start.m_block) {
            m_buffer->removeMultilineRange(range);
        }
    });
    // move cursors
    auto first_insertion_pos = targetBlock->m_cursors.insert(targetBlock->m_cursors.cend(), m_cursors.cbegin(), m_cursors.cend());
    m_cursors.clear();
    // keep targetBlock->m_cursors sorted
    std::inplace_merge(targetBlock->m_cursors.begin(), first_insertion_pos, targetBlock->m_cursors.end());
    Q_ASSERT(std::is_sorted(targetBlock->m_cursors.cbegin(), targetBlock->m_cursors.cend()));
    // move lines
    targetBlock->m_lines.insert(targetBlock->m_lines.cend(), std::make_move_iterator(m_lines.begin()), std::make_move_iterator(m_lines.end()));
    m_lines.clear();
}

void TextBlock::rangesForLine(const int line, KTextEditor::View *view, bool rangesWithAttributeOnly, QList<TextRange *> &outRanges) const
{
    const int lineInBlock = line - startLine(); // line number in block
    for (TextCursor *cursor : std::as_const(m_cursors)) {
        if (!cursor->kateRange()) {
            continue;
        }
        TextRange *range = cursor->kateRange();
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

        if (
            // simple case
            (cursor->lineInBlock() == lineInBlock) ||
            // if line is in the range, ok
            (range->startInternal().lineInternal() <= line && line <= range->endInternal().lineInternal())
        ) {
            outRanges.append(range);
        }
    }
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
