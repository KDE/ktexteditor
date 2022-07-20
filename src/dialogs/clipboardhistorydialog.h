/*
    SPDX-FileCopyrightText: 2022 Eric Armbruster <eric1@armbruster-online.de>
    SPDX-FileCopyrightText: 2022 Waqar Ahmed <waqar.17a@gmail.com>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef CLIPBOARD_HISTORY_DIALOG_H
#define CLIPBOARD_HISTORY_DIALOG_H

#include "kateglobal.h"
#include "quickdialog.h"

class QFont;
class QPalette;
class QSortFilterProxyModel;
class QTextEdit;

class ClipboardHistoryModel;
class ClipboardHistoryFilterModel;

namespace KTextEditor
{
class DocumentPrivate;
class ViewPrivate;
}

class ClipboardHistoryDialog : public QuickDialog
{
    Q_OBJECT

public:
    ClipboardHistoryDialog(QWidget *mainwindow, KTextEditor::ViewPrivate *mainWindow);

    void resetValues();
    void openDialog(const QVector<KTextEditor::EditorPrivate::ClipboardEntry> &clipboardHistory);

private Q_SLOTS:
    void slotReturnPressed() override;

private:
    QFont editorFont();
    void showSelectedText(const QModelIndex &idx);

private:
    /*
     * View containing the currently open document
     */
    KTextEditor::ViewPrivate *m_viewPrivate;

    ClipboardHistoryModel *m_model;
    ClipboardHistoryFilterModel *m_proxyModel;

    /*
     * Document for the selected text to paste
     */
    KTextEditor::DocumentPrivate *m_selectedDoc;

    /*
     * View containing the selected text to paste
     */
    KTextEditor::ViewPrivate *m_selectedView;
};

#endif