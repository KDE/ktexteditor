/*  This file is part of the KDE libraries and the Kate part.
 *
 *  Copyright (C) 2008 - 2009 Erlend Hamberg <ehamberg@gmail.com>
 *  Copyright (C) 2009 Paul Gideon Dann <pdgiddie@gmail.com>
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

#include "kateviinputmodemanager.h"

#include <QKeyEvent>
#include <QString>
#include <QApplication>

#include <KConfig>
#include <KConfigGroup>
#include <KLocalizedString>

#include "kateconfig.h"
#include "kateglobal.h"
#include "kateviglobal.h"
#include "kateviewinternal.h"
#include "katevinormalmode.h"
#include "kateviinsertmode.h"
#include "katevivisualmode.h"
#include "katevireplacemode.h"
#include "katevikeyparser.h"
#include "katevikeymapper.h"
#include "kateviemulatedcommandbar.h"
#include "katepartdebug.h"
#include "kateviinputmode.h"
#include "macros.h"
#include "registers.h"

using KTextEditor::Cursor;
using KTextEditor::Document;
using KTextEditor::Mark;
using KTextEditor::MarkInterface;
using KTextEditor::MovingCursor;

KateViInputModeManager::KateViInputModeManager(KateViInputMode *inputAdapter, KTextEditor::ViewPrivate *view, KateViewInternal *viewInternal)
    : m_inputAdapter(inputAdapter)
{
    m_currentViMode = NormalMode;
    m_previousViMode = NormalMode;

    m_viNormalMode = new KateViNormalMode(this, view, viewInternal);
    m_viInsertMode = new KateViInsertMode(this, view, viewInternal);
    m_viVisualMode = new KateViVisualMode(this, view, viewInternal);
    m_viReplaceMode = new KateViReplaceMode(this, view, viewInternal);

    m_view = view;
    m_viewInternal = viewInternal;

    m_insideHandlingKeyPressCount = 0;

    m_isReplayingLastChange = false;

    m_isRecordingMacro = false;
    m_macrosBeingReplayedCount = 0;
    m_lastPlayedMacroRegister = QChar::Null;

    m_keyMapperStack.push(QSharedPointer<KateViKeyMapper>(new KateViKeyMapper(this, m_view->doc(), m_view)));

    m_lastSearchBackwards = false;
    m_lastSearchCaseSensitive = false;
    m_lastSearchPlacedCursorAtEndOfMatch = false;

    m_temporaryNormalMode = false;

    m_jumps = new KateVi::Jumps();
    m_marks = new KateVi::Marks(this);

    // We have to do this outside of KateViNormalMode, as we don't want
    // KateViVisualMode (which inherits from KateViNormalMode) to respond
    // to changes in the document as well.
    m_viNormalMode->beginMonitoringDocumentChanges();
}

KateViInputModeManager::~KateViInputModeManager()
{
    delete m_viNormalMode;
    delete m_viInsertMode;
    delete m_viVisualMode;
    delete m_viReplaceMode;
    delete m_jumps;
    delete m_marks;
}

bool KateViInputModeManager::handleKeypress(const QKeyEvent *e)
{
    m_insideHandlingKeyPressCount++;
    bool res = false;
    bool keyIsPartOfMapping = false;
    const bool isSyntheticSearchCompletedKeyPress = m_inputAdapter->viModeEmulatedCommandBar()->isSendingSyntheticSearchCompletedKeypress();

    // With macros, we want to record the keypresses *before* they are mapped, but if they end up *not* being part
    // of a mapping, we don't want to record them when they are played back by m_keyMapper, hence
    // the "!isPlayingBackRejectedKeys()". And obviously, since we're recording keys before they are mapped, we don't
    // want to also record the executed mapping, as when we replayed the macro, we'd get duplication!
    if (isRecordingMacro() && !isReplayingMacro() && !isSyntheticSearchCompletedKeyPress && !keyMapper()->isExecutingMapping() && !keyMapper()->isPlayingBackRejectedKeys()) {
        QKeyEvent copy(e->type(), e->key(), e->modifiers(), e->text());
        m_currentMacroKeyEventsLog.append(copy);
    }

    if (!isReplayingLastChange() && !isSyntheticSearchCompletedKeyPress) {
        if (e->key() == Qt::Key_AltGr) {
            return true; // do nothing
        }

        // Hand off to the key mapper, and decide if this key is part of a mapping.
        if (e->key() != Qt::Key_Control && e->key() != Qt::Key_Shift && e->key() != Qt::Key_Alt && e->key() != Qt::Key_Meta) {
            const QChar key = KateViKeyParser::self()->KeyEventToQChar(*e);
            if (keyMapper()->handleKeypress(key)) {
                keyIsPartOfMapping = true;
                res = true;
            }
        }
    }

    if (!keyIsPartOfMapping) {
        if (!isReplayingLastChange() && !isSyntheticSearchCompletedKeyPress) {
            // record key press so that it can be repeated via "."
            QKeyEvent copy(e->type(), e->key(), e->modifiers(), e->text());
            appendKeyEventToLog(copy);
        }

        if (m_inputAdapter->viModeEmulatedCommandBar()->isActive()) {
            res = m_inputAdapter->viModeEmulatedCommandBar()->handleKeyPress(e);
        } else {
            res = getCurrentViModeHandler()->handleKeypress(e);
        }
    }

    m_insideHandlingKeyPressCount--;
    Q_ASSERT(m_insideHandlingKeyPressCount >= 0);

    return res;
}

void KateViInputModeManager::feedKeyPresses(const QString &keyPresses) const
{
    int key;
    Qt::KeyboardModifiers mods;
    QString text;

    foreach (const QChar &c, keyPresses) {
        QString decoded = KateViKeyParser::self()->decodeKeySequence(QString(c));
        key = -1;
        mods = Qt::NoModifier;
        text.clear();

        qCDebug(LOG_PART) << "\t" << decoded;

        if (decoded.length() > 1) {  // special key

            // remove the angle brackets
            decoded.remove(0, 1);
            decoded.remove(decoded.indexOf(QLatin1String(">")), 1);
            qCDebug(LOG_PART) << QLatin1String("\t Special key:") << decoded;

            // check if one or more modifier keys where used
            if (decoded.indexOf(QLatin1String("s-")) != -1 || decoded.indexOf(QLatin1String("c-")) != -1
                    || decoded.indexOf(QLatin1String("m-")) != -1 || decoded.indexOf(QLatin1String("a-")) != -1) {

                int s = decoded.indexOf(QLatin1String("s-"));
                if (s != -1) {
                    mods |= Qt::ShiftModifier;
                    decoded.remove(s, 2);
                }

                int c = decoded.indexOf(QLatin1String("c-"));
                if (c != -1) {
                    mods |= Qt::ControlModifier;
                    decoded.remove(c, 2);
                }

                int a = decoded.indexOf(QLatin1String("a-"));
                if (a != -1) {
                    mods |= Qt::AltModifier;
                    decoded.remove(a, 2);
                }

                int m = decoded.indexOf(QLatin1String("m-"));
                if (m != -1) {
                    mods |= Qt::MetaModifier;
                    decoded.remove(m, 2);
                }

                if (decoded.length() > 1) {
                    key = KateViKeyParser::self()->vi2qt(decoded);
                } else if (decoded.length() == 1) {
                    key = int(decoded.at(0).toUpper().toLatin1());
                    text = decoded.at(0);
                    qCDebug(LOG_PART) << "###########" << key;
                } else {
                    qCWarning(LOG_PART) << "decoded is empty. skipping key press.";
                }
            } else { // no modifiers
                key = KateViKeyParser::self()->vi2qt(decoded);
            }
        } else {
            key = decoded.at(0).unicode();
            text = decoded.at(0);
        }

        // We have to be clever about which widget we dispatch to, as we can trigger
        // shortcuts if we're not careful (even if Vim mode is configured to steal shortcuts).
        QKeyEvent k(QEvent::KeyPress, key, mods, text);
        QWidget *destWidget = NULL;
        if (QApplication::activePopupWidget()) {
            // According to the docs, the activePopupWidget, if present, takes all events.
            destWidget = QApplication::activePopupWidget();
        } else if (QApplication::focusWidget()) {
            if (QApplication::focusWidget()->focusProxy()) {
                destWidget = QApplication::focusWidget()->focusProxy();
            } else {
                destWidget = QApplication::focusWidget();
            }
        } else {
            destWidget = m_view->focusProxy();
        }
        QApplication::sendEvent(destWidget, &k);
    }
}

bool KateViInputModeManager::isHandlingKeypress() const
{
    return m_insideHandlingKeyPressCount > 0;
}

void KateViInputModeManager::appendKeyEventToLog(const QKeyEvent &e)
{
    if (e.key() != Qt::Key_Shift && e.key() != Qt::Key_Control
            && e.key() != Qt::Key_Meta && e.key() != Qt::Key_Alt) {
        m_currentChangeKeyEventsLog.append(e);
    }
}

void KateViInputModeManager::storeLastChangeCommand()
{
    m_lastChange.clear();

    QList<QKeyEvent> keyLog = m_currentChangeKeyEventsLog;

    for (int i = 0; i < keyLog.size(); i++) {
        int keyCode = keyLog.at(i).key();
        QString text = keyLog.at(i).text();
        int mods = keyLog.at(i).modifiers();
        QChar key;

        if (text.length() > 0) {
            key = text.at(0);
        }

        if (text.isEmpty() || (text.length() == 1 && text.at(0) < 0x20)
                || (mods != Qt::NoModifier && mods != Qt::ShiftModifier)) {
            QString keyPress;

            keyPress.append(QLatin1Char('<'));
            keyPress.append((mods & Qt::ShiftModifier)   ? QLatin1String("s-") : QString());
            keyPress.append((mods & Qt::ControlModifier) ? QLatin1String("c-") : QString());
            keyPress.append((mods & Qt::AltModifier)     ? QLatin1String("a-") : QString());
            keyPress.append((mods & Qt::MetaModifier)    ? QLatin1String("m-") : QString());
            keyPress.append(keyCode <= 0xFF ? QChar(keyCode) : KateViKeyParser::self()->qt2vi(keyCode));
            keyPress.append(QLatin1Char('>'));

            key = KateViKeyParser::self()->encodeKeySequence(keyPress).at(0);
        }

        m_lastChange.append(key);
    }
    m_lastChangeCompletionsLog = m_currentChangeCompletionsLog;
}

void KateViInputModeManager::repeatLastChange()
{
    m_isReplayingLastChange = true;
    m_nextLoggedLastChangeComplexIndex = 0;
    feedKeyPresses(m_lastChange);
    m_isReplayingLastChange = false;
}

void KateViInputModeManager::findNext()
{
    getCurrentViModeHandler()->findNext();
}

void KateViInputModeManager::findPrevious()
{
    getCurrentViModeHandler()->findPrev();
}

void KateViInputModeManager::startRecordingMacro(QChar macroRegister)
{
    Q_ASSERT(!m_isRecordingMacro);
    qCDebug(LOG_PART) << "Recording macro: " << macroRegister;
    m_isRecordingMacro = true;
    m_recordingMacroRegister = macroRegister;
    m_inputAdapter->viGlobal()->macros()->remove(macroRegister);
    m_currentMacroKeyEventsLog.clear();
    m_currentMacroCompletionsLog.clear();
}

void KateViInputModeManager::finishRecordingMacro()
{
    Q_ASSERT(m_isRecordingMacro);
    m_isRecordingMacro = false;
    m_inputAdapter->viGlobal()->macros()->store(m_recordingMacroRegister, m_currentMacroKeyEventsLog, m_currentMacroCompletionsLog);
}

bool KateViInputModeManager::isRecordingMacro()
{
    return m_isRecordingMacro;
}

void KateViInputModeManager::replayMacro(QChar macroRegister)
{
    if (macroRegister == QLatin1Char('@')) {
        macroRegister = m_lastPlayedMacroRegister;
    }
    m_lastPlayedMacroRegister = macroRegister;
    qCDebug(LOG_PART) << "Replaying macro: " << macroRegister;
    const QString macroAsFeedableKeypresses = m_inputAdapter->viGlobal()->macros()->get(macroRegister);
    qCDebug(LOG_PART) << "macroAsFeedableKeypresses:  " << macroAsFeedableKeypresses;

    m_macrosBeingReplayedCount++;
    m_nextLoggedMacroCompletionIndex.push(0);
    m_macroCompletionsToReplay.push(m_inputAdapter->viGlobal()->macros()->getCompletions(macroRegister));
    m_keyMapperStack.push(QSharedPointer<KateViKeyMapper>(new KateViKeyMapper(this, m_view->doc(), m_view)));
    feedKeyPresses(macroAsFeedableKeypresses);
    m_keyMapperStack.pop();
    m_macroCompletionsToReplay.pop();
    m_nextLoggedMacroCompletionIndex.pop();
    m_macrosBeingReplayedCount--;
    qCDebug(LOG_PART) << "Finished replaying: " << macroRegister;
}

bool KateViInputModeManager::isReplayingMacro()
{
    return m_macrosBeingReplayedCount > 0;
}

void KateViInputModeManager::logCompletionEvent(const KateViInputModeManager::Completion &completion)
{
    // Ctrl-space is a special code that means: if you're replaying a macro, fetch and execute
    // the next logged completion.
    QKeyEvent ctrlSpace(QKeyEvent::KeyPress, Qt::Key_Space, Qt::ControlModifier, QLatin1String(" "));
    if (isRecordingMacro()) {
        m_currentMacroKeyEventsLog.append(ctrlSpace);
        m_currentMacroCompletionsLog.append(completion);
    }
    m_currentChangeKeyEventsLog.append(ctrlSpace);
    m_currentChangeCompletionsLog.append(completion);
}

KateViInputModeManager::Completion KateViInputModeManager::nextLoggedCompletion()
{
    Q_ASSERT(isReplayingLastChange() || isReplayingMacro());
    if (isReplayingLastChange()) {
        if (m_nextLoggedLastChangeComplexIndex >= m_lastChangeCompletionsLog.length()) {
            qCDebug(LOG_PART) << "Something wrong here: requesting more completions for last change than we actually have.  Returning dummy.";
            return Completion(QString(), false, Completion::PlainText);
        }
        return m_lastChangeCompletionsLog[m_nextLoggedLastChangeComplexIndex++];
    } else {
        if (m_nextLoggedMacroCompletionIndex.top() >= m_macroCompletionsToReplay.top().length()) {
            qCDebug(LOG_PART) << "Something wrong here: requesting more completions for macro than we actually have.  Returning dummy.";
            return Completion(QString(), false, Completion::PlainText);
        }
        return m_macroCompletionsToReplay.top()[m_nextLoggedMacroCompletionIndex.top()++];
    }
}

void KateViInputModeManager::doNotLogCurrentKeypress()
{
    if (m_isRecordingMacro) {
        Q_ASSERT(!m_currentMacroKeyEventsLog.isEmpty());
        m_currentMacroKeyEventsLog.pop_back();
    }
    Q_ASSERT(!m_currentChangeKeyEventsLog.isEmpty());
    m_currentChangeKeyEventsLog.pop_back();
}

const QString KateViInputModeManager::getLastSearchPattern() const
{
    return m_lastSearchPattern;
}

void KateViInputModeManager::setLastSearchPattern(const QString &p)
{
    m_lastSearchPattern = p;
}

void KateViInputModeManager::changeViMode(ViMode newMode)
{
    m_previousViMode = m_currentViMode;
    m_currentViMode = newMode;
}

ViMode KateViInputModeManager::getCurrentViMode() const
{
    return m_currentViMode;
}

KTextEditor::View::ViewMode KateViInputModeManager::getCurrentViewMode() const
{
    switch (m_currentViMode) {
        case InsertMode:
            return KTextEditor::View::ViModeInsert;
        case VisualMode:
            return KTextEditor::View::ViModeVisual;
        case VisualLineMode:
            return KTextEditor::View::ViModeVisualLine;
        case VisualBlockMode:
            return KTextEditor::View::ViModeVisualBlock;
        case ReplaceMode:
            return KTextEditor::View::ViModeReplace;
        case NormalMode:
        default:
            return KTextEditor::View::ViModeNormal;
    }
}

ViMode KateViInputModeManager::getPreviousViMode() const
{
    return m_previousViMode;
}

bool KateViInputModeManager::isAnyVisualMode() const
{
    return ((m_currentViMode == VisualMode) || (m_currentViMode == VisualLineMode) || (m_currentViMode == VisualBlockMode));
}

KateViModeBase *KateViInputModeManager::getCurrentViModeHandler() const
{
    switch (m_currentViMode) {
    case NormalMode:
        return m_viNormalMode;
    case InsertMode:
        return m_viInsertMode;
    case VisualMode:
    case VisualLineMode:
    case VisualBlockMode:
        return m_viVisualMode;
    case ReplaceMode:
        return m_viReplaceMode;
    }
    qCDebug(LOG_PART) << "WARNING: Unknown Vi mode.";
    return NULL;
}

void KateViInputModeManager::viEnterNormalMode()
{
    bool moveCursorLeft = (m_currentViMode == InsertMode || m_currentViMode == ReplaceMode)
                          && m_viewInternal->getCursor().column() > 0;

    if (!isReplayingLastChange() && m_currentViMode == InsertMode) {
        // '^ is the insert mark and "^ is the insert register,
        // which holds the last inserted text
        Range r(m_view->cursorPosition(), m_marks->getInsertStopped());

        if (r.isValid()) {
            QString insertedText = m_view->doc()->text(r);
            m_inputAdapter->viGlobal()->registers()->setInsertStopped(insertedText);
        }

        m_marks->setInsertStopped(Cursor(m_view->cursorPosition()));
    }

    changeViMode(NormalMode);

    if (moveCursorLeft) {
        m_viewInternal->cursorPrevChar();
    }
    m_inputAdapter->setCaretStyle(KateRenderer::Block);
    m_viewInternal->update();
}

void KateViInputModeManager::viEnterInsertMode()
{
    changeViMode(InsertMode);
    m_marks->setInsertStopped(Cursor(m_view->cursorPosition()));
    if (getTemporaryNormalMode()) {
        // Ensure the key log contains a request to re-enter Insert mode, else the keystrokes made
        // after returning from temporary normal mode will be treated as commands!
        m_currentChangeKeyEventsLog.append(QKeyEvent(QEvent::KeyPress, Qt::Key_I, Qt::NoModifier, QLatin1String("i")));
    }
    m_inputAdapter->setCaretStyle(KateRenderer::Line);
    setTemporaryNormalMode(false);
    m_viewInternal->update();
}

void KateViInputModeManager::viEnterVisualMode(ViMode mode)
{
    changeViMode(mode);

    // If the selection is inclusive, the caret should be a block.
    // If the selection is exclusive, the caret should be a line.
    m_inputAdapter->setCaretStyle(KateRenderer::Block);
    m_viewInternal->update();
    getViVisualMode()->setVisualModeType(mode);
    getViVisualMode()->init();
}

void KateViInputModeManager::viEnterReplaceMode()
{
    changeViMode(ReplaceMode);
    m_inputAdapter->setCaretStyle(KateRenderer::Underline);
    m_viewInternal->update();
}

KateViNormalMode *KateViInputModeManager::getViNormalMode()
{
    return m_viNormalMode;
}

KateViInsertMode *KateViInputModeManager::getViInsertMode()
{
    return m_viInsertMode;
}

KateViVisualMode *KateViInputModeManager::getViVisualMode()
{
    return m_viVisualMode;
}

KateViReplaceMode *KateViInputModeManager::getViReplaceMode()
{
    return m_viReplaceMode;
}

const QString KateViInputModeManager::getVerbatimKeys() const
{
    QString cmd;

    switch (getCurrentViMode()) {
    case NormalMode:
        cmd = m_viNormalMode->getVerbatimKeys();
        break;
    case InsertMode:
    case ReplaceMode:
        // ...
        break;
    case VisualMode:
    case VisualLineMode:
    case VisualBlockMode:
        cmd = m_viVisualMode->getVerbatimKeys();
        break;
    }

    return cmd;
}

void KateViInputModeManager::readSessionConfig(const KConfigGroup &config)
{
    m_jumps->readSessionConfig(config);
    m_marks->readSessionConfig(config);
}

void KateViInputModeManager::writeSessionConfig(KConfigGroup &config)
{
    m_jumps->writeSessionConfig(config);
    m_marks->writeSessionConfig(config);
}

void KateViInputModeManager::reset()
{
    if (m_viVisualMode) {
        m_viVisualMode->reset();
    }
}

KateViKeyMapper *KateViInputModeManager::keyMapper()
{
    return m_keyMapperStack.top().data();
}

KateViInputModeManager::Completion::Completion(const QString &completedText, bool removeTail, CompletionType completionType)
    : m_completedText(completedText),
      m_removeTail(removeTail),
      m_completionType(completionType)
{
    if (m_completionType == FunctionWithArgs || m_completionType == FunctionWithoutArgs) {
        qCDebug(LOG_PART) << "Completing a function while not removing tail currently unsupported; will remove tail instead";
        m_removeTail = true;
    }
}
QString KateViInputModeManager::Completion::completedText() const
{
    return m_completedText;
}
bool KateViInputModeManager::Completion::removeTail() const
{
    return m_removeTail;
}
KateViInputModeManager::Completion::CompletionType KateViInputModeManager::Completion::completionType() const
{
    return m_completionType;
}

void KateViInputModeManager::updateCursor(const Cursor &c)
{
    m_inputAdapter->updateCursor(c);
}

KateViGlobal *KateViInputModeManager::viGlobal() const
{
    return m_inputAdapter->viGlobal();
}

KTextEditor::ViewPrivate *KateViInputModeManager::view() const
{
    return m_view;
}

void KateViInputModeManager::error(const QString &msg)
{
    m_viNormalMode->error(msg);
}

void KateViInputModeManager::message(const QString &msg)
{
    m_viNormalMode->message(msg);
}
