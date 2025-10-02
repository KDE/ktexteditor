/*
    SPDX-FileCopyrightText: 2025 Waqar Ahmed <waqar.17a@gmail.com>
    SPDX-License-Identifier: LGPL-2.0-or-later
*/
#include "katemodelinecompletion.h"
#include "variableitem.h"
#include "variablelineedit.h"
#include "variablelistview.h"

#include <ktexteditor/document.h>
#include <ktexteditor/view.h>

static const QList<KateModelineCompletionModel::ModelineCompletion> &allVariables()
{
    static QList<KateModelineCompletionModel::ModelineCompletion> variables;
    if (variables.isEmpty()) {
        VariableListView list(QString(), nullptr);
        VariableLineEdit::addKateItems(&list);

        for (VariableItem *varItem : list.items()) {
            variables.append({varItem->variable(), varItem->helpText()});
        }
    }

    return variables;
}

static const QList<KateModelineCompletionModel::ModelineCompletion> &allModelineStarts()
{
    using Completion = KateModelineCompletionModel::ModelineCompletion;
    static const QList<KateModelineCompletionModel::ModelineCompletion> starts{
        Completion{QStringLiteral("kate-wildcard():"),
                   QStringLiteral("Apply settings to files matching a wildcard. e.g., kate-wildcard(*.cpp): indent-width 4;")},
        Completion{QStringLiteral("kate-mimetype():"),
                   QStringLiteral("Apply settings to files matching a MIME type. e.g., kate-mimetype(text/x-c++src): indent-width 4;")}};
    return starts;
}

KateModelineCompletionModel::KateModelineCompletionModel(QObject *parent)
    : KTextEditor::CodeCompletionModel(parent)
{
}

void KateModelineCompletionModel::completionInvoked(KTextEditor::View *view, const KTextEditor::Range &range, InvocationType)
{
    if (shouldStartCompletion(view, {}, true, range.start())) {
        auto cursor = view->cursorPosition();
        const QString line = view->document()->line(cursor.line());
        if (line.endsWith(QLatin1String("kate")) || line.endsWith(QLatin1String("kate-"))) {
            m_matches = allModelineStarts();
        } else {
            m_matches = allVariables();
        }
        setRowCount(int(m_matches.size()));
    }
}

bool KateModelineCompletionModel::shouldStartCompletion(KTextEditor::View *view, const QString &, bool userInsertion, const KTextEditor::Cursor &position)
{
    if (!userInsertion) {
        return false;
    }
    if (view->document()->url().fileName() == QLatin1String(".kateconfig")) {
        return true;
    }

    const QString line = view->document()->line(position.line());
    if (int katePos = line.indexOf(QLatin1String("kate")); katePos != -1) {
        katePos += 4;
        if (katePos >= line.size() - 1) {
            return true;
        }
        if (line[katePos] == u':' || line[katePos] == u'-') {
            return true;
        }
    }
    return false;
}

int KateModelineCompletionModel::rowCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : int(m_matches.size());
}

QVariant KateModelineCompletionModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid())
        return {};

    if (index.column() == KTextEditor::CodeCompletionModel::Name && role == Qt::DisplayRole) {
        return m_matches.at(index.row()).variable;
    }

    if (role == KTextEditor::CodeCompletionModel::ExpandingWidget) {
        return m_matches.at(index.row()).description;
    }

    return {};
}

void KateModelineCompletionModel::executeCompletionItem(KTextEditor::View *view, const KTextEditor::Range &word, const QModelIndex &index) const
{
    const QString text = this->data(index.sibling(index.row(), Name), Qt::DisplayRole).toString();
    KTextEditor::CodeCompletionModel::executeCompletionItem(view, word, index);
    if (text.startsWith(u"kate-wildcard") || text.startsWith(u"kate-mimetype")) {
        view->setCursorPosition({view->cursorPosition().line(), view->cursorPosition().column() - 2});
    } else {
        view->insertText(QStringLiteral(" ;"));
        view->setCursorPosition({view->cursorPosition().line(), view->cursorPosition().column() - 1});
    }
}
