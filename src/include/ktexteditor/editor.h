/* This file is part of the KDE libraries
 *  Copyright (C) 2005-2014 Christoph Cullmann <cullmann@kde.org>
 *  Copyright (C) 2005-2014 Dominik Haumann (dhaumann@kde.org)
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Library General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Library General Public License for more details.
 *
 *  You should have received a copy of the GNU Library General Public License
 *  along with this library; see the file COPYING.LIB.  If not, write to
 *  the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 *  Boston, MA 02110-1301, USA.*/

#ifndef KTEXTEDITOR_EDITOR_H
#define KTEXTEDITOR_EDITOR_H

#include <ktexteditor_export.h>

#include <QObject>

class KAboutData;
class KConfig;

/**
 * The KTextEditor namespace contains all the public API that is required
 * to use the KTextEditor component. Using the KTextEditor interfaces is
 * described in the article \ref index.
 *
 * @warning All classes that are \e not part of the KTextEditor namespace
 *          are internal and subject to change. We mean it!
 */
namespace KTextEditor
{

class Application;
class Command;
class Document;
class View;
class EditorPrivate;
class ConfigPage;


/**
 * \brief Accessor interface for the KTextEditor framework.
 *
 * Topics:
 *  - \ref editor_intro
 *  - \ref editor_config
 *  - \ref editor_commands
 *
 * \section editor_intro Introduction
 *
 * The Editor part can either be accessed through the static accessor Editor::instance()
 * or through the KParts component model (see \ref kte_design_part).
 * The Editor singleton provides general information and configuration methods
 * for the Editor, for example KAboutData by using aboutData().
 *
 * The Editor has a list of all opened documents. Get this list with documents().
 * To create a new Document call createDocument(). The signal documentCreated()
 * is emitted whenever the Editor created a new document.
 *
 * \section editor_config Editor Configuration
 *
 * The config dialog can be shown with configDialog().
 * Instead of using the config dialog, the config pages can also be embedded
 * into the application's config dialog. To do this, configPages() returns the
 * number of config pages that exist and configPage() returns the requested
 * page. The configuration are saved automatically by the Editor.
 *
 * \note It is recommended to embed the config pages into the main application's
 *       config dialog instead of using a separate config dialog, if the config
 *       dialog does not look cluttered then. This way, all settings are grouped
 *       together in one place.
 *
 * \section editor_commands Command Line Commands
 *
 * With Commands it is possible to add new commands to the command line.
 * These Command%s then are added to all document View%s.
 * Common use cases include commands like \e find or setting document variables.
 * The list of all registered commands can be obtained either through commandList()
 * or through commands(). Further, a specific command can be obtained through
 * queryCommand(). For further information, read the Command API documentation.
 *
 * \see KTextEditor::Document, KTextEditor::ConfigPage, KTextEditor::Command
 * \author Christoph Cullmann \<cullmann@kde.org\>
 */
class KTEXTEDITOR_EXPORT Editor : public QObject
{
    Q_OBJECT

protected:
    /**
     * Constructor.
     *
     * Create the Editor object and pass it the internal
     * implementation to store a d-pointer.
     *
     * @param impl d-pointer to use
     */
    Editor(EditorPrivate *impl);

    /**
     * Virtual destructor.
     */
    virtual ~Editor();

public:
    /**
     * Accessor to get the Editor instance.
     *
     * @note This object will stay alive until QCoreApplication terminates.
     *       You shall not delete it yourself.
     *       There is only ONE Editor instance of this per process.
     *
     * \return Editor controller, after initial construction, will
     *        live until QCoreApplication is terminating.
     */
    static Editor *instance();

public:
    /**
     * Set the global application object.
     * This will allow the editor component to access
     * the hosting application.
     * @param application application object
     *        if the argument is a nullptr, this will reset the application back to a dummy interface
     */
    virtual void setApplication(KTextEditor::Application *application) = 0;

    /**
     * Current hosting application, if any set.
     * @return current application object or a dummy interface that allows you to call the functions
     *         will never return a nullptr
     */
    virtual KTextEditor::Application *application() const = 0;

    /*
     * Methods to create and manage the documents.
     */
public:
    /**
     * Create a new document object with \p parent.
     *
     * For each created document, the signal documentCreated() is emitted.
     *
     * \param parent parent object
     * \return new KTextEditor::Document object
     * \see documents(), documentCreated()
     */
    virtual Document *createDocument(QObject *parent) = 0;

    /**
     * Get a list of all documents of this editor.
     * \return list of all existing documents
     * \see createDocument()
     */
    virtual QList<Document *> documents() = 0;

Q_SIGNALS:
    /**
     * The \p editor emits this signal whenever a \p document was successfully
     * created.
     * \param editor pointer to the Editor singleton which created the new document
     * \param document the newly created document instance
     * \see createDocument()
     */
    void documentCreated(KTextEditor::Editor *editor,
                         KTextEditor::Document *document);

    /*
     * General Information about this editor.
     */
public:
    /**
     * Get the about data of this Editor part.
     * \return about data
     */
    virtual const KAboutData &aboutData() const = 0;

    /**
     * Get the current default encoding for this Editor part.
     * \return default encoding
     */
    QString defaultEncoding() const;

    /*
     * Configuration management.
     */
public:
    /**
     * Show the editor's config dialog, changes will be applied to the
     * editor and the configuration changes are saved.
     *
     * \note Instead of using the config dialog, the config pages can be
     *       embedded into your own config dialog by using configPages() and
     *       configPage().
     * \param parent parent widget
     */
    virtual void configDialog(QWidget *parent) = 0;

    /**
     * Get the number of available config pages.
     * If a number < 1 is returned, it does not support config pages.
     * \return number of config pages
     * \see configPage()
     */
    virtual int configPages() const = 0;

    /**
     * Get the config page with the \p number, config pages from 0 to
     * configPages()-1 are available if configPages() > 0.
     * Configuration changes done over this widget are automatically
     * saved.
     * \param number index of config page
     * \param parent parent widget for config page
     * \return created config page or NULL, if the number is out of bounds
     * \see configPages()
     */
    virtual ConfigPage *configPage(int number, QWidget *parent) = 0;

public:
    /**
     * Query for the command \p cmd.
     * If the command \p cmd does not exist the return value is NULL.
     *
     * \param cmd name of command to query for
     * \return the found command or NULL if no such command exists
     */
    virtual Command *queryCommand(const QString &cmd) const = 0;

    /**
     * Get a list of all registered commands.
     * \return list of all commands
     * \see queryCommand(), commandList()
     */
    virtual QList<Command *> commands() const = 0;

    /**
     * Get a list of available command line strings.
     * \return command line strings
     * \see commands()
     */
    virtual QStringList commandList() const = 0;

public:
    /**
     * Function that is called to expand a variable in @p text.
     */
    using ExpandFunction = QString (*)(const QStringView& text, KTextEditor::View* view);

    /**
     * Registers a variable called @p name for exact matches.
     * For instance, a variable called "CurrentDocument:Path" could be
     * registered which then expands to the path the current document.
     *
     * @return true on success, false if the variable could not be registered,
     *         e.g. because it already was registered previously.
     *
     * @since 5.57
     */
    bool registerVariableMatch(const QString& name, const QString& description, ExpandFunction expansionFunc);

    /**
     * Registers a variable for arbitrary text that matches the specified
     * prefix. For instance, a variable called "ENV:" could be registered
     * which then expands arbitrary environment variables, e.g. ENV:HOME
     * would expand to the user's home directory.
     *
     * @note A colon ':' is used as separator for the prefix and the text
     *       after the colon that should be evaluated.
     *
     * @return true on success, false if a prefix could not be registered,
     *         e.g. because it already was registered previously.
     *
     * @since 5.57
     */
    bool registerVariablePrefix(const QString& prefix, const QString& description, ExpandFunction expansionFunc);

    /**
     * Unregisters a variable that was previously registered with
     * registerVariableMatch().
     *
     * @return true if the variable was successfully unregistered, and
     *         false if the variable did not exist.
     *
     * @since 5.57
     */
    bool unregisterVariableMatch(const QString& variable);

    /**
     * Unregisters a prefix of variable that was previously registered with
     * registerVariableMatch().
     *
     * @return true if the variable was successfully unregistered, and
     *         false if the variable did not exist.
     *
     * @since 5.57
     */
    bool unregisterVariablePrefix(const QString& variable);

    /**
     * Expands a single @p variable, writing the expanded value to @p output.
     *
     * @return true on success, otherwise false.
     *
     * @since 5.57
     */
    bool expandVariable(const QString& variable, KTextEditor::View* view, QString& output) const;

    /**
     * Expands arbitrary @p text that may contain arbitrary many variables.
     * On success, the expanded text is written to @p output.
     *
     * @since 5.57
     */
    void expandText(const QString& text, KTextEditor::View* view, QString& output) const;

private:
    /**
     * private d-pointer, pointing to the internal implementation
     */
    EditorPrivate *const d;
};

}

#endif
