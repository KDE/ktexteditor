/*
    SPDX-FileCopyrightText: 2005-2006 Hamish Rodda <rodda@kde.org>
    SPDX-FileCopyrightText: 2007-2008 David Nolden <david.nolden.kdevelop@art-master.de>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef KATECOMPLETIONMODEL_H
#define KATECOMPLETIONMODEL_H

#include <QAbstractProxyModel>
#include <QList>
#include <QPair>

#include <ktexteditor/codecompletionmodel.h>

#include "expandingtree/expandingwidgetmodel.h"
#include <ktexteditor_export.h>

#include <set>

class KateCompletionWidget;
class KateArgumentHintModel;
namespace KTextEditor
{
class ViewPrivate;
}
class QWidget;
class QTextEdit;
class QTimer;
class HierarchicalModelHandler;

/**
 * This class has the responsibility for filtering, sorting, and manipulating
 * code completion data provided by a CodeCompletionModel.
 *
 * @author Hamish Rodda <rodda@kde.org>
 */
class KTEXTEDITOR_EXPORT KateCompletionModel : public ExpandingWidgetModel
{
    Q_OBJECT

public:
    enum InternalRole {
        IsNonEmptyGroup = KTextEditor::CodeCompletionModel::LastExtraItemDataRole + 1,
    };

    explicit KateCompletionModel(KateCompletionWidget *parent = nullptr);
    ~KateCompletionModel() override;

    QList<KTextEditor::CodeCompletionModel *> completionModels() const;
    void clearCompletionModels();
    void addCompletionModel(KTextEditor::CodeCompletionModel *model);
    void setCompletionModel(KTextEditor::CodeCompletionModel *model);
    void setCompletionModels(const QList<KTextEditor::CodeCompletionModel *> &models);
    void removeCompletionModel(KTextEditor::CodeCompletionModel *model);

    KTextEditor::ViewPrivate *view() const;
    KateCompletionWidget *widget() const;

    QString currentCompletion(KTextEditor::CodeCompletionModel *model) const;
    void setCurrentCompletion(QMap<KTextEditor::CodeCompletionModel *, QString> currentMatch);

    int translateColumn(int sourceColumn) const;

    /// Returns a common prefix for all current visible completion entries
    /// If there is no common prefix, extracts the next useful prefix for the selected index
    QString commonPrefix(QModelIndex selectedIndex) const;

    void rowSelected(const QModelIndex &row) const;

    bool indexIsItem(const QModelIndex &index) const override;

    int columnCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    Qt::ItemFlags flags(const QModelIndex &index) const override;
    bool hasChildren(const QModelIndex &parent = QModelIndex()) const override;
    virtual bool hasIndex(int row, int column, const QModelIndex &parent = QModelIndex()) const;
    QModelIndex index(int row, int column, const QModelIndex &parent = QModelIndex()) const override;

    // Disabled in case of bugs, reenable once fully debugged.
    // virtual QMap<int, QVariant> itemData ( const QModelIndex & index ) const;
    QModelIndex parent(const QModelIndex &index) const override;
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;

    /// Maps from this display-model into the appropriate source code-completion model
    virtual QModelIndex mapToSource(const QModelIndex &proxyIndex) const;

    /// Maps from an index in a source-model to the index of the item in this display-model
    virtual QModelIndex mapFromSource(const QModelIndex &sourceIndex) const;

    enum gm { ScopeType = 0x1, Scope = 0x2, AccessType = 0x4, ItemType = 0x8 };

    enum { // An own property that will be used to mark the best-matches group internally
        BestMatchesProperty = 2 * KTextEditor::CodeCompletionModel::LastProperty
    };

    Q_DECLARE_FLAGS(GroupingMethods, gm)

    static const int ScopeTypeMask = 0x380000;
    static const int AccessTypeMask = 0x7;
    static const int ItemTypeMask = 0xfe0;

    void debugStats();

    /// Returns whether one of the filtered items exactly matches its completion string
    bool shouldMatchHideCompletionList() const;

    uint filteredItemCount() const;

protected:
    int contextMatchQuality(const QModelIndex &index) const override;

Q_SIGNALS:
    void expandIndex(const QModelIndex &index);
    // Emitted whenever something has changed about the group of argument-hints
    void argumentHintsChanged();

private Q_SLOTS:
    void slotRowsInserted(const QModelIndex &parent, int start, int end);
    void slotRowsRemoved(const QModelIndex &parent, int start, int end);
    void slotModelReset();

    // Updates the best-matches group
    void updateBestMatches();
    // Makes sure that the ungrouped group contains each item only once
    // Must only be called right after the group was created
    void makeGroupItemsUnique(bool onlyFiltered = false);

private:
    typedef QPair<KTextEditor::CodeCompletionModel *, QModelIndex> ModelRow;
    virtual int contextMatchQuality(const ModelRow &sourceRow) const;

    QTreeView *treeView() const override;

    friend class KateArgumentHintModel;
    KTEXTEDITOR_NO_EXPORT
    static ModelRow modelRowPair(const QModelIndex &index);

    // Represents a source row; provides sorting method
    class KTEXTEDITOR_NO_EXPORT Item
    {
    public:
        Item(bool doInitialMatch, KateCompletionModel *model, const HierarchicalModelHandler &handler, ModelRow sourceRow);

        bool isValid() const;
        // Returns true if the item is not filtered and matches the current completion string
        bool isVisible() const;

        enum MatchType { NoMatch = 0, PerfectMatch, StartsWithMatch, AbbreviationMatch, ContainsMatch };
        MatchType match();

        const ModelRow &sourceRow() const;

        // Sorting operator
        bool operator<(const Item &rhs) const;

        bool haveExactMatch() const
        {
            return m_haveExactMatch;
        }

        void clearExactMatch()
        {
            m_haveExactMatch = false;
        }

        QString name() const
        {
            return m_nameColumn;
        }

    private:
        KateCompletionModel *model;
        ModelRow m_sourceRow;

        QString m_nameColumn;

        int inheritanceDepth;

        // True when currently matching completion string
        MatchType matchCompletion;
        bool m_haveExactMatch;
        bool m_unimportant;
    };

public:
    // Grouping and sorting of rows
    class Group
    {
    public:
        explicit Group(const QString &title, int attribute, KateCompletionModel *model);

        void addItem(const Item &i, bool notifyModel = false);
        /// Removes the item specified by \a row.  Returns true if a change was made to rows.
        bool removeItem(const ModelRow &row);
        void resort();
        void clear();
        // Returns whether this group should be ordered before other
        bool orderBefore(Group *other) const;
        // Returns a number that can be used for ordering
        int orderNumber() const;

        /// Returns the row in the this group's filtered list of the given model-row in a source-model
        ///-1 if the item is not in the filtered list
        ///@todo Implement an efficient way of doing this map, that does _not_ iterate over all items!
        int rowOf(const ModelRow &item)
        {
            for (int a = 0; a < (int)filtered.size(); ++a) {
                if (filtered[a].sourceRow() == item) {
                    return a;
                }
            }
            return -1;
        }

        KateCompletionModel *model;
        int attribute;
        QString title, scope;
        std::vector<Item> filtered;
        std::vector<Item> prefilter;
        bool isEmpty;
        //-1 if none was set
        int customSortingKey;
    };

    typedef std::set<Group *> GroupSet;

    bool hasGroups() const;

private:
    KTEXTEDITOR_NO_EXPORT
    QString commonPrefixInternal(const QString &forcePrefix) const;
    /// @note performs model reset
    KTEXTEDITOR_NO_EXPORT
    void createGroups();
    /// Creates all sub-items of index i, or the item corresponding to index i. Returns the affected groups.
    /// i must be an index in the source model
    KTEXTEDITOR_NO_EXPORT
    GroupSet createItems(const HierarchicalModelHandler &, const QModelIndex &i, bool notifyModel = false);
    /// Deletes all sub-items of index i, or the item corresponding to index i. Returns the affected groups.
    /// i must be an index in the source model
    KTEXTEDITOR_NO_EXPORT
    GroupSet deleteItems(const QModelIndex &i);
    KTEXTEDITOR_NO_EXPORT
    Group *createItem(const HierarchicalModelHandler &, const QModelIndex &i, bool notifyModel = false);
    /// @note Make sure you're in a {begin,end}ResetModel block when calling this!
    KTEXTEDITOR_NO_EXPORT
    void clearGroups();
    KTEXTEDITOR_NO_EXPORT
    void hideOrShowGroup(Group *g, bool notifyModel = false);
    /// When forceGrouping is enabled, all given attributes will be used for grouping, regardless of the completion settings.
    KTEXTEDITOR_NO_EXPORT
    Group *fetchGroup(int attribute, bool forceGrouping = false);
    // If this returns nonzero on an index, the index is the header of the returned group
    KTEXTEDITOR_NO_EXPORT
    Group *groupForIndex(const QModelIndex &index) const;
    inline Group *groupOfParent(const QModelIndex &child) const
    {
        return static_cast<Group *>(child.internalPointer());
    }
    KTEXTEDITOR_NO_EXPORT
    QModelIndex indexForRow(Group *g, int row) const;
    KTEXTEDITOR_NO_EXPORT
    QModelIndex indexForGroup(Group *g) const;

    enum changeTypes { Broaden, Narrow, Change };

    // Returns whether the model needs to be reset
    KTEXTEDITOR_NO_EXPORT
    void changeCompletions(Group *g);

    KTEXTEDITOR_NO_EXPORT
    bool hasCompletionModel() const;

    /// Removes attributes not used in grouping from the input \a attribute
    KTEXTEDITOR_NO_EXPORT
    int groupingAttributes(int attribute) const;
    KTEXTEDITOR_NO_EXPORT
    static int countBits(int value);

    KTEXTEDITOR_NO_EXPORT
    void resort();

    static bool matchesAbbreviation(const QString &word, const QString &typed, int &score);
    // exported for completion_test

    bool m_hasGroups = false;

    // ### Runtime state
    // General
    QList<KTextEditor::CodeCompletionModel *> m_completionModels;
    QMap<KTextEditor::CodeCompletionModel *, QString> m_currentMatch;

    // Column merging
    QList<QList<int>> m_columnMerges;

    QTimer *m_updateBestMatchesTimer;

    Group *m_ungrouped;
    Group *m_argumentHints; // The argument-hints will be passed on to another model, to be shown in another widget
    Group *m_bestMatches; // A temporary group used for holding the best matches of all visible items

    // Storing the sorted order
    QList<Group *> m_rowTable;
    QList<Group *> m_emptyGroups;
    // Quick access to each specific group (if it exists)
    QMultiHash<int, Group *> m_groupHash;
    // Maps custom group-names to their specific groups
    QHash<QString, Group *> m_customGroupHash;

    friend class CompletionTest;
};

Q_DECLARE_OPERATORS_FOR_FLAGS(KateCompletionModel::GroupingMethods)

#endif
