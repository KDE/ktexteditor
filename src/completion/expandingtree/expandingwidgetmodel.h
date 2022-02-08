/*
    SPDX-FileCopyrightText: 2007 David Nolden <david.nolden.kdevelop@art-master.de>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef EXPANDING_WIDGET_MODEL_H
#define EXPANDING_WIDGET_MODEL_H

#include <QAbstractTableModel>
#include <QIcon>
#include <QPointer>

class QTreeView;

/**
 * Cares about expanding/un-expanding items in a tree-view together with ExpandingDelegate
 */
class ExpandingWidgetModel : public QAbstractItemModel
{
    Q_OBJECT
public:
    explicit ExpandingWidgetModel(QWidget *parent);
    ~ExpandingWidgetModel() override;

    enum ExpandingType { NotExpandable = 0, Expandable, Expanded };

    /// Unexpand all rows and clear all cached information about them(this includes deleting the expanding-widgets)
    void clearExpanding();

    ///@return whether the row given through index is expandable
    bool isExpandable(const QModelIndex &index) const;

    ///@return whether row is currently expanded
    bool isExpanded(const QModelIndex &row) const;
    /// Change the expand-state of the row given through index. The display will be updated.
    void setExpanded(QModelIndex index, bool expanded);

    ///@return the expanding-widget for the given row, if available. Expanding-widgets are in best case available for all expanded rows.
    /// This does not return the partially-expand widget.
    QWidget *expandingWidget(const QModelIndex &row) const;

    /// Places and shows the expanding-widget for the given row, if it should be visible and is valid.
    /// Also shows the partial-expanding-widget when it should be visible.
    void placeExpandingWidget(const QModelIndex &row);

    virtual QTreeView *treeView() const = 0;

    /// Should return true if the given row should be painted like a contained item(as opposed to label-rows etc.)
    virtual bool indexIsItem(const QModelIndex &index) const = 0;

    /// Does not request data from index, this only returns local data like highlighting for expanded rows and similar
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;

    /// Returns the match-color for the given index, or zero if match-quality could not be computed.
    uint matchColor(const QModelIndex &index) const;

public Q_SLOTS:
    /// Place or hides all expanding-widgets to the correct positions. Should be called after the view was scrolled.
    void placeExpandingWidgets();

protected:
    /**
     * @return the context-match quality from 0 to 10 if it could be determined, else -1
     * */
    virtual int contextMatchQuality(const QModelIndex &index) const = 0;

    // Makes sure m_expandedIcon and m_collapsedIcon are loaded
    void cacheIcons() const;

    mutable QIcon m_expandedIcon;
    mutable QIcon m_collapsedIcon;

    // Finds out the basic height of the row represented by the given index. Basic means without respecting any expansion.
    int basicRowHeight(const QModelIndex &index) const;

private:
    //     QMap<QModelIndex, ExpansionType> m_partiallyExpanded;
    // Store expanding-widgets and cache whether items can be expanded
    mutable QMap<QModelIndex, ExpandingType> m_expandState;
    QMap<QModelIndex, QPointer<QWidget>> m_expandingWidgets; // Map rows to their expanding-widgets
};

/**
 * Helper-function to merge custom-highlighting variant-lists.
 *
 * @param strings A list of strings that should be merged
 * @param highlights One variant-list for highlighting, as described in the kde header ktextedtor/codecompletionmodel.h
 * @param gapBetweenStrings How many signs are inserted between 2 strings?
 * */
QList<QVariant> mergeCustomHighlighting(QStringList strings, QList<QVariantList> highlights, int gapBetweenStrings = 0);
#endif
