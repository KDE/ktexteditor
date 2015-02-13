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

#ifndef __KATE_VI_INPUT_MODE_H__
#define __KATE_VI_INPUT_MODE_H__

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
    virtual ~KateViInputMode();

    KTextEditor::View::ViewMode viewMode() const Q_DECL_OVERRIDE;
    QString viewModeHuman() const Q_DECL_OVERRIDE;
    KTextEditor::View::InputMode viewInputMode() const Q_DECL_OVERRIDE;
    QString viewInputModeHuman() const Q_DECL_OVERRIDE;

    void activate() Q_DECL_OVERRIDE;
    void deactivate() Q_DECL_OVERRIDE;
    void reset() Q_DECL_OVERRIDE;

    bool overwrite() const Q_DECL_OVERRIDE;
    void overwrittenChar(const QChar &) Q_DECL_OVERRIDE;

    void clearSelection() Q_DECL_OVERRIDE;
    bool stealKey(const QKeyEvent *) const Q_DECL_OVERRIDE;

    void gotFocus() Q_DECL_OVERRIDE;
    void lostFocus() Q_DECL_OVERRIDE;

    void readSessionConfig(const KConfigGroup &config) Q_DECL_OVERRIDE;
    void writeSessionConfig(KConfigGroup &config) Q_DECL_OVERRIDE;
    void updateRendererConfig() Q_DECL_OVERRIDE;
    void updateConfig() Q_DECL_OVERRIDE;
    void readWriteChanged(bool rw) Q_DECL_OVERRIDE;

    void find() Q_DECL_OVERRIDE;
    void findSelectedForwards() Q_DECL_OVERRIDE;
    void findSelectedBackwards() Q_DECL_OVERRIDE;
    void findReplace() Q_DECL_OVERRIDE;
    void findNext() Q_DECL_OVERRIDE;
    void findPrevious() Q_DECL_OVERRIDE;

    void activateCommandLine() Q_DECL_OVERRIDE;

    bool keyPress(QKeyEvent *) Q_DECL_OVERRIDE;
    bool blinkCaret() const Q_DECL_OVERRIDE;
    KateRenderer::caretStyles caretStyle() const Q_DECL_OVERRIDE;

    void toggleInsert() Q_DECL_OVERRIDE;
    void launchInteractiveCommand(const QString &command) Q_DECL_OVERRIDE;

    QString bookmarkLabel(int line) const Q_DECL_OVERRIDE;

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

    // configs
    bool m_relLineNumbers;
    bool m_activated;
};

#endif
