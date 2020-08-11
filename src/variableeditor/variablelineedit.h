/*
    SPDX-FileCopyrightText: 2011-2018 Dominik Haumann <dhaumann@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef VARIABLE_LINE_EDIT_H
#define VARIABLE_LINE_EDIT_H

#include <QWidget>

class QFrame;
class QLineEdit;
class QToolButton;
class VariableListView;

class VariableLineEdit : public QWidget
{
    Q_OBJECT

public:
    explicit VariableLineEdit(QWidget *parent = nullptr);
    virtual ~VariableLineEdit();

    void addKateItems(VariableListView *listview);
    QString text();

public Q_SLOTS:
    void editVariables();
    void setText(const QString &text);
    void clear();
    void updateVariableLine();

Q_SIGNALS:
    void textChanged(const QString &);

private:
    QFrame *m_popup;
    QLineEdit *m_lineedit;
    QToolButton *m_button;
    VariableListView *m_listview;
};

#endif
