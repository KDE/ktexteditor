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
#include "vimode/kateviinputmodemanager.h"
#include "vimode/kateviemulatedcommandbar.h"
#include "vimode/katevireplacemode.h"
#include "vimode/katevivisualmode.h"

#include <KLocalizedString>

#include <QCoreApplication>

namespace {
    QString viModeToString(ViMode mode)
    {
        QString modeStr;
        switch (mode) {
            case InsertMode:
                modeStr = i18n("VI: INSERT MODE");
                break;
            case NormalMode:
                modeStr = i18n("VI: NORMAL MODE");
                break;
            case VisualMode:
                modeStr = i18n("VI: VISUAL");
                break;
            case VisualBlockMode:
                modeStr = i18n("VI: VISUAL BLOCK");
                break;
            case VisualLineMode:
                modeStr = i18n("VI: VISUAL LINE");
                break;
            case ReplaceMode:
                modeStr = i18n("VI: REPLACE");
                break;
        }

        return modeStr;
    }
}

KateViInputMode::KateViInputMode(KateViewInternal *viewInternal, KateViGlobal *global)
    : KateAbstractInputMode(viewInternal)
    , m_viModeEmulatedCommandBar(0)
    , m_viGlobal(global)
    , m_caret(KateRenderer::Block)
    , m_activated(false)
{
    m_relLineNumbers = KateViewConfig::global()->viRelativeLineNumbers();
    m_viModeManager = new KateViInputModeManager(this, view(), viewInternal);
}

KateViInputMode::~KateViInputMode()
{
    delete m_viModeManager;
    delete m_viModeEmulatedCommandBar;
}

void KateViInputMode::activate()
{
    m_activated = true;
    setCaretStyle(KateRenderer::Block); // TODO: can we end up in insert mode?
    reset(); // TODO: is this necessary? (well, not anymore I guess)

    if (view()->selection()) {
        m_viModeManager->changeViMode(VisualMode);
        view()->setCursorPosition(Cursor(view()->selectionRange().end().line(), view()->selectionRange().end().column() - 1));
        m_viModeManager->m_viVisualMode->updateSelection();
    }
    viewInternal()->iconBorder()->setRelLineNumbersOn(m_relLineNumbers);
}

void KateViInputMode::deactivate()
{
    if (m_viModeEmulatedCommandBar) {
        m_viModeEmulatedCommandBar->hideMe();
    }

    // make sure to turn off edits mergin when leaving vi input mode
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
    m_viModeManager = new KateViInputModeManager(this, view(), viewInternal());

    if (m_viModeEmulatedCommandBar) {
        m_viModeEmulatedCommandBar->setViInputModeManager(m_viModeManager);
    }
}

bool KateViInputMode::overwrite() const
{
    return  m_viModeManager->getCurrentViMode() == ReplaceMode;
}

void KateViInputMode::overwrittenChar(const QChar &c)
{
    m_viModeManager->getViReplaceMode()->overwrittenChar(c);
}

void KateViInputMode::clearSelection()
{
    // do nothing, handled elsewhere
}

bool KateViInputMode::stealKey(const QKeyEvent *k) const
{
    const bool steal = KateViewConfig::global()->viInputModeStealKeys();
    return steal && (m_viModeManager->getCurrentViMode() != InsertMode || k->modifiers() == Qt::ControlModifier || k->key() == Qt::Key_Insert);
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

    if (m_viModeManager->isRecordingMacro()) {
        currentMode.prepend(QLatin1String("(") + i18n("recording") + QLatin1String(") "));
    }

    QString cmd = m_viModeManager->getVerbatimKeys();
    if (!cmd.isEmpty()) {
        currentMode.prepend(QString::fromLatin1("<em>%1</em> ").arg(cmd));
    }

    return QString::fromLatin1("<b>%1</b>").arg(currentMode);
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
    viModeEmulatedCommandBar()->init(KateViEmulatedCommandBar::SearchForward);
}

void KateViInputMode::findSelectedForwards()
{
    m_viModeManager->findNext();
}

void KateViInputMode::findSelectedBackwards()
{
    m_viModeManager->findPrevious();
}

void KateViInputMode::findReplace()
{
    showViModeEmulatedCommandBar();
    viModeEmulatedCommandBar()->init(KateViEmulatedCommandBar::SearchForward);
}

void KateViInputMode::findNext()
{
    m_viModeManager->findNext();
}

void KateViInputMode::findPrevious()
{
    m_viModeManager->findPrevious();
}

void KateViInputMode::activateCommandLine()
{
    showViModeEmulatedCommandBar();
    viModeEmulatedCommandBar()->init(KateViEmulatedCommandBar::Command);
}

void KateViInputMode::showViModeEmulatedCommandBar()
{
    view()->bottomViewBar()->addBarWidget(viModeEmulatedCommandBar());
    view()->bottomViewBar()->showBarWidget(viModeEmulatedCommandBar());
}

KateViEmulatedCommandBar *KateViInputMode::viModeEmulatedCommandBar()
{
    if (!m_viModeEmulatedCommandBar) {
        m_viModeEmulatedCommandBar = new KateViEmulatedCommandBar(m_viModeManager, view());
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
    if (m_viModeManager->getCurrentViMode() == InsertMode || m_viModeManager->getCurrentViMode() == ReplaceMode) {
        if (m_viModeManager->handleKeypress(e)) {
            return true;
        } else if (e->modifiers() != Qt::NoModifier && e->modifiers() != Qt::ShiftModifier) {
            // re-post key events not handled if they have a modifier other than shift
            QEvent *copy = new QKeyEvent(e->type(), e->key(), e->modifiers(), e->text(),
                                         e->isAutoRepeat(), e->count());
            QCoreApplication::postEvent(viewInternal()->parent(), copy);
        }
    } else { // !InsertMode
        if (!m_viModeManager->handleKeypress(e)) {
            // we didn't need that keypress, un-steal it :-)
            QEvent *copy = new QKeyEvent(e->type(), e->key(), e->modifiers(), e->text(),
                                            e->isAutoRepeat(), e->count());
            QCoreApplication::postEvent(viewInternal()->parent(), copy);
        }
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

KateViGlobal *KateViInputMode::viGlobal()
{
    return m_viGlobal;
}

KateViInputModeManager *KateViInputMode::viInputModeManager()
{
    return m_viModeManager;
}

void KateViInputMode::launchInteractiveCommand(const QString &)
{
    // do nothing so far
}

QString KateViInputMode::bookmarkLabel(int line) const
{
    return m_viModeManager->getMarksOnTheLine(line);
}

void KateViInputMode::setCaretStyle(KateRenderer::caretStyles caret)
{
    if (m_caret != caret) {
        m_caret = caret;

        view()->renderer()->setCaretStyle(m_caret);
        view()->renderer()->setDrawCaret(true);
        viewInternal()->paintCursor();
    }
}
