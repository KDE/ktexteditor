/* This file is part of the KDE libraries
   Copyright (C) 2001-2004 Christoph Cullmann <cullmann@kde.org>
   Copyright (C) 2001 Joseph Wenninger <jowenn@kde.org>
   Copyright (C) 1999 Jochen Wilhelmy <digisnap@cs.tu-berlin.de>
   Copyright (C) 2006 Hamish Rodda <rodda@kde.org>
   Copyright (C) 2007 Mirko Stocker <me@misto.ch>
   Copyright (C) 2009-2010 Michel Ludwig <michel.ludwig@kdemail.net>
   Copyright (C) 2013 Gerald Senarclens de Grancy <oss@senarclens.eu>
   Copyright (C) 2013 Andrey Matveyakin <a.matveyakin@gmail.com>

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
   Boston, MA 02111-13020, USA.
*/

//BEGIN includes
#include "katedocument.h"
#include "kateglobal.h"
#include "katedialogs.h"
#include "katehighlight.h"
#include "kateview.h"
#include "kateautoindent.h"
#include "katetextline.h"
#include "katehighlighthelpers.h"
#include "katerenderer.h"
#include "kateregexp.h"
#include "kateplaintextsearch.h"
#include "kateregexpsearch.h"
#include "kateconfig.h"
#include "katemodemanager.h"
#include "kateschema.h"
#include "katebuffer.h"
#include "kateundomanager.h"
#include "spellcheck/prefixstore.h"
#include "spellcheck/ontheflycheck.h"
#include "spellcheck/spellcheck.h"
#include "katescriptmanager.h"
#include "kateswapfile.h"
#include "katepartdebug.h"
#include "printing/kateprinter.h"
#include "kateabstractinputmode.h"
#include "katetemplatehandler.h"

#include <KTextEditor/DocumentCursor>
#include <KTextEditor/Attribute>

#include <KParts/OpenUrlArguments>
#include <KIO/Job>
#include <KIO/JobUiDelegate>
#include <KFileItem>
#include <KJobWidgets>
#include <KMessageBox>
#include <KStandardAction>
#include <KXMLGUIFactory>
#include <kdirwatch.h>
#include <KCodecs>
#include <KStringHandler>
#include <KConfigGroup>
#include <KMountPoint>

#include <QCryptographicHash>
#include <QFile>
#include <QMap>
#include <QTextCodec>
#include <QTextStream>
#include <QTimer>
#include <QClipboard>
#include <QApplication>
#include <QFileDialog>
#include <QMimeDatabase>
#include <QTemporaryFile>

#include <cmath>

#ifdef LIBGIT2_FOUND
#include <git2.h>
#include <git2/oid.h>
#include <git2/repository.h>
#endif

//END  includes

#if 0
#define EDIT_DEBUG qCDebug(LOG_KTE)
#else
#define EDIT_DEBUG if (0) qCDebug(LOG_KTE)
#endif

static inline QChar matchingStartBracket(QChar c, bool withQuotes)
{
    switch (c.toLatin1()) {
        case '}': return QLatin1Char('{');
        case ']': return QLatin1Char('[');
        case ')': return QLatin1Char('(');
        case '\'': return withQuotes ? QLatin1Char('\'') : QChar();
        case '"': return withQuotes ? QLatin1Char('"') : QChar();
    }
    return QChar();
}

static inline QChar matchingEndBracket(QChar c, bool withQuotes)
{
    switch (c.toLatin1()) {
        case '{': return QLatin1Char('}');
        case '[': return QLatin1Char(']');
        case '(': return QLatin1Char(')');
        case '\'': return withQuotes ? QLatin1Char('\'') : QChar();
        case '"': return withQuotes ? QLatin1Char('"') : QChar();
    }
    return QChar();
}

static inline QChar matchingBracket(QChar c, bool withQuotes)
{
    QChar bracket = matchingStartBracket(c, withQuotes);
    if (bracket.isNull()) {
        bracket = matchingEndBracket(c, false);
    }
    return bracket;
}

static inline bool isStartBracket(const QChar &c)
{
    return ! matchingEndBracket(c, false).isNull();
}

static inline bool isEndBracket(const QChar &c)
{
    return ! matchingStartBracket(c, false).isNull();
}

static inline bool isBracket(const QChar &c)
{
    return isStartBracket(c) || isEndBracket(c);
}

/**
 * normalize given url
 * @param url input url
 * @return normalized url
 */
static QUrl normalizeUrl (const QUrl &url)
{
    /**
     * only normalize local urls
     */
    if (url.isEmpty() || !url.isLocalFile())
        return url;

    /**
     * don't normalize if not existing!
     * canonicalFilePath won't work!
     */
    const QString normalizedUrl(QFileInfo(url.toLocalFile()).canonicalFilePath());
    if (normalizedUrl.isEmpty())
        return url;

    /**
     * else: use canonicalFilePath to normalize
     */
    return QUrl::fromLocalFile(normalizedUrl);
}

//BEGIN d'tor, c'tor
//
// KTextEditor::DocumentPrivate Constructor
//
KTextEditor::DocumentPrivate::DocumentPrivate(bool bSingleViewMode,
                           bool bReadOnly, QWidget *parentWidget,
                           QObject *parent)
    : KTextEditor::Document (this, parent),
      m_bSingleViewMode(bSingleViewMode),
      m_bReadOnly(bReadOnly),
      m_activeView(nullptr),
      editSessionNumber(0),
      editIsRunning(false),
      m_undoMergeAllEdits(false),
      m_undoManager(new KateUndoManager(this)),
      m_editableMarks(markType01),
      m_annotationModel(nullptr),
      m_buffer(new KateBuffer(this)),
      m_indenter(new KateAutoIndent(this)),
      m_hlSetByUser(false),
      m_bomSetByUser(false),
      m_indenterSetByUser(false),
      m_userSetEncodingForNextReload(false),
      m_modOnHd(false),
      m_modOnHdReason(OnDiskUnmodified),
      m_prevModOnHdReason(OnDiskUnmodified),
      m_docName(QStringLiteral("need init")),
      m_docNameNumber(0),
      m_fileType(QStringLiteral("Normal")),
      m_fileTypeSetByUser(false),
      m_reloading(false),
      m_config(new KateDocumentConfig(this)),
      m_fileChangedDialogsActivated(false),
      m_onTheFlyChecker(nullptr),
      m_documentState(DocumentIdle),
      m_readWriteStateBeforeLoading(false),
      m_isUntitled(true),
      m_openingError(false)
{
    /**
     * no plugins from kparts here
     */
    setPluginLoadingMode (DoNotLoadPlugins);

    /**
     * pass on our component data, do this after plugin loading is off
     */
    setComponentData(KTextEditor::EditorPrivate::self()->aboutData());

    /**
     * avoid spamming plasma and other window managers with progress dialogs
     * we show such stuff inline in the views!
     */
    setProgressInfoEnabled(false);

    // register doc at factory
    KTextEditor::EditorPrivate::self()->registerDocument(this);

    // normal hl
    m_buffer->setHighlight(0);

    // swap file
    m_swapfile = (config()->swapFileMode() == KateDocumentConfig::DisableSwapFile) ? nullptr : new Kate::SwapFile(this);

    // important, fill in the config into the indenter we use...
    m_indenter->updateConfig();

    // some nice signals from the buffer
    connect(m_buffer, SIGNAL(tagLines(int,int)), this, SLOT(tagLines(int,int)));

    // if the user changes the highlight with the dialog, notify the doc
    connect(KateHlManager::self(), SIGNAL(changed()), SLOT(internalHlChanged()));

    // signals for mod on hd
    connect(KTextEditor::EditorPrivate::self()->dirWatch(), SIGNAL(dirty(QString)),
            this, SLOT(slotModOnHdDirty(QString)));

    connect(KTextEditor::EditorPrivate::self()->dirWatch(), SIGNAL(created(QString)),
            this, SLOT(slotModOnHdCreated(QString)));

    connect(KTextEditor::EditorPrivate::self()->dirWatch(), SIGNAL(deleted(QString)),
            this, SLOT(slotModOnHdDeleted(QString)));

    /**
     * singleshot timer to handle updates of mod on hd state delayed
     */
    m_modOnHdTimer.setSingleShot(true);
    m_modOnHdTimer.setInterval(200);
    connect(&m_modOnHdTimer, SIGNAL(timeout()), this, SLOT(slotDelayedHandleModOnHd()));

    /**
     * load handling
     * this is needed to ensure we signal the user if a file ist still loading
     * and to disallow him to edit in that time
     */
    connect(this, SIGNAL(started(KIO::Job*)), this, SLOT(slotStarted(KIO::Job*)));
    connect(this, SIGNAL(completed()), this, SLOT(slotCompleted()));
    connect(this, SIGNAL(canceled(QString)), this, SLOT(slotCanceled()));

    connect(this, SIGNAL(urlChanged(QUrl)), this, SLOT(slotUrlChanged(QUrl)));

    // update doc name
    updateDocName();

    // if single view mode, like in the konqui embedding, create a default view ;)
    // be lazy, only create it now, if any parentWidget is given, otherwise widget()
    // will create it on demand...
    if (m_bSingleViewMode && parentWidget) {
        KTextEditor::View *view = (KTextEditor::View *)createView(parentWidget);
        insertChildClient(view);
        view->setContextMenu(view->defaultContextMenu());
        setWidget(view);
    }

    connect(m_undoManager, SIGNAL(undoChanged()), this, SIGNAL(undoChanged()));
    connect(m_undoManager, SIGNAL(undoStart(KTextEditor::Document*)),   this, SIGNAL(editingStarted(KTextEditor::Document*)));
    connect(m_undoManager, SIGNAL(undoEnd(KTextEditor::Document*)),     this, SIGNAL(editingFinished(KTextEditor::Document*)));
    connect(m_undoManager, SIGNAL(redoStart(KTextEditor::Document*)),   this, SIGNAL(editingStarted(KTextEditor::Document*)));
    connect(m_undoManager, SIGNAL(redoEnd(KTextEditor::Document*)),     this, SIGNAL(editingFinished(KTextEditor::Document*)));

    connect(this, SIGNAL(sigQueryClose(bool*,bool*)), this, SLOT(slotQueryClose_save(bool*,bool*)));

    connect(this, &KTextEditor::DocumentPrivate::textRemoved, this, &KTextEditor::DocumentPrivate::saveEditingPositions);
    connect(this, &KTextEditor::DocumentPrivate::textInserted, this, &KTextEditor::DocumentPrivate::saveEditingPositions);
    connect(this, SIGNAL(aboutToInvalidateMovingInterfaceContent(KTextEditor::Document*)), this, SLOT(clearEditingPosStack()));
    onTheFlySpellCheckingEnabled(config()->onTheFlySpellCheck());
}

//
// KTextEditor::DocumentPrivate Destructor
//
KTextEditor::DocumentPrivate::~DocumentPrivate()
{
    // delete pending mod-on-hd message, if applicable
    delete m_modOnHdHandler;

    /**
     * we are about to delete cursors/ranges/...
     */
    emit aboutToDeleteMovingInterfaceContent(this);

    // kill it early, it has ranges!
    delete m_onTheFlyChecker;
    m_onTheFlyChecker = nullptr;

    clearDictionaryRanges();

    // Tell the world that we're about to close (== destruct)
    // Apps must receive this in a direct signal-slot connection, and prevent
    // any further use of interfaces once they return.
    emit aboutToClose(this);

    // remove file from dirwatch
    deactivateDirWatch();

    // thanks for offering, KPart, but we're already self-destructing
    setAutoDeleteWidget(false);
    setAutoDeletePart(false);

    // clean up remaining views
    qDeleteAll (m_views.keys());
    m_views.clear();

    // cu marks
    for (QHash<int, KTextEditor::Mark *>::const_iterator i = m_marks.constBegin(); i != m_marks.constEnd(); ++i) {
        delete i.value();
    }
    m_marks.clear();

    delete m_config;
    KTextEditor::EditorPrivate::self()->deregisterDocument(this);
}
//END

void KTextEditor::DocumentPrivate::saveEditingPositions(KTextEditor::Document *, const KTextEditor::Range &range)
{
    if (m_editingStackPosition != m_editingStack.size() - 1) {
        m_editingStack.resize(m_editingStackPosition);
    }
    KTextEditor::MovingInterface *moving = qobject_cast<KTextEditor::MovingInterface *>(this);
    const auto c = range.start();
    QSharedPointer<KTextEditor::MovingCursor> mc (moving->newMovingCursor(c));
    if (!m_editingStack.isEmpty() && c.line() == m_editingStack.top()->line()) {
        m_editingStack.pop();
    }
    m_editingStack.push(mc);
    if (m_editingStack.size() > s_editingStackSizeLimit) {
        m_editingStack.removeFirst();
    }
    m_editingStackPosition = m_editingStack.size() - 1;
}

KTextEditor::Cursor KTextEditor::DocumentPrivate::lastEditingPosition(EditingPositionKind nextOrPrev, KTextEditor::Cursor currentCursor)
{
    if (m_editingStack.isEmpty()) {
      return KTextEditor::Cursor::invalid();
    }
    auto targetPos = m_editingStack.at(m_editingStackPosition)->toCursor();
    if (targetPos == currentCursor) {
        if (nextOrPrev == Previous) {
            m_editingStackPosition--;
        }
        else {
            m_editingStackPosition++;
        }
        m_editingStackPosition = qBound(0, m_editingStackPosition, m_editingStack.size() - 1);
    }
    return m_editingStack.at(m_editingStackPosition)->toCursor();
}

void KTextEditor::DocumentPrivate::clearEditingPosStack()
{
    m_editingStack.clear();
    m_editingStackPosition = -1;
}

// on-demand view creation
QWidget *KTextEditor::DocumentPrivate::widget()
{
    // no singleViewMode -> no widget()...
    if (!singleViewMode()) {
        return nullptr;
    }

    // does a widget exist already? use it!
    if (KTextEditor::Document::widget()) {
        return KTextEditor::Document::widget();
    }

    // create and return one...
    KTextEditor::View *view = (KTextEditor::View *)createView(nullptr);
    insertChildClient(view);
    view->setContextMenu(view->defaultContextMenu());
    setWidget(view);
    return view;
}

//BEGIN KTextEditor::Document stuff

KTextEditor::View *KTextEditor::DocumentPrivate::createView(QWidget *parent, KTextEditor::MainWindow *mainWindow)
{
    KTextEditor::ViewPrivate *newView = new KTextEditor::ViewPrivate(this, parent, mainWindow);

    if (m_fileChangedDialogsActivated) {
        connect(newView, SIGNAL(focusIn(KTextEditor::View*)), this, SLOT(slotModifiedOnDisk()));
    }

    emit viewCreated(this, newView);

    // post existing messages to the new view, if no specific view is given
    foreach (KTextEditor::Message *message, m_messageHash.keys()) {
        if (!message->view()) {
            newView->postMessage(message, m_messageHash[message]);
        }
    }

    return newView;
}

KTextEditor::Range KTextEditor::DocumentPrivate::rangeOnLine(KTextEditor::Range range, int line) const
{
    const int col1 = toVirtualColumn(range.start());
    const int col2 = toVirtualColumn(range.end());
    return KTextEditor::Range(line, fromVirtualColumn(line, col1), line, fromVirtualColumn(line, col2));
}

//BEGIN KTextEditor::EditInterface stuff

bool KTextEditor::DocumentPrivate::isEditingTransactionRunning() const
{
    return editSessionNumber > 0;
}

QString KTextEditor::DocumentPrivate::text() const
{
    return m_buffer->text();
}

QString KTextEditor::DocumentPrivate::text(const KTextEditor::Range &range, bool blockwise) const
{
    if (!range.isValid()) {
        qCWarning(LOG_KTE) << "Text requested for invalid range" << range;
        return QString();
    }

    QString s;

    if (range.start().line() == range.end().line()) {
        if (range.start().column() > range.end().column()) {
            return QString();
        }

        Kate::TextLine textLine = m_buffer->plainLine(range.start().line());

        if (!textLine) {
            return QString();
        }

        return textLine->string(range.start().column(), range.end().column() - range.start().column());
    } else {

        for (int i = range.start().line(); (i <= range.end().line()) && (i < m_buffer->count()); ++i) {
            Kate::TextLine textLine = m_buffer->plainLine(i);

            if (!blockwise) {
                if (i == range.start().line()) {
                    s.append(textLine->string(range.start().column(), textLine->length() - range.start().column()));
                } else if (i == range.end().line()) {
                    s.append(textLine->string(0, range.end().column()));
                } else {
                    s.append(textLine->string());
                }
            } else {
                KTextEditor::Range subRange = rangeOnLine(range, i);
                s.append(textLine->string(subRange.start().column(), subRange.columnWidth()));
            }

            if (i < range.end().line()) {
                s.append(QLatin1Char('\n'));
            }
        }
    }

    return s;
}

QChar KTextEditor::DocumentPrivate::characterAt(const KTextEditor::Cursor &position) const
{
    Kate::TextLine textLine = m_buffer->plainLine(position.line());

    if (!textLine) {
        return QChar();
    }

    return textLine->at(position.column());
}

QString KTextEditor::DocumentPrivate::wordAt(const KTextEditor::Cursor &cursor) const
{
    return text(wordRangeAt(cursor));
}

KTextEditor::Range KTextEditor::DocumentPrivate::wordRangeAt(const KTextEditor::Cursor &cursor) const
{
    // get text line
    const int line = cursor.line();
    Kate::TextLine textLine = m_buffer->plainLine(line);
    if (!textLine) {
        return KTextEditor::Range::invalid();
    }

    // make sure the cursor is
    const int lineLenth = textLine->length();
    if (cursor.column() > lineLenth) {
        return KTextEditor::Range::invalid();
    }

    int start = cursor.column();
    int end = start;

    while (start > 0 && highlight()->isInWord(textLine->at(start - 1), textLine->attribute(start - 1))) {
        start--;
    }
    while (end < lineLenth && highlight()->isInWord(textLine->at(end), textLine->attribute(end))) {
        end++;
    }

    return KTextEditor::Range(line, start, line, end);
}

bool KTextEditor::DocumentPrivate::isValidTextPosition(const KTextEditor::Cursor& cursor) const
{
    const int ln = cursor.line();
    const int col = cursor.column();
    // cursor in document range?
    if (ln < 0 || col < 0 || ln >= lines() || col > lineLength(ln)) {
        return false;
    }

    const QString str = line(ln);
    Q_ASSERT(str.length() >= col);

    // cursor at end of line?
    const int len = lineLength(ln);
    if (col == 0 || col == len) {
        return true;
    }

    // cursor in the middle of a valid utf32-surrogate?
    return (! str.at(col).isLowSurrogate())
        || (! str.at(col-1).isHighSurrogate());
}

QStringList KTextEditor::DocumentPrivate::textLines(const KTextEditor::Range &range, bool blockwise) const
{
    QStringList ret;

    if (!range.isValid()) {
        qCWarning(LOG_KTE) << "Text requested for invalid range" << range;
        return ret;
    }

    if (blockwise && (range.start().column() > range.end().column())) {
        return ret;
    }

    if (range.start().line() == range.end().line()) {
        Q_ASSERT(range.start() <= range.end());

        Kate::TextLine textLine = m_buffer->plainLine(range.start().line());

        if (!textLine) {
            return ret;
        }

        ret << textLine->string(range.start().column(), range.end().column() - range.start().column());
    } else {
        for (int i = range.start().line(); (i <= range.end().line()) && (i < m_buffer->count()); ++i) {
            Kate::TextLine textLine = m_buffer->plainLine(i);

            if (!blockwise) {
                if (i == range.start().line()) {
                    ret << textLine->string(range.start().column(), textLine->length() - range.start().column());
                } else if (i == range.end().line()) {
                    ret << textLine->string(0, range.end().column());
                } else {
                    ret << textLine->string();
                }
            } else {
                KTextEditor::Range subRange = rangeOnLine(range, i);
                ret << textLine->string(subRange.start().column(), subRange.columnWidth());
            }
        }
    }

    return ret;
}

QString KTextEditor::DocumentPrivate::line(int line) const
{
    Kate::TextLine l = m_buffer->plainLine(line);

    if (!l) {
        return QString();
    }

    return l->string();
}

bool KTextEditor::DocumentPrivate::setText(const QString &s)
{
    if (!isReadWrite()) {
        return false;
    }

    QList<KTextEditor::Mark> msave;

    foreach (KTextEditor::Mark *mark, m_marks) {
        msave.append(*mark);
    }

    editStart();

    // delete the text
    clear();

    // insert the new text
    insertText(KTextEditor::Cursor(), s);

    editEnd();

    foreach (KTextEditor::Mark mark, msave) {
        setMark(mark.line, mark.type);
    }

    return true;
}

bool KTextEditor::DocumentPrivate::setText(const QStringList &text)
{
    if (!isReadWrite()) {
        return false;
    }

    QList<KTextEditor::Mark> msave;

    foreach (KTextEditor::Mark *mark, m_marks) {
        msave.append(*mark);
    }

    editStart();

    // delete the text
    clear();

    // insert the new text
    insertText(KTextEditor::Cursor::start(), text);

    editEnd();

    foreach (KTextEditor::Mark mark, msave) {
        setMark(mark.line, mark.type);
    }

    return true;
}

bool KTextEditor::DocumentPrivate::clear()
{
    if (!isReadWrite()) {
        return false;
    }

    foreach (KTextEditor::ViewPrivate *view, m_views) {
        view->clear();
        view->tagAll();
        view->update();
    }

    clearMarks();

    emit aboutToInvalidateMovingInterfaceContent(this);
    m_buffer->invalidateRanges();

    emit aboutToRemoveText(documentRange());

    return editRemoveLines(0, lastLine());
}

bool KTextEditor::DocumentPrivate::insertText(const KTextEditor::Cursor &position, const QString &text, bool block)
{
    if (!isReadWrite()) {
        return false;
    }

    if (text.isEmpty()) {
        return true;
    }

    editStart();

    int currentLine = position.line();
    int currentLineStart = 0;
    const int totalLength = text.length();
    int insertColumn = position.column();

    // pad with empty lines, if insert position is after last line
    if (position.line() > lines()) {
        int line = lines();
        while (line <= position.line()) {
            editInsertLine(line, QString());
            line++;
        }
    }

    const int tabWidth = config()->tabWidth();

    static const QChar newLineChar(QLatin1Char('\n'));

    int insertColumnExpanded = insertColumn;
    Kate::TextLine l = plainKateTextLine(currentLine);
    if (l) {
        insertColumnExpanded = l->toVirtualColumn(insertColumn, tabWidth);
    }
    int positionColumnExpanded = insertColumnExpanded;

    int pos = 0;
    for (; pos < totalLength; pos++) {
        const QChar &ch = text.at(pos);

        if (ch == newLineChar) {
            // Only perform the text insert if there is text to insert
            if (currentLineStart < pos) {
                editInsertText(currentLine, insertColumn, text.mid(currentLineStart, pos - currentLineStart));
            }

            if (!block) {
                editWrapLine(currentLine, insertColumn + pos - currentLineStart);
                insertColumn = 0;
            }

            currentLine++;
            l = plainKateTextLine(currentLine);

            if (block) {
                if (currentLine == lastLine() + 1) {
                    editInsertLine(currentLine, QString());
                }
                insertColumn = positionColumnExpanded;
                if (l) {
                    insertColumn = l->fromVirtualColumn(insertColumn, tabWidth);
                }
            }

            currentLineStart = pos + 1;
            if (l) {
                insertColumnExpanded = l->toVirtualColumn(insertColumn, tabWidth);
            }
        }
    }

    // Only perform the text insert if there is text to insert
    if (currentLineStart < pos) {
        editInsertText(currentLine, insertColumn, text.mid(currentLineStart, pos - currentLineStart));
    }

    editEnd();
    return true;
}

bool KTextEditor::DocumentPrivate::insertText(const KTextEditor::Cursor &position, const QStringList &textLines, bool block)
{
    if (!isReadWrite()) {
        return false;
    }

    // just reuse normal function
    return insertText(position, textLines.join(QStringLiteral("\n")), block);
}

bool KTextEditor::DocumentPrivate::removeText(const KTextEditor::Range &_range, bool block)
{
    KTextEditor::Range range = _range;

    if (!isReadWrite()) {
        return false;
    }

    // Should now be impossible to trigger with the new Range class
    Q_ASSERT(range.start().line() <= range.end().line());

    if (range.start().line() > lastLine()) {
        return false;
    }

    if (!block) {
        emit aboutToRemoveText(range);
    }

    editStart();

    if (!block) {
        if (range.end().line() > lastLine()) {
            range.setEnd(KTextEditor::Cursor(lastLine() + 1, 0));
        }

        if (range.onSingleLine()) {
            editRemoveText(range.start().line(), range.start().column(), range.columnWidth());
        } else {
            int from = range.start().line();
            int to = range.end().line();

            // remove last line
            if (to <= lastLine()) {
                editRemoveText(to, 0, range.end().column());
            }

            // editRemoveLines() will be called on first line (to remove bookmark)
            if (range.start().column() == 0 && from > 0) {
                --from;
            }

            // remove middle lines
            editRemoveLines(from + 1, to - 1);

            // remove first line if not already removed by editRemoveLines()
            if (range.start().column() > 0 || range.start().line() == 0) {
                editRemoveText(from, range.start().column(), m_buffer->plainLine(from)->length() - range.start().column());
                editUnWrapLine(from);
            }
        }
    } // if ( ! block )
    else {
        int startLine = qMax(0, range.start().line());
        int vc1 = toVirtualColumn(range.start());
        int vc2 = toVirtualColumn(range.end());
        for (int line = qMin(range.end().line(), lastLine()); line >= startLine; --line) {
            int col1 = fromVirtualColumn(line, vc1);
            int col2 = fromVirtualColumn(line, vc2);
            editRemoveText(line, qMin(col1, col2), qAbs(col2 - col1));
        }
    }

    editEnd();
    return true;
}

bool KTextEditor::DocumentPrivate::insertLine(int l, const QString &str)
{
    if (!isReadWrite()) {
        return false;
    }

    if (l < 0 || l > lines()) {
        return false;
    }

    return editInsertLine(l, str);
}

bool KTextEditor::DocumentPrivate::insertLines(int line, const QStringList &text)
{
    if (!isReadWrite()) {
        return false;
    }

    if (line < 0 || line > lines()) {
        return false;
    }

    bool success = true;
    foreach (const QString &string, text) {
        success &= editInsertLine(line++, string);
    }

    return success;
}

bool KTextEditor::DocumentPrivate::removeLine(int line)
{
    if (!isReadWrite()) {
        return false;
    }

    if (line < 0 || line > lastLine()) {
        return false;
    }

    return editRemoveLine(line);
}

int KTextEditor::DocumentPrivate::totalCharacters() const
{
    int l = 0;

    for (int i = 0; i < m_buffer->count(); ++i) {
        Kate::TextLine line = m_buffer->plainLine(i);

        if (line) {
            l += line->length();
        }
    }

    return l;
}

int KTextEditor::DocumentPrivate::lines() const
{
    return m_buffer->count();
}

int KTextEditor::DocumentPrivate::lineLength(int line) const
{
    if (line < 0 || line > lastLine()) {
        return -1;
    }

    Kate::TextLine l = m_buffer->plainLine(line);

    if (!l) {
        return -1;
    }

    return l->length();
}

bool KTextEditor::DocumentPrivate::isLineModified(int line) const
{
    if (line < 0 || line >= lines()) {
        return false;
    }

    Kate::TextLine l = m_buffer->plainLine(line);
    Q_ASSERT(l);

    return l->markedAsModified();
}

bool KTextEditor::DocumentPrivate::isLineSaved(int line) const
{
    if (line < 0 || line >= lines()) {
        return false;
    }

    Kate::TextLine l = m_buffer->plainLine(line);
    Q_ASSERT(l);

    return l->markedAsSavedOnDisk();
}

bool KTextEditor::DocumentPrivate::isLineTouched(int line) const
{
    if (line < 0 || line >= lines()) {
        return false;
    }

    Kate::TextLine l = m_buffer->plainLine(line);
    Q_ASSERT(l);

    return l->markedAsModified() || l->markedAsSavedOnDisk();
}
//END

//BEGIN KTextEditor::EditInterface internal stuff
//
// Starts an edit session with (or without) undo, update of view disabled during session
//
bool KTextEditor::DocumentPrivate::editStart()
{
    editSessionNumber++;

    if (editSessionNumber > 1) {
        return false;
    }

    editIsRunning = true;

    m_undoManager->editStart();

    foreach (KTextEditor::ViewPrivate *view, m_views) {
        view->editStart();
    }

    m_buffer->editStart();
    return true;
}

//
// End edit session and update Views
//
bool KTextEditor::DocumentPrivate::editEnd()
{
    if (editSessionNumber == 0) {
        Q_ASSERT(0);
        return false;
    }

    // wrap the new/changed text, if something really changed!
    if (m_buffer->editChanged() && (editSessionNumber == 1))
        if (m_undoManager->isActive() && config()->wordWrap()) {
            wrapText(m_buffer->editTagStart(), m_buffer->editTagEnd());
        }

    editSessionNumber--;

    if (editSessionNumber > 0) {
        return false;
    }

    // end buffer edit, will trigger hl update
    // this will cause some possible adjustment of tagline start/end
    m_buffer->editEnd();

    m_undoManager->editEnd();

    // edit end for all views !!!!!!!!!
    foreach (KTextEditor::ViewPrivate *view, m_views) {
        view->editEnd(m_buffer->editTagStart(), m_buffer->editTagEnd(), m_buffer->editTagFrom());
    }

    if (m_buffer->editChanged()) {
        setModified(true);
        emit textChanged(this);
    }

    editIsRunning = false;
    return true;
}

void KTextEditor::DocumentPrivate::pushEditState()
{
    editStateStack.push(editSessionNumber);
}

void KTextEditor::DocumentPrivate::popEditState()
{
    if (editStateStack.isEmpty()) {
        return;
    }

    int count = editStateStack.pop() - editSessionNumber;
    while (count < 0) {
        ++count;
        editEnd();
    }
    while (count > 0) {
        --count;
        editStart();
    }
}

void KTextEditor::DocumentPrivate::inputMethodStart()
{
    m_undoManager->inputMethodStart();
}

void KTextEditor::DocumentPrivate::inputMethodEnd()
{
    m_undoManager->inputMethodEnd();
}

bool KTextEditor::DocumentPrivate::wrapText(int startLine, int endLine)
{
    if (startLine < 0 || endLine < 0) {
        return false;
    }

    if (!isReadWrite()) {
        return false;
    }

    int col = config()->wordWrapAt();

    if (col == 0) {
        return false;
    }

    editStart();

    for (int line = startLine; (line <= endLine) && (line < lines()); line++) {
        Kate::TextLine l = kateTextLine(line);

        if (!l) {
            break;
        }

        //qCDebug(LOG_KTE) << "try wrap line: " << line;

        if (l->virtualLength(m_buffer->tabWidth()) > col) {
            Kate::TextLine nextl = kateTextLine(line + 1);

            //qCDebug(LOG_KTE) << "do wrap line: " << line;

            int eolPosition = l->length() - 1;

            // take tabs into account here, too
            int x = 0;
            const QString &t = l->string();
            int z2 = 0;
            for (; z2 < l->length(); z2++) {
                static const QChar tabChar(QLatin1Char('\t'));
                if (t.at(z2) == tabChar) {
                    x += m_buffer->tabWidth() - (x % m_buffer->tabWidth());
                } else {
                    x++;
                }

                if (x > col) {
                    break;
                }
            }

            const int colInChars = qMin(z2, l->length() - 1);
            int searchStart = colInChars;

            // If where we are wrapping is an end of line and is a space we don't
            // want to wrap there
            if (searchStart == eolPosition && t.at(searchStart).isSpace()) {
                searchStart--;
            }

            // Scan backwards looking for a place to break the line
            // We are not interested in breaking at the first char
            // of the line (if it is a space), but we are at the second
            // anders: if we can't find a space, try breaking on a word
            // boundary, using KateHighlight::canBreakAt().
            // This could be a priority (setting) in the hl/filetype/document
            int z = -1;
            int nw = -1; // alternative position, a non word character
            for (z = searchStart; z >= 0; z--) {
                if (t.at(z).isSpace()) {
                    break;
                }
                if ((nw < 0) && highlight()->canBreakAt(t.at(z), l->attribute(z))) {
                    nw = z;
                }
            }

            if (z >= 0) {
                // So why don't we just remove the trailing space right away?
                // Well, the (view's) cursor may be directly in front of that space
                // (user typing text before the last word on the line), and if that
                // happens, the cursor would be moved to the next line, which is not
                // what we want (bug #106261)
                z++;
            } else {
                // There was no space to break at so break at a nonword character if
                // found, or at the wrapcolumn ( that needs be configurable )
                // Don't try and add any white space for the break
                if ((nw >= 0) && nw < colInChars) {
                    nw++;    // break on the right side of the character
                }
                z = (nw >= 0) ? nw : colInChars;
            }

            if (nextl && !nextl->isAutoWrapped()) {
                editWrapLine(line, z, true);
                editMarkLineAutoWrapped(line + 1, true);

                endLine++;
            } else {
                if (nextl && (nextl->length() > 0) && !nextl->at(0).isSpace() && ((l->length() < 1) || !l->at(l->length() - 1).isSpace())) {
                    editInsertText(line + 1, 0, QLatin1String(" "));
                }

                bool newLineAdded = false;
                editWrapLine(line, z, false, &newLineAdded);

                editMarkLineAutoWrapped(line + 1, true);

                endLine++;
            }
        }
    }

    editEnd();

    return true;
}

bool KTextEditor::DocumentPrivate::editInsertText(int line, int col, const QString &s)
{
    // verbose debug
    EDIT_DEBUG << "editInsertText" << line << col << s;

    if (line < 0 || col < 0) {
        return false;
    }

    if (!isReadWrite()) {
        return false;
    }

    Kate::TextLine l = kateTextLine(line);

    if (!l) {
        return false;
    }

    // nothing to do, do nothing!
    if (s.isEmpty()) {
        return true;
    }

    editStart();

    QString s2 = s;
    int col2 = col;
    if (col2 > l->length()) {
        s2 = QString(col2 - l->length(), QLatin1Char(' ')) + s;
        col2 = l->length();
    }

    m_undoManager->slotTextInserted(line, col2, s2);

    // insert text into line
    m_buffer->insertText(KTextEditor::Cursor(line, col2), s2);

    emit textInserted(this, KTextEditor::Range(line, col2, line, col2 + s2.length()));

    editEnd();

    return true;
}

bool KTextEditor::DocumentPrivate::editRemoveText(int line, int col, int len)
{
    // verbose debug
    EDIT_DEBUG << "editRemoveText" << line << col << len;

    if (line < 0 || col < 0 || len < 0) {
        return false;
    }

    if (!isReadWrite()) {
        return false;
    }

    Kate::TextLine l = kateTextLine(line);

    if (!l) {
        return false;
    }

    // nothing to do, do nothing!
    if (len == 0) {
        return true;
    }

    // wrong column
    if (col >= l->text().size()) {
        return false;
    }

    // don't try to remove what's not there
    len = qMin(len, l->text().size() - col);

    editStart();

    QString oldText = l->string().mid(col, len);

    m_undoManager->slotTextRemoved(line, col, oldText);

    // remove text from line
    m_buffer->removeText(KTextEditor::Range(KTextEditor::Cursor(line, col), KTextEditor::Cursor(line, col + len)));

    emit textRemoved(this, KTextEditor::Range(line, col, line, col + len), oldText);

    editEnd();

    return true;
}

bool KTextEditor::DocumentPrivate::editMarkLineAutoWrapped(int line, bool autowrapped)
{
    // verbose debug
    EDIT_DEBUG << "editMarkLineAutoWrapped" << line << autowrapped;

    if (line < 0) {
        return false;
    }

    if (!isReadWrite()) {
        return false;
    }

    Kate::TextLine l = kateTextLine(line);

    if (!l) {
        return false;
    }

    editStart();

    m_undoManager->slotMarkLineAutoWrapped(line, autowrapped);

    l->setAutoWrapped(autowrapped);

    editEnd();

    return true;
}

bool KTextEditor::DocumentPrivate::editWrapLine(int line, int col, bool newLine, bool *newLineAdded)
{
    // verbose debug
    EDIT_DEBUG << "editWrapLine" << line << col << newLine;

    if (line < 0 || col < 0) {
        return false;
    }

    if (!isReadWrite()) {
        return false;
    }

    Kate::TextLine l = kateTextLine(line);

    if (!l) {
        return false;
    }

    editStart();

    Kate::TextLine nextLine = kateTextLine(line + 1);

    const int length = l->length();
    m_undoManager->slotLineWrapped(line, col, length - col, (!nextLine || newLine));

    if (!nextLine || newLine) {
        m_buffer->wrapLine(KTextEditor::Cursor(line, col));

        QList<KTextEditor::Mark *> list;
        for (QHash<int, KTextEditor::Mark *>::const_iterator i = m_marks.constBegin(); i != m_marks.constEnd(); ++i) {
            if (i.value()->line >= line) {
                if ((col == 0) || (i.value()->line > line)) {
                    list.append(i.value());
                }
            }
        }

        for (int i = 0; i < list.size(); ++i) {
            m_marks.take(list.at(i)->line);
        }

        for (int i = 0; i < list.size(); ++i) {
            list.at(i)->line++;
            m_marks.insert(list.at(i)->line, list.at(i));
        }

        if (!list.isEmpty()) {
            emit marksChanged(this);
        }

        // yes, we added a new line !
        if (newLineAdded) {
            (*newLineAdded) = true;
        }
    } else {
        m_buffer->wrapLine(KTextEditor::Cursor(line, col));
        m_buffer->unwrapLine(line + 2);

        // no, no new line added !
        if (newLineAdded) {
            (*newLineAdded) = false;
        }
    }

    emit textInserted(this, KTextEditor::Range(line, col, line + 1, 0));

    editEnd();

    return true;
}

bool KTextEditor::DocumentPrivate::editUnWrapLine(int line, bool removeLine, int length)
{
    // verbose debug
    EDIT_DEBUG << "editUnWrapLine" << line << removeLine << length;

    if (line < 0 || length < 0) {
        return false;
    }

    if (!isReadWrite()) {
        return false;
    }

    Kate::TextLine l = kateTextLine(line);
    Kate::TextLine nextLine = kateTextLine(line + 1);

    if (!l || !nextLine) {
        return false;
    }

    editStart();

    int col = l->length();

    m_undoManager->slotLineUnWrapped(line, col, length, removeLine);

    if (removeLine) {
        m_buffer->unwrapLine(line + 1);
    } else {
        m_buffer->wrapLine(KTextEditor::Cursor(line + 1, length));
        m_buffer->unwrapLine(line + 1);
    }

    QList<KTextEditor::Mark *> list;
    for (QHash<int, KTextEditor::Mark *>::const_iterator i = m_marks.constBegin(); i != m_marks.constEnd(); ++i) {
        if (i.value()->line >= line + 1) {
            list.append(i.value());
        }

        if (i.value()->line == line + 1) {
            KTextEditor::Mark *mark = m_marks.take(line);

            if (mark) {
                i.value()->type |= mark->type;
            }
        }
    }

    for (int i = 0; i < list.size(); ++i) {
        m_marks.take(list.at(i)->line);
    }

    for (int i = 0; i < list.size(); ++i) {
        list.at(i)->line--;
        m_marks.insert(list.at(i)->line, list.at(i));
    }

    if (!list.isEmpty()) {
        emit marksChanged(this);
    }

    emit textRemoved(this, KTextEditor::Range(line, col, line + 1, 0), QStringLiteral("\n"));

    editEnd();

    return true;
}

bool KTextEditor::DocumentPrivate::editInsertLine(int line, const QString &s)
{
    // verbose debug
    EDIT_DEBUG << "editInsertLine" << line << s;

    if (line < 0) {
        return false;
    }

    if (!isReadWrite()) {
        return false;
    }

    if (line > lines()) {
        return false;
    }

    editStart();

    m_undoManager->slotLineInserted(line, s);

    // wrap line
    if (line > 0) {
        Kate::TextLine previousLine = m_buffer->line(line - 1);
        m_buffer->wrapLine(KTextEditor::Cursor(line - 1, previousLine->text().size()));
    } else {
        m_buffer->wrapLine(KTextEditor::Cursor(0, 0));
    }

    // insert text
    m_buffer->insertText(KTextEditor::Cursor(line, 0), s);

    Kate::TextLine tl = m_buffer->line(line);

    QList<KTextEditor::Mark *> list;
    for (QHash<int, KTextEditor::Mark *>::const_iterator i = m_marks.constBegin(); i != m_marks.constEnd(); ++i) {
        if (i.value()->line >= line) {
            list.append(i.value());
        }
    }

    for (int i = 0; i < list.size(); ++i) {
        m_marks.take(list.at(i)->line);
    }

    for (int i = 0; i < list.size(); ++i) {
        list.at(i)->line++;
        m_marks.insert(list.at(i)->line, list.at(i));
    }

    if (!list.isEmpty()) {
        emit marksChanged(this);
    }

    KTextEditor::Range rangeInserted(line, 0, line, tl->length());

    if (line) {
        Kate::TextLine prevLine = plainKateTextLine(line - 1);
        rangeInserted.setStart(KTextEditor::Cursor(line - 1, prevLine->length()));
    } else {
        rangeInserted.setEnd(KTextEditor::Cursor(line + 1, 0));
    }

    emit textInserted(this, rangeInserted);

    editEnd();

    return true;
}

bool KTextEditor::DocumentPrivate::editRemoveLine(int line)
{
    return editRemoveLines(line, line);
}

bool KTextEditor::DocumentPrivate::editRemoveLines(int from, int to)
{
    // verbose debug
    EDIT_DEBUG << "editRemoveLines" << from << to;

    if (to < from || from < 0 || to > lastLine()) {
        return false;
    }

    if (!isReadWrite()) {
        return false;
    }

    if (lines() == 1) {
        return editRemoveText(0, 0, kateTextLine(0)->length());
    }

    editStart();
    QStringList oldText;

    /**
     * first remove text
     */
    for (int line = to; line >= from; --line) {
        Kate::TextLine tl = m_buffer->line(line);
        oldText.prepend(this->line(line));
        m_undoManager->slotLineRemoved(line, this->line(line));

        m_buffer->removeText(KTextEditor::Range(KTextEditor::Cursor(line, 0), KTextEditor::Cursor(line, tl->text().size())));
    }

    /**
     * then collapse lines
     */
    for (int line = to; line >= from; --line) {
        /**
         * unwrap all lines, prefer to unwrap line behind, skip to wrap line 0
         */
        if (line + 1 < m_buffer->lines()) {
            m_buffer->unwrapLine(line + 1);
        } else if (line) {
            m_buffer->unwrapLine(line);
        }
    }

    QList<int> rmark;
    QList<int> list;

    foreach (KTextEditor::Mark *mark, m_marks) {
        int line = mark->line;
        if (line > to) {
            list << line;
        } else if (line >= from) {
            rmark << line;
        }
    }

    foreach (int line, rmark) {
        delete m_marks.take(line);
    }

    foreach (int line, list) {
        KTextEditor::Mark *mark = m_marks.take(line);
        mark->line -= to - from + 1;
        m_marks.insert(mark->line, mark);
    }

    if (!list.isEmpty()) {
        emit marksChanged(this);
    }

    KTextEditor::Range rangeRemoved(from, 0, to + 1, 0);

    if (to == lastLine() + to - from + 1) {
        rangeRemoved.setEnd(KTextEditor::Cursor(to, oldText.last().length()));
        if (from > 0) {
            Kate::TextLine prevLine = plainKateTextLine(from - 1);
            rangeRemoved.setStart(KTextEditor::Cursor(from - 1, prevLine->length()));
        }
    }

    emit textRemoved(this, rangeRemoved, oldText.join(QStringLiteral("\n")) + QLatin1Char('\n'));

    editEnd();

    return true;
}
//END

//BEGIN KTextEditor::UndoInterface stuff
uint KTextEditor::DocumentPrivate::undoCount() const
{
    return m_undoManager->undoCount();
}

uint KTextEditor::DocumentPrivate::redoCount() const
{
    return m_undoManager->redoCount();
}

void KTextEditor::DocumentPrivate::undo()
{
    m_undoManager->undo();
}

void KTextEditor::DocumentPrivate::redo()
{
    m_undoManager->redo();
}
//END

//BEGIN KTextEditor::SearchInterface stuff
QVector<KTextEditor::Range> KTextEditor::DocumentPrivate::searchText(
    const KTextEditor::Range &range,
    const QString &pattern,
    const KTextEditor::SearchOptions options) const
{
    const bool escapeSequences =  options.testFlag(KTextEditor::EscapeSequences);
    const bool regexMode       =  options.testFlag(KTextEditor::Regex);
    const bool backwards       =  options.testFlag(KTextEditor::Backwards);
    const bool wholeWords      =  options.testFlag(KTextEditor::WholeWords);
    const Qt::CaseSensitivity caseSensitivity = options.testFlag(KTextEditor::CaseInsensitive) ? Qt::CaseInsensitive : Qt::CaseSensitive;

    if (regexMode) {
        // regexp search
        // escape sequences are supported by definition
        KateRegExpSearch searcher(this, caseSensitivity);
        return searcher.search(pattern, range, backwards);
    }

    if (escapeSequences) {
        // escaped search
        KatePlainTextSearch searcher(this, caseSensitivity, wholeWords);
        KTextEditor::Range match = searcher.search(KateRegExpSearch::escapePlaintext(pattern), range, backwards);

        QVector<KTextEditor::Range> result;
        result.append(match);
        return result;
    }

    // plaintext search
    KatePlainTextSearch searcher(this, caseSensitivity, wholeWords);
    KTextEditor::Range match = searcher.search(pattern, range, backwards);

    QVector<KTextEditor::Range> result;
    result.append(match);
    return result;
}
//END

QWidget *KTextEditor::DocumentPrivate::dialogParent()
{
    QWidget *w = widget();

    if (!w) {
        w = activeView();

        if (!w) {
            w = QApplication::activeWindow();
        }
    }

    return w;
}

//BEGIN KTextEditor::HighlightingInterface stuff
bool KTextEditor::DocumentPrivate::setMode(const QString &name)
{
    updateFileType(name);
    return true;
}

KTextEditor::DefaultStyle KTextEditor::DocumentPrivate::defaultStyleAt(const KTextEditor::Cursor &position) const
{
    // TODO, FIXME KDE5: in surrogate, use 2 bytes before
    if (! isValidTextPosition(position)) {
        return dsNormal;
    }

    int ds = const_cast<KTextEditor::DocumentPrivate*>(this)->
        defStyleNum(position.line(), position.column());
    if (ds < 0 || ds > static_cast<int>(dsError)) {
        return dsNormal;
    }

    return static_cast<DefaultStyle>(ds);
}

QString KTextEditor::DocumentPrivate::mode() const
{
    return m_fileType;
}

QStringList KTextEditor::DocumentPrivate::modes() const
{
    QStringList m;

    const QList<KateFileType *> &modeList = KTextEditor::EditorPrivate::self()->modeManager()->list();
    foreach (KateFileType *type, modeList) {
        m << type->name;
    }

    return m;
}

bool KTextEditor::DocumentPrivate::setHighlightingMode(const QString &name)
{
    int mode = KateHlManager::self()->nameFind(name);
    if (mode == -1) {
        return false;
    }
    m_buffer->setHighlight(mode);
    return true;
}

QString KTextEditor::DocumentPrivate::highlightingMode() const
{
    return highlight()->name();
}

QStringList KTextEditor::DocumentPrivate::highlightingModes() const
{
    QStringList hls;

    for (int i = 0; i < KateHlManager::self()->highlights(); ++i) {
        hls << KateHlManager::self()->hlName(i);
    }

    return hls;
}

QString KTextEditor::DocumentPrivate::highlightingModeSection(int index) const
{
    return KateHlManager::self()->hlSection(index);
}

QString KTextEditor::DocumentPrivate::modeSection(int index) const
{
    return KTextEditor::EditorPrivate::self()->modeManager()->list().at(index)->section;
}

void KTextEditor::DocumentPrivate::bufferHlChanged()
{
    // update all views
    makeAttribs(false);

    // deactivate indenter if necessary
    m_indenter->checkRequiredStyle();

    emit highlightingModeChanged(this);
}

void KTextEditor::DocumentPrivate::setDontChangeHlOnSave()
{
    m_hlSetByUser = true;
}

void KTextEditor::DocumentPrivate::bomSetByUser()
{
    m_bomSetByUser = true;
}
//END

//BEGIN KTextEditor::SessionConfigInterface and KTextEditor::ParameterizedSessionConfigInterface stuff
void KTextEditor::DocumentPrivate::readSessionConfig(const KConfigGroup &kconfig, const QSet<QString> &flags)
{
    if (!flags.contains(QStringLiteral("SkipEncoding"))) {
        // get the encoding
        QString tmpenc = kconfig.readEntry("Encoding");
        if (!tmpenc.isEmpty() && (tmpenc != encoding())) {
            setEncoding(tmpenc);
        }
    }

    if (!flags.contains(QStringLiteral("SkipUrl"))) {
        // restore the url
        QUrl url(kconfig.readEntry("URL"));

        // open the file if url valid
        if (!url.isEmpty() && url.isValid()) {
            openUrl(url);
        } else {
            completed();    //perhaps this should be emitted at the end of this function
        }
    } else {
        completed(); //perhaps this should be emitted at the end of this function
    }

    if (!flags.contains(QStringLiteral("SkipMode"))) {
        // restore the filetype
        if (kconfig.hasKey("Mode")) {
            updateFileType(kconfig.readEntry("Mode", fileType()));
        }
    }

    if (!flags.contains(QStringLiteral("SkipHighlighting"))) {
        // restore the hl stuff
        if (kconfig.hasKey("Highlighting")) {
            const int mode = KateHlManager::self()->nameFind(kconfig.readEntry("Highlighting"));
            if (mode >= 0) {
                m_buffer->setHighlight(mode);

                // restore if set by user, too! see bug 332605, otherwise we loose the hl later again on save
                m_hlSetByUser = kconfig.readEntry("Highlighting Set By User", false);
            }
        }
    }

    // indent mode
    config()->setIndentationMode(kconfig.readEntry("Indentation Mode", config()->indentationMode()));

    // Restore Bookmarks
    const QList<int> marks = kconfig.readEntry("Bookmarks", QList<int>());
    for (int i = 0; i < marks.count(); i++) {
        addMark(marks.at(i), KTextEditor::DocumentPrivate::markType01);
    }
}

void KTextEditor::DocumentPrivate::writeSessionConfig(KConfigGroup &kconfig, const QSet<QString> &flags)
{
    if (this->url().isLocalFile()) {
        const QString path = this->url().toLocalFile();
        if (path.startsWith(QDir::tempPath())) {
            return; // inside tmp resource, do not save
        }
    }

    if (!flags.contains(QStringLiteral("SkipUrl"))) {
        // save url
        kconfig.writeEntry("URL", this->url().toString());
    }

    if (!flags.contains(QStringLiteral("SkipEncoding"))) {
        // save encoding
        kconfig.writeEntry("Encoding", encoding());
    }

    if (!flags.contains(QStringLiteral("SkipMode"))) {
        // save file type
        kconfig.writeEntry("Mode", m_fileType);
    }

    if (!flags.contains(QStringLiteral("SkipHighlighting"))) {
        // save hl
        kconfig.writeEntry("Highlighting", highlight()->name());

        // save if set by user, too! see bug 332605, otherwise we loose the hl later again on save
        kconfig.writeEntry("Highlighting Set By User", m_hlSetByUser);
    }

    // indent mode
    kconfig.writeEntry("Indentation Mode", config()->indentationMode());

    // Save Bookmarks
    QList<int> marks;
    for (QHash<int, KTextEditor::Mark *>::const_iterator i = m_marks.constBegin(); i != m_marks.constEnd(); ++i)
        if (i.value()->type & KTextEditor::MarkInterface::markType01) {
            marks << i.value()->line;
        }

    kconfig.writeEntry("Bookmarks", marks);
}

//END KTextEditor::SessionConfigInterface and KTextEditor::ParameterizedSessionConfigInterface stuff

uint KTextEditor::DocumentPrivate::mark(int line)
{
    KTextEditor::Mark *m = m_marks.value(line);
    if (!m) {
        return 0;
    }

    return m->type;
}

void KTextEditor::DocumentPrivate::setMark(int line, uint markType)
{
    clearMark(line);
    addMark(line, markType);
}

void KTextEditor::DocumentPrivate::clearMark(int line)
{
    if (line < 0 || line > lastLine()) {
        return;
    }

    if (!m_marks.value(line)) {
        return;
    }

    KTextEditor::Mark *mark = m_marks.take(line);
    emit markChanged(this, *mark, MarkRemoved);
    emit marksChanged(this);
    delete mark;
    tagLines(line, line);
    repaintViews(true);
}

void KTextEditor::DocumentPrivate::addMark(int line, uint markType)
{
    KTextEditor::Mark *mark;

    if (line < 0 || line > lastLine()) {
        return;
    }

    if (markType == 0) {
        return;
    }

    if ((mark = m_marks.value(line))) {
        // Remove bits already set
        markType &= ~mark->type;

        if (markType == 0) {
            return;
        }

        // Add bits
        mark->type |= markType;
    } else {
        mark = new KTextEditor::Mark;
        mark->line = line;
        mark->type = markType;
        m_marks.insert(line, mark);
    }

    // Emit with a mark having only the types added.
    KTextEditor::Mark temp;
    temp.line = line;
    temp.type = markType;
    emit markChanged(this, temp, MarkAdded);

    emit marksChanged(this);
    tagLines(line, line);
    repaintViews(true);
}

void KTextEditor::DocumentPrivate::removeMark(int line, uint markType)
{
    if (line < 0 || line > lastLine()) {
        return;
    }

    KTextEditor::Mark *mark = m_marks.value(line);

    if (!mark) {
        return;
    }

    // Remove bits not set
    markType &= mark->type;

    if (markType == 0) {
        return;
    }

    // Subtract bits
    mark->type &= ~markType;

    // Emit with a mark having only the types removed.
    KTextEditor::Mark temp;
    temp.line = line;
    temp.type = markType;
    emit markChanged(this, temp, MarkRemoved);

    if (mark->type == 0) {
        m_marks.remove(line);
        delete mark;
    }

    emit marksChanged(this);
    tagLines(line, line);
    repaintViews(true);
}

const QHash<int, KTextEditor::Mark *> &KTextEditor::DocumentPrivate::marks()
{
    return m_marks;
}

void KTextEditor::DocumentPrivate::requestMarkTooltip(int line, QPoint position)
{
    KTextEditor::Mark *mark = m_marks.value(line);
    if (!mark) {
        return;
    }

    bool handled = false;
    emit markToolTipRequested(this, *mark, position, handled);
}

bool KTextEditor::DocumentPrivate::handleMarkClick(int line)
{
    KTextEditor::Mark *mark = m_marks.value(line);
    if (!mark) {
        return false;
    }

    bool handled = false;
    emit markClicked(this, *mark, handled);

    return handled;
}

bool KTextEditor::DocumentPrivate::handleMarkContextMenu(int line, QPoint position)
{
    KTextEditor::Mark *mark = m_marks.value(line);
    if (!mark) {
        return false;
    }

    bool handled = false;

    emit markContextMenuRequested(this, *mark, position, handled);

    return handled;
}

void KTextEditor::DocumentPrivate::clearMarks()
{
    while (!m_marks.isEmpty()) {
        QHash<int, KTextEditor::Mark *>::iterator it = m_marks.begin();
        KTextEditor::Mark mark = *it.value();
        delete it.value();
        m_marks.erase(it);

        emit markChanged(this, mark, MarkRemoved);
        tagLines(mark.line, mark.line);
    }

    m_marks.clear();

    emit marksChanged(this);
    repaintViews(true);
}

void KTextEditor::DocumentPrivate::setMarkPixmap(MarkInterface::MarkTypes type, const QPixmap &pixmap)
{
    m_markPixmaps.insert(type, pixmap);
}

void KTextEditor::DocumentPrivate::setMarkDescription(MarkInterface::MarkTypes type, const QString &description)
{
    m_markDescriptions.insert(type, description);
}

QPixmap KTextEditor::DocumentPrivate::markPixmap(MarkInterface::MarkTypes type) const
{
    return m_markPixmaps.value(type, QPixmap());
}

QColor KTextEditor::DocumentPrivate::markColor(MarkInterface::MarkTypes type) const
{
    uint reserved = (0x1 << KTextEditor::MarkInterface::reservedMarkersCount()) - 1;
    if ((uint)type >= (uint)markType01 && (uint)type <= reserved) {
        return KateRendererConfig::global()->lineMarkerColor(type);
    } else {
        return QColor();
    }
}

QString KTextEditor::DocumentPrivate::markDescription(MarkInterface::MarkTypes type) const
{
    return m_markDescriptions.value(type, QString());
}

void KTextEditor::DocumentPrivate::setEditableMarks(uint markMask)
{
    m_editableMarks = markMask;
}

uint KTextEditor::DocumentPrivate::editableMarks() const
{
    return m_editableMarks;
}
//END

//BEGIN KTextEditor::PrintInterface stuff
bool KTextEditor::DocumentPrivate::print()
{
    return KatePrinter::print(this);
}

void KTextEditor::DocumentPrivate::printPreview()
{
    KatePrinter::printPreview(this);
}
//END KTextEditor::PrintInterface stuff

//BEGIN KTextEditor::DocumentInfoInterface (### unfinished)
QString KTextEditor::DocumentPrivate::mimeType()
{
    /**
     * collect first 4k of text
     * only heuristic
     */
    QByteArray buf;
    for (int i = 0; (i < lines()) && (buf.size() <= 4096); ++i) {
        buf.append(line(i).toUtf8());
        buf.append('\n');
    }

    // use path of url, too, if set
    if (!url().path().isEmpty()) {
        return QMimeDatabase().mimeTypeForFileNameAndData(url().path(), buf).name();
    }

    // else only use the content
    return QMimeDatabase().mimeTypeForData(buf).name();
}
//END KTextEditor::DocumentInfoInterface

//BEGIN: error
void KTextEditor::DocumentPrivate::showAndSetOpeningErrorAccess()
{
    QPointer<KTextEditor::Message> message
        = new KTextEditor::Message(i18n("The file %1 could not be loaded, as it was not possible to read from it.<br />Check if you have read access to this file.", this->url().toDisplayString(QUrl::PreferLocalFile)),
                                   KTextEditor::Message::Error);
    message->setWordWrap(true);
    QAction *tryAgainAction = new QAction(QIcon::fromTheme(QStringLiteral("view-refresh")), i18nc("translators: you can also translate 'Try Again' with 'Reload'", "Try Again"), nullptr);
    connect(tryAgainAction, SIGNAL(triggered()), SLOT(documentReload()), Qt::QueuedConnection);

    QAction *closeAction = new QAction(QIcon::fromTheme(QStringLiteral("window-close")), i18n("&Close"), nullptr);
    closeAction->setToolTip(i18n("Close message"));

    // add try again and close actions
    message->addAction(tryAgainAction);
    message->addAction(closeAction);

    // finally post message
    postMessage(message);

    // remember error
    m_openingError = true;
    m_openingErrorMessage = i18n("The file %1 could not be loaded, as it was not possible to read from it.\n\nCheck if you have read access to this file.", this->url().toDisplayString(QUrl::PreferLocalFile));

}
//END: error


void KTextEditor::DocumentPrivate::openWithLineLengthLimitOverride()
{
    // raise line length limit to the next power of 2
    const int longestLine = m_buffer->longestLineLoaded();
    int newLimit = pow(2, ceil(log(longestLine) / log(2))); // TODO: C++11: use log2(x)
    if (newLimit <= longestLine) {
        newLimit *= 2;
    }

    // do the raise
    config()->setLineLengthLimit(newLimit);

    // just reload
    m_buffer->clear();
    openFile();
    if (!m_openingError) {
        setReadWrite(true);
        m_readWriteStateBeforeLoading = true;
    }
}

int KTextEditor::DocumentPrivate::lineLengthLimit() const
{
    return config()->lineLengthLimit();
}

//BEGIN KParts::ReadWrite stuff
bool KTextEditor::DocumentPrivate::openFile()
{
    /**
     * we are about to invalidate all cursors/ranges/.. => m_buffer->openFile will do so
     */
    emit aboutToInvalidateMovingInterfaceContent(this);

    // no open errors until now...
    m_openingError = false;
    m_openingErrorMessage.clear ();

    // add new m_file to dirwatch
    activateDirWatch();

    // remember current encoding
    QString currentEncoding = encoding();

    //
    // mime type magic to get encoding right
    //
    QString mimeType = arguments().mimeType();
    int pos = mimeType.indexOf(QLatin1Char(';'));
    if (pos != -1 && !(m_reloading && m_userSetEncodingForNextReload)) {
        setEncoding(mimeType.mid(pos + 1));
    }

    // update file type, we do this here PRE-LOAD, therefore pass file name for reading from
    updateFileType(KTextEditor::EditorPrivate::self()->modeManager()->fileType(this, localFilePath()));

    // read dir config (if possible and wanted)
    // do this PRE-LOAD to get encoding info!
    readDirConfig();

    // perhaps we need to re-set again the user encoding
    if (m_reloading && m_userSetEncodingForNextReload && (currentEncoding != encoding())) {
        setEncoding(currentEncoding);
    }

    bool success = m_buffer->openFile(localFilePath(), (m_reloading && m_userSetEncodingForNextReload));

    //
    // yeah, success
    // read variables
    //
    if (success) {
        readVariables();
    }

    //
    // update views
    //
    foreach (KTextEditor::ViewPrivate *view, m_views) {
        // This is needed here because inserting the text moves the view's start position (it is a MovingCursor)
        view->setCursorPosition(KTextEditor::Cursor());
        view->updateView(true);
    }

    // Inform that the text has changed (required as we're not inside the usual editStart/End stuff)
    emit textChanged(this);
    emit loaded(this);

    //
    // to houston, we are not modified
    //
    if (m_modOnHd) {
        m_modOnHd = false;
        m_modOnHdReason = OnDiskUnmodified;
        m_prevModOnHdReason = OnDiskUnmodified;
        emit modifiedOnDisk(this, m_modOnHd, m_modOnHdReason);
    }

    //
    // display errors
    //
    if (!success) {
        showAndSetOpeningErrorAccess();
    }

    // warn: broken encoding
    if (m_buffer->brokenEncoding()) {
        // this file can't be saved again without killing it
        setReadWrite(false);
        m_readWriteStateBeforeLoading=false;
        QPointer<KTextEditor::Message> message
            = new KTextEditor::Message(i18n("The file %1 was opened with %2 encoding but contained invalid characters.<br />"
                                            "It is set to read-only mode, as saving might destroy its content.<br />"
                                            "Either reopen the file with the correct encoding chosen or enable the read-write mode again in the tools menu to be able to edit it.", this->url().toDisplayString(QUrl::PreferLocalFile),
                                            QString::fromLatin1(m_buffer->textCodec()->name())),
                                       KTextEditor::Message::Warning);
        message->setWordWrap(true);
        postMessage(message);

        // remember error
        m_openingError = true;
        m_openingErrorMessage = i18n("The file %1 was opened with %2 encoding but contained invalid characters."
                                    " It is set to read-only mode, as saving might destroy its content."
                                    " Either reopen the file with the correct encoding chosen or enable the read-write mode again in the tools menu to be able to edit it.", this->url().toDisplayString(QUrl::PreferLocalFile), QString::fromLatin1(m_buffer->textCodec()->name()));
    }

    // warn: too long lines
    if (m_buffer->tooLongLinesWrapped()) {
        // this file can't be saved again without modifications
        setReadWrite(false);
        m_readWriteStateBeforeLoading = false;
        QPointer<KTextEditor::Message> message
            = new KTextEditor::Message(i18n("The file %1 was opened and contained lines longer than the configured Line Length Limit (%2 characters).<br />"
                                            "The longest of those lines was %3 characters long<br/>"
                                            "Those lines were wrapped and the document is set to read-only mode, as saving will modify its content.",
                                            this->url().toDisplayString(QUrl::PreferLocalFile), config()->lineLengthLimit(), m_buffer->longestLineLoaded()),
                                       KTextEditor::Message::Warning);
        QAction *increaseAndReload = new QAction(i18n("Temporarily raise limit and reload file"), message);
        connect(increaseAndReload, SIGNAL(triggered()), this, SLOT(openWithLineLengthLimitOverride()));
        message->addAction(increaseAndReload, true);
        message->addAction(new QAction(i18n("Close"), message), true);
        message->setWordWrap(true);
        postMessage(message);

        // remember error
        m_openingError = true;
        m_openingErrorMessage = i18n("The file %1 was opened and contained lines longer than the configured Line Length Limit (%2 characters).<br/>"
                                     "The longest of those lines was %3 characters long<br/>"
                                     "Those lines were wrapped and the document is set to read-only mode, as saving will modify its content.", this->url().toDisplayString(QUrl::PreferLocalFile), config()->lineLengthLimit(),m_buffer->longestLineLoaded());
    }

    //
    // return the success
    //
    return success;
}

bool KTextEditor::DocumentPrivate::saveFile()
{
    // delete pending mod-on-hd message if applicable.
    delete m_modOnHdHandler;

    // some warnings, if file was changed by the outside!
    if (!url().isEmpty()) {
        if (m_fileChangedDialogsActivated && m_modOnHd) {
            QString str = reasonedMOHString() + QLatin1String("\n\n");

            if (!isModified()) {
                if (KMessageBox::warningContinueCancel(dialogParent(),
                                                       str + i18n("Do you really want to save this unmodified file? You could overwrite changed data in the file on disk."), i18n("Trying to Save Unmodified File"), KGuiItem(i18n("Save Nevertheless"))) != KMessageBox::Continue) {
                    return false;
                }
            } else {
                if (KMessageBox::warningContinueCancel(dialogParent(),
                                                       str + i18n("Do you really want to save this file? Both your open file and the file on disk were changed. There could be some data lost."), i18n("Possible Data Loss"), KGuiItem(i18n("Save Nevertheless"))) != KMessageBox::Continue) {
                    return false;
                }
            }
        }
    }

    //
    // can we encode it if we want to save it ?
    //
    if (!m_buffer->canEncode()
            && (KMessageBox::warningContinueCancel(dialogParent(),
                    i18n("The selected encoding cannot encode every Unicode character in this document. Do you really want to save it? There could be some data lost."), i18n("Possible Data Loss"), KGuiItem(i18n("Save Nevertheless"))) != KMessageBox::Continue)) {
        return false;
    }

    /**
     * create a backup file or abort if that fails!
     * if no backup file wanted, this routine will just return true
     */
    if (!createBackupFile())
        return false;

    // update file type, pass no file path, read file type content from this document
    updateFileType(KTextEditor::EditorPrivate::self()->modeManager()->fileType(this, QString()));

    // remember the oldpath...
    QString oldPath = m_dirWatchFile;

    // read dir config (if possible and wanted)
    if (url().isLocalFile()) {
        QFileInfo fo(oldPath), fn(localFilePath());

        if (fo.path() != fn.path()) {
            readDirConfig();
        }
    }

    // read our vars
    readVariables();

    // remove file from dirwatch
    deactivateDirWatch();

    // remove all trailing spaces in the document (as edit actions)
    // NOTE: we need this as edit actions, since otherwise the edit actions
    //       in the swap file recovery may happen at invalid cursor positions
    removeTrailingSpaces();

    //
    // try to save
    //
    if (!m_buffer->saveFile(localFilePath())) {
        // add m_file again to dirwatch
        activateDirWatch(oldPath);
        KMessageBox::error(dialogParent(), i18n("The document could not be saved, as it was not possible to write to %1.\n\nCheck that you have write access to this file or that enough disk space is available.", this->url().toDisplayString(QUrl::PreferLocalFile)));
        return false;
    }

    // update the checksum
    createDigest();

    // add m_file again to dirwatch
    activateDirWatch();

    //
    // we are not modified
    //
    if (m_modOnHd) {
        m_modOnHd = false;
        m_modOnHdReason = OnDiskUnmodified;
        m_prevModOnHdReason = OnDiskUnmodified;
        emit modifiedOnDisk(this, m_modOnHd, m_modOnHdReason);
    }

    // (dominik) mark last undo group as not mergeable, otherwise the next
    // edit action might be merged and undo will never stop at the saved state
    m_undoManager->undoSafePoint();
    m_undoManager->updateLineModifications();

    //
    // return success
    //
    return true;
}

bool KTextEditor::DocumentPrivate::createBackupFile()
{
    /**
     * backup for local or remote files wanted?
     */
    const bool backupLocalFiles = (config()->backupFlags() & KateDocumentConfig::LocalFiles);
    const bool backupRemoteFiles = (config()->backupFlags() & KateDocumentConfig::RemoteFiles);

    /**
     * early out, before mount check: backup wanted at all?
     * => if not, all fine, just return
     */
    if (!backupLocalFiles && !backupRemoteFiles) {
        return true;
    }

    /**
     * decide if we need backup based on locality
     * skip that, if we always want backups, as currentMountPoints is not that fast
     */
    QUrl u(url());
    bool needBackup = backupLocalFiles && backupRemoteFiles;
    if (!needBackup) {
        bool slowOrRemoteFile = !u.isLocalFile();
        if (!slowOrRemoteFile) {
            // could be a mounted remote filesystem (e.g. nfs, sshfs, cifs)
            // we have the early out above to skip this, if we want no backup, which is the default
            KMountPoint::Ptr mountPoint = KMountPoint::currentMountPoints().findByDevice(u.toLocalFile());
            slowOrRemoteFile = (mountPoint && mountPoint->probablySlow());
        }
        needBackup = (!slowOrRemoteFile && backupLocalFiles) || (slowOrRemoteFile && backupRemoteFiles);
    }

    /**
     * no backup needed? be done
     */
    if (!needBackup) {
        return true;
    }

    /**
     * else: try to backup
     */
    if (config()->backupPrefix().contains(QDir::separator())) {
        /**
         * replace complete path, as prefix is a path!
         */
        u.setPath(config()->backupPrefix() + u.fileName() + config()->backupSuffix());
    } else {
        /**
         * replace filename in url
         */
        const QString fileName = u.fileName();
        u = u.adjusted(QUrl::RemoveFilename);
        u.setPath(u.path() + config()->backupPrefix() + fileName + config()->backupSuffix());
    }

    qCDebug(LOG_KTE) << "backup src file name: " << url();
    qCDebug(LOG_KTE) << "backup dst file name: " << u;

    // handle the backup...
    bool backupSuccess = false;

    // local file mode, no kio
    if (u.isLocalFile()) {
        if (QFile::exists(url().toLocalFile())) {
            // first: check if backupFile is already there, if true, unlink it
            QFile backupFile(u.toLocalFile());
            if (backupFile.exists()) {
                backupFile.remove();
            }

            backupSuccess = QFile::copy(url().toLocalFile(), u.toLocalFile());
        } else {
            backupSuccess = true;
        }
    } else { // remote file mode, kio
        // get the right permissions, start with safe default
        KIO::StatJob *statJob = KIO::stat(url(), KIO::StatJob::SourceSide, 2);
        KJobWidgets::setWindow(statJob, QApplication::activeWindow());

        if (!statJob->exec()) {
            // do a evil copy which will overwrite target if possible
            KFileItem item(statJob->statResult(), url());
            KIO::FileCopyJob *job = KIO::file_copy(url(), u, item.permissions(), KIO::Overwrite);
            KJobWidgets::setWindow(job, QApplication::activeWindow());
            job->exec();
            backupSuccess = !job->error();
        } else {
            backupSuccess = true;
        }
    }

    // backup has failed, ask user how to proceed
    if (!backupSuccess && (KMessageBox::warningContinueCancel(dialogParent()
                            , i18n("For file %1 no backup copy could be created before saving."
                                    " If an error occurs while saving, you might lose the data of this file."
                                    " A reason could be that the media you write to is full or the directory of the file is read-only for you.", url().toDisplayString(QUrl::PreferLocalFile))
                            , i18n("Failed to create backup copy.")
                            , KGuiItem(i18n("Try to Save Nevertheless"))
                            , KStandardGuiItem::cancel(), QStringLiteral("Backup Failed Warning")) != KMessageBox::Continue)) {
        return false;
    }

    return true;
}

void KTextEditor::DocumentPrivate::readDirConfig()
{
    if (!url().isLocalFile()) {
        return;
    }

    /**
     * search .kateconfig upwards
     * with recursion guard
     */
    QSet<QString> seenDirectories;
    QDir dir (QFileInfo(localFilePath()).absolutePath());
    while (!seenDirectories.contains (dir.absolutePath ())) {
        /**
         * fill recursion guard
         */
        seenDirectories.insert (dir.absolutePath ());

        // try to open config file in this dir
        QFile f(dir.absolutePath () + QLatin1String("/.kateconfig"));
        if (f.open(QIODevice::ReadOnly)) {
            QTextStream stream(&f);

            uint linesRead = 0;
            QString line = stream.readLine();
            while ((linesRead < 32) && !line.isNull()) {
                readVariableLine(line);

                line = stream.readLine();

                linesRead++;
            }

            break;
        }

        /**
         * else: cd up, if possible or abort
         */
        if (!dir.cdUp()) {
            break;
        }
    }
}

void KTextEditor::DocumentPrivate::activateDirWatch(const QString &useFileName)
{
    QString fileToUse = useFileName;
    if (fileToUse.isEmpty()) {
        fileToUse = localFilePath();
    }

    QFileInfo fileInfo = QFileInfo(fileToUse);
    if (fileInfo.isSymLink()) {
        // Monitor the actual data and not the symlink
        fileToUse = fileInfo.canonicalFilePath();
    }

    // same file as we are monitoring, return
    if (fileToUse == m_dirWatchFile) {
        return;
    }

    // remove the old watched file
    deactivateDirWatch();

    // add new file if needed
    if (url().isLocalFile() && !fileToUse.isEmpty()) {
        KTextEditor::EditorPrivate::self()->dirWatch()->addFile(fileToUse);
        m_dirWatchFile = fileToUse;
    }
}

void KTextEditor::DocumentPrivate::deactivateDirWatch()
{
    if (!m_dirWatchFile.isEmpty()) {
        KTextEditor::EditorPrivate::self()->dirWatch()->removeFile(m_dirWatchFile);
    }

    m_dirWatchFile.clear();
}

bool KTextEditor::DocumentPrivate::openUrl(const QUrl &url)
{
    bool res = KTextEditor::Document::openUrl(normalizeUrl(url));
    updateDocName();
    return res;
}

bool KTextEditor::DocumentPrivate::closeUrl()
{
    //
    // file mod on hd
    //
    if (!m_reloading && !url().isEmpty()) {
        if (m_fileChangedDialogsActivated && m_modOnHd) {
            // make sure to not forget pending mod-on-hd handler
            delete m_modOnHdHandler;

            QWidget *parentWidget(dialogParent());
            if (!(KMessageBox::warningContinueCancel(
                        parentWidget,
                        reasonedMOHString() + QLatin1String("\n\n") + i18n("Do you really want to continue to close this file? Data loss may occur."),
                        i18n("Possible Data Loss"), KGuiItem(i18n("Close Nevertheless")), KStandardGuiItem::cancel(),
                        QStringLiteral("kate_close_modonhd_%1").arg(m_modOnHdReason)) == KMessageBox::Continue)) {
                /**
                 * reset reloading
                 */
                m_reloading = false;
                return false;
            }
        }
    }

    //
    // first call the normal kparts implementation
    //
    if (!KParts::ReadWritePart::closeUrl()) {
        /**
         * reset reloading
         */
        m_reloading = false;
        return false;
    }

    // Tell the world that we're about to go ahead with the close
    if (!m_reloading) {
        emit aboutToClose(this);
    }

    /**
     * delete all KTE::Messages
     */
    if (!m_messageHash.isEmpty()) {
        QList<KTextEditor::Message *> keys = m_messageHash.keys();
        foreach (KTextEditor::Message *message, keys) {
            delete message;
        }
    }

    /**
     * we are about to invalidate all cursors/ranges/.. => m_buffer->clear will do so
     */
    emit aboutToInvalidateMovingInterfaceContent(this);

    // remove file from dirwatch
    deactivateDirWatch();

    //
    // empty url + fileName
    //
    setUrl(QUrl());
    setLocalFilePath(QString());

    // we are not modified
    if (m_modOnHd) {
        m_modOnHd = false;
        m_modOnHdReason = OnDiskUnmodified;
        m_prevModOnHdReason = OnDiskUnmodified;
        emit modifiedOnDisk(this, m_modOnHd, m_modOnHdReason);
    }

    // remove all marks
    clearMarks();

    // clear the buffer
    m_buffer->clear();

    // clear undo/redo history
    m_undoManager->clearUndo();
    m_undoManager->clearRedo();

    // no, we are no longer modified
    setModified(false);

    // we have no longer any hl
    m_buffer->setHighlight(0);

    // update all our views
    foreach (KTextEditor::ViewPrivate *view, m_views) {
        view->clearSelection(); // fix bug #118588
        view->clear();
    }

    // purge swap file
    if (m_swapfile) {
        m_swapfile->fileClosed();
    }

    // success
    return true;
}

bool KTextEditor::DocumentPrivate::isDataRecoveryAvailable() const
{
    return m_swapfile && m_swapfile->shouldRecover();
}

void KTextEditor::DocumentPrivate::recoverData()
{
    if (isDataRecoveryAvailable()) {
        m_swapfile->recover();
    }
}

void KTextEditor::DocumentPrivate::discardDataRecovery()
{
    if (isDataRecoveryAvailable()) {
        m_swapfile->discard();
    }
}

void KTextEditor::DocumentPrivate::setReadWrite(bool rw)
{
    if (isReadWrite() == rw) {
        return;
    }

    KParts::ReadWritePart::setReadWrite(rw);

    foreach (KTextEditor::ViewPrivate *view, m_views) {
        view->slotUpdateUndo();
        view->slotReadWriteChanged();
    }

    emit readWriteChanged(this);
}

void KTextEditor::DocumentPrivate::setModified(bool m)
{
    if (isModified() != m) {
        KParts::ReadWritePart::setModified(m);

        foreach (KTextEditor::ViewPrivate *view, m_views) {
            view->slotUpdateUndo();
        }

        emit modifiedChanged(this);
    }

    m_undoManager->setModified(m);
}
//END

//BEGIN Kate specific stuff ;)

void KTextEditor::DocumentPrivate::makeAttribs(bool needInvalidate)
{
    foreach (KTextEditor::ViewPrivate *view, m_views) {
        view->renderer()->updateAttributes();
    }

    if (needInvalidate) {
        m_buffer->invalidateHighlighting();
    }

    foreach (KTextEditor::ViewPrivate *view, m_views) {
        view->tagAll();
        view->updateView(true);
    }
}

// the attributes of a hl have changed, update
void KTextEditor::DocumentPrivate::internalHlChanged()
{
    makeAttribs();
}

void KTextEditor::DocumentPrivate::addView(KTextEditor::View *view)
{
    Q_ASSERT (!m_views.contains(view));
    m_views.insert(view, static_cast<KTextEditor::ViewPrivate *>(view));

    // apply the view & renderer vars from the file type
    if (!m_fileType.isEmpty()) {
        readVariableLine(KTextEditor::EditorPrivate::self()->modeManager()->fileType(m_fileType).varLine, true);
    }

    // apply the view & renderer vars from the file
    readVariables(true);

    setActiveView(view);
}

void KTextEditor::DocumentPrivate::removeView(KTextEditor::View *view)
{
    Q_ASSERT (m_views.contains(view));
    m_views.remove(view);

    if (activeView() == view) {
        setActiveView(nullptr);
    }
}

void KTextEditor::DocumentPrivate::setActiveView(KTextEditor::View *view)
{
    if (m_activeView == view) {
        return;
    }

    m_activeView = static_cast<KTextEditor::ViewPrivate *>(view);
}

bool KTextEditor::DocumentPrivate::ownedView(KTextEditor::ViewPrivate *view)
{
    // do we own the given view?
    return (m_views.contains(view));
}

int KTextEditor::DocumentPrivate::toVirtualColumn(int line, int column) const
{
    Kate::TextLine textLine = m_buffer->plainLine(line);

    if (textLine) {
        return textLine->toVirtualColumn(column, config()->tabWidth());
    } else {
        return 0;
    }
}

int KTextEditor::DocumentPrivate::toVirtualColumn(const KTextEditor::Cursor &cursor) const
{
    return toVirtualColumn(cursor.line(), cursor.column());
}

int KTextEditor::DocumentPrivate::fromVirtualColumn(int line, int column) const
{
    Kate::TextLine textLine = m_buffer->plainLine(line);

    if (textLine) {
        return textLine->fromVirtualColumn(column, config()->tabWidth());
    } else {
        return 0;
    }
}

int KTextEditor::DocumentPrivate::fromVirtualColumn(const KTextEditor::Cursor &cursor) const
{
    return fromVirtualColumn(cursor.line(), cursor.column());
}

bool KTextEditor::DocumentPrivate::typeChars(KTextEditor::ViewPrivate *view, const QString &realChars)
{
    /**
     * filter out non-printable chars (convert to utf-32 to support surrogate pairs)
     */
    const auto realUcs4Chars = realChars.toUcs4();
    QVector<uint> ucs4Chars;
    Q_FOREACH (auto c, realUcs4Chars)
        if (QChar::isPrint(c) || c == QChar::fromLatin1('\t') || c == QChar::fromLatin1('\n') || c == QChar::fromLatin1('\r')) {
            ucs4Chars.append(c);
        }

    QString chars = QString::fromUcs4(ucs4Chars.data(), ucs4Chars.size());
    /**
     * no printable chars => nothing to insert!
     */
    if (chars.isEmpty()) {
        return false;
    }

    /**
     * auto bracket handling for newly inserted text
     * remember if we should auto add some
     */
    QChar closingBracket;
    if (view->config()->autoBrackets() && chars.size() == 1) {
        /**
         * we inserted a bracket?
         * => remember the matching closing one
         */
        closingBracket = matchingEndBracket(chars[0], true);

        /**
         * closing bracket for the autobracket we inserted earlier?
         */
        if ( m_currentAutobraceClosingChar == chars[0] && m_currentAutobraceRange ) {
            // do nothing
            m_currentAutobraceRange.reset(nullptr);
            view->cursorRight();
            return true;
        }
    }

    /**
     * selection around => special handling if we want to add auto brackets
     */
    if (view->selection() && !closingBracket.isNull()) {
        /**
         * add bracket at start + end of the selection
         */
        KTextEditor::Cursor oldCur = view->cursorPosition();
        insertText(view->selectionRange().start(), chars);
        view->slotTextInserted(view, oldCur, chars);

        view->setCursorPosition(view->selectionRange().end());
        oldCur = view->cursorPosition();
        insertText(view->selectionRange().end(), QString(closingBracket));
        view->slotTextInserted(view, oldCur, QString(closingBracket));

        /**
         * expand selection
         */
        view->setSelection(KTextEditor::Range(view->selectionRange().start() + Cursor{0, 1},
                                              view->cursorPosition() - Cursor{0, 1}));
        view->setCursorPosition(view->selectionRange().start());
    }

    /**
     * else normal handling
     */
    else {
        editStart();

        if (!view->config()->persistentSelection() && view->selection()) {
            view->removeSelectedText();
        }

        const KTextEditor::Cursor oldCur(view->cursorPosition());

        const bool multiLineBlockMode = view->blockSelection() && view->selection();
        if (view->currentInputMode()->overwrite()) {
            // blockmode multiline selection case: remove chars in every line
            const KTextEditor::Range selectionRange = view->selectionRange();
            const int startLine = multiLineBlockMode ? qMax(0, selectionRange.start().line()) : view->cursorPosition().line();
            const int endLine = multiLineBlockMode ? qMin(selectionRange.end().line(), lastLine()) : startLine;
            const int virtualColumn = toVirtualColumn(multiLineBlockMode ? selectionRange.end() : view->cursorPosition());

            for (int line = endLine; line >= startLine; --line) {
                Kate::TextLine textLine = m_buffer->plainLine(line);
                Q_ASSERT(textLine);
                const int column = fromVirtualColumn(line, virtualColumn);
                KTextEditor::Range r = KTextEditor::Range(KTextEditor::Cursor(line, column), qMin(chars.length(),
                    textLine->length() - column));

                // replace mode needs to know what was removed so it can be restored with backspace
                if (oldCur.column() < lineLength(line)) {
                    QChar removed = characterAt(KTextEditor::Cursor(line, column));
                    view->currentInputMode()->overwrittenChar(removed);
                }

                removeText(r);
            }
        }

        if (multiLineBlockMode) {
            KTextEditor::Range selectionRange = view->selectionRange();
            const int startLine = qMax(0, selectionRange.start().line());
            const int endLine = qMin(selectionRange.end().line(), lastLine());
            const int column = toVirtualColumn(selectionRange.end());
            for (int line = endLine; line >= startLine; --line) {
                editInsertText(line, fromVirtualColumn(line, column), chars);
            }
            int newSelectionColumn = toVirtualColumn(view->cursorPosition());
            selectionRange.setRange(KTextEditor::Cursor(selectionRange.start().line(), fromVirtualColumn(selectionRange.start().line(), newSelectionColumn))
                                    , KTextEditor::Cursor(selectionRange.end().line(), fromVirtualColumn(selectionRange.end().line(), newSelectionColumn)));
            view->setSelection(selectionRange);
        } else {
            chars = eventuallyReplaceTabs(view->cursorPosition(), chars);
            insertText(view->cursorPosition(), chars);
        }

        /**
         * auto bracket handling for newly inserted text
         * we inserted a bracket?
         * => add the matching closing one to the view + input chars
         * try to preserve the cursor position
         */
        bool skipAutobrace = closingBracket == QLatin1Char('\'');
        if ( highlight() && skipAutobrace ) {
            auto context = highlight()->contextForLocation(this, view->cursorPosition() - Cursor{0, 1});
            // skip adding ' in spellchecked areas, because those are text
            skipAutobrace = !context || highlight()->attributeRequiresSpellchecking(context->attr);
        }
        if (!closingBracket.isNull() && !skipAutobrace ) {
            // add bracket to the view
            const auto cursorPos(view->cursorPosition());
            const auto nextChar = view->document()->text({cursorPos, cursorPos + Cursor{0, 1}}).trimmed();
            if ( nextChar.isEmpty() || ! nextChar.at(0).isLetterOrNumber() ) {
                insertText(view->cursorPosition(), QString(closingBracket));
                const auto insertedAt(view->cursorPosition());
                view->setCursorPosition(cursorPos);
                m_currentAutobraceRange.reset(newMovingRange({cursorPos - Cursor{0, 1}, insertedAt},
                                                            KTextEditor::MovingRange::DoNotExpand));
                connect(view, &View::cursorPositionChanged,
                        this, &DocumentPrivate::checkCursorForAutobrace, Qt::UniqueConnection);

                // add bracket to chars inserted! needed for correct signals + indent
                chars.append(closingBracket);
            }
            m_currentAutobraceClosingChar = closingBracket;
        }

        // end edit session here, to have updated HL in userTypedChar!
        editEnd();

        // trigger indentation
        KTextEditor::Cursor b(view->cursorPosition());
        m_indenter->userTypedChar(view, b, chars.isEmpty() ? QChar() :  chars.at(chars.length() - 1));

        /**
         * inform the view about the original inserted chars
         */
        view->slotTextInserted(view, oldCur, chars);
    }

    /**
     * be done
     */
    return true;
}

void KTextEditor::DocumentPrivate::checkCursorForAutobrace(KTextEditor::View*, const KTextEditor::Cursor& newPos) {
    if ( m_currentAutobraceRange && ! m_currentAutobraceRange->toRange().contains(newPos) ) {
        m_currentAutobraceRange.clear();
    }
}

void KTextEditor::DocumentPrivate::newLine(KTextEditor::ViewPrivate *v)
{
    editStart();

    if (!v->config()->persistentSelection() && v->selection()) {
        v->removeSelectedText();
        v->clearSelection();
    }

    // query cursor position
    KTextEditor::Cursor c = v->cursorPosition();

    if (c.line() > (int)lastLine()) {
        c.setLine(lastLine());
    }

    if (c.line() < 0) {
        c.setLine(0);
    }

    uint ln = c.line();

    Kate::TextLine textLine = plainKateTextLine(ln);

    if (c.column() > (int)textLine->length()) {
        c.setColumn(textLine->length());
    }

    // first: wrap line
    editWrapLine(c.line(), c.column());

    // end edit session here, to have updated HL in userTypedChar!
    editEnd();

    // second: indent the new line, if needed...
    m_indenter->userTypedChar(v, v->cursorPosition(), QLatin1Char('\n'));
}

void KTextEditor::DocumentPrivate::transpose(const KTextEditor::Cursor &cursor)
{
    Kate::TextLine textLine = m_buffer->plainLine(cursor.line());

    if (!textLine || (textLine->length() < 2)) {
        return;
    }

    uint col = cursor.column();

    if (col > 0) {
        col--;
    }

    if ((textLine->length() - col) < 2) {
        return;
    }

    uint line = cursor.line();
    QString s;

    //clever swap code if first character on the line swap right&left
    //otherwise left & right
    s.append(textLine->at(col + 1));
    s.append(textLine->at(col));
    //do the swap

    // do it right, never ever manipulate a textline
    editStart();
    editRemoveText(line, col, 2);
    editInsertText(line, col, s);
    editEnd();
}

void KTextEditor::DocumentPrivate::backspace(KTextEditor::ViewPrivate *view, const KTextEditor::Cursor &c)
{
    if (!view->config()->persistentSelection() && view->selection()) {
        if (view->blockSelection() && view->selection() && toVirtualColumn(view->selectionRange().start()) == toVirtualColumn(view->selectionRange().end())) {
            // Remove one character after selection line
            KTextEditor::Range range = view->selectionRange();
            range.setStart(KTextEditor::Cursor(range.start().line(), range.start().column() - 1));
            view->setSelection(range);
        }
        view->removeSelectedText();
        return;
    }

    uint col = qMax(c.column(), 0);
    uint line = qMax(c.line(), 0);

    if ((col == 0) && (line == 0)) {
        return;
    }

    if (col > 0) {
        if (!(config()->backspaceIndents())) {
            // ordinary backspace
            KTextEditor::Cursor beginCursor(line, view->textLayout(c)->previousCursorPosition(c.column()));
            KTextEditor::Cursor endCursor(line, col);

            removeText(KTextEditor::Range(beginCursor, endCursor));
            // in most cases cursor is moved by removeText, but we should do it manually
            // for past-end-of-line cursors in block mode
            view->setCursorPosition(beginCursor);
        } else {
            // backspace indents: erase to next indent position
            Kate::TextLine textLine = m_buffer->plainLine(line);

            // don't forget this check!!!! really!!!!
            if (!textLine) {
                return;
            }

            int colX = textLine->toVirtualColumn(col, config()->tabWidth());
            int pos = textLine->firstChar();
            if (pos > 0) {
                pos = textLine->toVirtualColumn(pos, config()->tabWidth());
            }

            if (pos < 0 || pos >= (int)colX) {
                // only spaces on left side of cursor
                indent(KTextEditor::Range(line, 0, line, 0), -1);
            } else {
                KTextEditor::Cursor beginCursor(line, view->textLayout(c)->previousCursorPosition(c.column()));
                KTextEditor::Cursor endCursor(line, col);

                removeText(KTextEditor::Range(beginCursor, endCursor));
                // in most cases cursor is moved by removeText, but we should do it manually
                // for past-end-of-line cursors in block mode
                view->setCursorPosition(beginCursor);
            }
        }
    } else {
        // col == 0: wrap to previous line
        if (line >= 1) {
            Kate::TextLine textLine = m_buffer->plainLine(line - 1);

            // don't forget this check!!!! really!!!!
            if (!textLine) {
                return;
            }

            if (config()->wordWrap() && textLine->endsWith(QLatin1String(" "))) {
                // gg: in hard wordwrap mode, backspace must also eat the trailing space
                removeText(KTextEditor::Range(line - 1, textLine->length() - 1, line, 0));
            } else {
                removeText(KTextEditor::Range(line - 1, textLine->length(), line, 0));
            }
        }
    }
    if ( m_currentAutobraceRange ) {
        const auto r = m_currentAutobraceRange->toRange();
        if ( r.columnWidth() == 1 && view->cursorPosition() == r.start() ) {
            // start parenthesis removed and range length is 1, remove end as well
            del(view, view->cursorPosition());
            m_currentAutobraceRange.clear();
        }
    }
}

void KTextEditor::DocumentPrivate::del(KTextEditor::ViewPrivate *view, const KTextEditor::Cursor &c)
{
    if (!view->config()->persistentSelection() && view->selection()) {
        if (view->blockSelection() && view->selection() && toVirtualColumn(view->selectionRange().start()) == toVirtualColumn(view->selectionRange().end())) {
            // Remove one character after selection line
            KTextEditor::Range range = view->selectionRange();
            range.setEnd(KTextEditor::Cursor(range.end().line(), range.end().column() + 1));
            view->setSelection(range);
        }
        view->removeSelectedText();
        return;
    }

    if (c.column() < (int) m_buffer->plainLine(c.line())->length()) {
        KTextEditor::Cursor endCursor(c.line(), view->textLayout(c)->nextCursorPosition(c.column()));

        removeText(KTextEditor::Range(c, endCursor));
    } else if (c.line() < lastLine()) {
        removeText(KTextEditor::Range(c.line(), c.column(), c.line() + 1, 0));
    }
}

void KTextEditor::DocumentPrivate::paste(KTextEditor::ViewPrivate *view, const QString &text)
{
    static const QChar newLineChar(QLatin1Char('\n'));
    QString s = text;

    if (s.isEmpty()) {
        return;
    }

    int lines = s.count(newLineChar);

    m_undoManager->undoSafePoint();

    editStart();

    KTextEditor::Cursor pos = view->cursorPosition();
    if (!view->config()->persistentSelection() && view->selection()) {
        pos = view->selectionRange().start();
        if (view->blockSelection()) {
            pos = rangeOnLine(view->selectionRange(), pos.line()).start();
            if (lines == 0) {
                s += newLineChar;
                s = s.repeated(view->selectionRange().numberOfLines() + 1);
                s.chop(1);
            }
        }
        view->removeSelectedText();
    }

    if (config()->ovr()) {
        QStringList pasteLines = s.split(newLineChar);

        if (!view->blockSelection()) {
            int endColumn = (pasteLines.count() == 1 ? pos.column() : 0) + pasteLines.last().length();
            removeText(KTextEditor::Range(pos,
                                          pos.line() + pasteLines.count() - 1, endColumn));
        } else {
            int maxi = qMin(pos.line() + pasteLines.count(), this->lines());

            for (int i = pos.line(); i < maxi; ++i) {
                int pasteLength = pasteLines.at(i - pos.line()).length();
                removeText(KTextEditor::Range(i, pos.column(),
                                              i, qMin(pasteLength + pos.column(), lineLength(i))));
            }
        }
    }

    insertText(pos, s, view->blockSelection());
    editEnd();

    // move cursor right for block select, as the user is moved right internal
    // even in that case, but user expects other behavior in block selection
    // mode !
    // just let cursor stay, that was it before I changed to moving ranges!
    if (view->blockSelection()) {
        view->setCursorPositionInternal(pos);
    }

    if (config()->indentPastedText()) {
        KTextEditor::Range range = KTextEditor::Range(KTextEditor::Cursor(pos.line(), 0),
                                   KTextEditor::Cursor(pos.line() + lines, 0));

        m_indenter->indent(view, range);
    }

    if (!view->blockSelection()) {
        emit charactersSemiInteractivelyInserted(pos, s);
    }
    m_undoManager->undoSafePoint();
}

void KTextEditor::DocumentPrivate::indent(KTextEditor::Range range, int change)
{
    if (!isReadWrite()) {
        return;
    }

    editStart();
    m_indenter->changeIndent(range, change);
    editEnd();
}

void KTextEditor::DocumentPrivate::align(KTextEditor::ViewPrivate *view, const KTextEditor::Range &range)
{
    m_indenter->indent(view, range);
}

void KTextEditor::DocumentPrivate::insertTab(KTextEditor::ViewPrivate *view, const KTextEditor::Cursor &)
{
    if (!isReadWrite()) {
        return;
    }

    int lineLen = line(view->cursorPosition().line()).length();
    KTextEditor::Cursor c = view->cursorPosition();

    editStart();

    if (!view->config()->persistentSelection() && view->selection()) {
        view->removeSelectedText();
    } else if (view->currentInputMode()->overwrite() && c.column() < lineLen) {
        KTextEditor::Range r = KTextEditor::Range(view->cursorPosition(), 1);

        // replace mode needs to know what was removed so it can be restored with backspace
        QChar removed = line(view->cursorPosition().line()).at(r.start().column());
        view->currentInputMode()->overwrittenChar(removed);
        removeText(r);
    }

    c = view->cursorPosition();
    editInsertText(c.line(), c.column(), QStringLiteral("\t"));

    editEnd();
}

/*
  Remove a given string at the beginning
  of the current line.
*/
bool KTextEditor::DocumentPrivate::removeStringFromBeginning(int line, const QString &str)
{
    Kate::TextLine textline = m_buffer->plainLine(line);

    KTextEditor::Cursor cursor(line, 0);
    bool there = textline->startsWith(str);

    if (!there) {
        cursor.setColumn(textline->firstChar());
        there = textline->matchesAt(cursor.column(), str);
    }

    if (there) {
        // Remove some chars
        removeText(KTextEditor::Range(cursor, str.length()));
    }

    return there;
}

/*
  Remove a given string at the end
  of the current line.
*/
bool KTextEditor::DocumentPrivate::removeStringFromEnd(int line, const QString &str)
{
    Kate::TextLine textline = m_buffer->plainLine(line);

    KTextEditor::Cursor cursor(line, 0);
    bool there = textline->endsWith(str);

    if (there) {
        cursor.setColumn(textline->length() - str.length());
    } else {
        cursor.setColumn(textline->lastChar() - str.length() + 1);
        there = textline->matchesAt(cursor.column(), str);
    }

    if (there) {
        // Remove some chars
        removeText(KTextEditor::Range(cursor, str.length()));
    }

    return there;
}

/*
  Replace tabs by spaces in the given string, if enabled.
 */
QString KTextEditor::DocumentPrivate::eventuallyReplaceTabs(const KTextEditor::Cursor &cursorPos, const QString &str) const
{
    const bool replacetabs = config()->replaceTabsDyn();
    if ( ! replacetabs ) {
        return str;
    }
    const int indentWidth = config()->indentationWidth();
    static const QLatin1Char tabChar('\t');

    int column = cursorPos.column();

    // The result will always be at least as long as the input
    QString result;
    result.reserve(str.size());

    Q_FOREACH (const QChar ch, str) {
        if (ch == tabChar) {
            // Insert only enough spaces to align to the next indentWidth column
            // This fixes bug #340212
            int spacesToInsert = indentWidth - (column % indentWidth);
            result += QStringLiteral(" ").repeated(spacesToInsert);
            column += spacesToInsert;
        } else {
            // Just keep all other typed characters as-is
            result += ch;
            ++column;
        }
    }
    return result;
}

/*
  Add to the current line a comment line mark at the beginning.
*/
void KTextEditor::DocumentPrivate::addStartLineCommentToSingleLine(int line, int attrib)
{
    QString commentLineMark = highlight()->getCommentSingleLineStart(attrib);
    int pos = -1;

    if (highlight()->getCommentSingleLinePosition(attrib) == KateHighlighting::CSLPosColumn0) {
        pos = 0;
        commentLineMark += QLatin1Char(' ');
    } else {
        const Kate::TextLine l = kateTextLine(line);
        pos = l->firstChar();
    }

    if (pos >= 0) {
        insertText(KTextEditor::Cursor(line, pos), commentLineMark);
    }
}

/*
  Remove from the current line a comment line mark at
  the beginning if there is one.
*/
bool KTextEditor::DocumentPrivate::removeStartLineCommentFromSingleLine(int line, int attrib)
{
    const QString shortCommentMark = highlight()->getCommentSingleLineStart(attrib);
    const QString longCommentMark = shortCommentMark + QLatin1Char(' ');

    editStart();

    // Try to remove the long comment mark first
    bool removed = (removeStringFromBeginning(line, longCommentMark)
                    || removeStringFromBeginning(line, shortCommentMark));

    editEnd();

    return removed;
}

/*
  Add to the current line a start comment mark at the
  beginning and a stop comment mark at the end.
*/
void KTextEditor::DocumentPrivate::addStartStopCommentToSingleLine(int line, int attrib)
{
    const QString startCommentMark = highlight()->getCommentStart(attrib) + QLatin1Char(' ');
    const QString stopCommentMark = QLatin1Char(' ') + highlight()->getCommentEnd(attrib);

    editStart();

    // Add the start comment mark
    insertText(KTextEditor::Cursor(line, 0), startCommentMark);

    // Go to the end of the line
    const int col = m_buffer->plainLine(line)->length();

    // Add the stop comment mark
    insertText(KTextEditor::Cursor(line, col), stopCommentMark);

    editEnd();
}

/*
  Remove from the current line a start comment mark at
  the beginning and a stop comment mark at the end.
*/
bool KTextEditor::DocumentPrivate::removeStartStopCommentFromSingleLine(int line, int attrib)
{
    const QString shortStartCommentMark = highlight()->getCommentStart(attrib);
    const QString longStartCommentMark = shortStartCommentMark + QLatin1Char(' ');
    const QString shortStopCommentMark = highlight()->getCommentEnd(attrib);
    const QString longStopCommentMark = QLatin1Char(' ') + shortStopCommentMark;

    editStart();

    // Try to remove the long start comment mark first
    const bool removedStart = (removeStringFromBeginning(line, longStartCommentMark)
                         || removeStringFromBeginning(line, shortStartCommentMark));

    // Try to remove the long stop comment mark first
    const bool removedStop = removedStart
        && (removeStringFromEnd(line, longStopCommentMark) || removeStringFromEnd(line, shortStopCommentMark));

    editEnd();

    return (removedStart || removedStop);
}

/*
  Add to the current selection a start comment mark at the beginning
  and a stop comment mark at the end.
*/
void KTextEditor::DocumentPrivate::addStartStopCommentToSelection(KTextEditor::ViewPrivate *view, int attrib)
{
    const QString startComment = highlight()->getCommentStart(attrib);
    const QString endComment = highlight()->getCommentEnd(attrib);

    KTextEditor::Range range = view->selectionRange();

    if ((range.end().column() == 0) && (range.end().line() > 0)) {
        range.setEnd(KTextEditor::Cursor(range.end().line() - 1, lineLength(range.end().line() - 1)));
    }

    editStart();

    if (!view->blockSelection()) {
        insertText(range.end(), endComment);
        insertText(range.start(), startComment);
    } else {
        for (int line = range.start().line(); line <= range.end().line(); line++) {
            KTextEditor::Range subRange = rangeOnLine(range, line);
            insertText(subRange.end(), endComment);
            insertText(subRange.start(), startComment);
        }
    }

    editEnd();
    // selection automatically updated (MovingRange)
}

/*
  Add to the current selection a comment line mark at the beginning of each line.
*/
void KTextEditor::DocumentPrivate::addStartLineCommentToSelection(KTextEditor::ViewPrivate *view, int attrib)
{
    const QString commentLineMark = highlight()->getCommentSingleLineStart(attrib) + QLatin1Char(' ');

    int sl = view->selectionRange().start().line();
    int el = view->selectionRange().end().line();

    // if end of selection is in column 0 in last line, omit the last line
    if ((view->selectionRange().end().column() == 0) && (el > 0)) {
        el--;
    }

    editStart();

    // For each line of the selection
    for (int z = el; z >= sl; z--) {
        //insertText (z, 0, commentLineMark);
        addStartLineCommentToSingleLine(z, attrib);
    }

    editEnd();
    // selection automatically updated (MovingRange)
}

bool KTextEditor::DocumentPrivate::nextNonSpaceCharPos(int &line, int &col)
{
    for (; line < (int)m_buffer->count(); line++) {
        Kate::TextLine textLine = m_buffer->plainLine(line);

        if (!textLine) {
            break;
        }

        col = textLine->nextNonSpaceChar(col);
        if (col != -1) {
            return true;    // Next non-space char found
        }
        col = 0;
    }
    // No non-space char found
    line = -1;
    col = -1;
    return false;
}

bool KTextEditor::DocumentPrivate::previousNonSpaceCharPos(int &line, int &col)
{
    while (true) {
        Kate::TextLine textLine = m_buffer->plainLine(line);

        if (!textLine) {
            break;
        }

        col = textLine->previousNonSpaceChar(col);
        if (col != -1) {
            return true;
        }
        if (line == 0) {
            return false;
        }
        --line;
        col = textLine->length();
    }
    // No non-space char found
    line = -1;
    col = -1;
    return false;
}

/*
  Remove from the selection a start comment mark at
  the beginning and a stop comment mark at the end.
*/
bool KTextEditor::DocumentPrivate::removeStartStopCommentFromSelection(KTextEditor::ViewPrivate *view, int attrib)
{
    const QString startComment = highlight()->getCommentStart(attrib);
    const QString endComment = highlight()->getCommentEnd(attrib);

    int sl = qMax<int> (0, view->selectionRange().start().line());
    int el = qMin<int> (view->selectionRange().end().line(), lastLine());
    int sc = view->selectionRange().start().column();
    int ec = view->selectionRange().end().column();

    // The selection ends on the char before selectEnd
    if (ec != 0) {
        --ec;
    } else if (el > 0) {
        --el;
        ec = m_buffer->plainLine(el)->length() - 1;
    }

    const int startCommentLen = startComment.length();
    const int endCommentLen = endComment.length();

    // had this been perl or sed: s/^\s*$startComment(.+?)$endComment\s*/$2/

    bool remove = nextNonSpaceCharPos(sl, sc)
                  && m_buffer->plainLine(sl)->matchesAt(sc, startComment)
                  && previousNonSpaceCharPos(el, ec)
                  && ((ec - endCommentLen + 1) >= 0)
                  && m_buffer->plainLine(el)->matchesAt(ec - endCommentLen + 1, endComment);

    if (remove) {
        editStart();

        removeText(KTextEditor::Range(el, ec - endCommentLen + 1, el, ec + 1));
        removeText(KTextEditor::Range(sl, sc, sl, sc + startCommentLen));

        editEnd();
        // selection automatically updated (MovingRange)
    }

    return remove;
}

bool KTextEditor::DocumentPrivate::removeStartStopCommentFromRegion(const KTextEditor::Cursor &start, const KTextEditor::Cursor &end, int attrib)
{
    const QString startComment = highlight()->getCommentStart(attrib);
    const QString endComment = highlight()->getCommentEnd(attrib);
    const int startCommentLen = startComment.length();
    const int endCommentLen = endComment.length();

    const bool remove = m_buffer->plainLine(start.line())->matchesAt(start.column(), startComment)
                        && m_buffer->plainLine(end.line())->matchesAt(end.column() - endCommentLen, endComment);
    if (remove) {
        editStart();
        removeText(KTextEditor::Range(end.line(), end.column() - endCommentLen, end.line(), end.column()));
        removeText(KTextEditor::Range(start, startCommentLen));
        editEnd();
    }
    return remove;
}

/*
  Remove from the beginning of each line of the
  selection a start comment line mark.
*/
bool KTextEditor::DocumentPrivate::removeStartLineCommentFromSelection(KTextEditor::ViewPrivate *view, int attrib)
{
    const QString shortCommentMark = highlight()->getCommentSingleLineStart(attrib);
    const QString longCommentMark = shortCommentMark + QLatin1Char(' ');

    int sl = view->selectionRange().start().line();
    int el = view->selectionRange().end().line();

    if ((view->selectionRange().end().column() == 0) && (el > 0)) {
        el--;
    }

    bool removed = false;

    editStart();

    // For each line of the selection
    for (int z = el; z >= sl; z--) {
        // Try to remove the long comment mark first
        removed = (removeStringFromBeginning(z, longCommentMark)
                   || removeStringFromBeginning(z, shortCommentMark)
                   || removed);
    }

    editEnd();
    // selection automatically updated (MovingRange)

    return removed;
}

/*
  Comment or uncomment the selection or the current
  line if there is no selection.
*/
void KTextEditor::DocumentPrivate::comment(KTextEditor::ViewPrivate *v, uint line, uint column, int change)
{
    // skip word wrap bug #105373
    const bool skipWordWrap = wordWrap();
    if (skipWordWrap) {
        setWordWrap(false);
    }

    bool hassel = v->selection();
    int c = 0;

    if (hassel) {
        c = v->selectionRange().start().column();
    }

    int startAttrib = 0;
    Kate::TextLine ln = kateTextLine(line);

    if (c < ln->length()) {
        startAttrib = ln->attribute(c);
    } else if (!ln->contextStack().isEmpty()) {
        startAttrib = highlight()->attribute(ln->contextStack().last());
    }

    bool hasStartLineCommentMark = !(highlight()->getCommentSingleLineStart(startAttrib).isEmpty());
    bool hasStartStopCommentMark = (!(highlight()->getCommentStart(startAttrib).isEmpty())
                                    && !(highlight()->getCommentEnd(startAttrib).isEmpty()));

    if (change > 0) { // comment
        if (!hassel) {
            if (hasStartLineCommentMark) {
                addStartLineCommentToSingleLine(line, startAttrib);
            } else if (hasStartStopCommentMark) {
                addStartStopCommentToSingleLine(line, startAttrib);
            }
        } else {
            // anders: prefer single line comment to avoid nesting probs
            // If the selection starts after first char in the first line
            // or ends before the last char of the last line, we may use
            // multiline comment markers.
            // TODO We should try to detect nesting.
            //    - if selection ends at col 0, most likely she wanted that
            // line ignored
            const KTextEditor::Range sel = v->selectionRange();
            if (hasStartStopCommentMark &&
                    (!hasStartLineCommentMark || (
                         (sel.start().column() > m_buffer->plainLine(sel.start().line())->firstChar()) ||
                         (sel.end().column() > 0 &&
                          sel.end().column() < (m_buffer->plainLine(sel.end().line())->length()))
                     ))) {
                addStartStopCommentToSelection(v, startAttrib);
            } else if (hasStartLineCommentMark) {
                addStartLineCommentToSelection(v, startAttrib);
            }
        }
    } else { // uncomment
        bool removed = false;
        if (!hassel) {
            removed = (hasStartLineCommentMark
                       && removeStartLineCommentFromSingleLine(line, startAttrib))
                      || (hasStartStopCommentMark
                          && removeStartStopCommentFromSingleLine(line, startAttrib));
        } else {
            // anders: this seems like it will work with above changes :)
            removed = (hasStartStopCommentMark
                       && removeStartStopCommentFromSelection(v, startAttrib))
                      || (hasStartLineCommentMark
                          && removeStartLineCommentFromSelection(v, startAttrib));
        }

        // recursive call for toggle comment
        if (!removed && change == 0) {
            comment(v, line, column, 1);
        }
    }

    if (skipWordWrap) {
        setWordWrap(true);    // see begin of function ::comment (bug #105373)
    }
}

void KTextEditor::DocumentPrivate::transform(KTextEditor::ViewPrivate *v, const KTextEditor::Cursor &c,
                             KTextEditor::DocumentPrivate::TextTransform t)
{
    if (v->selection()) {
        editStart();

        // cache the selection and cursor, so we can be sure to restore.
        KTextEditor::Range selection = v->selectionRange();

        KTextEditor::Range range(selection.start(), 0);
        while (range.start().line() <= selection.end().line()) {
            int start = 0;
            int end = lineLength(range.start().line());

            if (range.start().line() == selection.start().line() || v->blockSelection()) {
                start = selection.start().column();
            }

            if (range.start().line() == selection.end().line() || v->blockSelection()) {
                end = selection.end().column();
            }

            if (start > end) {
                int swapCol = start;
                start = end;
                end = swapCol;
            }
            range.setStart(KTextEditor::Cursor(range.start().line(), start));
            range.setEnd(KTextEditor::Cursor(range.end().line(),  end));

            QString s = text(range);
            QString old = s;

            if (t == Uppercase) {
                s = s.toUpper();
            } else if (t == Lowercase) {
                s = s.toLower();
            } else { // Capitalize
                Kate::TextLine l = m_buffer->plainLine(range.start().line());
                int p(0);
                while (p < s.length()) {
                    // If bol or the character before is not in a word, up this one:
                    // 1. if both start and p is 0, upper char.
                    // 2. if blockselect or first line, and p == 0 and start-1 is not in a word, upper
                    // 3. if p-1 is not in a word, upper.
                    if ((! range.start().column() && ! p) ||
                            ((range.start().line() == selection.start().line() || v->blockSelection()) &&
                             ! p && ! highlight()->isInWord(l->at(range.start().column() - 1))) ||
                            (p && ! highlight()->isInWord(s.at(p - 1)))
                       ) {
                        s[p] = s.at(p).toUpper();
                    }
                    p++;
                }
            }

            if (s != old) {
                removeText(range);
                insertText(range.start(), s);
            }

            range.setBothLines(range.start().line() + 1);
        }

        editEnd();

        // restore selection & cursor
        v->setSelection(selection);
        v->setCursorPosition(c);

    } else {  // no selection
        editStart();

        // get cursor
        KTextEditor::Cursor cursor = c;

        QString old = text(KTextEditor::Range(cursor, 1));
        QString s;
        switch (t) {
        case Uppercase:
            s = old.toUpper();
            break;
        case Lowercase:
            s = old.toLower();
            break;
        case Capitalize: {
            Kate::TextLine l = m_buffer->plainLine(cursor.line());
            while (cursor.column() > 0 && highlight()->isInWord(l->at(cursor.column() - 1), l->attribute(cursor.column() - 1))) {
                cursor.setColumn(cursor.column() - 1);
            }
            old = text(KTextEditor::Range(cursor, 1));
            s = old.toUpper();
        }
        break;
        default:
            break;
        }

        removeText(KTextEditor::Range(cursor, 1));
        insertText(cursor, s);

        editEnd();
    }

}

void KTextEditor::DocumentPrivate::joinLines(uint first, uint last)
{
//   if ( first == last ) last += 1;
    editStart();
    int line(first);
    while (first < last) {
        // Normalize the whitespace in the joined lines by making sure there's
        // always exactly one space between the joined lines
        // This cannot be done in editUnwrapLine, because we do NOT want this
        // behavior when deleting from the start of a line, just when explicitly
        // calling the join command
        Kate::TextLine l = kateTextLine(line);
        Kate::TextLine tl = kateTextLine(line + 1);

        if (!l || !tl) {
            editEnd();
            return;
        }

        int pos = tl->firstChar();
        if (pos >= 0) {
            if (pos != 0) {
                editRemoveText(line + 1, 0, pos);
            }
            if (!(l->length() == 0 || l->at(l->length() - 1).isSpace())) {
                editInsertText(line + 1, 0, QLatin1String(" "));
            }
        } else {
            // Just remove the whitespace and let Kate handle the rest
            editRemoveText(line + 1, 0, tl->length());
        }

        editUnWrapLine(line);
        first++;
    }
    editEnd();
}

void KTextEditor::DocumentPrivate::tagLines(int start, int end)
{
    foreach (KTextEditor::ViewPrivate *view, m_views) {
        view->tagLines(start, end, true);
    }
}

void KTextEditor::DocumentPrivate::repaintViews(bool paintOnlyDirty)
{
    foreach (KTextEditor::ViewPrivate *view, m_views) {
        view->repaintText(paintOnlyDirty);
    }
}

/*
   Bracket matching uses the following algorithm:
   If in overwrite mode, match the bracket currently underneath the cursor.
   Otherwise, if the character to the left is a bracket,
   match it. Otherwise if the character to the right of the cursor is a
   bracket, match it. Otherwise, don't match anything.
*/
KTextEditor::Range KTextEditor::DocumentPrivate::findMatchingBracket(const KTextEditor::Cursor &start, int maxLines)
{
    if (maxLines < 0) {
        return KTextEditor::Range::invalid();
    }

    Kate::TextLine textLine = m_buffer->plainLine(start.line());
    if (!textLine) {
        return KTextEditor::Range::invalid();
    }

    KTextEditor::Range range(start, start);
    const QChar right = textLine->at(range.start().column());
    const QChar left  = textLine->at(range.start().column() - 1);
    QChar bracket;

    if (config()->ovr()) {
        if (isBracket(right)) {
            bracket = right;
        } else {
            return KTextEditor::Range::invalid();
        }
    } else if (isBracket(right)) {
        bracket = right;
    } else if (isBracket(left)) {
        range.setStart(KTextEditor::Cursor(range.start().line(), range.start().column() - 1));
        bracket = left;
    } else {
        return KTextEditor::Range::invalid();
    }

    const QChar opposite = matchingBracket(bracket, false);
    if (opposite.isNull()) {
        return KTextEditor::Range::invalid();
    }

    const int searchDir = isStartBracket(bracket) ? 1 : -1;
    uint nesting = 0;

    const int minLine = qMax(range.start().line() - maxLines, 0);
    const int maxLine = qMin(range.start().line() + maxLines, documentEnd().line());

    range.setEnd(range.start());
    KTextEditor::DocumentCursor cursor(this);
    cursor.setPosition(range.start());
    int validAttr = kateTextLine(cursor.line())->attribute(cursor.column());

    while (cursor.line() >= minLine && cursor.line() <= maxLine) {

        if (!cursor.move(searchDir)) {
            return KTextEditor::Range::invalid();
        }

        Kate::TextLine textLine = kateTextLine(cursor.line());
        if (textLine->attribute(cursor.column()) == validAttr) {
            // Check for match
            QChar c = textLine->at(cursor.column());
            if (c == opposite) {
                if (nesting == 0) {
                    if (searchDir > 0) { // forward
                        range.setEnd(cursor.toCursor());
                    } else {
                        range.setStart(cursor.toCursor());
                    }
                    return range;
                }
                nesting--;
            } else if (c == bracket) {
                nesting++;
            }
        }
    }

    return KTextEditor::Range::invalid();
}

// helper: remove \r and \n from visible document name (bug #170876)
inline static QString removeNewLines(const QString &str)
{
    QString tmp(str);
    return tmp.replace(QLatin1String("\r\n"), QLatin1String(" "))
           .replace(QLatin1Char('\r'), QLatin1Char(' '))
           .replace(QLatin1Char('\n'), QLatin1Char(' '));
}

void KTextEditor::DocumentPrivate::updateDocName()
{
    // if the name is set, and starts with FILENAME, it should not be changed!
    if (! url().isEmpty()
            && (m_docName == removeNewLines(url().fileName()) ||
                m_docName.startsWith(removeNewLines(url().fileName()) + QLatin1String(" (")))) {
        return;
    }

    int count = -1;

    foreach (KTextEditor::DocumentPrivate *doc, KTextEditor::EditorPrivate::self()->kateDocuments()) {
        if ((doc != this) && (doc->url().fileName() == url().fileName()))
            if (doc->m_docNameNumber > count) {
                count = doc->m_docNameNumber;
            }
    }

    m_docNameNumber = count + 1;

    QString oldName = m_docName;
    m_docName = removeNewLines(url().fileName());

    m_isUntitled = m_docName.isEmpty();
    if (m_isUntitled) {
        m_docName = i18n("Untitled");
    }

    if (m_docNameNumber > 0) {
        m_docName = QString(m_docName + QLatin1String(" (%1)")).arg(m_docNameNumber + 1);
    }

    /**
     * avoid to emit this, if name doesn't change!
     */
    if (oldName != m_docName) {
        emit documentNameChanged(this);
    }
}

void KTextEditor::DocumentPrivate::slotModifiedOnDisk(KTextEditor::View * /*v*/)
{
    if (url().isEmpty() || !m_modOnHd) {
        return;
    }

    if (!m_fileChangedDialogsActivated || m_modOnHdHandler) {
        return;
    }

    // don't ask the user again and again the same thing
    if (m_modOnHdReason == m_prevModOnHdReason) {
        return;
    }
    m_prevModOnHdReason = m_modOnHdReason;

    m_modOnHdHandler = new KateModOnHdPrompt(this, m_modOnHdReason, reasonedMOHString());
    connect(m_modOnHdHandler.data(), &KateModOnHdPrompt::saveAsTriggered, this, &DocumentPrivate::onModOnHdSaveAs);
    connect(m_modOnHdHandler.data(), &KateModOnHdPrompt::reloadTriggered, this, &DocumentPrivate::onModOnHdReload);
    connect(m_modOnHdHandler.data(), &KateModOnHdPrompt::ignoreTriggered, this, &DocumentPrivate::onModOnHdIgnore);
}

void KTextEditor::DocumentPrivate::onModOnHdSaveAs()
{
    m_modOnHd = false;
    QWidget *parentWidget(dialogParent());
    const QUrl res = QFileDialog::getSaveFileUrl(parentWidget, i18n("Save File"), url(), {}, nullptr,
                                                 QFileDialog::DontConfirmOverwrite);
    if (!res.isEmpty() && checkOverwrite(res, parentWidget)) {
        if (! saveAs(res)) {
            KMessageBox::error(parentWidget, i18n("Save failed"));
            m_modOnHd = true;
        } else {
            delete m_modOnHdHandler;
            m_prevModOnHdReason = OnDiskUnmodified;
            emit modifiedOnDisk(this, false, OnDiskUnmodified);
        }
    } else { // the save as dialog was canceled, we are still modified on disk
        m_modOnHd = true;
    }
}

void KTextEditor::DocumentPrivate::onModOnHdReload()
{
    m_modOnHd = false;
    m_prevModOnHdReason = OnDiskUnmodified;
    emit modifiedOnDisk(this, false, OnDiskUnmodified);
    documentReload();
    delete m_modOnHdHandler;
}

void KTextEditor::DocumentPrivate::onModOnHdIgnore()
{
    // ignore as long as m_prevModOnHdReason == m_modOnHdReason
    delete m_modOnHdHandler;
}

void KTextEditor::DocumentPrivate::setModifiedOnDisk(ModifiedOnDiskReason reason)
{
    m_modOnHdReason = reason;
    m_modOnHd = (reason != OnDiskUnmodified);
    emit modifiedOnDisk(this, (reason != OnDiskUnmodified), reason);
}

class KateDocumentTmpMark
{
public:
    QString line;
    KTextEditor::Mark mark;
};

void KTextEditor::DocumentPrivate::setModifiedOnDiskWarning(bool on)
{
    m_fileChangedDialogsActivated = on;
}

bool KTextEditor::DocumentPrivate::documentReload()
{
    if (url().isEmpty()) {
        return false;
    }

    // typically, the message for externally modified files is visible. Since it
    // does not make sense showing an additional dialog, just hide the message.
    delete m_modOnHdHandler;

    if (m_modOnHd && m_fileChangedDialogsActivated) {
        QWidget *parentWidget(dialogParent());

        int i = KMessageBox::warningYesNoCancel
                (parentWidget, reasonedMOHString() + QLatin1String("\n\n") + i18n("What do you want to do?"),
                    i18n("File Was Changed on Disk"),
                    KGuiItem(i18n("&Reload File"), QStringLiteral("view-refresh")),
                    KGuiItem(i18n("&Ignore Changes"), QStringLiteral("dialog-warning")));

        if (i != KMessageBox::Yes) {
            if (i == KMessageBox::No) {
                m_modOnHd = false;
                m_modOnHdReason = OnDiskUnmodified;
                m_prevModOnHdReason = OnDiskUnmodified;
                emit modifiedOnDisk(this, m_modOnHd, m_modOnHdReason);
            }

            // reset some flags only valid for one reload!
            m_userSetEncodingForNextReload = false;
            return false;
        }
    }

    emit aboutToReload(this);

    QList<KateDocumentTmpMark> tmp;

    for (QHash<int, KTextEditor::Mark *>::const_iterator i = m_marks.constBegin(); i != m_marks.constEnd(); ++i) {
        KateDocumentTmpMark m;

        m.line = line(i.value()->line);
        m.mark = *i.value();

        tmp.append(m);
    }

    const QString oldMode = mode();
    const bool byUser = m_fileTypeSetByUser;
    const QString hl_mode = highlightingMode();

    m_storedVariables.clear();

    // save cursor positions for all views
    QHash<KTextEditor::ViewPrivate *, KTextEditor::Cursor> cursorPositions;
    for (auto it = m_views.constBegin(); it != m_views.constEnd(); ++it) {
        auto v = it.value();
        cursorPositions.insert(v, v->cursorPosition());
    }

    m_reloading = true;
    KTextEditor::DocumentPrivate::openUrl(url());

    // reset some flags only valid for one reload!
    m_userSetEncodingForNextReload = false;

    // restore cursor positions for all views
    for (auto it = m_views.constBegin(); it != m_views.constEnd(); ++it) {
        auto v = it.value();
        setActiveView(v);
        v->setCursorPosition(cursorPositions.value(v));
        if (v->isVisible()) {
            v->repaintText(false);
        }
    }

    for (int z = 0; z < tmp.size(); z++) {
        if (z < (int)lines()) {
            if (line(tmp.at(z).mark.line) == tmp.at(z).line) {
                setMark(tmp.at(z).mark.line, tmp.at(z).mark.type);
            }
        }
    }

    if (byUser) {
        setMode(oldMode);
    }
    setHighlightingMode(hl_mode);

    emit reloaded(this);

    return true;
}

bool KTextEditor::DocumentPrivate::documentSave()
{
    if (!url().isValid() || !isReadWrite()) {
        return documentSaveAs();
    }

    return save();
}

bool KTextEditor::DocumentPrivate::documentSaveAs()
{
    const QUrl saveUrl = QFileDialog::getSaveFileUrl(dialogParent(), i18n("Save File"), url(), {}, nullptr,
                                                     QFileDialog::DontConfirmOverwrite);
    if (saveUrl.isEmpty() || !checkOverwrite(saveUrl, dialogParent())) {
        return false;
    }

    return saveAs(saveUrl);
}

bool KTextEditor::DocumentPrivate::documentSaveAsWithEncoding(const QString &encoding)
{
    const QUrl saveUrl = QFileDialog::getSaveFileUrl(dialogParent(), i18n("Save File"), url(), {}, nullptr,
                                                     QFileDialog::DontConfirmOverwrite);
    if (saveUrl.isEmpty() || !checkOverwrite(saveUrl, dialogParent())) {
        return false;
    }

    setEncoding(encoding);
    return saveAs(saveUrl);
}

bool KTextEditor::DocumentPrivate::documentSaveCopyAs()
{
    const QUrl saveUrl = QFileDialog::getSaveFileUrl(dialogParent(), i18n("Save Copy of File"), url(),  {}, nullptr,
                                                     QFileDialog::DontConfirmOverwrite);
    if (saveUrl.isEmpty() || !checkOverwrite(saveUrl, dialogParent())) {
        return false;
    }

    QTemporaryFile file;
    if (!file.open()) {
        return false;
    }

    if (!m_buffer->saveFile(file.fileName())) {
        KMessageBox::error(dialogParent(), i18n("The document could not be saved, as it was not possible to write to %1.\n\nCheck that you have write access to this file or that enough disk space is available.", this->url().toDisplayString(QUrl::PreferLocalFile)));
        return false;
    }

    // KIO move
    KIO::FileCopyJob *job = KIO::file_copy(QUrl::fromLocalFile(file.fileName()), saveUrl);
    KJobWidgets::setWindow(job, QApplication::activeWindow());
    return job->exec();
}

void KTextEditor::DocumentPrivate::setWordWrap(bool on)
{
    config()->setWordWrap(on);
}

bool KTextEditor::DocumentPrivate::wordWrap() const
{
    return config()->wordWrap();
}

void KTextEditor::DocumentPrivate::setWordWrapAt(uint col)
{
    config()->setWordWrapAt(col);
}

unsigned int KTextEditor::DocumentPrivate::wordWrapAt() const
{
    return config()->wordWrapAt();
}

void KTextEditor::DocumentPrivate::setPageUpDownMovesCursor(bool on)
{
    config()->setPageUpDownMovesCursor(on);
}

bool KTextEditor::DocumentPrivate::pageUpDownMovesCursor() const
{
    return config()->pageUpDownMovesCursor();
}
//END

bool KTextEditor::DocumentPrivate::setEncoding(const QString &e)
{
    return m_config->setEncoding(e);
}

QString KTextEditor::DocumentPrivate::encoding() const
{
    return m_config->encoding();
}

void KTextEditor::DocumentPrivate::updateConfig()
{
    m_undoManager->updateConfig();

    // switch indenter if needed and update config....
    m_indenter->setMode(m_config->indentationMode());
    m_indenter->updateConfig();

    // set tab width there, too
    m_buffer->setTabWidth(config()->tabWidth());

    // update all views, does tagAll and updateView...
    foreach (KTextEditor::ViewPrivate *view, m_views) {
        view->updateDocumentConfig();
    }

    // update on-the-fly spell checking as spell checking defaults might have changes
    if (m_onTheFlyChecker) {
        m_onTheFlyChecker->updateConfig();
    }

    emit configChanged();
}

//BEGIN Variable reader
// "local variable" feature by anders, 2003
/* TODO
      add config options (how many lines to read, on/off)
      add interface for plugins/apps to set/get variables
      add view stuff
*/
void KTextEditor::DocumentPrivate::readVariables(bool onlyViewAndRenderer)
{
    if (!onlyViewAndRenderer) {
        m_config->configStart();
    }

    // views!
    KTextEditor::ViewPrivate *v;
    foreach (v, m_views) {
        v->config()->configStart();
        v->renderer()->config()->configStart();
    }
    // read a number of lines in the top/bottom of the document
    for (int i = 0; i < qMin(9, lines()); ++i) {
        readVariableLine(line(i), onlyViewAndRenderer);
    }
    if (lines() > 10) {
        for (int i = qMax(10, lines() - 10); i < lines(); i++) {
            readVariableLine(line(i), onlyViewAndRenderer);
        }
    }

    if (!onlyViewAndRenderer) {
        m_config->configEnd();
    }

    foreach (v, m_views) {
        v->config()->configEnd();
        v->renderer()->config()->configEnd();
    }
}

void KTextEditor::DocumentPrivate::readVariableLine(QString t, bool onlyViewAndRenderer)
{
    static const QRegExp kvLine(QStringLiteral("kate:(.*)"));
    static const QRegExp kvLineWildcard(QStringLiteral("kate-wildcard\\((.*)\\):(.*)"));
    static const QRegExp kvLineMime(QStringLiteral("kate-mimetype\\((.*)\\):(.*)"));
    static const QRegExp kvVar(QStringLiteral("([\\w\\-]+)\\s+([^;]+)"));

    // simple check first, no regex
    // no kate inside, no vars, simple...
    if (!t.contains(QLatin1String("kate"))) {
        return;
    }

    // found vars, if any
    QString s;

    // now, try first the normal ones
    if (kvLine.indexIn(t) > -1) {
        s = kvLine.cap(1);

        //qCDebug(LOG_KTE) << "normal variable line kate: matched: " << s;
    } else if (kvLineWildcard.indexIn(t) > -1) { // regex given
        const QStringList wildcards(kvLineWildcard.cap(1).split(QLatin1Char(';'), QString::SkipEmptyParts));
        const QString nameOfFile = url().fileName();

        bool found = false;
        foreach (const QString &pattern, wildcards) {
            QRegExp wildcard(pattern, Qt::CaseSensitive, QRegExp::Wildcard);

            found = wildcard.exactMatch(nameOfFile);
        }

        // nothing usable found...
        if (!found) {
            return;
        }

        s = kvLineWildcard.cap(2);

        //qCDebug(LOG_KTE) << "guarded variable line kate-wildcard: matched: " << s;
    } else if (kvLineMime.indexIn(t) > -1) { // mime-type given
        const QStringList types(kvLineMime.cap(1).split(QLatin1Char(';'), QString::SkipEmptyParts));

        // no matching type found
        if (!types.contains(mimeType())) {
            return;
        }

        s = kvLineMime.cap(2);

        //qCDebug(LOG_KTE) << "guarded variable line kate-mimetype: matched: " << s;
    } else { // nothing found
        return;
    }

    QStringList vvl; // view variable names
    vvl << QStringLiteral("dynamic-word-wrap") << QStringLiteral("dynamic-word-wrap-indicators")
        << QStringLiteral("line-numbers") << QStringLiteral("icon-border") << QStringLiteral("folding-markers")
        << QStringLiteral("folding-preview")
        << QStringLiteral("bookmark-sorting") << QStringLiteral("auto-center-lines")
        << QStringLiteral("icon-bar-color")
        << QStringLiteral("scrollbar-minimap")
        << QStringLiteral("scrollbar-preview")
        // renderer
        << QStringLiteral("background-color") << QStringLiteral("selection-color")
        << QStringLiteral("current-line-color") << QStringLiteral("bracket-highlight-color")
        << QStringLiteral("word-wrap-marker-color")
        << QStringLiteral("font") << QStringLiteral("font-size") << QStringLiteral("scheme");
    int spaceIndent = -1;  // for backward compatibility; see below
    bool replaceTabsSet = false;
    int p(0);

    QString  var, val;
    while ((p = kvVar.indexIn(s, p)) > -1) {
        p += kvVar.matchedLength();
        var = kvVar.cap(1);
        val = kvVar.cap(2).trimmed();
        bool state; // store booleans here
        int n; // store ints here

        // only apply view & renderer config stuff
        if (onlyViewAndRenderer) {
            if (vvl.contains(var)) {   // FIXME define above
                setViewVariable(var, val);
            }
        } else {
            // BOOL  SETTINGS
            if (var == QLatin1String("word-wrap") && checkBoolValue(val, &state)) {
                setWordWrap(state);    // ??? FIXME CHECK
            }
            // KateConfig::configFlags
            // FIXME should this be optimized to only a few calls? how?
            else if (var == QLatin1String("backspace-indents") && checkBoolValue(val, &state)) {
                m_config->setBackspaceIndents(state);
            } else if (var == QLatin1String("indent-pasted-text") && checkBoolValue(val, &state)) {
                m_config->setIndentPastedText(state);
            } else if (var == QLatin1String("replace-tabs") && checkBoolValue(val, &state)) {
                m_config->setReplaceTabsDyn(state);
                replaceTabsSet = true;  // for backward compatibility; see below
            } else if (var == QLatin1String("remove-trailing-space") && checkBoolValue(val, &state)) {
                qCWarning(LOG_KTE) << i18n("Using deprecated modeline 'remove-trailing-space'. "
                                            "Please replace with 'remove-trailing-spaces modified;', see "
                                            "http://docs.kde.org/stable/en/applications/kate/config-variables.html#variable-remove-trailing-spaces");
                m_config->setRemoveSpaces(state ? 1 : 0);
            } else if (var == QLatin1String("replace-trailing-space-save") && checkBoolValue(val, &state)) {
                qCWarning(LOG_KTE) << i18n("Using deprecated modeline 'replace-trailing-space-save'. "
                                            "Please replace with 'remove-trailing-spaces all;', see "
                                            "http://docs.kde.org/stable/en/applications/kate/config-variables.html#variable-remove-trailing-spaces");
                m_config->setRemoveSpaces(state ? 2 : 0);
            } else if (var == QLatin1String("overwrite-mode") && checkBoolValue(val, &state)) {
                m_config->setOvr(state);
            } else if (var == QLatin1String("keep-extra-spaces") && checkBoolValue(val, &state)) {
                m_config->setKeepExtraSpaces(state);
            } else if (var == QLatin1String("tab-indents") && checkBoolValue(val, &state)) {
                m_config->setTabIndents(state);
            } else if (var == QLatin1String("show-tabs") && checkBoolValue(val, &state)) {
                m_config->setShowTabs(state);
            } else if (var == QLatin1String("show-trailing-spaces") && checkBoolValue(val, &state)) {
                m_config->setShowSpaces(state);
            } else if (var == QLatin1String("space-indent") && checkBoolValue(val, &state)) {
                // this is for backward compatibility; see below
                spaceIndent = state;
            } else if (var == QLatin1String("smart-home") && checkBoolValue(val, &state)) {
                m_config->setSmartHome(state);
            } else if (var == QLatin1String("newline-at-eof") && checkBoolValue(val, &state)) {
                m_config->setNewLineAtEof(state);
            }

            // INTEGER SETTINGS
            else if (var == QLatin1String("tab-width") && checkIntValue(val, &n)) {
                m_config->setTabWidth(n);
            } else if (var == QLatin1String("indent-width")  && checkIntValue(val, &n)) {
                m_config->setIndentationWidth(n);
            } else if (var == QLatin1String("indent-mode")) {
                m_config->setIndentationMode(val);
            } else if (var == QLatin1String("word-wrap-column") && checkIntValue(val, &n) && n > 0) { // uint, but hard word wrap at 0 will be no fun ;)
                m_config->setWordWrapAt(n);
            }

            // STRING SETTINGS
            else if (var == QLatin1String("eol") || var == QLatin1String("end-of-line")) {
                QStringList l;
                l << QStringLiteral("unix") << QStringLiteral("dos") << QStringLiteral("mac");
                if ((n = l.indexOf(val.toLower())) != -1) {
                    /**
                     * set eol + avoid that it is overwritten by auto-detection again!
                     * this fixes e.g. .kateconfig files with // kate: eol dos; to work, bug 365705
                     */
                    m_config->setEol(n);
                    m_config->setAllowEolDetection(false);
                }
            } else if (var == QLatin1String("bom") || var == QLatin1String("byte-order-mark") || var == QLatin1String("byte-order-marker")) {
                if (checkBoolValue(val, &state)) {
                    m_config->setBom(state);
                }
            } else if (var == QLatin1String("remove-trailing-spaces")) {
                val = val.toLower();
                if (val == QLatin1String("1") || val == QLatin1String("modified") || val == QLatin1String("mod") || val == QLatin1String("+")) {
                    m_config->setRemoveSpaces(1);
                } else if (val == QLatin1String("2") || val == QLatin1String("all") || val == QLatin1String("*")) {
                    m_config->setRemoveSpaces(2);
                } else {
                    m_config->setRemoveSpaces(0);
                }
            } else if (var == QLatin1String("syntax") || var == QLatin1String("hl")) {
                setHighlightingMode(val);
            } else if (var == QLatin1String("mode")) {
                setMode(val);
            } else if (var == QLatin1String("encoding")) {
                setEncoding(val);
            } else if (var == QLatin1String("default-dictionary")) {
                setDefaultDictionary(val);
            } else if (var == QLatin1String("automatic-spell-checking") && checkBoolValue(val, &state)) {
                onTheFlySpellCheckingEnabled(state);
            }

            // VIEW SETTINGS
            else if (vvl.contains(var)) {
                setViewVariable(var, val);
            } else {
                m_storedVariables.insert(var, val);
            }
        }
    }

    // Backward compatibility
    // If space-indent was set, but replace-tabs was not set, we assume
    // that the user wants to replace tabulators and set that flag.
    // If both were set, replace-tabs has precedence.
    // At this point spaceIndent is -1 if it was never set,
    // 0 if it was set to off, and 1 if it was set to on.
    // Note that if onlyViewAndRenderer was requested, spaceIndent is -1.
    if (!replaceTabsSet && spaceIndent >= 0) {
        m_config->setReplaceTabsDyn(spaceIndent > 0);
    }
}

void KTextEditor::DocumentPrivate::setViewVariable(QString var, QString val)
{
    KTextEditor::ViewPrivate *v;
    bool state;
    int n;
    QColor c;
    foreach (v, m_views) {
        if (var == QLatin1String("auto-brackets") && checkBoolValue(val, &state)) {
            v->config()->setAutoBrackets(state);
        } else if (var == QLatin1String("dynamic-word-wrap") && checkBoolValue(val, &state)) {
            v->config()->setDynWordWrap(state);
        } else if (var == QLatin1String("persistent-selection") && checkBoolValue(val, &state)) {
            v->config()->setPersistentSelection(state);
        } else if (var == QLatin1String("block-selection")  && checkBoolValue(val, &state)) {
            v->setBlockSelection(state);
        }
        //else if ( var = "dynamic-word-wrap-indicators" )
        else if (var == QLatin1String("line-numbers") && checkBoolValue(val, &state)) {
            v->config()->setLineNumbers(state);
        } else if (var == QLatin1String("icon-border") && checkBoolValue(val, &state)) {
            v->config()->setIconBar(state);
        } else if (var == QLatin1String("folding-markers") && checkBoolValue(val, &state)) {
            v->config()->setFoldingBar(state);
        } else if (var == QLatin1String("folding-preview") && checkBoolValue(val, &state)) {
            v->config()->setFoldingPreview(state);
        } else if (var == QLatin1String("auto-center-lines") && checkIntValue(val, &n)) {
            v->config()->setAutoCenterLines(n);
        } else if (var == QLatin1String("icon-bar-color") && checkColorValue(val, c)) {
            v->renderer()->config()->setIconBarColor(c);
        } else if (var == QLatin1String("scrollbar-minimap") && checkBoolValue(val, &state)) {
            v->config()->setScrollBarMiniMap(state);
        } else if (var == QLatin1String("scrollbar-preview") && checkBoolValue(val, &state)) {
            v->config()->setScrollBarPreview(state);
        }
        // RENDERER
        else if (var == QLatin1String("background-color") && checkColorValue(val, c)) {
            v->renderer()->config()->setBackgroundColor(c);
        } else if (var == QLatin1String("selection-color") && checkColorValue(val, c)) {
            v->renderer()->config()->setSelectionColor(c);
        } else if (var == QLatin1String("current-line-color") && checkColorValue(val, c)) {
            v->renderer()->config()->setHighlightedLineColor(c);
        } else if (var == QLatin1String("bracket-highlight-color") && checkColorValue(val, c)) {
            v->renderer()->config()->setHighlightedBracketColor(c);
        } else if (var == QLatin1String("word-wrap-marker-color") && checkColorValue(val, c)) {
            v->renderer()->config()->setWordWrapMarkerColor(c);
        } else if (var == QLatin1String("font") || (checkIntValue(val, &n) && var == QLatin1String("font-size"))) {
            QFont _f(v->renderer()->config()->font());

            if (var == QLatin1String("font")) {
                _f.setFamily(val);
                _f.setFixedPitch(QFont(val).fixedPitch());
            } else {
                _f.setPointSize(n);
            }

            v->renderer()->config()->setFont(_f);
        } else if (var == QLatin1String("scheme")) {
            v->renderer()->config()->setSchema(val);
        }
    }
}

bool KTextEditor::DocumentPrivate::checkBoolValue(QString val, bool *result)
{
    val = val.trimmed().toLower();
    static const QStringList trueValues = QStringList() << QStringLiteral("1") << QStringLiteral("on") << QStringLiteral("true");
    if (trueValues.contains(val)) {
        *result = true;
        return true;
    }

    static const QStringList falseValues = QStringList() << QStringLiteral("0") << QStringLiteral("off") << QStringLiteral("false");
    if (falseValues.contains(val)) {
        *result = false;
        return true;
    }
    return false;
}

bool KTextEditor::DocumentPrivate::checkIntValue(QString val, int *result)
{
    bool ret(false);
    *result = val.toInt(&ret);
    return ret;
}

bool KTextEditor::DocumentPrivate::checkColorValue(QString val, QColor &c)
{
    c.setNamedColor(val);
    return c.isValid();
}

// KTextEditor::variable
QString KTextEditor::DocumentPrivate::variable(const QString &name) const
{
    return m_storedVariables.value(name, QString());
}

void KTextEditor::DocumentPrivate::setVariable(const QString &name, const QString &value)
{
    QString s = QStringLiteral("kate: ");
    s.append(name);
    s.append(QLatin1Char(' '));
    s.append(value);
    readVariableLine(s);
}

//END

void KTextEditor::DocumentPrivate::slotModOnHdDirty(const QString &path)
{
    if ((path == m_dirWatchFile) && (!m_modOnHd || m_modOnHdReason != OnDiskModified)) {
        m_modOnHd = true;
        m_modOnHdReason = OnDiskModified;

        if (!m_modOnHdTimer.isActive()) {
            m_modOnHdTimer.start();
        }
    }
}

void KTextEditor::DocumentPrivate::slotModOnHdCreated(const QString &path)
{
    if ((path == m_dirWatchFile) && (!m_modOnHd || m_modOnHdReason != OnDiskCreated)) {
        m_modOnHd = true;
        m_modOnHdReason = OnDiskCreated;

        if (!m_modOnHdTimer.isActive()) {
            m_modOnHdTimer.start();
        }
    }
}

void KTextEditor::DocumentPrivate::slotModOnHdDeleted(const QString &path)
{
    if ((path == m_dirWatchFile) && (!m_modOnHd || m_modOnHdReason != OnDiskDeleted)) {
        m_modOnHd = true;
        m_modOnHdReason = OnDiskDeleted;

        if (!m_modOnHdTimer.isActive()) {
            m_modOnHdTimer.start();
        }
    }
}

void KTextEditor::DocumentPrivate::slotDelayedHandleModOnHd()
{
    // compare git hash with the one we have (if we have one)
    const QByteArray oldDigest = checksum();
    if (!oldDigest.isEmpty() && !url().isEmpty() && url().isLocalFile()) {
        /**
         * if current checksum == checksum of new file => unmodified
         */
        if (m_modOnHdReason != OnDiskDeleted && createDigest() && oldDigest == checksum()) {
            m_modOnHd = false;
            m_modOnHdReason = OnDiskUnmodified;
            m_prevModOnHdReason = OnDiskUnmodified;
        }

#ifdef LIBGIT2_FOUND
        /**
         * if still modified, try to take a look at git
         * skip that, if document is modified!
         * only do that, if the file is still there, else reload makes no sense!
         */
        if (m_modOnHd && !isModified() && QFile::exists(url().toLocalFile())) {
            /**
             * try to discover the git repo of this file
             * libgit2 docs state that UTF-8 is the right encoding, even on windows
             * I hope that is correct!
             */
            git_repository *repository = nullptr;
            const QByteArray utf8Path = url().toLocalFile().toUtf8();
            if (git_repository_open_ext(&repository, utf8Path.constData(), 0, nullptr) == 0) {
                /**
                 * if we have repo, convert the git hash to an OID
                 */
                git_oid oid;
                if (git_oid_fromstr(&oid, oldDigest.toHex().data()) == 0) {
                    /**
                     * finally: is there a blob for this git hash?
                     */
                    git_blob *blob = nullptr;
                    if (git_blob_lookup(&blob, repository, &oid) == 0) {
                        /**
                        * this hash exists still in git => just reload
                        */
                        m_modOnHd = false;
                        m_modOnHdReason = OnDiskUnmodified;
                        m_prevModOnHdReason = OnDiskUnmodified;
                        documentReload();
                    }
                    git_blob_free(blob);
                }

            }
            git_repository_free(repository);
        }
#endif
    }

    /**
     * emit our signal to the outside!
     */
    emit modifiedOnDisk(this, m_modOnHd, m_modOnHdReason);
}

QByteArray KTextEditor::DocumentPrivate::checksum() const
{
    return m_buffer->digest();
}

bool KTextEditor::DocumentPrivate::createDigest()
{
    QByteArray digest;

    if (url().isLocalFile()) {
        QFile f(url().toLocalFile());
        if (f.open(QIODevice::ReadOnly)) {
            // init the hash with the git header
            QCryptographicHash crypto(QCryptographicHash::Sha1);
            const QString header = QStringLiteral("blob %1").arg(f.size());
            crypto.addData(header.toLatin1() + '\0');

            while (!f.atEnd()) {
                crypto.addData(f.read(256 * 1024));
            }

            digest = crypto.result();
        }
    }

    /**
     * set new digest
     */
    m_buffer->setDigest(digest);
    return !digest.isEmpty();
}

QString KTextEditor::DocumentPrivate::reasonedMOHString() const
{
    // squeeze path
    const QString str = KStringHandler::csqueeze(url().toDisplayString(QUrl::PreferLocalFile));

    switch (m_modOnHdReason) {
    case OnDiskModified:
        return i18n("The file '%1' was modified by another program.", str);
        break;
    case OnDiskCreated:
        return i18n("The file '%1' was created by another program.", str);
        break;
    case OnDiskDeleted:
        return i18n("The file '%1' was deleted by another program.", str);
        break;
    default:
        return QString();
    }
    Q_UNREACHABLE();
    return QString();
}

void KTextEditor::DocumentPrivate::removeTrailingSpaces()
{
    const int remove = config()->removeSpaces();
    if (remove == 0) {
        return;
    }

    // temporarily disable static word wrap (see bug #328900)
    const bool wordWrapEnabled = config()->wordWrap();
    if (wordWrapEnabled) {
        setWordWrap(false);
    }

    editStart();

    for (int line = 0; line < lines(); ++line) {
        Kate::TextLine textline = plainKateTextLine(line);

        // remove trailing spaces in entire document, remove = 2
        // remove trailing spaces of touched lines, remove = 1
        // remove trailing spaces of lines saved on disk, remove = 1
        if (remove == 2 || textline->markedAsModified() || textline->markedAsSavedOnDisk()) {
            const int p = textline->lastChar() + 1;
            const int l = textline->length() - p;
            if (l > 0) {
                editRemoveText(line, p, l);
            }
        }
    }

    editEnd();

    // enable word wrap again, if it was enabled (see bug #328900)
    if (wordWrapEnabled) {
        setWordWrap(true);    // see begin of this function
    }
}

void KTextEditor::DocumentPrivate::updateFileType(const QString &newType, bool user)
{
    if (user || !m_fileTypeSetByUser) {
        if (!newType.isEmpty()) {
            // remember that we got set by user
            m_fileTypeSetByUser = user;

            m_fileType = newType;

            m_config->configStart();

            if (!m_hlSetByUser && !KTextEditor::EditorPrivate::self()->modeManager()->fileType(newType).hl.isEmpty()) {
                int hl(KateHlManager::self()->nameFind(KTextEditor::EditorPrivate::self()->modeManager()->fileType(newType).hl));

                if (hl >= 0) {
                    m_buffer->setHighlight(hl);
                }
            }

            /**
             * set the indentation mode, if any in the mode...
             * and user did not set it before!
             */
            if (!m_indenterSetByUser && !KTextEditor::EditorPrivate::self()->modeManager()->fileType(newType).indenter.isEmpty()) {
                config()->setIndentationMode(KTextEditor::EditorPrivate::self()->modeManager()->fileType(newType).indenter);
            }

            // views!
            KTextEditor::ViewPrivate *v;
            foreach (v, m_views) {
                v->config()->configStart();
                v->renderer()->config()->configStart();
            }

            bool bom_settings = false;
            if (m_bomSetByUser) {
                bom_settings = m_config->bom();
            }
            readVariableLine(KTextEditor::EditorPrivate::self()->modeManager()->fileType(newType).varLine);
            if (m_bomSetByUser) {
                m_config->setBom(bom_settings);
            }
            m_config->configEnd();
            foreach (v, m_views) {
                v->config()->configEnd();
                v->renderer()->config()->configEnd();
            }
        }
    }

    // fixme, make this better...
    emit modeChanged(this);
}

void KTextEditor::DocumentPrivate::slotQueryClose_save(bool *handled, bool *abortClosing)
{
    *handled = true;
    *abortClosing = true;
    if (this->url().isEmpty()) {
        QWidget *parentWidget(dialogParent());
        const QUrl res = QFileDialog::getSaveFileUrl(parentWidget, i18n("Save File"), QUrl(), {}, nullptr,
                                                     QFileDialog::DontConfirmOverwrite);
        if (res.isEmpty() || !checkOverwrite(res, parentWidget)) {
            *abortClosing = true;
            return;
        }
        saveAs(res);
        *abortClosing = false;
    } else {
        save();
        *abortClosing = false;
    }

}

bool KTextEditor::DocumentPrivate::checkOverwrite(QUrl u, QWidget *parent)
{
    if (!u.isLocalFile()) {
        return true;
    }

    QFileInfo info(u.path());
    if (!info.exists()) {
        return true;
    }

    return KMessageBox::Cancel != KMessageBox::warningContinueCancel(parent,
            i18n("A file named \"%1\" already exists. "
                 "Are you sure you want to overwrite it?",  info.fileName()),
            i18n("Overwrite File?"), KStandardGuiItem::overwrite(),
            KStandardGuiItem::cancel(), QString(), KMessageBox::Options(KMessageBox::Notify | KMessageBox::Dangerous));
}

//BEGIN KTextEditor::ConfigInterface

// BEGIN ConfigInterface stff
QStringList KTextEditor::DocumentPrivate::configKeys() const
{
    static const QStringList keys = {
        QStringLiteral("backup-on-save-local"),
        QStringLiteral("backup-on-save-suffix"),
        QStringLiteral("backup-on-save-prefix"),
        QStringLiteral("replace-tabs"),
        QStringLiteral("indent-pasted-text"),
        QStringLiteral("tab-width"),
        QStringLiteral("indent-width"),
    };
    return keys;
}

QVariant KTextEditor::DocumentPrivate::configValue(const QString &key)
{
    if (key == QLatin1String("backup-on-save-local")) {
        return m_config->backupFlags() & KateDocumentConfig::LocalFiles;
    } else if (key == QLatin1String("backup-on-save-remote")) {
        return m_config->backupFlags() & KateDocumentConfig::RemoteFiles;
    } else if (key == QLatin1String("backup-on-save-suffix")) {
        return m_config->backupSuffix();
    } else if (key == QLatin1String("backup-on-save-prefix")) {
        return m_config->backupPrefix();
    } else if (key == QLatin1String("replace-tabs")) {
        return m_config->replaceTabsDyn();
    } else if (key == QLatin1String("indent-pasted-text")) {
        return m_config->indentPastedText();
    } else if (key == QLatin1String("tab-width")) {
        return m_config->tabWidth();
    } else if (key == QLatin1String("indent-width")) {
        return m_config->indentationWidth();
    }

    // return invalid variant
    return QVariant();
}

void KTextEditor::DocumentPrivate::setConfigValue(const QString &key, const QVariant &value)
{
    if (value.type() == QVariant::String) {
        if (key == QLatin1String("backup-on-save-suffix")) {
            m_config->setBackupSuffix(value.toString());
        } else if (key == QLatin1String("backup-on-save-prefix")) {
            m_config->setBackupPrefix(value.toString());
        }
    } else if (value.canConvert(QVariant::Bool)) {
        const bool bValue = value.toBool();
        if (key == QLatin1String("backup-on-save-local") && value.type() == QVariant::String) {
            uint f = m_config->backupFlags();
            if (bValue) {
                f |= KateDocumentConfig::LocalFiles;
            } else {
                f ^= KateDocumentConfig::LocalFiles;
            }

            m_config->setBackupFlags(f);
        } else if (key == QLatin1String("backup-on-save-remote")) {
            uint f = m_config->backupFlags();
            if (bValue) {
                f |= KateDocumentConfig::RemoteFiles;
            } else {
                f ^= KateDocumentConfig::RemoteFiles;
            }

            m_config->setBackupFlags(f);
        } else if (key == QLatin1String("replace-tabs")) {
            m_config->setReplaceTabsDyn(bValue);
        } else if (key == QLatin1String("indent-pasted-text")) {
            m_config->setIndentPastedText(bValue);
        }
    } else if (value.canConvert(QVariant::Int)) {
        if (key == QLatin1String("tab-width")) {
            config()->setTabWidth(value.toInt());
        } else if (key == QLatin1String("indent-width")) {
            config()->setIndentationWidth(value.toInt());
        }
    }
}

//END KTextEditor::ConfigInterface

KTextEditor::Cursor KTextEditor::DocumentPrivate::documentEnd() const
{
    return KTextEditor::Cursor(lastLine(), lineLength(lastLine()));
}

bool KTextEditor::DocumentPrivate::replaceText(const KTextEditor::Range &range, const QString &s, bool block)
{
    // TODO more efficient?
    editStart();
    bool changed = removeText(range, block);
    changed |= insertText(range.start(), s, block);
    editEnd();
    return changed;
}

KateHighlighting *KTextEditor::DocumentPrivate::highlight() const
{
    return m_buffer->highlight();
}

Kate::TextLine KTextEditor::DocumentPrivate::kateTextLine(int i)
{
    m_buffer->ensureHighlighted(i);
    return m_buffer->plainLine(i);
}

Kate::TextLine KTextEditor::DocumentPrivate::plainKateTextLine(int i)
{
    return m_buffer->plainLine(i);
}

bool KTextEditor::DocumentPrivate::isEditRunning() const
{
    return editIsRunning;
}

void KTextEditor::DocumentPrivate::setUndoMergeAllEdits(bool merge)
{
    if (merge && m_undoMergeAllEdits) {
        // Don't add another undo safe point: it will override our current one,
        // meaning we'll need two undo's to get back there - which defeats the object!
        return;
    }
    m_undoManager->undoSafePoint();
    m_undoManager->setAllowComplexMerge(merge);
    m_undoMergeAllEdits = merge;
}

//BEGIN KTextEditor::MovingInterface
KTextEditor::MovingCursor *KTextEditor::DocumentPrivate::newMovingCursor(const KTextEditor::Cursor &position, KTextEditor::MovingCursor::InsertBehavior insertBehavior)
{
    return new Kate::TextCursor(buffer(), position, insertBehavior);
}

KTextEditor::MovingRange *KTextEditor::DocumentPrivate::newMovingRange(const KTextEditor::Range &range, KTextEditor::MovingRange::InsertBehaviors insertBehaviors, KTextEditor::MovingRange::EmptyBehavior emptyBehavior)
{
    return new Kate::TextRange(buffer(), range, insertBehaviors, emptyBehavior);
}

qint64 KTextEditor::DocumentPrivate::revision() const
{
    return m_buffer->history().revision();
}

qint64 KTextEditor::DocumentPrivate::lastSavedRevision() const
{
    return m_buffer->history().lastSavedRevision();
}

void KTextEditor::DocumentPrivate::lockRevision(qint64 revision)
{
    m_buffer->history().lockRevision(revision);
}

void KTextEditor::DocumentPrivate::unlockRevision(qint64 revision)
{
    m_buffer->history().unlockRevision(revision);
}

void KTextEditor::DocumentPrivate::transformCursor(int &line, int &column, KTextEditor::MovingCursor::InsertBehavior insertBehavior, qint64 fromRevision, qint64 toRevision)
{
    m_buffer->history().transformCursor(line, column, insertBehavior, fromRevision, toRevision);
}

void KTextEditor::DocumentPrivate::transformCursor(KTextEditor::Cursor &cursor, KTextEditor::MovingCursor::InsertBehavior insertBehavior, qint64 fromRevision, qint64 toRevision)
{
    int line = cursor.line(), column = cursor.column();
    m_buffer->history().transformCursor(line, column, insertBehavior, fromRevision, toRevision);
    cursor.setLine(line);
    cursor.setColumn(column);
}

void KTextEditor::DocumentPrivate::transformRange(KTextEditor::Range &range, KTextEditor::MovingRange::InsertBehaviors insertBehaviors, KTextEditor::MovingRange::EmptyBehavior emptyBehavior, qint64 fromRevision, qint64 toRevision)
{
    m_buffer->history().transformRange(range, insertBehaviors, emptyBehavior, fromRevision, toRevision);
}

//END

//BEGIN KTextEditor::AnnotationInterface
void KTextEditor::DocumentPrivate::setAnnotationModel(KTextEditor::AnnotationModel *model)
{
    KTextEditor::AnnotationModel *oldmodel = m_annotationModel;
    m_annotationModel = model;
    emit annotationModelChanged(oldmodel, m_annotationModel);
}

KTextEditor::AnnotationModel *KTextEditor::DocumentPrivate::annotationModel() const
{
    return m_annotationModel;
}
//END KTextEditor::AnnotationInterface

//TAKEN FROM kparts.h
bool KTextEditor::DocumentPrivate::queryClose()
{
    if (!isReadWrite() || !isModified()) {
        return true;
    }

    QString docName = documentName();

    int res = KMessageBox::warningYesNoCancel(dialogParent(),
              i18n("The document \"%1\" has been modified.\n"
                   "Do you want to save your changes or discard them?",  docName),
              i18n("Close Document"), KStandardGuiItem::save(), KStandardGuiItem::discard());

    bool abortClose = false;
    bool handled = false;

    switch (res) {
    case KMessageBox::Yes :
        sigQueryClose(&handled, &abortClose);
        if (!handled) {
            if (url().isEmpty()) {
                QUrl url = QFileDialog::getSaveFileUrl(dialogParent());
                if (url.isEmpty()) {
                    return false;
                }

                saveAs(url);
            } else {
                save();
            }
        } else if (abortClose) {
            return false;
        }
        return waitSaveComplete();
    case KMessageBox::No :
        return true;
    default : // case KMessageBox::Cancel :
        return false;
    }
}

void KTextEditor::DocumentPrivate::slotStarted(KIO::Job *job)
{
    /**
     * if we are idle before, we are now loading!
     */
    if (m_documentState == DocumentIdle) {
        m_documentState = DocumentLoading;
    }

    /**
     * if loading:
     * - remember pre loading read-write mode
     * if remote load:
     * - set to read-only
     * - trigger possible message
     */
    if (m_documentState == DocumentLoading) {
        /**
         * remember state
         */
        m_readWriteStateBeforeLoading = isReadWrite();

        /**
         * perhaps show loading message, but wait one second
         */
        if (job) {
            /**
             * only read only if really remote file!
             */
            setReadWrite(false);

            /**
             * perhaps some message about loading in one second!
             * remember job pointer, we want to be able to kill it!
             */
            m_loadingJob = job;
            QTimer::singleShot(1000, this, SLOT(slotTriggerLoadingMessage()));
        }
    }
}

void KTextEditor::DocumentPrivate::slotCompleted()
{
    /**
     * if were loading, reset back to old read-write mode before loading
     * and kill the possible loading message
     */
    if (m_documentState == DocumentLoading) {
        setReadWrite(m_readWriteStateBeforeLoading);
        delete m_loadingMessage;
    }

    /**
     * Emit signal that we saved  the document, if needed
     */
    if (m_documentState == DocumentSaving || m_documentState == DocumentSavingAs) {
        emit documentSavedOrUploaded(this, m_documentState == DocumentSavingAs);
    }

    /**
     * back to idle mode
     */
    m_documentState = DocumentIdle;
    m_reloading = false;
}

void KTextEditor::DocumentPrivate::slotCanceled()
{
    /**
     * if were loading, reset back to old read-write mode before loading
     * and kill the possible loading message
     */
    if (m_documentState == DocumentLoading) {
        setReadWrite(m_readWriteStateBeforeLoading);
        delete m_loadingMessage;

        showAndSetOpeningErrorAccess();

        updateDocName();
    }

    /**
     * back to idle mode
     */
    m_documentState = DocumentIdle;
    m_reloading = false;

}

void KTextEditor::DocumentPrivate::slotTriggerLoadingMessage()
{
    /**
     * no longer loading?
     * no message needed!
     */
    if (m_documentState != DocumentLoading) {
        return;
    }

    /**
     * create message about file loading in progress
     */
    delete m_loadingMessage;
    m_loadingMessage = new KTextEditor::Message(i18n("The file <a href=\"%1\">%2</a> is still loading.", url().toDisplayString(QUrl::PreferLocalFile), url().fileName()));
    m_loadingMessage->setPosition(KTextEditor::Message::TopInView);

    /**
     * if around job: add cancel action
     */
    if (m_loadingJob) {
        QAction *cancel = new QAction(i18n("&Abort Loading"), nullptr);
        connect(cancel, SIGNAL(triggered()), this, SLOT(slotAbortLoading()));
        m_loadingMessage->addAction(cancel);
    }

    /**
     * really post message
     */
    postMessage(m_loadingMessage);
}

void KTextEditor::DocumentPrivate::slotAbortLoading()
{
    /**
     * no job, no work
     */
    if (!m_loadingJob) {
        return;
    }

    /**
     * abort loading if any job
     * signal results!
     */
    m_loadingJob->kill(KJob::EmitResult);
    m_loadingJob = nullptr;
}

void KTextEditor::DocumentPrivate::slotUrlChanged(const QUrl &url)
{
    if (m_reloading) {
        // the URL is temporarily unset and then reset to the previous URL during reload
        // we do not want to notify the outside about this
        return;
    }

    Q_UNUSED(url);
    updateDocName();
    emit documentUrlChanged(this);
}

bool KTextEditor::DocumentPrivate::save()
{
    /**
     * no double save/load
     * we need to allow DocumentPreSavingAs here as state, as save is called in saveAs!
     */
    if ((m_documentState != DocumentIdle) && (m_documentState != DocumentPreSavingAs)) {
        return false;
    }

    /**
     * if we are idle, we are now saving
     */
    if (m_documentState == DocumentIdle) {
        m_documentState = DocumentSaving;
    } else {
        m_documentState = DocumentSavingAs;
    }

    /**
     * call back implementation for real work
     */
    return KTextEditor::Document::save();
}

bool KTextEditor::DocumentPrivate::saveAs(const QUrl &url)
{
    /**
     * abort on bad URL
     * that is done in saveAs below, too
     * but we must check it here already to avoid messing up
     * as no signals will be send, then
     */
    if (!url.isValid()) {
        return false;
    }

    /**
     * no double save/load
     */
    if (m_documentState != DocumentIdle) {
        return false;
    }

    /**
     * we enter the pre save as phase
     */
    m_documentState = DocumentPreSavingAs;

    /**
     * call base implementation for real work
     */
    return KTextEditor::Document::saveAs(normalizeUrl(url));
}

QString KTextEditor::DocumentPrivate::defaultDictionary() const
{
    return m_defaultDictionary;
}

QList<QPair<KTextEditor::MovingRange *, QString> > KTextEditor::DocumentPrivate::dictionaryRanges() const
{
    return m_dictionaryRanges;
}

void KTextEditor::DocumentPrivate::clearDictionaryRanges()
{
    for (QList<QPair<KTextEditor::MovingRange *, QString> >::iterator i = m_dictionaryRanges.begin();
            i != m_dictionaryRanges.end(); ++i) {
        delete(*i).first;
    }
    m_dictionaryRanges.clear();
    if (m_onTheFlyChecker) {
        m_onTheFlyChecker->refreshSpellCheck();
    }
    emit dictionaryRangesPresent(false);
}

void KTextEditor::DocumentPrivate::setDictionary(const QString &newDictionary, const KTextEditor::Range &range)
{
    KTextEditor::Range newDictionaryRange = range;
    if (!newDictionaryRange.isValid() || newDictionaryRange.isEmpty()) {
        return;
    }
    QList<QPair<KTextEditor::MovingRange *, QString> > newRanges;
    // all ranges is 'm_dictionaryRanges' are assumed to be mutually disjoint
    for (QList<QPair<KTextEditor::MovingRange *, QString> >::iterator i = m_dictionaryRanges.begin();
            i != m_dictionaryRanges.end();) {
        qCDebug(LOG_KTE) << "new iteration" << newDictionaryRange;
        if (newDictionaryRange.isEmpty()) {
            break;
        }
        QPair<KTextEditor::MovingRange *, QString> pair = *i;
        QString dictionarySet = pair.second;
        KTextEditor::MovingRange *dictionaryRange = pair.first;
        qCDebug(LOG_KTE) << *dictionaryRange << dictionarySet;
        if (dictionaryRange->contains(newDictionaryRange) && newDictionary == dictionarySet) {
            qCDebug(LOG_KTE) << "dictionaryRange contains newDictionaryRange";
            return;
        }
        if (newDictionaryRange.contains(*dictionaryRange)) {
            delete dictionaryRange;
            i = m_dictionaryRanges.erase(i);
            qCDebug(LOG_KTE) << "newDictionaryRange contains dictionaryRange";
            continue;
        }

        KTextEditor::Range intersection = dictionaryRange->toRange().intersect(newDictionaryRange);
        if (!intersection.isEmpty() && intersection.isValid()) {
            if (dictionarySet == newDictionary) { // we don't have to do anything for 'intersection'
                // except cut off the intersection
                QList<KTextEditor::Range> remainingRanges = KateSpellCheckManager::rangeDifference(newDictionaryRange, intersection);
                Q_ASSERT(remainingRanges.size() == 1);
                newDictionaryRange = remainingRanges.first();
                ++i;
                qCDebug(LOG_KTE) << "dictionarySet == newDictionary";
                continue;
            }
            QList<KTextEditor::Range> remainingRanges = KateSpellCheckManager::rangeDifference(*dictionaryRange, intersection);
            for (QList<KTextEditor::Range>::iterator j = remainingRanges.begin(); j != remainingRanges.end(); ++j) {
                KTextEditor::MovingRange *remainingRange = newMovingRange(*j,
                        KTextEditor::MovingRange::ExpandLeft | KTextEditor::MovingRange::ExpandRight);
                remainingRange->setFeedback(this);
                newRanges.push_back(QPair<KTextEditor::MovingRange *, QString>(remainingRange, dictionarySet));
            }
            i = m_dictionaryRanges.erase(i);
            delete dictionaryRange;
        } else {
            ++i;
        }
    }
    m_dictionaryRanges += newRanges;
    if (!newDictionaryRange.isEmpty() && !newDictionary.isEmpty()) { // we don't add anything for the default dictionary
        KTextEditor::MovingRange *newDictionaryMovingRange = newMovingRange(newDictionaryRange,
                KTextEditor::MovingRange::ExpandLeft | KTextEditor::MovingRange::ExpandRight);
        newDictionaryMovingRange->setFeedback(this);
        m_dictionaryRanges.push_back(QPair<KTextEditor::MovingRange *, QString>(newDictionaryMovingRange, newDictionary));
    }
    if (m_onTheFlyChecker && !newDictionaryRange.isEmpty()) {
        m_onTheFlyChecker->refreshSpellCheck(newDictionaryRange);
    }
    emit dictionaryRangesPresent(!m_dictionaryRanges.isEmpty());
}

void KTextEditor::DocumentPrivate::revertToDefaultDictionary(const KTextEditor::Range &range)
{
    setDictionary(QString(), range);
}

void KTextEditor::DocumentPrivate::setDefaultDictionary(const QString &dict)
{
    if (m_defaultDictionary == dict) {
        return;
    }

    m_defaultDictionary = dict;

    if (m_onTheFlyChecker) {
        m_onTheFlyChecker->updateConfig();
        refreshOnTheFlyCheck();
    }
    emit defaultDictionaryChanged(this);
}

void KTextEditor::DocumentPrivate::onTheFlySpellCheckingEnabled(bool enable)
{
    if (isOnTheFlySpellCheckingEnabled() == enable) {
        return;
    }

    if (enable) {
        Q_ASSERT(m_onTheFlyChecker == nullptr);
        m_onTheFlyChecker = new KateOnTheFlyChecker(this);
    } else {
        delete m_onTheFlyChecker;
        m_onTheFlyChecker = nullptr;
    }

    foreach (KTextEditor::ViewPrivate *view, m_views) {
        view->reflectOnTheFlySpellCheckStatus(enable);
    }
}

bool KTextEditor::DocumentPrivate::isOnTheFlySpellCheckingEnabled() const
{
    return m_onTheFlyChecker != nullptr;
}

QString KTextEditor::DocumentPrivate::dictionaryForMisspelledRange(const KTextEditor::Range &range) const
{
    if (!m_onTheFlyChecker) {
        return QString();
    } else {
        return m_onTheFlyChecker->dictionaryForMisspelledRange(range);
    }
}

void KTextEditor::DocumentPrivate::clearMisspellingForWord(const QString &word)
{
    if (m_onTheFlyChecker) {
        m_onTheFlyChecker->clearMisspellingForWord(word);
    }
}

void KTextEditor::DocumentPrivate::refreshOnTheFlyCheck(const KTextEditor::Range &range)
{
    if (m_onTheFlyChecker) {
        m_onTheFlyChecker->refreshSpellCheck(range);
    }
}

void KTextEditor::DocumentPrivate::rangeInvalid(KTextEditor::MovingRange *movingRange)
{
    deleteDictionaryRange(movingRange);
}

void KTextEditor::DocumentPrivate::rangeEmpty(KTextEditor::MovingRange *movingRange)
{
    deleteDictionaryRange(movingRange);
}

void KTextEditor::DocumentPrivate::deleteDictionaryRange(KTextEditor::MovingRange *movingRange)
{
    qCDebug(LOG_KTE) << "deleting" << movingRange;

    auto finder = [=] (const QPair<KTextEditor::MovingRange *, QString>& item) -> bool {
        return item.first == movingRange;
    };

    auto it = std::find_if(m_dictionaryRanges.begin(), m_dictionaryRanges.end(), finder);

    if (it != m_dictionaryRanges.end()) {
        m_dictionaryRanges.erase(it);
        delete movingRange;
    }

    Q_ASSERT(std::find_if(m_dictionaryRanges.begin(), m_dictionaryRanges.end(), finder) == m_dictionaryRanges.end());
}

bool KTextEditor::DocumentPrivate::containsCharacterEncoding(const KTextEditor::Range &range)
{
    KateHighlighting *highlighting = highlight();
    Kate::TextLine textLine;

    const int rangeStartLine = range.start().line();
    const int rangeStartColumn = range.start().column();
    const int rangeEndLine = range.end().line();
    const int rangeEndColumn = range.end().column();

    for (int line = range.start().line(); line <= rangeEndLine; ++line) {
        textLine = kateTextLine(line);
        int startColumn = (line == rangeStartLine) ? rangeStartColumn : 0;
        int endColumn = (line == rangeEndLine) ? rangeEndColumn : textLine->length();
        for (int col = startColumn; col < endColumn; ++col) {
            int attr = textLine->attribute(col);
            const KatePrefixStore &prefixStore = highlighting->getCharacterEncodingsPrefixStore(attr);
            if (!prefixStore.findPrefix(textLine, col).isEmpty()) {
                return true;
            }
        }
    }

    return false;
}

int KTextEditor::DocumentPrivate::computePositionWrtOffsets(const OffsetList &offsetList, int pos)
{
    int previousOffset = 0;
    for (OffsetList::const_iterator i = offsetList.begin(); i != offsetList.end(); ++i) {
        if ((*i).first > pos) {
            break;
        }
        previousOffset = (*i).second;
    }
    return pos + previousOffset;
}

QString KTextEditor::DocumentPrivate::decodeCharacters(const KTextEditor::Range &range, KTextEditor::DocumentPrivate::OffsetList &decToEncOffsetList,
                                       KTextEditor::DocumentPrivate::OffsetList &encToDecOffsetList)
{
    QString toReturn;
    KTextEditor::Cursor previous = range.start();
    int decToEncCurrentOffset = 0, encToDecCurrentOffset = 0;
    int i = 0;
    int newI = 0;

    KateHighlighting *highlighting = highlight();
    Kate::TextLine textLine;

    const int rangeStartLine = range.start().line();
    const int rangeStartColumn = range.start().column();
    const int rangeEndLine = range.end().line();
    const int rangeEndColumn = range.end().column();

    for (int line = range.start().line(); line <= rangeEndLine; ++line) {
        textLine = kateTextLine(line);
        int startColumn = (line == rangeStartLine) ? rangeStartColumn : 0;
        int endColumn = (line == rangeEndLine) ? rangeEndColumn : textLine->length();
        for (int col = startColumn; col < endColumn;) {
            int attr = textLine->attribute(col);
            const KatePrefixStore &prefixStore = highlighting->getCharacterEncodingsPrefixStore(attr);
            const QHash<QString, QChar> &characterEncodingsHash = highlighting->getCharacterEncodings(attr);
            QString matchingPrefix = prefixStore.findPrefix(textLine, col);
            if (!matchingPrefix.isEmpty()) {
                toReturn += text(KTextEditor::Range(previous, KTextEditor::Cursor(line, col)));
                const QChar &c = characterEncodingsHash.value(matchingPrefix);
                const bool isNullChar = c.isNull();
                if (!c.isNull()) {
                    toReturn += c;
                }
                i += matchingPrefix.length();
                col += matchingPrefix.length();
                previous = KTextEditor::Cursor(line, col);
                decToEncCurrentOffset = decToEncCurrentOffset - (isNullChar ? 0 : 1) + matchingPrefix.length();
                encToDecCurrentOffset = encToDecCurrentOffset - matchingPrefix.length() + (isNullChar ? 0 : 1);
                newI += (isNullChar ? 0 : 1);
                decToEncOffsetList.push_back(QPair<int, int>(newI, decToEncCurrentOffset));
                encToDecOffsetList.push_back(QPair<int, int>(i, encToDecCurrentOffset));
                continue;
            }
            ++col;
            ++i;
            ++newI;
        }
        ++i;
        ++newI;
    }
    if (previous < range.end()) {
        toReturn += text(KTextEditor::Range(previous, range.end()));
    }
    return toReturn;
}

void KTextEditor::DocumentPrivate::replaceCharactersByEncoding(const KTextEditor::Range &range)
{
    KateHighlighting *highlighting = highlight();
    Kate::TextLine textLine;

    const int rangeStartLine = range.start().line();
    const int rangeStartColumn = range.start().column();
    const int rangeEndLine = range.end().line();
    const int rangeEndColumn = range.end().column();

    for (int line = range.start().line(); line <= rangeEndLine; ++line) {
        textLine = kateTextLine(line);
        int startColumn = (line == rangeStartLine) ? rangeStartColumn : 0;
        int endColumn = (line == rangeEndLine) ? rangeEndColumn : textLine->length();
        for (int col = startColumn; col < endColumn;) {
            int attr = textLine->attribute(col);
            const QHash<QChar, QString> &reverseCharacterEncodingsHash = highlighting->getReverseCharacterEncodings(attr);
            QHash<QChar, QString>::const_iterator it = reverseCharacterEncodingsHash.find(textLine->at(col));
            if (it != reverseCharacterEncodingsHash.end()) {
                replaceText(KTextEditor::Range(line, col, line, col + 1), *it);
                col += (*it).length();
                continue;
            }
            ++col;
        }
    }
}

//
// Highlighting information
//

KTextEditor::Attribute::Ptr KTextEditor::DocumentPrivate::attributeAt(const KTextEditor::Cursor &position)
{
    KTextEditor::Attribute::Ptr attrib(new KTextEditor::Attribute());

    KTextEditor::ViewPrivate *view = m_views.empty() ? nullptr : m_views.begin().value();
    if (!view) {
        qCWarning(LOG_KTE) << "ATTENTION: cannot access lineAttributes() without any View (will be fixed eventually)";
        return attrib;
    }

    Kate::TextLine kateLine = kateTextLine(position.line());
    if (!kateLine) {
        return attrib;
    }

    *attrib = *view->renderer()->attribute(kateLine->attribute(position.column()));

    return attrib;
}

QStringList KTextEditor::DocumentPrivate::embeddedHighlightingModes() const
{
    return highlight()->getEmbeddedHighlightingModes();
}

QString KTextEditor::DocumentPrivate::highlightingModeAt(const KTextEditor::Cursor &position)
{
    Kate::TextLine kateLine = kateTextLine(position.line());

//   const QVector< short >& attrs = kateLine->ctxArray();
//   qCDebug(LOG_KTE) << "----------------------------------------------------------------------";
//   foreach( short a, attrs ) {
//     qCDebug(LOG_KTE) << a;
//   }
//   qCDebug(LOG_KTE) << "----------------------------------------------------------------------";
//   qCDebug(LOG_KTE) << "col: " << position.column() << " lastchar:" << kateLine->lastChar() << " length:" << kateLine->length() << "global mode:" << highlightingMode();

    int len = kateLine->length();
    int pos = position.column();
    if (pos >= len) {
        const Kate::TextLineData::ContextStack &ctxs = kateLine->contextStack();
        int ctxcnt = ctxs.count();
        if (ctxcnt == 0) {
            return highlightingMode();
        }
        int ctx = ctxs.at(ctxcnt - 1);
        if (ctx == 0) {
            return highlightingMode();
        }
        return KateHlManager::self()->nameForIdentifier(highlight()->hlKeyForContext(ctx));
    }

    int attr = kateLine->attribute(pos);
    if (attr == 0) {
        return mode();
    }

    return KateHlManager::self()->nameForIdentifier(highlight()->hlKeyForAttrib(attr));

}

Kate::SwapFile *KTextEditor::DocumentPrivate::swapFile()
{
    return m_swapfile;
}

/**
 * \return \c -1 if \c line or \c column invalid, otherwise one of
 * standard style attribute number
 */
int KTextEditor::DocumentPrivate::defStyleNum(int line, int column)
{
    // Validate parameters to prevent out of range access
    if (line < 0 || line >= lines() || column < 0) {
        return -1;
    }

    // get highlighted line
    Kate::TextLine tl = kateTextLine(line);

    // make sure the textline is a valid pointer
    if (!tl) {
        return -1;
    }

    /**
     * either get char attribute or attribute of context still active at end of line
     */
    int attribute = 0;
    if (column < tl->length()) {
        attribute = tl->attribute(column);
    } else if (column == tl->length()) {
        KateHlContext *context = tl->contextStack().isEmpty() ? highlight()->contextNum(0) : highlight()->contextNum(tl->contextStack().back());
        attribute = context->attr;
    } else {
        return -1;
    }

    return highlight()->defaultStyleForAttribute(attribute);
}

bool KTextEditor::DocumentPrivate::isComment(int line, int column)
{
    const int defaultStyle = defStyleNum(line, column);
    return defaultStyle == KTextEditor::dsComment;
}

int KTextEditor::DocumentPrivate::findTouchedLine(int startLine, bool down)
{
    const int offset = down ? 1 : -1;
    const int lineCount = lines();
    while (startLine >= 0 && startLine < lineCount) {
        Kate::TextLine tl = m_buffer->plainLine(startLine);
        if (tl && (tl->markedAsModified() || tl->markedAsSavedOnDisk())) {
            return startLine;
        }
        startLine += offset;
    }

    return -1;
}

void KTextEditor::DocumentPrivate::setActiveTemplateHandler(KateTemplateHandler* handler)
{
    // delete any active template handler
    delete m_activeTemplateHandler.data();
    m_activeTemplateHandler = handler;
}

//BEGIN KTextEditor::MessageInterface
bool KTextEditor::DocumentPrivate::postMessage(KTextEditor::Message *message)
{
    // no message -> cancel
    if (!message) {
        return false;
    }

    // make sure the desired view belongs to this document
    if (message->view() && message->view()->document() != this) {
        qCWarning(LOG_KTE) << "trying to post a message to a view of another document:" << message->text();
        return false;
    }

    message->setParent(this);
    message->setDocument(this);

    // if there are no actions, add a close action by default if widget does not auto-hide
    if (message->actions().count() == 0 && message->autoHide() < 0) {
        QAction *closeAction = new QAction(QIcon::fromTheme(QStringLiteral("window-close")), i18n("&Close"), nullptr);
        closeAction->setToolTip(i18n("Close message"));
        message->addAction(closeAction);
    }

    // make sure the message is registered even if no actions and no views exist
    m_messageHash[message] = QList<QSharedPointer<QAction> >();

    // reparent actions, as we want full control over when they are deleted
    foreach (QAction *action, message->actions()) {
        action->setParent(nullptr);
        m_messageHash[message].append(QSharedPointer<QAction>(action));
    }

    // post message to requested view, or to all views
    if (KTextEditor::ViewPrivate *view = qobject_cast<KTextEditor::ViewPrivate *>(message->view())) {
        view->postMessage(message, m_messageHash[message]);
    } else {
        foreach (KTextEditor::ViewPrivate *view, m_views) {
            view->postMessage(message, m_messageHash[message]);
        }
    }

    // also catch if the user manually calls delete message
    connect(message, SIGNAL(closed(KTextEditor::Message*)), SLOT(messageDestroyed(KTextEditor::Message*)));

    return true;
}

void KTextEditor::DocumentPrivate::messageDestroyed(KTextEditor::Message *message)
{
    // KTE:Message is already in destructor
    Q_ASSERT(m_messageHash.contains(message));
    m_messageHash.remove(message);
}
//END KTextEditor::MessageInterface

void KTextEditor::DocumentPrivate::closeDocumentInApplication()
{
    KTextEditor::EditorPrivate::self()->application()->closeDocument(this);
}
