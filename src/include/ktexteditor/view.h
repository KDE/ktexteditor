/*
    SPDX-FileCopyrightText: 2001 Christoph Cullmann <cullmann@kde.org>

    Documentation:
    SPDX-FileCopyrightText: 2005 Dominik Haumann <dhdev@gmx.de>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef KTEXTEDITOR_VIEW_H
#define KTEXTEDITOR_VIEW_H

#include <ktexteditor_export.h>

// gui merging
#include <KSyntaxHighlighting/Theme>
#include <KXMLGUIClient>
#include <ktexteditor/codecompletionmodel.h>

// widget
#include <QFlags>
#include <QSet>
#include <QWidget>

class QMenu;
class QScrollBar;
class KConfigGroup;

namespace KSyntaxHighlighting
{
class Theme;
}

namespace KTextEditor
{
class Document;
class MainWindow;
class ViewPrivate;
class Attribute;
class AttributeBlock;
class Range;
class Cursor;
class AnnotationModel;
class AbstractAnnotationItemDelegate;
class InlineNoteProvider;
class TextHintProvider;
class CodeCompletionModel;

/*!
 * \class KTextEditor::View
 * \inmodule KTextEditor
 * \inheaderfile KTextEditor/View
 *
 * \brief A text widget with KXMLGUIClient that represents a Document.
 *
 * \target view_intro
 * \section1 Introduction
 *
 * The View class represents a single view of a KTextEditor::Document,
 * get the document on which the view operates with document().
 * A view provides both the graphical representation of the text and the
 * KXMLGUIClient for the actions. The view itself does not provide
 * text manipulation, use the methods from the Document instead. The only
 * method to insert text is insertText(), which inserts the given text
 * at the current cursor position and emits the signal textInserted().
 *
 * Usually a view is created by using Document::createView().
 * Furthermore a view can have a context menu. Set it with setContextMenu()
 * and get it with contextMenu().
 *
 * \target view_hook_into_gui
 * \section1 Merging the View's GUI
 *
 * A View is derived from the class KXMLGUIClient, so its GUI elements (like
 * menu entries and toolbar items) can be merged into the application's GUI
 * (or into a KXMLGUIFactory) by calling
 * \code
 * // view is of type KTextEditor::View*
 * mainWindow()->guiFactory()->addClient( view );
 * \endcode
 * You can add only one view as client, so if you have several views, you first
 * have to remove the current view, and then add the new one, like this
 * \code
 * mainWindow()->guiFactory()->removeClient( currentView );
 * mainWindow()->guiFactory()->addClient( newView );
 * \endcode
 *
 * \target view_selection
 * \section1 Text Selection
 *
 * As the view is a graphical text editor it provides \e normal and \e block
 * text selection. You can check with selection() whether a selection exists.
 * removeSelection() will remove the selection without removing the text,
 * whereas removeSelectionText() also removes both, the selection and the
 * selected text. Use selectionText() to get the selected text and
 * setSelection() to specify the selected text range. The signal
 * selectionChanged() is emitted whenever the selection changed.
 *
 * \target view_cursors
 * \section1 Cursor Positions
 *
 * A view has one Cursor which represents a line/column tuple. Two different
 * kinds of cursor positions are supported: first is the \e real cursor
 * position where a \e tab character only counts one character. Second is the
 * \e virtual cursor position, where a \e tab character counts as many
 * spaces as defined. Get the real position with cursorPosition() and the
 * virtual position with cursorPositionVirtual(). Set the real cursor
 * position with setCursorPosition(). The signal cursorPositionChanged() is
 * emitted whenever the cursor position changed.
 *
 * Screen coordinates of the current text cursor position in pixels are obtained
 * through cursorPositionCoordinates(). Further conversion of screen pixel
 * coordinates and text cursor positions are provided by cursorToCoordinate()
 * and coordinatesToCursor().
 *
 * \target view_mouse_tracking
 * \section1 Mouse Tracking
 *
 * It is possible to get notified via the signal mousePositionChanged() for
 * mouse move events, if mouseTrackingEnabled() returns \e true. Mouse tracking
 * can be turned on/off by calling setMouseTrackingEnabled(). If an editor
 * implementation does not support mouse tracking, mouseTrackingEnabled() will
 * always return \e false.
 *
 * \target view_modes
 * \section1 Input/View Modes
 *
 * A view supports several input modes. Common input modes are
 * \e NormalInputMode and \e ViInputMode. Which input modes the editor supports depends on the
 * implementation. The getter viewInputMode() returns enum \InputMode representing the current mode.
 *
 * Input modes can have their own view modes. In case of default \e NormalInputMode those are
 * \e NormalModeInsert and \e NormalModeOverwrite. You can use viewMode() getter to obtain those.
 *
 * For viewMode() and viewInputMode() there are also variants with \e Human suffix, which
 * returns the human readable representation (i18n) usable for displaying in user interface.
 *
 * Whenever the input/view mode changes the signals
 * viewInputModeChanged()/viewModeChanged() are emitted.
 *
 * \target view_config
 * \section1 View Config
 * Config provides methods to access and modify the low level config information for a given
 * View
 * KTextEditor::View has support for the following keys:
 * \table
 *    \header
 *       \li Key
 *       \li Type
 *       \li Description
 *    \row
 *       \li line-numbers
 *       \li [bool]
 *       \li show/hide line numbers
 *    \row
 *       \li icon-bar
 *       \li [bool]
 *       \li show/hide icon bar
 *    \row
 *       \li folding-bar
 *       \li [bool]
 *       \li show/hide the folding bar
 *    \row
 *       \li folding-preview
 *       \li [bool]
 *       \li enable/disable folding preview when mouse hovers on folded region
 *    \row
 *       \li dynamic-word-wrap
 *       \li [bool]
 *       \li enable/disable dynamic word wrap
 *    \row
 *       \li background-color
 *       \li [QColor]
 *       \li read/set the default background color
 *    \row
 *       \li selection-color
 *       \li [QColor]
 *       \li read/set the default color for selections
 *    \row
 *       \li search-highlight-color
 *       \li [QColor]
 *       \li read/set the background color for search
 *    \row
 *       \li replace-highlight-color
 *       \li [QColor]
 *       \li read/set the background color for replaces
 *    \row
 *       \li default-mark-type
 *       \li [uint]
 *       \li read/set the default mark type
 *    \row
 *       \li allow-mark-menu
 *       \li [bool]
 *       \li enable/disable the menu shown when right clicking
 *    on the left gutter. When disabled, click on the gutter will always set
 *    or clear the mark of default type.
 *    \row
 *       \li icon-border-color
 *       \li [QColor]
 *       \li read/set the icon border color (on the left with the line numbers)
 *    \row
 *       \li folding-marker-color
 *       \li [QColor]
 *       \li read/set folding marker colors (in the icon border)
 *    \row
 *       \li line-number-color
 *       \li [QColor]
 *       \li read/set line number colors (in the icon border)
 *    \row
 *       \li current-line-number-color
 *       \li [QColor]
 *       \li read/set current line number color (in the icon border)
 *    \row
 *       \li modification-markers
 *       \li [bool]
 *       \li read/set whether the modification markers are shown
 *    \row
 *       \li word-count
 *       \li [bool]
 *       \li enable/disable the counting of words and characters in the statusbar
 *    \row
 *       \li line-count
 *       \li [bool]
 *       \li show/hide the total number of lines in the status bar (\since 5.66)
 *    \row
 *       \li scrollbar-minimap
 *       \li [bool]
 *       \li enable/disable scrollbar minimap
 *    \row
 *       \li scrollbar-preview
 *       \li [bool]
 *       \li enable/disable scrollbar text preview on hover
 *    \row
 *       \li font
 *       \li [QFont]
 *       \li change the font
 *    \row
 *       \li theme
 *       \li [QString]
 *       \li change the theme
 *    \row
 *       \li word-completion-minimal-word-length
 *       \li [int]
 *       \li minimal word length to trigger word completion
 *    \row
 *       \li enter-to-insert-completion
 *       \li [bool]
 *       \li enable/disable whether pressing enter inserts completion
 *    \row
 *       \li disable-current-line-highlight-if-inactive
 *       \li [bool]
 *       \li disable current line highlight if the view has no focus (\since 6.20)
 *    \row
 *       \li hide-cursor-if-inactive
 *       \li [bool]
 *       \li hide cursor if the view has no focus (\since 6.20)
 *    \row
 *       \li disable-bracket-match-highlight-if-inactive
 *       \li [bool]
 *       \li disable bracket match and code folding highlight if the view has no focus (\since 6.23)
 * \endtable
 *
 * You can retrieve the value of a config key using configValue() and set the value
 * for a config key using setConfigValue().
 *
 * \target view_annoview
 * \section1 Annotation Interface
 *
 * The Annotation Interface allows to do these things:
 * \list 1
 * \li show/hide the annotation border along with the possibility to add actions
 *     into its context menu.
 * \li set a separate AnnotationModel for the View: Note that this interface
 *     inherits the AnnotationInterface.
 * \li set a custom AbstractAnnotationItemDelegate for the View.
 * \endlist
 *
 * For a more detailed explanation about whether you want to set a custom
 * delegate for rendering the annotations, read the detailed documentation about the
 * AbstractAnnotationItemDelegate.
 *
 * \target view_inlinenote
 * \section1 Inline Notes
 *
 * The inline notes interface provides a way to render arbitrary things in
 * the text. The text layout of the line is adapted to create space for the
 * note. Possible applications include showing a name of a function parameter
 * in a function call or rendering a square with a color preview next to CSS
 * color property.
 *
 * \image inlinenote.png "Inline note showing a CSS color preview"
 *
 * To register as inline note provider, call registerInlineNoteProvider() with
 * an instance that inherits InlineNoteProvider. Finally, make sure you remove
 * your inline note provider by calling unregisterInlineNoteProvider().
 *
 * \target view_texthint
 * \section1 Text Hints
 *
 * The text hint interface provides a way to show tool tips for text located
 * under the mouse. Possible applications include showing a value of a variable
 * when debugging an application, or showing a complete path of an include
 * directive.
 *
 * \image texthint.png "Text hint showing the contents of a variable"
 *
 * To register as text hint provider, call registerTextHintProvider() with an
 * instance that inherits TextHintProvider. Finally, make sure you remove your
 * text hint provider by calling unregisterTextHintProvider().
 *
 * Text hints are shown after the user hovers with the mouse for a delay of
 * textHintDelay() milliseconds over the same word. To change the delay, call
 * setTextHintDelay().
 *
 * \target view_compiface
 * \section1 Completion Interface
 *
 * The Completion Interface is designed to provide code completion
 * functionality for a KTextEditor::View. This interface provides the basic
 * mechanisms to display a list of completions, update this list according
 * to user input, and allow the user to select a completion.
 *
 * Essentially, this provides an item view for the available completions. In
 * order to use this interface, you will need to implement a
 * CodeCompletionModel that generates the relevant completions given the
 * current input.
 *
 * More information about interfaces for the view can be found in:
 * \l kte_group_view_extensions.
 *
 * \sa KTextEditor::Document, KXMLGUIClient
 */
class KTEXTEDITOR_EXPORT View : public QWidget, public KXMLGUIClient
{
    Q_OBJECT

protected:
    /*!
     * Constructor.
     *
     * Create a view attached to the widget \a parent.
     *
     * Pass it the internal implementation to store a d-pointer.
     *
     * \a impl is the d-pointer to use
     *
     * \a parent is the parent widget
     *
     * \sa Document::createView()
     */
    View(ViewPrivate *impl, QWidget *parent);

public:
    /*!
     * Virtual destructor.
     */
    ~View() override;

    /*
     * Accessor for the document
     */
public:
    /*!
     * Get the view's \e document, that means the view is a view of the
     * returned document.
     * Returns the view's document
     */
    virtual Document *document() const = 0;

    /*
     * General information about this view
     */
public:
    /*!
       \enum KTextEditor::View::InputMode

       Possible input modes.
       These correspond to various modes the text editor might be in.

       \value NormalInputMode
       Normal mode.
       \value ViInputMode
       Vi mode. The view will behave like the editor vi(m).
     */
    enum InputMode {
        NormalInputMode = 0,
        ViInputMode = 1
    };

    /*!
        \enum KTextEditor::View::ViewMode

        \brief Possible view modes the text editor might be in.

        \value NormalModeInsert
        Insert mode. Characters will be added.
        \value NormalModeOverwrite
        Overwrite mode. Characters will be replaced.
        \value ViModeNormal
        \value ViModeInsert
        \value ViModeVisual
        \value ViModeVisualLine
        \value ViModeVisualBlock
        \value ViModeReplace
     */
    enum ViewMode {
        NormalModeInsert = 0,
        NormalModeOverwrite = 1,

        ViModeNormal = 10,
        ViModeInsert = 11,
        ViModeVisual = 12,
        ViModeVisualLine = 13,
        ViModeVisualBlock = 14,
        ViModeReplace = 15
    };

    /*!
       \enum KTextEditor::View::LineType

       \brief Possible line types.
       \since 5.33

       \value RealLine
       A real line.
       \value VisibleLine
       A visible line.  Line that is not folded.
     */
    enum LineType {
        RealLine = 0,
        VisibleLine = 1
    };
    /*!
     * Get the current view mode/state.
     * This can be used to detect the view's current mode. For
     * example \NormalInputMode, \ViInputMode or whatever other input modes are
     * supported.
     *
     * Returns current view mode/state
     *
     * \sa viewModeChanged()
     * \sa viewModeHuman()
     */
    virtual ViewMode viewMode() const = 0;

    /*!
     * Get the current view mode state.
     *
     * This can be used to visually indicate the view's current mode, for
     * example \e INSERT mode, \e OVERWRITE mode or \e COMMAND mode - or
     * whatever other edit modes are supported. The string should be
     * translated (i18n), as this is a user aimed representation of the view
     * state, which should be shown in the GUI, for example in the status bar.
     * This string may be rich-text.
     *
     * Returns Human-readable version of the view mode state
     * \sa viewModeChanged()
     */
    virtual QString viewModeHuman() const = 0;

    /*!
     * Set the view's new input mode.
     *
     * \a inputMode is new InputMode value
     *
     * \sa viewInputMode()
     * \since 5.54
     */
    virtual void setViewInputMode(InputMode inputMode) = 0;

    /*!
     * Get the view's current input mode.
     * The current mode can be \NormalInputMode and \ViInputMode.
     * For human translated version \sa viewInputModeHuman.
     *
     * Returns the current input mode of this view
     * \sa viewInputModeChanged()
     */
    virtual InputMode viewInputMode() const = 0;

    /*!
     * Get the view's current input mode in human readable form.
     * The string should be translated (i18n). For id like version \sa viewInputMode
     *
     * Returns the current input mode of this view in human readable form
     */
    virtual QString viewInputModeHuman() const = 0;

    /*!
     * Get the view's main window, if any
     *
     * Returns the view's main window, will always return at least some non-nullptr dummy interface
     */
    virtual KTextEditor::MainWindow *mainWindow() const = 0;

    /*!
     * Get the view's editor widget.
     *
     * Returns the non-null editor widget, which contains the actual text editing area
     *
     * \sa fromEditorWidget()
     * \since 6.20
     */
    [[nodiscard]] QWidget *editorWidget() const;

    /*!
     * Get the view that owns a given editor widget.
     *
     * \a editorWidget is a widget that has been returned from editorWidget()
     *
     * Returns the matching view or \c nullptr for a null widget
     *
     * \since 6.20
     */
    [[nodiscard]] static View *fromEditorWidget(QWidget *editorWidget);

    /*
     * SIGNALS
     * following signals should be emitted by the editor view
     */
Q_SIGNALS:
    /*!
     * This signal is emitted whenever the \a view gets the focus.
     *
     * \a view is the view which gets focus
     *
     * \sa focusOut()
     */
    void focusIn(KTextEditor::View *view);

    /*!
     * This signal is emitted whenever the \a view loses the focus.
     *
     * \a view is view which lost focus
     *
     * \sa focusIn()
     */
    void focusOut(KTextEditor::View *view);

    /*!
     * This signal is emitted whenever the view mode of \a view changes.
     *
     * \a view is the view which changed its mode
     *
     * \a mode is the new view mode
     *
     * \sa viewMode()
     */
    void viewModeChanged(KTextEditor::View *view, KTextEditor::View::ViewMode mode);

    /*!
     * This signal is emitted whenever the \a view's input \a mode changes.
     *
     * \a view is the view which changed its input mode
     *
     * \a mode is the new input mode
     *
     * \sa viewInputMode()
     */
    void viewInputModeChanged(KTextEditor::View *view, KTextEditor::View::InputMode mode);

    /*!
     * This signal is emitted from \a view whenever the users inserts \a text
     * at \a position, that means the user typed/pasted text.
     *
     * \a view is the view in which the text was inserted
     *
     * \a position is the position where the text was inserted
     *
     * \a text is the inserted text
     *
     * \sa insertText()
     */
    void textInserted(KTextEditor::View *view, KTextEditor::Cursor position, const QString &text);

    /*
     * Context menu handling
     */
public:
    /*!
     * Set a context menu for this view to \a menu.
     *
     * \note any previously assigned menu is not deleted.  If you are finished
     *       with the previous menu, you may delete it.
     *
     * \warning Use this with care! Plugin xml gui clients are not merged
     *          into this menu!
     * \warning !!!!!! DON'T USE THIS FUNCTION, UNLESS YOU ARE SURE YOU DON'T WANT PLUGINS TO WORK !!!!!!
     *
     * \a menu is the new context menu object for this view
     *
     * \sa contextMenu()
     */
    virtual void setContextMenu(QMenu *menu) = 0;

    /*!
     * Get the context menu for this view. The return value can be NULL
     * if no context menu object was set and kxmlgui is not initialized yet.
     * If there is no user set menu, the kxmlgui menu is returned. Do not delete this menu, if
     * if it is the xmlgui menu.
     *
     * Returns context menu object
     *
     * \sa setContextMenu()
     */
    virtual QMenu *contextMenu() const = 0;

    /*!
     * Populate \a menu with default text editor actions.  If \a menu is
     * null, a menu will be created with the view as its parent.
     *
     * \note to use this menu, you will next need to call setContextMenu(),
     *       as this does not assign the new context menu.
     *
     * \warning This contains only basic options from the editor component
     *          (katepart). Plugins are \b not merged/integrated into it!
     *          If you want to be a better citizen and take full advantage
     *          of KTextEditor plugins do something like:
     * \code
     * KXMLGUIClient* client = view;
     * // search parent XmlGuiClient
     * while (client->parentClient()) {
     *   client = client->parentClient();
     * }
     *
     * if (client->factory()) {
     *   const QList<QWidget*> menuContainers = client->factory()->containers("menu");
     *   for (QWidget *w : menuContainers) {
     *     if (w->objectName() == "ktexteditor_popup") {
     *       // do something with the menu (ie adding an onshow handler)
     *       break;
     *     }
     *   }
     * }
     * \endcode
     * \warning or simply use the aboutToShow, aboutToHide signals !!!!!
     *
     * \a menu is the menu to be populated, or null to create a new menu.
     *
     * Returns the menu, whether created or passed initially
     */
    virtual QMenu *defaultContextMenu(QMenu *menu = nullptr) const = 0;

Q_SIGNALS:
    /*!
     * Signal which is emitted immediately prior to showing the current
     * context \a menu.
     *
     * \a view is the related view
     *
     */
    void contextMenuAboutToShow(KTextEditor::View *view, QMenu *menu);

    /*
     * Cursor handling
     */
public:
    /*!
     * Set the view's new cursor to \a position. A \e TAB character
     * is handled as only on character.
     *
     * \a position is the new cursor position
     *
     * Returns \e true on success, otherwise \e false
     * \sa cursorPosition()
     */
    virtual bool setCursorPosition(Cursor position) = 0;

    /*!
     * Set the view's new cursors to \a positions. A \e TAB character
     * is handled as only on character.
     *
     * This allows to create multiple cursors in this view.
     *
     * The first passed position will be used for the primary cursor
     * just like if you would call setCursorPosition().
     *
     * \a positions is the new cursor positions
     *
     * \sa cursorPositions()
     *
     * \since 5.95
     */
    void setCursorPositions(const QList<KTextEditor::Cursor> &positions);

    /*!
     * Get the view's current cursor position. A \e TAB character is
     * handled as only one character.
     *
     * Returns current cursor position
     *
     * \sa setCursorPosition()
     */
    virtual Cursor cursorPosition() const = 0;

    /*!
     * Get the view's current cursor positions. A \e TAB character is
     * handled as only one character.
     *
     * The returned vector contains the primary cursor as first element.
     *
     * \since 5.95
     *
     * Returns all currently existing cursors
     */
    QList<KTextEditor::Cursor> cursorPositions() const;

    /*!
     * Get the current \e virtual cursor position, \e virtual means the
     * tabulator character (TAB) counts \e multiple characters, as configured
     * by the user (e.g. one TAB is 8 spaces). The virtual cursor
     * position provides access to the user visible values of the current
     * cursor position.
     *
     * Returns virtual cursor position
     *
     * \sa cursorPosition()
     */
    virtual Cursor cursorPositionVirtual() const = 0;

    /*!
     * Get the screen coordinates (x, y) of the supplied \a cursor relative
     * to the view widget in pixels. Thus, (0, 0) represents the top left hand
     * of the view widget.
     *
     * To map pixel coordinates to a Cursor position (the reverse transformation)
     * use coordinatesToCursor().
     *
     * \a cursor is the cursor to determine coordinate for.
     *
     * Returns cursor screen coordinates relative to the view widget
     *
     * \sa cursorPositionCoordinates(), coordinatesToCursor()
     */
    virtual QPoint cursorToCoordinate(KTextEditor::Cursor cursor) const = 0;

    /*!
     * Get the screen coordinates (x, y) of the cursor position in pixels.
     * The returned coordinates are relative to the View such that (0, 0)
     * represents the top-left corner of the View.
     *
     * If global screen coordinates are required, e.g. for showing a QToolTip,
     * convert the view coordinates to global screen coordinates as follows:
     * \code
     * QPoint viewCoordinates = view->cursorPositionCoordinates();
     * QPoint globalCoorinates = view->mapToGlobal(viewCoordinates);
     * \endcode
     *
     * Returns cursor screen coordinates
     *
     * \sa cursorToCoordinate(), coordinatesToCursor()
     */
    virtual QPoint cursorPositionCoordinates() const = 0;

    /*!
       \enum KTextEditor::View::CoordinatesToCursorFlag

       \brief Modifiers to the conversion of coordinates to cursor.
       \since 6.20

       \value InvalidCursorOutsideText
       If this flag is set, an invalid cursor is returned for coordinates outside text.
       Otherwise (by default) the closest cursor on the same line is returned.
     */
    enum CoordinatesToCursorFlag {
        InvalidCursorOutsideText = 1,
    };
    Q_DECLARE_FLAGS(CoordinatesToCursorFlags, CoordinatesToCursorFlag)

    /*!
     * Get the text-cursor in the document from the screen coordinates,
     * relative to the view widget.
     *
     * To map a cursor to pixel coordinates (the reverse transformation)
     * use cursorToCoordinate().
     *
     * \a coords is the coordinates relative to the view widget
     * \a flags is the modifiers to the conversion
     *
     * Returns cursor in the View, that points onto the character under
     *         the given coordinate. May be KTextEditor::Cursor::invalid().
     */
    [[nodiscard]] KTextEditor::Cursor coordinatesToCursor(QPoint coords, CoordinatesToCursorFlags flags) const;

    /*!
     * Returns coordinatesToCursor(coord, {})
     */
    virtual KTextEditor::Cursor coordinatesToCursor(const QPoint &coord) const = 0;

    /*
     * SIGNALS
     * following signals should be emitted by the editor view
     * if the cursor position changes
     */
Q_SIGNALS:
    /*!
     * This signal is emitted whenever the \a view's cursor position changed.
     *
     * \a view is the view which emitted the signal
     *
     * \a newPosition is the new position of the cursor (Kate will pass the real
     * cursor position, not the virtual)
     *
     * \sa cursorPosition(), cursorPositionVirtual()
     */
    void cursorPositionChanged(KTextEditor::View *view, KTextEditor::Cursor newPosition);

    /*!
     * This signal should be emitted whenever the \a view is scrolled vertically.
     *
     * \a view is the view which emitted the signal
     *
     * \a newPos is the new scroll position
     *
     */
    void verticalScrollPositionChanged(KTextEditor::View *view, KTextEditor::Cursor newPos);

    /*!
     * This signal should be emitted whenever the \a view is scrolled horizontally.
     *
     * \a view is the view which emitted the signal
     *
     */
    void horizontalScrollPositionChanged(KTextEditor::View *view);
    /*
     * Mouse position
     */
public:
    /*!
     * Check, whether mouse tracking is enabled.
     *
     * Mouse tracking is required to have the signal mousePositionChanged()
     * emitted.
     *
     * Returns \e true, if mouse tracking is enabled, otherwise \e false
     *
     * \sa setMouseTrackingEnabled(), mousePositionChanged()
     */
    virtual bool mouseTrackingEnabled() const = 0;

    /*!
     * Try to enable or disable mouse tracking according to \a enable.
     * The return value contains the state of mouse tracking \e after the
     * request. Mouse tracking is required to have the mousePositionChanged()
     * signal emitted.
     *
     * \note Implementation Notes: An implementation is not forced to support
     *       this, and should always return \e false if it does not have
     *       support.
     *
     * \a enable if \e true, try to enable mouse tracking, otherwise disable
     * it.
     *
     * Returns the current state of mouse tracking
     * \sa mouseTrackingEnabled(), mousePositionChanged()
     */
    virtual bool setMouseTrackingEnabled(bool enable) = 0;

Q_SIGNALS:
    /*!
     * This signal is emitted whenever the position of the mouse changes over
     * this \a view. If the mouse moves off the view, an invalid cursor position
     * should be emitted, i.e. Cursor::invalid().
     * \note If mouseTrackingEnabled() returns \e false, this signal is never
     *       emitted.
     *
     * \a view is the view which emitted the signal
     *
     * \a newPosition is the new position of the mouse or Cursor::invalid(), if the
     *        mouse moved out of the \a view.
     * \sa mouseTrackingEnabled()
     */
    void mousePositionChanged(KTextEditor::View *view, KTextEditor::Cursor newPosition);

    /*
     * Selection methods.
     * This deals with text selection and copy&paste
     */
public:
    /*!
     * Set the view's selection to the range selection.
     * The old selection will be discarded.
     *
     * \a range is the range of the new selection
     *
     * Returns \e true on success, otherwise \e false (e.g. when the cursor
     *         range is invalid)
     * \sa selectionRange(), selection()
     */
    virtual bool setSelection(Range range) = 0;

    /*!
     * Set the view's selections to the ranges selection.
     * The old selection will be discarded.
     *
     * \a ranges is the ranges of the new selections
     *
     * \sa selectionRanges(), selection()
     *
     * \since 5.95
     */
    void setSelections(const QList<KTextEditor::Range> &ranges);

    /*!
     * Query the view whether it has selected text, i.e. whether a selection
     * exists.
     *
     * Returns \e true if a text selection exists, otherwise \e false
     * \sa setSelection(), selectionRange()
     */
    virtual bool selection() const = 0;

    /*!
     * Get the range occupied by the current selection.
     *
     * Returns selection range, valid only if a selection currently exists.
     * \sa setSelection()
     */
    virtual Range selectionRange() const = 0;

    /*!
     * Get the ranges occupied by the current selections.
     *
     * Returns selection ranges, valid only if a selection currently exists.
     * \sa setSelections()
     *
     * \since 5.95
     */
    QList<KTextEditor::Range> selectionRanges() const;

    /*!
     * Get the view's selected text.
     *
     * Returns the selected text
     * \sa setSelection()
     */
    virtual QString selectionText() const = 0;

    /*!
     * Remove the view's current selection, \e without deleting the selected
     * text.
     *
     * Returns \e true on success, otherwise \e false
     * \sa removeSelectionText()
     */
    virtual bool removeSelection() = 0;

    /*!
     * Remove the view's current selection \e including the selected text.
     *
     * Returns \e true on success, otherwise \e false
     * \sa removeSelection()
     */
    virtual bool removeSelectionText() = 0;

    /*
     * Blockselection stuff
     */
public:
    /*!
     * Set block selection mode to state \a on.
     *
     * \a on if \e true, block selection mode is turned on, otherwise off
     *
     * Returns \e true on success, otherwise \e false
     * \sa blockSelection()
     */
    virtual bool setBlockSelection(bool on) = 0;

    /*!
     * Get the status of the selection mode. \e true indicates that block
     * selection mode is on. If this is \e true, selections applied via the
     * SelectionInterface are handled as block selections and the Copy&Paste
     * functions work on rectangular blocks of text rather than normal.
     *
     * Returns \e true, if block selection mode is enabled, otherwise \e false
     * \sa setBlockSelection()
     */
    virtual bool blockSelection() const = 0;

    /*
     * SIGNALS
     * following signals should be emitted by the editor view for selection
     * handling.
     */
Q_SIGNALS:
    /*!
     * This signal is emitted whenever the \a view's selection changes.
     * \note If the mode switches from block selection to normal selection
     *       or vice versa this signal should also be emitted.
     *
     * \a view is the view in which the selection changed
     *
     * \sa selection(), selectionRange(), selectionText()
     */
    void selectionChanged(KTextEditor::View *view);

public:
    /*!
     * This is a convenience function which inserts \a text at the view's
     * current cursor position. You do not necessarily need to reimplement
     * it, except you want to do some special things.
     *
     * \a text is the text to be inserted
     *
     * Returns \e true on success of insertion, otherwise \e false
     * \sa textInserted()
     */
    virtual bool insertText(const QString &text);

    /*!
     * Insert a template into the document. The template can have editable fields
     * which can be filled by the user. You can create editable fields
     * with ${fieldname}; multiple fields with the same name will have their
     * contents synchronized automatically, and only the first one is editable
     * in this case.
     * Fields can have a default value specified by writing ${fieldname=default}.
     * Note that `default' is a JavaScript expression and strings need to be quoted.
     * You can also provide a piece of JavaScript for more complex logic.
     * To create a field which provides text based on a JS function call and the values
     * of the other, editable fields, use the ${func()} syntax. func() must be a callable
     * object defined in \a script. You can pass arguments to the function by just
     * writing any constant expression or a field name.
     *
     * \a insertPosition is where to insert the template
     *
     * \a templateString is the template to insert using the above syntax
     *
     * \a script is a script with functions which can be used in \a templateString
     *
     * Returns true on success, false if insertion failed (e.g. read-only mode)
     *
     * \sa evaluateScript()
     */
    bool insertTemplate(KTextEditor::Cursor insertPosition, const QString &templateString, const QString &script = QString());

    /*!
     * Run a piece of javascript.
     *
     * \a script is the javascript code to run
     *
     * \a result is set to a QVariant representing the return value, or
     *           - in case of an error - the error message as a string.
     *           If set to nullptr, no result is passed.
     *           Note that many but not all javascript objects can be accurately
     *           represented as a QVariant. Consider simplifying / stringifying
     *           complex objects, if this may be an issue.
     *
     * Returns \e true on success, \e false if an error orrcured. Note that the document may still have been modified, before an error!
     *
     * \since 6.15
     *
     * \sa insertTemplate()
     */
    bool evaluateScript(const QString &script, QVariant *result = nullptr);

    /*!
     * Scroll view to cursor.
     *
     * \a cursor is the cursor position to scroll to.
     *
     * \since 5.33
     */
    void setScrollPosition(KTextEditor::Cursor cursor);

    /*!
     * Horizontally scroll view to position.
     *
     * \a x is the pixel position to scroll to.
     *
     * \since 5.33
     */
    void setHorizontalScrollPosition(int x);

    /*!
     * Get the cursor corresponding to the maximum position
     * the view can vertically scroll to.
     *
     * Returns cursor position of the maximum vertical scroll position.
     *
     * \since 5.33
     */
    KTextEditor::Cursor maxScrollPosition() const;

    /*!
     * Get the first displayed line in the view.
     *
     * \note If code is folded, many hundred lines can be
     * between firstDisplayedLine() and lastDisplayedLine().
     *
     * \a lineType if RealLine (the default), it returns the real line number
     * accounting for folded regions. In that case it walks over all folded
     * regions
     * O(n) for n == number of folded ranges
     *
     * Returns the first displayed line
     *
     * \sa lastDisplayedLine()
     * \since 5.33
     */
    int firstDisplayedLine(LineType lineType = RealLine) const;

    /*!
     * Get the last displayed line in the view.
     *
     * \note If code is folded, many hundred lines can be
     * between firstDisplayedLine() and lastDisplayedLine().
     *
     * \a lineType is if RealLine (the default), it returns the real line number
     * accounting for folded regions. In that case it walks over all folded
     * regions.
     * O(n) for n == number of folded ranges
     *
     * Returns the last displayed line
     *
     * \sa firstDisplayedLine()
     * \since 5.33
     */
    int lastDisplayedLine(LineType lineType = RealLine) const;

    /*!
     * Get the view's text area rectangle excluding border, scrollbars, etc.
     *
     * Returns the view's text area rectangle
     *
     * \since 5.33
     */
    QRect textAreaRect() const;

    /*!
     * Returns The vertical scrollbar of this view
     * \since 6.0
     */
    virtual QScrollBar *verticalScrollBar() const = 0;

    /*!
     * Returns The horizontal scrollbar of this view
     * \since 6.0
     */
    virtual QScrollBar *horizontalScrollBar() const = 0;

Q_SIGNALS:
    /*!
     * This signal is emitted whenever the displayed range changes
     *
     * \see firstDisplayedLine()
     * \see lastDisplayedLine()
     * \since 6.8
     */
    void displayRangeChanged(KTextEditor::View *view);

public:
    /*!
     * Print the document. This should result in showing the print dialog.
     *
     * Returns true if document was printed
     */
    virtual bool print() = 0;

    /*!
     * Shows the print preview dialog/
     */
    virtual void printPreview() = 0;

    /*!
     * Is the status bar enabled?
     *
     * Returns status bar enabled?
     */
    bool isStatusBarEnabled() const;

    /*!
     * Show/hide the status bar of the view.
     * Per default, the status bar is enabled.
     *
     * \a enable should the status bar be enabled?
     */
    void setStatusBarEnabled(bool enable);

Q_SIGNALS:
    /*!
     * This signal is emitted whenever the status bar of \a view is toggled.
     *
     * \a enabled Whether the status bar is currently enabled or not
     */
    void statusBarEnabledChanged(KTextEditor::View *view, bool enabled);

public:
    /*!
     * Read session settings from the given \a config.
     *
     * Known flags:
     *  none atm
     *
     * \a config is the KConfigGroup to read the session settings from
     *
     * \a flags is additional flags
     *
     * \sa writeSessionConfig()
     */
    virtual void readSessionConfig(const KConfigGroup &config, const QSet<QString> &flags = QSet<QString>()) = 0;

    /*!
     * Write session settings to the \a config.
     * See readSessionConfig() for more details.
     *
     * \a config is the KConfigGroup to write the session settings to
     *
     * \a flags is additional flags (see the known flags listed in readSessionConfig())
     *
     * \sa readSessionConfig()
     */
    virtual void writeSessionConfig(KConfigGroup &config, const QSet<QString> &flags = QSet<QString>()) = 0;

public:
    /*!
     * Returns the attribute for the default style \a defaultStyle.
     *
     * \a defaultStyle default style to get the attribute for
     *
     * \sa KTextEditor::Attribute
     */
    virtual QExplicitlySharedDataPointer<KTextEditor::Attribute> defaultStyleAttribute(KSyntaxHighlighting::Theme::TextStyle defaultStyle) const = 0;

    /*!
     * Get the list of AttributeBlocks for a given \a line in the document.
     *
     * Returns list of AttributeBlocks for given \a line.
     */
    virtual QList<KTextEditor::AttributeBlock> lineAttributes(int line) = 0;

Q_SIGNALS:
    /*!
     * This signal is emitted whenever the current view configuration is changed.
     *
     * \a view is the view whose config has changed
     *
     * \since 5.79
     */
    void configChanged(KTextEditor::View *view);

    /*
     * View Config
     */
public:
    /*!
     * Get a list of all available keys.
     */
    virtual QStringList configKeys() const = 0;
    /*!
     * Get a value for the \a key.
     */
    virtual QVariant configValue(const QString &key) = 0;
    /*!
     * Set a the \a key's value to \a value.
     */
    virtual void setConfigValue(const QString &key, const QVariant &value) = 0;

    /*
     * View Annotation Interface
     */
public:
    /*!
     * Sets a new AnnotationModel for this document to provide
     * annotation information for each line.
     *
     * \a model is the new AnnotationModel
     *
     */
    virtual void setAnnotationModel(AnnotationModel *model) = 0;

    /*!
     * Returns the currently set AnnotationModel or 0 if there's none
     * set
     */
    virtual AnnotationModel *annotationModel() const = 0;

    /*!
     * This function can be used to show or hide the annotation border
     * The annotation border is hidden by default.
     *
     * \a visible if \e true the annotation border is shown, otherwise hidden
     *
     */
    virtual void setAnnotationBorderVisible(bool visible) = 0;

    /*!
     * Returns true if he View's annotation border is visible.
     */
    virtual bool isAnnotationBorderVisible() const = 0;

    /*!
     * Sets the AbstractAnnotationItemDelegate for this view and the model
     * to provide custom rendering of annotation information for each line.
     * Ownership is not transferred.
     *
     * \a delegate is the new AbstractAnnotationItemDelegate, or \c nullptr to reset to the default delegate
     *
     * \since 6.0
     */
    virtual void setAnnotationItemDelegate(KTextEditor::AbstractAnnotationItemDelegate *delegate) = 0;

    /*!
     * Returns the currently used AbstractAnnotationItemDelegate
     *
     * Returns the current AbstractAnnotationItemDelegate
     *
     * \since 6.0
     */
    virtual KTextEditor::AbstractAnnotationItemDelegate *annotationItemDelegate() const = 0;

    /*!
     * This function can be used to declare whether it is known that the annotation items
     * rendered by the set delegate all have the same size.
     * This enables the view to do some optimizations for performance purposes.
     *
     * By default the value of this property is \c false .
     *
     * \a uniformItemSizes if \c true the annotation items are considered to all have the same size
     *
     * \since 6.0
     */
    virtual void setAnnotationUniformItemSizes(bool uniformItemSizes) = 0;

    /*!
     * Returns true if the annotation items all have the same size.
     *
     * \since 6.0
     */
    virtual bool uniformAnnotationItemSizes() const = 0;

Q_SIGNALS:
    /*!
     * This signal is emitted before a context menu is shown on the annotation
     * border for the given line and view.
     *
     * \note Kate Part implementation detail: In Kate Part, the menu has an
     *       entry to hide the annotation border.
     *
     * \a view is the view that the annotation border belongs to
     *
     * \a menu is the context menu that will be shown
     *
     * \a line is the annotated line for which the context menu is shown
     *
     */
    void annotationContextMenuAboutToShow(KTextEditor::View *view, QMenu *menu, int line);

    /*!
     * This signal is emitted when an entry on the annotation border was activated,
     * for example by clicking or double-clicking it. This follows the KDE wide
     * setting for activation via click or double-clcik
     *
     * \a view is the view to which the activated border belongs to
     *
     * \a line is the document line that the activated position belongs to
     *
     */
    void annotationActivated(KTextEditor::View *view, int line);

    /*!
     * This signal is emitted when the annotation border is shown or hidden.
     *
     * \a view is the view to which the border belongs to
     *
     * \a visible is the current visibility state
     *
     */
    void annotationBorderVisibilityChanged(KTextEditor::View *view, bool visible);

    /*
     * Inline Note
     */
public:
    /*!
     * Register the inline note provider \a provider.
     *
     * Whenever a line is painted, the \a provider will be queried for notes
     * that should be painted in it. When the provider is about to be
     * destroyed, make sure to call unregisterInlineNoteProvider() to avoid a
     * dangling pointer.
     *
     * \a provider inline note provider
     *
     * \sa unregisterInlineNoteProvider(), KTextEditor::InlineNoteProvider
     */
    virtual void registerInlineNoteProvider(KTextEditor::InlineNoteProvider *provider) = 0;

    /*!
     * Unregister the inline note provider \a provider.
     *
     * \a provider inline note provider to unregister
     *
     * \sa registerInlineNoteProvider(), KTextEditor::InlineNoteProvider
     */
    virtual void unregisterInlineNoteProvider(KTextEditor::InlineNoteProvider *provider) = 0;

    /*
     * Text Hint
     */
public:
    /*!
     * Register the text hint provider \a provider.
     *
     * Whenever the user hovers over text, \a provider will be asked for
     * a text hint. When the provider is about to be destroyed, make
     * sure to call unregisterTextHintProvider() to avoid a dangling pointer.
     *
     * \a provider text hint provider
     *
     * \sa unregisterTextHintProvider(), KTextEditor::TextHintProvider
     */
    virtual void registerTextHintProvider(KTextEditor::TextHintProvider *provider) = 0;

    /*!
     * Unregister the text hint provider \a provider.
     *
     * \a provider text hint provider to unregister
     *
     * \sa registerTextHintProvider(), KTextEditor::TextHintProvider
     */
    virtual void unregisterTextHintProvider(KTextEditor::TextHintProvider *provider) = 0;

    /*!
     * Set the text hint delay to \a delay milliseconds.
     *
     * The delay specifies the time the user needs to hover over the text
     * before the tool tip is shown. Therefore, \a delay should not be
     * too large, a value of 500 milliseconds is recommended and set by
     * default.
     *
     * If \a delay is <= 0, the default delay will be set.
     *
     * \a delay is the tool tip delay in milliseconds
     *
     */
    virtual void setTextHintDelay(int delay) = 0;

    /*!
     * Get the text hint delay in milliseconds.
     * By default, the text hint delay is set to 500 milliseconds.
     * It can be changed by calling setTextHintDelay().
     */
    virtual int textHintDelay() const = 0;

    /*
     * Completion
     */
public:
    /*!
     * Returns true if the code completion box is currently displayed.
     */
    virtual bool isCompletionActive() const = 0;

    /*!
     * Invoke code completion over a given range \a word with a specific \a model.
     */
    virtual void startCompletion(Range word, CodeCompletionModel *model) = 0;

    /*!
     * Abort the currently displayed code completion without executing any currently
     * selected completion. This is safe, even when the completion box is not currently
     * active.
     * \sa isCompletionActive()
     */
    virtual void abortCompletion() = 0;

    /*!
     * Force execution of the currently selected completion, and hide the code completion
     * box.
     */
    virtual void forceCompletion() = 0;

    /*!
     * Register a new code completion \a model.
     *
     * \a model is new completion model
     *
     * \sa unregisterCompletionModel()
     */
    virtual void registerCompletionModel(CodeCompletionModel *model) = 0;

    /*!
     * Unregister a code completion \a model.
     *
     * \a model is the model that should be unregistered
     *
     * \sa registerCompletionModel()
     */
    virtual void unregisterCompletionModel(CodeCompletionModel *model) = 0;

    /*!
     * Returns true if automatic code completion invocation is enabled.
     */
    virtual bool isAutomaticInvocationEnabled() const = 0;

    /*!
     * Enable (if \a enabled is true) or disable automatic code completion invocation.
     */
    virtual void setAutomaticInvocationEnabled(bool enabled = true) = 0;

    /*!
     * Invoke code completion over a given range, with specific models and invocation type.
     *
     * \a models is the list of models to start. If this is an empty list, all registered models are started.
     *
     */
    virtual void startCompletion(const Range &word,
                                 const QList<CodeCompletionModel *> &models = QList<CodeCompletionModel *>(),
                                 KTextEditor::CodeCompletionModel::InvocationType invocationType = KTextEditor::CodeCompletionModel::ManualInvocation) = 0;

    /*!
     * Obtain the list of registered code completion models.
     *
     * Returns a list of a models that are currently registered
     * \sa registerCompletionModel(CodeCompletionModel*)
     */
    virtual QList<CodeCompletionModel *> codeCompletionModels() const = 0;

public:
    /*!
     * Get the current active theme of this view.
     * Might change during runtime, configChanged() will be emitted in that cases.
     *
     * Returns current active theme
     *
     * \since 5.79
     */
    KSyntaxHighlighting::Theme theme() const;

private:
    ViewPrivate *const d;
};

}

#endif
