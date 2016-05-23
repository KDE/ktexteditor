#include "activemode.h"
#include "completer.h"
#include "emulatedcommandbar.h"
#include "matchhighlighter.h"

using namespace KateVi;

CompletionStartParams ActiveMode::completionInvoked(Completer::CompletionInvocation invocationType)
{
    Q_UNUSED(invocationType);
    return CompletionStartParams();
}

ActiveMode::~ActiveMode()
{

}

void ActiveMode::hideAllWidgetsExcept(QWidget* widgetToKeepVisible)
{
    m_emulatedCommandBar->hideAllWidgetsExcept(widgetToKeepVisible);
}

void ActiveMode::moveCursorTo(const KTextEditor::Cursor &cursorPos)
{
    m_emulatedCommandBar->moveCursorTo(cursorPos);
}

void ActiveMode::updateMatchHighlight(const KTextEditor::Range& matchRange)
{
    m_matchHighligher->updateMatchHighlight(matchRange);
}

void ActiveMode::close( bool wasAborted )
{
    m_emulatedCommandBar->m_wasAborted = wasAborted;
    m_emulatedCommandBar->hideMe();
}

void ActiveMode::closeWithStatusMessage(const QString& exitStatusMessage)
{
    m_emulatedCommandBar->closeWithStatusMessage(exitStatusMessage);
}

void ActiveMode::startCompletion ( const CompletionStartParams& completionStartParams )
{
    m_emulatedCommandBar->m_completer->startCompletion(completionStartParams);
}

