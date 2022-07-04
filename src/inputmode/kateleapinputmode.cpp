/*
    SPDX-FileCopyrightText: KDE Developers

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "kateleapinputmode.h"
#include "kateconfig.h"
#include "katedocument.h"
#include "katematch.h"
#include "katerenderer.h"
#include "kateview.h"
#include "kateviewinternal.h"

#include <KLocalizedString>

#include <QCoreApplication>

struct KateLeapInputMode::Private {
    KTextEditor::View::ViewMode mode = KTextEditor::View::LeapModeNormal;
    QString leapBuffer;
    KTextEditor::Cursor leapFrom;
    KTextEditor::Cursor leapTo;
    KTextEditor::Range leapingText;

    bool pressedLeapBack = false;
    bool pressedLeapForward = false;
    bool releasedLeapBack = false;
    bool releasedLeapForward = false;
};

KateLeapInputMode::KateLeapInputMode(KateViewInternal *viewInternal)
    : KateAbstractInputMode(viewInternal)
    , d(new Private)
{
}

void KateLeapInputMode::activate()
{
}

void KateLeapInputMode::deactivate()
{
}

void KateLeapInputMode::reset()
{
}

bool KateLeapInputMode::overwrite() const
{
    return false;
}

void KateLeapInputMode::overwrittenChar(const QChar &c)
{
}

void KateLeapInputMode::clearSelection()
{
}

bool KateLeapInputMode::stealKey(QKeyEvent *k)
{
    return false;
}

KTextEditor::View::InputMode KateLeapInputMode::viewInputMode() const
{
    return KTextEditor::View::LeapInputMode;
}

QString KateLeapInputMode::viewInputModeHuman() const
{
    return i18n("leap-mode");
}

KTextEditor::View::ViewMode KateLeapInputMode::viewMode() const
{
    return KTextEditor::View::LeapModeNormal;
}

QString KateLeapInputMode::viewModeHuman() const
{
    return QString();
}

void KateLeapInputMode::gotFocus()
{
}

void KateLeapInputMode::lostFocus()
{
}

void KateLeapInputMode::readSessionConfig(const KConfigGroup &config)
{
}

void KateLeapInputMode::writeSessionConfig(KConfigGroup &config)
{
}

void KateLeapInputMode::updateConfig()
{
}

void KateLeapInputMode::readWriteChanged(bool)
{
    // nothing todo
}

void KateLeapInputMode::find()
{
}

void KateLeapInputMode::findSelectedForwards()
{
}

void KateLeapInputMode::findSelectedBackwards()
{
}

void KateLeapInputMode::findReplace()
{
}

void KateLeapInputMode::findNext()
{
}

void KateLeapInputMode::findPrevious()
{
}

void KateLeapInputMode::activateCommandLine()
{
}

void KateLeapInputMode::updateRendererConfig()
{
}

// TODO: figure out a more portable way of distinguishing between left alt and right alt that doesn't
// rely on keyboard layout being configured to treat right alt as altgr

auto isLeapBack(QKeyEvent *event)
{
    if (event->nativeScanCode() == 64)
        return true;
    return false;
}

auto isLeapForward(QKeyEvent *event)
{
    if (event->nativeScanCode() == 108)
        return true;
    return false;
}

bool KateLeapInputMode::keyPress(QKeyEvent *e)
{
    using namespace KTextEditor;

    if (isLeapBack(e)) {
        d->pressedLeapBack = true;
    } else if (isLeapForward(e)) {
        d->pressedLeapForward = true;
    }

    if (d->mode == View::LeapModeLeapBack || d->mode == View::LeapModeLeapForward) {
        // only initialise the leapFrom position if we actually type anything
        // to avoid setting it in the case of a leap select
        if (d->leapBuffer.isEmpty() && !isLeapBack(e) && !isLeapForward(e)) {
            d->leapFrom = view()->cursorPosition();
        }
        d->leapBuffer += e->text();

        SearchOptions enabledOptions = Default;
        if (d->mode == View::LeapModeLeapBack) {
            enabledOptions |= Backwards;
        }
        KateMatch match(view()->doc(), enabledOptions);

        view()->selectionRange();

        if (d->mode == View::LeapModeLeapBack) {
            const Range inputRange = KTextEditor::Range(view()->document()->documentRange().start(), d->leapFrom);
            match.searchText(inputRange, d->leapBuffer);
        } else {
            const Range inputRange = KTextEditor::Range(d->leapFrom, view()->document()->documentEnd());
            match.searchText(inputRange, d->leapBuffer);
        }

        if (!match.isValid()) {
            viewInternal()->editSetCursor(d->leapFrom);
            return true;
        }

        view()->setCursorPosition(match.range().start());
        d->leapTo = match.range().start();

        return true;
    }

    if (isLeapBack(e)) {
        d->mode = View::LeapModeLeapBack;
        return true;
    } else if (isLeapForward(e)) {
        d->mode = View::LeapModeLeapForward;
        return true;
    } else if (e->key() == Qt::Key_Backspace) {
        view()->backspace();
    }
    return false;
}

bool KateLeapInputMode::keyRelease(QKeyEvent *e)
{
    if (isLeapBack(e) || isLeapForward(e)) {
        if (d->pressedLeapBack && d->pressedLeapForward) {
            if (isLeapBack(e)) {
                d->releasedLeapBack = true;
            } else {
                d->releasedLeapForward = true;
            }
            if (d->releasedLeapForward && d->releasedLeapBack) {
                const auto range = d->leapFrom < d->leapTo ? KTextEditor::Range(d->leapFrom, d->leapTo) : KTextEditor::Range(d->leapTo, d->leapFrom);

                view()->setSelection(range);

                d->leapingText = range;

                d->pressedLeapBack = false;
                d->pressedLeapForward = false;
                d->releasedLeapBack = false;
                d->releasedLeapForward = false;

                d->mode = KTextEditor::View::LeapModeNormal;
                d->leapBuffer.clear();
            }
            return true;
        }
        if (d->leapBuffer.isEmpty()) {
            if (isLeapBack(e)) {
                view()->cursorLeft();
            } else {
                view()->cursorRight();
            }
        }
        d->mode = KTextEditor::View::LeapModeNormal;
        d->leapBuffer.clear();

        if (isLeapBack(e)) {
            d->pressedLeapBack = false;
        } else if (isLeapForward(e)) {
            d->pressedLeapForward = false;
        }

        return true;
    }
    return false;
}

bool KateLeapInputMode::blinkCaret() const
{
    return false;
}

KateRenderer::caretStyles KateLeapInputMode::caretStyle() const
{
    return KateRenderer::Block;
}

void KateLeapInputMode::toggleInsert()
{
}

void KateLeapInputMode::launchInteractiveCommand(const QString &)
{
}

QString KateLeapInputMode::bookmarkLabel(int line) const
{
    return QString();
}
