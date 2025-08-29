/*
    SPDX-FileCopyrightText: 2025 Waqar Ahmed <waqar.17a@gmail.com>
    SPDX-License-Identifier: LGPL-2.0-or-later
*/
#ifndef KATEMODELINECOMPLETION_H
#define KATEMODELINECOMPLETION_H

#include <ktexteditor/codecompletionmodel.h>
#include <ktexteditor/codecompletionmodelcontrollerinterface.h>

#include <ktexteditor_export.h>

class KateModelineCompletionModel : public KTextEditor::CodeCompletionModel, public KTextEditor::CodeCompletionModelControllerInterface
{
    Q_INTERFACES(KTextEditor::CodeCompletionModelControllerInterface)
public:
    explicit KateModelineCompletionModel(QObject *parent);

    struct ModelineCompletion {
        QString variable;
        QString description;
    };

    void completionInvoked(KTextEditor::View *view, const KTextEditor::Range &range, InvocationType invocationType) override;
    bool shouldStartCompletion(KTextEditor::View *view, const QString &insertedText, bool userInsertion, const KTextEditor::Cursor &position) override;
    int rowCount(const QModelIndex &parent) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    void executeCompletionItem(KTextEditor::View *view, const KTextEditor::Range &word, const QModelIndex &index) const override;

private:
    QList<ModelineCompletion> m_matches;
};

#endif
