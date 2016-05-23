#ifndef KATEVI_EMULATED_COMMAND_BAR_ACTIVEMODE_H
#define KATEVI_EMULATED_COMMAND_BAR_ACTIVEMODE_H

#include "completer.h"

class QKeyEvent;
class QString;
class QWidget;

namespace KTextEditor
{
    class Cursor;
    class Range;
    class ViewPrivate;
}

namespace KateVi
{
class EmulatedCommandBar;
class CompletionStartParams;
class MatchHighlighter;
class InputModeManager;

class ActiveMode
{
public:
    ActiveMode(EmulatedCommandBar* emulatedCommandBar, MatchHighlighter* matchHighlighter, InputModeManager* viInputModeManager, KTextEditor::ViewPrivate* view)
    : m_emulatedCommandBar(emulatedCommandBar),
      m_viInputModeManager(viInputModeManager),
      m_view(view),
      m_matchHighligher(matchHighlighter)
    {
    }
    virtual ~ActiveMode() = 0;
    virtual bool handleKeyPress(const QKeyEvent *keyEvent) = 0;
    virtual void editTextChanged(const QString &newText)
    {
        Q_UNUSED(newText);
    }
    virtual KateVi::CompletionStartParams completionInvoked(Completer::CompletionInvocation invocationType);
    virtual void completionChosen()
    {
    }
    virtual void deactivate(bool wasAborted) = 0;
    void setViInputModeManager(InputModeManager *viInputModeManager);
protected:
    // Helper methods.
    void hideAllWidgetsExcept(QWidget* widgetToKeepVisible);
    void updateMatchHighlight(const KTextEditor::Range &matchRange);
    void close(bool wasAborted);
    void closeWithStatusMessage(const QString& exitStatusMessage);
    void startCompletion(const CompletionStartParams& completionStartParams);
    void moveCursorTo(const KTextEditor::Cursor &cursorPos);
    EmulatedCommandBar *m_emulatedCommandBar = nullptr;
    InputModeManager* m_viInputModeManager = nullptr;
    KTextEditor::ViewPrivate* m_view = nullptr;
private:
    MatchHighlighter *m_matchHighligher = nullptr;
};
}
#endif
