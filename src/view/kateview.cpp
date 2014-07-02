/* This file is part of the KDE libraries
   Copyright (C) 2009 Michel Ludwig <michel.ludwig@kdemail.net>
   Copyright (C) 2007 Mirko Stocker <me@misto.ch>
   Copyright (C) 2003 Hamish Rodda <rodda@kde.org>
   Copyright (C) 2002 John Firebaugh <jfirebaugh@kde.org>
   Copyright (C) 2001-2004 Christoph Cullmann <cullmann@kde.org>
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

//BEGIN includes
#include "kateview.h"

#include "kateviewinternal.h"
#include "kateviewhelpers.h"
#include "katerenderer.h"
#include "katedocument.h"
#include "kateundomanager.h"
#include "kateglobal.h"
#include "katehighlight.h"
#include "katehighlightmenu.h"
#include "katedialogs.h"
#include "katetextline.h"
#include "kateschema.h"
#include "katebookmarks.h"
#include "kateconfig.h"
#include "katemodemenu.h"
#include "kateautoindent.h"
#include "katecompletionwidget.h"
#include "katewordcompletion.h"
#include "katekeywordcompletion.h"
#include "katelayoutcache.h"
#include "spellcheck/spellcheck.h"
#include "spellcheck/spellcheckdialog.h"
#include "spellcheck/spellingmenu.h"
#include "katebuffer.h"
#include "script/katescriptmanager.h"
#include "script/katescriptaction.h"
#include "export/exporter.h"
#include "katemessagewidget.h"
#include "katetemplatehandler.h"
#include "katepartdebug.h"
#include "printing/kateprinter.h"
#include "katestatusbar.h"
#include "kateabstractinputmode.h"

#include <KTextEditor/Message>

#include <KParts/Event>

#include <KConfig>
#include <KConfigGroup>
#include <KCursor>
#include <KCharsets>
#include <KMessageBox>
#include <KStandardAction>
#include <KXMLGUIFactory>
#include <KXMLGUIClient>
#include <KEncodingFileDialog>
#include <KStandardShortcut>
#include <KToggleAction>
#include <KSelectAction>
#include <KActionCollection>

#include <QFileInfo>
#include <QMimeData>
#include <QFont>
#include <QKeyEvent>
#include <QApplication>
#include <QLayout>
#include <QClipboard>

//#define VIEW_RANGE_DEBUG

//END includes

void KTextEditor::ViewPrivate::blockFix(KTextEditor::Range &range)
{
    if (range.start().column() > range.end().column()) {
        int tmp = range.start().column();
        range.setStart(KTextEditor::Cursor(range.start().line(), range.end().column()));
        range.setEnd(KTextEditor::Cursor(range.end().line(), tmp));
    }
}

KTextEditor::ViewPrivate::ViewPrivate(KTextEditor::DocumentPrivate *doc, QWidget *parent, KTextEditor::MainWindow *mainWindow)
    : KTextEditor::View (this, parent)
    , m_completionWidget(0)
    , m_annotationModel(0)
    , m_hasWrap(false)
    , m_doc(doc)
    , m_textFolding(doc->buffer())
    , m_config(new KateViewConfig(this))
    , m_renderer(new KateRenderer(doc, m_textFolding, this))
    , m_viewInternal(new KateViewInternal(this))
    , m_spell(new KateSpellCheckDialog(this))
    , m_bookmarks(new KateBookmarks(this))
    , m_startingUp(true)
    , m_updatingDocumentConfig(false)
    , m_selection(m_doc->buffer(), KTextEditor::Range::invalid(), Kate::TextRange::ExpandLeft, Kate::TextRange::AllowEmpty)
    , blockSelect(false)
    , m_bottomViewBar(0)
    , m_gotoBar(0)
    , m_dictionaryBar(NULL)
    , m_spellingMenu(new KateSpellingMenu(this))
    , m_userContextMenuSet(false)
    , m_delayedUpdateTriggered(false)
    , m_lineToUpdateMin(-1)
    , m_lineToUpdateMax(-1)
    , m_floatTopMessageWidget(0)
    , m_floatBottomMessageWidget(0)
    , m_mainWindow(mainWindow)
    , m_statusBar(nullptr)
{
    // queued connect to collapse view updates for range changes, INIT THIS EARLY ENOUGH!
    connect(this, SIGNAL(delayedUpdateOfView()), this, SLOT(slotDelayedUpdateOfView()), Qt::QueuedConnection);

    KXMLGUIClient::setComponentName(KTextEditor::EditorPrivate::self()->aboutData().componentName(), KTextEditor::EditorPrivate::self()->aboutData().displayName());

    // selection if for this view only and will invalidate if becoming empty
    m_selection.setView(this);

    // use z depth defined in moving ranges interface
    m_selection.setZDepth(-100000.0);

    KTextEditor::EditorPrivate::self()->registerView(this);

    /**
     * try to let the main window, if any, create a view bar for this view
     */
    QWidget *bottomBarParent = m_mainWindow->createViewBar(this);

    m_bottomViewBar = new KateViewBar(bottomBarParent != 0, bottomBarParent ? bottomBarParent : this, this);

    // ugly workaround:
    // Force the layout to be left-to-right even on RTL deskstop, as discussed
    // on the mailing list. This will cause the lines and icons panel to be on
    // the left, even for Arabic/Hebrew/Farsi/whatever users.
    setLayoutDirection(Qt::LeftToRight);

    // layouting ;)
    m_vBox = new QVBoxLayout(this);
    m_vBox->setMargin(0);
    m_vBox->setSpacing(0);

    m_bottomViewBar->installEventFilter(m_viewInternal);

    // add KateMessageWidget for KTE::MessageInterface immediately above view
    m_topMessageWidget = new KateMessageWidget(this);
    m_vBox->addWidget(m_topMessageWidget);
    m_topMessageWidget->hide();

    // add hbox: KateIconBorder | KateViewInternal | KateScrollBar
    QHBoxLayout *hbox = new QHBoxLayout();
    m_vBox->addLayout(hbox, 100);
    hbox->setMargin(0);
    hbox->setSpacing(0);

    /**
     * add ICONBORDER | VIEWINTERNAL | LINE-SCROLLBAR
     */
    hbox->addWidget(m_viewInternal->m_leftBorder);
    hbox->addWidget(m_viewInternal);
    hbox->addWidget(m_viewInternal->m_lineScroll);

    // add hbox: ColumnsScrollBar | Dummy
    hbox = new QHBoxLayout();
    m_vBox->addLayout(hbox);
    hbox->setMargin(0);
    hbox->setSpacing(0);

    hbox->addWidget(m_viewInternal->m_columnScroll);
    hbox->addWidget(m_viewInternal->m_dummy);

    // add KateMessageWidget for KTE::MessageInterface immediately above view
    m_bottomMessageWidget = new KateMessageWidget(this);
    m_vBox->addWidget(m_bottomMessageWidget);
    m_bottomMessageWidget->hide();

    // add bottom viewbar...
    if (bottomBarParent) {
        m_mainWindow->addWidgetToViewBar(this, m_bottomViewBar);
    } else {
        m_vBox->addWidget(m_bottomViewBar);
    }

    // add layout for floating message widgets to KateViewInternal
    m_notificationLayout = new QVBoxLayout(m_viewInternal);
    m_notificationLayout->setContentsMargins(20, 20, 20, 20);
    m_viewInternal->setLayout(m_notificationLayout);

    // this really is needed :)
    m_viewInternal->updateView();

    doc->addView(this);

    setFocusProxy(m_viewInternal);
    setFocusPolicy(Qt::StrongFocus);

    setXMLFile(QLatin1String("katepart5ui.rc"));

    setupConnections();
    setupActions();

    // auto word completion
    new KateWordCompletionView(this, actionCollection());

    // update the enabled state of the undo/redo actions...
    slotUpdateUndo();

    /**
     * create the status bar of this view
     * do this after action creation, we use some of them!
     */
    toggleStatusBar();

    m_startingUp = false;
    updateConfig();

    slotHlChanged();
    KCursor::setAutoHideCursor(m_viewInternal, true);

    // user interaction (scrollling) starts notification auto-hide timer
    connect(this, SIGNAL(displayRangeChanged(KTextEditor::ViewPrivate*)), m_topMessageWidget, SLOT(startAutoHideTimer()));
    connect(this, SIGNAL(displayRangeChanged(KTextEditor::ViewPrivate*)), m_bottomMessageWidget, SLOT(startAutoHideTimer()));

    // user interaction (cursor navigation) starts notification auto-hide timer
    connect(this, SIGNAL(cursorPositionChanged(KTextEditor::View*,KTextEditor::Cursor)), m_topMessageWidget, SLOT(startAutoHideTimer()));
    connect(this, SIGNAL(cursorPositionChanged(KTextEditor::View*,KTextEditor::Cursor)), m_bottomMessageWidget, SLOT(startAutoHideTimer()));

    // folding restoration on reload
    connect(m_doc, SIGNAL(aboutToReload(KTextEditor::Document*)), SLOT(saveFoldingState()));
    connect(m_doc, SIGNAL(reloaded(KTextEditor::Document*)), SLOT(applyFoldingState()));
    
    // clear highlights on reload
    connect(m_doc, SIGNAL(aboutToReload(KTextEditor::Document*)), SLOT(clearHighlights()));
}

KTextEditor::ViewPrivate::~ViewPrivate()
{
    // invalidate update signal
    m_delayedUpdateTriggered = false;

    // remove from xmlgui factory, to be safe
    if (factory()) {
        factory()->removeClient(this);
    }

    /**
     * remove view bar again, if needed
     */
    m_mainWindow->deleteViewBar(this);
    m_bottomViewBar = 0;

    m_doc->removeView(this);

    delete m_viewInternal;

    delete m_renderer;

    delete m_config;

    KTextEditor::EditorPrivate::self()->deregisterView(this);
}

void KTextEditor::ViewPrivate::toggleStatusBar()
{
    /**
     * if there, delete it
     */
    if (m_statusBar) {
        bottomViewBar()->removePermanentBarWidget(m_statusBar);
        delete m_statusBar;
        m_statusBar = nullptr;
        emit statusBarEnabledChanged(this, false);
        return;
    }

    /**
     * else: create it
     */
    m_statusBar = new KateStatusBar(this);
    bottomViewBar()->addPermanentBarWidget(m_statusBar);
    emit statusBarEnabledChanged(this, true);
}

void KTextEditor::ViewPrivate::setupConnections()
{
    connect(m_doc, SIGNAL(undoChanged()),
            this, SLOT(slotUpdateUndo()));
    connect(m_doc, SIGNAL(highlightingModeChanged(KTextEditor::Document*)),
            this, SLOT(slotHlChanged()));
    connect(m_doc, SIGNAL(canceled(QString)),
            this, SLOT(slotSaveCanceled(QString)));
    connect(m_viewInternal, SIGNAL(dropEventPass(QDropEvent*)),
            this,           SIGNAL(dropEventPass(QDropEvent*)));

    connect(m_doc, SIGNAL(annotationModelChanged(KTextEditor::AnnotationModel*,KTextEditor::AnnotationModel*)),
            m_viewInternal->m_leftBorder, SLOT(annotationModelChanged(KTextEditor::AnnotationModel*,KTextEditor::AnnotationModel*)));
}

void KTextEditor::ViewPrivate::setupActions()
{
    KActionCollection *ac = actionCollection();
    QAction *a;

    m_toggleWriteLock = 0;

    m_cut = a = ac->addAction(KStandardAction::Cut, this, SLOT(cut()));
    a->setWhatsThis(i18n("Cut the selected text and move it to the clipboard"));

    m_paste = a = ac->addAction(KStandardAction::PasteText, this, SLOT(paste()));
    a->setWhatsThis(i18n("Paste previously copied or cut clipboard contents"));

    m_copy = a = ac->addAction(KStandardAction::Copy, this, SLOT(copy()));
    a->setWhatsThis(i18n("Use this command to copy the currently selected text to the system clipboard."));

    m_pasteMenu = ac->addAction(QLatin1String("edit_paste_menu"), new KatePasteMenu(i18n("Clipboard &History"), this));
    connect(KTextEditor::EditorPrivate::self(), SIGNAL(clipboardHistoryChanged()), this, SLOT(slotClipboardHistoryChanged()));

    if (!m_doc->readOnly()) {
        a = ac->addAction(KStandardAction::Save, m_doc, SLOT(documentSave()));
        a->setWhatsThis(i18n("Save the current document"));

        a = m_editUndo = ac->addAction(KStandardAction::Undo, m_doc, SLOT(undo()));
        a->setWhatsThis(i18n("Revert the most recent editing actions"));

        a = m_editRedo = ac->addAction(KStandardAction::Redo, m_doc, SLOT(redo()));
        a->setWhatsThis(i18n("Revert the most recent undo operation"));

        // Tools > Scripts
        KateScriptActionMenu *scriptActionMenu = new KateScriptActionMenu(this, i18n("&Scripts"));
        ac->addAction(QLatin1String("tools_scripts"), scriptActionMenu);

        a = ac->addAction(QLatin1String("tools_apply_wordwrap"));
        a->setText(i18n("Apply &Word Wrap"));
        a->setWhatsThis(i18n("Use this command to wrap all lines of the current document which are longer than the width of the"
                             " current view, to fit into this view.<br /><br /> This is a static word wrap, meaning it is not updated"
                             " when the view is resized."));
        connect(a, SIGNAL(triggered(bool)), SLOT(applyWordWrap()));

        a = ac->addAction(QLatin1String("tools_cleanIndent"));
        a->setText(i18n("&Clean Indentation"));
        a->setWhatsThis(i18n("Use this to clean the indentation of a selected block of text (only tabs/only spaces).<br /><br />"
                             "You can configure whether tabs should be honored and used or replaced with spaces, in the configuration dialog."));
        connect(a, SIGNAL(triggered(bool)), SLOT(cleanIndent()));

        a = ac->addAction(QLatin1String("tools_align"));
        a->setText(i18n("&Align"));
        a->setWhatsThis(i18n("Use this to align the current line or block of text to its proper indent level."));
        connect(a, SIGNAL(triggered(bool)), SLOT(align()));

        a = ac->addAction(QLatin1String("tools_comment"));
        a->setText(i18n("C&omment"));
        a->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_D));
        a->setWhatsThis(i18n("This command comments out the current line or a selected block of text.<br /><br />"
                             "The characters for single/multiple line comments are defined within the language's highlighting."));
        connect(a, SIGNAL(triggered(bool)), SLOT(comment()));

        a = ac->addAction(QLatin1String("tools_uncomment"));
        a->setText(i18n("Unco&mment"));
        a->setShortcut(QKeySequence(Qt::CTRL + Qt::SHIFT + Qt::Key_D));
        a->setWhatsThis(i18n("This command removes comments from the current line or a selected block of text.<br /><br />"
                             "The characters for single/multiple line comments are defined within the language's highlighting."));
        connect(a, SIGNAL(triggered(bool)), SLOT(uncomment()));

        a = ac->addAction(QLatin1String("tools_toggle_comment"));
        a->setText(i18n("Toggle Comment"));
        connect(a, SIGNAL(triggered(bool)), SLOT(toggleComment()));

        a = m_toggleWriteLock = new KToggleAction(i18n("&Read Only Mode"), this);
        a->setWhatsThis(i18n("Lock/unlock the document for writing"));
        a->setChecked(!m_doc->isReadWrite());
        connect(a, SIGNAL(triggered(bool)), SLOT(toggleWriteLock()));
        ac->addAction(QLatin1String("tools_toggle_write_lock"), a);

        a = ac->addAction(QLatin1String("tools_uppercase"));
        a->setText(i18n("Uppercase"));
        a->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_U));
        a->setWhatsThis(i18n("Convert the selection to uppercase, or the character to the "
                             "right of the cursor if no text is selected."));
        connect(a, SIGNAL(triggered(bool)), SLOT(uppercase()));

        a = ac->addAction(QLatin1String("tools_lowercase"));
        a->setText(i18n("Lowercase"));
        a->setShortcut(QKeySequence(Qt::CTRL + Qt::SHIFT + Qt::Key_U));
        a->setWhatsThis(i18n("Convert the selection to lowercase, or the character to the "
                             "right of the cursor if no text is selected."));
        connect(a, SIGNAL(triggered(bool)), SLOT(lowercase()));

        a = ac->addAction(QLatin1String("tools_capitalize"));
        a->setText(i18n("Capitalize"));
        a->setShortcut(QKeySequence(Qt::CTRL + Qt::ALT + Qt::Key_U));
        a->setWhatsThis(i18n("Capitalize the selection, or the word under the "
                             "cursor if no text is selected."));
        connect(a, SIGNAL(triggered(bool)), SLOT(capitalize()));

        a = ac->addAction(QLatin1String("tools_join_lines"));
        a->setText(i18n("Join Lines"));
        a->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_J));
        connect(a, SIGNAL(triggered(bool)), SLOT(joinLines()));

        a = ac->addAction(QLatin1String("tools_invoke_code_completion"));
        a->setText(i18n("Invoke Code Completion"));
        a->setWhatsThis(i18n("Manually invoke command completion, usually by using a shortcut bound to this action."));
        a->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_Space));
        connect(a, SIGNAL(triggered(bool)), SLOT(userInvokedCompletion()));
    } else {
        m_cut->setEnabled(false);
        m_paste->setEnabled(false);
        m_pasteMenu->setEnabled(false);
        m_editUndo = 0;
        m_editRedo = 0;
    }

    a = ac->addAction(KStandardAction::Print, this, SLOT(print()));
    a->setWhatsThis(i18n("Print the current document."));

    a = ac->addAction(KStandardAction::PrintPreview, this, SLOT(printPreview()));
    a->setWhatsThis(i18n("Show print preview of current document"));

    a = ac->addAction(QLatin1String("file_reload"));
    a->setIcon(QIcon::fromTheme(QLatin1String("view-refresh")));
    a->setText(i18n("Reloa&d"));
    a->setShortcuts(KStandardShortcut::reload());
    a->setWhatsThis(i18n("Reload the current document from disk."));
    connect(a, SIGNAL(triggered(bool)), SLOT(reloadFile()));

    a = ac->addAction(KStandardAction::SaveAs, m_doc, SLOT(documentSaveAs()));
    a->setWhatsThis(i18n("Save the current document to disk, with a name of your choice."));

    a = ac->addAction(QLatin1String("file_save_copy_as"));
    a->setIcon(QIcon::fromTheme(QLatin1String("document-save-as")));
    a->setText(i18n("Save &Copy As..."));
    a->setWhatsThis(i18n("Save a copy of the current document to disk."));
    connect(a, SIGNAL(triggered(bool)), m_doc, SLOT(documentSaveCopyAs()));

    a = ac->addAction(KStandardAction::GotoLine, this, SLOT(gotoLine()));
    a->setWhatsThis(i18n("This command opens a dialog and lets you choose a line that you want the cursor to move to."));

    a = ac->addAction(QLatin1String("modified_line_up"));
    a->setText(i18n("Move to Previous Modified Line"));
    a->setWhatsThis(i18n("Move upwards to the previous modified line."));
    connect(a, SIGNAL(triggered(bool)), SLOT(toPrevModifiedLine()));

    a = ac->addAction(QLatin1String("modified_line_down"));
    a->setText(i18n("Move to Next Modified Line"));
    a->setWhatsThis(i18n("Move downwards to the next modified line."));
    connect(a, SIGNAL(triggered(bool)), SLOT(toNextModifiedLine()));

    a = ac->addAction(QLatin1String("set_confdlg"));
    a->setText(i18n("&Configure Editor..."));
    a->setWhatsThis(i18n("Configure various aspects of this editor."));
    connect(a, SIGNAL(triggered(bool)), SLOT(slotConfigDialog()));

    m_modeAction = new KateModeMenu(i18n("&Mode"), this);
    ac->addAction(QLatin1String("tools_mode"), m_modeAction);
    m_modeAction->setWhatsThis(i18n("Here you can choose which mode should be used for the current document. This will influence the highlighting and folding being used, for example."));
    m_modeAction->updateMenu(m_doc);

    KateHighlightingMenu *menu = new KateHighlightingMenu(i18n("&Highlighting"), this);
    ac->addAction(QLatin1String("tools_highlighting"), menu);
    menu->setWhatsThis(i18n("Here you can choose how the current document should be highlighted."));
    menu->updateMenu(m_doc);

    KateViewSchemaAction *schemaMenu = new KateViewSchemaAction(i18n("&Schema"), this);
    ac->addAction(QLatin1String("view_schemas"), schemaMenu);
    schemaMenu->updateMenu(this);

    // indentation menu
    KateViewIndentationAction *indentMenu = new KateViewIndentationAction(m_doc, i18n("&Indentation"), this);
    ac->addAction(QLatin1String("tools_indentation"), indentMenu);

    m_selectAll = a = ac->addAction(KStandardAction::SelectAll, this, SLOT(selectAll()));
    a->setWhatsThis(i18n("Select the entire text of the current document."));

    m_deSelect = a = ac->addAction(KStandardAction::Deselect, this, SLOT(clearSelection()));
    a->setWhatsThis(i18n("If you have selected something within the current document, this will no longer be selected."));

    a = ac->addAction(QLatin1String("view_inc_font_sizes"));
    a->setIcon(QIcon::fromTheme(QLatin1String("zoom-in")));
    a->setText(i18n("Enlarge Font"));
    a->setShortcuts(KStandardShortcut::zoomIn());
    a->setWhatsThis(i18n("This increases the display font size."));
    connect(a, SIGNAL(triggered(bool)), m_viewInternal, SLOT(slotIncFontSizes()));

    a = ac->addAction(QLatin1String("view_dec_font_sizes"));
    a->setIcon(QIcon::fromTheme(QLatin1String("zoom-out")));
    a->setText(i18n("Shrink Font"));
    a->setShortcuts(KStandardShortcut::zoomOut());
    a->setWhatsThis(i18n("This decreases the display font size."));
    connect(a, SIGNAL(triggered(bool)), m_viewInternal, SLOT(slotDecFontSizes()));

    a = m_toggleBlockSelection = new KToggleAction(i18n("Bl&ock Selection Mode"), this);
    ac->addAction(QLatin1String("set_verticalSelect"), a);
    a->setShortcut(QKeySequence(Qt::CTRL + Qt::SHIFT + Qt::Key_B));
    a->setWhatsThis(i18n("This command allows switching between the normal (line based) selection mode and the block selection mode."));
    connect(a, SIGNAL(triggered(bool)), SLOT(toggleBlockSelection()));

    a = m_toggleInsert = new KToggleAction(i18n("Overwr&ite Mode"), this);
    ac->addAction(QLatin1String("set_insert"), a);
    a->setShortcut(QKeySequence(Qt::Key_Insert));
    a->setWhatsThis(i18n("Choose whether you want the text you type to be inserted or to overwrite existing text."));
    connect(a, SIGNAL(triggered(bool)), SLOT(toggleInsert()));

    KToggleAction *toggleAction;
    a = m_toggleDynWrap = toggleAction = new KToggleAction(i18n("&Dynamic Word Wrap"), this);
    ac->addAction(QLatin1String("view_dynamic_word_wrap"), a);
    a->setShortcut(QKeySequence(Qt::Key_F10));
    a->setWhatsThis(i18n("If this option is checked, the text lines will be wrapped at the view border on the screen."));
    connect(a, SIGNAL(triggered(bool)), SLOT(toggleDynWordWrap()));

    a = m_setDynWrapIndicators = new KSelectAction(i18n("Dynamic Word Wrap Indicators"), this);
    ac->addAction(QLatin1String("dynamic_word_wrap_indicators"), a);
    a->setWhatsThis(i18n("Choose when the Dynamic Word Wrap Indicators should be displayed"));

    connect(m_setDynWrapIndicators, SIGNAL(triggered(int)), this, SLOT(setDynWrapIndicators(int)));
    QStringList list2;
    list2.append(i18n("&Off"));
    list2.append(i18n("Follow &Line Numbers"));
    list2.append(i18n("&Always On"));
    m_setDynWrapIndicators->setItems(list2);
    m_setDynWrapIndicators->setEnabled(m_toggleDynWrap->isChecked()); // only synced on real change, later

    a = toggleAction = m_toggleFoldingMarkers = new KToggleAction(i18n("Show Folding &Markers"), this);
    ac->addAction(QLatin1String("view_folding_markers"), a);
    a->setShortcut(QKeySequence(Qt::Key_F9));
    a->setWhatsThis(i18n("You can choose if the codefolding marks should be shown, if codefolding is possible."));
    connect(a, SIGNAL(triggered(bool)), SLOT(toggleFoldingMarkers()));

    a = m_toggleIconBar = toggleAction = new KToggleAction(i18n("Show &Icon Border"), this);
    ac->addAction(QLatin1String("view_border"), a);
    a->setShortcut(QKeySequence(Qt::Key_F6));
    a->setWhatsThis(i18n("Show/hide the icon border.<br /><br />The icon border shows bookmark symbols, for instance."));
    connect(a, SIGNAL(triggered(bool)), SLOT(toggleIconBorder()));

    a = toggleAction = m_toggleLineNumbers = new KToggleAction(i18n("Show &Line Numbers"), this);
    ac->addAction(QLatin1String("view_line_numbers"), a);
    a->setShortcut(QKeySequence(Qt::Key_F11));
    a->setWhatsThis(i18n("Show/hide the line numbers on the left hand side of the view."));
    connect(a, SIGNAL(triggered(bool)), SLOT(toggleLineNumbersOn()));

    a = m_toggleScrollBarMarks = toggleAction = new KToggleAction(i18n("Show Scroll&bar Marks"), this);
    ac->addAction(QLatin1String("view_scrollbar_marks"), a);
    a->setWhatsThis(i18n("Show/hide the marks on the vertical scrollbar.<br /><br />The marks show bookmarks, for instance."));
    connect(a, SIGNAL(triggered(bool)), SLOT(toggleScrollBarMarks()));

    a = m_toggleScrollBarMiniMap = toggleAction = new KToggleAction(i18n("Show Scrollbar Mini-Map"), this);
    ac->addAction(QLatin1String("view_scrollbar_minimap"), a);
    a->setWhatsThis(i18n("Show/hide the mini-map on the vertical scrollbar.<br /><br />The mini-map shows an overview of the whole document."));
    connect(a, SIGNAL(triggered(bool)), SLOT(toggleScrollBarMiniMap()));

//   a = m_toggleScrollBarMiniMapAll = toggleAction = new KToggleAction(i18n("Show the whole document in the Mini-Map"), this);
//   ac->addAction(QLatin1String("view_scrollbar_minimap_all"), a);
//   a->setWhatsThis(i18n("Display the whole document in the mini-map.<br /><br />With this option set the whole document will be visible in the mini-map."));
//   connect(a, SIGNAL(triggered(bool)), SLOT(toggleScrollBarMiniMapAll()));
//   connect(m_toggleScrollBarMiniMap, SIGNAL(triggered(bool)), m_toggleScrollBarMiniMapAll, SLOT(setEnabled(bool)));

    a = toggleAction = m_toggleWWMarker = new KToggleAction(i18n("Show Static &Word Wrap Marker"), this);
    ac->addAction(QLatin1String("view_word_wrap_marker"), a);
    a->setWhatsThis(i18n(
                        "Show/hide the Word Wrap Marker, a vertical line drawn at the word "
                        "wrap column as defined in the editing properties"));
    connect(a, SIGNAL(triggered(bool)), SLOT(toggleWWMarker()));

    a = m_toggleNPSpaces = new KToggleAction(i18n("Show Non-Printable Spaces"), this);
    ac->addAction(QLatin1String("view_non_printable_spaces"), a);
    a->setWhatsThis(i18n("Show/hide bounding box around non-printable spaces"));
    connect(a, SIGNAL(triggered(bool)), SLOT(toggleNPSpaces()));

    a = m_switchCmdLine = ac->addAction(QLatin1String("switch_to_cmd_line"));
    a->setText(i18n("Switch to Command Line"));
    a->setShortcut(QKeySequence(Qt::Key_F7));
    a->setWhatsThis(i18n("Show/hide the command line on the bottom of the view."));
    connect(a, SIGNAL(triggered(bool)), SLOT(switchToCmdLine()));

    KActionMenu *am = new KActionMenu(i18n("Input Modes"), this);
    ac->addAction(QLatin1String("view_input_modes"), am);

    Q_FOREACH(KateAbstractInputMode *mode, m_viewInternal->m_inputModes) {
        a = new KToggleAction(mode->viewInputModeHuman(), this);
        am->addAction(a);
        a->setWhatsThis(i18n("Activate/deactivate %1", mode->viewInputModeHuman()));
        a->setData(static_cast<int>(mode->viewInputMode()));
        connect(a, SIGNAL(triggered(bool)), SLOT(toggleInputMode(bool)));
        m_inputModeActions << a;
    }

    a = m_setEndOfLine = new KSelectAction(i18n("&End of Line"), this);
    ac->addAction(QLatin1String("set_eol"), a);
    a->setWhatsThis(i18n("Choose which line endings should be used, when you save the document"));
    QStringList list;
    list.append(i18nc("@item:inmenu End of Line", "&UNIX"));
    list.append(i18nc("@item:inmenu End of Line", "&Windows/DOS"));
    list.append(i18nc("@item:inmenu End of Line", "&Macintosh"));
    m_setEndOfLine->setItems(list);
    m_setEndOfLine->setCurrentItem(m_doc->config()->eol());
    connect(m_setEndOfLine, SIGNAL(triggered(int)), this, SLOT(setEol(int)));

    a = m_addBom = new KToggleAction(i18n("Add &Byte Order Mark (BOM)"), this);
    m_addBom->setChecked(m_doc->config()->bom());
    ac->addAction(QLatin1String("add_bom"), a);
    a->setWhatsThis(i18n("Enable/disable adding of byte order markers for UTF-8/UTF-16 encoded files while saving"));
    connect(m_addBom, SIGNAL(triggered(bool)), this, SLOT(setAddBom(bool)));
    // encoding menu
    m_encodingAction = new KateViewEncodingAction(m_doc, this, i18n("E&ncoding"), this);
    ac->addAction(QLatin1String("set_encoding"), m_encodingAction);

    a = ac->addAction(KStandardAction::Find, this, SLOT(find()));
    a->setWhatsThis(i18n("Look up the first occurrence of a piece of text or regular expression."));
    addAction(a);

    a = ac->addAction(QLatin1String("edit_find_selected"));
    a->setText(i18n("Find Selected"));
    a->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_H));
    a->setWhatsThis(i18n("Finds next occurrence of selected text."));
    connect(a, SIGNAL(triggered(bool)), SLOT(findSelectedForwards()));

    a = ac->addAction(QLatin1String("edit_find_selected_backwards"));
    a->setText(i18n("Find Selected Backwards"));
    a->setShortcut(QKeySequence(Qt::CTRL + Qt::SHIFT + Qt::Key_H));
    a->setWhatsThis(i18n("Finds previous occurrence of selected text."));
    connect(a, SIGNAL(triggered(bool)), SLOT(findSelectedBackwards()));

    a = ac->addAction(KStandardAction::FindNext, this, SLOT(findNext()));
    a->setWhatsThis(i18n("Look up the next occurrence of the search phrase."));
    addAction(a);

    a = ac->addAction(KStandardAction::FindPrev, QLatin1String("edit_find_prev"), this, SLOT(findPrevious()));
    a->setWhatsThis(i18n("Look up the previous occurrence of the search phrase."));
    addAction(a);

    a = ac->addAction(KStandardAction::Replace, this, SLOT(replace()));
    a->setWhatsThis(i18n("Look up a piece of text or regular expression and replace the result with some given text."));

    m_spell->createActions(ac);
    m_toggleOnTheFlySpellCheck = new KToggleAction(i18n("Automatic Spell Checking"), this);
    m_toggleOnTheFlySpellCheck->setWhatsThis(i18n("Enable/disable automatic spell checking"));
    m_toggleOnTheFlySpellCheck->setShortcut(QKeySequence(Qt::CTRL + Qt::SHIFT + Qt::Key_O));
    connect(m_toggleOnTheFlySpellCheck, SIGNAL(triggered(bool)), SLOT(toggleOnTheFlySpellCheck(bool)));
    ac->addAction(QLatin1String("tools_toggle_automatic_spell_checking"), m_toggleOnTheFlySpellCheck);

    a = ac->addAction(QLatin1String("tools_change_dictionary"));
    a->setText(i18n("Change Dictionary..."));
    a->setWhatsThis(i18n("Change the dictionary that is used for spell checking."));
    connect(a, SIGNAL(triggered()), SLOT(changeDictionary()));

    a = ac->addAction(QLatin1String("tools_clear_dictionary_ranges"));
    a->setText(i18n("Clear Dictionary Ranges"));
    a->setVisible(false);
    a->setWhatsThis(i18n("Remove all the separate dictionary ranges that were set for spell checking."));
    connect(a, SIGNAL(triggered()), m_doc, SLOT(clearDictionaryRanges()));
    connect(m_doc, SIGNAL(dictionaryRangesPresent(bool)), a, SLOT(setVisible(bool)));

    m_copyHtmlAction = ac->addAction(QLatin1String("edit_copy_html"), this, SLOT(exportHtmlToClipboard()));
    m_copyHtmlAction->setIcon(QIcon::fromTheme(QLatin1String("edit-copy")));
    m_copyHtmlAction->setText(i18n("Copy as &HTML"));
    m_copyHtmlAction->setWhatsThis(i18n("Use this command to copy the currently selected text as HTML to the system clipboard."));

    a = ac->addAction(QLatin1String("file_export_html"), this, SLOT(exportHtmlToFile()));
    a->setText(i18n("E&xport as HTML..."));
    a->setWhatsThis(i18n("This command allows you to export the current document"
                        " with all highlighting information into a HTML document."));

    m_spellingMenu->createActions(ac);

    m_bookmarks->createActions(ac);

    slotSelectionChanged();

    //Now setup the editing actions before adding the associated
    //widget and setting the shortcut context
    setupEditActions();
    setupCodeFolding();
    slotClipboardHistoryChanged();

    ac->addAssociatedWidget(m_viewInternal);

    foreach (QAction *action, ac->actions()) {
        action->setShortcutContext(Qt::WidgetWithChildrenShortcut);
    }

    connect(this, SIGNAL(selectionChanged(KTextEditor::View*)), this, SLOT(slotSelectionChanged()));
}

void KTextEditor::ViewPrivate::slotConfigDialog()
{
    // invoke config dialog, will auto-save configuration to katepartrc
    KTextEditor::EditorPrivate::self()->configDialog(this);
}

void KTextEditor::ViewPrivate::setupEditActions()
{
    //If you add an editing action to this
    //function make sure to include the line
    //m_editActions << a after creating the action
    KActionCollection *ac = actionCollection();

    QAction *a = ac->addAction(QLatin1String("word_left"));
    a->setText(i18n("Move Word Left"));
    a->setShortcuts(KStandardShortcut::backwardWord());
    connect(a, SIGNAL(triggered(bool)),  SLOT(wordLeft()));
    m_editActions << a;

    a = ac->addAction(QLatin1String("select_char_left"));
    a->setText(i18n("Select Character Left"));
    a->setShortcut(QKeySequence(Qt::SHIFT + Qt::Key_Left));
    connect(a, SIGNAL(triggered(bool)), SLOT(shiftCursorLeft()));
    m_editActions << a;

    a = ac->addAction(QLatin1String("select_word_left"));
    a->setText(i18n("Select Word Left"));
    a->setShortcut(QKeySequence(Qt::SHIFT + Qt::CTRL + Qt::Key_Left));
    connect(a, SIGNAL(triggered(bool)), SLOT(shiftWordLeft()));
    m_editActions << a;

    a = ac->addAction(QLatin1String("word_right"));
    a->setText(i18n("Move Word Right"));
    a->setShortcuts(KStandardShortcut::forwardWord());
    connect(a, SIGNAL(triggered(bool)), SLOT(wordRight()));
    m_editActions << a;

    a = ac->addAction(QLatin1String("select_char_right"));
    a->setText(i18n("Select Character Right"));
    a->setShortcut(QKeySequence(Qt::SHIFT + Qt::Key_Right));
    connect(a, SIGNAL(triggered(bool)), SLOT(shiftCursorRight()));
    m_editActions << a;

    a = ac->addAction(QLatin1String("select_word_right"));
    a->setText(i18n("Select Word Right"));
    a->setShortcut(QKeySequence(Qt::SHIFT + Qt::CTRL + Qt::Key_Right));
    connect(a, SIGNAL(triggered(bool)), SLOT(shiftWordRight()));
    m_editActions << a;

    a = ac->addAction(QLatin1String("beginning_of_line"));
    a->setText(i18n("Move to Beginning of Line"));
    a->setShortcuts(KStandardShortcut::beginningOfLine());
    connect(a, SIGNAL(triggered(bool)), SLOT(home()));
    m_editActions << a;

    a = ac->addAction(QLatin1String("beginning_of_document"));
    a->setText(i18n("Move to Beginning of Document"));
    a->setShortcuts(KStandardShortcut::begin());
    connect(a, SIGNAL(triggered(bool)), SLOT(top()));
    m_editActions << a;

    a = ac->addAction(QLatin1String("select_beginning_of_line"));
    a->setText(i18n("Select to Beginning of Line"));
    a->setShortcut(QKeySequence(Qt::SHIFT + Qt::Key_Home));
    connect(a, SIGNAL(triggered(bool)), SLOT(shiftHome()));
    m_editActions << a;

    a = ac->addAction(QLatin1String("select_beginning_of_document"));
    a->setText(i18n("Select to Beginning of Document"));
    a->setShortcut(QKeySequence(Qt::SHIFT + Qt::CTRL + Qt::Key_Home));
    connect(a, SIGNAL(triggered(bool)), SLOT(shiftTop()));
    m_editActions << a;

    a = ac->addAction(QLatin1String("end_of_line"));
    a->setText(i18n("Move to End of Line"));
    a->setShortcuts(KStandardShortcut::endOfLine());
    connect(a, SIGNAL(triggered(bool)), SLOT(end()));
    m_editActions << a;

    a = ac->addAction(QLatin1String("end_of_document"));
    a->setText(i18n("Move to End of Document"));
    a->setShortcuts(KStandardShortcut::end());
    connect(a, SIGNAL(triggered(bool)), SLOT(bottom()));
    m_editActions << a;

    a = ac->addAction(QLatin1String("select_end_of_line"));
    a->setText(i18n("Select to End of Line"));
    a->setShortcut(QKeySequence(Qt::SHIFT + Qt::Key_End));
    connect(a, SIGNAL(triggered(bool)), SLOT(shiftEnd()));
    m_editActions << a;

    a = ac->addAction(QLatin1String("select_end_of_document"));
    a->setText(i18n("Select to End of Document"));
    a->setShortcut(QKeySequence(Qt::SHIFT + Qt::CTRL + Qt::Key_End));
    connect(a, SIGNAL(triggered(bool)), SLOT(shiftBottom()));
    m_editActions << a;

    a = ac->addAction(QLatin1String("select_line_up"));
    a->setText(i18n("Select to Previous Line"));
    a->setShortcut(QKeySequence(Qt::SHIFT + Qt::Key_Up));
    connect(a, SIGNAL(triggered(bool)), SLOT(shiftUp()));
    m_editActions << a;

    a = ac->addAction(QLatin1String("scroll_line_up"));
    a->setText(i18n("Scroll Line Up"));
    a->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_Up));
    connect(a, SIGNAL(triggered(bool)), SLOT(scrollUp()));
    m_editActions << a;

    a = ac->addAction(QLatin1String("move_line_down"));
    a->setText(i18n("Move to Next Line"));
    a->setShortcut(QKeySequence(Qt::Key_Down));
    connect(a, SIGNAL(triggered(bool)), SLOT(down()));
    m_editActions << a;

    a = ac->addAction(QLatin1String("move_line_up"));
    a->setText(i18n("Move to Previous Line"));
    a->setShortcut(QKeySequence(Qt::Key_Up));
    connect(a, SIGNAL(triggered(bool)), SLOT(up()));
    m_editActions << a;

    a = ac->addAction(QLatin1String("move_cursor_right"));
    a->setText(i18n("Move Cursor Right"));
    a->setShortcut(QKeySequence(Qt::Key_Right));
    connect(a, SIGNAL(triggered(bool)), SLOT(cursorRight()));
    m_editActions << a;

    a = ac->addAction(QLatin1String("move_cusor_left"));
    a->setText(i18n("Move Cursor Left"));
    a->setShortcut(QKeySequence(Qt::Key_Left));
    connect(a, SIGNAL(triggered(bool)), SLOT(cursorLeft()));
    m_editActions << a;

    a = ac->addAction(QLatin1String("select_line_down"));
    a->setText(i18n("Select to Next Line"));
    a->setShortcut(QKeySequence(Qt::SHIFT + Qt::Key_Down));
    connect(a, SIGNAL(triggered(bool)), SLOT(shiftDown()));
    m_editActions << a;

    a = ac->addAction(QLatin1String("scroll_line_down"));
    a->setText(i18n("Scroll Line Down"));
    a->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_Down));
    connect(a, SIGNAL(triggered(bool)), SLOT(scrollDown()));
    m_editActions << a;

    a = ac->addAction(QLatin1String("scroll_page_up"));
    a->setText(i18n("Scroll Page Up"));
    a->setShortcuts(KStandardShortcut::prior());
    connect(a, SIGNAL(triggered(bool)), SLOT(pageUp()));
    m_editActions << a;

    a = ac->addAction(QLatin1String("select_page_up"));
    a->setText(i18n("Select Page Up"));
    a->setShortcut(QKeySequence(Qt::SHIFT + Qt::Key_PageUp));
    connect(a, SIGNAL(triggered(bool)), SLOT(shiftPageUp()));
    m_editActions << a;

    a = ac->addAction(QLatin1String("move_top_of_view"));
    a->setText(i18n("Move to Top of View"));
    a->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_PageUp));
    connect(a, SIGNAL(triggered(bool)), SLOT(topOfView()));
    m_editActions << a;

    a = ac->addAction(QLatin1String("select_top_of_view"));
    a->setText(i18n("Select to Top of View"));
    a->setShortcut(QKeySequence(Qt::CTRL + Qt::SHIFT +  Qt::Key_PageUp));
    connect(a, SIGNAL(triggered(bool)), SLOT(shiftTopOfView()));
    m_editActions << a;

    a = ac->addAction(QLatin1String("scroll_page_down"));
    a->setText(i18n("Scroll Page Down"));
    a->setShortcuts(KStandardShortcut::next());
    connect(a, SIGNAL(triggered(bool)), SLOT(pageDown()));
    m_editActions << a;

    a = ac->addAction(QLatin1String("select_page_down"));
    a->setText(i18n("Select Page Down"));
    a->setShortcut(QKeySequence(Qt::SHIFT + Qt::Key_PageDown));
    connect(a, SIGNAL(triggered(bool)), SLOT(shiftPageDown()));
    m_editActions << a;

    a = ac->addAction(QLatin1String("move_bottom_of_view"));
    a->setText(i18n("Move to Bottom of View"));
    a->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_PageDown));
    connect(a, SIGNAL(triggered(bool)), SLOT(bottomOfView()));
    m_editActions << a;

    a = ac->addAction(QLatin1String("select_bottom_of_view"));
    a->setText(i18n("Select to Bottom of View"));
    a->setShortcut(QKeySequence(Qt::CTRL + Qt::SHIFT + Qt::Key_PageDown));
    connect(a, SIGNAL(triggered(bool)), SLOT(shiftBottomOfView()));
    m_editActions << a;

    a = ac->addAction(QLatin1String("to_matching_bracket"));
    a->setText(i18n("Move to Matching Bracket"));
    a->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_6));
    connect(a, SIGNAL(triggered(bool)), SLOT(toMatchingBracket()));
    //m_editActions << a;

    a = ac->addAction(QLatin1String("select_matching_bracket"));
    a->setText(i18n("Select to Matching Bracket"));
    a->setShortcut(QKeySequence(Qt::SHIFT + Qt::CTRL + Qt::Key_6));
    connect(a, SIGNAL(triggered(bool)), SLOT(shiftToMatchingBracket()));
    //m_editActions << a;

    // anders: shortcuts doing any changes should not be created in read-only mode
    if (!m_doc->readOnly()) {
        a = ac->addAction(QLatin1String("transpose_char"));
        a->setText(i18n("Transpose Characters"));
        a->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_T));
        connect(a, SIGNAL(triggered(bool)), SLOT(transpose()));
        m_editActions << a;

        a = ac->addAction(QLatin1String("delete_line"));
        a->setText(i18n("Delete Line"));
        a->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_K));
        connect(a, SIGNAL(triggered(bool)), SLOT(killLine()));
        m_editActions << a;

        a = ac->addAction(QLatin1String("delete_word_left"));
        a->setText(i18n("Delete Word Left"));
        a->setShortcuts(KStandardShortcut::deleteWordBack());
        connect(a, SIGNAL(triggered(bool)), SLOT(deleteWordLeft()));
        m_editActions << a;

        a = ac->addAction(QLatin1String("delete_word_right"));
        a->setText(i18n("Delete Word Right"));
        a->setShortcuts(KStandardShortcut::deleteWordForward());
        connect(a, SIGNAL(triggered(bool)), SLOT(deleteWordRight()));
        m_editActions << a;

        a = ac->addAction(QLatin1String("delete_next_character"));
        a->setText(i18n("Delete Next Character"));
        a->setShortcut(QKeySequence(Qt::Key_Delete));
        connect(a, SIGNAL(triggered(bool)), SLOT(keyDelete()));
        m_editActions << a;

        a = ac->addAction(QLatin1String("backspace"));
        a->setText(i18n("Backspace"));
        QList<QKeySequence> scuts;
        scuts << QKeySequence(Qt::Key_Backspace)
              << QKeySequence(Qt::SHIFT + Qt::Key_Backspace);
        a->setShortcuts(scuts);
        connect(a, SIGNAL(triggered(bool)), SLOT(backspace()));
        m_editActions << a;

        a = ac->addAction(QLatin1String("insert_tabulator"));
        a->setText(i18n("Insert Tab"));
        connect(a, SIGNAL(triggered(bool)), SLOT(insertTab()));
        m_editActions << a;

        a = ac->addAction(QLatin1String("smart_newline"));
        a->setText(i18n("Insert Smart Newline"));
        a->setWhatsThis(i18n("Insert newline including leading characters of the current line which are not letters or numbers."));
        scuts.clear();
        scuts << QKeySequence(Qt::SHIFT + Qt::Key_Return)
              << QKeySequence(Qt::SHIFT + Qt::Key_Enter);
        a->setShortcuts(scuts);
        connect(a, SIGNAL(triggered(bool)), SLOT(smartNewline()));
        m_editActions << a;

        a = ac->addAction(QLatin1String("tools_indent"));
        a->setIcon(QIcon::fromTheme(QLatin1String("format-indent-more")));
        a->setText(i18n("&Indent"));
        a->setWhatsThis(i18n("Use this to indent a selected block of text.<br /><br />"
                             "You can configure whether tabs should be honored and used or replaced with spaces, in the configuration dialog."));
        a->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_I));
        connect(a, SIGNAL(triggered(bool)), SLOT(indent()));

        a = ac->addAction(QLatin1String("tools_unindent"));
        a->setIcon(QIcon::fromTheme(QLatin1String("format-indent-less")));
        a->setText(i18n("&Unindent"));
        a->setWhatsThis(i18n("Use this to unindent a selected block of text."));
        a->setShortcut(QKeySequence(Qt::CTRL + Qt::SHIFT + Qt::Key_I));
        connect(a, SIGNAL(triggered(bool)), SLOT(unIndent()));
    }

    if (hasFocus()) {
        slotGotFocus();
    } else {
        slotLostFocus();
    }
}

void KTextEditor::ViewPrivate::setupCodeFolding()
{
    //FIXME: FOLDING
    KActionCollection *ac = this->actionCollection();

    QAction *a;

    a = ac->addAction(QLatin1String("folding_toplevel"));
    a->setText(i18n("Fold Toplevel Nodes"));
    a->setShortcut(QKeySequence(Qt::CTRL + Qt::SHIFT + Qt::Key_Minus));
    connect(a, SIGNAL(triggered(bool)), SLOT(slotFoldToplevelNodes()));

    /*a = ac->addAction(QLatin1String("folding_expandtoplevel"));
    a->setText(i18n("Unfold Toplevel Nodes"));
    a->setShortcut(QKeySequence(Qt::CTRL+Qt::SHIFT+Qt::Key_Plus));
    connect(a, SIGNAL(triggered(bool)), m_doc->foldingTree(), SLOT(expandToplevelNodes()));

    a = ac->addAction(QLatin1String("folding_expandall"));
    a->setText(i18n("Unfold All Nodes"));
    connect(a, SIGNAL(triggered(bool)), m_doc->foldingTree(), SLOT(expandAll()));

    a = ac->addAction(QLatin1String("folding_collapse_dsComment"));
    a->setText(i18n("Fold Multiline Comments"));
    connect(a, SIGNAL(triggered(bool)), m_doc->foldingTree(), SLOT(collapseAll_dsComments()));
    */
    a = ac->addAction(QLatin1String("folding_collapselocal"));
    a->setText(i18n("Fold Current Node"));
    connect(a, SIGNAL(triggered(bool)), SLOT(slotCollapseLocal()));

    a = ac->addAction(QLatin1String("folding_expandlocal"));
    a->setText(i18n("Unfold Current Node"));
    connect(a, SIGNAL(triggered(bool)), SLOT(slotExpandLocal()));
}

void KTextEditor::ViewPrivate::slotFoldToplevelNodes()
{
    // FIXME: more performant implementation
    for (int line = 0; line < doc()->lines(); ++line) {
        if (textFolding().isLineVisible(line)) {
            foldLine(line);
        }
    }
}

void KTextEditor::ViewPrivate::slotCollapseLocal()
{
    foldLine(cursorPosition().line());
}

void KTextEditor::ViewPrivate::slotExpandLocal()
{
    unfoldLine(cursorPosition().line());
}

void KTextEditor::ViewPrivate::slotCollapseLevel()
{
    //FIXME: FOLDING
#if 0
    if (!sender()) {
        return;
    }
    QAction *action = qobject_cast<QAction *>(sender());
    if (!action) {
        return;
    }

    const int level = action->data().toInt();
    Q_ASSERT(level > 0);
    m_doc->foldingTree()->collapseLevel(level);
#endif
}

void KTextEditor::ViewPrivate::slotExpandLevel()
{
    //FIXME: FOLDING
#if 0
    if (!sender()) {
        return;
    }
    QAction *action = qobject_cast<QAction *>(sender());
    if (!action) {
        return;
    }

    const int level = action->data().toInt();
    Q_ASSERT(level > 0);
    m_doc->foldingTree()->expandLevel(level);
#endif
}

void KTextEditor::ViewPrivate::foldLine(int startLine)
{
    // only for valid lines
    if (startLine < 0) {
        return;
    }

    // try to fold all known ranges
    QVector<QPair<qint64, Kate::TextFolding::FoldingRangeFlags> > startingRanges = textFolding().foldingRangesStartingOnLine(startLine);
    for (int i = 0; i < startingRanges.size(); ++i) {
        textFolding().foldRange(startingRanges[i].first);
    }

    // try if the highlighting can help us and create a fold
    textFolding().newFoldingRange(doc()->buffer().computeFoldingRangeForStartLine(startLine), Kate::TextFolding::Folded);
}

void KTextEditor::ViewPrivate::unfoldLine(int startLine)
{
    // only for valid lines
    if (startLine < 0) {
        return;
    }

    // try to unfold all known ranges
    QVector<QPair<qint64, Kate::TextFolding::FoldingRangeFlags> > startingRanges = textFolding().foldingRangesStartingOnLine(startLine);
    for (int i = 0; i < startingRanges.size(); ++i) {
        textFolding().unfoldRange(startingRanges[i].first);
    }
}

KTextEditor::View::ViewMode KTextEditor::ViewPrivate::viewMode() const
{
    return currentInputMode()->viewMode();
}

QString KTextEditor::ViewPrivate::viewModeHuman() const
{
    QString currentMode = currentInputMode()->viewModeHuman();

    /**
     * append read-only if needed
     */
    if (!m_doc->isReadWrite()) {
        currentMode = i18n("(R/O) %1", currentMode);
    }

    /**
     * return full mode
     */
    return currentMode;
}

KTextEditor::View::InputMode KTextEditor::ViewPrivate::viewInputMode() const
{
    return currentInputMode()->viewInputMode();
}

QString KTextEditor::ViewPrivate::viewInputModeHuman() const
{
    return currentInputMode()->viewInputModeHuman();
}

void KTextEditor::ViewPrivate::setInputMode(KTextEditor::View::InputMode mode)
{
    if (currentInputMode()->viewInputMode() == mode) {
        return;
    }

    if (!m_viewInternal->m_inputModes.contains(mode)) {
        return;
    }

    m_viewInternal->m_currentInputMode->deactivate();
    m_viewInternal->m_currentInputMode = m_viewInternal->m_inputModes[mode];
    m_viewInternal->m_currentInputMode->activate();

    config()->setInputMode(mode); // TODO: this could be called from read config procedure, so it's not a good idea to set a specific view mode here

    /* small duplication, but need to do this; TODO: make it more sane */
    Q_FOREACH(QAction *action, m_inputModeActions) {
        bool checked = static_cast<InputMode>(action->data().toInt()) == mode;
        action->setChecked(checked);
    }

    /* inform the rest of the system about the change */
    emit viewInputModeChanged(this, mode);
    emit viewModeChanged(this, viewMode());
}

void KTextEditor::ViewPrivate::slotGotFocus()
{
    //qCDebug(LOG_PART) << "KTextEditor::ViewPrivate::slotGotFocus";
    currentInputMode()->gotFocus();
    emit focusIn(this);
}

void KTextEditor::ViewPrivate::slotLostFocus()
{
    //qCDebug(LOG_PART) << "KTextEditor::ViewPrivate::slotLostFocus";
    currentInputMode()->lostFocus();
    emit focusOut(this);
}

void KTextEditor::ViewPrivate::setDynWrapIndicators(int mode)
{
    config()->setDynWordWrapIndicators(mode);
}

bool KTextEditor::ViewPrivate::isOverwriteMode() const
{
    return m_doc->config()->ovr();
}

void KTextEditor::ViewPrivate::reloadFile()
{
    // bookmarks and cursor positions are temporarily saved by the document
    m_doc->documentReload();
}

void KTextEditor::ViewPrivate::slotReadWriteChanged()
{
    if (m_toggleWriteLock) {
        m_toggleWriteLock->setChecked(! m_doc->isReadWrite());
    }

    m_cut->setEnabled(m_doc->isReadWrite() && (selection() || m_config->smartCopyCut()));
    m_paste->setEnabled(m_doc->isReadWrite());
    m_pasteMenu->setEnabled(m_doc->isReadWrite() && !KTextEditor::EditorPrivate::self()->clipboardHistory().isEmpty());
    m_setEndOfLine->setEnabled(m_doc->isReadWrite());

    QStringList l;

    l << QLatin1String("edit_replace")
      << QLatin1String("tools_spelling")
      << QLatin1String("tools_indent")
      << QLatin1String("tools_unindent")
      << QLatin1String("tools_cleanIndent")
      << QLatin1String("tools_align")
      << QLatin1String("tools_comment")
      << QLatin1String("tools_uncomment")
      << QLatin1String("tools_toggle_comment")
      << QLatin1String("tools_uppercase")
      << QLatin1String("tools_lowercase")
      << QLatin1String("tools_capitalize")
      << QLatin1String("tools_join_lines")
      << QLatin1String("tools_apply_wordwrap")
      << QLatin1String("tools_spelling_from_cursor")
      << QLatin1String("tools_spelling_selection");

    QAction *a = 0;
    for (int z = 0; z < l.size(); z++)
        if ((a = actionCollection()->action(l[z]))) {
            a->setEnabled(m_doc->isReadWrite());
        }
    slotUpdateUndo();

    currentInputMode()->readWriteChanged(m_doc->isReadWrite());

    // => view mode changed
    emit viewModeChanged(this, viewMode());
    emit viewInputModeChanged(this, viewInputMode());
}

void KTextEditor::ViewPrivate::slotClipboardHistoryChanged()
{
    m_pasteMenu->setEnabled(m_doc->isReadWrite() && !KTextEditor::EditorPrivate::self()->clipboardHistory().isEmpty());
}

void KTextEditor::ViewPrivate::slotUpdateUndo()
{
    if (m_doc->readOnly()) {
        return;
    }

    m_editUndo->setEnabled(m_doc->isReadWrite() && m_doc->undoCount() > 0);
    m_editRedo->setEnabled(m_doc->isReadWrite() && m_doc->redoCount() > 0);
}

bool KTextEditor::ViewPrivate::setCursorPositionInternal(const KTextEditor::Cursor &position, uint tabwidth, bool calledExternally)
{
    Kate::TextLine l = m_doc->kateTextLine(position.line());

    if (!l) {
        return false;
    }

    QString line_str = m_doc->line(position.line());

    int x = 0;
    int z = 0;
    for (; z < line_str.length() && z < position.column(); z++) {
        if (line_str[z] == QLatin1Char('\t')) {
            x += tabwidth - (x % tabwidth);
        } else {
            x++;
        }
    }

    if (blockSelection())
        if (z < position.column()) {
            x += position.column() - z;
        }

    m_viewInternal->updateCursor(KTextEditor::Cursor(position.line(), x), false, true, calledExternally);

    return true;
}

void KTextEditor::ViewPrivate::toggleInsert()
{
    m_doc->config()->setOvr(!m_doc->config()->ovr());
    m_toggleInsert->setChecked(isOverwriteMode());

    emit viewModeChanged(this, viewMode());
    emit viewInputModeChanged(this, viewInputMode());
}

void KTextEditor::ViewPrivate::slotSaveCanceled(const QString &error)
{
    if (!error.isEmpty()) { // happens when canceling a job
        KMessageBox::error(this, error);
    }
}

void KTextEditor::ViewPrivate::gotoLine()
{
    gotoBar()->updateData();
    bottomViewBar()->showBarWidget(gotoBar());
}

void KTextEditor::ViewPrivate::changeDictionary()
{
    dictionaryBar()->updateData();
    bottomViewBar()->showBarWidget(dictionaryBar());
}

void KTextEditor::ViewPrivate::joinLines()
{
    int first = selectionRange().start().line();
    int last = selectionRange().end().line();
    //int left = m_doc->line( last ).length() - m_doc->selEndCol();
    if (first == last) {
        first = cursorPosition().line();
        last = first + 1;
    }
    m_doc->joinLines(first, last);
}

void KTextEditor::ViewPrivate::readSessionConfig(const KConfigGroup &config, const QSet<QString> &flags)
{
    Q_UNUSED(flags)

    // cursor position
    setCursorPositionInternal(KTextEditor::Cursor(config.readEntry("CursorLine", 0), config.readEntry("CursorColumn", 0)));

    // TODO: text folding state
//   m_savedFoldingState = config.readEntry("TextFolding", QVariantList());
//   applyFoldingState ();

    Q_FOREACH(KateAbstractInputMode *mode, m_viewInternal->m_inputModes) {
        mode->readSessionConfig(config);
    }
}

void KTextEditor::ViewPrivate::writeSessionConfig(KConfigGroup &config, const QSet<QString> &flags)
{
    Q_UNUSED(flags)

    // cursor position
    config.writeEntry("CursorLine", m_viewInternal->m_cursor.line());
    config.writeEntry("CursorColumn", m_viewInternal->m_cursor.column());

    // TODO: text folding state
//   saveFoldingState();
//   config.writeEntry("TextFolding", m_savedFoldingState);
//   m_savedFoldingState.clear ();

    Q_FOREACH(KateAbstractInputMode *mode, m_viewInternal->m_inputModes) {
        mode->writeSessionConfig(config);
    }
}

int KTextEditor::ViewPrivate::getEol() const
{
    return m_doc->config()->eol();
}

void KTextEditor::ViewPrivate::setEol(int eol)
{
    if (!doc()->isReadWrite()) {
        return;
    }

    if (m_updatingDocumentConfig) {
        return;
    }

    if (eol != m_doc->config()->eol()) {
        m_doc->setModified(true); // mark modified (bug #143120)
        m_doc->config()->setEol(eol);
    }
}

void KTextEditor::ViewPrivate::setAddBom(bool enabled)
{
    if (!doc()->isReadWrite()) {
        return;
    }

    if (m_updatingDocumentConfig) {
        return;
    }

    m_doc->config()->setBom(enabled);
    m_doc->bomSetByUser();
}

void KTextEditor::ViewPrivate::setIconBorder(bool enable)
{
    config()->setIconBar(enable);
}

void KTextEditor::ViewPrivate::toggleIconBorder()
{
    config()->setIconBar(!config()->iconBar());
}

void KTextEditor::ViewPrivate::setLineNumbersOn(bool enable)
{
    config()->setLineNumbers(enable);
}

void KTextEditor::ViewPrivate::toggleLineNumbersOn()
{
    config()->setLineNumbers(!config()->lineNumbers());
}

void KTextEditor::ViewPrivate::setScrollBarMarks(bool enable)
{
    config()->setScrollBarMarks(enable);
}

void KTextEditor::ViewPrivate::toggleScrollBarMarks()
{
    config()->setScrollBarMarks(!config()->scrollBarMarks());
}

void KTextEditor::ViewPrivate::setScrollBarMiniMap(bool enable)
{
    config()->setScrollBarMiniMap(enable);
}

void KTextEditor::ViewPrivate::toggleScrollBarMiniMap()
{
    config()->setScrollBarMiniMap(!config()->scrollBarMiniMap());
}

void KTextEditor::ViewPrivate::setScrollBarMiniMapAll(bool enable)
{
    config()->setScrollBarMiniMapAll(enable);
}

void KTextEditor::ViewPrivate::toggleScrollBarMiniMapAll()
{
    config()->setScrollBarMiniMapAll(!config()->scrollBarMiniMapAll());
}

void KTextEditor::ViewPrivate::setScrollBarMiniMapWidth(int width)
{
    config()->setScrollBarMiniMapWidth(width);
}

void KTextEditor::ViewPrivate::toggleDynWordWrap()
{
    config()->setDynWordWrap(!config()->dynWordWrap());
}

void KTextEditor::ViewPrivate::toggleWWMarker()
{
    m_renderer->config()->setWordWrapMarker(!m_renderer->config()->wordWrapMarker());
}

void KTextEditor::ViewPrivate::toggleNPSpaces()
{
    m_renderer->setShowNonPrintableSpaces(!m_renderer->showNonPrintableSpaces());
    m_viewInternal->update(); // force redraw
}

void KTextEditor::ViewPrivate::setFoldingMarkersOn(bool enable)
{
    config()->setFoldingBar(enable);
}

void KTextEditor::ViewPrivate::toggleFoldingMarkers()
{
    config()->setFoldingBar(!config()->foldingBar());
}

bool KTextEditor::ViewPrivate::iconBorder()
{
    return m_viewInternal->m_leftBorder->iconBorderOn();
}

bool KTextEditor::ViewPrivate::lineNumbersOn()
{
    return m_viewInternal->m_leftBorder->lineNumbersOn();
}

bool KTextEditor::ViewPrivate::scrollBarMarks()
{
    return m_viewInternal->m_lineScroll->showMarks();
}

bool KTextEditor::ViewPrivate::scrollBarMiniMap()
{
    return m_viewInternal->m_lineScroll->showMiniMap();
}

int KTextEditor::ViewPrivate::dynWrapIndicators()
{
    return m_viewInternal->m_leftBorder->dynWrapIndicators();
}

bool KTextEditor::ViewPrivate::foldingMarkersOn()
{
    return m_viewInternal->m_leftBorder->foldingMarkersOn();
}

void KTextEditor::ViewPrivate::toggleWriteLock()
{
    m_doc->setReadWrite(! m_doc->isReadWrite());
}

void KTextEditor::ViewPrivate::registerTextHintProvider(KTextEditor::TextHintProvider *provider)
{
    m_viewInternal->registerTextHintProvider(provider);
}

void KTextEditor::ViewPrivate::unregisterTextHintProvider(KTextEditor::TextHintProvider *provider)
{
    m_viewInternal->unregisterTextHintProvider(provider);
}

void KTextEditor::ViewPrivate::setTextHintDelay(int delay)
{
    m_viewInternal->setTextHintDelay(delay);
}

int KTextEditor::ViewPrivate::textHintDelay() const
{
    return m_viewInternal->textHintDelay();
}

void KTextEditor::ViewPrivate::find()
{
    currentInputMode()->find();
}

void KTextEditor::ViewPrivate::findSelectedForwards()
{
    currentInputMode()->findSelectedForwards();
}

void KTextEditor::ViewPrivate::findSelectedBackwards()
{
    currentInputMode()->findSelectedBackwards();
}

void KTextEditor::ViewPrivate::replace()
{
    currentInputMode()->findReplace();
}

void KTextEditor::ViewPrivate::findNext()
{
    currentInputMode()->findNext();
}

void KTextEditor::ViewPrivate::findPrevious()
{
    currentInputMode()->findPrevious();
}

void KTextEditor::ViewPrivate::slotSelectionChanged()
{
    m_copy->setEnabled(selection() || m_config->smartCopyCut());
    m_deSelect->setEnabled(selection());
    m_copyHtmlAction->setEnabled (selection());

    if (m_doc->readOnly()) {
        return;
    }

    m_cut->setEnabled(selection() || m_config->smartCopyCut());

    m_spell->updateActions();
    
    // update highlighting of current selected word
    selectionChangedForHighlights ();
}

void KTextEditor::ViewPrivate::switchToCmdLine()
{
    currentInputMode()->activateCommandLine();
}

KateRenderer *KTextEditor::ViewPrivate::renderer()
{
    return m_renderer;
}

void KTextEditor::ViewPrivate::updateConfig()
{
    if (m_startingUp) {
        return;
    }

    // dyn. word wrap & markers
    if (m_hasWrap != config()->dynWordWrap()) {
        m_viewInternal->prepareForDynWrapChange();

        m_hasWrap = config()->dynWordWrap();

        m_viewInternal->dynWrapChanged();

        m_setDynWrapIndicators->setEnabled(config()->dynWordWrap());
        m_toggleDynWrap->setChecked(config()->dynWordWrap());
    }

    m_viewInternal->m_leftBorder->setDynWrapIndicators(config()->dynWordWrapIndicators());
    m_setDynWrapIndicators->setCurrentItem(config()->dynWordWrapIndicators());

    // line numbers
    m_viewInternal->m_leftBorder->setLineNumbersOn(config()->lineNumbers());
    m_toggleLineNumbers->setChecked(config()->lineNumbers());

    // icon bar
    m_viewInternal->m_leftBorder->setIconBorderOn(config()->iconBar());
    m_toggleIconBar->setChecked(config()->iconBar());

    // scrollbar marks
    m_viewInternal->m_lineScroll->setShowMarks(config()->scrollBarMarks());
    m_toggleScrollBarMarks->setChecked(config()->scrollBarMarks());

    // scrollbar mini-map
    m_viewInternal->m_lineScroll->setShowMiniMap(config()->scrollBarMiniMap());
    m_toggleScrollBarMiniMap->setChecked(config()->scrollBarMiniMap());

    // scrollbar mini-map - (whole document)
    m_viewInternal->m_lineScroll->setMiniMapAll(config()->scrollBarMiniMapAll());
    //m_toggleScrollBarMiniMapAll->setChecked( config()->scrollBarMiniMapAll() );

    // scrollbar mini-map.width
    m_viewInternal->m_lineScroll->setMiniMapWidth(config()->scrollBarMiniMapWidth());

    // misc edit
    m_toggleBlockSelection->setChecked(blockSelection());
    m_toggleInsert->setChecked(isOverwriteMode());

    updateFoldingConfig();

    // bookmark
    m_bookmarks->setSorting((KateBookmarks::Sorting) config()->bookmarkSort());

    m_viewInternal->setAutoCenterLines(config()->autoCenterLines());

    Q_FOREACH(KateAbstractInputMode *input, m_viewInternal->m_inputModes) {
        input->updateConfig();
    }

    setInputMode(config()->inputMode());

    reflectOnTheFlySpellCheckStatus(m_doc->isOnTheFlySpellCheckingEnabled());

    // register/unregister word completion...
    unregisterCompletionModel(KTextEditor::EditorPrivate::self()->wordCompletionModel());
    unregisterCompletionModel (KTextEditor::EditorPrivate::self()->keywordCompletionModel());
    if (config()->wordCompletion()) {
        registerCompletionModel(KTextEditor::EditorPrivate::self()->wordCompletionModel());
    }
    if (config()->keywordCompletion()) {
        registerCompletionModel(KTextEditor::EditorPrivate::self()->keywordCompletionModel());
    }

    m_cut->setEnabled(m_doc->isReadWrite() && (selection() || m_config->smartCopyCut()));
    m_copy->setEnabled(selection() || m_config->smartCopyCut());

    // now redraw...
    m_viewInternal->cache()->clear();
    tagAll();
    updateView(true);

    emit configChanged();
}

void KTextEditor::ViewPrivate::updateDocumentConfig()
{
    if (m_startingUp) {
        return;
    }

    m_updatingDocumentConfig = true;

    m_setEndOfLine->setCurrentItem(m_doc->config()->eol());

    m_addBom->setChecked(m_doc->config()->bom());

    m_updatingDocumentConfig = false;

    // maybe block selection or wrap-cursor mode changed
    ensureCursorColumnValid();

    // first change this
    m_renderer->setTabWidth(m_doc->config()->tabWidth());
    m_renderer->setIndentWidth(m_doc->config()->indentationWidth());

    // now redraw...
    m_viewInternal->cache()->clear();
    tagAll();
    updateView(true);
}

void KTextEditor::ViewPrivate::updateRendererConfig()
{
    if (m_startingUp) {
        return;
    }

    m_toggleWWMarker->setChecked(m_renderer->config()->wordWrapMarker());

    m_viewInternal->updateBracketMarkAttributes();
    m_viewInternal->updateBracketMarks();

    // now redraw...
    m_viewInternal->cache()->clear();
    tagAll();
    m_viewInternal->updateView(true);

    // update the left border right, for example linenumbers
    m_viewInternal->m_leftBorder->updateFont();
    m_viewInternal->m_leftBorder->repaint();

    m_viewInternal->m_lineScroll->queuePixmapUpdate();

    currentInputMode()->updateRendererConfig();

// @@ showIndentLines is not cached anymore.
//  m_renderer->setShowIndentLines (m_renderer->config()->showIndentationLines());
    emit configChanged();
}

void KTextEditor::ViewPrivate::updateFoldingConfig()
{
    // folding bar
    m_viewInternal->m_leftBorder->setFoldingMarkersOn(config()->foldingBar());
    m_toggleFoldingMarkers->setChecked(config()->foldingBar());

#if 0
    // FIXME: FOLDING
    QStringList l;

    l << "folding_toplevel" << "folding_expandtoplevel"
      << "folding_collapselocal" << "folding_expandlocal";

    QAction *a = 0;
    for (int z = 0; z < l.size(); z++)
        if ((a = actionCollection()->action(l[z].toAscii().constData()))) {
            a->setEnabled(m_doc->highlight() && m_doc->highlight()->allowsFolding());
        }
#endif
}

void KTextEditor::ViewPrivate::ensureCursorColumnValid()
{
    KTextEditor::Cursor c = m_viewInternal->getCursor();

    // make sure the cursor is valid:
    // - in block selection mode or if wrap cursor is off, the column is arbitrary
    // - otherwise: it's bounded by the line length
    if (!blockSelection() && wrapCursor()
            && (!c.isValid() || c.column() > m_doc->lineLength(c.line()))) {
        c.setColumn(m_doc->kateTextLine(cursorPosition().line())->length());
        setCursorPosition(c);
    }
}

//BEGIN EDIT STUFF
void KTextEditor::ViewPrivate::editStart()
{
    m_viewInternal->editStart();
}

void KTextEditor::ViewPrivate::editEnd(int editTagLineStart, int editTagLineEnd, bool tagFrom)
{
    m_viewInternal->editEnd(editTagLineStart, editTagLineEnd, tagFrom);
}

void KTextEditor::ViewPrivate::editSetCursor(const KTextEditor::Cursor &cursor)
{
    m_viewInternal->editSetCursor(cursor);
}
//END

//BEGIN TAG & CLEAR
bool KTextEditor::ViewPrivate::tagLine(const KTextEditor::Cursor &virtualCursor)
{
    return m_viewInternal->tagLine(virtualCursor);
}

bool KTextEditor::ViewPrivate::tagRange(const KTextEditor::Range &range, bool realLines)
{
    return m_viewInternal->tagRange(range, realLines);
}

bool KTextEditor::ViewPrivate::tagLines(int start, int end, bool realLines)
{
    return m_viewInternal->tagLines(start, end, realLines);
}

bool KTextEditor::ViewPrivate::tagLines(KTextEditor::Cursor start, KTextEditor::Cursor end, bool realCursors)
{
    return m_viewInternal->tagLines(start, end, realCursors);
}

void KTextEditor::ViewPrivate::tagAll()
{
    m_viewInternal->tagAll();
}

void KTextEditor::ViewPrivate::clear()
{
    m_viewInternal->clear();
}

void KTextEditor::ViewPrivate::repaintText(bool paintOnlyDirty)
{
    if (paintOnlyDirty) {
        m_viewInternal->updateDirty();
    } else {
        m_viewInternal->update();
    }
}

void KTextEditor::ViewPrivate::updateView(bool changed)
{
    //qCDebug(LOG_PART) << "KTextEditor::ViewPrivate::updateView";

    m_viewInternal->updateView(changed);
    m_viewInternal->m_leftBorder->update();
}

//END

void KTextEditor::ViewPrivate::slotHlChanged()
{
    KateHighlighting *hl = m_doc->highlight();
    bool ok(!hl->getCommentStart(0).isEmpty() || !hl->getCommentSingleLineStart(0).isEmpty());

    if (actionCollection()->action(QLatin1String("tools_comment"))) {
        actionCollection()->action(QLatin1String("tools_comment"))->setEnabled(ok);
    }

    if (actionCollection()->action(QLatin1String("tools_uncomment"))) {
        actionCollection()->action(QLatin1String("tools_uncomment"))->setEnabled(ok);
    }

    if (actionCollection()->action(QLatin1String("tools_toggle_comment"))) {
        actionCollection()->action(QLatin1String("tools_toggle_comment"))->setEnabled(ok);
    }

    // show folding bar if "view defaults" says so, otherwise enable/disable only the menu entry
    updateFoldingConfig();
}

int KTextEditor::ViewPrivate::virtualCursorColumn() const
{
    return m_doc->toVirtualColumn(m_viewInternal->getCursor());
}

void KTextEditor::ViewPrivate::notifyMousePositionChanged(const KTextEditor::Cursor &newPosition)
{
    emit mousePositionChanged(this, newPosition);
}

//BEGIN KTextEditor::SelectionInterface stuff

bool KTextEditor::ViewPrivate::setSelection(const KTextEditor::Range &selection)
{
    /**
     * anything to do?
     */
    if (selection == m_selection) {
        return true;
    }

    /**
     * backup old range
     */
    KTextEditor::Range oldSelection = m_selection;

    /**
     * set new range
     */
    m_selection.setRange(selection.isEmpty() ? KTextEditor::Range::invalid() : selection);

    /**
     * trigger update of correct area
     */
    tagSelection(oldSelection);
    repaintText(true);

    /**
     * emit holy signal
     */
    emit selectionChanged(this);

    /**
     * be done
     */
    return true;
}

bool KTextEditor::ViewPrivate::clearSelection()
{
    return clearSelection(true);
}

bool KTextEditor::ViewPrivate::clearSelection(bool redraw, bool finishedChangingSelection)
{
    /**
     * no selection, nothing to do...
     */
    if (!selection()) {
        return false;
    }

    /**
     * backup old range
     */
    KTextEditor::Range oldSelection = m_selection;

    /**
     * invalidate current selection
     */
    m_selection.setRange(KTextEditor::Range::invalid());

    /**
     * trigger update of correct area
     */
    tagSelection(oldSelection);
    if (redraw) {
        repaintText(true);
    }

    /**
     * emit holy signal
     */
    if (finishedChangingSelection) {
        emit selectionChanged(this);
    }

    /**
     * be done
     */
    return true;
}

bool KTextEditor::ViewPrivate::selection() const
{
    if (!wrapCursor()) {
        return m_selection != KTextEditor::Range::invalid();
    } else {
        return m_selection.toRange().isValid();
    }
}

QString KTextEditor::ViewPrivate::selectionText() const
{
    return m_doc->text(m_selection, blockSelect);
}

bool KTextEditor::ViewPrivate::removeSelectedText()
{
    if (!selection()) {
        return false;
    }

    m_doc->editStart();

    // Optimization: clear selection before removing text
    KTextEditor::Range selection = m_selection;

    m_doc->removeText(selection, blockSelect);

    // don't redraw the cleared selection - that's done in editEnd().
    if (blockSelect) {
        int selectionColumn = qMin(m_doc->toVirtualColumn(selection.start()), m_doc->toVirtualColumn(selection.end()));
        KTextEditor::Range newSelection = selection;
        newSelection.setStart(KTextEditor::Cursor(newSelection.start().line(), m_doc->fromVirtualColumn(newSelection.start().line(), selectionColumn)));
        newSelection.setEnd(KTextEditor::Cursor(newSelection.end().line(), m_doc->fromVirtualColumn(newSelection.end().line(), selectionColumn)));
        setSelection(newSelection);
        setCursorPositionInternal(newSelection.start());
    } else {
        clearSelection(false);
    }

    m_doc->editEnd();

    return true;
}

bool KTextEditor::ViewPrivate::selectAll()
{
    setBlockSelection(false);
    top();
    shiftBottom();
    return true;
}

bool KTextEditor::ViewPrivate::cursorSelected(const KTextEditor::Cursor &cursor)
{
    KTextEditor::Cursor ret = cursor;
    if ((!blockSelect) && (ret.column() < 0)) {
        ret.setColumn(0);
    }

    if (blockSelect)
        return cursor.line() >= m_selection.start().line() && ret.line() <= m_selection.end().line()
               && ret.column() >= m_selection.start().column() && ret.column() <= m_selection.end().column();
    else {
        return m_selection.toRange().contains(cursor) || m_selection.end() == cursor;
    }
}

bool KTextEditor::ViewPrivate::lineSelected(int line)
{
    return !blockSelect && m_selection.toRange().containsLine(line);
}

bool KTextEditor::ViewPrivate::lineEndSelected(const KTextEditor::Cursor &lineEndPos)
{
    return (!blockSelect)
           && (lineEndPos.line() > m_selection.start().line() || (lineEndPos.line() == m_selection.start().line() && (m_selection.start().column() < lineEndPos.column() || lineEndPos.column() == -1)))
           && (lineEndPos.line() < m_selection.end().line() || (lineEndPos.line() == m_selection.end().line() && (lineEndPos.column() <= m_selection.end().column() && lineEndPos.column() != -1)));
}

bool KTextEditor::ViewPrivate::lineHasSelected(int line)
{
    return selection() && m_selection.toRange().containsLine(line);
}

bool KTextEditor::ViewPrivate::lineIsSelection(int line)
{
    return (line == m_selection.start().line() && line == m_selection.end().line());
}

void KTextEditor::ViewPrivate::tagSelection(const KTextEditor::Range &oldSelection)
{
    if (selection()) {
        if (oldSelection.start().line() == -1) {
            // We have to tag the whole lot if
            // 1) we have a selection, and:
            //  a) it's new; or
            tagLines(m_selection, true);

        } else if (blockSelection() && (oldSelection.start().column() != m_selection.start().column() || oldSelection.end().column() != m_selection.end().column())) {
            //  b) we're in block selection mode and the columns have changed
            tagLines(m_selection, true);
            tagLines(oldSelection, true);

        } else {
            if (oldSelection.start() != m_selection.start()) {
                if (oldSelection.start() < m_selection.start()) {
                    tagLines(oldSelection.start(), m_selection.start(), true);
                } else {
                    tagLines(m_selection.start(), oldSelection.start(), true);
                }
            }

            if (oldSelection.end() != m_selection.end()) {
                if (oldSelection.end() < m_selection.end()) {
                    tagLines(oldSelection.end(), m_selection.end(), true);
                } else {
                    tagLines(m_selection.end(), oldSelection.end(), true);
                }
            }
        }

    } else {
        // No more selection, clean up
        tagLines(oldSelection, true);
    }
}

void KTextEditor::ViewPrivate::selectWord(const KTextEditor::Cursor &cursor)
{
    setSelection(m_doc->wordRangeAt(cursor));
}

void KTextEditor::ViewPrivate::selectLine(const KTextEditor::Cursor &cursor)
{
    int line = cursor.line();
    if (line + 1 >= m_doc->lines()) {
        setSelection(KTextEditor::Range(line, 0, line, m_doc->lineLength(line)));
    } else {
        setSelection(KTextEditor::Range(line, 0, line + 1, 0));
    }
}

void KTextEditor::ViewPrivate::cut()
{
    if (!selection() && !m_config->smartCopyCut()) {
        return;
    }

    copy();
    if (!selection()) {
        selectLine(m_viewInternal->m_cursor);
    }
    removeSelectedText();
}

void KTextEditor::ViewPrivate::copy() const
{
    QString text = selectionText();

    if (!selection()) {
        if (!m_config->smartCopyCut()) {
            return;
        }
        text = m_doc->line(m_viewInternal->m_cursor.line()) + QLatin1Char('\n');
        m_viewInternal->moveEdge(KateViewInternal::left, false);
    }

    // copy to clipboard and our history!
    KTextEditor::EditorPrivate::self()->copyToClipboard(text);
}

void KTextEditor::ViewPrivate::applyWordWrap()
{
    if (selection()) {
        m_doc->wrapText(selectionRange().start().line(), selectionRange().end().line());
    } else {
        m_doc->wrapText(0, m_doc->lastLine());
    }
}

//END

//BEGIN KTextEditor::BlockSelectionInterface stuff

bool KTextEditor::ViewPrivate::blockSelection() const
{
    return blockSelect;
}

bool KTextEditor::ViewPrivate::setBlockSelection(bool on)
{
    if (on != blockSelect) {
        blockSelect = on;

        KTextEditor::Range oldSelection = m_selection;

        const bool hadSelection = clearSelection(false, false);

        setSelection(oldSelection);

        m_toggleBlockSelection->setChecked(blockSelection());

        // when leaving block selection mode, if cursor is at an invalid position or past the end of the
        // line, move the cursor to the last column of the current line unless cursor wrapping is off
        ensureCursorColumnValid();

        if (!hadSelection) {
            // emit selectionChanged() according to the KTextEditor::View api
            // documentation also if there is no selection around. This is needed,
            // as e.g. the Kate App status bar uses this signal to update the state
            // of the selection mode (block selection, line based selection)
            emit selectionChanged(this);
        }
    }

    return true;
}

bool KTextEditor::ViewPrivate::toggleBlockSelection()
{
    m_toggleBlockSelection->setChecked(!blockSelect);
    return setBlockSelection(!blockSelect);
}

bool KTextEditor::ViewPrivate::wrapCursor() const
{
    return !blockSelection();
}

//END

void KTextEditor::ViewPrivate::slotTextInserted(KTextEditor::View *view, const KTextEditor::Cursor &position, const QString &text)
{
    emit textInserted(view, position, text);
}

bool KTextEditor::ViewPrivate::insertTemplateTextImplementation(const KTextEditor::Cursor &c,
        const QString &templateString,
        const QMap<QString, QString> &initialValues)
{
    return insertTemplateTextImplementation(c, templateString, initialValues, 0);
}

bool KTextEditor::ViewPrivate::insertTemplateTextImplementation(const KTextEditor::Cursor &c,
        const QString &templateString,
        const QMap<QString, QString> &initialValues,
        KTextEditor::TemplateScript *templateScript)
{
    /**
     * no empty templates
     */
    if (templateString.isEmpty()) {
        return false;
    }

    /**
     * not for read-only docs
     */
    if (!m_doc->isReadWrite()) {
        return false;
    }

    /**
     * get script
     */
    KateTemplateScript *kateTemplateScript = KTextEditor::EditorPrivate::self()->scriptManager()->templateScript(templateScript);

    /**
     * the handler will delete itself when necessary
     */
    new KateTemplateHandler(this, c, templateString, initialValues, m_doc->undoManager(), kateTemplateScript);
    return true;
}

bool KTextEditor::ViewPrivate::tagLines(KTextEditor::Range range, bool realRange)
{
    return tagLines(range.start(), range.end(), realRange);
}

void KTextEditor::ViewPrivate::deactivateEditActions()
{
    foreach (QAction *action, m_editActions) {
        action->setEnabled(false);
    }
}

void KTextEditor::ViewPrivate::activateEditActions()
{
    foreach (QAction *action, m_editActions) {
        action->setEnabled(true);
    }
}

bool KTextEditor::ViewPrivate::mouseTrackingEnabled() const
{
    // FIXME support
    return true;
}

bool KTextEditor::ViewPrivate::setMouseTrackingEnabled(bool)
{
    // FIXME support
    return true;
}

bool KTextEditor::ViewPrivate::isCompletionActive() const
{
    return completionWidget()->isCompletionActive();
}

KateCompletionWidget *KTextEditor::ViewPrivate::completionWidget() const
{
    if (!m_completionWidget) {
        m_completionWidget = new KateCompletionWidget(const_cast<KTextEditor::ViewPrivate *>(this));
    }

    return m_completionWidget;
}

void KTextEditor::ViewPrivate::startCompletion(const KTextEditor::Range &word, KTextEditor::CodeCompletionModel *model)
{
    completionWidget()->startCompletion(word, model);
}

void KTextEditor::ViewPrivate::abortCompletion()
{
    completionWidget()->abortCompletion();
}

void KTextEditor::ViewPrivate::forceCompletion()
{
    completionWidget()->execute();
}

void KTextEditor::ViewPrivate::registerCompletionModel(KTextEditor::CodeCompletionModel *model)
{
    completionWidget()->registerCompletionModel(model);
}

void KTextEditor::ViewPrivate::unregisterCompletionModel(KTextEditor::CodeCompletionModel *model)
{
    completionWidget()->unregisterCompletionModel(model);
}

bool KTextEditor::ViewPrivate::isAutomaticInvocationEnabled() const
{
    return m_config->automaticCompletionInvocation();
}

void KTextEditor::ViewPrivate::setAutomaticInvocationEnabled(bool enabled)
{
    config()->setAutomaticCompletionInvocation(enabled);
}

void KTextEditor::ViewPrivate::sendCompletionExecuted(const KTextEditor::Cursor &position, KTextEditor::CodeCompletionModel *model, const QModelIndex &index)
{
    emit completionExecuted(this, position, model, index);
}

void KTextEditor::ViewPrivate::sendCompletionAborted()
{
    emit completionAborted(this);
}

void KTextEditor::ViewPrivate::paste(const QString *textToPaste)
{
    const bool completionEnabled = isAutomaticInvocationEnabled();
    if (completionEnabled) {
        setAutomaticInvocationEnabled(false);
    }

    m_doc->paste(this, textToPaste ? *textToPaste : QApplication::clipboard()->text(QClipboard::Clipboard));

    if (completionEnabled) {
        setAutomaticInvocationEnabled(true);
    }
}

bool KTextEditor::ViewPrivate::setCursorPosition(KTextEditor::Cursor position)
{
    return setCursorPositionInternal(position, 1, true);
}

KTextEditor::Cursor KTextEditor::ViewPrivate::cursorPosition() const
{
    return m_viewInternal->getCursor();
}

KTextEditor::Cursor KTextEditor::ViewPrivate::cursorPositionVirtual() const
{
    return KTextEditor::Cursor(m_viewInternal->getCursor().line(), virtualCursorColumn());
}

QPoint KTextEditor::ViewPrivate::cursorToCoordinate(const KTextEditor::Cursor &cursor) const
{
    return m_viewInternal->cursorToCoordinate(cursor);
}

KTextEditor::Cursor KTextEditor::ViewPrivate::coordinatesToCursor(const QPoint &coords) const
{
    return m_viewInternal->coordinatesToCursor(coords);
}

QPoint KTextEditor::ViewPrivate::cursorPositionCoordinates() const
{
    return m_viewInternal->cursorCoordinates();
}

bool KTextEditor::ViewPrivate::setCursorPositionVisual(const KTextEditor::Cursor &position)
{
    return setCursorPositionInternal(position, m_doc->config()->tabWidth(), true);
}

QString KTextEditor::ViewPrivate::currentTextLine()
{
    return m_doc->line(cursorPosition().line());
}

void KTextEditor::ViewPrivate::indent()
{
    KTextEditor::Cursor c(cursorPosition().line(), 0);
    KTextEditor::Range r = selection() ? selectionRange() : KTextEditor::Range(c, c);
    m_doc->indent(r, 1);
}

void KTextEditor::ViewPrivate::unIndent()
{
    KTextEditor::Cursor c(cursorPosition().line(), 0);
    KTextEditor::Range r = selection() ? selectionRange() : KTextEditor::Range(c, c);
    m_doc->indent(r, -1);
}

void KTextEditor::ViewPrivate::cleanIndent()
{
    KTextEditor::Cursor c(cursorPosition().line(), 0);
    KTextEditor::Range r = selection() ? selectionRange() : KTextEditor::Range(c, c);
    m_doc->indent(r, 0);
}

void KTextEditor::ViewPrivate::align()
{
    // no selection: align current line; selection: use selection range
    const int line = cursorPosition().line();
    KTextEditor::Range alignRange(KTextEditor::Cursor(line, 0), KTextEditor::Cursor(line, 0));
    if (selection()) {
        alignRange = selectionRange();
    }

    m_doc->align(this, alignRange);
}

void KTextEditor::ViewPrivate::comment()
{
    m_selection.setInsertBehaviors(Kate::TextRange::ExpandLeft | Kate::TextRange::ExpandRight);
    m_doc->comment(this, cursorPosition().line(), cursorPosition().column(), 1);
    m_selection.setInsertBehaviors(Kate::TextRange::ExpandRight);
}

void KTextEditor::ViewPrivate::uncomment()
{
    m_doc->comment(this, cursorPosition().line(), cursorPosition().column(), -1);
}

void KTextEditor::ViewPrivate::toggleComment()
{
    m_selection.setInsertBehaviors(Kate::TextRange::ExpandLeft | Kate::TextRange::ExpandRight);
    m_doc->comment(this, cursorPosition().line(), cursorPosition().column(), 0);
    m_selection.setInsertBehaviors(Kate::TextRange::ExpandRight);
}

void KTextEditor::ViewPrivate::uppercase()
{
    m_doc->transform(this, m_viewInternal->m_cursor, KTextEditor::DocumentPrivate::Uppercase);
}

void KTextEditor::ViewPrivate::killLine()
{
    if (m_selection.isEmpty()) {
        m_doc->removeLine(cursorPosition().line());
    } else {
        m_doc->editStart();
        for (int line = m_selection.end().line(); line >= m_selection.start().line(); line--) {
            m_doc->removeLine(line);
        }
        m_doc->editEnd();
    }
}

void KTextEditor::ViewPrivate::lowercase()
{
    m_doc->transform(this, m_viewInternal->m_cursor, KTextEditor::DocumentPrivate::Lowercase);
}

void KTextEditor::ViewPrivate::capitalize()
{
    m_doc->editStart();
    m_doc->transform(this, m_viewInternal->m_cursor, KTextEditor::DocumentPrivate::Lowercase);
    m_doc->transform(this, m_viewInternal->m_cursor, KTextEditor::DocumentPrivate::Capitalize);
    m_doc->editEnd();
}

void KTextEditor::ViewPrivate::keyReturn()
{
    m_viewInternal->doReturn();
}

void KTextEditor::ViewPrivate::smartNewline()
{
    m_viewInternal->doSmartNewline();
}

void KTextEditor::ViewPrivate::backspace()
{
    m_viewInternal->doBackspace();
}

void KTextEditor::ViewPrivate::insertTab()
{
    m_viewInternal->doTabulator();
}

void KTextEditor::ViewPrivate::deleteWordLeft()
{
    m_viewInternal->doDeletePrevWord();
}

void KTextEditor::ViewPrivate::keyDelete()
{
    m_viewInternal->doDelete();
}

void KTextEditor::ViewPrivate::deleteWordRight()
{
    m_viewInternal->doDeleteNextWord();
}

void KTextEditor::ViewPrivate::transpose()
{
    m_viewInternal->doTranspose();
}

void KTextEditor::ViewPrivate::cursorLeft()
{
    if (m_viewInternal->m_view->currentTextLine().isRightToLeft()) {
        m_viewInternal->cursorNextChar();
    } else {
        m_viewInternal->cursorPrevChar();
    }
}

void KTextEditor::ViewPrivate::shiftCursorLeft()
{
    if (m_viewInternal->m_view->currentTextLine().isRightToLeft()) {
        m_viewInternal->cursorNextChar(true);
    } else {
        m_viewInternal->cursorPrevChar(true);
    }
}

void KTextEditor::ViewPrivate::cursorRight()
{
    if (m_viewInternal->m_view->currentTextLine().isRightToLeft()) {
        m_viewInternal->cursorPrevChar();
    } else {
        m_viewInternal->cursorNextChar();
    }
}

void KTextEditor::ViewPrivate::shiftCursorRight()
{
    if (m_viewInternal->m_view->currentTextLine().isRightToLeft()) {
        m_viewInternal->cursorPrevChar(true);
    } else {
        m_viewInternal->cursorNextChar(true);
    }
}

void KTextEditor::ViewPrivate::wordLeft()
{
    if (m_viewInternal->m_view->currentTextLine().isRightToLeft()) {
        m_viewInternal->wordNext();
    } else {
        m_viewInternal->wordPrev();
    }
}

void KTextEditor::ViewPrivate::shiftWordLeft()
{
    if (m_viewInternal->m_view->currentTextLine().isRightToLeft()) {
        m_viewInternal->wordNext(true);
    } else {
        m_viewInternal->wordPrev(true);
    }
}

void KTextEditor::ViewPrivate::wordRight()
{
    if (m_viewInternal->m_view->currentTextLine().isRightToLeft()) {
        m_viewInternal->wordPrev();
    } else {
        m_viewInternal->wordNext();
    }
}

void KTextEditor::ViewPrivate::shiftWordRight()
{
    if (m_viewInternal->m_view->currentTextLine().isRightToLeft()) {
        m_viewInternal->wordPrev(true);
    } else {
        m_viewInternal->wordNext(true);
    }
}

void KTextEditor::ViewPrivate::home()
{
    m_viewInternal->home();
}

void KTextEditor::ViewPrivate::shiftHome()
{
    m_viewInternal->home(true);
}

void KTextEditor::ViewPrivate::end()
{
    m_viewInternal->end();
}

void KTextEditor::ViewPrivate::shiftEnd()
{
    m_viewInternal->end(true);
}

void KTextEditor::ViewPrivate::up()
{
    m_viewInternal->cursorUp();
}

void KTextEditor::ViewPrivate::shiftUp()
{
    m_viewInternal->cursorUp(true);
}

void KTextEditor::ViewPrivate::down()
{
    m_viewInternal->cursorDown();
}

void KTextEditor::ViewPrivate::shiftDown()
{
    m_viewInternal->cursorDown(true);
}

void KTextEditor::ViewPrivate::scrollUp()
{
    m_viewInternal->scrollUp();
}

void KTextEditor::ViewPrivate::scrollDown()
{
    m_viewInternal->scrollDown();
}

void KTextEditor::ViewPrivate::topOfView()
{
    m_viewInternal->topOfView();
}

void KTextEditor::ViewPrivate::shiftTopOfView()
{
    m_viewInternal->topOfView(true);
}

void KTextEditor::ViewPrivate::bottomOfView()
{
    m_viewInternal->bottomOfView();
}

void KTextEditor::ViewPrivate::shiftBottomOfView()
{
    m_viewInternal->bottomOfView(true);
}

void KTextEditor::ViewPrivate::pageUp()
{
    m_viewInternal->pageUp();
}

void KTextEditor::ViewPrivate::shiftPageUp()
{
    m_viewInternal->pageUp(true);
}

void KTextEditor::ViewPrivate::pageDown()
{
    m_viewInternal->pageDown();
}

void KTextEditor::ViewPrivate::shiftPageDown()
{
    m_viewInternal->pageDown(true);
}

void KTextEditor::ViewPrivate::top()
{
    m_viewInternal->top_home();
}

void KTextEditor::ViewPrivate::shiftTop()
{
    m_viewInternal->top_home(true);
}

void KTextEditor::ViewPrivate::bottom()
{
    m_viewInternal->bottom_end();
}

void KTextEditor::ViewPrivate::shiftBottom()
{
    m_viewInternal->bottom_end(true);
}

void KTextEditor::ViewPrivate::toMatchingBracket()
{
    m_viewInternal->cursorToMatchingBracket();
}

void KTextEditor::ViewPrivate::shiftToMatchingBracket()
{
    m_viewInternal->cursorToMatchingBracket(true);
}

void KTextEditor::ViewPrivate::toPrevModifiedLine()
{
    const int startLine = m_viewInternal->m_cursor.line() - 1;
    const int line = m_doc->findTouchedLine(startLine, false);
    if (line >= 0) {
        KTextEditor::Cursor c(line, 0);
        m_viewInternal->updateSelection(c, false);
        m_viewInternal->updateCursor(c);
    }
}

void KTextEditor::ViewPrivate::toNextModifiedLine()
{
    const int startLine = m_viewInternal->m_cursor.line() + 1;
    const int line = m_doc->findTouchedLine(startLine, true);
    if (line >= 0) {
        KTextEditor::Cursor c(line, 0);
        m_viewInternal->updateSelection(c, false);
        m_viewInternal->updateCursor(c);
    }
}

KTextEditor::Range KTextEditor::ViewPrivate::selectionRange() const
{
    return m_selection;
}

KTextEditor::Document *KTextEditor::ViewPrivate::document() const
{
    return m_doc;
}

void KTextEditor::ViewPrivate::setContextMenu(QMenu *menu)
{
    if (m_contextMenu) {
        disconnect(m_contextMenu, SIGNAL(aboutToShow()), this, SLOT(aboutToShowContextMenu()));
        disconnect(m_contextMenu, SIGNAL(aboutToHide()), this, SLOT(aboutToHideContextMenu()));
    }
    m_contextMenu = menu;
    m_userContextMenuSet = true;

    if (m_contextMenu) {
        connect(m_contextMenu, SIGNAL(aboutToShow()), this, SLOT(aboutToShowContextMenu()));
        connect(m_contextMenu, SIGNAL(aboutToHide()), this, SLOT(aboutToHideContextMenu()));
    }
}

QMenu *KTextEditor::ViewPrivate::contextMenu() const
{
    if (m_userContextMenuSet) {
        return m_contextMenu;
    } else {
        KXMLGUIClient *client = const_cast<KTextEditor::ViewPrivate *>(this);
        while (client->parentClient()) {
            client = client->parentClient();
        }

        //qCDebug(LOG_PART) << "looking up all menu containers";
        if (client->factory()) {
            QList<QWidget *> conts = client->factory()->containers(QLatin1String("menu"));
            foreach (QWidget *w, conts) {
                if (w->objectName() == QLatin1String("ktexteditor_popup")) {
                    //perhaps optimize this block
                    QMenu *menu = (QMenu *)w;
                    disconnect(menu, SIGNAL(aboutToShow()), this, SLOT(aboutToShowContextMenu()));
                    disconnect(menu, SIGNAL(aboutToHide()), this, SLOT(aboutToHideContextMenu()));
                    connect(menu, SIGNAL(aboutToShow()), this, SLOT(aboutToShowContextMenu()));
                    connect(menu, SIGNAL(aboutToHide()), this, SLOT(aboutToHideContextMenu()));
                    return menu;
                }
            }
        }
    }
    return 0;
}

QMenu *KTextEditor::ViewPrivate::defaultContextMenu(QMenu *menu) const
{
    if (!menu) {
        menu = new QMenu(const_cast<KTextEditor::ViewPrivate *>(this));
    }

    menu->addAction(m_editUndo);
    menu->addAction(m_editRedo);
    menu->addSeparator();
    menu->addAction(m_cut);
    menu->addAction(m_copy);
    menu->addAction(m_paste);
    menu->addSeparator();
    menu->addAction(m_selectAll);
    menu->addAction(m_deSelect);
    if (QAction *spellingSuggestions = actionCollection()->action(QLatin1String("spelling_suggestions"))) {
        menu->addSeparator();
        menu->addAction(spellingSuggestions);
    }
    if (QAction *bookmark = actionCollection()->action(QLatin1String("bookmarks"))) {
        menu->addSeparator();
        menu->addAction(bookmark);
    }
    return menu;
}

void KTextEditor::ViewPrivate::aboutToShowContextMenu()
{
    QMenu *menu = qobject_cast<QMenu *>(sender());

    if (menu) {
        emit contextMenuAboutToShow(this, menu);
    }
}

void KTextEditor::ViewPrivate::aboutToHideContextMenu()
{
    m_spellingMenu->setUseMouseForMisspelledRange(false);
}

// BEGIN ConfigInterface stff
QStringList KTextEditor::ViewPrivate::configKeys() const
{
    return QStringList()  << QLatin1String("icon-bar")  << QLatin1String("line-numbers") << QLatin1String("dynamic-word-wrap")
           << QLatin1String("background-color")  << QLatin1String("selection-color")
           << QLatin1String("search-highlight-color")  << QLatin1String("replace-highlight-color")
           << QLatin1String("folding-bar") << QLatin1String("icon-border-color") << QLatin1String("folding-marker-color")
           << QLatin1String("line-number-color") << QLatin1String("modification-markers");

}

QVariant KTextEditor::ViewPrivate::configValue(const QString &key)
{
    if (key == QLatin1String("icon-bar")) {
        return config()->iconBar();
    } else if (key == QLatin1String("line-numbers")) {
        return config()->lineNumbers();
    } else if (key == QLatin1String("dynamic-word-wrap")) {
        return config()->dynWordWrap();
    } else if (key == QLatin1String("background-color")) {
        return renderer()->config()->backgroundColor();
    } else if (key == QLatin1String("selection-color")) {
        return renderer()->config()->selectionColor();
    } else if (key == QLatin1String("search-highlight-color")) {
        return renderer()->config()->searchHighlightColor();
    } else if (key == QLatin1String("replace-highlight-color")) {
        return renderer()->config()->replaceHighlightColor();
    } else if (key == QLatin1String("default-mark-type")) {
        return config()->defaultMarkType();
    } else if (key == QLatin1String("allow-mark-menu")) {
        return config()->allowMarkMenu();
    } else if (key == QLatin1String("folding-bar")) {
        return config()->foldingBar();
    } else if (key == QLatin1String("icon-border-color")) {
        return renderer()->config()->iconBarColor();
    } else if (key == QLatin1String("folding-marker-color")) {
        return renderer()->config()->foldingColor();
    } else if (key == QLatin1String("line-number-color")) {
        return renderer()->config()->lineNumberColor();
    } else if (key == QLatin1String("modification-markers")) {
        return config()->lineModification();
    }

    // return invalid variant
    return QVariant();
}

void KTextEditor::ViewPrivate::setConfigValue(const QString &key, const QVariant &value)
{
    if (value.canConvert(QVariant::Color)) {
        if (key == QLatin1String("background-color")) {
            renderer()->config()->setBackgroundColor(value.value<QColor>());
        } else if (key == QLatin1String("selection-color")) {
            renderer()->config()->setSelectionColor(value.value<QColor>());
        } else if (key == QLatin1String("search-highlight-color")) {
            renderer()->config()->setSearchHighlightColor(value.value<QColor>());
        } else if (key == QLatin1String("replace-highlight-color")) {
            renderer()->config()->setReplaceHighlightColor(value.value<QColor>());
        } else if (key == QLatin1String("icon-border-color")) {
            renderer()->config()->setIconBarColor(value.value<QColor>());
        } else if (key == QLatin1String("folding-marker-color")) {
            renderer()->config()->setFoldingColor(value.value<QColor>());
        } else if (key == QLatin1String("line-number-color")) {
            renderer()->config()->setLineNumberColor(value.value<QColor>());
        }

    } else if (value.type() == QVariant::Bool) {
        // Note explicit type check above. If we used canConvert, then
        // values of type UInt will be trapped here.
        if (key == QLatin1String("icon-bar")) {
            config()->setIconBar(value.toBool());
        } else if (key == QLatin1String("line-numbers")) {
            config()->setLineNumbers(value.toBool());
        } else if (key == QLatin1String("dynamic-word-wrap")) {
            config()->setDynWordWrap(value.toBool());
        } else if (key == QLatin1String("allow-mark-menu")) {
            config()->setAllowMarkMenu(value.toBool());
        } else if (key == QLatin1String("folding-bar")) {
            config()->setFoldingBar(value.toBool());
        } else if (key == QLatin1String("modification-markers")) {
            config()->setLineModification(value.toBool());
        }

    } else if (value.canConvert(QVariant::UInt)) {
        if (key == QLatin1String("default-mark-type")) {
            config()->setDefaultMarkType(value.toUInt());
        }
    }
}

// END ConfigInterface

void KTextEditor::ViewPrivate::userInvokedCompletion()
{
    completionWidget()->userInvokedCompletion();
}

KateViewBar *KTextEditor::ViewPrivate::bottomViewBar() const
{
    return m_bottomViewBar;
}

KateGotoBar *KTextEditor::ViewPrivate::gotoBar()
{
    if (!m_gotoBar) {
        m_gotoBar = new KateGotoBar(this);
        bottomViewBar()->addBarWidget(m_gotoBar);
    }

    return m_gotoBar;
}

KateDictionaryBar *KTextEditor::ViewPrivate::dictionaryBar()
{
    if (!m_dictionaryBar) {
        m_dictionaryBar = new KateDictionaryBar(this);
        bottomViewBar()->addBarWidget(m_dictionaryBar);
    }

    return m_dictionaryBar;
}

void KTextEditor::ViewPrivate::setAnnotationModel(KTextEditor::AnnotationModel *model)
{
    KTextEditor::AnnotationModel *oldmodel = m_annotationModel;
    m_annotationModel = model;
    m_viewInternal->m_leftBorder->annotationModelChanged(oldmodel, m_annotationModel);
}

KTextEditor::AnnotationModel *KTextEditor::ViewPrivate::annotationModel() const
{
    return m_annotationModel;
}

void KTextEditor::ViewPrivate::setAnnotationBorderVisible(bool visible)
{
    m_viewInternal->m_leftBorder->setAnnotationBorderOn(visible);
}

bool KTextEditor::ViewPrivate::isAnnotationBorderVisible() const
{
    return m_viewInternal->m_leftBorder->annotationBorderOn();
}

KTextEditor::Range KTextEditor::ViewPrivate::visibleRange()
{
    //ensure that the view is up-to-date, otherwise 'endPos()' might fail!
    m_viewInternal->updateView();
    return KTextEditor::Range(m_viewInternal->toRealCursor(m_viewInternal->startPos()),
                              m_viewInternal->toRealCursor(m_viewInternal->endPos()));
}

void KTextEditor::ViewPrivate::toggleOnTheFlySpellCheck(bool b)
{
    m_doc->onTheFlySpellCheckingEnabled(b);
}

void KTextEditor::ViewPrivate::reflectOnTheFlySpellCheckStatus(bool enabled)
{
    m_spellingMenu->setVisible(enabled);
    m_toggleOnTheFlySpellCheck->setChecked(enabled);
}

KateSpellingMenu *KTextEditor::ViewPrivate::spellingMenu()
{
    return m_spellingMenu;
}

void KTextEditor::ViewPrivate::notifyAboutRangeChange(int startLine, int endLine, bool rangeWithAttribute)
{
#ifdef VIEW_RANGE_DEBUG
    // output args
    qCDebug(LOG_PART) << "trigger attribute changed from" << startLine << "to" << endLine << "rangeWithAttribute" << rangeWithAttribute;
#endif

    // first call:
    if (!m_delayedUpdateTriggered) {
        m_delayedUpdateTriggered = true;
        m_lineToUpdateMin = -1;
        m_lineToUpdateMax = -1;

        // only set initial line range, if range with attribute!
        if (rangeWithAttribute) {
            m_lineToUpdateMin = startLine;
            m_lineToUpdateMax = endLine;
        }

        // emit queued signal and be done
        emit delayedUpdateOfView();
        return;
    }

    // ignore lines if no attribute
    if (!rangeWithAttribute) {
        return;
    }

    // update line range
    if (startLine != -1 && (m_lineToUpdateMin == -1 || startLine < m_lineToUpdateMin)) {
        m_lineToUpdateMin = startLine;
    }

    if (endLine != -1 && endLine > m_lineToUpdateMax) {
        m_lineToUpdateMax = endLine;
    }
}

void KTextEditor::ViewPrivate::slotDelayedUpdateOfView()
{
    if (!m_delayedUpdateTriggered) {
        return;
    }

#ifdef VIEW_RANGE_DEBUG
    // output args
    qCDebug(LOG_PART) << "delayed attribute changed from" << m_lineToUpdateMin << "to" << m_lineToUpdateMax;
#endif

    // update ranges in
    updateRangesIn(KTextEditor::Attribute::ActivateMouseIn);
    updateRangesIn(KTextEditor::Attribute::ActivateCaretIn);

    // update view, if valid line range, else only feedback update wanted anyway
    if (m_lineToUpdateMin != -1 && m_lineToUpdateMax != -1) {
        tagLines(m_lineToUpdateMin, m_lineToUpdateMax, true);
        updateView(true);
    }

    // reset flags
    m_delayedUpdateTriggered = false;
    m_lineToUpdateMin = -1;
    m_lineToUpdateMax = -1;
}

void KTextEditor::ViewPrivate::updateRangesIn(KTextEditor::Attribute::ActivationType activationType)
{
    // new ranges with cursor in, default none
    QSet<Kate::TextRange *> newRangesIn;

    // on which range set we work?
    QSet<Kate::TextRange *> &oldSet = (activationType == KTextEditor::Attribute::ActivateMouseIn) ? m_rangesMouseIn : m_rangesCaretIn;

    // which cursor position to honor?
    KTextEditor::Cursor currentCursor = (activationType == KTextEditor::Attribute::ActivateMouseIn) ? m_viewInternal->getMouse() : m_viewInternal->getCursor();

    // first: validate the remembered ranges
    QSet<Kate::TextRange *> validRanges;
    foreach (Kate::TextRange *range, oldSet)
        if (m_doc->buffer().rangePointerValid(range)) {
            validRanges.insert(range);
        }

    // cursor valid? else no new ranges can be found
    if (currentCursor.isValid() && currentCursor.line() < m_doc->buffer().lines()) {
        // now: get current ranges for the line of cursor with an attribute
        QList<Kate::TextRange *> rangesForCurrentCursor = m_doc->buffer().rangesForLine(currentCursor.line(), this, false);

        // match which ranges really fit the given cursor
        foreach (Kate::TextRange *range, rangesForCurrentCursor) {
            // range has no dynamic attribute of right type and no feedback object
            if ((!range->attribute() || !range->attribute()->dynamicAttribute(activationType)) && !range->feedback()) {
                continue;
            }

            // range doesn't contain cursor, not interesting
            if ((range->start().insertBehavior() == KTextEditor::MovingCursor::StayOnInsert)
                    ? (currentCursor < range->start().toCursor()) : (currentCursor <= range->start().toCursor())) {
                continue;
            }

            if ((range->end().insertBehavior() == KTextEditor::MovingCursor::StayOnInsert)
                    ? (range->end().toCursor() <= currentCursor) : (range->end().toCursor() < currentCursor)) {
                continue;
            }

            // range contains cursor, was it already in old set?
            if (validRanges.contains(range)) {
                // insert in new, remove from old, be done with it
                newRangesIn.insert(range);
                validRanges.remove(range);
                continue;
            }

            // oh, new range, trigger update and insert into new set
            newRangesIn.insert(range);

            if (range->attribute() && range->attribute()->dynamicAttribute(activationType)) {
                notifyAboutRangeChange(range->start().line(), range->end().line(), true);
            }

            // feedback
            if (range->feedback()) {
                if (activationType == KTextEditor::Attribute::ActivateMouseIn) {
                    range->feedback()->mouseEnteredRange(range, this);
                } else {
                    range->feedback()->caretEnteredRange(range, this);
                }
            }

#ifdef VIEW_RANGE_DEBUG
            // found new range for activation
            qCDebug(LOG_PART) << "activated new range" << range << "by" << activationType;
#endif
        }
    }

    // now: notify for left ranges!
    foreach (Kate::TextRange *range, validRanges) {
        // range valid + right dynamic attribute, trigger update
        if (range->toRange().isValid() && range->attribute() && range->attribute()->dynamicAttribute(activationType)) {
            notifyAboutRangeChange(range->start().line(), range->end().line(), true);
        }

        // feedback
        if (range->feedback()) {
            if (activationType == KTextEditor::Attribute::ActivateMouseIn) {
                range->feedback()->mouseExitedRange(range, this);
            } else {
                range->feedback()->caretExitedRange(range, this);
            }
        }
    }

    // set new ranges
    oldSet = newRangesIn;
}

void KTextEditor::ViewPrivate::postMessage(KTextEditor::Message *message,
                           QList<QSharedPointer<QAction> > actions)
{
    // just forward to KateMessageWidget :-)
    if (message->position() == KTextEditor::Message::AboveView) {
        m_topMessageWidget->postMessage(message, actions);
    } else if (message->position() == KTextEditor::Message::BelowView) {
        m_bottomMessageWidget->postMessage(message, actions);
    } else if (message->position() == KTextEditor::Message::TopInView) {
        if (!m_floatTopMessageWidget) {
            m_floatTopMessageWidget = new KateMessageWidget(m_viewInternal, true);
            m_notificationLayout->insertWidget(0, m_floatTopMessageWidget, 0, Qt::Alignment(Qt::AlignTop | Qt::AlignRight));
            connect(this, SIGNAL(displayRangeChanged(KTextEditor::ViewPrivate*)), m_floatTopMessageWidget, SLOT(startAutoHideTimer()));
            connect(this, SIGNAL(cursorPositionChanged(KTextEditor::View*,KTextEditor::Cursor)), m_floatTopMessageWidget, SLOT(startAutoHideTimer()));
        }
        m_floatTopMessageWidget->postMessage(message, actions);
    } else if (message->position() == KTextEditor::Message::BottomInView) {
        if (!m_floatBottomMessageWidget) {
            m_floatBottomMessageWidget = new KateMessageWidget(m_viewInternal, true);
            m_notificationLayout->addWidget(m_floatBottomMessageWidget, 0, Qt::Alignment(Qt::AlignBottom | Qt::AlignRight));
            connect(this, SIGNAL(displayRangeChanged(KTextEditor::ViewPrivate*)), m_floatBottomMessageWidget, SLOT(startAutoHideTimer()));
            connect(this, SIGNAL(cursorPositionChanged(KTextEditor::View*,KTextEditor::Cursor)), m_floatBottomMessageWidget, SLOT(startAutoHideTimer()));
        }
        m_floatBottomMessageWidget->postMessage(message, actions);
    }
}

void KTextEditor::ViewPrivate::saveFoldingState()
{
    m_savedFoldingState = m_textFolding.exportFoldingRanges();
}

void KTextEditor::ViewPrivate::applyFoldingState()
{
    m_textFolding.importFoldingRanges(m_savedFoldingState);
    m_savedFoldingState.clear();
}

void KTextEditor::ViewPrivate::exportHtmlToClipboard ()
{
    KateExporter (this).exportToClipboard ();
}

void KTextEditor::ViewPrivate::exportHtmlToFile ()
{
    KateExporter (this).exportToFile ();
}

void KTextEditor::ViewPrivate::clearHighlights()
{
    qDeleteAll(m_rangesForHighlights);
    m_rangesForHighlights.clear();
    m_currentTextForHighlights.clear();
}

void KTextEditor::ViewPrivate::selectionChangedForHighlights()
{
    QString text;
    // if text of selection is still the same, abort
    if (selection() && selectionRange().onSingleLine()) {
        text = selectionText();
        if (text == m_currentTextForHighlights)
            return;
    }

    // text changed: remove all highlights + create new ones
    // (do not call clearHighlights(), since this also resets the m_currentTextForHighlights
    qDeleteAll(m_rangesForHighlights);
    m_rangesForHighlights.clear();
  
    // do not highlight strings with leading and trailing spaces
    if (!text.isEmpty() && (text.at(0).isSpace() || text.at(text.length()-1).isSpace()))
        return; 

    m_currentTextForHighlights = text;
    if (!m_currentTextForHighlights.isEmpty())
        createHighlights();
}

void KTextEditor::ViewPrivate::createHighlights()
{
    KTextEditor::Attribute::Ptr attr(new KTextEditor::Attribute());
    attr->setBackground(Qt::yellow);

    // set correct highlight color from Kate's color schema
    QColor color = configValue(QLatin1String("search-highlight-color")).value<QColor>();
    attr->setBackground(color);

    KTextEditor::Cursor start(0, 0);
    KTextEditor::Range searchRange;

    /**
     * only add word boundary if we can find the text then
     * fixes $lala hl
     */
    QString regex = QRegExp::escape (m_currentTextForHighlights);
    if (QRegExp (QString::fromLatin1("\\b%1").arg(regex)).indexIn (QString::fromLatin1(" %1 ").arg(m_currentTextForHighlights)) != -1)
        regex = QString::fromLatin1("\\b%1").arg(regex);
    if (QRegExp (QString::fromLatin1("%1\\b").arg(regex)).indexIn (QString::fromLatin1(" %1 ").arg(m_currentTextForHighlights)) != -1)
        regex = QString::fromLatin1("%1\\b").arg(regex);

    QVector<KTextEditor::Range> matches;
    do {
        searchRange.setRange(start, document()->documentEnd());

        matches = m_doc->searchText(searchRange, regex, KTextEditor::Search::Regex);

        if (matches.first().isValid()) {
            KTextEditor::MovingRange* mr = m_doc->newMovingRange(matches.first());
            mr->setAttribute(attr);
            mr->setView(this);
            mr->setZDepth(-90000.0); // Set the z-depth to slightly worse than the selection
            mr->setAttributeOnlyForViews(true);
            m_rangesForHighlights.append(mr);
            start = matches.first().end();
        }
    } while (matches.first().isValid());
}

KateAbstractInputMode *KTextEditor::ViewPrivate::currentInputMode() const
{
    return m_viewInternal->m_currentInputMode;
}

void KTextEditor::ViewPrivate::toggleInputMode(bool on)
{
    QAction *a = dynamic_cast<QAction *>(sender());
    if (!a) {
        return;
    }

    InputMode newmode = static_cast<KTextEditor::View::InputMode>(a->data().toInt());

    if (currentInputMode()->viewInputMode() == newmode) {
        if (!on) {
            a->setChecked(true); // nasty
        }

        return;
    }

    Q_FOREACH(QAction *ac, m_inputModeActions) {
        if (ac != a) {
            ac->setChecked(false);
        }
    }

    setInputMode(newmode);
}

//BEGIN KTextEditor::PrintInterface stuff
bool KTextEditor::ViewPrivate::print()
{
    return KatePrinter::print(this);
}

void KTextEditor::ViewPrivate::printPreview()
{
    KatePrinter::printPreview(this);
}

//END

KTextEditor::Attribute::Ptr KTextEditor::ViewPrivate::defaultStyleAttribute(KTextEditor::DefaultStyle defaultStyle) const
{
    KateRendererConfig * renderConfig = const_cast<KTextEditor::ViewPrivate*>(this)->renderer()->config();

    KTextEditor::Attribute::Ptr style = m_doc->highlight()->attributes(renderConfig->schema()).at(defaultStyle);
    if (!style->hasProperty(QTextFormat::BackgroundBrush)) {
        // make sure the returned style has the default background color set
        style = new KTextEditor::Attribute(*style);
        style->setBackground(QBrush(renderConfig->backgroundColor()));
    }
    return style;
}

QList<KTextEditor::AttributeBlock> KTextEditor::ViewPrivate::lineAttributes(int line)
{
    QList<KTextEditor::AttributeBlock> attribs;

    if (line < 0 || line >= m_doc->lines())
        return attribs;

    Kate::TextLine kateLine = m_doc->kateTextLine(line);
    if (!kateLine) {
        return attribs;
    }

    const QVector<Kate::TextLineData::Attribute> &intAttrs = kateLine->attributesList();
    for (int i = 0; i < intAttrs.size(); ++i) {
        if (intAttrs[i].length > 0 && intAttrs[i].attributeValue > 0) {
            attribs << KTextEditor::AttributeBlock(
                        intAttrs.at(i).offset,
                        intAttrs.at(i).length,
                        renderer()->attribute(intAttrs.at(i).attributeValue)
                    );
        }
    }

    return attribs;
}
