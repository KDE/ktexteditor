/*
    SPDX-FileCopyrightText: 2013 Christoph Cullmann <cullmann@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef KTEXTEDITOR_MAINWINDOW_H
#define KTEXTEDITOR_MAINWINDOW_H

#include <ktexteditor_export.h>

#include <QObject>
#include <QWidgetList>

class QEvent;
class QIcon;
class QUrl;
class QWidget;

class KXMLGUIFactory;

namespace KTextEditor
{
class Plugin;
class Document;
class View;

/*!
 * \class KTextEditor::MainWindow
 * \inmodule KTextEditor
 * \inheaderfile KTextEditor/MainWindow
 *
 * \brief This class allows the application that embeds the KTextEditor component to
 * allow it to access parts of its main window.
 *
 * For example the component can get a place to show view bar widgets (e.g. search&replace, goto line, ...).
 * This is useful to e.g. have one place inside the window to show such stuff even if the application allows
 * the user to have multiple split views available per window.
 *
 * The application must pass a pointer to the MainWindow object to the createView method on view creation
 * and ensure that this main window stays valid for the complete lifetime of the view.
 *
 * It must not reimplement this class but construct an instance and pass a pointer to a QObject that
 * has the required slots to receive the requests.
 *
 * \l kte_plugin_hosting
 */
class KTEXTEDITOR_EXPORT MainWindow : public QObject
{
    Q_OBJECT

public:
    /*!
     * Construct an MainWindow wrapper object.
     * The passed parent is both the parent of this QObject and the receiver of all interface
     * calls via invokeMethod.
     *
     * \a parent is the object the calls are relayed to
     *
     */
    MainWindow(QObject *parent);

    /*!
     * Virtual Destructor
     */
    ~MainWindow() override;

    //
    // Accessors to some window properties and contents
    //
public:
    /*!
     * Get the toplevel widget.
     *
     * Returns the real main window widget, nullptr if not available
     */
    QWidget *window();

    /*!
     * Accessor to the XMLGUIFactory.
     *
     * Returns the mainwindow's KXMLGUIFactory, nullptr if not available
     */
    KXMLGUIFactory *guiFactory();

    //
    // Signals related to the main window
    //
Q_SIGNALS:
    /*!
     * This signal is emitted for every unhandled ShortcutOverride in the window
     *
     * \a e is the responsible event
     *
     */
    void unhandledShortcutOverride(QEvent *e);

    //
    // View access and manipulation interface
    //
public:
    /*!
     * Get a list of all views for this main window.
     *
     * It is beneficial if the list is sorted by most recently used,
     * as the library will e.g. try to use the most recent used url() by walking over this
     * list for save and other such things.
     *
     * Returns all views, might be empty!
     */
    QList<KTextEditor::View *> views();

    /*!
     * Access the active view.
     * Returns active view, nullptr if not available
     */
    KTextEditor::View *activeView();

    /*!
     * Activate the view with the corresponding \a document.
     * If none exist for this document, create one
     *
     * \a document is the document
     *
     * Returns activated view of this document,
     *         return nullptr if not possible
     */
    KTextEditor::View *activateView(KTextEditor::Document *document);

    /*!
     * Open the document URL with the given encoding.
     *
     * \a url is the document's URL
     *
     * \a encoding is the preferred encoding. If encoding is QString() the
     * encoding will be guessed or the default encoding will be used.
     *
     * Returns a pointer to the created view for the new document, if a document
     *         with this url is already existing, its view will be activated,
     *         return nullptr if not possible
     */
    KTextEditor::View *openUrl(const QUrl &url, const QString &encoding = QString());

    /*!
     * Close selected view
     *
     * \a view is the view to close
     *
     * Returns true if view was closed
     */
    bool closeView(KTextEditor::View *view);

    /*!
     * Split current view space according to \a orientation
     *
     * \a orientation defines the split orientation (horizontal or vertical)
     *
     */
    void splitView(Qt::Orientation orientation);

    /*!
     * Close the split view that contains the given view.
     *
     * \a view is the view.
     *
     * Returns true if the split view was closed.
     */
    bool closeSplitView(KTextEditor::View *view);

    /*!
     * Returns \c true if the given views \a view1 and \a view2 share
     * the same split view, false otherwise.
     */
    bool viewsInSameSplitView(KTextEditor::View *view1, KTextEditor::View *view2);

    //
    // Signals related to view handling
    //
Q_SIGNALS:
    /*!
     * This signal is emitted whenever the active view changes.
     *
     * \a view is the new active view
     */
    void viewChanged(KTextEditor::View *view);

    /*!
     * This signal is emitted whenever a new view is created
     *
     * \a view is the view that was created
     */
    void viewCreated(KTextEditor::View *view);

    //
    // Interface to allow view bars to be constructed in a central place per window
    //
public:
    /*!
     * Try to create a view bar for the given view.
     *
     * \a view is the view for which we want a view bar
     *
     * Returns suitable widget that can host view bars widgets or nullptr
     */
    QWidget *createViewBar(KTextEditor::View *view);

    /*!
     * Delete the view bar for the given view.
     *
     * \a view is the view for which we want to delete the view bar
     *
     */
    void deleteViewBar(KTextEditor::View *view);

    /*!
     * Add a widget to the view bar.
     *
     * \a view is the view for which the view bar is used
     *
     * \a bar is the widget, shall have the viewBarParent() as parent widget
     *
     */
    void addWidgetToViewBar(KTextEditor::View *view, QWidget *bar);

    /*!
     * Show the view bar for the given view
     *
     * \a view is the view for which the view bar is used
     *
     */
    void showViewBar(KTextEditor::View *view);

    /*!
     * Hide the view bar for the given view
     *
     * \a view is the view for which the view bar is used
     *
     */
    void hideViewBar(KTextEditor::View *view);

    //
    // ToolView stuff, here all stuff belong which allows to
    // add/remove and manipulate the toolview of this main windows
    //
public:
    /*!
        \enum KTextEditor::MainWindow::ToolViewPosition

        Toolview position.
        A toolview can only be at one side at a time.

        \value Left
        Left side
        \value Right
        Right side
        \value Top
        Top side
        \value Bottom
        Bottom side
     */
    enum ToolViewPosition {
        Left = 0,
        Right = 1,
        Top = 2,
        Bottom = 3
    };

    /*!
     * Create a new toolview with unique \a identifier at side \a pos
     * with \a icon and caption \a text. Use the returned widget to embed
     * your widgets.
     *
     * \a plugin is the plugin which owns this tool view
     *
     * \a identifier is unique identifier for this toolview
     *
     * \a pos is the position for the toolview, if we are in session restore,
     * this is only a preference
     *
     * \a icon is the icon to use in the sidebar for the toolview
     *
     * \a text is translated text (i18n()) to use in addition to icon
     *
     * Returns created toolview on success, otherwise NULL
     */
    QWidget *createToolView(KTextEditor::Plugin *plugin,
                            const QString &identifier,
                            KTextEditor::MainWindow::ToolViewPosition pos,
                            const QIcon &icon,
                            const QString &text);

    /*!
     * Move the toolview \a widget to position \a pos.
     *
     * \a widget is the toolview to move, where the widget was constructed
     * by createToolView().
     *
     * \a pos is the new position to move the widget to
     *
     * Returns \e true on success, otherwise \e false
     */
    bool moveToolView(QWidget *widget, KTextEditor::MainWindow::ToolViewPosition pos);

    /*!
     * Show the toolview \a widget.
     *
     * \a widget is the toolview to show, where the widget was constructed
     * by createToolView().
     *
     * Returns \e true on success, otherwise \e false
     * TODO add focus parameter: bool showToolView (QWidget *widget, bool giveFocus );
     */
    bool showToolView(QWidget *widget);

    /*!
     * Hide the toolview \a widget.
     *
     * \a widget is the toolview to hide, where the widget was constructed
     * by createToolView().
     *
     * Returns \e true on success, otherwise \e false
     */
    bool hideToolView(QWidget *widget);

    //
    // Application plugin accessors
    //
public:
    /*!
     * Shows the \a plugin's config page. The \a page specifies which
     * config page will be shown, see KTextEditor::Plugin::configPages().
     *
     * Returns \e true on success, otherwise \e false
     * \since 5.63
     */
    bool showPluginConfigPage(KTextEditor::Plugin *plugin, int page);

    /*!
     * Get a plugin view for the plugin with with identifier \a name.
     *
     * \a name is the plugin's name
     *
     * Returns pointer to the plugin view if a plugin with \a name is loaded and has a view for this mainwindow,
     *         otherwise NULL
     */
    QObject *pluginView(const QString &name);

    //
    // Signals related to application plugins
    //
Q_SIGNALS:
    /*!
     * This signal is emitted when the view of some Plugin is created for this main window.
     *
     * \a name is the name of the plugin
     *
     * \a pluginView is the new plugin view
     *
     */
    void pluginViewCreated(const QString &name, QObject *pluginView);

    /*!
     * This signal is emitted when the view of some Plugin got deleted.
     *
     * \warning Do not access the data referenced by the pointer, it is already invalid.
     * Use the pointer only to remove mappings in hash or maps
     *
     * \a name is the name of the plugin
     *
     * \a pluginView is the deleted plugin view
     *
     */
    void pluginViewDeleted(const QString &name, QObject *pluginView);

    //
    // Custom widget handling
    //
public:
    /*!
     * Add a widget to the main window.
     * This is useful to show non-KTextEditor::View widgets in the main window.
     * The host application should try to manage this like some KTextEditor::View (e.g. as a tab) and provide
     * the means to close it.
     *
     * \a widget is the widget to add
     *
     * Returns success, if false, the plugin needs to take care to show the widget itself, otherwise
     *         the main window will take ownership of the widget
     * \since 5.98
     */
    bool addWidget(QWidget *widget);

    /*!
     * \brief remove this \a widget from this mainwindow. The widget will be deleted afterwards
     *
     * \a widget is the widget to be removed
     *
     * Returns true on success
     * \since 6.0
     */
    bool removeWidget(QWidget *widget);

    /*!
     * \brief returns the list of non-KTextEditor::View widgets in this main window.
     * \sa addWidget
     * \since 6.0
     */
    QWidgetList widgets();

    /*!
     * \brief returns the currently active widget. It can be a non-KTextEditor::View widget or a KTextEditor::View
     * \since 6.0
     */
    QWidget *activeWidget();

    /*!
     * \brief activate \a widget. If the widget is not present in the window, it will be added to the window
     *
     * \a widget is the widget to activate
     *
     * \since 6.0
     */
    void activateWidget(QWidget *widget);

    //
    // Signals related to widgets
    //
Q_SIGNALS:
    /*!
     * Emitted when a widget was added to this window.
     *
     * \a widget is the widget that was added
     *
     * \since 6.0
     */
    void widgetAdded(QWidget *widget);

    /*!
     * Emitted when a widget was added to this window.
     *
     * \a widget is the widget that was removed
     *
     * \since 6.0
     */
    void widgetRemoved(QWidget *widget);

    //
    // Message output
    //
public:
    /*!
     * Display a message to the user.
     * The host application might show this inside a dedicated output view.
     *
     * \a message is the incoming message we shall handle
     *
     * Returns true, if the host application was able to handle the message, else false
     * \since 5.98
     *
     * details of message format:
     *
     * message text, will be trimmed before output
     *
     *    message["text"] = i18n("your cool message")
     *
     * the text will be split in lines, all lines beside the first can be collapsed away
     *
     * message type, we support at the moment
     *
     *    message["type"] = "Error"
     *    message["type"] = "Warning"
     *    message["type"] = "Info"
     *    message["type"] = "Log"
     *
     * this is take from https://microsoft.github.io/language-server-protocol/specification#window_showMessage MessageType of LSP
     *
     * will lead to appropriate icons/... in the output view
     *
     * a message should have some category, like Git, LSP, ....
     *
     *    message["category"] = i18n(...)
     *
     * will be used to allow the user to filter for
     *
     * one can additionally provide a categoryIcon
     *
     *    message["categoryIcon"] = QIcon(...)
     *
     * the categoryIcon icon QVariant must contain a QIcon, nothing else!
     *
     * A string token can be passed to allow to replace messages already send out with new ones.
     * That is useful for e.g. progress output
     *
     *     message["token"] = "yourmessagetoken"
     *
     */
    bool showMessage(const QVariantMap &message);

private:
    friend class MainWindowPrivate;

    class MainWindowPrivate *const d;
};

} // namespace KTextEditor

#endif
