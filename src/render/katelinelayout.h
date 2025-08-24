/*
    SPDX-FileCopyrightText: 2002-2005 Hamish Rodda <rodda@kde.org>
    SPDX-FileCopyrightText: 2003 Anakim Border <aborder@sources.sourceforge.net>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef _KATE_LINELAYOUT_H_
#define _KATE_LINELAYOUT_H_

#include <QExplicitlySharedDataPointer>
#include <QSharedData>
#include <QTextLayout>

#include <ktexteditor/cursor.h>

namespace KTextEditor
{
class DocumentPrivate;
}
namespace Kate
{
class TextFolding;
}
class KateTextLayout;
class KateRenderer;

class KateLineLayout
{
public:
    explicit KateLineLayout();

    void debugOutput() const;

    void clear();
    bool isValid() const;

    bool isRightToLeft() const;

    bool includesCursor(const KTextEditor::Cursor realCursor) const;

    friend bool operator>(const KateLineLayout &r, const KTextEditor::Cursor c);
    friend bool operator>=(const KateLineLayout &r, const KTextEditor::Cursor c);
    friend bool operator<(const KateLineLayout &r, const KTextEditor::Cursor c);
    friend bool operator<=(const KateLineLayout &r, const KTextEditor::Cursor c);

    int line() const;
    /**
     * Only pass virtualLine if you know it (and thus we shouldn't try to look it up)
     */
    void setLine(Kate::TextFolding &folding, int line, int virtualLine = -1);
    KTextEditor::Cursor start() const;

    int virtualLine() const;
    void setVirtualLine(int virtualLine);

    bool isDirty(int viewLine) const;
    bool setDirty(int viewLine, bool dirty = true);

    int width() const;
    int widthOfLastLine();

    int viewLineCount() const;
    KateTextLayout viewLine(int viewLine);
    int viewLineForColumn(int column) const;

    bool startsInvisibleBlock(Kate::TextFolding &folding) const;

    const QTextLayout &layout() const
    {
        return m_layout;
    }

    // just used to generate a new layout together with endLayout
    QTextLayout &modifiableLayout()
    {
        return m_layout;
    }

    void endLayout();
    void invalidateLayout();

    bool layoutDirty = true;

    // This variable is used as follows:
    // non-dynamic-wrapping mode: unused
    // dynamic wrapping mode:
    //   first viewLine of a line: the X position of the first non-whitespace char
    //   subsequent viewLines: the X offset from the left of the display.
    //
    // this is used to provide a dynamic-wrapping-retains-indent feature.
    int shiftX = 0;

private:
    // Disable copy
    KateLineLayout(const KateLineLayout &copy);

    int m_line;
    int m_virtualLine;

    QTextLayout m_layout;
    QList<bool> m_dirtyList;
};

#endif
