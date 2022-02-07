/*
    SPDX-FileCopyrightText: 2007 David Nolden <david.nolden.kdevelop@art-master.de>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef KATEARGUMENTHINTTREE_H
#define KATEARGUMENTHINTTREE_H

#include <QTextDocument>
#include <QTreeView>

class KateCompletionWidget;
class KateArgumentHintModel;
class QRect;

class KateArgumentHintTree : public QTreeView
{
    Q_OBJECT
public:
    explicit KateArgumentHintTree(KateCompletionWidget *parent);

    // Navigation
    bool nextCompletion();
    bool previousCompletion();
    bool pageDown();
    bool pageUp();
    void top();
    void bottom();

    // Returns the total size of all columns
    int resizeColumns();

    void clearCompletion();
public Q_SLOTS:
    void updateGeometry();
    void updateGeometry(QRect rect);

    KateArgumentHintModel *model() const;

protected:
    void paintEvent(QPaintEvent *event) override;
    void rowsInserted(const QModelIndex &parent, int start, int end) override;

private:
    uint rowHeight(const QModelIndex &index) const;
    int sizeHintForColumn(int column) const override;

    KateCompletionWidget *m_parent;
};

#endif
