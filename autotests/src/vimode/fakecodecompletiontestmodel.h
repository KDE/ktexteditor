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


#ifndef FAKE_CODE_COMPLETION_TEST_MODEL_H
#define FAKE_CODE_COMPLETION_TEST_MODEL_H


#include <ktexteditor/codecompletionmodel.h>


/**
 * Helper class that mimics some of the behaviour of KDevelop's code completion, in particular
 * whether it performs "bracket merging" on completed function calls e.g. if we complete a call
 * to "functionCall(int a)" at the end of the -> here:
 *
 *  object->(
 *
 * we end up with
 *
 *  object->functionCall(
 *
 * and the cursor placed after the closing bracket: the opening bracket is merged with the existing
 * bracket.
 *
 * However, if we do the same with
 *
 *  object->
 *
 * we end up with
 *
 *  object->functionCall()
 *
 * again with the cursor placed after the opening bracket.  This time, the brackets were not merged.
 *
 * This helper class is used to test how Macros and replaying of last changes works with complex
 * code completion.
 */
class FakeCodeCompletionTestModel : public KTextEditor::CodeCompletionModel
{
    Q_OBJECT

public:
    explicit FakeCodeCompletionTestModel(KTextEditor::View *parent);
    /**
     * List of completions, in sorted order.
     * A string ending with "()" is treated as a call to a function with no arguments.
     * A string ending with "(...)" is treated as a call to a function with at least one argument.  The "..." is not
     * inserted into the text.
     * A string ending with "();" or "(...);" is the same as above, and the semi-colon is added.  Bracket merging
     * never happens with strings ending with ";".
     */
    void setCompletions(const QStringList &completions);
    void setRemoveTailOnComplete(bool removeTailOnCompletion);
    void setFailTestOnInvocation(bool failTestOnInvocation);
    bool wasInvoked();
    void clearWasInvoked();
    /**
     * A more reliable form of setAutomaticInvocationEnabled().
     */
    void forceInvocationIfDocTextIs(const QString &desiredDocText);
    void doNotForceInvocation();
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    void executeCompletionItem (KTextEditor::View *view, const KTextEditor::Range &word, const QModelIndex &index) const override;
    KTextEditor::CodeCompletionInterface *cc() const;

private:
    void failTest() const;
    QStringList m_completions;
    KTextEditor::ViewPrivate *m_kateView;
    KTextEditor::Document *m_kateDoc;
    bool m_removeTailOnCompletion;
    bool m_failTestOnInvocation;
    mutable bool m_wasInvoked;
    QString m_forceInvocationIfDocTextIs;

private Q_SLOTS:
    void textInserted(KTextEditor::Document *document, KTextEditor::Range range);
    void textRemoved(KTextEditor::Document *document, KTextEditor::Range range);
    void checkIfShouldForceInvocation();

};


#endif /* FAKE_CODE_COMPLETION_TEST_MODEL_H */
