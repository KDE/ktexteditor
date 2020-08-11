/*
    SPDX-FileCopyrightText: 2007 David Nolden <david.nolden.kdevelop@art-master.de>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef KATEARGUMENTHINTMODEL_H
#define KATEARGUMENTHINTMODEL_H

#include "expandingtree/expandingwidgetmodel.h"
#include "katecompletionmodel.h"
#include <QAbstractListModel>

class KateCompletionWidget;

class KateArgumentHintModel : public ExpandingWidgetModel
{
    Q_OBJECT
public:
    explicit KateArgumentHintModel(KateCompletionWidget *parent);

    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;

    int rowCount(const QModelIndex &parent = {}) const override;

    int columnCount(const QModelIndex &parent = {}) const override;

    QModelIndex index(int row, int column, const QModelIndex &parent = {}) const override;

    QModelIndex parent(const QModelIndex &parent) const override;

    QTreeView *treeView() const override;

    bool indexIsItem(const QModelIndex &index) const override;

    void emitDataChanged(const QModelIndex &start, const QModelIndex &end);

    // Returns the index in the source-model for an index within this model
    QModelIndex mapToSource(const QModelIndex &proxyIndex) const;

    void buildRows();
    void clear();

protected:
    int contextMatchQuality(const QModelIndex &row) const override;
public Q_SLOTS:
    void parentModelReset();
Q_SIGNALS:
    void contentStateChanged(bool hasContent);

private:
    KateCompletionModel::Group *group() const;
    KateCompletionModel *model() const;

    QList<int> m_rows; // Maps rows to either a positive row-number in the source group, or to a negative number which indicates a label

    KateCompletionWidget *m_parent;
};

#endif
