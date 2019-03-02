/*  This file is part of the KDE libraries and the Kate part.
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

#include "kateviinputmode.h"
#include "kateviewinternal.h"
#include "kateconfig.h"
#include <vimode/inputmodemanager.h>
#include <vimode/emulatedcommandbar/emulatedcommandbar.h>
#include <vimode/modes/replacevimode.h>
#include <vimode/modes/visualvimode.h>
#include <vimode/marks.h>
#include <vimode/searcher.h>
#include <vimode/macrorecorder.h>

#include <KLocalizedString>

#include <QCoreApplication>

namespace {
    QString viModeToString(KateVi::ViMode mode)
    {
        QString modeStr;
        switch (mode) {
            case KateVi::InsertMode:
                modeStr = i18n("VI: INSERT MODE");
                break;
            case KateVi::NormalMode:
                modeStr = i18n("VI: NORMAL MODE");
                break;
            case KateVi::VisualMode:
                modeStr = i18n("VI: VISUAL");
                break;
            case KateVi::VisualBlockMode:
                modeStr = i18n("VI: VISUAL BLOCK");
                break;
            case KateVi::VisualLineMode:
                modeStr = i18n("VI: VISUAL LINE");
                break;
            case KateVi::ReplaceMode:
                modeStr = i18n("VI: REPLACE");
                break;
        }

        return modeStr;
    }
}

KateViInputMode::KateViInputMode(KateViewInternal *viewInternal, KateVi::GlobalState *global)
    : KateAbstractInputMode(viewInternal)
    , m_viModeEmulatedCommandBar(nullptr)
    , m_viGlobal(global)
    , m_caret(KateRenderer::Block)
    , m_nextKeypressIsOverriddenShortCut(false)
    , m_activated(false)
{
    m_relLineNumbers = KateViewConfig::global()->viRelativeLineNumbers();
    m_viModeManager = new KateVi::InputModeManager(this, view(), viewInternal);
}

KateViInputMode::~KateViInputMode()
{
    delete m_viModeManager;
}

void KateViInputMode::activate()
{
    m_activated = true;
    setCaretStyle(KateRenderer::Block); // TODO: can we end up in insert mode?
    reset(); // TODO: is this necessary? (well, not anymore I guess)

    if (view()->selection()) {
        m_viModeManager->changeViMode(KateVi::VisualMode);
        view()->setCursorPosition(KTextEditor::Cursor(view()->selectionRange().end().line(), view()->selectionRange().end().column() - 1));
        m_viModeManager->m_viVisualMode->updateSelection();
    }
    viewInternal()->iconBorder()->setRelLineNumbersOn(m_relLineNumbers);
}

void KateViInputMode::deactivate()
{
    if (m_viModeEmulatedCommandBar) {
        m_viModeEmulatedCommandBar->hideMe();
    }

    // make sure to turn off edits merging when leaving vi input mode
    view()->doc()->setUndoMergeAllEdits(false);
    m_activated = false;
    viewInternal()->iconBorder()->setRelLineNumbersOn(false);
}

void KateViInputMode::reset()
{
    if (m_viModeEmulatedCommandBar) {
        m_viModeEmulatedCommandBar->hideMe();
    }

    delete m_viModeManager;
    m_viModeManager = new KateVi::InputModeManager(this, view(), viewInternal());

    if (m_viModeEmulatedCommandBar) {
        m_viModeEmulatedCommandBar->setViInputModeManager(m_viModeManager);
    }
}

bool KateViInputMode::overwrite() const
{
    return  m_viModeManager->getCurrentViMode() == KateVi::ViMode::ReplaceMode;
}

void KateViInputMode::overwrittenChar(const QChar &c)
{
    m_viModeManager->getViReplaceMode()->overwrittenChar(c);
}

void KateViInputMode::clearSelection()
{
    // do nothing, handled elsewhere
}

bool KateViInputMode::stealKey(QKeyEvent *k)
{
    if (!KateViewConfig::global()->viInputModeStealKeys())
    {
        return false;
    }

    // Actually see if we can make use of this key - if so, we've stolen it; if not,
    // let Qt's shortcut handling system deal with it.
    const bool stolen = keyPress(k);
    if (stolen)
    {
        // Qt will replay this QKeyEvent, next time as an ordinary KeyPress.
        m_nextKeypressIsOverriddenShortCut = true;
    }
    return stolen;
}

KTextEditor::View::InputMode KateViInputMode::viewInputMode() const
{
    return KTextEditor::View::ViInputMode;
}

QString KateViInputMode::viewInputModeHuman() const
{
    return i18n("vi-mode");
}

KTextEditor::View::ViewMode KateViInputMode::viewMode() const
{
    return m_viModeManager->getCurrentViewMode();
}

QString KateViInputMode::viewModeHuman() const
{
    QString currentMode = viModeToString(m_viModeManager->getCurrentViMode());

    if (m_viModeManager->macroRecorder()->isRecording()) {
        currentMode.prepend(QLatin1String("(") + i18n("recording") + QLatin1String(") "));
    }

    QString cmd = m_viModeManager->getVerbatimKeys();
    if (!cmd.isEmpty()) {
        currentMode.prepend(QStringLiteral("%1 ").arg(cmd));
    }

    return QStringLiteral("%1").arg(currentMode);
}

void KateViInputMode::gotFocus()
{
    // nothing to do
}

void KateViInputMode::lostFocus()
{
    // nothing to do
}

void KateViInputMode::readSessionConfig(const KConfigGroup &config)
{
    // restore vi registers and jump list
    m_viModeManager->readSessionConfig(config);
}

void KateViInputMode::writeSessionConfig(KConfigGroup &config)
{
    // save vi registers and jump list
    m_viModeManager->writeSessionConfig(config);
}

void KateViInputMode::updateConfig()
{
    KateViewConfig *cfg = view()->config();

    // whether relative line numbers should be used or not.
    m_relLineNumbers = cfg->viRelativeLineNumbers();

    if (m_activated) {
        viewInternal()->iconBorder()->setRelLineNumbersOn(m_relLineNumbers);
    }
}

void KateViInputMode::readWriteChanged(bool)
{
    // nothing todo
}

void KateViInputMode::find()
{
    showViModeEmulatedCommandBar();
    viModeEmulatedCommandBar()->init(KateVi::EmulatedCommandBar::SearchForward);
}

void KateViInputMode::findSelectedForwards()
{
    m_viModeManager->searcher()->findNext();
}

void KateViInputMode::findSelectedBackwards()
{
    m_viModeManager->searcher()->findPrevious();
}

void KateViInputMode::findReplace()
{
    showViModeEmulatedCommandBar();
    viModeEmulatedCommandBar()->init(KateVi::EmulatedCommandBar::SearchForward);
}

void KateViInputMode::findNext()
{
    m_viModeManager->searcher()->findNext();
}

void KateViInputMode::findPrevious()
{
    m_viModeManager->searcher()->findPrevious();
}

void KateViInputMode::activateCommandLine()
{
    showViModeEmulatedCommandBar();
    viModeEmulatedCommandBar()->init(KateVi::EmulatedCommandBar::Command);
}

void KateViInputMode::showViModeEmulatedCommandBar()
{
    view()->bottomViewBar()->addBarWidget(viModeEmulatedCommandBar());
    view()->bottomViewBar()->showBarWidget(viModeEmulatedCommandBar());
}

KateVi::EmulatedCommandBar *KateViInputMode::viModeEmulatedCommandBar()
{
    if (!m_viModeEmulatedCommandBar) {
        m_viModeEmulatedCommandBar = new KateVi::EmulatedCommandBar(this, m_viModeManager, view());
        m_viModeEmulatedCommandBar->hide();
    }

    return m_viModeEmulatedCommandBar;
}

void KateViInputMode::updateRendererConfig()
{
    // do nothing
}

bool KateViInputMode::keyPress(QKeyEvent *e)
{
    if (m_nextKeypressIsOverriddenShortCut)
    {
        // This is just the replay of a shortcut that we stole, this time as a QKeyEvent.
        // Ignore it, as we'll have already handled it via stealKey()!
        m_nextKeypressIsOverriddenShortCut = false;
        return true;
    }

    if (m_viModeManager->handleKeypress(e)) {
        emit view()->viewModeChanged(view(), viewMode());
        return true;
    }

    return false;
}

bool KateViInputMode::blinkCaret() const
{
    return false;
}

KateRenderer::caretStyles KateViInputMode::caretStyle() const
{
    return m_caret;
}

void KateViInputMode::toggleInsert()
{
    // do nothing
}

void KateViInputMode::launchInteractiveCommand(const QString &)
{
    // do nothing so far
}

QString KateViInputMode::bookmarkLabel(int line) const
{
    return m_viModeManager->marks()->getMarksOnTheLine(line);
}

void KateViInputMode::setCaretStyle(const KateRenderer::caretStyles caret)
{
    if (m_caret != caret) {
        m_caret = caret;

        view()->renderer()->setCaretStyle(m_caret);
        view()->renderer()->setDrawCaret(true);
        viewInternal()->paintCursor();
    }
}
