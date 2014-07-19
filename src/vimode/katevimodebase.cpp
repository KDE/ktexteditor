/*  This file is part of the KDE libraries and the Kate part.
 *
 *  Copyright (C) 2008 - 2012 Erlend Hamberg <ehamberg@gmail.com>
 *  Copyright (C) 2009 Paul Gideon Dann <pdgiddie@gmail.com>
 *  Copyright (C) 2011 Svyatoslav Kuzmich <svatoslav1@gmail.com>
 *  Copyright (C) 2012 - 2013 Simon St James <kdedevel@etotheipiplusone.com>
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

#include "katevimodebase.h"
#include <vimode/range.h>
#include "kateglobal.h"
#include "kateviinputmode.h"
#include "globalstate.h"
#include "katevivisualmode.h"
#include "katevinormalmode.h"
#include "katevireplacemode.h"
#include "kateviinputmodemanager.h"
#include "katelayoutcache.h"
#include "kateconfig.h"
#include "katedocument.h"
#include "kateviewinternal.h"
#include "katerenderer.h"
#include "katepartdebug.h"
#include "marks.h"
#include "jumps.h"
#include "registers.h"
#include "searcher.h"
#include "lastchangerecorder.h"

#include <QString>
#include <KLocalizedString>
#include <QRegExp>

using namespace KateVi;

// TODO: the "previous word/WORD [end]" methods should be optimized. now they're being called in a
// loop and all calculations done up to finding a match are trown away when called with a count > 1
// because they will simply be called again from the last found position.
// They should take the count as a parameter and collect the positions in a QList, then return
// element (count - 1)

////////////////////////////////////////////////////////////////////////////////
// HELPER METHODS
////////////////////////////////////////////////////////////////////////////////

void KateViModeBase::yankToClipBoard(QChar chosen_register, QString text)
{
    //only yank to the clipboard if no register was specified,
    // textlength > 1 and there is something else then whitespace
    if ((chosen_register == QLatin1Char('0') || chosen_register == QLatin1Char('-'))
        && text.length() > 1 && !text.trimmed().isEmpty())
    {
        KTextEditor::EditorPrivate::self()->copyToClipboard(text);
    }
}

bool KateViModeBase::deleteRange(KateVi::ViRange &r, OperationMode mode, bool addToRegister)
{
    r.normalize();
    bool res = false;
    QString removedText = getRange(r, mode);

    if (mode == LineWise) {
        doc()->editStart();
        for (int i = 0; i < r.endLine - r.startLine + 1; i++) {
            res = doc()->removeLine(r.startLine);
        }
        doc()->editEnd();
    } else {
        res = doc()->removeText(KTextEditor::Range(r.startLine, r.startColumn, r.endLine, r.endColumn), mode == Block);
    }

    QChar chosenRegister = getChosenRegister(ZeroRegister);
    if (addToRegister) {
        if (r.startLine == r.endLine) {
            chosenRegister = getChosenRegister(SmallDeleteRegister);
            fillRegister(chosenRegister, removedText, mode);
        } else {
            fillRegister(chosenRegister,  removedText, mode);
        }
    }
    yankToClipBoard(chosenRegister, removedText);

    return res;
}

const QString KateViModeBase::getRange(KateVi::ViRange &r, OperationMode mode) const
{
    r.normalize();
    QString s;

    if (mode == LineWise) {
        r.startColumn = 0;
        r.endColumn = getLine(r.endLine).length();
    }

    if (r.motionType == InclusiveMotion) {
        r.endColumn++;
    }

    KTextEditor::Range range(r.startLine, r.startColumn, r.endLine, r.endColumn);

    if (mode == LineWise) {
        s = doc()->textLines(range).join(QLatin1Char('\n'));
        s.append(QLatin1Char('\n'));
    } else if (mode == Block) {
        s = doc()->text(range, true);
    } else {
        s = doc()->text(range);
    }

    return s;
}

const QString KateViModeBase::getLine(int line) const
{
    return (line < 0) ? m_view->currentTextLine() : doc()->line(line);
}

const QChar KateViModeBase::getCharUnderCursor() const
{
    KTextEditor::Cursor c(m_view->cursorPosition());

    QString line = getLine(c.line());

    if (line.length() == 0 && c.column() >= line.length()) {
        return QChar::Null;
    }

    return line.at(c.column());
}

const QString KateViModeBase::getWordUnderCursor() const
{

    return doc()->text(getWordRangeUnderCursor());
}

const KTextEditor::Range KateViModeBase::getWordRangeUnderCursor() const
{
    KTextEditor::Cursor c(m_view->cursorPosition());

    // find first character that is a “word letter” and start the search there
    QChar ch = doc()->characterAt(c);
    int i = 0;
    while (!ch.isLetterOrNumber() && ! ch.isMark() && ch != QLatin1Char('_')
            && m_extraWordCharacters.indexOf(ch) == -1) {

        // advance cursor one position
        c.setColumn(c.column() + 1);
        if (c.column() > doc()->lineLength(c.line())) {
            c.setColumn(0);
            c.setLine(c.line() + 1);
            if (c.line() == doc()->lines()) {
                return KTextEditor::Range::invalid();
            }
        }

        ch = doc()->characterAt(c);
        i++; // count characters that were advanced so we know where to start the search
    }

    // move cursor the word (if cursor was placed on e.g. a paren, this will move
    // it to the right
    updateCursor(c);

    KTextEditor::Cursor c1 = findPrevWordStart(c.line(), c.column() + 1 + i, true);
    KTextEditor::Cursor c2 = findWordEnd(c1.line(), c1.column() + i - 1, true);
    c2.setColumn(c2.column() + 1);

    return KTextEditor::Range(c1, c2);
}

KTextEditor::Cursor KateViModeBase::findNextWordStart(int fromLine, int fromColumn, bool onlyCurrentLine) const
{
    QString line = getLine(fromLine);

    // the start of word pattern need to take m_extraWordCharacters into account if defined
    QString startOfWordPattern = QString::fromLatin1("\\b(\\w");
    if (m_extraWordCharacters.length() > 0) {
        startOfWordPattern.append(QLatin1String("|[") + m_extraWordCharacters + QLatin1Char(']'));
    }
    startOfWordPattern.append(QLatin1Char(')'));

    QRegExp startOfWord(startOfWordPattern);      // start of a word
    QRegExp nonSpaceAfterSpace(QLatin1String("\\s\\S"));       // non-space right after space
    QRegExp nonWordAfterWord(QLatin1String("\\b(?!\\s)\\W"));  // word-boundary followed by a non-word which is not a space

    int l = fromLine;
    int c = fromColumn;

    bool found = false;

    while (!found) {
        int c1 = startOfWord.indexIn(line, c + 1);
        int c2 = nonSpaceAfterSpace.indexIn(line, c);
        int c3 = nonWordAfterWord.indexIn(line, c + 1);

        if (c1 == -1 && c2 == -1 && c3 == -1) {
            if (onlyCurrentLine) {
                return KTextEditor::Cursor::invalid();
            } else if (l >= doc()->lines() - 1) {
                c = qMax(line.length() - 1, 0);
                return KTextEditor::Cursor::invalid();
            } else {
                c = 0;
                l++;

                line = getLine(l);

                if (line.length() == 0 || !line.at(c).isSpace()) {
                    found = true;
                }

                continue;
            }
        }

        c2++; // the second regexp will match one character *before* the character we want to go to

        if (c1 <= 0) {
            c1 = line.length() - 1;
        }
        if (c2 <= 0) {
            c2 = line.length() - 1;
        }
        if (c3 <= 0) {
            c3 = line.length() - 1;
        }

        c = qMin(c1, qMin(c2, c3));

        found = true;
    }

    return KTextEditor::Cursor(l, c);
}

KTextEditor::Cursor KateViModeBase::findNextWORDStart(int fromLine, int fromColumn, bool onlyCurrentLine) const
{
    KTextEditor::Cursor cursor(m_view->cursorPosition());
    QString line = getLine();

    int l = fromLine;
    int c = fromColumn;

    bool found = false;
    QRegExp startOfWORD(QLatin1String("\\s\\S"));

    while (!found) {
        c = startOfWORD.indexIn(line, c);

        if (c == -1) {
            if (onlyCurrentLine) {
                return KTextEditor::Cursor(l, c);
            } else if (l >= doc()->lines() - 1) {
                c = line.length() - 1;
                break;
            } else {
                c = 0;
                l++;

                line = getLine(l);

                if (line.length() == 0 || !line.at(c).isSpace()) {
                    found = true;
                }

                continue;
            }
        } else {
            c++;
            found = true;
        }
    }

    return KTextEditor::Cursor(l, c);
}

KTextEditor::Cursor KateViModeBase::findPrevWordEnd(int fromLine, int fromColumn, bool onlyCurrentLine) const
{
    QString line = getLine(fromLine);

    QString endOfWordPattern = QString::fromLatin1("\\S\\s|\\S$|\\w\\W|\\S\\b|^$");

    if (m_extraWordCharacters.length() > 0) {
        endOfWordPattern.append(QLatin1String("|[") + m_extraWordCharacters + QLatin1String("][^") + m_extraWordCharacters + QLatin1Char(']'));
    }

    QRegExp endOfWord(endOfWordPattern);

    int l = fromLine;
    int c = fromColumn;

    bool found = false;

    while (!found) {
        int c1 = endOfWord.lastIndexIn(line, c - 1);

        if (c1 != -1 && c - 1 != -1) {
            found = true;
            c = c1;
        } else {
            if (onlyCurrentLine) {
                return KTextEditor::Cursor::invalid();
            } else if (l > 0) {
                line = getLine(--l);
                c = line.length();

                continue;
            } else {
                return KTextEditor::Cursor::invalid();
            }
        }
    }

    return KTextEditor::Cursor(l, c);
}

KTextEditor::Cursor KateViModeBase::findPrevWORDEnd(int fromLine, int fromColumn, bool onlyCurrentLine) const
{
    QString line = getLine(fromLine);

    QRegExp endOfWORDPattern(QLatin1String("\\S\\s|\\S$|^$"));

    QRegExp endOfWORD(endOfWORDPattern);

    int l = fromLine;
    int c = fromColumn;

    bool found = false;

    while (!found) {
        int c1 = endOfWORD.lastIndexIn(line, c - 1);

        if (c1 != -1 && c - 1 != -1) {
            found = true;
            c = c1;
        } else {
            if (onlyCurrentLine) {
                return KTextEditor::Cursor::invalid();
            } else if (l > 0) {
                line = getLine(--l);
                c = line.length();

                continue;
            } else {
                c = 0;
                return KTextEditor::Cursor::invalid();
            }
        }
    }

    return KTextEditor::Cursor(l, c);
}

KTextEditor::Cursor KateViModeBase::findPrevWordStart(int fromLine, int fromColumn, bool onlyCurrentLine) const
{
    QString line = getLine(fromLine);

    // the start of word pattern need to take m_extraWordCharacters into account if defined
    QString startOfWordPattern = QString::fromLatin1("\\b(\\w");
    if (m_extraWordCharacters.length() > 0) {
        startOfWordPattern.append(QLatin1String("|[") + m_extraWordCharacters + QLatin1Char(']'));
    }
    startOfWordPattern.append(QLatin1Char(')'));

    QRegExp startOfWord(startOfWordPattern);      // start of a word
    QRegExp nonSpaceAfterSpace(QLatin1String("\\s\\S"));       // non-space right after space
    QRegExp nonWordAfterWord(QLatin1String("\\b(?!\\s)\\W"));  // word-boundary followed by a non-word which is not a space
    QRegExp startOfLine(QLatin1String("^\\S"));                // non-space at start of line

    int l = fromLine;
    int c = fromColumn;

    bool found = false;

    while (!found) {
        int c1 = startOfWord.lastIndexIn(line, -line.length() + c - 1);
        int c2 = nonSpaceAfterSpace.lastIndexIn(line, -line.length() + c - 2);
        int c3 = nonWordAfterWord.lastIndexIn(line, -line.length() + c - 1);
        int c4 = startOfLine.lastIndexIn(line, -line.length() + c - 1);

        if (c1 == -1 && c2 == -1 && c3 == -1 && c4 == -1) {
            if (onlyCurrentLine) {
                return KTextEditor::Cursor::invalid();
            } else if (l <= 0) {
                return KTextEditor::Cursor::invalid();
            } else {
                line = getLine(--l);
                c = line.length();

                if (line.length() == 0) {
                    c = 0;
                    found = true;
                }

                continue;
            }
        }

        c2++; // the second regexp will match one character *before* the character we want to go to

        if (c1 <= 0) {
            c1 = 0;
        }
        if (c2 <= 0) {
            c2 = 0;
        }
        if (c3 <= 0) {
            c3 = 0;
        }
        if (c4 <= 0) {
            c4 = 0;
        }

        c = qMax(c1, qMax(c2, qMax(c3, c4)));

        found = true;
    }

    return KTextEditor::Cursor(l, c);
}

KTextEditor::Cursor KateViModeBase::findPrevWORDStart(int fromLine, int fromColumn, bool onlyCurrentLine) const
{
    QString line = getLine(fromLine);

    QRegExp startOfWORD(QLatin1String("\\s\\S"));
    QRegExp startOfLineWORD(QLatin1String("^\\S"));

    int l = fromLine;
    int c = fromColumn;

    bool found = false;

    while (!found) {
        int c1 = startOfWORD.lastIndexIn(line, -line.length() + c - 2);
        int c2 = startOfLineWORD.lastIndexIn(line, -line.length() + c - 1);

        if (c1 == -1 && c2 == -1) {
            if (onlyCurrentLine) {
                return KTextEditor::Cursor::invalid();
            } else if (l <= 0) {
                return KTextEditor::Cursor::invalid();
            } else {
                line = getLine(--l);
                c = line.length();

                if (line.length() == 0) {
                    c = 0;
                    found = true;
                }

                continue;
            }
        }

        c1++; // the startOfWORD pattern matches one character before the word

        c = qMax(c1, c2);

        if (c <= 0) {
            c = 0;
        }

        found = true;
    }

    return KTextEditor::Cursor(l, c);
}

KTextEditor::Cursor KateViModeBase::findWordEnd(int fromLine, int fromColumn, bool onlyCurrentLine) const
{
    QString line = getLine(fromLine);

    QString endOfWordPattern = QString::fromLatin1("\\S\\s|\\S$|\\w\\W|\\S\\b");

    if (m_extraWordCharacters.length() > 0) {
        endOfWordPattern.append(QLatin1String("|[") + m_extraWordCharacters + QLatin1String("][^") + m_extraWordCharacters + QLatin1Char(']'));
    }

    QRegExp endOfWORD(endOfWordPattern);

    int l = fromLine;
    int c = fromColumn;

    bool found = false;

    while (!found) {
        int c1 = endOfWORD.indexIn(line, c + 1);

        if (c1 != -1) {
            found = true;
            c = c1;
        } else {
            if (onlyCurrentLine) {
                return KTextEditor::Cursor::invalid();
            } else if (l >= doc()->lines() - 1) {
                c = line.length() - 1;
                return KTextEditor::Cursor::invalid();
            } else {
                c = -1;
                line = getLine(++l);

                continue;
            }
        }
    }

    return KTextEditor::Cursor(l, c);
}

KTextEditor::Cursor KateViModeBase::findWORDEnd(int fromLine, int fromColumn, bool onlyCurrentLine) const
{
    QString line = getLine(fromLine);

    QRegExp endOfWORD(QLatin1String("\\S\\s|\\S$"));

    int l = fromLine;
    int c = fromColumn;

    bool found = false;

    while (!found) {
        int c1 = endOfWORD.indexIn(line, c + 1);

        if (c1 != -1) {
            found = true;
            c = c1;
        } else {
            if (onlyCurrentLine) {
                return KTextEditor::Cursor::invalid();
            } else if (l >= doc()->lines() - 1) {
                c = line.length() - 1;
                return KTextEditor::Cursor::invalid();
            } else {
                c = -1;
                line = getLine(++l);

                continue;
            }
        }
    }

    return KTextEditor::Cursor(l, c);
}

ViRange innerRange(KateVi::ViRange range, bool inner)
{
    ViRange r = range;

    if (inner) {
        const int columnDistance = qAbs(r.startColumn - r.endColumn);
        if ((r.startLine == r.endLine) && columnDistance == 1) {
            // Start and end are right next to each other; there is nothing inside them.
            return ViRange::invalid();
        }
        r.startColumn++;
        r.endColumn--;
    }

    return r;
}

ViRange KateViModeBase::findSurroundingQuotes(const QChar &c, bool inner) const
{
    KTextEditor::Cursor cursor(m_view->cursorPosition());
    ViRange r;
    r.startLine = cursor.line();
    r.endLine = cursor.line();

    QString line = doc()->line(cursor.line());

    // If cursor on the quote we should shoose the best direction.
    if (line.at(cursor.column()) == c) {

        int attribute = m_view->doc()->kateTextLine(cursor.line())->attribute(cursor.column());

        //  If at the beginning of the line - then we might search the end.
        if (doc()->kateTextLine(cursor.line())->attribute(cursor.column() + 1) == attribute &&
                doc()->kateTextLine(cursor.line())->attribute(cursor.column() - 1) != attribute) {
            r.startColumn = cursor.column();
            r.endColumn = line.indexOf(c, cursor.column() + 1);

            return innerRange(r, inner);
        }

        //  If at the end of the line - then we might search the beginning.
        if (doc()->kateTextLine(cursor.line())->attribute(cursor.column() + 1) != attribute &&
                doc()->kateTextLine(cursor.line())->attribute(cursor.column() - 1) == attribute) {

            r.startColumn = line.lastIndexOf(c, cursor.column() - 1);
            r.endColumn = cursor.column();

            return innerRange(r, inner);

        }
        // Try to search the quote to right
        int c1 = line.indexOf(c, cursor.column() + 1);
        if (c1 != -1) {
            r.startColumn = cursor.column();
            r.endColumn = c1;

            return innerRange(r, inner);
        }

        // Try to search the quote to left
        int c2 = line.lastIndexOf(c, cursor.column() - 1);
        if (c2 != -1) {
            r.startColumn = c2;
            r.endColumn =  cursor.column();

            return innerRange(r, inner);
        }

        // Nothing found - give up :)
        return ViRange::invalid();
    }

    r.startColumn = line.lastIndexOf(c, cursor.column());
    r.endColumn = line.indexOf(c, cursor.column());

    if (r.startColumn == -1 || r.endColumn == -1 || r.startColumn > r.endColumn) {
        return ViRange::invalid();
    }

    return innerRange(r, inner);
}

ViRange KateViModeBase::findSurroundingBrackets(const QChar &c1,
        const QChar &c2,
        bool inner,
        const QChar &nested1,
        const QChar &nested2) const
{

    KTextEditor::Cursor cursor(m_view->cursorPosition());

    ViRange r(cursor, InclusiveMotion);

    // Chars should not differs. For equal chars use findSurroundingQuotes.
    Q_ASSERT(c1 != c2);

    QStack<QChar> stack;
    int column = cursor.column();
    int line = cursor.line();
    bool should_break = false;

    // Going through the text and pushing respectively brackets to the stack.
    // Then pop it out if the stack head is the bracket under cursor.

    if (column < m_view->doc()->line(line).size() && m_view->doc()->line(line).at(column) == c2) {
        r.endLine = line;
        r.endColumn = column;
    } else {

        if (column < m_view->doc()->line(line).size() && m_view->doc()->line(line).at(column) == c1) {
            column++;
        }

        stack.push(c2);
        for (; line < m_view->doc()->lines() && !should_break; line++) {
            for (; column < m_view->doc()->line(line).size(); column++) {
                QChar next_char = stack.pop();

                if (next_char != m_view->doc()->line(line).at(column)) {
                    stack.push(next_char);
                }

                if (stack.isEmpty()) {
                    should_break = true;
                    break;
                }

                if (m_view->doc()->line(line).at(column) == nested1) {
                    stack.push(nested2);
                }
            }
            if (should_break) {
                break;
            }

            column = 0;
        }

        if (!should_break) {
            return ViRange::invalid();
        }

        r.endColumn = column;
        r.endLine = line;

    }

    // The same algorythm but going from the left to right.

    line = cursor.line();
    column = cursor.column();

    if (column < m_view->doc()->line(line).size() && m_view->doc()->line(line).at(column) == c1) {
        r.startLine = line;
        r.startColumn = column;
    } else {
        if (column < m_view->doc()->line(line).size() && m_view->doc()->line(line).at(column) == c2) {
            column--;
        }

        stack.clear();
        stack.push(c1);

        should_break = false;
        for (; line >= 0 && !should_break; line--) {
            for (; column >= 0 &&  column < m_view->doc()->line(line).size(); column--) {
                QChar next_char = stack.pop();

                if (next_char != m_view->doc()->line(line).at(column)) {
                    stack.push(next_char);
                }

                if (stack.isEmpty()) {
                    should_break = true;
                    break;
                }

                if (m_view->doc()->line(line).at(column) == nested2) {
                    stack.push(nested1);
                }
            }

            if (should_break) {
                break;
            }

            column = m_view->doc()->line(line - 1).size() - 1;
        }

        if (!should_break) {
            return ViRange::invalid();
        }

        r.startColumn = column;
        r.startLine = line;

    }

    return innerRange(r, inner);
}

ViRange KateViModeBase::findSurrounding(const QRegExp &c1, const QRegExp &c2, bool inner) const
{
    KTextEditor::Cursor cursor(m_view->cursorPosition());
    QString line = getLine();

    int col1 = line.lastIndexOf(c1, cursor.column());
    int col2 = line.indexOf(c2, cursor.column());

    ViRange r(cursor.line(), col1, cursor.line(), col2, InclusiveMotion);

    if (col1 == -1 || col2 == -1 || col1 > col2) {
        return ViRange::invalid();
    }

    if (inner) {
        r.startColumn++;
        r.endColumn--;
    }

    return r;
}

int KateViModeBase::findLineStartingWitchChar(const QChar &c, unsigned int count, bool forward) const
{
    int line = m_view->cursorPosition().line();
    int lines = doc()->lines();
    unsigned int hits = 0;

    if (forward) {
        line++;
    } else {
        line--;
    }

    while (line < lines && line >= 0 && hits < count) {
        QString l = getLine(line);
        if (l.length() > 0 && l.at(0) == c) {
            hits++;
        }
        if (hits != count) {
            if (forward) {
                line++;
            } else {
                line--;
            }
        }
    }

    if (hits == getCount()) {
        return line;
    }

    return -1;
}

void KateViModeBase::updateCursor(const KTextEditor::Cursor &c) const
{
    m_viInputModeManager->updateCursor(c);
}

/**
 * @return the register given for the command. If no register was given, defaultReg is returned.
 */
QChar KateViModeBase::getChosenRegister(const QChar &defaultReg) const
{
    return (m_register != QChar::Null) ? m_register : defaultReg;
}

QString KateViModeBase::getRegisterContent(const QChar &reg)
{
    QString r = m_viInputModeManager->globalState()->registers()->getContent(reg);

    if (r.isNull()) {
        error(i18n("Nothing in register %1", reg));
    }

    return r;
}

OperationMode KateViModeBase::getRegisterFlag(const QChar &reg) const
{
    return m_viInputModeManager->globalState()->registers()->getFlag(reg);
}

void KateViModeBase::fillRegister(const QChar &reg, const QString &text, OperationMode flag)
{
    m_viInputModeManager->globalState()->registers()->set(reg, text, flag);
}

KTextEditor::Cursor KateViModeBase::getNextJump(KTextEditor::Cursor cursor) const
{
    return m_viInputModeManager->jumps()->next(cursor);
}

KTextEditor::Cursor KateViModeBase::getPrevJump(KTextEditor::Cursor cursor) const
{
    return m_viInputModeManager->jumps()->prev(cursor);
}

ViRange KateViModeBase::goLineDown()
{
    return goLineUpDown(getCount());
}

ViRange KateViModeBase::goLineUp()
{
    return goLineUpDown(-getCount());
}

/**
 * method for moving up or down one or more lines
 * note: the sticky column is always a virtual column
 */
ViRange KateViModeBase::goLineUpDown(int lines)
{
    KTextEditor::Cursor c(m_view->cursorPosition());
    ViRange r(c, InclusiveMotion);
    int tabstop = doc()->config()->tabWidth();

    // if in an empty document, just return
    if (lines == 0) {
        return r;
    }

    r.endLine += lines;

    // limit end line to be from line 0 through the last line
    if (r.endLine < 0) {
        r.endLine = 0;
    } else if (r.endLine > doc()->lines() - 1) {
        r.endLine = doc()->lines() - 1;
    }

    Kate::TextLine startLine = doc()->plainKateTextLine(c.line());
    Kate::TextLine endLine = doc()->plainKateTextLine(r.endLine);

    int endLineLen = doc()->lineLength(r.endLine) - 1;

    if (endLineLen < 0) {
        endLineLen = 0;
    }

    int endLineLenVirt = endLine->toVirtualColumn(endLineLen, tabstop);
    int virtColumnStart = startLine->toVirtualColumn(c.column(), tabstop);

    // if sticky column isn't set, set end column and set sticky column to its virtual column
    if (m_stickyColumn == -1) {
        r.endColumn = endLine->fromVirtualColumn(virtColumnStart, tabstop);
        m_stickyColumn = virtColumnStart;
    } else {
        // sticky is set - set end column to its value
        r.endColumn = endLine->fromVirtualColumn(m_stickyColumn, tabstop);
    }

    // make sure end column won't be after the last column of a line
    if (r.endColumn > endLineLen) {
        r.endColumn = endLineLen;
    }

    // if we move to a line shorter than the current column, go to its end
    if (virtColumnStart > endLineLenVirt) {
        r.endColumn = endLineLen;
    }

    return r;
}

ViRange KateViModeBase::goVisualLineUpDown(int lines)
{
    KTextEditor::Cursor c(m_view->cursorPosition());
    ViRange r(c, InclusiveMotion);
    int tabstop = doc()->config()->tabWidth();

    if (lines == 0) {
        // We're not moving anywhere.
        return r;
    }

    KateLayoutCache *cache = m_viInputModeManager->inputAdapter()->layoutCache();

    // Work out the real and visual line pair of the beginning of the visual line we'd end up
    // on by moving lines visual lines.  We ignore the column, for now.
    int finishVisualLine = cache->viewLine(m_view->cursorPosition());
    int finishRealLine = m_view->cursorPosition().line();
    int count = qAbs(lines);
    bool invalidPos = false;
    if (lines > 0) {
        // Find the beginning of the visual line "lines" visual lines down.
        while (count > 0) {
            finishVisualLine++;
            if (finishVisualLine >= cache->line(finishRealLine)->viewLineCount()) {
                finishRealLine++;
                finishVisualLine = 0;
            }
            if (finishRealLine >= doc()->lines()) {
                invalidPos = true;
                break;
            }
            count--;
        }
    } else {
        // Find the beginning of the visual line "lines" visual lines up.
        while (count > 0) {
            finishVisualLine--;
            if (finishVisualLine < 0) {
                finishRealLine--;
                if (finishRealLine < 0) {
                    invalidPos = true;
                    break;
                }
                finishVisualLine = cache->line(finishRealLine)->viewLineCount() - 1;
            }
            count--;
        }
    }
    if (invalidPos) {
        r.endLine = -1;
        r.endColumn = -1;
        return r;
    }

    // We know the final (real) line ...
    r.endLine = finishRealLine;
    // ... now work out the final (real) column.

    if (m_stickyColumn == -1 || !m_lastMotionWasVisualLineUpOrDown) {
        // Compute new sticky column. It is a *visual* sticky column.
        int startVisualLine = cache->viewLine(m_view->cursorPosition());
        int startRealLine = m_view->cursorPosition().line();
        const Kate::TextLine startLine = doc()->plainKateTextLine(c.line());
        // Adjust for the fact that if the portion of the line before wrapping is indented,
        // the continuations are also "invisibly" (i.e. without any spaces in the text itself) indented.
        const bool isWrappedContinuation = (cache->textLayout(startRealLine, startVisualLine).lineLayout().lineNumber() != 0);
        const int numInvisibleIndentChars = isWrappedContinuation ? startLine->toVirtualColumn(cache->line(startRealLine)->textLine()->nextNonSpaceChar(0), tabstop) : 0;

        const int realLineStartColumn = cache->textLayout(startRealLine, startVisualLine).startCol();
        const int lineStartVirtualColumn = startLine->toVirtualColumn(realLineStartColumn, tabstop);
        const int visualColumnNoInvisibleIndent  = startLine->toVirtualColumn(c.column(), tabstop) - lineStartVirtualColumn;
        m_stickyColumn = visualColumnNoInvisibleIndent + numInvisibleIndentChars;
        Q_ASSERT(m_stickyColumn >= 0);
    }

    // The "real" (non-virtual) beginning of the current "line", which might be a wrapped continuation of a
    // "real" line.
    const int realLineStartColumn = cache->textLayout(finishRealLine, finishVisualLine).startCol();
    const Kate::TextLine endLine = doc()->plainKateTextLine(r.endLine);
    // Adjust for the fact that if the portion of the line before wrapping is indented,
    // the continuations are also "invisibly" (i.e. without any spaces in the text itself) indented.
    const bool isWrappedContinuation = (cache->textLayout(finishRealLine, finishVisualLine).lineLayout().lineNumber() != 0);
    const int numInvisibleIndentChars = isWrappedContinuation ? endLine->toVirtualColumn(cache->line(finishRealLine)->textLine()->nextNonSpaceChar(0), tabstop) : 0;
    if (m_stickyColumn == (unsigned int)KateVi::EOL) {
        const int visualEndColumn = cache->textLayout(finishRealLine, finishVisualLine).lineLayout().textLength() - 1;
        r.endColumn = endLine->fromVirtualColumn(visualEndColumn + realLineStartColumn - numInvisibleIndentChars, tabstop);
    } else {
        // Algorithm: find the "real" column corresponding to the start of the line.  Offset from that
        // until the "visual" column is equal to the "visual" sticky column.
        int realOffsetToVisualStickyColumn = 0;
        const int lineStartVirtualColumn = endLine->toVirtualColumn(realLineStartColumn, tabstop);
        while (true) {
            const int visualColumn = endLine->toVirtualColumn(realLineStartColumn + realOffsetToVisualStickyColumn, tabstop) - lineStartVirtualColumn + numInvisibleIndentChars;
            if (visualColumn >= m_stickyColumn) {
                break;
            }
            realOffsetToVisualStickyColumn++;
        }
        r.endColumn = realLineStartColumn + realOffsetToVisualStickyColumn;
    }
    m_currentMotionWasVisualLineUpOrDown = true;

    return r;
}

bool KateViModeBase::startNormalMode()
{
    /* store the key presses for this "insert mode session" so that it can be repeated with the
     * '.' command
     * - ignore transition from Visual Modes
     */
    if (!(m_viInputModeManager->isAnyVisualMode() || m_viInputModeManager->lastChangeRecorder()->isReplaying())) {
        m_viInputModeManager->storeLastChangeCommand();
        m_viInputModeManager->clearCurrentChangeLog();
    }

    m_viInputModeManager->viEnterNormalMode();
    m_view->doc()->setUndoMergeAllEdits(false);
    emit m_view->viewModeChanged(m_view, m_view->viewMode());

    return true;
}

bool KateViModeBase::startInsertMode()
{
    m_viInputModeManager->viEnterInsertMode();
    m_view->doc()->setUndoMergeAllEdits(true);
    emit m_view->viewModeChanged(m_view, m_view->viewMode());

    return true;
}

bool KateViModeBase::startReplaceMode()
{
    m_view->doc()->setUndoMergeAllEdits(true);
    m_viInputModeManager->viEnterReplaceMode();
    emit m_view->viewModeChanged(m_view, m_view->viewMode());

    return true;
}

bool KateViModeBase::startVisualMode()
{
    if (m_viInputModeManager->getCurrentViMode() == VisualLineMode) {
        m_viInputModeManager->getViVisualMode()->setVisualModeType(VisualMode);
        m_viInputModeManager->changeViMode(VisualMode);
    } else if (m_viInputModeManager->getCurrentViMode() == VisualBlockMode) {
        m_viInputModeManager->getViVisualMode()->setVisualModeType(VisualMode);
        m_viInputModeManager->changeViMode(VisualMode);
    } else {
        m_viInputModeManager->viEnterVisualMode();
    }

    emit m_view->viewModeChanged(m_view, m_view->viewMode());

    return true;
}

bool KateViModeBase::startVisualBlockMode()
{
    if (m_viInputModeManager->getCurrentViMode() == VisualMode) {
        m_viInputModeManager->getViVisualMode()->setVisualModeType(VisualBlockMode);
        m_viInputModeManager->changeViMode(VisualBlockMode);
    } else {
        m_viInputModeManager->viEnterVisualMode(VisualBlockMode);
    }

    emit m_view->viewModeChanged(m_view, m_view->viewMode());

    return true;
}

bool KateViModeBase::startVisualLineMode()
{
    if (m_viInputModeManager->getCurrentViMode() == VisualMode) {
        m_viInputModeManager->getViVisualMode()->setVisualModeType(VisualLineMode);
        m_viInputModeManager->changeViMode(VisualLineMode);
    } else {
        m_viInputModeManager->viEnterVisualMode(VisualLineMode);
    }

    emit m_view->viewModeChanged(m_view, m_view->viewMode());

    return true;
}

void KateViModeBase::error(const QString &errorMsg)
{
    delete m_infoMessage;

    m_infoMessage = new KTextEditor::Message(errorMsg, KTextEditor::Message::Error);
    m_infoMessage->setPosition(KTextEditor::Message::BottomInView);
    m_infoMessage->setAutoHide(2000); // 2 seconds
    m_infoMessage->setView(m_view);

    m_view->doc()->postMessage(m_infoMessage);
}

void KateViModeBase::message(const QString &msg)
{
    delete m_infoMessage;

    m_infoMessage = new KTextEditor::Message(msg, KTextEditor::Message::Positive);
    m_infoMessage->setPosition(KTextEditor::Message::BottomInView);
    m_infoMessage->setAutoHide(2000); // 2 seconds
    m_infoMessage->setView(m_view);

    m_view->doc()->postMessage(m_infoMessage);
}

QString KateViModeBase::getVerbatimKeys() const
{
    return m_keysVerbatim;
}

const QChar KateViModeBase::getCharAtVirtualColumn(const QString &line, int virtualColumn,
        int tabWidth) const
{
    int column = 0;
    int tempCol = 0;

    // sanity check: if the line is empty, there are no chars
    if (line.length() == 0) {
        return QChar::Null;
    }

    while (tempCol < virtualColumn) {
        if (line.at(column) == QLatin1Char('\t')) {
            tempCol += tabWidth - (tempCol % tabWidth);
        } else {
            tempCol++;
        }

        if (tempCol <= virtualColumn) {
            column++;

            if (column >= line.length()) {
                return QChar::Null;
            }
        }
    }

    if (line.length() > column) {
        return line.at(column);
    }

    return QChar::Null;
}

void KateViModeBase::addToNumberUnderCursor(int count)
{
    KTextEditor::Cursor c(m_view->cursorPosition());
    QString line = getLine();

    if (line.isEmpty()) {
        return;
    }

    int numberStartPos = -1;
    QString numberAsString;
    QRegExp numberRegex(QLatin1String("(0x)([0-9a-fA-F]+)|\\-?\\d+"));
    const int cursorColumn = c.column();
    const int currentLineLength = doc()->lineLength(c.line());
    const KTextEditor::Cursor prevWordStart = findPrevWordStart(c.line(), cursorColumn);
    int wordStartPos = prevWordStart.column();
    if (prevWordStart.line() < c.line()) {
        // The previous word starts on the previous line: ignore.
        wordStartPos = 0;
    }
    if (wordStartPos > 0 && line.at(wordStartPos - 1) == QLatin1Char('-')) {
        wordStartPos--;
    }
    for (int searchFromColumn = wordStartPos; searchFromColumn < currentLineLength; searchFromColumn++) {
        numberStartPos = numberRegex.indexIn(line, searchFromColumn);

        numberAsString = numberRegex.cap();

        const bool numberEndedBeforeCursor = (numberStartPos + numberAsString.length() <= c.column());
        if (!numberEndedBeforeCursor) {
            // This is the first number-like string under or after the cursor - this'll do!
            break;
        }
    }

    if (numberStartPos == -1) {
        // None found.
        return;
    }

    bool parsedNumberSuccessfully = false;
    int base = numberRegex.cap(1).isEmpty() ? 10 : 16;
    if (base != 16 && numberAsString.startsWith(QLatin1String("0")) && numberAsString.length() > 1) {
        // If a non-hex number with a leading 0 can be parsed as octal, then assume
        // it is octal.
        numberAsString.toInt(&parsedNumberSuccessfully, 8);
        if (parsedNumberSuccessfully) {
            base = 8;
        }
    }
    const int originalNumber = numberAsString.toInt(&parsedNumberSuccessfully, base);

    qCDebug(LOG_PART) << "base: " << base;
    qCDebug(LOG_PART) << "n: " << originalNumber;

    if (!parsedNumberSuccessfully) {
        // conversion to int failed. give up.
        return;
    }

    QString basePrefix;
    if (base == 16) {
        basePrefix = QLatin1String("0x");
    } else if (base == 8) {
        basePrefix = QLatin1String("0");
    }
    const QString withoutBase = numberAsString.mid(basePrefix.length());

    const int newNumber = originalNumber + count;

    // Create the new text string to be inserted. Prepend with “0x” if in base 16, and "0" if base 8.
    // For non-decimal numbers, try to keep the length of the number the same (including leading 0's).
    const QString newNumberPadded = (base == 10) ?
                                    QString::fromLatin1("%1").arg(newNumber, 0, base) :
                                    QString::fromLatin1("%1").arg(newNumber, withoutBase.length(), base, QLatin1Char('0'));
    const QString newNumberText = basePrefix + newNumberPadded;

    // Replace the old number string with the new.
    doc()->editStart();
    doc()->removeText(KTextEditor::Range(c.line(), numberStartPos, c.line(), numberStartPos + numberAsString.length()));
    doc()->insertText(KTextEditor::Cursor(c.line(), numberStartPos), newNumberText);
    doc()->editEnd();
    updateCursor(KTextEditor::Cursor(m_view->cursorPosition().line(), numberStartPos + newNumberText.length() - 1));
}

void KateViModeBase::switchView(Direction direction)
{

    QList<KTextEditor::ViewPrivate *> visible_views;
    foreach (KTextEditor::ViewPrivate *view,  KTextEditor::EditorPrivate::self()->views()) {
        if (view->isVisible()) {
            visible_views.push_back(view);
        }
    }

    QPoint current_point = m_view->mapToGlobal(m_view->pos());
    int curr_x1 = current_point.x();
    int curr_x2 = current_point.x() + m_view->width();
    int curr_y1 = current_point.y();
    int curr_y2 = current_point.y() + m_view->height();
    int curr_cursor_y = m_view->mapToGlobal(m_view->cursorToCoordinate(m_view->cursorPosition())).y();
    int curr_cursor_x = m_view->mapToGlobal(m_view->cursorToCoordinate(m_view->cursorPosition())).x();

    KTextEditor::ViewPrivate *bestview = NULL;
    int  best_x1 = -1, best_x2 = -1, best_y1 = -1, best_y2 = -1, best_center_y = -1, best_center_x = -1;

    if (direction == Next && visible_views.count() != 1) {
        for (int i = 0; i < visible_views.count(); i++) {
            if (visible_views.at(i) == m_view) {
                if (i != visible_views.count() - 1) {
                    bestview = visible_views.at(i + 1);
                } else {
                    bestview = visible_views.at(0);
                }
            }
        }
    } else {
        foreach (KTextEditor::ViewPrivate *view, visible_views) {
            QPoint point = view->mapToGlobal(view->pos());
            int x1 = point.x();
            int x2 = point.x() + view->width();
            int y1 = point.y();
            int y2 = point.y() + m_view->height();
            int  center_y = (y1 + y2) / 2;
            int  center_x = (x1 + x2) / 2;

            switch (direction) {
            case Left:
                if (view != m_view && x2 <= curr_x1 &&
                        (x2 > best_x2 ||
                         (x2 == best_x2 && qAbs(curr_cursor_y - center_y) < qAbs(curr_cursor_y - best_center_y)) ||
                         bestview == NULL)) {
                    bestview = view;
                    best_x2 = x2;
                    best_center_y = center_y;
                }
                break;
            case Right:
                if (view != m_view && x1 >= curr_x2 &&
                        (x1 < best_x1 ||
                         (x1 == best_x1 && qAbs(curr_cursor_y - center_y) < qAbs(curr_cursor_y - best_center_y)) ||
                         bestview == NULL)) {
                    bestview = view;
                    best_x1 = x1;
                    best_center_y = center_y;
                }
                break;
            case Down:
                if (view != m_view && y1 >= curr_y2 &&
                        (y1 < best_y1 ||
                         (y1 == best_y1 && qAbs(curr_cursor_x - center_x) < qAbs(curr_cursor_x - best_center_x)) ||
                         bestview == NULL)) {
                    bestview = view;
                    best_y1 = y1;
                    best_center_x = center_x;
                }
            case Up:
                if (view != m_view && y2 <= curr_y1 &&
                        (y2 > best_y2 ||
                         (y2 == best_y2 && qAbs(curr_cursor_x - center_x) < qAbs(curr_cursor_x - best_center_x)) ||
                         bestview == NULL)) {
                    bestview = view;
                    best_y2 = y2;
                    best_center_x = center_x;
                }
                break;
            default:
                return;
            }

        }
    }
    if (bestview != NULL) {
        bestview->setFocus();
        bestview->setInputMode(KTextEditor::View::ViInputMode);
    }
}

ViRange KateViModeBase::motionFindPrev()
{
    return m_viInputModeManager->searcher()->motionFindPrev(getCount());
}

ViRange KateViModeBase::motionFindNext()
{
    return m_viInputModeManager->searcher()->motionFindNext(getCount());
}

void KateViModeBase::goToPos(const ViRange &r)
{
    KTextEditor::Cursor c;
    c.setLine(r.endLine);
    c.setColumn(r.endColumn);

    if (r.jump) {
        m_viInputModeManager->jumps()->add(m_view->cursorPosition());
    }

    if (c.line() >= doc()->lines()) {
        c.setLine(doc()->lines() - 1);
    }

    updateCursor(c);
}

unsigned int KateViModeBase::linesDisplayed() const
{
    return m_viInputModeManager->inputAdapter()->linesDisplayed();
}

void KateViModeBase::scrollViewLines(int l)
{
    m_viInputModeManager->inputAdapter()->scrollViewLines(l);
}

unsigned int KateViModeBase::getCount() const
{
    if (m_oneTimeCountOverride != -1) {
        return m_oneTimeCountOverride;
    }
    return (m_count > 0) ? m_count : 1;
}
