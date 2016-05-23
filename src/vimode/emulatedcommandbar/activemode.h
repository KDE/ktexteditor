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
}

namespace KateVi
{
class EmulatedCommandBar;
class CompletionStartParams;
class MatchHighlighter;

class ActiveMode
{
public:
    ActiveMode(EmulatedCommandBar* emulatedCommandBar, MatchHighlighter* matchHighlighter)
    : m_emulatedCommandBar(emulatedCommandBar),
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
protected:
    // Helper methods.
    void hideAllWidgetsExcept(QWidget* widgetToKeepVisible);
    void moveCursorTo(const KTextEditor::Cursor &cursorPos);
    void updateMatchHighlight(const KTextEditor::Range &matchRange);
    void close(bool wasAborted);
    void closeWithStatusMessage(const QString& exitStatusMessage);
    void startCompletion(const CompletionStartParams& completionStartParams);
    EmulatedCommandBar *m_emulatedCommandBar = nullptr;
private:
    MatchHighlighter *m_matchHighligher = nullptr;
};
}
#endif
