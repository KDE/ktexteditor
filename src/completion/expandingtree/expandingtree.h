/*
    SPDX-FileCopyrightText: 2007 David Nolden <david.nolden.kdevelop@art-master.de>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef ExpandingTree_H
#define ExpandingTree_H

#include <QTextDocument>
#include <QTreeView>

// A tree that allows drawing additional information
class ExpandingTree : public QTreeView
{
public:
    explicit ExpandingTree(QWidget *parent);

protected:
    void drawRow(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const override;
    int sizeHintForColumn(int column) const override;

private:
    mutable QTextDocument m_drawText;
};

#endif
