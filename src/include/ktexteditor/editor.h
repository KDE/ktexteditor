/*
    SPDX-FileCopyrightText: 2005-2014 Christoph Cullmann <cullmann@kde.org>
    SPDX-FileCopyrightText: 2005-2014 Dominik Haumann <dhaumann@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef KTEXTEDITOR_EDITOR_H
#define KTEXTEDITOR_EDITOR_H

#include <ktexteditor_export.h>

#include <QList>
#include <QObject>

// theme support
#include <KSyntaxHighlighting/Theme>

class KAboutData;
class KConfig;

namespace KSyntaxHighlighting
{
class Repository;
}

/*!
 * \namespace KTextEditor
 * \inmodule KTextEditor
 * \inheaderfile KTextEditor/Editor
 *
 * The KTextEditor namespace contains all the public API that is required
 * to use the KTextEditor component. Using the KTextEditor interfaces is
 * described in the article \l index.
 *
 * \warning All classes that are \e not part of the KTextEditor namespace
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

/*!
 * \class KTextEditor::Editor
 * \inmodule KTextEditor
 * \inheaderfile KTextEditor/Editor
 *
 * \brief Accessor interface for the KTextEditor framework.
 *
 * \target editor_intro
 * \section1 Introduction
 *
 * The Editor part can either be accessed through the static accessor Editor::instance()
 * or through the KParts component model (see \l kte_design_part).
 * The Editor singleton provides general information and configuration methods
 * for the Editor, for example KAboutData by using aboutData().
 *
 * The Editor has a list of all opened documents. Get this list with documents().
 * To create a new Document call createDocument(). The signal documentCreated()
 * is emitted whenever the Editor created a new document.
 *
 * \target editor_config
 * \section1 Configuration
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
 * \target editor_commands
 * \section1 Command Line Commands
 *
 * With Commands it is possible to add new commands to the command line.
 * These Command%s then are added to all document View%s.
 * Common use cases include commands like \e find or setting document variables.
 * The list of all registered commands can be obtained either through commandList()
 * or through commands(). Further, a specific command can be obtained through
 * queryCommand(). For further information, read the Command API documentation.
 *
 * \sa KTextEditor::Document, KTextEditor::ConfigPage, KTextEditor::Command
 */
class KTEXTEDITOR_EXPORT Editor : public QObject
{
    Q_OBJECT

protected:
    /*!
     * Constructor.
     *
     * Create the Editor object and pass it the internal
     * implementation to store a d-pointer.
     *
     * \a impl d-pointer to use
     *
     */
    Editor(EditorPrivate *impl);

    /*!
     * Virtual destructor.
     */
    ~Editor() override;

public:
    /*!
     * Accessor to get the Editor instance.
     *
     * \note This object will stay alive until QCoreApplication terminates.
     *       You shall not delete it yourself.
     *       There is only ONE Editor instance of this per process.
     *
     * Returns Editor controller, after initial construction, will
     *        live until QCoreApplication is terminating.
     */
    static Editor *instance();

public:
    /*!
     * Set the global application object.
     * This will allow the editor component to access
     * the hosting application.
     *
     * \a application is the application object
     * (if the argument is a nullptr, this will reset the application back to a dummy interface)
     *
     */
    virtual void setApplication(KTextEditor::Application *application) = 0;

    /*!
     * Current hosting application, if any set.
     * Returns current application object or a dummy interface that allows you to call the functions
     *         will never return a nullptr
     */
    virtual KTextEditor::Application *application() const = 0;

    /*
     * Methods to create and manage the documents.
     */
public:
    /*!
     * Create a new document object with \a parent.
     *
     * For each created document, the signal documentCreated() is emitted.
     *
     * \a parent is the document's parent object
     *
     * Returns new KTextEditor::Document object
     * \sa documents(), documentCreated()
     */
    virtual Document *createDocument(QObject *parent) = 0;

    /*!
     * Get a list of all documents of this editor.
     * Returns list of all existing documents
     * \sa createDocument()
     */
    virtual QList<Document *> documents() = 0;

Q_SIGNALS:
    /*!
     * The \a editor emits this signal whenever a \a document was successfully
     * created.
     *
     * \a editor is a pointer to the Editor singleton which created the new document
     *
     * \a document is the newly created document instance
     *
     * \sa createDocument()
     */
    void documentCreated(KTextEditor::Editor *editor, KTextEditor::Document *document);

    /*
     * General Information about this editor.
     */
public:
    /*!
     * Get the about data of this Editor part.
     * Returns about data
     */
    virtual const KAboutData &aboutData() const = 0;

    /*!
     * Get the current default encoding for this Editor part.
     * Returns default encoding
     */
    QString defaultEncoding() const;

    /*
     * Configuration management.
     */
public:
    /*!
     * Show the editor's config dialog.  Changes will be applied to the
     * editor and the configuration changes are saved.
     *
     * \note Instead of using the config dialog, the config pages can be
     *       embedded into your own config dialog by using configPages() and
     *       configPage().
     *
     * \a parent is parent widget
     *
     */
    virtual void configDialog(QWidget *parent) = 0;

    /*!
     * Get the number of available config pages.
     * If a number < 1 is returned, it does not support config pages.
     * Returns number of config pages
     * \sa configPage()
     */
    virtual int configPages() const = 0;

    /*!
     * Get the config page with the \a number, config pages from 0 to
     * configPages()-1 are available if configPages() > 0.
     * Configuration changes done over this widget are automatically
     * saved.
     *
     * \a number is the config page index
     *
     * \a parent is the parent widget for the config page
     *
     * Returns created config page or NULL, if the number is out of bounds
     * \sa configPages()
     */
    virtual ConfigPage *configPage(int number, QWidget *parent) = 0;

Q_SIGNALS:
    /*!
     * This signal is emitted whenever the editor configuration is changed.
     *
     * \a editor is the editor whose config has changed
     *
     * \since 5.79
     */
    void configChanged(KTextEditor::Editor *editor);

public:
    /*!
     * Get the current global editor font.
     * Might change during runtime, configChanged() will be emitted in that cases.
     * Individual views might have set different fonts, can be queried with the "font" key via \sa KTextEditor::ConfigInterface::configValue().
     *
     * Returns current global font for all views
     *
     * \since 5.80
     */
    QFont font() const;

    /*!
     * Get the current global theme.
     * Might change during runtime, configChanged() will be emitted in that cases.
     * Individual views might have set different themes, \sa KTextEditor::View::theme().
     *
     * Returns current global theme for all views
     *
     * \since 5.79
     */
    KSyntaxHighlighting::Theme theme() const;

public:
    /*!
     * Get read-only access to the syntax highlighting repository the editor uses.
     * Might be reloaded during runtime, repositoryReloaded() will be emitted in that cases.
     *
     * Returns syntax repository used by the editor
     *
     * \since 5.79
     */
    const KSyntaxHighlighting::Repository &repository() const;

Q_SIGNALS:
    /*!
     * This signal is emitted whenever the editor syntax repository is reloaded.
     * Can be used to e.g. re-instantiate syntax definitions that got invalidated by
     * the repository reload.
     *
     * \a editor is the editor whose repository was reloaded
     *
     * \since 5.79
     */
    void repositoryReloaded(KTextEditor::Editor *editor);

public:
    /*!
     * Query for the command \a cmd.
     * If the command \a cmd does not exist the return value is NULL.
     *
     * \a cmd is the name of command to query for
     *
     * Returns the found command or NULL if no such command exists
     */
    virtual Command *queryCommand(const QString &cmd) const = 0;

    /*!
     * Get a list of all registered commands.
     * Returns list of all commands
     * \sa queryCommand(), commandList()
     */
    virtual QList<Command *> commands() const = 0;

    /*!
     * Get a list of available command line strings.
     * Returns command line strings
     * \sa commands()
     */
    virtual QStringList commandList() const = 0;

public:
    /*!
     * Function that is called to expand a variable in \a text.
     */
    // TODO KF6: Use std::function to allow captures (context via closure)
    // using ExpandFunction = std::function<QString(const QStringView &text, KTextEditor::View *view)>;
    using ExpandFunction = QString (*)(const QStringView &text, KTextEditor::View *view);

    /*!
     * Registers a variable called \a name for exact matches.
     * For instance, a variable called "CurrentDocument:Path" could be
     * registered which then expands to the path the current document.
     *
     * Returns true on success, false if the variable could not be registered,
     *         e.g. because it already was registered previously.
     *
     * \since 5.57
     */
    bool registerVariableMatch(const QString &name, const QString &description, ExpandFunction expansionFunc);

    /*!
     * Registers a variable for arbitrary text that matches the specified
     * \a prefix. For instance, a variable called "ENV:" could be registered
     * which then expands arbitrary environment variables, e.g. ENV:HOME
     * would expand to the user's home directory.
     *
     * \note A colon ':' is used as separator for the prefix and the text
     *       after the colon that should be evaluated.
     *
     * Returns true on success, false if a prefix could not be registered,
     * e.g. because it already was registered previously.
     *
     * \since 5.57
     */
    bool registerVariablePrefix(const QString &prefix, const QString &description, ExpandFunction expansionFunc);

    /*!
     * Unregisters a variable that was previously registered with
     * registerVariableMatch() or registerVariablePrefix().
     *
     * \a variableName is the variable's name
     *
     * Returns true if the variable was successfully unregistered, and
     * false if the variable did not exist.
     *
     * \since 6.0
     */
    bool unregisterVariable(const QString &variableName);

    /*!
     * Expands a single \a variable, writing the expanded value to \a output.
     *
     * Returns true on success, otherwise false.
     *
     * \since 5.57
     */
    bool expandVariable(const QString &variable, KTextEditor::View *view, QString &output) const;

    /*!
     * Expands arbitrary \a text that may contain arbitrary many variables.
     * On success, the expanded text is written to \a output.
     *
     * Returns the expanded text.
     *
     * \since 6.0
     */
    QString expandText(const QString &text, KTextEditor::View *view) const;

    /*!
     * Adds a QAction to the widget in \a widgets that whenever focus is
     * gained. When the action is invoked, a non-modal dialog is shown that
     * lists all \a variables. If \a variables is non-empty, then only the
     * variables in \a variables are listed.
     *
     * The supported QWidgets in the \a widgets argument currently are:
     * \list
     * \li QLineEdit
     * \li QTextEdit
     * \endlist
     *
     * \since 5.63
     */
    void addVariableExpansion(const QList<QWidget *> &widgets, const QStringList &variables = QStringList()) const;

private:
    EditorPrivate *const d;
};

}

#endif
