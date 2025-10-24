/*
    SPDX-FileCopyrightText: 2010 Christoph Cullmann <cullmann@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef KATE_TEXTBLOCK_H
#define KATE_TEXTBLOCK_H

#include "katetextline.h"

#include <QList>

#include <ktexteditor/cursor.h>
#include <ktexteditor_export.h>

namespace KTextEditor
{
class View;
}

namespace Kate
{
class TextBuffer;
class TextCursor;
class TextRange;

/**
 * Class representing a text block.
 * This is used to build up a Kate::TextBuffer.
 * This class should only be used by TextBuffer/Cursor/Range.
 */
class TextBlock
{
    friend class TextBuffer;

public:
    /**
     * Construct an empty text block.
     * @param buffer parent text buffer
     * @param startLine start line of this block
     */
    TextBlock(TextBuffer *buffer, int blockIndex);

    /**
     * Destruct the text block
     */
    ~TextBlock();

    /**
     * Start line of this block.
     * @return start line of this block
     */
    KTEXTEDITOR_EXPORT int startLine() const;

    void setBlockIndex(int index)
    {
        m_blockIndex = index;
    }

    /**
     * Retrieve a text line.
     * @param line wanted line number
     * @return text line
     */
    TextLine line(int line) const;

    /**
     * Transfer all non text attributes for the given line from the given text line to the one in the block.
     * @param line line number to set attributes
     * @param textLine line reference to get attributes from
     */
    void setLineMetaData(int line, const TextLine &textLine);

    /**
     * Retrieve length for @p line.
     * @param line wanted line number
     * @return length of line
     */
    int lineLength(int line) const
    {
        Q_ASSERT(line >= startLine() && (line - startLine()) < lines());
        return m_lines[line - startLine()].length();
    }

    /**
     * Append a new line with given text.
     * @param textOfLine text of the line to append
     */
    void appendLine(const QString &textOfLine);

    /**
     * Clear the lines.
     */
    void clearLines();

    /**
     * Number of lines in this block.
     * @return number of lines
     */
    int lines() const
    {
        return static_cast<int>(m_lines.size());
    }

    /**
     * Retrieve text of block.
     * @param text for this block, lines separated by '\n'
     */
    void text(QString &text) const;

    /**
     * Wrap line at given cursor position.
     * @param position line/column as cursor where to wrap
     * @param fixStartLinesStartIndex start index to fix start lines, normally this is this block
     */
    void wrapLine(const KTextEditor::Cursor position, int fixStartLinesStartIndex);

    /**
     * Unwrap given line.
     * @param line line to unwrap
     * @param previousBlock previous block, if any, if we unwrap first line in block, we need to have this
     * @param fixStartLinesStartIndex start index to fix start lines, normally this is this block or the previous one
     */
    void unwrapLine(int line, TextBlock *previousBlock, int fixStartLinesStartIndex);

    /**
     * Insert text at given cursor position.
     * @param position position where to insert text
     * @param text text to insert
     */
    void insertText(const KTextEditor::Cursor position, const QString &text);

    /**
     * Remove text at given range.
     * @param range range of text to remove, must be on one line only.
     * @param removedText will be filled with removed text
     */
    void removeText(KTextEditor::Range range, QString &removedText);

    /**
     * Debug output, print whole block content with line numbers and line length
     * @param blockIndex index of this block in buffer
     */
    void debugPrint(int blockIndex) const;

    /**
     * Split given block. All lines starting from @p fromLine will
     * be moved to it, together with the cursors belonging to it.
     * @param fromLine line from which to split
     * @param newBlock The block to which the data will be moved after splitting. It shouldn't have any cursors before call.
     */
    void splitBlock(int fromLine, TextBlock *newBlock);

    /**
     * Merge this block with given one, the given one must be a direct predecessor.
     * @param targetBlock block to merge with
     */
    void mergeBlock(TextBlock *targetBlock);

    /**
     * Append to outRanges addresses of all ranges in this block which might intersect the given line.
     * @param line                          line to check intersection
     * @param view                          only return ranges associated with given view
     * @param rangesWithAttributeOnly       ranges with attributes only?
     * @param outRanges                     where to append results
     */
    KTEXTEDITOR_NO_EXPORT void rangesForLine(int line, KTextEditor::View *view, bool rangesWithAttributeOnly, QList<TextRange *> &outRanges) const;

    /**
     * Flag all modified text lines as saved on disk.
     */
    void markModifiedLinesAsSaved();

    /**
     * Insert cursor into this block.
     * @param cursor cursor to insert
     */
    void insertCursor(Kate::TextCursor *cursor);

    /**
     * Remove cursor from this block.
     * @param cursor cursor to remove
     */
    void removeCursor(Kate::TextCursor *cursor);

private:
    /**
     * parent text buffer
     */
    TextBuffer *m_buffer;

    /**
     * The index of this block in TextBuffer::m_blocks
     */
    int m_blockIndex;

    /**
     * Lines contained in this buffer.
     * We need no sharing, use STL.
     */
    std::vector<Kate::TextLine> m_lines;

    /**
     * Set of cursors for this block.
     */
    std::vector<TextCursor *> m_cursors;
};
}

#endif
