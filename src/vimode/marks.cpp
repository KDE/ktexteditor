/* This file is part of the KDE libraries
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

#include "marks.h"
#include "kateview.h"
#include "katedocument.h"
#include "kateviinputmodemanager.h"

#include <KLocalizedString>

using namespace KateVi;

namespace {
    const QChar BeginEditYanked = QLatin1Char('[');
    const QChar EndEditYanked = QLatin1Char(']');
    const QChar LastChange = QLatin1Char('.');
    const QChar InsertStopped = QLatin1Char('^');
    const QChar SelectionBegin = QLatin1Char('<');
    const QChar SelectionEnd = QLatin1Char('>');
    const QChar FirstUserMark = QLatin1Char('a');
    const QChar LastUserMark = QLatin1Char('z');
    const QString UserMarks = QLatin1String("abcdefghijklmnopqrstuvwxyz");
}

Marks::Marks(KateViInputModeManager *imm)
    : m_inputModeManager(imm)
    , m_doc(imm->view()->doc())
    , m_settingMark(false)
{
    connect(m_doc, &KTextEditor::DocumentPrivate::markChanged, this, &Marks::markChanged);
}

Marks::~Marks()
{
}

void Marks::readSessionConfig(const KConfigGroup &config)
{
    QStringList marks = config.readEntry("ViMarks", QStringList());
    for (int i = 0; i + 2 < marks.size(); i += 3) {
        KTextEditor::Cursor c(marks.at(i + 1).toInt(), marks.at(i + 2).toInt());
        setMark(marks.at(i).at(0), c);
    }

    syncViMarksAndBookmarks();
}

void Marks::writeSessionConfig(KConfigGroup &config)
{
    QStringList l;
    Q_FOREACH (QChar key, m_marks.keys()) {
        l << key << QString::number(m_marks.value(key)->line())
        << QString::number(m_marks.value(key)->column());
    }
    config.writeEntry("ViMarks", l);
}

void Marks::setMark(const QChar &mark, const KTextEditor::Cursor &pos, const bool moveoninsert)
{
    m_settingMark = true;
    uint marktype = m_doc->mark(pos.line());

    // delete old cursor if any
    if (KTextEditor::MovingCursor *oldCursor = m_marks.value(mark)) {

        int  number_of_marks = 0;

        foreach (QChar c, m_marks.keys()) {
            if (m_marks.value(c)->line() ==  oldCursor->line()) {
                number_of_marks++;
            }
        }

        if (number_of_marks == 1 && pos.line() != oldCursor->line()) {
            m_doc->removeMark(oldCursor->line(), KTextEditor::MarkInterface::markType01);
        }

        delete oldCursor;
    }

    KTextEditor::MovingCursor::InsertBehavior behavior = moveoninsert ? KTextEditor::MovingCursor::MoveOnInsert : KTextEditor::MovingCursor::StayOnInsert;
    // create and remember new one
    m_marks.insert(mark, m_doc->newMovingCursor(pos, behavior));

    // Showing what mark we set:
    if (isShowable(mark)) {
        if (!marktype & KTextEditor::MarkInterface::markType01) {
            m_doc->addMark(pos.line(), KTextEditor::MarkInterface::markType01);
        }

        // only show message for active view
        if (m_doc->activeView() == m_inputModeManager->view()) {
            m_inputModeManager->message(i18n("Mark set: %1", mark));
        }
    }

    m_settingMark = false;
}

KTextEditor::Cursor Marks::getMarkPosition(const QChar &mark) const
{
    if (m_marks.contains(mark)) {
        KTextEditor::MovingCursor *c = m_marks.value(mark);
        return KTextEditor::Cursor(c->line(), c->column());
    }

    return KTextEditor::Cursor::invalid();
}

void Marks::markChanged(KTextEditor::Document *doc,
                        KTextEditor::Mark mark,
                        KTextEditor::MarkInterface::MarkChangeAction action)
{

    Q_UNUSED(doc)

    if (mark.type != KTextEditor::MarkInterface::Bookmark || m_settingMark) {
        return;
    }

    if (action == KTextEditor::MarkInterface::MarkRemoved) {
        foreach (QChar markerChar, m_marks.keys()) {
            if (m_marks.value(markerChar)->line() == mark.line) {
                m_marks.remove(markerChar);
            }
        }
    } else if (action == KTextEditor::MarkInterface::MarkAdded) {
        bool freeMarkerCharFound = false;

        Q_FOREACH(const QChar &markerChar, UserMarks) {
            if (!m_marks.value(markerChar)) {
                setMark(markerChar, KTextEditor::Cursor(mark.line, 0));
                freeMarkerCharFound = true;
                break;
            }
        }

        if (!freeMarkerCharFound) {
            m_inputModeManager->error(i18n("There are no more chars for the next bookmark."));
        }
    }
}

void Marks::syncViMarksAndBookmarks()
{
    const QHash<int, KTextEditor::Mark *> &marks = m_doc->marks();

    //  Each bookmark should have a vi mark on the same line.
    for (auto mark : marks) {
        if (!(mark->type & KTextEditor::MarkInterface::markType01)) {
            continue;
        }

        bool thereIsViMarkForThisLine = false;
        Q_FOREACH (const KTextEditor::MovingCursor *cursor, m_marks) {
            if (cursor->line() == mark->line) {
                thereIsViMarkForThisLine = true;
                break;
            }
        }

        if (thereIsViMarkForThisLine) {
            continue;
        }

        Q_FOREACH (const QChar &markerChar, UserMarks) {
            if (!m_marks.value(markerChar)) {
                setMark(markerChar, KTextEditor::Cursor(mark->line, 0));
                break;
            }
        }
    }

    // For showable vi mark a line should be bookmarked.
    Q_FOREACH (const QChar &markChar, m_marks.keys()) {
        if (!isShowable(markChar)) {
            continue;
        }

        bool thereIsKateMarkForThisLine = false;
        for (auto mark : marks) {
            if (!(mark->type & KTextEditor::MarkInterface::markType01)) {
                continue;
            }

            if (m_marks.value(markChar)->line() == mark->line) {
                thereIsKateMarkForThisLine = true;
                break;
            }
        }

        if (!thereIsKateMarkForThisLine) {
            m_doc->addMark(m_marks.value(markChar)->line(), KTextEditor::MarkInterface::markType01);
        }
    }
}

QString Marks::getMarksOnTheLine(int line)
{
    QString res;

    Q_FOREACH (QChar markerChar, m_marks.keys()) {
        if (m_marks.value(markerChar)->line() == line) {
            res += markerChar + QLatin1String(":") + QString::number(m_marks.value(markerChar)->column()) + QLatin1String(" ");
        }
    }

    return res;
}

bool Marks::isShowable(const QChar &mark)
{
    return FirstUserMark <= mark && mark <= LastUserMark;
}

void Marks::setStartEditYanked(const KTextEditor::Cursor &pos)
{
    setMark(BeginEditYanked, pos, false);
}

void Marks::setFinishEditYanked(const KTextEditor::Cursor &pos)
{
    setMark(EndEditYanked, pos);
}

void Marks::setLastChange(const KTextEditor::Cursor &pos)
{
    setMark(LastChange, pos);
}

void Marks::setInsertStopped(const KTextEditor::Cursor &pos)
{
    setMark(InsertStopped, pos);
}

void Marks::setSelectionStart(const KTextEditor::Cursor &pos)
{
    setMark(SelectionBegin, pos);
}

void Marks::setSelectionFinish(const KTextEditor::Cursor &pos)
{
    setMark(SelectionEnd, pos);
}

void Marks::setUserMark(const QChar &mark, const KTextEditor::Cursor &pos)
{
    Q_ASSERT(FirstUserMark <= mark && mark <= LastUserMark);
    setMark(mark, pos);
}

KTextEditor::Cursor Marks::getStartEditYanked() const
{
    return getMarkPosition(BeginEditYanked);
}

KTextEditor::Cursor Marks::getFinishEditYanked() const
{
    return getMarkPosition(EndEditYanked);
}

KTextEditor::Cursor Marks::getSelectionStart() const
{
    return getMarkPosition(SelectionBegin);
}

KTextEditor::Cursor Marks::getSelectionFinish() const
{
    return getMarkPosition(SelectionEnd);
}

KTextEditor::Cursor Marks::getLastChange() const
{
    return getMarkPosition(LastChange);
}

KTextEditor::Cursor Marks::getInsertStopped() const
{
    return getMarkPosition(InsertStopped);
}
