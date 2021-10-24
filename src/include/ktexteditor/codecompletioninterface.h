/*
    SPDX-FileCopyrightText: 2005-2006 Hamish Rodda <rodda@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef KTEXTEDITOR_CODECOMPLETIONINTERFACE_H
#define KTEXTEDITOR_CODECOMPLETIONINTERFACE_H

#include <QObject>
#include <ktexteditor/codecompletionmodel.h>
#include <ktexteditor/range.h>
#include <ktexteditor_export.h>

namespace KTextEditor
{
class CodeCompletionModel;

/**
 * \class CodeCompletionInterface codecompletioninterface.h <KTextEditor/CodeCompletionInterface>
 *
 * \brief Code completion extension interface for the View.
 *
 * \ingroup kte_group_view_extensions
 *
 * \section compiface_intro Introduction
 *
 * The CodeCompletionInterface is designed to provide code completion
 * functionality for a KTextEditor::View. This interface provides the basic
 * mechanisms to display a list of completions, update this list according
 * to user input, and allow the user to select a completion.
 *
 * Essentially, this provides an item view for the available completions. In
 * order to use this interface, you will need to implement a
 * CodeCompletionModel that generates the relevant completions given the
 * current input.
 *
 * \section compiface_access Accessing the CodeCompletionInterface
 *
 * The CodeCompletionInterface is an extension interface for a
 * View, i.e. the View inherits the interface \e provided that the
 * used KTextEditor library implements the interface. Use qobject_cast to
 * access the interface:
 * \code
 * // view is of type KTextEditor::View*
 * auto iface = qobject_cast<KTextEditor::CodeCompletionInterface*>(view);
 *
 * if (iface) {
 *     // the implementation supports the interface
 *     // do stuff
 * } else {
 *     // the implementation does not support the interface
 * }
 * \endcode
 *
 * \section compiface_usage Using the CodeCompletionInterface
 *
 * The CodeCompletionInterface can be used in different ways, which we
 * will call "automatic", and "manual".
 *
 * \subsection compiface_automatic Automatic
 *
 * In automatic mode, the CodeCompletionInterface will take care of
 * starting and aborting the generation of code completions as
 * appropriate, when the users inserts or changes text.
 *
 * To use the interface in this way, first register a CodeCompletionModel
 * using registerCompletionModel(). Now call setAutomaticCompletionEnabled()
 * to enabled automatic completions.
 *
 * \subsection compiface_manual Manual
 *
 * If you need more control over when code completions get shown or not,
 * or which fragment of the text should be considered as the basis for
 * generated completions, proceed as follows:
 *
 * Call setAutomaticCompletionEnabled(false) to disable automatic
 * completions. To start the generation of code completions for the current
 * word, call startCompletion(), with the appropriate parameters. To hide the
 * generated completions, use abortCompletion().
 *
 * \see KTextEditor::View, KTextEditor::CodeCompletionModel
 */
class KTEXTEDITOR_EXPORT CodeCompletionInterface
{
public:
    virtual ~CodeCompletionInterface();

    /**
     * Query whether the code completion box is currently displayed.
     */
    virtual bool isCompletionActive() const = 0;

    /**
     * Invoke code completion over a given range, with a specific \a model.
     */
    virtual void startCompletion(const Range &word, CodeCompletionModel *model) = 0;

    /**
     * Abort the currently displayed code completion without executing any currently
     * selected completion. This is safe, even when the completion box is not currently
     * active.
     * \see isCompletionActive()
     */
    virtual void abortCompletion() = 0;

    /**
     * Force execution of the currently selected completion, and hide the code completion
     * box.
     */
    virtual void forceCompletion() = 0;

    /**
     * Register a new code completion \p model.
     * \param model new completion model
     * \see unregisterCompletionModel()
     */
    virtual void registerCompletionModel(CodeCompletionModel *model) = 0;

    /**
     * Unregister a code completion \p model.
     * \param model the model that should be unregistered
     * \see registerCompletionModel()
     */
    virtual void unregisterCompletionModel(CodeCompletionModel *model) = 0;

    /**
     * Determine the status of automatic code completion invocation.
     */
    virtual bool isAutomaticInvocationEnabled() const = 0;

    /**
     * Enable or disable automatic code completion invocation.
     */
    virtual void setAutomaticInvocationEnabled(bool enabled = true) = 0;

    // Q_SIGNALS:
    // void completionExecuted(KTextEditor::View* view, const KTextEditor::Cursor& position, KTextEditor::CodeCompletionModel* model, int row);
    // void completionAborted(KTextEditor::View* view);
};

/**
 * \class CodeCompletionInterfaceV2 codecompletioninterface.h <KTextEditor/CodeCompletionInterface>
 *
 * \brief Code completion extension interface for the View, version 2.
 *
 * \ingroup kte_group_view_extensions
 *
 * \section compifacev2_intro Introduction
 *
 * The CodeCompletionInterfaceV2 is an extended version of the CodeCompletionInterface.
 * Refer to the base CodeCompletionInterface for a more detailed description.
 *
 * \section compifacev2_access Accessing the CodeCompletionInterfaceV2
 *
 * The CodeCompletionInterfaceV2 is an extension interface for a
 * View, i.e. the View inherits the interface \e provided that the
 * used KTextEditor library implements the interface. Use qobject_cast to
 * access the interface:
 * \code
 * // view is of type KTextEditor::View*
 * auto iface = qobject_cast<KTextEditor::CodeCompletionInterfaceV2*>(view);
 *
 * if (iface) {
 *     // the implementation supports the interface
 *     // do stuff
 * } else {
 *     // the implementation does not support the interface
 * }
 * \endcode
 *
 * \since 5.74
 */
class KTEXTEDITOR_EXPORT CodeCompletionInterfaceV2 : public CodeCompletionInterface
{
    // KF6: Merge KTextEditor::CodeCompletionInterfaceV2 into KTextEditor::CodeCompletionInterface
public:
    ~CodeCompletionInterfaceV2() override;
    /**
     * Invoke code completion over a given range, with specific models and invocation type.
     * \param models list of models to start. If this is an empty list, all registered models are started.
     */
    virtual void startCompletion(const Range &word,
                                 const QList<CodeCompletionModel *> &models = QList<CodeCompletionModel *>(),
                                 CodeCompletionModel::InvocationType invocationType = CodeCompletionModel::ManualInvocation) = 0;
    using CodeCompletionInterface::startCompletion;

    /**
     * Obtain the list of registered code completion models.
     * \returns a list of a models that are currently registered
     * \see registerCompletionModel(CodeCompletionModel*)
     */
    virtual QList<CodeCompletionModel *> codeCompletionModels() const = 0;
};

}

Q_DECLARE_INTERFACE(KTextEditor::CodeCompletionInterface, "org.kde.KTextEditor.CodeCompletionInterface")
Q_DECLARE_INTERFACE(KTextEditor::CodeCompletionInterfaceV2, "org.kde.KTextEditor.CodeCompletionInterfaceV2")

#endif // KTEXTEDITOR_CODECOMPLETIONINTERFACE_H
