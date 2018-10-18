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


#ifndef COMPLETION_TEST_H
#define COMPLETION_TEST_H


#include "base.h"
#include "fakecodecompletiontestmodel.h"


/**
 * This class handles implements a completion model for the completion
 * tests defined in the CompletionTest class.
 */
class VimCodeCompletionTestModel : public KTextEditor::CodeCompletionModel
{
public:
    explicit VimCodeCompletionTestModel(KTextEditor::View *parent);
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    KTextEditor::CodeCompletionInterface *cc() const;
};

/**
 * This class implements a completion model used in the CompletionTest class
 * to test that code completion is not invoked.
 */
class FailTestOnInvocationModel : public KTextEditor::CodeCompletionModel
{
public:
    explicit FailTestOnInvocationModel(KTextEditor::View *parent);
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    void failTest() const;
    KTextEditor::CodeCompletionInterface *cc() const;
};


class CompletionTest : public BaseTest
{
    Q_OBJECT

private Q_SLOTS:
    void FakeCodeCompletionTests();
    void CompletionTests();

private:
    void waitForCompletionWidgetToActivate();
    void clearTrackedDocumentChanges();
};


#endif /* COMPLETION_TEST_H */
