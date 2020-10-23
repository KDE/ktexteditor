/*
    SPDX-FileCopyrightText: 2013 Sven Brauch <svenbrauch@gmail.com>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "wordcompletiontest.h"

#include <kateglobal.h>
#include <katewordcompletion.h>
#include <ktexteditor/editor.h>
#include <ktexteditor/view.h>

#include <QtTestWidgets>

QTEST_MAIN(WordCompletionTest)

// was 500000, but that takes 30 seconds on build.kde.org, removed two 0 ;)
static const int count = 5000;

using namespace KTextEditor;

void WordCompletionTest::initTestCase()
{
    KTextEditor::EditorPrivate::enableUnitTestMode();
    Editor *editor = KTextEditor::Editor::instance();
    QVERIFY(editor);

    m_doc = editor->createDocument(this);
    QVERIFY(m_doc);
}

void WordCompletionTest::cleanupTestCase()
{
}

void WordCompletionTest::init()
{
    m_doc->clear();
}

void WordCompletionTest::cleanup()
{
}

void WordCompletionTest::benchWordRetrievalMixed()
{
    const int distinctWordRatio = 100;
    QStringList s;
    s.reserve(count);
    for (int i = 0; i < count; i++) {
        s.append(QLatin1String("HelloWorld") + QString::number(i / distinctWordRatio));
    }
    s.prepend("\n");
    m_doc->setText(s);

    // creating the view only after inserting the text makes test execution much faster
    QSharedPointer<KTextEditor::View> v(m_doc->createView(nullptr));
    QBENCHMARK {
        KateWordCompletionModel m(nullptr);
        QCOMPARE(m.allMatches(v.data(), KTextEditor::Range()).size(), count / distinctWordRatio);
    }
}

void WordCompletionTest::benchWordRetrievalSame()
{
    QStringList s;
    s.reserve(count);
    // add a number so the words have roughly the same length as in the other tests
    const QString str = QLatin1String("HelloWorld") + QString::number(count);
    for (int i = 0; i < count; i++) {
        s.append(str);
    }
    s.prepend("\n");
    m_doc->setText(s);

    QSharedPointer<KTextEditor::View> v(m_doc->createView(nullptr));
    QBENCHMARK {
        KateWordCompletionModel m(nullptr);
        QCOMPARE(m.allMatches(v.data(), KTextEditor::Range()).size(), 1);
    }
}

void WordCompletionTest::benchWordRetrievalDistinct()
{
    QStringList s;
    s.reserve(count);
    for (int i = 0; i < count; i++) {
        s.append(QLatin1String("HelloWorld") + QString::number(i));
    }
    s.prepend("\n");
    m_doc->setText(s);

    QSharedPointer<KTextEditor::View> v(m_doc->createView(nullptr));
    QBENCHMARK {
        KateWordCompletionModel m(nullptr);
        QCOMPARE(m.allMatches(v.data(), KTextEditor::Range()).size(), count);
    }
}
