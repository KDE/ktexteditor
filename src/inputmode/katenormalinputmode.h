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

    virtual KTextEditor::View::ViewMode viewMode() const;
    virtual QString viewModeHuman() const;
    virtual KTextEditor::View::InputMode viewInputMode() const;
    virtual QString viewInputModeHuman() const;

    virtual void activate();
    virtual void deactivate();
    virtual void reset();

    virtual bool overwrite() const;
    virtual void overwrittenChar(const QChar &);

    virtual void clearSelection();
    virtual bool stealKey(const QKeyEvent *) const;

    virtual void gotFocus();
    virtual void lostFocus();

    virtual void readSessionConfig(const KConfigGroup &config);
    virtual void writeSessionConfig(KConfigGroup &config);
    virtual void updateRendererConfig();
    virtual void updateConfig();
    virtual void readWriteChanged(bool rw);

    virtual void find();
    virtual void findSelectedForwards();
    virtual void findSelectedBackwards();
    virtual void findReplace();
    virtual void findNext();
    virtual void findPrevious();

    virtual void activateCommandLine();

    virtual bool keyPress(QKeyEvent *);
    virtual bool blinkCaret() const;
    virtual KateRenderer::caretStyles caretStyle() const;

    virtual void toggleInsert();
    virtual void launchInteractiveCommand(const QString &command);

    virtual QString bookmarkLabel(int line) const;

private:
    KateSearchBar *searchBar(bool initHintAsPower = false);
    bool hasSearchBar() const;
    KateCommandLineBar *cmdLineBar();

private:
    KateSearchBar *m_searchBar;
    KateCommandLineBar *m_cmdLine;
};

#endif
