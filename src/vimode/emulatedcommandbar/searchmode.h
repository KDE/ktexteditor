/*  This file is part of the KDE libraries and the Kate part.
 *
 *  Copyright (C) 2013-2016 Simon St James <kdedevel@etotheipiplusone.com>
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

#ifndef KATEVI_EMULATED_COMMAND_BAR_SEARCHMODE_H
#define KATEVI_EMULATED_COMMAND_BAR_SEARCHMODE_H

#include "activemode.h"
#include "../searcher.h"

namespace KTextEditor {
    class ViewPrivate;
}

#include <KTextEditor/Cursor>

namespace KateVi
{
class EmulatedCommandBar;
QString vimRegexToQtRegexPattern(const QString &vimRegexPattern); // TODO - move these generic helper functions into their own file?
QString withCaseSensitivityMarkersStripped(const QString &originalSearchTerm);
QString ensuredCharEscaped(const QString &originalString, QChar charToEscape);
QStringList reversed(const QStringList &originalList);

class SearchMode : public ActiveMode
{
public:
    SearchMode(EmulatedCommandBar* emulatedCommandBar, MatchHighlighter* matchHighlighter, InputModeManager* viInputModeManager, KTextEditor::ViewPrivate* view, QLineEdit* edit);
    ~SearchMode() override
    {
    }
    enum class SearchDirection { Forward, Backward };
    void init(SearchDirection);
    bool handleKeyPress ( const QKeyEvent* keyEvent ) override;
    void editTextChanged(const QString &newText) override;
    CompletionStartParams completionInvoked(Completer::CompletionInvocation invocationType) override;
    void completionChosen() override;
    void deactivate(bool wasAborted) override;
    bool isSendingSyntheticSearchCompletedKeypress() const
    {
        return m_isSendingSyntheticSearchCompletedKeypress;
    }
private:
    QLineEdit *m_edit = nullptr;
    SearchDirection m_searchDirection;
    KTextEditor::Cursor m_startingCursorPos;
    KateVi::Searcher::SearchParams m_currentSearchParams;
    CompletionStartParams activateSearchHistoryCompletion();
    enum BarBackgroundStatus { Normal, MatchFound, NoMatchFound };
    void setBarBackground(BarBackgroundStatus status);
    bool m_isSendingSyntheticSearchCompletedKeypress = false;
};
}

#endif
