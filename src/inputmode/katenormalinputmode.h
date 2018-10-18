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

#ifndef KATE_NORMAL_INPUT_MODE_H
#define KATE_NORMAL_INPUT_MODE_H

#include "kateabstractinputmode.h"

class KateNormalInputModeFactory;
class KateSearchBar;
class KateCommandLineBar;

class KateNormalInputMode : public KateAbstractInputMode
{
    explicit KateNormalInputMode(KateViewInternal *viewInternal);
    friend KateNormalInputModeFactory;

public:
    ~KateNormalInputMode() override;

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

private:
    /**
     * Search bar mode:
     *   - Setup Incremental mode, among other things: potential new search pattern
     *   - Setup Power mode, aka find & replace: Also potential new search pattern
     *   - Use current mode and current search pattern or if no Search bar exists, launch Incremental mode
     */
    enum SearchBarMode {
        IncrementalSearchBar,
        PowerSearchBar,
        IncrementalSearchBarOrKeepMode
    };

    /**
     * Get search bar, create it on demand. (with right mode)
     * @param mode wanted search bar mode
     * @return search bar widget
     */
    KateSearchBar *searchBar(const SearchBarMode mode);

    /**
     * search bar around?
     * @return search bar around?
     */
    bool hasSearchBar() const
    {
        return m_searchBar;
    }

    /**
     * Get command line bar, create it on demand.
     * @return command line bar, created if not already there
     */
    KateCommandLineBar *cmdLineBar();

private:
    KateSearchBar *m_searchBar;
    KateCommandLineBar *m_cmdLine;
};

#endif
