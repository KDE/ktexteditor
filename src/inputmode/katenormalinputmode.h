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

#ifndef __KATE_NORMAL_INPUT_MODE_H__
#define __KATE_NORMAL_INPUT_MODE_H__

#include "kateabstractinputmode.h"

class KateNormalInputModeFactory;
class KateSearchBar;
class KateCommandLineBar;

class KateNormalInputMode : public KateAbstractInputMode
{
    KateNormalInputMode(KateViewInternal *viewInternal);
    friend KateNormalInputModeFactory;

public:
    virtual ~KateNormalInputMode();

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
    bool stealKey(QKeyEvent *) Q_DECL_OVERRIDE;

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

private:
    KateSearchBar *searchBar(bool initHintAsPower = false);
    bool hasSearchBar() const;
    KateCommandLineBar *cmdLineBar();

private:
    KateSearchBar *m_searchBar;
    KateCommandLineBar *m_cmdLine;
};

#endif
