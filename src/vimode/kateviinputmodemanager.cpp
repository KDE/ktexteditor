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
#include "globalstate.h"
#include "kateviewinternal.h"
#include <vimode/modes/normalmode.h>
#include <vimode/modes/insertmode.h>
#include <vimode/modes/visualmode.h>
#include <vimode/modes/replacemode.h>
#include "katevikeyparser.h"
#include "katevikeymapper.h"
#include "kateviemulatedcommandbar.h"
#include "katepartdebug.h"
#include "kateviinputmode.h"
#include "marks.h"
#include "jumps.h"
#include "macros.h"
#include "registers.h"
#include "searcher.h"
#include "completionrecorder.h"
#include "completionreplayer.h"
#include "macrorecorder.h"
#include "lastchangerecorder.h"

KateViInputModeManager::KateViInputModeManager(KateViInputMode *inputAdapter, KTextEditor::ViewPrivate *view, KateViewInternal *viewInternal)
    : m_inputAdapter(inputAdapter)
{
    m_currentViMode = ViMode::NormalMode;
    m_previousViMode = ViMode::NormalMode;

    m_viNormalMode = new KateVi::NormalMode(this, view, viewInternal);
    m_viInsertMode = new KateVi::InsertMode(this, view, viewInternal);
    m_viVisualMode = new KateVi::VisualMode(this, view, viewInternal);
    m_viReplaceMode = new KateVi::ReplaceMode(this, view, viewInternal);

    m_view = view;
    m_viewInternal = viewInternal;

    m_insideHandlingKeyPressCount = 0;

    m_keyMapperStack.push(QSharedPointer<KateViKeyMapper>(new KateViKeyMapper(this, m_view->doc(), m_view)));

    m_temporaryNormalMode = false;

    m_jumps = new KateVi::Jumps();
    m_marks = new KateVi::Marks(this);

    m_searcher = new KateVi::Searcher(this);
    m_completionRecorder = new KateVi::CompletionRecorder(this);
    m_completionReplayer = new KateVi::CompletionReplayer(this);

    m_macroRecorder = new KateVi::MacroRecorder(this);

    m_lastChangeRecorder = new KateVi::LastChangeRecorder(this);

    // We have to do this outside of KateVi::NormalMode, as we don't want
    // KateViVisualMode (which inherits from KateVi::NormalMode) to respond
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
    delete m_macroRecorder;
    delete m_completionRecorder;
    delete m_completionReplayer;
    delete m_lastChangeRecorder;
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
    if (m_macroRecorder->isRecording() && !m_macroRecorder->isReplaying() && !isSyntheticSearchCompletedKeyPress && !keyMapper()->isExecutingMapping() && !keyMapper()->isPlayingBackRejectedKeys()) {
        m_macroRecorder->record(*e);
    }

    if (!m_lastChangeRecorder->isReplaying() && !isSyntheticSearchCompletedKeyPress) {
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
        if (!m_lastChangeRecorder->isReplaying() && !isSyntheticSearchCompletedKeyPress) {
            // record key press so that it can be repeated via "."
            m_lastChangeRecorder->record(*e);
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

void KateViInputModeManager::storeLastChangeCommand()
{
    m_lastChange = m_lastChangeRecorder->encodedChanges();
    m_lastChangeCompletionsLog = m_completionRecorder->currentChangeCompletionsLog();
}

void KateViInputModeManager::repeatLastChange()
{
    m_lastChangeRecorder->replay(m_lastChange, m_lastChangeCompletionsLog);
}

void KateViInputModeManager::clearCurrentChangeLog()
{
    m_lastChangeRecorder->clear();
    m_completionRecorder->clearCurrentChangeCompletionsLog();
}

void KateViInputModeManager::doNotLogCurrentKeypress()
{
    m_macroRecorder->dropLast();
    m_lastChangeRecorder->dropLast();
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
        case ViMode::InsertMode:
            return KTextEditor::View::ViModeInsert;
        case ViMode::VisualMode:
            return KTextEditor::View::ViModeVisual;
        case ViMode::VisualLineMode:
            return KTextEditor::View::ViModeVisualLine;
        case ViMode::VisualBlockMode:
            return KTextEditor::View::ViModeVisualBlock;
        case ViMode::ReplaceMode:
            return KTextEditor::View::ViModeReplace;
        case ViMode::NormalMode:
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
    return ((m_currentViMode == ViMode::VisualMode) ||
            (m_currentViMode == ViMode::VisualLineMode) ||
            (m_currentViMode == ViMode::VisualBlockMode));
}

KateVi::ModeBase *KateViInputModeManager::getCurrentViModeHandler() const
{
    switch (m_currentViMode) {
    case ViMode::NormalMode:
        return m_viNormalMode;
    case ViMode::InsertMode:
        return m_viInsertMode;
    case ViMode::VisualMode:
    case ViMode::VisualLineMode:
    case ViMode::VisualBlockMode:
        return m_viVisualMode;
    case ViMode::ReplaceMode:
        return m_viReplaceMode;
    }
    qCDebug(LOG_PART) << "WARNING: Unknown Vi mode.";
    return NULL;
}

void KateViInputModeManager::viEnterNormalMode()
{
    bool moveCursorLeft = (m_currentViMode == ViMode::InsertMode || m_currentViMode == ViMode::ReplaceMode)
                          && m_viewInternal->getCursor().column() > 0;

    if (!m_lastChangeRecorder->isReplaying() && m_currentViMode == ViMode::InsertMode) {
        // '^ is the insert mark and "^ is the insert register,
        // which holds the last inserted text
        KTextEditor::Range r(m_view->cursorPosition(), m_marks->getInsertStopped());

        if (r.isValid()) {
            QString insertedText = m_view->doc()->text(r);
            m_inputAdapter->globalState()->registers()->setInsertStopped(insertedText);
        }

        m_marks->setInsertStopped(KTextEditor::Cursor(m_view->cursorPosition()));
    }

    changeViMode(ViMode::NormalMode);

    if (moveCursorLeft) {
        m_viewInternal->cursorPrevChar();
    }
    m_inputAdapter->setCaretStyle(KateRenderer::Block);
    m_viewInternal->update();
}

void KateViInputModeManager::viEnterInsertMode()
{
    changeViMode(ViMode::InsertMode);
    m_marks->setInsertStopped(KTextEditor::Cursor(m_view->cursorPosition()));
    if (getTemporaryNormalMode()) {
        // Ensure the key log contains a request to re-enter Insert mode, else the keystrokes made
        // after returning from temporary normal mode will be treated as commands!
        m_lastChangeRecorder->record(QKeyEvent(QEvent::KeyPress, Qt::Key_I, Qt::NoModifier, QLatin1String("i")));
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

KateVi::NormalMode *KateViInputModeManager::getViNormalMode()
{
    return m_viNormalMode;
}

KateVi::InsertMode *KateViInputModeManager::getViInsertMode()
{
    return m_viInsertMode;
}

KateVi::VisualMode *KateViInputModeManager::getViVisualMode()
{
    return m_viVisualMode;
}

KateVi::ReplaceMode *KateViInputModeManager::getViReplaceMode()
{
    return m_viReplaceMode;
}

const QString KateViInputModeManager::getVerbatimKeys() const
{
    QString cmd;

    switch (getCurrentViMode()) {
    case ViMode::NormalMode:
        cmd = m_viNormalMode->getVerbatimKeys();
        break;
    case ViMode::InsertMode:
    case ViMode::ReplaceMode:
        // ...
        break;
    case ViMode::VisualMode:
    case ViMode::VisualLineMode:
    case ViMode::VisualBlockMode:
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

void KateViInputModeManager::updateCursor(const KTextEditor::Cursor &c)
{
    m_inputAdapter->updateCursor(c);
}

KateVi::GlobalState *KateViInputModeManager::globalState() const
{
    return m_inputAdapter->globalState();
}

KTextEditor::ViewPrivate *KateViInputModeManager::view() const
{
    return m_view;
}


void KateViInputModeManager::pushKeyMapper(QSharedPointer<KateViKeyMapper> mapper)
{
    m_keyMapperStack.push(mapper);
}

void KateViInputModeManager::popKeyMapper()
{
    m_keyMapperStack.pop();
}
