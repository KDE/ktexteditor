#ifndef KATEVI_EMULATED_COMMAND_BAR_INTERACTIVESEDREPLACEMODE_H
#define KATEVI_EMULATED_COMMAND_BAR_INTERACTIVESEDREPLACEMODE_H

#include "activemode.h"

#include "../cmds.h"

#include <QSharedPointer>

class QKeyEvent;
class QLabel;

namespace KateVi
{
class EmulatedCommandBar;
class MatchHighlighter;

class InteractiveSedReplaceMode : public ActiveMode
{
public:
    InteractiveSedReplaceMode(EmulatedCommandBar* emulatedCommandBar, MatchHighlighter* matchHighlighter, InputModeManager* viInputModeManager, KTextEditor::ViewPrivate* view);
    virtual ~InteractiveSedReplaceMode()
    {
    };
    void activate(QSharedPointer<SedReplace::InteractiveSedReplacer> interactiveSedReplace);
    bool isActive() const
    {
        return m_isActive;
    }
    virtual bool handleKeyPress(const QKeyEvent* keyEvent);
    virtual void deactivate(bool wasAborted);
    QWidget *label();
private:
    void updateInteractiveSedReplaceLabelText();
    void finishInteractiveSedReplace();
    QSharedPointer<SedReplace::InteractiveSedReplacer> m_interactiveSedReplacer;
    bool m_isActive;
    QLabel *m_interactiveSedReplaceLabel;
};
}

#endif
