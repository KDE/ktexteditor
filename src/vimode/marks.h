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

#ifndef KATE_VIMODE_MARKS_H
#define KATE_VIMODE_MARKS_H

#include "KTextEditor/MarkInterface"

#include <KConfigGroup>

#include <QMap>

namespace KTextEditor {
    class DocumentPrivate;
    class Cursor;
    class MovingCursor;
}

namespace KateVi
{
class InputModeManager;

class Marks : public QObject
{
    Q_OBJECT

public:
    explicit Marks(InputModeManager *imm);
    ~Marks();

    /** JBOS == Just a Bunch Of Shortcuts **/
    void setStartEditYanked(const KTextEditor::Cursor &pos);
    void setFinishEditYanked(const KTextEditor::Cursor &pos);
    void setLastChange(const KTextEditor::Cursor &pos);
    void setInsertStopped(const KTextEditor::Cursor &pos);
    void setSelectionStart(const KTextEditor::Cursor &pos);
    void setSelectionFinish(const KTextEditor::Cursor &pos);
    void setUserMark(const QChar &mark, const KTextEditor::Cursor &pos);

    KTextEditor::Cursor getStartEditYanked() const;
    KTextEditor::Cursor getFinishEditYanked() const;
    KTextEditor::Cursor getLastChange() const;
    KTextEditor::Cursor getInsertStopped() const;
    KTextEditor::Cursor getSelectionStart() const;
    KTextEditor::Cursor getSelectionFinish() const;
    KTextEditor::Cursor getMarkPosition(const QChar &mark) const;

    void writeSessionConfig(KConfigGroup &config) const;
    void readSessionConfig(const KConfigGroup &config);

    QString getMarksOnTheLine(int line) const;

private:
    void syncViMarksAndBookmarks();
    bool isShowable(const QChar &mark);

    void setMark(const QChar &mark, const KTextEditor::Cursor &pos);

private Q_SLOTS:
    void markChanged(KTextEditor::Document *doc,
                     KTextEditor::Mark mark,
                     KTextEditor::MarkInterface::MarkChangeAction action);

private:
    InputModeManager *m_inputModeManager;
    KTextEditor::DocumentPrivate *m_doc;

    QMap<QChar, KTextEditor::MovingCursor *> m_marks;
    bool m_settingMark;
};
}

#endif // KATE_VIMODE_MARKS_H
