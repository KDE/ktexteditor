/* This file is part of the KDE libraries

   Copyright (C) 2018 Dominik Haumann <dhaumann@kde.org>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 2.1 of the License, or (at your option) version 3, or any
   later version accepted by the membership of KDE e.V. (or its
   successor approved by the membership of KDE e.V.), which shall
   act as a proxy defined in Section 6 of version 3 of the license.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public
   License along with this library.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "inlinenote_test.h"
#include "moc_inlinenote_test.cpp"

#include <kateglobal.h>
#include <katedocument.h>
#include <kateview.h>
#include <ktexteditor/inlinenoteinterface.h>
#include <ktexteditor/inlinenoteprovider.h>
#include <ktexteditor/inlinenote.h>

#include <QtTestWidgets>
#include <QTemporaryFile>
#include <QPainter>

using namespace KTextEditor;

QTEST_MAIN(InlineNoteTest)

namespace
{
    QWidget *findViewInternal(KTextEditor::View* view)
    {
        foreach (QObject* child, view->children()) {
            if (child->metaObject()->className() == QByteArrayLiteral("KateViewInternal")) {
                return qobject_cast<QWidget*>(child);
            }
        }
        return nullptr;
    }

    class NoteProvider : public InlineNoteProvider
    {
    public:
        QVector<int> inlineNotes(int line) const override
        {
            if (line == 0) {
                return { 5, 10 };
            }

            return {};
        }

        QSize inlineNoteSize(const InlineNote& note) const override
        {
            if (note.position().column() == 5) {
                const auto xWidth = QFontMetrics(note.font()).width(QStringLiteral("x"));
                return QSize(xWidth, note.lineHeight());
            } else if (note.position().column() == 10) {
                return QSize(note.lineHeight(), note.lineHeight());
            }

            return QSize();
        }

        void paintInlineNote(const InlineNote& note, QPainter& painter) const override
        {
            if (note.position().column() == 5) {
                painter.setPen(Qt::darkGreen);
                painter.setBrush(Qt::green);
                painter.drawEllipse(1, 1, note.width() - 2, note.lineHeight() - 2);
            } else if (note.position().column() == 10) {
                painter.setPen(Qt::darkRed);
                painter.setBrush(Qt::red);
                painter.drawRoundedRect(1, 1, note.width() - 2, note.lineHeight() - 2, 2, 2);
            }
        }

        void inlineNoteActivated(const InlineNote& note, Qt::MouseButtons buttons, const QPoint& globalPos) override
        {
            Q_UNUSED(note)
            Q_UNUSED(buttons)
            Q_UNUSED(globalPos)
            ++noteActivatedCount;
        }

        void inlineNoteFocusInEvent(const InlineNote& note, const QPoint& globalPos) override
        {
            Q_UNUSED(note)
            Q_UNUSED(globalPos)
            ++focusInCount;
        }

        void inlineNoteFocusOutEvent(const InlineNote& note) override
        {
            Q_UNUSED(note)
            ++focusOutCount;
        }

        void inlineNoteMouseMoveEvent(const InlineNote& note, const QPoint& globalPos) override
        {
            Q_UNUSED(note)
            Q_UNUSED(globalPos)
            ++mouseMoveCount;
        }

    public:
        int noteActivatedCount = 0;
        int focusInCount = 0;
        int focusOutCount = 0;
        int mouseMoveCount = 0;
    };
}

InlineNoteTest::InlineNoteTest()
    : QObject()
{
    KTextEditor::EditorPrivate::enableUnitTestMode();
}

InlineNoteTest::~InlineNoteTest()
{
}

void InlineNoteTest::testInlineNote()
{
    KTextEditor::DocumentPrivate doc;
    doc.setText(QLatin1String("xxxxxxxxxx\nxxxxxxxxxx"));

    KTextEditor::ViewPrivate view(&doc, nullptr);
    view.show();
    view.setCursorPosition({ 0, 5 });
    QCOMPARE(view.cursorPosition(), Cursor(0, 5));

    const auto coordCol04 = view.cursorToCoordinate({ 0, 4 });
    const auto coordCol05 = view.cursorToCoordinate({ 0, 5 });
    const auto coordCol10 = view.cursorToCoordinate({ 0, 10 });
    QVERIFY(coordCol05.x() > coordCol04.x());
    QVERIFY(coordCol10.x() > coordCol05.x());

    const auto xWidth = coordCol05.x() - coordCol04.x();

    auto iface = qobject_cast<KTextEditor::InlineNoteInterface*>(&view);
    QVERIFY(iface != nullptr);

    NoteProvider noteProvider;
    const QVector<int> expectedColumns = { 5, 10 };
    QCOMPARE(noteProvider.inlineNotes(0), expectedColumns);
    QCOMPARE(noteProvider.inlineNotes(1), QVector<int>());
    iface->registerInlineNoteProvider(&noteProvider);

    QTest::qWait(1000);

    const auto newCoordCol04 = view.cursorToCoordinate({ 0, 4 });
    const auto newCoordCol05 = view.cursorToCoordinate({ 0, 5 });
    const auto newCoordCol10 = view.cursorToCoordinate({ 0, 10 });

    QVERIFY(newCoordCol05.x() > newCoordCol04.x());
    QVERIFY(newCoordCol10.x() > newCoordCol05.x());

    QCOMPARE(newCoordCol04, coordCol04);
    QVERIFY(newCoordCol05.x() > coordCol05.x());
    QVERIFY(newCoordCol10.x() > coordCol10.x());

    // so far, we should not have any activation event
    QCOMPARE(noteProvider.noteActivatedCount, 0);
    QCOMPARE(noteProvider.focusInCount, 0);
    QCOMPARE(noteProvider.focusOutCount, 0);
    QCOMPARE(noteProvider.mouseMoveCount, 0);

    // move mouse onto first note
    auto internalView = findViewInternal(&view);
    QVERIFY(internalView);

    // focus in
    QTest::mouseMove(&view, coordCol05 + QPoint(xWidth / 2, 1));
    QTest::qWait(100);
    QCOMPARE(noteProvider.focusInCount, 1);
    QCOMPARE(noteProvider.focusOutCount, 0);
    QCOMPARE(noteProvider.mouseMoveCount, 0);
    QCOMPARE(noteProvider.noteActivatedCount, 0);

    // move one pixel
    QTest::mouseMove(&view, coordCol05 + QPoint(xWidth / 2 + 1, 1));
    QTest::qWait(100);
    QCOMPARE(noteProvider.focusInCount, 1);
    QCOMPARE(noteProvider.focusOutCount, 0);
    QCOMPARE(noteProvider.mouseMoveCount, 1);
    QCOMPARE(noteProvider.noteActivatedCount, 0);

    // activate
    QTest::mousePress(internalView, Qt::LeftButton, Qt::NoModifier, internalView->mapFromGlobal(view.mapToGlobal(coordCol05 + QPoint(xWidth / 2 + 1, 1))));
    QTest::mouseRelease(internalView, Qt::LeftButton, Qt::NoModifier, internalView->mapFromGlobal(view.mapToGlobal(coordCol05 + QPoint(xWidth / 2 + 1, 1))));
    QTest::qWait(100);
    QCOMPARE(noteProvider.focusInCount, 1);
    QCOMPARE(noteProvider.focusOutCount, 0);
    QCOMPARE(noteProvider.mouseMoveCount, 1);
    QCOMPARE(noteProvider.noteActivatedCount, 1);

    // focus out
    QTest::mouseMove(&view, coordCol04 + QPoint(0, 1));
    QTest::mouseMove(&view, coordCol04 + QPoint(-1, 1));
    QTest::qWait(200);
    QCOMPARE(noteProvider.focusInCount, 1);
    QCOMPARE(noteProvider.focusOutCount, 1);
    QCOMPARE(noteProvider.mouseMoveCount, 1);
    QCOMPARE(noteProvider.noteActivatedCount, 1);

    iface->unregisterInlineNoteProvider(&noteProvider);
}

// kate: indent-mode cstyle; indent-width 4; replace-tabs on;
