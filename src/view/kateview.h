/* This file is part of the KDE libraries
  Copyright (C) 2002 John Firebaugh <jfirebaugh@kde.org>
  Copyright (C) 2001 Christoph Cullmann <cullmann@kde.org>
  Copyright (C) 2001-2010 Joseph Wenninger <jowenn@kde.org>
  Copyright (C) 1999 Jochen Wilhelmy <digisnap@cs.tu-berlin.de>

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Library General Public
  License version 2 as published by the Free Software Foundation.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Library General Public License for more details.

  You should have received a copy of the GNU Library General Public License
  along with this library; see the file COPYING.LIB.  If not, write to
  the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
  Boston, MA 02110-1301, USA.
*/

#ifndef kate_view_h
#define kate_view_h

#include <ktexteditor/view.h>
#include <ktexteditor/texthintinterface.h>
#include <ktexteditor/markinterface.h>
#include <ktexteditor/codecompletioninterface.h>
#include <ktexteditor/configinterface.h>
#include <ktexteditor/annotationinterface.h>
#include <ktexteditor/mainwindow.h>

#include <QPointer>
#include <QModelIndex>
#include <QMenu>
#include <QSpacerItem>

#include "katetextrange.h"
#include "katetextfolding.h"
#include "katerenderer.h"

namespace KTextEditor
{
class AnnotationModel;
class Message;
}

namespace KTextEditor { class DocumentPrivate; }
class KateBookmarks;
class KateViewConfig;
class KateRenderer;
class KateSpellCheckDialog;
class KateCompletionWidget;
class KateViewInternal;
class KateViewBar;
class KateGotoBar;
class KateDictionaryBar;
class KateSpellingMenu;
class KateMessageWidget;
class KateIconBorder;
class KateStatusBar;
class KateViewEncodingAction;
class KateModeMenu;
class KateAbstractInputMode;

class KToggleAction;
class KSelectAction;

class QAction;
class QGridLayout;
class QVBoxLayout;

namespace KTextEditor
{

//
// Kate KTextEditor::View class ;)
//
class KTEXTEDITOR_EXPORT ViewPrivate : public KTextEditor::View,
    public KTextEditor::TextHintInterface,
    public KTextEditor::CodeCompletionInterface,
    public KTextEditor::ConfigInterface,
    public KTextEditor::AnnotationViewInterface
{
    Q_OBJECT
    Q_INTERFACES(KTextEditor::TextHintInterface)
    Q_INTERFACES(KTextEditor::ConfigInterface)
    Q_INTERFACES(KTextEditor::CodeCompletionInterface)
    Q_INTERFACES(KTextEditor::AnnotationViewInterface)

    friend class KTextEditor::View;
    friend class ::KateViewInternal;
    friend class ::KateIconBorder;

public:
    ViewPrivate (KTextEditor::DocumentPrivate *doc, QWidget *parent, KTextEditor::MainWindow *mainWindow = Q_NULLPTR);
    ~ViewPrivate ();

    /**
     * Get the view's main window, if any
     * \return the view's main window
     */
    KTextEditor::MainWindow *mainWindow() const Q_DECL_OVERRIDE
    {
        return m_mainWindow;
    }

    KTextEditor::Document *document() const Q_DECL_OVERRIDE;

    ViewMode viewMode() const Q_DECL_OVERRIDE;
    QString viewModeHuman() const Q_DECL_OVERRIDE;
    InputMode viewInputMode() const Q_DECL_OVERRIDE;
    QString viewInputModeHuman() const Q_DECL_OVERRIDE;

    void setInputMode(InputMode mode);

    //
    // KTextEditor::ClipboardInterface
    //
public Q_SLOTS:
    void paste(const QString *textToPaste = 0);
    void cut();
    void copy() const;

private Q_SLOTS:
    /**
     * internal use, apply word wrap
     */
    void applyWordWrap();

    //
    // KTextEditor::PopupMenuInterface
    //
public:
    void setContextMenu(QMenu *menu) Q_DECL_OVERRIDE;
    QMenu *contextMenu() const Q_DECL_OVERRIDE;
    QMenu *defaultContextMenu(QMenu *menu = 0L) const Q_DECL_OVERRIDE;

private Q_SLOTS:
    void aboutToShowContextMenu();
    void aboutToHideContextMenu();

private:
    QPointer<QMenu> m_contextMenu;

    //
    // KTextEditor::ViewCursorInterface
    //
public:
    bool setCursorPosition(KTextEditor::Cursor position) Q_DECL_OVERRIDE;

    KTextEditor::Cursor cursorPosition() const Q_DECL_OVERRIDE;

    KTextEditor::Cursor cursorPositionVirtual() const Q_DECL_OVERRIDE;

    QPoint cursorToCoordinate(const KTextEditor::Cursor &cursor) const Q_DECL_OVERRIDE;

    KTextEditor::Cursor coordinatesToCursor(const QPoint &coord) const Q_DECL_OVERRIDE;

    QPoint cursorPositionCoordinates() const Q_DECL_OVERRIDE;

    bool setCursorPositionVisual(const KTextEditor::Cursor &position);

    /**
     * Return the virtual cursor column, each tab is expanded into the
     * document's tabWidth characters. If word wrap is off, the cursor may be
     * behind the line's length.
     */
    int virtualCursorColumn() const;

    bool mouseTrackingEnabled() const Q_DECL_OVERRIDE;
    bool setMouseTrackingEnabled(bool enable) Q_DECL_OVERRIDE;

private:
    void notifyMousePositionChanged(const KTextEditor::Cursor &newPosition);

    // Internal
public:
    bool setCursorPositionInternal(const KTextEditor::Cursor &position, uint tabwidth = 1, bool calledExternally = false);

    //
    // KTextEditor::ConfigInterface
    //
public:
    QStringList configKeys() const Q_DECL_OVERRIDE;
    QVariant configValue(const QString &key) Q_DECL_OVERRIDE;
    void setConfigValue(const QString &key, const QVariant &value) Q_DECL_OVERRIDE;

Q_SIGNALS:
    void configChanged();

public:
    /**
     * Try to fold starting at the given line.
     * This will both try to fold existing folding ranges of this line and to query the highlighting what to fold.
     * @param startLine start line to fold at
     */
    void foldLine(int startLine);

    /**
    * Try to unfold all foldings starting at the given line.
    * @param startLine start line to unfold at
    */
    void unfoldLine(int startLine);

    //
    // KTextEditor::CodeCompletionInterface2
    //
public:
    bool isCompletionActive() const Q_DECL_OVERRIDE;
    void startCompletion(const KTextEditor::Range &word, KTextEditor::CodeCompletionModel *model) Q_DECL_OVERRIDE;
    void abortCompletion() Q_DECL_OVERRIDE;
    void forceCompletion() Q_DECL_OVERRIDE;
    void registerCompletionModel(KTextEditor::CodeCompletionModel *model) Q_DECL_OVERRIDE;
    void unregisterCompletionModel(KTextEditor::CodeCompletionModel *model) Q_DECL_OVERRIDE;
    bool isAutomaticInvocationEnabled() const Q_DECL_OVERRIDE;
    void setAutomaticInvocationEnabled(bool enabled = true) Q_DECL_OVERRIDE;

Q_SIGNALS:
    void completionExecuted(KTextEditor::View *view, const KTextEditor::Cursor &position, KTextEditor::CodeCompletionModel *model, const QModelIndex &);
    void completionAborted(KTextEditor::View *view);

public Q_SLOTS:
    void userInvokedCompletion();

public:
    KateCompletionWidget *completionWidget() const;
    mutable KateCompletionWidget *m_completionWidget;
    void sendCompletionExecuted(const KTextEditor::Cursor &position, KTextEditor::CodeCompletionModel *model, const QModelIndex &index);
    void sendCompletionAborted();

    //
    // KTextEditor::TextHintInterface
    //
public:
    void registerTextHintProvider(KTextEditor::TextHintProvider *provider) Q_DECL_OVERRIDE;
    void unregisterTextHintProvider(KTextEditor::TextHintProvider *provider) Q_DECL_OVERRIDE;
    void setTextHintDelay(int delay) Q_DECL_OVERRIDE;
    int textHintDelay() const Q_DECL_OVERRIDE;

public:
    bool dynWordWrap() const
    {
        return m_hasWrap;
    }

    //
    // KTextEditor::SelectionInterface stuff
    //
public Q_SLOTS:
    bool setSelection(const KTextEditor::Range &selection) Q_DECL_OVERRIDE;

    bool removeSelection() Q_DECL_OVERRIDE
    {
        return clearSelection();
    }

    bool removeSelectionText() Q_DECL_OVERRIDE
    {
        return removeSelectedText();
    }

    bool setBlockSelection(bool on) Q_DECL_OVERRIDE;
    bool toggleBlockSelection();

    bool clearSelection();
    bool clearSelection(bool redraw, bool finishedChangingSelection = true);

    bool removeSelectedText();

    bool selectAll();

public:
    bool selection() const Q_DECL_OVERRIDE;
    QString selectionText() const Q_DECL_OVERRIDE;
    bool blockSelection() const Q_DECL_OVERRIDE;
    KTextEditor::Range selectionRange() const Q_DECL_OVERRIDE;

    static void blockFix(KTextEditor::Range &range);

    //
    // Arbitrary Syntax HL + Action extensions
    //
public:
    // Action association extension
    void deactivateEditActions();
    void activateEditActions();

    //
    // internal helper stuff, for katerenderer and so on
    //
public:
    // should cursor be wrapped ? take config + blockselection state in account
    bool wrapCursor() const;

    // some internal functions to get selection state of a line/col
    bool cursorSelected(const KTextEditor::Cursor &cursor);
    bool lineSelected(int line);
    bool lineEndSelected(const KTextEditor::Cursor &lineEndPos);
    bool lineHasSelected(int line);
    bool lineIsSelection(int line);

    void ensureCursorColumnValid();

    void tagSelection(const KTextEditor::Range &oldSelection);

    void selectWord(const KTextEditor::Cursor &cursor);
    void selectLine(const KTextEditor::Cursor &cursor);

    //BEGIN EDIT STUFF
public:
    void editStart();
    void editEnd(int editTagLineStart, int editTagLineEnd, bool tagFrom);

    void editSetCursor(const KTextEditor::Cursor &cursor);
    //END

    //BEGIN TAG & CLEAR
public:
    bool tagLine(const KTextEditor::Cursor &virtualCursor);

    bool tagRange(const KTextEditor::Range &range, bool realLines = false);
    bool tagLines(int start, int end, bool realLines = false);
    bool tagLines(KTextEditor::Cursor start, KTextEditor::Cursor end, bool realCursors = false);
    bool tagLines(KTextEditor::Range range, bool realRange = false);

    void tagAll();

    void clear();

    void repaintText(bool paintOnlyDirty = false);

    void updateView(bool changed = false);
    //END

    //
    // KTextEditor::AnnotationView
    //
public:
    void setAnnotationModel(KTextEditor::AnnotationModel *model) Q_DECL_OVERRIDE;
    KTextEditor::AnnotationModel *annotationModel() const Q_DECL_OVERRIDE;
    void setAnnotationBorderVisible(bool visible) Q_DECL_OVERRIDE;
    bool isAnnotationBorderVisible() const Q_DECL_OVERRIDE;

Q_SIGNALS:
    void annotationContextMenuAboutToShow(KTextEditor::View *view, QMenu *menu, int line) Q_DECL_OVERRIDE;
    void annotationActivated(KTextEditor::View *view, int line) Q_DECL_OVERRIDE;
    void annotationBorderVisibilityChanged(View *view, bool visible) Q_DECL_OVERRIDE;

    void navigateLeft();
    void navigateRight();
    void navigateUp();
    void navigateDown();
    void navigateAccept();
    void navigateBack();

private:
    KTextEditor::AnnotationModel *m_annotationModel;

    //
    // KTextEditor::View
    //
public:
    void emitNavigateLeft()
    {
        emit navigateLeft();
    }
    void emitNavigateRight()
    {
        emit navigateRight();
    }
    void emitNavigateUp()
    {
        emit navigateUp();
    }
    void emitNavigateDown()
    {
        emit navigateDown();
    }
    void emitNavigateAccept()
    {
        emit navigateAccept();
    }
    void emitNavigateBack()
    {
        emit navigateBack();
    }
    /**
     Return values for "save" related commands.
    */
    bool isOverwriteMode() const;

    QString currentTextLine();

public Q_SLOTS:
    void indent();
    void unIndent();
    void cleanIndent();
    void align();
    void comment();
    void uncomment();
    void toggleComment();
    void killLine();

    /**
     * Sets the cursor to the previous editing position in this document
     */
    void goToPreviousEditingPosition();

    /**
     * Sets the cursor to the next editing position in this document
     */
    void goToNextEditingPosition();

    /**
      Uppercases selected text, or an alphabetic character next to the cursor.
    */
    void uppercase();
    /**
      Lowercases selected text, or an alphabetic character next to the cursor.
    */
    void lowercase();
    /**
      Capitalizes the selection (makes each word start with an uppercase) or
      the word under the cursor.
    */
    void capitalize();
    /**
      Joins lines touched by the selection
    */
    void joinLines();

    // Note - the following functions simply forward to KateViewInternal
    void keyReturn();
    void smartNewline();
    void backspace();
    void deleteWordLeft();
    void keyDelete();
    void deleteWordRight();
    void transpose();
    void cursorLeft();
    void shiftCursorLeft();
    void cursorRight();
    void shiftCursorRight();
    void wordLeft();
    void shiftWordLeft();
    void wordRight();
    void shiftWordRight();
    void home();
    void shiftHome();
    void end();
    void shiftEnd();
    void up();
    void shiftUp();
    void down();
    void shiftDown();
    void scrollUp();
    void scrollDown();
    void topOfView();
    void shiftTopOfView();
    void bottomOfView();
    void shiftBottomOfView();
    void pageUp();
    void shiftPageUp();
    void pageDown();
    void shiftPageDown();
    void top();
    void shiftTop();
    void bottom();
    void shiftBottom();
    void toMatchingBracket();
    void shiftToMatchingBracket();
    void toPrevModifiedLine();
    void toNextModifiedLine();
    void insertTab();

    void gotoLine();

    // config file / session management functions
public:
    /**
     * Read session settings from the given \p config.
     *
     * Known flags:
     *  "SkipUrl" => don't save/restore the file
     *  "SkipMode" => don't save/restore the mode
     *  "SkipHighlighting" => don't save/restore the highlighting
     *  "SkipEncoding" => don't save/restore the encoding
     *
     * \param config read the session settings from this KConfigGroup
     * \param flags additional flags
     * \see writeSessionConfig()
     */
    void readSessionConfig(const KConfigGroup &config, const QSet<QString> &flags = QSet<QString>()) Q_DECL_OVERRIDE;

    /**
     * Write session settings to the \p config.
     * See readSessionConfig() for more details.
     *
     * \param config write the session settings to this KConfigGroup
     * \param flags additional flags
     * \see readSessionConfig()
     */
    void writeSessionConfig(KConfigGroup &config, const QSet<QString> &flags = QSet<QString>()) Q_DECL_OVERRIDE;

public Q_SLOTS:
    void setEol(int eol);
    void setAddBom(bool enabled);
    void find();
    void findSelectedForwards();
    void findSelectedBackwards();
    void replace();
    void findNext();
    void findPrevious();

    void setFoldingMarkersOn(bool enable);   // Not in KTextEditor::View, but should be
    void setIconBorder(bool enable);
    void setLineNumbersOn(bool enable);
    void setScrollBarMarks(bool enable);
    void setScrollBarMiniMap(bool enable);
    void setScrollBarMiniMapAll(bool enable);
    void setScrollBarMiniMapWidth(int width);
    void toggleFoldingMarkers();
    void toggleIconBorder();
    void toggleLineNumbersOn();
    void toggleScrollBarMarks();
    void toggleScrollBarMiniMap();
    void toggleScrollBarMiniMapAll();
    void toggleDynWordWrap();
    void setDynWrapIndicators(int mode);

public:
    int getEol() const;

public:
    KateRenderer *renderer();

    bool iconBorder();
    bool lineNumbersOn();
    bool scrollBarMarks();
    bool scrollBarMiniMap();
    bool scrollBarMiniMapAll();
    int dynWrapIndicators();
    bool foldingMarkersOn();

private Q_SLOTS:
    /**
     * used to update actions after selection changed
     */
    void slotSelectionChanged();

    void toggleInputMode(bool);

public:
    /**
     * accessor to katedocument pointer
     * @return pointer to document
     */
    KTextEditor::DocumentPrivate  *doc()
    {
        return m_doc;
    }
    const KTextEditor::DocumentPrivate  *doc() const
    {
        return m_doc;
    }

public Q_SLOTS:
    void slotUpdateUndo();
    void toggleInsert();
    void reloadFile();
    void toggleWWMarker();
    void toggleNPSpaces();
    void toggleWordCount(bool on);
    void toggleWriteLock();
    void switchToCmdLine();
    void slotReadWriteChanged();
    void slotClipboardHistoryChanged();

Q_SIGNALS:
    void dropEventPass(QDropEvent *);

public:
    /**
     * Folding handler for this view.
     * @return folding handler
     */
    Kate::TextFolding &textFolding()
    {
        return m_textFolding;
    }

public:
    void slotTextInserted(KTextEditor::View *view, const KTextEditor::Cursor &position, const QString &text);

    void exportHtmlToFile(const QString &file);

private Q_SLOTS:
    void slotGotFocus();
    void slotLostFocus();
    void slotSaveCanceled(const QString &error);
    void slotConfigDialog();
    void exportHtmlToClipboard ();
    void exportHtmlToFile ();

public Q_SLOTS: // TODO: turn into good interface, see kte5/foldinginterface.h
    void slotFoldToplevelNodes();
    void slotCollapseLocal();
    void slotCollapseLevel();
    void slotExpandLevel();
    void slotExpandLocal();

private:
    void setupLayout();
    void setupConnections();
    void setupActions();
    void setupEditActions();
    void setupCodeFolding();

    QList<QAction *>        m_editActions;
    QAction               *m_editUndo;
    QAction               *m_editRedo;
    QAction               *m_pasteMenu;
    KToggleAction         *m_toggleFoldingMarkers;
    KToggleAction         *m_toggleIconBar;
    KToggleAction         *m_toggleLineNumbers;
    KToggleAction         *m_toggleScrollBarMarks;
    KToggleAction         *m_toggleScrollBarMiniMap;
    KToggleAction         *m_toggleScrollBarMiniMapAll;
    KToggleAction         *m_toggleDynWrap;
    KSelectAction         *m_setDynWrapIndicators;
    KToggleAction         *m_toggleWWMarker;
    KToggleAction         *m_toggleNPSpaces;
    KToggleAction         *m_toggleWordCount;
    QAction               *m_switchCmdLine;
    KToggleAction         *m_viInputModeAction;

    KSelectAction         *m_setEndOfLine;
    KToggleAction         *m_addBom;

    QAction *m_cut;
    QAction *m_copy;
    QAction *m_copyHtmlAction;
    QAction *m_paste;
    QAction *m_selectAll;
    QAction *m_deSelect;

    QList<QAction *> m_inputModeActions;

    KToggleAction *m_toggleBlockSelection;
    KToggleAction *m_toggleInsert;
    KToggleAction *m_toggleWriteLock;

    bool m_hasWrap;

    KTextEditor::DocumentPrivate     *const m_doc;
    Kate::TextFolding m_textFolding;
    KateViewConfig   *const m_config;
    KateRenderer     *const m_renderer;
    KateViewInternal *const m_viewInternal;
    KateSpellCheckDialog  *m_spell;
    KateBookmarks    *const m_bookmarks;

    //* margins
    QSpacerItem *m_topSpacer;
    QSpacerItem *m_leftSpacer;
    QSpacerItem *m_rightSpacer;
    QSpacerItem *m_bottomSpacer;

private Q_SLOTS:
    void slotHlChanged();

    /**
     * Configuration
     */
public:
    inline KateViewConfig *config()
    {
        return m_config;
    }

    void updateConfig();

    void updateDocumentConfig();

    void updateRendererConfig();

private Q_SLOTS:
    void updateFoldingConfig();

private:
    bool m_startingUp;
    bool m_updatingDocumentConfig;

    // stores the current selection
    Kate::TextRange m_selection;

    // do we select normal or blockwise ?
    bool blockSelect;

    // templates
public:
    bool insertTemplateInternal(const KTextEditor::Cursor& insertPosition,
                                const QString& templateString,
                                const QString& script = QString());

    /**
     * Accessors to the bars...
     */
public:
    KateViewBar *bottomViewBar() const;
    KateDictionaryBar *dictionaryBar();

private:
    KateGotoBar *gotoBar();

    /**
     * viewbar + its widgets
     * they are created on demand...
     */
private:
    // created in constructor of the view
    KateViewBar *m_bottomViewBar;

    // created on demand..., only access them through the above accessors....
    KateGotoBar *m_gotoBar;
    KateDictionaryBar *m_dictionaryBar;

    // input modes
public:
    KateAbstractInputMode *currentInputMode() const;
public:
    KTextEditor::Range visibleRange();

Q_SIGNALS:
    void displayRangeChanged(KTextEditor::ViewPrivate *view);

protected:
    bool event(QEvent *e) Q_DECL_OVERRIDE;
    void paintEvent(QPaintEvent *e) Q_DECL_OVERRIDE;

    KToggleAction               *m_toggleOnTheFlySpellCheck;
    KateSpellingMenu *m_spellingMenu;

protected Q_SLOTS:
    void toggleOnTheFlySpellCheck(bool b);

public Q_SLOTS:
    void changeDictionary();
    void reflectOnTheFlySpellCheckStatus(bool enabled);

public:
    KateSpellingMenu *spellingMenu();
private:
    bool m_userContextMenuSet;

private Q_SLOTS:
    /**
     * save folding state before document reload
     */
    void saveFoldingState();

    /**
     * restore folding state after document reload
     */
    void applyFoldingState();

    void clearHighlights();
    void createHighlights();

private:
    void selectionChangedForHighlights();

    /**
     * saved folding state
     */
    QJsonDocument m_savedFoldingState;

    QString m_currentTextForHighlights;

    QList<KTextEditor::MovingRange*> m_rangesForHighlights;

public:
    /**
     * Attribute of a range changed or range with attribute changed in given line range.
     * @param startLine start line of change
     * @param endLine end line of change
     * @param rangeWithAttribute attribute changed or is active, this will perhaps lead to repaints
     */
    void notifyAboutRangeChange(int startLine, int endLine, bool rangeWithAttribute);

private Q_SLOTS:
    /**
     * Delayed update for view after text ranges changed
     */
    void slotDelayedUpdateOfView();

Q_SIGNALS:
    /**
     * Delayed update for view after text ranges changed
     */
    void delayedUpdateOfView();

public:
    /**
     * set of ranges which had the mouse inside last time, used for rendering
     * @return set of ranges which had the mouse inside last time checked
     */
    const QSet<Kate::TextRange *> *rangesMouseIn() const
    {
        return &m_rangesMouseIn;
    }

    /**
     * set of ranges which had the caret inside last time, used for rendering
     * @return set of ranges which had the caret inside last time checked
     */
    const QSet<Kate::TextRange *> *rangesCaretIn() const
    {
        return &m_rangesCaretIn;
    }

    /**
     * check if ranges changed for mouse in and caret in
     * @param activationType type of activation to check
     */
    void updateRangesIn(KTextEditor::Attribute::ActivationType activationType);

    //
    // helpers for delayed view update after ranges changes
    //
private:
    /**
     * update already inited?
     */
    bool m_delayedUpdateTriggered;

    /**
     * minimal line to update
     */
    int m_lineToUpdateMin;

    /**
     * maximal line to update
     */
    int m_lineToUpdateMax;

    /**
     * set of ranges which had the mouse inside last time
     */
    QSet<Kate::TextRange *> m_rangesMouseIn;

    /**
     * set of ranges which had the caret inside last time
     */
    QSet<Kate::TextRange *> m_rangesCaretIn;

    //
    // forward impl for KTextEditor::MessageInterface
    //
public:
    /**
     * Used by Document::postMessage().
     */
    void postMessage(KTextEditor::Message *message, QList<QSharedPointer<QAction> > actions);

private:
    /** Message widget showing KTextEditor::Messages above the View. */
    KateMessageWidget *m_topMessageWidget;
    /** Message widget showing KTextEditor::Messages below the View. */
    KateMessageWidget *m_bottomMessageWidget;
    /** Message widget showing KTextEditor::Messages as view overlay in top right corner. */
    KateMessageWidget *m_floatTopMessageWidget;
    /** Message widget showing KTextEditor::Messages as view overlay in bottom left corner. */
    KateMessageWidget *m_floatBottomMessageWidget;
    /** Layout for floating notifications */
    QVBoxLayout *m_notificationLayout;

    // for unit test 'tests/messagetest.cpp'
public:
    KateMessageWidget *messageWidget()
    {
        return m_floatTopMessageWidget;
    }

private:
    /**
     * The main window responsible for this view, if any
     */
    QPointer<KTextEditor::MainWindow> m_mainWindow;

    //
    // KTextEditor::PrintInterface
    //
public Q_SLOTS:
    bool print() Q_DECL_OVERRIDE;
    void printPreview() Q_DECL_OVERRIDE;

public:
    /**
     * Get the view status bar
     * @return status bar, in enabled
     */
    KateStatusBar *statusBar () const {
        return m_statusBar;
    }

    /**
     * Toogle status bar, e.g. create or remove it
     */
    void toggleStatusBar ();

    /**
     * Get the encoding menu
     * @return the encoding menu
     */
    KateViewEncodingAction *encodingAction () const {
        return m_encodingAction;
    }

    /**
     * Get the mode menu
     * @return the mode menu
     */
    KateModeMenu *modeAction () const {
        return m_modeAction;
    }

private:
    /**
     * the status bar of this view
     */
    KateStatusBar *m_statusBar;

    /**
     * the encoding selection menu, used by view + status bar
     */
    KateViewEncodingAction *m_encodingAction;

    /**
     * the mode selection menu, used by view + status bar
     */
    KateModeMenu *m_modeAction;

    /**
     * is automatic invocation of completion disabled temporarily?
     */
    bool m_temporaryAutomaticInvocationDisabled;

public:
    /**
     * Returns the attribute for the default style \p defaultStyle.
     */
    Attribute::Ptr defaultStyleAttribute(DefaultStyle defaultStyle) const Q_DECL_OVERRIDE;

    /**
     * Get the list of AttributeBlocks for a given \p line in the document.
     *
     * \return list of AttributeBlocks for given \p line.
     */
    QList<KTextEditor::AttributeBlock> lineAttributes(int line) Q_DECL_OVERRIDE;

private:
    // remember folding state to prevent refolding the first line if it was manually unfolded,
    // e.g. when saving a file or changing other config vars
    bool m_autoFoldedFirstLine;
};

}

#endif

