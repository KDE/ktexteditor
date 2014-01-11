/* This file is part of the KDE libraries
   Copyright (C) 2010 Sebastian Sauer <mail@dipe.org>
   Copyright (C) 2012 Frederik Gladhorn <gladhorn@kde.org>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#ifndef _KATE_VIEW_ACCESSIBLE_
#define _KATE_VIEW_ACCESSIBLE_

#ifndef QT_NO_ACCESSIBILITY

#include "kateviewinternal.h"
#include "katetextcursor.h"
#include "katedocument.h"

#include <QAccessible>
#include <KLocalizedString>
#include <QAccessibleWidget>

/**
 * This class implements a QAccessible-interface for a KateViewInternal.
 *
 * This is the root class for the kateview. The \a KateCursorAccessible class
 * represents the cursor in the kateview and is a child of this class.
 */
class KateViewAccessible : public QAccessibleWidget, public QAccessibleTextInterface // FIXME maybe:, public QAccessibleEditableTextInterface
{
public:
    explicit KateViewAccessible(KateViewInternal *view)
        : QAccessibleWidget(view, QAccessible::EditableText)
    {
    }

    void *interface_cast(QAccessible::InterfaceType t)
    {
        if (t == QAccessible::TextInterface)
            return static_cast<QAccessibleTextInterface*>(this);
        return 0;
    }

    virtual ~KateViewAccessible()
    {
    }

    virtual QAccessibleInterface *childAt(int x, int y) const
    {
        Q_UNUSED(x);
        Q_UNUSED(y);
        return 0;
    }

    virtual void setText(QAccessible::Text t, const QString &text)
    {
        if (t == QAccessible::Value && view()->view()->document()) {
            view()->view()->document()->setText(text);
        }
    }

    virtual QAccessible::State state() const
    {
        QAccessible::State s = QAccessibleWidget::state();
        s.focusable = view()->focusPolicy() != Qt::NoFocus;
        s.focused = view()->hasFocus();
        s.editable = true;
        s.multiLine = true;
        s.selectableText = true;
        return s;
    }

    virtual QString text(QAccessible::Text t) const
    {
        QString s;
        if (view()->view()->document()) {
            if (t == QAccessible::Name) {
                s = view()->view()->document()->documentName();
            }
            if (t == QAccessible::Value) {
                s = view()->view()->document()->text();
            }
        }
        return s;
    }

    virtual int characterCount() const
    {
        return view()->view()->document()->text().size();
    }

    virtual void addSelection(int startOffset, int endOffset)
    {
        KTextEditor::Range range;
        range.setRange(cursorFromInt(startOffset), cursorFromInt(endOffset));
        view()->view()->setSelection(range);
        view()->view()->setCursorPosition(cursorFromInt(endOffset));
    }

    virtual QString attributes(int offset, int *startOffset, int *endOffset) const
    {
        Q_UNUSED(offset);
        *startOffset = 0;
        *endOffset = characterCount();
        return QString();
    }
    virtual QRect characterRect(int offset) const
    {
        KTextEditor::Cursor c = cursorFromInt(offset);
        if (!c.isValid()) {
            return QRect();
        }
        QPoint p = view()->view()->cursorToCoordinate(c);
        KTextEditor::Cursor endCursor = KTextEditor::Cursor(c.line(), c.column() + 1);
        QPoint size = view()->view()->cursorToCoordinate(endCursor) - p;
        return QRect(view()->mapToGlobal(p), QSize(size.x(), size.y()));
    }
    virtual int cursorPosition() const
    {
        KTextEditor::Cursor c = view()->getCursor();
        return positionFromCursor(c);
    }
    virtual int offsetAtPoint(const QPoint & /*point*/) const
    {
        return 0;
    }
    virtual void removeSelection(int selectionIndex)
    {
        if (selectionIndex != 0) {
            return;
        }
        view()->view()->clearSelection();
    }
    virtual void scrollToSubstring(int /*startIndex*/, int /*endIndex*/)
    {
        // FIXME
    }
    virtual void selection(int selectionIndex, int *startOffset, int *endOffset) const
    {
        if (selectionIndex != 0 || !view()->view()->selection()) {
            *startOffset = 0;
            *endOffset = 0;
            return;
        }
        KTextEditor::Range range = view()->view()->selectionRange();
        *startOffset = positionFromCursor(range.start());
        *endOffset = positionFromCursor(range.end());
    }

    virtual int selectionCount() const
    {
        return view()->view()->selection() ? 1 : 0;
    }
    virtual void setCursorPosition(int position)
    {
        view()->view()->setCursorPosition(cursorFromInt(position));
    }
    virtual void setSelection(int selectionIndex, int startOffset, int endOffset)
    {
        if (selectionIndex != 0) {
            return;
        }
        KTextEditor::Range range = KTextEditor::Range(cursorFromInt(startOffset), cursorFromInt(endOffset));
        view()->view()->setSelection(range);
    }
    virtual QString text(int startOffset, int endOffset) const
    {
        if (startOffset > endOffset) {
            return QString();
        }
        return view()->view()->document()->text().mid(startOffset, endOffset - startOffset);
    }

private:
    KateViewInternal *view() const
    {
        return static_cast<KateViewInternal *>(object());
    }

    KTextEditor::Cursor cursorFromInt(int position) const
    {
        int line = 0;
        for (;;) {
            const QString lineString = view()->view()->document()->line(line);
            if (position > lineString.length()) {
                // one is the newline
                position -= lineString.length() + 1;
                ++line;
            } else {
                break;
            }
        }
        return KTextEditor::Cursor(line, position);
    }

    int positionFromCursor(const KTextEditor::Cursor &cursor) const
    {
        int pos = 0;
        for (int line = 0; line < cursor.line(); ++line) {
            // length of the line plus newline
            pos += view()->view()->document()->line(line).size() + 1;
        }
        pos += cursor.column();

        return pos;
    }

    QString textLine(int shiftLines, int offset, int *startOffset, int *endOffset) const
    {
        KTextEditor::Cursor pos = cursorFromInt(offset);
        pos.setColumn(0);
        if (shiftLines) {
            pos.setLine(pos.line() + shiftLines);
        }
        *startOffset = positionFromCursor(pos);
        QString line = view()->view()->document()->line(pos.line()) + QLatin1Char('\n');
        *endOffset = *startOffset + line.length();
        return line;
    }
};

/**
 * Factory-function used to create \a KateViewAccessible instances for KateViewInternal
 * to make the KateViewInternal accessible.
 */
QAccessibleInterface *accessibleInterfaceFactory(const QString &key, QObject *object)
{
    Q_UNUSED(key)
    //if (key == QLatin1String("KateViewInternal"))
    if (KateViewInternal *view = qobject_cast<KateViewInternal *>(object)) {
        return new KateViewAccessible(view);
    }
    return 0;
}

#endif
#endif
