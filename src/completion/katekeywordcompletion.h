/* This file is part of the KDE libraries
   Copyright (C) 2014 Sven Brauch <svenbrauch@gmail.com>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License version 2, or any later version,
   as published by the Free Software Foundation.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#ifndef KATEKEYWORDCOMPLETIONMODEL_H
#define KATEKEYWORDCOMPLETIONMODEL_H

#include "ktexteditor/codecompletionmodel.h"
#include "codecompletionmodelcontrollerinterface.h"

/**
 * @brief Highlighting-file based keyword completion for the editor.
 *
 * This model offers completion of language-specific keywords based on information
 * taken from the kate syntax files. It queries the highlighting engine to get the
 * correct context for a given cursor position, then suggests all keyword items
 * from the XML file for the active language.
 */
class KateKeywordCompletionModel : public KTextEditor::CodeCompletionModel
                                 , public KTextEditor::CodeCompletionModelControllerInterface
{
    Q_OBJECT
    Q_INTERFACES(KTextEditor::CodeCompletionModelControllerInterface)

public:
    explicit KateKeywordCompletionModel(QObject* parent);
    QVariant data(const QModelIndex& index, int role) const Q_DECL_OVERRIDE;
    int rowCount(const QModelIndex& parent = QModelIndex()) const Q_DECL_OVERRIDE;
    QModelIndex parent(const QModelIndex& index) const Q_DECL_OVERRIDE;
    QModelIndex index(int row, int column, const QModelIndex& parent = QModelIndex()) const Q_DECL_OVERRIDE;
    void completionInvoked(KTextEditor::View* view, const KTextEditor::Range& range,
                           InvocationType invocationType) Q_DECL_OVERRIDE;
    KTextEditor::Range completionRange(KTextEditor::View* view, const KTextEditor::Cursor& position) Q_DECL_OVERRIDE;
    bool shouldAbortCompletion(KTextEditor::View* view, const KTextEditor::Range& range,
                               const QString& currentCompletion) Q_DECL_OVERRIDE;
    bool shouldStartCompletion(KTextEditor::View* view, const QString& insertedText, bool userInsertion,
                               const KTextEditor::Cursor& position) Q_DECL_OVERRIDE;
    MatchReaction matchingItem(const QModelIndex& matched) Q_DECL_OVERRIDE;
    bool shouldHideItemsWithEqualNames() const Q_DECL_OVERRIDE;

private:
    QList<QString> m_items;
};

#endif // KATEKEYWORDCOMPLETIONMODEL_H

// kate: indent-width 4; replace-tabs on
