/*
    SPDX-FileCopyrightText: 2005 Christoph Cullmann <cullmann@kde.org>
    SPDX-FileCopyrightText: 2005-2006 Dominik Haumann <dhdev@gmx.de>
    SPDX-FileCopyrightText: 2008 Erlend Hamberg <ehamberg@gmail.com>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef KTEXTEDITOR_COMMAND_H
#define KTEXTEDITOR_COMMAND_H

#include <KCompletion>
#include <ktexteditor/range.h>
#include <ktexteditor_export.h>

#include <QObject>
#include <QStringList>

class KCompletion;

namespace KTextEditor
{
class View;

/*!
 * \class KTextEditor::Command
 * \inmodule KTextEditor
 * \inheaderfile KTextEditor/Command
 *
 * \brief An Editor command line command.
 *
 * Introduction
 *
 * The Command class represents a command for the editor command line. A
 * command simply consists of a string, for example \c find. The command
 * auto-registers itself at the Editor. The Editor itself queries
 * the command for a list of accepted strings/commands by calling cmds().
 * If the command gets invoked the function exec() is called, i.e. you have
 * to implement the \c reaction in exec(). Whenever the user needs help for
 * a command help() is called.
 *
 * Command Information
 *
 * To provide reasonable information about a specific command there are the
 * following accessor functions for a given command string:
 * \list
 *  \li name() returns a label
 *  \li description() returns a descriptive text
 *  \li category() returns a category into which the command fits.
 * \endlist
 *
 * These getters allow KTextEditor implementations to plug commands into menus
 * and toolbars, so that a user can assign shortcuts.
 *
 * Command Completion
 *
 * The Command optionally can show a completion popup to help the user select
 * a valid entry as first parameter to the Command. To this end, return a
 * valid completion item by reiplementing completionObject().
 *
 * The returned completion object is deleted automatically once it is not needed
 * anymore. Therefore neither delete the completion object yourself nor return
 * the same completion object twice.
 *
 * Interactive Commands
 *
 * In case the Command needs to interactively process the text of the parameters,
 * override wantsToProcessText() by returning \c true and reimplement processText().
 *
 * A typical example of an iterative command would be the incremental search.
 *
 * \sa KTextEditor::CommandInterface
 */
class KTEXTEDITOR_EXPORT Command : public QObject
{
    Q_OBJECT

public:
    /*!
     * Constructor with \a parent.
     * Will register this command for the commands names given in \a cmds at the global editor instance.
     */
    Command(const QStringList &cmds, QObject *parent = nullptr);

    /*!
     * Virtual destructor.
     * Will unregister this command at the global editor instance.
     */
    ~Command() override;

public:
    /*!
     * Return a list of strings a command may begin with.
     * This is the same list the command was constructed with.
     * A string is the start part of a pure text which can be handled by this
     * command, i.e. for the command s/sdl/sdf/g the corresponding string is
     * simply \c s, and for char:1212 simply \c char.
     * Returns list of supported commands
     */
    inline const QStringList &cmds() const
    {
        return m_cmds;
    }

    /*!
     * Find out if a given command can act on a range. This is used for checking
     * if a command should be called when the user also gave a range or if an
     * error should be raised.
     *
     * \a cmd is the command
     *
     * Returns \c true if command supports acting on a range of lines, \c false
     *          not. The default implementation returns \c false.
     */
    virtual bool supportsRange(const QString &cmd);

    /*!
     * Execute the command for the given \a view and \a cmd string.
     * Return the success value and a \a msg for status. As example we
     * consider a replace command. The replace command would return the number
     * of replaced strings as \a msg, like "16 replacements made." If an error
     * occurred in the usage it would return \c false and set the \a msg to
     * something like "missing argument." or such.
     *
     * If a non-invalid \a range is given, the command shall be executed on that range.
     * supportsRange() tells if the command supports that.
     *
     * Returns \c true on success, otherwise \c false
     */
    virtual bool exec(KTextEditor::View *view, const QString &cmd, QString &msg, const KTextEditor::Range &range = KTextEditor::Range::invalid()) = 0;

    /*!
     * Shows help for the given \a view and \a cmd string.
     *
     * If your command has a help text for \a cmd you have to return \c true
     * and set the \a msg to a meaningful text. The help text is embedded by
     * the Editor in a Qt::RichText enabled widget, e.g. a QToolTip.
     *
     * Returns \c true if your command has a help text, otherwise \c false
     */
    virtual bool help(KTextEditor::View *view, const QString &cmd, QString &msg) = 0;

    /*!
     * Return a KCompletion object that will substitute the command line
     * default one while typing the first argument of the command cmdname.
     * The text will be added to the command separated by one space character.
     *
     * Override this method if your command can provide a completion object.
     * The returned completion object is deleted automatically once it is not needed
     * anymore. Therefore neither delete the completion object yourself nor return
     * the same completion object twice.
     *
     * The default implementation returns a null pointer (\c nullptr).
     *
     * \a view is the view the command will work on
     *
     * \a cmdname is the command name associated with this request.
     *
     * Returns a valid completion object or \c nullptr, if a completion object is
     *         not supported
     */
    virtual KCompletion *completionObject(KTextEditor::View *view, const QString &cmdname);

    /*!
     * Check, whether the command wants to process text interactively for the
     * given command with name cmdname.
     *
     * If you return true, the command's processText() method is called
     * whenever the text in the command line changed.
     *
     * Reimplement this to return true, if your commands wants to process the
     * text while typing.
     *
     * \a cmdname is the command name associated with this query.
     *
     * Returns \c true, if your command wants to process text interactively,
     *         otherwise \c false
     *
     * \sa processText()
     */
    virtual bool wantsToProcessText(const QString &cmdname);

    /*!
     * This is called by the command line each time the argument text for the
     * command changed, if wantsToProcessText() returns \c true.
     *
     * \a view is the current view
     *
     * \a text is the current command text typed by the user
     *
     * \sa wantsToProcessText()
     */
    virtual void processText(KTextEditor::View *view, const QString &text);

private:
    /*
     * the command list this command got constructed with
     */
    const QStringList m_cmds;

    class CommandPrivate *const d;
};

}

#endif
