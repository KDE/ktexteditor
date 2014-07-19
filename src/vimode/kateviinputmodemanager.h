/*  This file is part of the KDE libraries and the Kate part.
 *
 *  Copyright (C) 2008 Erlend Hamberg <ehamberg@gmail.com>
 *  Copyright (C) 2011 Svyatoslav Kuzmich <svatoslav1@gmail.com>
 *  Copyright (C) 2012 - 2013 Simon St James <kdedevel@etotheipiplusone.com>
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
 *  Boston, MA 02110-1301, USA.
 */

#ifndef KATE_VI_INPUT_MODE_MANAGER_INCLUDED
#define KATE_VI_INPUT_MODE_MANAGER_INCLUDED

#include <QKeyEvent>
#include <QStack>
#include <ktexteditor_export.h>
#include <ktexteditor/cursor.h>
#include <ktexteditor/view.h>

#include "definitions.h"
#include "completion.h"

namespace KateVi
{
    class GlobalState;
    class Searcher;
    class CompletionRecorder;
    class CompletionReplayer;
    class Marks;
    class Jumps;
    class MacroRecorder;
    class LastChangeRecorder;
    class ModeBase;
    class NormalMode;
    class InsertMode;
    class VisualMode;
    class ReplaceMode;
}

class KConfigGroup;
class KateViewInternal;
class KateViKeyParser;
class KateViKeyMapper;
class QString;

namespace KTextEditor
{
    class ViewPrivate;
    class DocumentPrivate;
    class MovingCursor;
    class Mark;
    class MarkInterface;
}

class KateViInputMode;

class KTEXTEDITOR_EXPORT KateViInputModeManager
{
    friend KateViInputMode;

public:
    KateViInputModeManager(KateViInputMode *inputAdapter, KTextEditor::ViewPrivate *view, KateViewInternal *viewInternal);
    ~KateViInputModeManager();

    /**
     * feed key the given key press to the command parser
     * @return true if keypress was is [part of a] command, false otherwise
     */
    bool handleKeypress(const QKeyEvent *e);

    /**
     * feed key the given list of key presses to the key handling code, one by one
     */
    void feedKeyPresses(const QString &keyPresses) const;

    /**
     * Determines whether we are currently processing a Vi keypress
     * @return true if we are still in a call to handleKeypress, false otherwise
     */
    bool isHandlingKeypress() const;

    /**
     * @return The current vi mode
     */
    ViMode getCurrentViMode() const;

    /**
     * @return The current vi mode string representation
     */
    KTextEditor::View::ViewMode getCurrentViewMode() const;

    /**
     * @return the previous vi mode
     */
    ViMode getPreviousViMode() const;

    /**
     * @return true if and only if the current mode is one of VisualMode, VisualBlockMode or VisualLineMode.
     */
    bool isAnyVisualMode() const;

    /**
     * @return one of getViNormalMode(), getViVisualMode(), etc, depending on getCurrentViMode().
     */
    KateVi::ModeBase *getCurrentViModeHandler() const;

    const QString getVerbatimKeys() const;

    /**
     * changes the current vi mode to the given mode
     */
    void changeViMode(ViMode newMode);

    /**
     * set normal mode to be the active vi mode and perform the needed setup work
     */
    void viEnterNormalMode();

    /**
     * set insert mode to be the active vi mode and perform the needed setup work
     */
    void viEnterInsertMode();

    /**
     * set visual mode to be the active vi mode and make the needed setup work
     */
    void viEnterVisualMode(ViMode visualMode = VisualMode);

    /**
     * set replace mode to be the active vi mode and make the needed setup work
     */
    void viEnterReplaceMode();

    /**
     * @return the KateVi::NormalMode instance
     */
    KateVi::NormalMode *getViNormalMode();

    /**
     * @return the KateVi::InsertMode instance
     */
    KateVi::InsertMode *getViInsertMode();

    /**
     * @return the KateViVisualMode instance
     */
    KateVi::VisualMode *getViVisualMode();

    /**
     * @return the KateViReplaceMode instance
     */
    KateVi::ReplaceMode *getViReplaceMode();

    /**
     * append a QKeyEvent to the key event log
     */
    void appendKeyEventToLog(const QKeyEvent &e);

    /**
     * clear the key event log
     */
    void clearCurrentChangeLog();

    /**
     * copy the contents of the key events log to m_lastChange so that it can be repeated
     */
    void storeLastChangeCommand();

    /**
     * repeat last change by feeding the contents of m_lastChange to feedKeys()
     */
    void repeatLastChange();

    void doNotLogCurrentKeypress();

    bool getTemporaryNormalMode()
    {
        return m_temporaryNormalMode;
    }

    void setTemporaryNormalMode(bool b)
    {
        m_temporaryNormalMode = b;
    }

    void reset();

    inline KateVi::Marks *marks() { return m_marks; }
    inline KateVi::Jumps *jumps() { return m_jumps; }

    inline KateVi::Searcher *searcher() { return m_searcher; }

    KateVi::CompletionRecorder *completionRecorder() { return m_completionRecorder; }
    KateVi::CompletionReplayer *completionReplayer() { return m_completionReplayer; }

    KateVi::MacroRecorder *macroRecorder() { return m_macroRecorder; }

    KateVi::LastChangeRecorder *lastChangeRecorder() { return m_lastChangeRecorder; }

    // session stuff
    void readSessionConfig(const KConfigGroup &config);
    void writeSessionConfig(KConfigGroup &config);

    KateViKeyMapper *keyMapper();
    KateVi::GlobalState *globalState() const;
    KTextEditor::ViewPrivate *view() const;

    KateViInputMode *inputAdapter() { return m_inputAdapter; }

    void updateCursor(const KTextEditor::Cursor &c);

    void pushKeyMapper(QSharedPointer<KateViKeyMapper> mapper);
    void popKeyMapper();

private:
    KateVi::NormalMode *m_viNormalMode;
    KateVi::InsertMode *m_viInsertMode;
    KateVi::VisualMode *m_viVisualMode;
    KateVi::ReplaceMode *m_viReplaceMode;

    ViMode m_currentViMode;
    ViMode m_previousViMode;

    KateViInputMode *m_inputAdapter;
    KTextEditor::ViewPrivate *m_view;
    KateViewInternal *m_viewInternal;
    KateViKeyParser *m_keyParser;

    // Create a new keymapper for each macro event, to simplify expansion of mappings in macros
    // where the macro itself was triggered by expanding a mapping!
    QStack<QSharedPointer<KateViKeyMapper> > m_keyMapperStack;

    int m_insideHandlingKeyPressCount;

    /**
     * a list of the (encoded) key events that was part of the last change.
     */
    QString m_lastChange;

    KateVi::CompletionList m_lastChangeCompletionsLog;

    /**
     * true when normal mode was started by Ctrl-O command in insert mode.
     */
    bool m_temporaryNormalMode;

    KateVi::Marks *m_marks;
    KateVi::Jumps *m_jumps;

    KateVi::Searcher *m_searcher;
    KateVi::CompletionRecorder *m_completionRecorder;
    KateVi::CompletionReplayer *m_completionReplayer;

    KateVi::MacroRecorder *m_macroRecorder;

    KateVi::LastChangeRecorder *m_lastChangeRecorder;
};

#endif
