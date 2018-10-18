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

#ifndef KATE_VI_INPUT_MODE_H
#define KATE_VI_INPUT_MODE_H

#include "kateabstractinputmode.h"

namespace KateVi {
    class GlobalState;
    class InputModeManager;
    class EmulatedCommandBar;
}
class KateViInputModeFactory;

class KTEXTEDITOR_EXPORT KateViInputMode : public KateAbstractInputMode
{
    explicit KateViInputMode(KateViewInternal *viewInternal, KateVi::GlobalState *global);
    friend KateViInputModeFactory;

public:
    ~KateViInputMode() override;

    KTextEditor::View::ViewMode viewMode() const override;
    QString viewModeHuman() const override;
    KTextEditor::View::InputMode viewInputMode() const override;
    QString viewInputModeHuman() const override;

    void activate() override;
    void deactivate() override;
    void reset() override;

    bool overwrite() const override;
    void overwrittenChar(const QChar &) override;

    void clearSelection() override;
    bool stealKey(QKeyEvent *) override;

    void gotFocus() override;
    void lostFocus() override;

    void readSessionConfig(const KConfigGroup &config) override;
    void writeSessionConfig(KConfigGroup &config) override;
    void updateRendererConfig() override;
    void updateConfig() override;
    void readWriteChanged(bool rw) override;

    void find() override;
    void findSelectedForwards() override;
    void findSelectedBackwards() override;
    void findReplace() override;
    void findNext() override;
    void findPrevious() override;

    void activateCommandLine() override;

    bool keyPress(QKeyEvent *) override;
    bool blinkCaret() const override;
    KateRenderer::caretStyles caretStyle() const override;

    void toggleInsert() override;
    void launchInteractiveCommand(const QString &command) override;

    QString bookmarkLabel(int line) const override;

public:
    void showViModeEmulatedCommandBar();
    KateVi::EmulatedCommandBar *viModeEmulatedCommandBar();
    inline KateVi::GlobalState *globalState() const { return m_viGlobal; }
    inline KateVi::InputModeManager *viInputModeManager() const { return m_viModeManager; }
    inline bool isActive() const { return m_activated; }
    void setCaretStyle(const KateRenderer::caretStyles caret);

private:
    KateVi::InputModeManager *m_viModeManager;
    KateVi::EmulatedCommandBar *m_viModeEmulatedCommandBar;
    KateVi::GlobalState *m_viGlobal;
    KateRenderer::caretStyles m_caret;

    bool m_nextKeypressIsOverriddenShortCut;

    // configs
    bool m_relLineNumbers;
    bool m_activated;
};

#endif
