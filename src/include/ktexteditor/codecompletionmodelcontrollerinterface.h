/*
    SPDX-FileCopyrightText: 2008 Niko Sams <niko.sams@gmail.com>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef KTEXTEDITOR_CODECOMPLETIONMODELCONTROLLERINTERFACE_H
#define KTEXTEDITOR_CODECOMPLETIONMODELCONTROLLERINTERFACE_H

#include "codecompletionmodel.h"
#include <ktexteditor/cursor.h>
#include <ktexteditor_export.h>

class QModelIndex;

namespace KTextEditor
{
class View;

/*!
 * \class KTextEditor::CodeCompletionModelControllerInterface
 * \inmodule KTextEditor
 * \inheaderfile KTextEditor/CodeCompletionModelControllerInterface
 *
 * \brief Controller interface for a CodeCompletionModel instance.
 *
 * The CodeCompletionModelControllerInterface gives a CodeCompletionModel better
 * control over the completion.
 *
 * By implementing methods defined in this interface you can:
 * \list
 * \li control when automatic completion should start - see shouldStartCompletion()
 * \li define a custom completion range (that will be replaced when the completion
 *   is executed) - see completionRange()
 * \li dynamically modify the completion range during completion -see updateCompletionRange()
 * \li specify the string used for filtering the completion - see filterString()
 * \li control when completion should stop - see shouldAbortCompletion()
 * \endlist
 *
 * When the interface is not implemented, or no methods are overridden the
 * default behaviour is used, which will be correct in many situations.
 *
 *
 * Implementing the Interface
 * To use this class implement it in your CodeCompletionModel.
 *
 * \code
 * class MyCodeCompletion : public KTextEditor::CodeCompletionTestModel,
 *                  public KTextEditor::CodeCompletionModelControllerInterface
 * {
 *   Q_OBJECT
 *   Q_INTERFACES(KTextEditor::CodeCompletionModelControllerInterface)
 * public:
 *   KTextEditor::Range completionRange(KTextEditor::View* view, const KTextEditor::Cursor &position);
 * };
 * \endcode
 *
 * \sa CodeCompletionModel
 * \author Joseph Wenninger
 * \ingroup kte_group_ccmodel_extensions
 */
class KTEXTEDITOR_EXPORT CodeCompletionModelControllerInterface
{
public:
    /*!
     *
     */
    CodeCompletionModelControllerInterface();
    virtual ~CodeCompletionModelControllerInterface();

    /*!
     * This function decides if the automatic completion should be started when
     * the user entered some text.
     *
     * The default implementation will return true if the last character in
     *
     * \a insertedText is a letter, a number, '.', '_' or '\>'
     *
     * \a view is the view to generate completions for
     *
     * \a insertedText is the text that was inserted by the user
     *
     * \a userInsertion is true if the text was inserted by the user using typing.
     * If false, it may have been inserted for example by code-completion.
     *
     * \a position is the current cursor position
     *
     * Returns \e true, if the completion should be started, otherwise \e false
     */
    virtual bool shouldStartCompletion(View *view, const QString &insertedText, bool userInsertion, const Cursor &position);

    /*!
     * This function returns the completion range that will be used for the
     * current completion.
     *
     * This range will be used for filtering the completion list and will get
     * replaced when executing the completion
     *
     * The default implementation will work for most languages that don't have
     * special chars in identifiers. Since 5.83 the default implementation takes
     * into account the wordCompletionRemoveTail configuration option, if that
     * option is enabled the whole word the cursor is inside is replaced with the
     * completion, however if it's disabled only the text on the left of the cursor
     * will be replaced with the completion.
     *
     * \a view is the view to generate completions for
     *
     * \a position is the current cursor position
     *
     * Returns the completion range
     */
    virtual Range completionRange(View *view, const Cursor &position);

    /*!
     * This function lets the CompletionModel dynamically modify the range.
     * Called after every change to the range (eg. when user entered text)
     *
     * The default implementation does nothing.
     *
     * \a view is the view to generate completions for
     *
     * \a range are reference to the current range
     *
     * Returns the modified range
     */
    virtual Range updateCompletionRange(View *view, const Range &range);

    /*!
     * This function returns the filter-text used for the current completion.
     * Can return an empty string to disable filtering.
     *
     * The default implementation will return the text from \a range start to
     * the cursor \a position.
     *
     * \a view is the view to generate completions for
     *
     * \a range is the completion range
     *
     * \a position is the current cursor position
     *
     * Returns the string used for filtering the completion list
     */
    virtual QString filterString(View *view, const Range &range, const Cursor &position);

    /*!
     * This function decides if the completion should be aborted.
     * Called after every change to the range (eg. when user entered text)
     *
     * The default implementation will return true when any special character was entered, or when the range is empty.
     *
     * \a view is the view to generate completions for
     *
     * \a range is the completion range
     *
     * \a currentCompletion is the text typed so far
     *
     * Returns \e true, if the completion should be aborted, otherwise \e false
     */
    virtual bool shouldAbortCompletion(View *view, const Range &range, const QString &currentCompletion);

    /*!
     * When an item within this model is currently selected in the completion-list, and the user inserted the given character,
     * should the completion-item be executed? This can be used to execute items from other inputs than the return-key.
     * For example a function item could be executed by typing '(', or variable items by typing '.'.
     *
     * \a selected is the currently selected index
     *
     * \a inserted is the character that was inserted by tue user
     *
     */
    virtual bool shouldExecute(const QModelIndex &selected, QChar inserted);

    /*!
     * Notification that completion for this model has been aborted.
     *
     * \a view is the view in which the completion for this model was aborted
     *
     */
    virtual void aborted(View *view);

    /*!
       \enum KTextEditor::CodeCompletionModelControllerInterface::MatchReaction

       \value None
       \value HideListIfAutomaticInvocation
       If this is returned, the completion-list is hidden if it was invoked automatically
       \value ForExtension
     */
    enum MatchReaction {
        None = 0,
        HideListIfAutomaticInvocation = 1,
        ForExtension = 0xffff
    };

    /*!
     * Called whenever an item in the completion-list perfectly matches the current filter text.
     *
     * \a matched is the index that is matched
     *
     * Returns Whether the completion-list should be hidden on this event. The default-implementation always returns HideListIfAutomaticInvocation
     */
    virtual MatchReaction matchingItem(const QModelIndex &matched);

    /*!
     * When multiple completion models are used at the same time, it may happen that multiple models add items with the same
     * name to the list. This option allows to hide items from this completion model when another model with higher priority
     * contains items with the same name.
     * Returns Whether items of this completion model should be hidden if another completion model has items with the same name
     */
    virtual bool shouldHideItemsWithEqualNames() const;
};

}

Q_DECLARE_INTERFACE(KTextEditor::CodeCompletionModelControllerInterface, "org.kde.KTextEditor.CodeCompletionModelControllerInterface")

#endif // KTEXTEDITOR_CODECOMPLETIONMODELCONTROLLERINTERFACE_H
