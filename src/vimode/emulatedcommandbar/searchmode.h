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
    virtual ~SearchMode()
    {
    };
    enum class SearchDirection { Forward, Backward };
    void init(SearchDirection);
    virtual bool handleKeyPress ( const QKeyEvent* keyEvent );
    virtual void editTextChanged(const QString &newText);
    virtual CompletionStartParams completionInvoked(Completer::CompletionInvocation invocationType);
    virtual void completionChosen();
    virtual void deactivate(bool wasAborted);
    bool isSendingSyntheticSearchCompletedKeypress() const
    {
        return m_isSendingSyntheticSearchCompletedKeypress;
    }
private:
    EmulatedCommandBar *m_emulatedCommandBar = nullptr;
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
