/*
    SPDX-FileCopyrightText: 2007-2008 David Nolden <david.nolden.kdevelop@art-master.de>
    SPDX-FileCopyrightText: 2005-2006 Hamish Rodda <rodda@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef KTEXTEDITOR_CODECOMPLETIONMODEL_H
#define KTEXTEDITOR_CODECOMPLETIONMODEL_H

#include <QModelIndex>
#include <ktexteditor/range.h>
#include <ktexteditor_export.h>

namespace KTextEditor
{
class Document;
class View;

/*!
 * \class KTextEditor::CodeCompletionModel
 * \inmodule KTextEditor
 * \inheaderfile KTextEditor/CodeCompletionModel
 *
 * \brief An item model for providing code completion, and meta information for
 * enhanced presentation.
 *
 * Introduction
 *
 * The CodeCompletionModel is the actual workhorse to provide code completions
 * in a KTextEditor::View. It is meant to be used in conjunction with the
 * View. The CodeCompletionModel is not meant to be used as
 * is. Rather you need to implement a subclass of CodeCompletionModel to actually
 * generate completions appropriate for your type of Document.
 *
 * Implementing a CodeCompletionModel
 *
 * The CodeCompletionModel is a QAbstractItemModel, and can be subclassed in the
 * same way. It provides default implementations of several members, however, so
 * in most cases (if your completions are essentially a non-hierarchical, flat list
 * of matches) you will only need to overload few virtual functions.
 *
 * Implementing a CodeCompletionModel for a flat list
 *
 * For the simple case of a flat list of completions, you will need to:
 * \list
 * \li Implement completionInvoked() to actually generate/update the list of completion
 * matches
 * \li implement itemData() (or QAbstractItemModel::data()) to return the information that
 * should be displayed for each match.
 * \li use setRowCount() to reflect the number of matches.
 * \endlist
 *
 * Columns and roles
 *
 * TODO document the meaning and usage of the columns and roles used by the
 *
 * Completion Interface
 *
 * Using the new CodeCompletionModel
 *
 * To start using your KTextEditor::View Completion Interface.
 *
 * ControllerInterface to get more control
 *
 * To have more control over code completion implement
 * CodeCompletionModelControllerInterface in your CodeCompletionModel.
 *
 * \sa KTextEditor::CodeCompletionModelControllerInterface
 */
class KTEXTEDITOR_EXPORT CodeCompletionModel : public QAbstractItemModel
{
    Q_OBJECT

public:
    /*!
     *
     */
    explicit CodeCompletionModel(QObject *parent);
    ~CodeCompletionModel() override;

    /*!
       \enum KTextEditor::CodeCompletionModel::Columns

       \value Prefix

       \value Icon
       Icon representing the type of completion. We have a separate icon field
       so that names remain aligned where only some completions have icons,
       and so that they can be rearranged by the user.

       \value Scope

       \value Name

       \value Arguments

       \value Postfix
     */
    enum Columns {
        Prefix = 0,
        Icon,
        Scope,
        Name,
        Arguments,
        Postfix
    };
    static const int ColumnCount = Postfix + 1;

    /*!
       \enum KTextEditor::CodeCompletionModel::CompletionProperty

       \value NoProperty

       \value FirstProperty

       \value Public

       \value Protected

       \value Private

       \value Static

       \value Const

       \value Namespace

       \value Class

       \value Struct

       \value Union

       \value Function

       \value Variable

       \value Enum

       \value Template

       \value TypeAlias

       \value Virtual

       \value Override

       \value Inline

       \value Friend

       \value Signal

       \value Slot

       \value LocalScope

       \value NamespaceScope

       \value GlobalScope

       \value LastProperty
     */
    enum CompletionProperty {
        NoProperty = 0x0,
        FirstProperty = 0x1,

        // Access specifiers - no more than 1 per item
        Public = 0x1,
        Protected = 0x2,
        Private = 0x4,

#if KTEXTEDITOR_ENABLE_DEPRECATED_SINCE(6, 19)
        // Extra access specifiers - any number per item
        Static = 0x8,
        Const = 0x10,

        // Type - no more than 1 per item (except for Template)
        Namespace = 0x20,
        Class = 0x40,
        Struct = 0x80,
        Union = 0x100,
        Function = 0x200,
        Variable = 0x400,
        Enum = 0x800,
        Template = 0x1000,
        TypeAlias = 0x2000,

        // Special attributes - any number per item
        Virtual = 0x4000,
        Override = 0x8000,
        Inline = 0x10000,
        Friend = 0x20000,
        Signal = 0x40000,
        Slot = 0x80000,
#endif

        // Scope - no more than 1 per item
        LocalScope = 0x100000,
        NamespaceScope = 0x200000,
        GlobalScope = 0x400000,

        // Keep this in sync so the code knows when to stop
        LastProperty = GlobalScope
    };
    Q_DECLARE_FLAGS(CompletionProperties, CompletionProperty)

    /*!
       \enum KTextEditor::CodeCompletionModel::HighlightMethod

       \value NoHighlighting

       \value InternalHighlighting

       \value CustomHighlighting
     */
    enum HighlightMethod {
        NoHighlighting = 0x0,
        InternalHighlighting = 0x1,
        CustomHighlighting = 0x2
    };
    Q_DECLARE_FLAGS(HighlightMethods, HighlightMethod)

    /*!
       \enum KTextEditor::CodeCompletionModel::ExtraItemDataRoles

       Meta information is passed through extra {Qt::ItemDataRole}s.
       This information should be returned when requested on the Name column.

       \value CompletionRole
       The model should return a set of CompletionProperties.

       \value ScopeIndex
       The model should return an index to the scope -1 represents no scope.
       TODO how to sort scope?

       \value MatchQuality
       If requested, your model should try to determine whether the
       completion in question is a suitable match for the context (ie.
       is accessible, exported, + returns the data type required).
       The returned data should ideally be matched against the argument-hint context
       set earlier by SetMatchContext.
       Return an integer value that should be positive if the completion is suitable,
       and zero if the completion is not suitable. The value should be between 0 an 10, where
       10 means perfect match.
       Return QVariant::Invalid if you are unable to determine this.

       \value SetMatchContext
       Is requested before MatchQuality(..) is requested. The item on which this is requested
       is an argument-hint item When this role is requested, the item should
       be noted, and whenever MatchQuality is requested, it should be computed by matching the item given
       with MatchQuality into the context chosen by SetMatchContext.
       Feel free to ignore this, but ideally you should return QVariant::Invalid to make clear that your model does not support this.
       \sa ArgumentHintDepth.

       \value HighlightingMethod
       Define which highlighting method will be used:
       \list
       \li QVariant::Invalid - allows the editor to choose (usually internal highlighting)
       \li QVariant::Integer - highlight as specified by HighlightMethods.
       \endlist

       \value CustomHighlight
       Allows an item to provide custom highlighting.
       \br
       Return a QList\<QVariant\> in the following format (repeat this triplet
       as many times as required, however each column must be >= the previous,
       and startColumn != endColumn):
       \list
          \li int startColumn (where 0 = start of the completion entry)
          \li int endColumn (note: not length)
          \li QTextFormat attribute (note: attribute may be a KTextEditor::Attribute, as it is a child class)
              If the attribute is invalid, and the item is an argument-hint, the text will be drawn with
              a background-color depending on match-quality, or yellow.
              You can use that to mark the actual arguments that are matched in an argument-hint.
       \endlist

       \value InheritanceDepth
       Returns the inheritance depth of the completion.  For example, a completion
       which comes from the base class would have depth 0, one from a parent class
       would have depth 1, one from that class' parent 2, etc. you can use this to
       symbolize the general distance of a completion-item from a user. It will be used
       for sorting.

       \value IsExpandable
       This allows items in the completion-list to be expandable. If a model returns an QVariant bool value
       that evaluates to true, the completion-widget will draw a handle to expand the item, and will also make
       that action accessible through keyboard.

       \value ExpandingWidget
       After a model returned true for a row on IsExpandable, the row may be expanded by the user.
       When this happens, ExpandingWidget is requested.
       The model may return two types of values:
       \list
       \li QWidget*:
        If the model returns a QVariant of type QWidget*, the code-completion takes over the given widget
        and embeds it into the completion-list under the completion-item. Ownership is transferred to completion-widget
        The completion-widget will use the height of the widget as a hint for its preferred size, but it will
        resize the widget at will.
       \li QString:
        If the mode returns a QVariant of type QString, it will create a small html-widget showing the given html-code,
        and embed it into the completion-list under the completion-item.

       \value ItemSelected
       Whenever an item is selected, this will be requested from the underlying model.
       It may be used as a simple notification that the item was selected.
       Above that, the model may return a QString, which then should then contain html-code. A html-widget
       will then be displayed as a one- or two-liner under the currently selected item(it will be partially expanded)

       \value ArgumentHintDepth
       Is this completion-item an argument-hint?
       The model should return an integral positive number if the item is an argument-hint, and QVariant() or 0 if it is not one.
       The returned depth-integer is important for sorting and matching.
       \br
       Example:
       "otherFunction(function1(function2("
       \br
       all functions named function2 should have ArgumentHintDepth 1, all functions found for function1 should have ArgumentHintDepth 2,
       and all functions named otherFunction should have ArgumentHintDepth 3
       \br
       Later, a completed item may then be matched with the first argument of function2, the return-type of function2 with the first
       argument-type of function1, and the return-type of function1 with the argument-type of otherFunction.
       \br
       If the model returns a positive value on this role for a row, the content will be treated specially:
       \list
       \li It will be shown in a separate argument-hint list
       \li It will be sorted by Argument-hint depth
       \li Match-qualities will be illustrated by differently highlighting the matched argument if possible
       The argument-hint list strings will be built from all source-model, with a little special behavior:
       \list
          \li Prefix - should be all text of the function-signature up to left of the matched argument of the function
          \li Name - should be the type and name of the function's matched argument. This part will be highlighted differently depending on the match-quality
          \li Suffix - should be all the text of the function-signature behind the matched argument
              \br
              Example: You are matching a function with signature "void test(int param1, int param2)", and you are matching the first argument.
              \br
              The model should then return:
              \br
              Prefix "void test("
              Name: "int param1"
              Suffix: ", int param2)"
              \br
              If you don't use the highlighting, matching, etc. you can also return the columns in the usual way.
          \endlist
       \endlist

       \value BestMatchesCount
       This will be requested for each item to ask whether it should be included in computing a best-matches list.
       If you return a valid positive integer n here, the n best matches will be listed at top of the completion-list separately.
       \br
       This is expensive because all items of the whole completion-list will be tested for their matching-quality, with each of the level 1
       argument-hints.
       \br
       For that reason the end-user should be able to disable this feature.

       \value AccessibilityNext
       The following three enumeration-values are only used on expanded completion-list items that contain an expanding-widget.
       \br
       You can use them to allow the user to interact with the widget by keyboard.
       \br
       AccessibilityNext will be requested on an item if it is expanded, contains an expanding-widget, and the user triggers a special navigation
       short-cut to go to navigate to the next position within the expanding-widget(if applicable).
       \br
       Return QVariant(true) if the input was used.
       \sa ExpandingWidget

       \value AccessibilityPrevious
       AccessibilityPrevious will be requested on an item if it is expanded, contains an expanding-widget, and the user triggers a special navigation
       short-cut to go to navigate to the previous position within the expanding-widget(if applicable).
       \br
       Return QVariant(true) if the input was used.

       \value AccessibilityAccept
       AccessibilityAccept will be requested on an item if it is expanded, contains an expanding-widget, and the user triggers a special
       shortcut to trigger the action associated with the position within the expanding-widget the user has navigated to using AccessibilityNext and
       AccessibilityPrevious.
       \br
       This should return QVariant(true) if an action was triggered, else QVariant(false) or QVariant().

       \value GroupRole
       Using this Role, it is possible to greatly optimize the time needed to process very long completion-lists.
       \br
       In the completion-list, the items are usually ordered by some properties like argument-hint depth,
       inheritance-depth and attributes. Kate usually has to query the completion-models for these values
       for each item in the completion-list in order to extract the argument-hints and correctly sort the
       completion-list. However, with a very long completion-list, only a very small fraction of the items is actually
       visible.
       \br
       By using a tree structure you can give the items in a grouped order to kate, so it does not need to look at each
       item and query data in order to initially show the completion-list.
       \br
       This is how it works:
       \list
          \li You create a tree-structure for your items
          \li Every inner node of the tree defines one attribute value that all sub-nodes have in common.
            \list
              \li When the inner node is queried for GroupRole, it should return the "ExtraItemDataRoles" that all sub-nodes have in common
              \li When the inner node is then queried for that exact role, it should return that value.
              \li No other queries will be done to inner nodes.
              \endlist
          \li Every leaf node stands for an actual item in the completion list.
          \li The recommended grouping order is: Argument-hint depth, inheritance depth, attributes.
          This role can also be used to define completely custom groups, bypassing the editors builtin grouping:
          \list
            \li Return Qt::DisplayRole when GroupRole is requested
            \li Return the label text of the group when Qt::DisplayRole
            \li Optional: Return an integer sorting-value when InheritanceDepth is  requested. This number will
                        be used to determine the order of the groups. The order of the builtin groups is:
               1=Best Matches, 100=Local Scope, 200=Public, 300=Protected, 400=Private, 500=Namespace, 600=Global
               \br
               You can pick any arbitrary number to position your group relative to these builtin groups.
          \endlist
       \endlist

       \value UnimportantItemRole
       Return a nonzero value here to enforce sorting the item at the end of the list.

       \value LastExtraItemDataRole
     */
    enum ExtraItemDataRoles {
        CompletionRole = Qt::UserRole,
#if KTEXTEDITOR_ENABLE_DEPRECATED_SINCE(6, 19)
        ScopeIndex = Qt::UserRole + 1,
#endif
        MatchQuality = Qt::UserRole + 2,
        SetMatchContext = Qt::UserRole + 3,
        HighlightingMethod = Qt::UserRole + 4,
        CustomHighlight = Qt::UserRole + 5,
        InheritanceDepth = Qt::UserRole + 6,
#if KTEXTEDITOR_ENABLE_DEPRECATED_SINCE(6, 19)
        IsExpandable = Qt::UserRole + 7,
#endif
        ExpandingWidget = Qt::UserRole + 8,
#if KTEXTEDITOR_ENABLE_DEPRECATED_SINCE(6, 19)
        ItemSelected = Qt::UserRole + 9,
#endif
        ArgumentHintDepth = Qt::UserRole + 10,
        BestMatchesCount = Qt::UserRole + 11,
        AccessibilityNext = Qt::UserRole + 12,
        AccessibilityPrevious = Qt::UserRole + 13,
        AccessibilityAccept = Qt::UserRole + 14,
        GroupRole = Qt::UserRole + 15,
        UnimportantItemRole = Qt::UserRole + 16,
        LastExtraItemDataRole
    };

    void setRowCount(int rowCount);

    /*!
       \enum KTextEditor::CodeCompletionModel::InvocationType

       \value AutomaticInvocation

       \value UserInvocation

       \value ManualInvocation
     */
    enum InvocationType {
        AutomaticInvocation,
        UserInvocation,
        ManualInvocation
    };

    /*!
     * This function is responsible to generating / updating the list of current
     * completions. The default implementation does nothing.
     *
     * When implementing this function, remember to call setRowCount() (or implement
     * rowCount()), and to generate the appropriate change notifications (for instance
     * by calling QAbstractItemModel::reset()).
     *
     * \a view is the view to generate completions for
     *
     * \a range is the range of text to generate completions for
     *
     * \a invocationType describes how the code completion was triggered
     *
     */
    virtual void completionInvoked(KTextEditor::View *view, const KTextEditor::Range &range, InvocationType invocationType);

    /*!
     * This function is responsible for inserting a selected completion into the
     * view. The default implementation replaces the text that the completions
     * were based on with the Qt::DisplayRole of the Name column of the given match.
     *
     * \a view is the view to insert the completion into
     *
     * \a word is the Range that the completions are based on (what the user entered
     * so far)
     *
     * \a index identifies the completion match to insert
     *
     */
    virtual void executeCompletionItem(KTextEditor::View *view, const Range &word, const QModelIndex &index) const;

    // Reimplementations
    /*!
     * Reimplemented from QAbstractItemModel::columnCount(). The default implementation
     * returns ColumnCount for all indices.
     * */
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;
    /*!
     * Reimplemented from QAbstractItemModel::index(). The default implementation
     * returns a standard QModelIndex as long as the row and column are valid.
     * */
    QModelIndex index(int row, int column, const QModelIndex &parent = QModelIndex()) const override;
    /*!
     * Reimplemented from QAbstractItemModel::itemData(). The default implementation
     * returns a map with the QAbstractItemModel::data() for all roles that are used
     * by the CodeCompletionInterface. You will need to reimplement either this
     * function or QAbstractItemModel::data() in your CodeCompletionModel.
     * */
    QMap<int, QVariant> itemData(const QModelIndex &index) const override;
    /*!
     * Reimplemented from QAbstractItemModel::parent(). The default implementation
     * returns an invalid QModelIndex for all items. This is appropriate for
     * non-hierarchical / flat lists of completions.
     * */
    QModelIndex parent(const QModelIndex &index) const override;
    /*!
     * Reimplemented from QAbstractItemModel::rowCount(). The default implementation
     * returns the value set by setRowCount() for invalid (toplevel) indices, and 0
     * for all other indices. This is appropriate for non-hierarchical / flat lists
     * of completions
     * */
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;

    /*!
     * This function returns true if the model needs grouping, otherwise false.
     * The default is false if not changed via setHasGroups().
     */
    bool hasGroups() const;

Q_SIGNALS:

    /*!
     * Emit this if the code-completion for this model was invoked, some time is needed in order to get the data,
     * and the model is reset once the data is available.
     *
     * This only has an effect if emitted from within completionInvoked(..).
     *
     * This prevents the code-completion list from showing until this model is reset,
     * so there is no annoying flashing in the user-interface resulting from other models
     * supplying their data earlier.
     *
     * \note The implementation may choose to show the completion-list anyway after some timeout
     *
     * \warning If you emit this, you _must_ also reset the model at some point,
     *                  else the code-completion will be completely broken to the user.
     *                  Consider that there may always be additional completion-models apart from yours.
     *
     * \since 4.3
     */
    void waitForReset();

    void hasGroupsChanged(KTextEditor::CodeCompletionModel *model, bool hasGroups);

protected:
    void setHasGroups(bool hasGroups);

private:
    class CodeCompletionModelPrivate *const d;
};

Q_DECLARE_OPERATORS_FOR_FLAGS(CodeCompletionModel::CompletionProperties)
Q_DECLARE_OPERATORS_FOR_FLAGS(CodeCompletionModel::HighlightMethods)

}

#endif // KTEXTEDITOR_CODECOMPLETIONMODEL_H
