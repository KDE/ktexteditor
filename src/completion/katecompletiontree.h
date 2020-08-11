/*
    SPDX-FileCopyrightText: 2006 Hamish Rodda <rodda@kde.org>
    SPDX-FileCopyrightText: 2007-2008 David Nolden <david.nolden.kdevelop@art-master.de>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef KATECOMPLETIONTREE_H
#define KATECOMPLETIONTREE_H

#include "expandingtree/expandingtree.h"

class KateCompletionWidget;
class KateCompletionModel;

class QTimer;

class KateCompletionTree : public ExpandingTree
{
    Q_OBJECT

public:
    explicit KateCompletionTree(KateCompletionWidget *parent);

    KateCompletionWidget *widget() const;
    KateCompletionModel *kateModel() const;

    void resizeColumns(bool firstShow = false, bool forceResize = false);

    // Navigation
    bool nextCompletion();
    bool previousCompletion();
    bool pageDown();
    bool pageUp();
    void top();
    void bottom();

    void scheduleUpdate();

    void setScrollingEnabled(bool);

    /// Returns the approximated viewport position of the text in the given column, skipping an eventual icon
    int columnTextViewportPosition(int column) const;

private Q_SLOTS:
    void resizeColumnsSlot();

protected:
    void currentChanged(const QModelIndex &current, const QModelIndex &previous) override; /// Not available as a signal in this way
    void scrollContentsBy(int dx, int dy) override;
    QStyleOptionViewItem viewOptions() const override;

private:
    bool m_scrollingEnabled;
    QTimer *m_resizeTimer;
};

#endif
