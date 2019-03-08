/*
 * This file is part of the KDE libraries
 *
 * Copyright (C) 2014 Miquel Sabaté Solà <mikisabate@gmail.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 * You should have received a copy of the GNU Library General Public License
 * along with this library; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */


#include <katecompletionwidget.h>
#include <kateglobal.h>
#include <katewordcompletion.h>
#include "base.h"
#include "fakecodecompletiontestmodel.h"

using namespace KTextEditor;


FakeCodeCompletionTestModel::FakeCodeCompletionTestModel(KTextEditor::View *parent)
    : KTextEditor::CodeCompletionModel(parent),
      m_kateView(qobject_cast<KTextEditor::ViewPrivate *>(parent)),
      m_kateDoc(parent->document()),
      m_removeTailOnCompletion(false),
      m_failTestOnInvocation(false),
      m_wasInvoked(false)
{
    Q_ASSERT(m_kateView);
    setRowCount(3);
    cc()->setAutomaticInvocationEnabled(false);
    cc()->unregisterCompletionModel(KTextEditor::EditorPrivate::self()->wordCompletionModel()); //would add additional items, we don't want that in tests
    connect(static_cast<KTextEditor::DocumentPrivate *>(parent->document()), &KTextEditor::DocumentPrivate::textInserted,
            this, &FakeCodeCompletionTestModel::textInserted);
    connect(static_cast<KTextEditor::DocumentPrivate *>(parent->document()), &KTextEditor::DocumentPrivate::textRemoved,
            this, &FakeCodeCompletionTestModel::textRemoved);
}

void FakeCodeCompletionTestModel::setCompletions(const QStringList &completions)
{
    QStringList sortedCompletions = completions;
    std::sort(sortedCompletions.begin(), sortedCompletions.end());
    Q_ASSERT(completions == sortedCompletions && "QCompleter seems to sort the items, so it's best to provide them pre-sorted so it's easier to predict the order");
    setRowCount(sortedCompletions.length());
    m_completions = completions;
}

void FakeCodeCompletionTestModel::setRemoveTailOnComplete(bool removeTailOnCompletion)
{
    m_removeTailOnCompletion = removeTailOnCompletion;
}

void FakeCodeCompletionTestModel::setFailTestOnInvocation(bool failTestOnInvocation)
{
    m_failTestOnInvocation = failTestOnInvocation;
}

bool FakeCodeCompletionTestModel::wasInvoked()
{
    return m_wasInvoked;
}

void FakeCodeCompletionTestModel::clearWasInvoked()
{
    m_wasInvoked = false;
}

void FakeCodeCompletionTestModel::forceInvocationIfDocTextIs(const QString &desiredDocText)
{
    m_forceInvocationIfDocTextIs = desiredDocText;
}

void FakeCodeCompletionTestModel::doNotForceInvocation()
{
    m_forceInvocationIfDocTextIs.clear();
}

QVariant FakeCodeCompletionTestModel::data(const QModelIndex &index, int role) const
{
    m_wasInvoked = true;
    if (m_failTestOnInvocation) {
        failTest();
        return QVariant();
    }
    // Order is important, here, as the completion widget seems to do its own sorting.
    if (role == Qt::DisplayRole) {
        if (index.column() == Name) {
            return QString(m_completions[index.row()]);
        }
    }
    return QVariant();
}

void FakeCodeCompletionTestModel::executeCompletionItem (KTextEditor::View *view, const KTextEditor::Range &word, const QModelIndex &index) const
{
    qDebug() << "word: " << word << "(" << view->document()->text(word) << ")";
    const Cursor origCursorPos = m_kateView->cursorPosition();
    const QString textToInsert = m_completions[index.row()];
    const QString textAfterCursor = view->document()->text(Range(word.end(), Cursor(word.end().line(), view->document()->lineLength(word.end().line()))));
    view->document()->removeText(Range(word.start(), origCursorPos));
    const int lengthStillToRemove = word.end().column() - origCursorPos.column();
    QString actualTextInserted = textToInsert;
    // Merge brackets?
    const QString noArgFunctionCallMarker = "()";
    const QString withArgFunctionCallMarker = "(...)";
    const bool endedWithSemiColon  = textToInsert.endsWith(';');
    if (textToInsert.contains(noArgFunctionCallMarker) || textToInsert.contains(withArgFunctionCallMarker)) {
        Q_ASSERT(m_removeTailOnCompletion && "Function completion items without removing tail is not yet supported!");
        const bool takesArgument = textToInsert.contains(withArgFunctionCallMarker);
        // The code for a function call to a function taking no arguments.
        const QString justFunctionName = textToInsert.left(textToInsert.indexOf("("));

        QRegExp whitespaceThenOpeningBracket("^\\s*(\\()");
        int openingBracketPos = -1;
        if (textAfterCursor.contains(whitespaceThenOpeningBracket)) {
            openingBracketPos = whitespaceThenOpeningBracket.pos(1) + word.start().column() + justFunctionName.length() + 1 + lengthStillToRemove;
        }
        const bool mergeOpenBracketWithExisting = (openingBracketPos != -1) && !endedWithSemiColon;
        // Add the function name, for now: we don't yet know if we'll be adding the "()", too.
        view->document()->insertText(word.start(), justFunctionName);
        if (mergeOpenBracketWithExisting) {
            // Merge with opening bracket.
            actualTextInserted = justFunctionName;
            m_kateView->setCursorPosition(Cursor(word.start().line(), openingBracketPos));
        } else {
            // Don't merge.
            const QString afterFunctionName = endedWithSemiColon ? "();" : "()";
            view->document()->insertText(Cursor(word.start().line(), word.start().column() + justFunctionName.length()), afterFunctionName);
            if (takesArgument) {
                // Place the cursor immediately after the opening "(" we just added.
                m_kateView->setCursorPosition(Cursor(word.start().line(), word.start().column() + justFunctionName.length() + 1));
            }
        }
    } else {
        // Plain text.
        view->document()->insertText(word.start(), textToInsert);
    }
    if (m_removeTailOnCompletion) {
        const int tailLength = word.end().column() - origCursorPos.column();
        const Cursor tailStart = Cursor(word.start().line(), word.start().column() + actualTextInserted.length());
        const Cursor tailEnd = Cursor(tailStart.line(), tailStart.column() + tailLength);
        view->document()->removeText(Range(tailStart, tailEnd));
    }
}

KTextEditor::CodeCompletionInterface *FakeCodeCompletionTestModel::cc() const
{
    return dynamic_cast<KTextEditor::CodeCompletionInterface *>(const_cast<QObject *>(QObject::parent()));
}

void FakeCodeCompletionTestModel::failTest() const
{
    QFAIL("Shouldn't be invoking me!");
}

void FakeCodeCompletionTestModel::textInserted(Document *document, Range range)
{
    Q_UNUSED(document);
    Q_UNUSED(range);
    checkIfShouldForceInvocation();
}

void FakeCodeCompletionTestModel::textRemoved(Document *document, Range range)
{
    Q_UNUSED(document);
    Q_UNUSED(range);
    checkIfShouldForceInvocation();
}

void FakeCodeCompletionTestModel::checkIfShouldForceInvocation()
{
    if (m_forceInvocationIfDocTextIs.isEmpty()) {
        return;
    }

    if (m_kateDoc->text() == m_forceInvocationIfDocTextIs) {
        m_kateView->completionWidget()->userInvokedCompletion();
        BaseTest::waitForCompletionWidgetToActivate(m_kateView);
    }
}
