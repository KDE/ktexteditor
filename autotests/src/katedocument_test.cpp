/* This file is part of the KDE libraries
   Copyright (C) 2010-2018 Dominik Haumann <dhaumann@kde.org>

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

#include "katedocument_test.h"
#include "moc_katedocument_test.cpp"

#include <katedocument.h>
#include <ktexteditor/movingcursor.h>
#include <kateconfig.h>
#include <kateview.h>
#include <kateglobal.h>

#include <QtTestWidgets>
#include <QTemporaryFile>
#include <QSignalSpy>

///TODO: is there a FindValgrind cmake command we could use to
///      define this automatically?
// comment this out and run the test case with:
//   valgrind --tool=callgrind --instr-atstart=no ./katedocument_test testSetTextPerformance
// or similar
//
// #define USE_VALGRIND

#ifdef USE_VALGRIND
#include <valgrind/callgrind.h>
#endif

using namespace KTextEditor;

QTEST_MAIN(KateDocumentTest)

class MovingRangeInvalidator : public QObject
{
    Q_OBJECT
public:
    explicit MovingRangeInvalidator(QObject *parent = nullptr)
        : QObject(parent)
    {
    }

    void addRange(MovingRange *range)
    {
        m_ranges << range;
    }
    QList<MovingRange *> ranges() const
    {
        return m_ranges;
    }

public Q_SLOTS:
    void aboutToInvalidateMovingInterfaceContent()
    {
        qDeleteAll(m_ranges);
        m_ranges.clear();
    }

private:
    QList<MovingRange *> m_ranges;
};

KateDocumentTest::KateDocumentTest()
    : QObject()
{
}

KateDocumentTest::~KateDocumentTest()
{
}

void KateDocumentTest::initTestCase()
{
    KTextEditor::EditorPrivate::enableUnitTestMode();
}

// tests:
// KTextEditor::DocumentPrivate::insertText with word wrap enabled. It is checked whether the
// text is correctly wrapped and whether the moving cursors maintain the correct
// position.
// see also: http://bugs.kde.org/show_bug.cgi?id=168534
void KateDocumentTest::testWordWrap()
{
    KTextEditor::DocumentPrivate doc(false, false);
    doc.setWordWrap(true);
    doc.setWordWrapAt(80);

    const QString content = QLatin1String(".........1.........2.........3.........4.........5.........6 ........7 ........8");

    // space after 7 is now kept
    // else we kill indentation...
    const QString firstWrap = QLatin1String(".........1.........2.........3.........4.........5.........6 ........7 \n....x....8");

    // space after 6 is now kept
    // else we kill indentation...
    const QString secondWrap = QLatin1String(".........1.........2.........3.........4.........5.........6 \n....ooooooooooo....7 ....x....8");

    doc.setText(content);
    MovingCursor *c = doc.newMovingCursor(Cursor(0, 75), MovingCursor::MoveOnInsert);

    QCOMPARE(doc.text(), content);
    QCOMPARE(c->toCursor(), Cursor(0, 75));

    // type a character at (0, 75)
    doc.insertText(c->toCursor(), QLatin1String("x"));
    QCOMPARE(doc.text(), firstWrap);
    QCOMPARE(c->toCursor(), Cursor(1, 5));

    // set cursor to (0, 65) and type "ooooooooooo"
    c->setPosition(Cursor(0, 65));
    doc.insertText(c->toCursor(), QLatin1String("ooooooooooo"));
    QCOMPARE(doc.text(), secondWrap);
    QCOMPARE(c->toCursor(), Cursor(1, 15));
}

void KateDocumentTest::testWrapParagraph()
{
    // Each paragraph must be kept as an own but re-wrapped nicely
    KTextEditor::DocumentPrivate doc(false, false);
    doc.setWordWrapAt(30); // Keep needed test data small

    const QString before = QLatin1String("aaaaa a aaaa\naaaaa aaa aa aaaa aaaa \naaaa a aaa aaaaaaa a aaaa\n\nxxxxx x\nxxxx xxxxx\nxxx xx xxxx \nxxxx xxxx x xxx xxxxxxx x xxxx");
    const QString after = QLatin1String("aaaaa a aaaa aaaaa aaa aa aaaa \naaaa aaaa a aaa aaaaaaa a aaaa\n\nxxxxx x xxxx xxxxx xxx xx xxxx \nxxxx xxxx x xxx xxxxxxx x xxxx");

    doc.setWordWrap(false); // First we try with disabled hard wrap
    doc.setText(before);
    doc.wrapParagraph(0, doc.lines() - 1);
    QCOMPARE(doc.text(), after);

    doc.setWordWrap(true); // Test again with enabled hard wrap, that had cause trouble due to twice wrapping
    doc.setText(before);
    doc.wrapParagraph(0, doc.lines() - 1);
    QCOMPARE(doc.text(), after);
}

void KateDocumentTest::testReplaceQStringList()
{
    KTextEditor::DocumentPrivate doc(false, false);
    doc.setWordWrap(false);
    doc.setText(QLatin1String("asdf\n"
                              "foo\n"
                              "foo\n"
                              "bar\n"));
    doc.replaceText(Range(1, 0, 3, 0), { "new", "text", "" }, false);
    QCOMPARE(doc.text(), QLatin1String("asdf\n"
                                       "new\n"
                                       "text\n"
                                       "bar\n"));
}

void KateDocumentTest::testMovingInterfaceSignals()
{
    KTextEditor::DocumentPrivate *doc = new KTextEditor::DocumentPrivate;
    QSignalSpy aboutToDeleteSpy(doc, SIGNAL(aboutToDeleteMovingInterfaceContent(KTextEditor::Document*)));
    QSignalSpy aboutToInvalidateSpy(doc, SIGNAL(aboutToInvalidateMovingInterfaceContent(KTextEditor::Document*)));

    QCOMPARE(doc->revision(), qint64(0));

    QCOMPARE(aboutToInvalidateSpy.count(), 0);
    QCOMPARE(aboutToDeleteSpy.count(), 0);

    QTemporaryFile f;
    f.open();
    doc->openUrl(QUrl::fromLocalFile(f.fileName()));
    QCOMPARE(doc->revision(), qint64(0));
    //TODO: gets emitted once in closeFile and once in openFile - is that OK?
    QCOMPARE(aboutToInvalidateSpy.count(), 2);
    QCOMPARE(aboutToDeleteSpy.count(), 0);

    doc->documentReload();
    QCOMPARE(doc->revision(), qint64(0));
    QCOMPARE(aboutToInvalidateSpy.count(), 4);
    //TODO: gets emitted once in closeFile and once in openFile - is that OK?
    QCOMPARE(aboutToDeleteSpy.count(), 0);

    delete doc;
    QCOMPARE(aboutToInvalidateSpy.count(), 4);
    QCOMPARE(aboutToDeleteSpy.count(), 1);
}

void KateDocumentTest::testSetTextPerformance()
{
    const int lines = 150;
    const int columns = 80;
    const int rangeLength = 4;
    const int rangeGap = 1;

    Q_ASSERT(columns % (rangeLength + rangeGap) == 0);

    KTextEditor::DocumentPrivate doc;
    MovingRangeInvalidator invalidator;
    connect(&doc, SIGNAL(aboutToInvalidateMovingInterfaceContent(KTextEditor::Document*)),
            &invalidator, SLOT(aboutToInvalidateMovingInterfaceContent()));

    QString text;
    QVector<Range> ranges;
    ranges.reserve(lines * columns / (rangeLength + rangeGap));
    const QString line = QString().fill('a', columns);
    for (int l = 0; l < lines; ++l) {
        text.append(line);
        text.append('\n');
        for (int c = 0; c < columns; c += rangeLength + rangeGap) {
            ranges << Range(l, c, l, c + rangeLength);
        }
    }

    // replace
    QBENCHMARK {
        // init
        doc.setText(text);
        foreach (const Range &range, ranges)
        {
            invalidator.addRange(doc.newMovingRange(range));
        }
        QCOMPARE(invalidator.ranges().size(), ranges.size());

#ifdef USE_VALGRIND
        CALLGRIND_START_INSTRUMENTATION
#endif

        doc.setText(text);

#ifdef USE_VALGRIND
        CALLGRIND_STOP_INSTRUMENTATION
#endif

        QCOMPARE(doc.text(), text);
        QVERIFY(invalidator.ranges().isEmpty());
    }
}

void KateDocumentTest::testRemoveTextPerformance()
{
    const int lines = 5000;
    const int columns = 80;

    KTextEditor::DocumentPrivate doc;

    QString text;
    const QString line = QString().fill('a', columns);
    for (int l = 0; l < lines; ++l) {
        text.append(line);
        text.append('\n');
    }

    doc.setText(text);

    // replace
    QBENCHMARK_ONCE {
#ifdef USE_VALGRIND
        CALLGRIND_START_INSTRUMENTATION
#endif

        doc.editStart();

        doc.removeText(doc.documentRange());

        doc.editEnd();

#ifdef USE_VALGRIND
        CALLGRIND_STOP_INSTRUMENTATION
#endif
    }
}

void KateDocumentTest::testForgivingApiUsage()
{
    KTextEditor::DocumentPrivate doc;

    QVERIFY(doc.isEmpty());
    QVERIFY(doc.replaceText(Range(0, 0, 100, 100), "asdf"));
    QCOMPARE(doc.text(), QString("asdf"));
    QCOMPARE(doc.lines(), 1);
    QVERIFY(doc.replaceText(Range(2, 5, 2, 100), "asdf"));
    QCOMPARE(doc.lines(), 3);
    QCOMPARE(doc.text(), QLatin1String("asdf\n\n     asdf"));

    QVERIFY(doc.removeText(Range(0, 0, 1000, 1000)));
    QVERIFY(doc.removeText(Range(0, 0, 0, 100)));
    QVERIFY(doc.isEmpty());
    doc.insertText(Cursor(100, 0), "foobar");
    QCOMPARE(doc.line(100), QString("foobar"));

    doc.setText("nY\nnYY\n");
    QVERIFY(doc.removeText(Range(0, 0, 0, 1000)));
}

void KateDocumentTest::testReplaceTabs()
{
    KTextEditor::DocumentPrivate doc;
    auto view = static_cast<KTextEditor::ViewPrivate*>(doc.createView(nullptr));

    auto reset = [&]() {
        doc.setText("  Hi!");
        view->setCursorPosition(Cursor(0, 0));
    };

    doc.setHighlightingMode ("C++");
    doc.config()->setTabWidth(4);
    doc.config()->setIndentationMode("cppstyle");

    // no replace tabs, no indent pasted text
    doc.setConfigValue("replace-tabs", false);
    doc.setConfigValue("indent-pasted-text", false);

    reset();
    doc.insertText(Cursor(0, 0), "\t");
    QCOMPARE(doc.text(), QStringLiteral("\t  Hi!"));

    reset();
    doc.typeChars(view, "\t");
    QCOMPARE(doc.text(), QStringLiteral("\t  Hi!"));

    reset();
    doc.paste(view, "some\ncode\n  3\nhi\n");
    QCOMPARE(doc.text(), QStringLiteral("some\ncode\n  3\nhi\n  Hi!"));

    // now, enable replace tabs
    doc.setConfigValue("replace-tabs", true);

    reset();
    doc.insertText(Cursor(0, 0), "\t");
    // calling insertText does not replace tabs
    QCOMPARE(doc.text(), QStringLiteral("\t  Hi!"));

    reset();
    doc.typeChars(view, "\t");
    // typing text replaces tabs
    QCOMPARE(doc.text(), QStringLiteral("      Hi!"));

    reset();
    doc.paste(view, "\t");
    // pasting text does not with indent-pasted off
    QCOMPARE(doc.text(), QStringLiteral("\t  Hi!"));

    doc.setConfigValue("indent-pasted-text", true);
    doc.setText("int main() {\n    \n}");
    view->setCursorPosition(Cursor(1, 4));
    doc.paste(view, "\tHi");
    // ... and it still does not with indent-pasted on.
    // This behaviour is up to discussion.
    QCOMPARE(doc.text(), QStringLiteral("int main() {\n    Hi\n}"));

    reset();
    doc.paste(view, "some\ncode\n  3\nhi");
    QCOMPARE(doc.text(), QStringLiteral("some\ncode\n3\nhi  Hi!"));
}

/**
 * Provides slots to check data sent in specific signals. Slot names are derived from corresponding test names.
 */
class SignalHandler : public QObject
{
    Q_OBJECT
public Q_SLOTS:
    void slotMultipleLinesRemoved(KTextEditor::Document *, const KTextEditor::Range &, const QString &oldText)
    {
        QCOMPARE(oldText, QString("line2\nline3\n"));
    }

    void slotNewlineInserted(KTextEditor::Document *, const KTextEditor::Range &range)
    {
        QCOMPARE(range, Range(Cursor(1, 4), Cursor(2, 0)));
    }
};

void KateDocumentTest::testRemoveMultipleLines()
{
    KTextEditor::DocumentPrivate doc;

    doc.setText("line1\n"
                "line2\n"
                "line3\n"
                "line4\n");

    SignalHandler handler;
    connect(&doc, &KTextEditor::DocumentPrivate::textRemoved, &handler, &SignalHandler::slotMultipleLinesRemoved);
    doc.removeText(Range(1, 0, 3, 0));
}

void KateDocumentTest::testInsertNewline()
{
    KTextEditor::DocumentPrivate doc;

    doc.setText("this is line\n"
                "this is line2\n");

    SignalHandler handler;
    connect(&doc, SIGNAL(textInserted(KTextEditor::Document*,KTextEditor::Range)), &handler, SLOT(slotNewlineInserted(KTextEditor::Document*,KTextEditor::Range)));
    doc.editWrapLine(1, 4);
}

void KateDocumentTest::testInsertAfterEOF()
{
    KTextEditor::DocumentPrivate doc;

    doc.setText("line0\n"
                "line1");

    const QString input = QLatin1String("line3\n"
                                        "line4");

    const QString expected = QLatin1String("line0\n"
                                           "line1\n"
                                           "\n"
                                           "line3\n"
                                           "line4");

    doc.insertText(KTextEditor::Cursor(3, 0), input);
    QCOMPARE(doc.text(), expected);
}

// we have two different ways of creating the checksum:
// in KateFileLoader and KTextEditor::DocumentPrivate::createDigest. Make
// sure, these two implementations result in the same checksum.
void KateDocumentTest::testDigest()
{
    // Git hash of test file (git hash-object data/md5checksum.txt):
    const QByteArray gitHash = "696e6d35a5d9cc28d16e56bdcb2d2a88126b814e";
    // QCryptographicHash is used, therefore we need fromHex here
    const QByteArray fileDigest = QByteArray::fromHex(gitHash);

    // make sure, Kate::TextBuffer and KTextEditor::DocumentPrivate::createDigest() equal
    KTextEditor::DocumentPrivate doc;
    doc.openUrl(QUrl::fromLocalFile(QLatin1String(TEST_DATA_DIR"md5checksum.txt")));
    const QByteArray bufferDigest(doc.checksum());
    QVERIFY(doc.createDigest());
    const QByteArray docDigest(doc.checksum());

    QCOMPARE(bufferDigest, fileDigest);
    QCOMPARE(docDigest, fileDigest);
}

void KateDocumentTest::testModelines()
{
    // honor document variable indent-width
    {
        KTextEditor::DocumentPrivate doc;
        QCOMPARE(doc.config()->indentationWidth(), 4);
        doc.readVariableLine(QStringLiteral("kate: indent-width 3;"));
        QCOMPARE(doc.config()->indentationWidth(), 3);
    }

    // honor document variable indent-width with * wildcard
    {
        KTextEditor::DocumentPrivate doc;
        QCOMPARE(doc.config()->indentationWidth(), 4);
        doc.readVariableLine(QStringLiteral("kate-wildcard(*): indent-width 3;"));
        QCOMPARE(doc.config()->indentationWidth(), 3);
    }

    // ignore document variable indent-width, since the wildcard does not match
    {
        KTextEditor::DocumentPrivate doc;
        QCOMPARE(doc.config()->indentationWidth(), 4);
        doc.readVariableLine(QStringLiteral("kate-wildcard(*.txt): indent-width 3;"));
        QCOMPARE(doc.config()->indentationWidth(), 4);
    }

    // document variable indent-width, since the wildcard does not match
    {
        KTextEditor::DocumentPrivate doc;
        doc.openUrl(QUrl::fromLocalFile(QLatin1String(TEST_DATA_DIR"modelines.txt")));
        QVERIFY(!doc.isEmpty());

        // ignore wrong wildcard
        QCOMPARE(doc.config()->indentationWidth(), 4);
        doc.readVariableLine(QStringLiteral("kate-wildcard(*.bar): indent-width 3;"));
        QCOMPARE(doc.config()->indentationWidth(), 4);

        // read correct wildcard
        QCOMPARE(doc.config()->indentationWidth(), 4);
        doc.readVariableLine(QStringLiteral("kate-wildcard(*.txt): indent-width 5;"));
        QCOMPARE(doc.config()->indentationWidth(), 5);

        // honor correct wildcard
        QCOMPARE(doc.config()->indentationWidth(), 5);
        doc.readVariableLine(QStringLiteral("kate-wildcard(*.foo;*.txt;*.bar): indent-width 6;"));
        QCOMPARE(doc.config()->indentationWidth(), 6);

        // ignore incorrect mimetype
        QCOMPARE(doc.config()->indentationWidth(), 6);
        doc.readVariableLine(QStringLiteral("kate-mimetype(text/unknown): indent-width 7;"));
        QCOMPARE(doc.config()->indentationWidth(), 6);

        // honor correct mimetype
        QCOMPARE(doc.config()->indentationWidth(), 6);
        doc.readVariableLine(QStringLiteral("kate-mimetype(text/plain): indent-width 8;"));
        QCOMPARE(doc.config()->indentationWidth(), 8);

        // honor correct mimetype
        QCOMPARE(doc.config()->indentationWidth(), 8);
        doc.readVariableLine(QStringLiteral("kate-mimetype(text/foo;text/plain;text/bar): indent-width 9;"));
        QCOMPARE(doc.config()->indentationWidth(), 9);
    }
}

void KateDocumentTest::testDefStyleNum()
{
    KTextEditor::DocumentPrivate doc;
    doc.setText("foo\nbar\nasdf");
    QCOMPARE(doc.defStyleNum(0, 0), 0);
}

void KateDocumentTest::testTypeCharsWithSurrogateAndNewLine()
{
    KTextEditor::DocumentPrivate doc;
    auto view = static_cast<KTextEditor::ViewPrivate*>(doc.createView(nullptr));
    const uint surrogateUcs4String[] = { 0x1f346, '\n', 0x1f346, 0 };
    const auto surrogateString = QString::fromUcs4(surrogateUcs4String);
    doc.typeChars(view, surrogateString);

    QCOMPARE(doc.text(), surrogateString);
}

void KateDocumentTest::testRemoveComposedCharacters()
{
    KTextEditor::DocumentPrivate doc;
    auto view = static_cast<KTextEditor::ViewPrivate*>(doc.createView(nullptr));
    view->config()->setBackspaceRemoveComposed(true);
    doc.setText(QString::fromUtf8("व्यक्तियों"));
    doc.del(view, Cursor(0, 0));

    QCOMPARE(doc.text(), QString::fromUtf8(("क्तियों")));

    doc.backspace(view, Cursor(0, 7));

    QCOMPARE(doc.text(), QString::fromUtf8(("क्ति")));
}

#include "katedocument_test.moc"
