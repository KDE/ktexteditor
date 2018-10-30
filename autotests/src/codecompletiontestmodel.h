/* This file is part of the KDE project
   Copyright (C) 2005 Hamish Rodda <rodda@kde.org>

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

#ifndef CODECOMPLETIONTEST_MODEL_H
#define CODECOMPLETIONTEST_MODEL_H

#include <ktexteditor/codecompletionmodel.h>
#include <QStringList>

namespace KTextEditor
{
class View;
class CodeCompletionInterface;
}

class CodeCompletionTestModel : public KTextEditor::CodeCompletionModel
{
    Q_OBJECT

public:
    explicit CodeCompletionTestModel(KTextEditor::View *parent = nullptr, const QString &startText = QString());

    KTextEditor::View *view() const;
    KTextEditor::CodeCompletionInterface *cc() const;

    void completionInvoked(KTextEditor::View *view, const KTextEditor::Range &range, InvocationType invocationType) override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;

private:
    QString m_startText;
    bool m_autoStartText;
};

class AbbreviationCodeCompletionTestModel : public CodeCompletionTestModel
{
    Q_OBJECT

public:
    explicit AbbreviationCodeCompletionTestModel(KTextEditor::View *parent = nullptr, const QString &startText = QString());

    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;

private:
    QStringList m_items;
};

#endif
