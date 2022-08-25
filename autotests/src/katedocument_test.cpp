/*
    This file is part of the KDE libraries
    SPDX-FileCopyrightText: 2010-2018 Dominik Haumann <dhaumann@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "katedocument_test.h"
#include "moc_katedocument_test.cpp"

#include <kateconfig.h>
#include <katedocument.h>
#include <kateglobal.h>
#include <kateview.h>
#include <ktexteditor/movingcursor.h>

#include <QRegularExpression>
#include <QSignalSpy>
#include <QTemporaryFile>
#include <QtTestWidgets>

#include <stdio.h>

/// TODO: is there a FindValgrind cmake command we could use to
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
// see also: https://bugs.kde.org/show_bug.cgi?id=168534
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

    const QString before =
        QLatin1String("aaaaa a aaaa\naaaaa aaa aa aaaa aaaa \naaaa a aaa aaaaaaa a aaaa\n\nxxxxx x\nxxxx xxxxx\nxxx xx xxxx \nxxxx xxxx x xxx xxxxxxx x xxxx");
    const QString after =
        QLatin1String("aaaaa a aaaa aaaaa aaa aa aaaa \naaaa aaaa a aaa aaaaaaa a aaaa\n\nxxxxx x xxxx xxxxx xxx xx xxxx \nxxxx xxxx x xxx xxxxxxx x xxxx");

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
    doc.setText(
        QLatin1String("asdf\n"
                      "foo\n"
                      "foo\n"
                      "bar\n"));
    doc.replaceText(Range(1, 0, 3, 0), {"new", "text", ""}, false);
    QCOMPARE(doc.text(),
             QLatin1String("asdf\n"
                           "new\n"
                           "text\n"
                           "bar\n"));
}

void KateDocumentTest::testMovingInterfaceSignals()
{
    KTextEditor::DocumentPrivate *doc = new KTextEditor::DocumentPrivate;
    QSignalSpy aboutToDeleteSpy(doc, &KTextEditor::DocumentPrivate::aboutToDeleteMovingInterfaceContent);
    QSignalSpy aboutToInvalidateSpy(doc, &KTextEditor::DocumentPrivate::aboutToInvalidateMovingInterfaceContent);

    QCOMPARE(doc->revision(), qint64(0));

    QCOMPARE(aboutToInvalidateSpy.count(), 0);
    QCOMPARE(aboutToDeleteSpy.count(), 0);

    QTemporaryFile f;
    f.open();
    doc->openUrl(QUrl::fromLocalFile(f.fileName()));
    QCOMPARE(doc->revision(), qint64(0));
    // TODO: gets emitted once in closeFile and once in openFile - is that OK?
    QCOMPARE(aboutToInvalidateSpy.count(), 2);
    QCOMPARE(aboutToDeleteSpy.count(), 0);

    doc->documentReload();
    QCOMPARE(doc->revision(), qint64(0));
    QCOMPARE(aboutToInvalidateSpy.count(), 4);
    // TODO: gets emitted once in closeFile and once in openFile - is that OK?
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
    connect(&doc,
            &KTextEditor::DocumentPrivate::aboutToInvalidateMovingInterfaceContent,
            &invalidator,
            &MovingRangeInvalidator::aboutToInvalidateMovingInterfaceContent);

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
        for (const Range &range : std::as_const(ranges)) {
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

void KateDocumentTest::testAutoBrackets()
{
    KTextEditor::DocumentPrivate doc;
    auto view = static_cast<KTextEditor::ViewPrivate *>(doc.createView(nullptr));

    auto reset = [&]() {
        doc.setText("");
        view->setCursorPosition(Cursor(0, 0));
    };

    auto typeText = [&](const QString &text) {
        for (int i = 0; i < text.size(); ++i) {
            doc.typeChars(view, text.at(i));
        }
    };

    doc.setHighlightingMode("Normal"); // Just to be sure
    view->config()->setValue(KateViewConfig::AutoBrackets, true);

    QString testInput;

    testInput = ("(");
    typeText(testInput);
    QCOMPARE(doc.text(), "()");

    reset();
    testInput = ("\"");
    typeText(testInput);
    QCOMPARE(doc.text(), "\"\"");

    reset();
    testInput = ("'");
    typeText(testInput);
    QCOMPARE(doc.text(), "'"); // In Normal mode there is only one quote to expect

    //
    // Switch over to some other mode
    //
    doc.setHighlightingMode("C++");

    reset();
    typeText(testInput);
    QCOMPARE(doc.text(), "''"); // Now it must be two

    reset();
    testInput = "('')";
    typeText(testInput);
    // Known bad behaviour in case of nested brackets
    QCOMPARE(doc.text(), testInput);

    reset();
    testInput = ("foo \"bar\" haz");
    typeText(testInput);
    QCOMPARE(doc.text(), testInput);
    // Simulate afterwards to add quotes, bug 405089
    doc.setText("foo \"bar");
    typeText("\" haz");
    QCOMPARE(doc.text(), testInput);

    // Let's check to add brackets to a selection...
    view->setBlockSelection(false);
    doc.setText("012xxx678");
    view->setSelection(Range(0, 3, 0, 6));
    typeText("[");
    QCOMPARE(doc.text(), "012[xxx]678");
    QCOMPARE(view->selectionRange(), Range(0, 4, 0, 7));

    // ...over multiple lines..
    doc.setText("012xxx678\n012xxx678");
    view->setSelection(Range(0, 3, 1, 6));
    typeText("[");
    QCOMPARE(doc.text(), "012[xxx678\n012xxx]678");
    QCOMPARE(view->selectionRange(), Range(0, 4, 1, 6));

    // ..once again in in block mode with increased complexity..
    view->setBlockSelection(true);
    doc.setText("012xxx678\n012xxx678");
    view->setSelection(Range(0, 3, 1, 6));
    typeText("[(");
    QCOMPARE(doc.text(), "012[(xxx)]678\n012[(xxx)]678");
    QCOMPARE(view->selectionRange(), Range(0, 5, 1, 8));

    // ..and the same with right->left selection
    view->setBlockSelection(true);
    doc.setText("012xxx678\n012xxx678");
    view->setSelection(Range(0, 6, 1, 3));
    typeText("[(");
    QCOMPARE(doc.text(), "012[(xxx)]678\n012[(xxx)]678");
    QCOMPARE(view->selectionRange(), Range(0, 8, 1, 5));
}

void KateDocumentTest::testReplaceTabs()
{
    KTextEditor::DocumentPrivate doc;
    auto view = static_cast<KTextEditor::ViewPrivate *>(doc.createView(nullptr));

    auto reset = [&]() {
        doc.setText("  Hi!");
        view->setCursorPosition(Cursor(0, 0));
    };

    doc.setHighlightingMode("C++");
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
    // \t survives as we don't indent in the given case anymore, see 077dfe954699c674d2c34caf380199a4af7d184a
    QCOMPARE(doc.text(), QStringLiteral("int main() {\n    \tHi\n}"));

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

    doc.setText(
        "line1\n"
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

    doc.setText(
        "this is line\n"
        "this is line2\n");

    SignalHandler handler;
    connect(&doc, &KTextEditor::DocumentPrivate::textInsertedRange, &handler, &SignalHandler::slotNewlineInserted);
    doc.editWrapLine(1, 4);
}

void KateDocumentTest::testInsertAfterEOF()
{
    KTextEditor::DocumentPrivate doc;

    doc.setText(
        "line0\n"
        "line1");

    const QString input = QLatin1String(
        "line3\n"
        "line4");

    const QString expected = QLatin1String(
        "line0\n"
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
    // we will write the test file here to avoid that any line ending conversion for git will break it
    const QByteArray fileDigest = "aa22605da164a4e4e55f4c9738cfe1e53d4467f9";
    QTemporaryFile file("testDigest");
    file.open();
    file.write("974d9ab0860c755a4f5686b3b6b429e1efd48a96\ntest\ntest\n\r\n\r\n\r\n");
    file.flush();

    // make sure, Kate::TextBuffer and KTextEditor::DocumentPrivate::createDigest() equal
    KTextEditor::DocumentPrivate doc;
    doc.openUrl(QUrl::fromLocalFile(file.fileName()));
    const QByteArray bufferDigest(doc.checksum().toHex());
    QVERIFY(doc.createDigest());
    const QByteArray docDigest(doc.checksum().toHex());

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
        doc.openUrl(QUrl::fromLocalFile(QLatin1String(TEST_DATA_DIR "modelines.txt")));
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
    auto view = static_cast<KTextEditor::ViewPrivate *>(doc.createView(nullptr));
    const char32_t surrogateUcs4String[] = {0x1f346, '\n', 0x1f346, 0};
    const auto surrogateString = QString::fromUcs4(surrogateUcs4String);
    doc.typeChars(view, surrogateString);

    QCOMPARE(doc.text(), surrogateString);
}

void KateDocumentTest::testRemoveComposedCharacters()
{
    KTextEditor::DocumentPrivate doc;
    auto view = static_cast<KTextEditor::ViewPrivate *>(doc.createView(nullptr));
    view->config()->setValue(KateViewConfig::BackspaceRemoveComposedCharacters, true);
    doc.setText(QString::fromUtf8("व्यक्तियों"));
    doc.del(view, Cursor(0, 0));

    QCOMPARE(doc.text(), QString::fromUtf8(("क्तियों")));

    view->setCursorPosition({0, 7});
    doc.backspace(view);

    QCOMPARE(doc.text(), QString::fromUtf8(("क्ति")));
}

void KateDocumentTest::testAutoReload()
{
    // ATM fails on Windows, mark as such to be able to enforce test success in CI
#ifdef Q_OS_WIN
    QSKIP("Fails ATM, please fix");
#endif

    QTemporaryFile file("AutoReloadTestFile");
    file.open();
    file.write("Hello");
    file.flush();

    KTextEditor::DocumentPrivate doc;
    auto view = static_cast<KTextEditor::ViewPrivate *>(doc.createView(nullptr));
    QVERIFY(doc.openUrl(QUrl::fromLocalFile(file.fileName())));
    QCOMPARE(doc.text(), "Hello");
    // The cursor should be and stay in the last line...
    QCOMPARE(view->cursorPosition().line(), doc.documentEnd().line());

    doc.autoReloadToggled(true);

    // Some magic value. You can wait as long as you like after write to file,
    // without to wait before it doesn't work here. It mostly fails with 500,
    // it tend to work with 600, but is not guarantied to work even with 800
    QTest::qWait(1000);

    file.write("\nTest");
    file.flush();

    // Hardcoded delay m_modOnHdTimer in DocumentPrivate
    // + min val with which it looks working reliable here
    QTest::qWait(1000);
    QCOMPARE(doc.text(), "Hello\nTest");
    // ...stay in the last line after reload!
    QCOMPARE(view->cursorPosition().line(), doc.documentEnd().line());

    // Now we have more then one line, set the cursor to some line != last line...
    Cursor c(0, 3);
    view->setCursorPosition(c);

    file.write("\nWorld!");
    file.flush();

    QTest::qWait(1000);
    QCOMPARE(doc.text(), "Hello\nTest\nWorld!");
    // ...and ensure we have not move around
    QCOMPARE(view->cursorPosition(), c);
}

void KateDocumentTest::testSearch()
{
    /**
     * this is the start of some new implementation of searchText that can handle multi-line regex stuff
     * just naturally and without always concatenating the full document.
     */

    KTextEditor::DocumentPrivate doc;
    doc.setText("foo\nbar\nzonk");

    const QRegularExpression pattern(QStringLiteral("ar\nzonk$"), QRegularExpression::MultilineOption | QRegularExpression::UseUnicodePropertiesOption);
    int startLine = 0;
    int endLine = 2;
    QString textToMatch;
    int partialMatchLine = -1;
    for (int currentLine = startLine; currentLine <= endLine; ++currentLine) {
        // if we had a partial match before, we need to keep the old text and append our new line
        int matchStartLine = currentLine;
        if (partialMatchLine >= 0) {
            textToMatch += doc.line(currentLine);
            textToMatch += QLatin1Char('\n');
            matchStartLine = partialMatchLine;
        } else {
            textToMatch = doc.line(currentLine);
            textToMatch += QLatin1Char('\n');
        }

        auto result = pattern.match(textToMatch, 0, QRegularExpression::PartialPreferFirstMatch);
        qDebug() << "match" << result.hasMatch() << result.hasPartialMatch() << result.capturedStart() << result.capturedLength();

        if (result.hasMatch()) {
            printf("found stuff starting in line %d\n", matchStartLine);
            break;
        }

        // else: remember if we need to keep text!
        if (result.hasPartialMatch()) {
            // if we had already a partial match before, keep the line!
            if (partialMatchLine >= 0) {
            } else {
                partialMatchLine = currentLine;
            }
        } else {
            // we can forget the old text
            partialMatchLine = -1;
        }
    }
}

void KateDocumentTest::testMatchingBracket_data()
{
    QTest::addColumn<QString>("text");
    QTest::addColumn<KTextEditor::Cursor>("cursor");
    QTest::addColumn<KTextEditor::Range>("match");
    QTest::addColumn<int>("maxLines");

    QTest::addRow("invalid") << "asdf\nasdf" << KTextEditor::Cursor(1, 0) << KTextEditor::Range::invalid() << 10;
    QTest::addRow("]-before") << "[\n]" << KTextEditor::Cursor(1, 0) << KTextEditor::Range({0, 0}, {1, 0}) << 10;
    QTest::addRow("]-after") << "[\n]" << KTextEditor::Cursor(1, 1) << KTextEditor::Range({0, 0}, {1, 0}) << 10;
    QTest::addRow("[-before") << "[\n]" << KTextEditor::Cursor(0, 0) << KTextEditor::Range({0, 0}, {1, 0}) << 10;
    QTest::addRow("[-after") << "[\n]" << KTextEditor::Cursor(0, 1) << KTextEditor::Range({0, 0}, {1, 0}) << 10;
    QTest::addRow("}-before") << "{\n}" << KTextEditor::Cursor(1, 0) << KTextEditor::Range({0, 0}, {1, 0}) << 10;
    QTest::addRow("}-after") << "{\n}" << KTextEditor::Cursor(1, 1) << KTextEditor::Range({0, 0}, {1, 0}) << 10;
    QTest::addRow("{-before") << "{\n}" << KTextEditor::Cursor(0, 0) << KTextEditor::Range({0, 0}, {1, 0}) << 10;
    QTest::addRow("{-after") << "{\n}" << KTextEditor::Cursor(0, 1) << KTextEditor::Range({0, 0}, {1, 0}) << 10;
    QTest::addRow(")-before") << "(\n)" << KTextEditor::Cursor(1, 0) << KTextEditor::Range({0, 0}, {1, 0}) << 10;
    QTest::addRow(")-after") << "(\n)" << KTextEditor::Cursor(1, 1) << KTextEditor::Range({0, 0}, {1, 0}) << 10;
    QTest::addRow("(-before") << "(\n)" << KTextEditor::Cursor(0, 0) << KTextEditor::Range({0, 0}, {1, 0}) << 10;
    QTest::addRow("(-after") << "(\n)" << KTextEditor::Cursor(0, 1) << KTextEditor::Range({0, 0}, {1, 0}) << 10;
    QTest::addRow("]-maxlines") << "[\n\n]" << KTextEditor::Cursor(1, 0) << KTextEditor::Range::invalid() << 1;
}

void KateDocumentTest::testMatchingBracket()
{
    QFETCH(QString, text);
    QFETCH(KTextEditor::Cursor, cursor);
    QFETCH(KTextEditor::Range, match);
    QFETCH(int, maxLines);

    KTextEditor::DocumentPrivate doc;
    doc.setText(text);
    QCOMPARE(doc.findMatchingBracket(cursor, maxLines), match);
}

void KateDocumentTest::testIndentOnPaste()
{
    KTextEditor::DocumentPrivate doc;
    auto view = static_cast<KTextEditor::ViewPrivate *>(doc.createView(nullptr));

    doc.setHighlightingMode("C++");
    doc.config()->setTabWidth(4);
    doc.config()->setIndentationMode("cppstyle");

    doc.setConfigValue("indent-pasted-text", true);

    /**
     * namespace ns
     * {
     * class MyClass
     */
    doc.setText(QStringLiteral("namespace ns\n{\nclass MyClass"));
    view->setCursorPosition({2, 5});
    doc.paste(view, QStringLiteral(" SOME_MACRO"));
    // We have text in the line we are pasting in so the existing indentation shouldn't be disturbed
    QCOMPARE(doc.text(), QStringLiteral("namespace ns\n{\nclass SOME_MACRO MyClass"));

    /**
     * namespace ns
     * {
     */
    doc.setText(QStringLiteral("namespace ns\n{\n"));
    view->setCursorPosition({2, 0});
    doc.paste(view, QStringLiteral("class MyClass"));
    // We have no text in the line we are pasting in so indentation will be adjusted
    QCOMPARE(doc.text(), QStringLiteral("namespace ns\n{\n    class MyClass"));
}

void KateDocumentTest::testAboutToSave()
{
    KTextEditor::DocumentPrivate doc;
    const QString thisFile = QString::fromUtf8(__FILE__);
    bool opened = doc.openUrl(QUrl::fromLocalFile(thisFile));

    QVERIFY(opened);

    QSignalSpy spy(&doc, &KTextEditor::DocumentPrivate::aboutToSave);
    QSignalSpy savedSpy(&doc, &KTextEditor::DocumentPrivate::documentSavedOrUploaded);

    doc.documentSave();

    QVERIFY(spy.count() == 1 || spy.wait());
    QVERIFY(savedSpy.count() == 1 || savedSpy.wait());
}

void KateDocumentTest::testKeepUndoOverReload()
{
    // create test document with some simple text
    KTextEditor::DocumentPrivate doc;
    const QString initialText = QStringLiteral("lala\ntest\nfoobar\n");
    doc.setText(initialText);
    QCOMPARE(doc.text(), initialText);

    // now: do some editing
    const QString insertedText = QStringLiteral("newfirstline\n");
    doc.insertText(KTextEditor::Cursor(0, 0), insertedText);
    QCOMPARE(doc.text(), insertedText + initialText);

    // test undo/redo
    doc.undo();
    QCOMPARE(doc.text(), initialText);
    doc.redo();
    QCOMPARE(doc.text(), insertedText + initialText);

    // save it to some local temporary file, for later reload
    QTemporaryFile tmpFile;
    tmpFile.open();
    QVERIFY(doc.saveAs(QUrl::fromLocalFile(tmpFile.fileName())));

    // first: try if normal reload works
    QVERIFY(doc.documentReload());
    QCOMPARE(doc.text(), insertedText + initialText);

    // test undo/redo AFTER reload
    doc.undo();
    QCOMPARE(doc.text(), initialText);
    doc.redo();
    QCOMPARE(doc.text(), insertedText + initialText);
}

void KateDocumentTest::testToggleComment()
{
    { // BUG: 451471
        KTextEditor::DocumentPrivate doc;
        QVERIFY(doc.highlightingModes().contains(QStringLiteral("Python")));
        doc.setHighlightingMode(QStringLiteral("Python"));
        const QString original = QStringLiteral("import hello;\ndef method():");
        doc.setText(original);
        QVERIFY(doc.lines() == 2);

        doc.commentSelection(doc.documentRange(), {1, 2}, false, DocumentPrivate::ToggleComment);
        QCOMPARE(doc.text(), QStringLiteral("# import hello;\n# def method():"));

        doc.commentSelection(doc.documentRange(), {1, 2}, false, DocumentPrivate::ToggleComment);
        QCOMPARE(doc.text(), original);
    }

    { // Comment C++;
        KTextEditor::DocumentPrivate doc;
        QVERIFY(doc.highlightingModes().contains(QStringLiteral("C++")));
        doc.setHighlightingMode(QStringLiteral("C++"));
        QString original = QStringLiteral("#include<iostream>\nint main()\n{\nreturn 0;\n}\n");
        doc.setText(original);
        QVERIFY(doc.lines() == 6);

        doc.commentSelection(doc.documentRange(), {5, 0}, false, DocumentPrivate::ToggleComment);
        QCOMPARE(doc.text(), QStringLiteral("// #include<iostream>\n// int main()\n// {\n// return 0;\n// }\n"));

        doc.commentSelection(doc.documentRange(), {5, 0}, false, DocumentPrivate::ToggleComment);
        QCOMPARE(doc.text(), original);

        // Comment just a portion
        doc.commentSelection(Range(1, 0, 1, 3), Cursor(1, 3), false, DocumentPrivate::ToggleComment);
        QCOMPARE(doc.text(), QStringLiteral("#include<iostream>\n/*int*/ main()\n{\nreturn 0;\n}\n"));
        doc.commentSelection(Range(1, 0, 1, 7), Cursor(1, 3), false, DocumentPrivate::ToggleComment);
        QCOMPARE(doc.text(), original);

        // mixed, one line commented, one not => both get commented
        original = QStringLiteral(" // int main()\n{}");
        doc.setText(original);
        doc.commentSelection(doc.documentRange(), {1, 2}, false, DocumentPrivate::ToggleComment);
        QCOMPARE(doc.text(), QStringLiteral("//  // int main()\n// {}"));
        doc.commentSelection(doc.documentRange(), {1, 2}, false, DocumentPrivate::ToggleComment);
        // after uncommenting, we get original text back with one line commented
        QCOMPARE(doc.text(), original);
    }

    { // BUG: 458126
        KTextEditor::DocumentPrivate doc;
        doc.setText("another:\n\nanother2: hello");
        QVERIFY(doc.highlightingModes().contains(QStringLiteral("YAML")));
        doc.setHighlightingMode("YAML");
        const QString original = doc.text();

        doc.commentSelection(doc.documentRange(), {2, 0}, false, DocumentPrivate::ToggleComment);
        QCOMPARE(doc.text(), "# another:\n# \n# another2: hello");

        doc.commentSelection(doc.documentRange(), {2, 0}, false, DocumentPrivate::ToggleComment);
        QCOMPARE(doc.text(), original);
    }
}

void KateDocumentTest::testInsertTextTooLargeColumn()
{
    KTextEditor::DocumentPrivate doc;
    const QString original = QStringLiteral("01234567\n01234567");
    doc.setText(original);
    QVERIFY(doc.lines() == 2);
    QCOMPARE(doc.text(), original);

    // try to insert text with initial \n at wrong column, did trigger invalid call to editWrapLine
    doc.insertText(KTextEditor::Cursor(0, 10), QStringLiteral("\nxxxx"));
    QCOMPARE(doc.text(), QStringLiteral("01234567  \nxxxx\n01234567"));
}

#include "katedocument_test.moc"
