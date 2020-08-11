/*
    SPDX-FileCopyrightText: 2007 David Nolden <david.nolden.kdevelop@art-master.de>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef KATEARGUMENTHINTTREE_H
#define KATEARGUMENTHINTTREE_H

#include "expandingtree/expandingtree.h"

class KateCompletionWidget;
class KateArgumentHintModel;
class QRect;

class KateArgumentHintTree : public ExpandingTree
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

protected:
    void paintEvent(QPaintEvent *event) override;
    void rowsInserted(const QModelIndex &parent, int start, int end) override;
    void dataChanged(const QModelIndex &topLeft, const QModelIndex &bottomRight, const QVector<int> &roles = QVector<int>()) override;
    void currentChanged(const QModelIndex &current, const QModelIndex &previous) override;

private:
    uint rowHeight(const QModelIndex &index) const;
    KateArgumentHintModel *model() const;
    int sizeHintForColumn(int column) const override;

    KateCompletionWidget *m_parent;
};

#endif
