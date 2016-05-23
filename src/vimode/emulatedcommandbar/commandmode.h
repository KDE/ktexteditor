#ifndef KATEVI_EMULATED_COMMAND_BAR_COMMANDMODE_H
#define KATEVI_EMULATED_COMMAND_BAR_COMMANDMODE_H

#include "activemode.h"

#include <KTextEditor/Command>

#include <QHash>

namespace KTextEditor {
    class ViewPrivate;
}

namespace KateVi
{
class EmulatedCommandBar;
class MatchHighlighter;
class InteractiveSedReplaceMode;
class Completer;
class InputModeManager;

class CommandMode : public ActiveMode
{
public:
    CommandMode(EmulatedCommandBar* emulatedCommandBar, MatchHighlighter* matchHighlighter, KTextEditor::ViewPrivate* view,  QLineEdit* edit, InteractiveSedReplaceMode *interactiveSedReplaceMode, Completer* completer);
    virtual ~CommandMode()
    {
    }
    void setViInputModeManager(InputModeManager *viInputModeManager);
    virtual bool handleKeyPress ( const QKeyEvent* keyEvent );
    virtual void editTextChanged(const QString &newText);
    virtual CompletionStartParams completionInvoked(Completer::CompletionInvocation invocationType);
    virtual void completionChosen();
    void deactivate(bool wasAborted);
    QString executeCommand(const QString &commandToExecute);
private:
    CompletionStartParams activateCommandCompletion();
    CompletionStartParams activateCommandHistoryCompletion();
    CompletionStartParams activateSedFindHistoryCompletion();
    CompletionStartParams activateSedReplaceHistoryCompletion();
    QString withoutRangeExpression();
    QString rangeExpression();
    QString withSedFindTermReplacedWith(const QString &newFindTerm);
    QString withSedDelimiterEscaped(const QString &text);
    bool isCursorInFindTermOfSed();
    bool isCursorInReplaceTermOfSed();
    QString sedFindTerm();
    QString sedReplaceTerm();
    /**
        * Stuff to do with expressions of the form:
        *
        *   s/find/replace/<sedflags>
        */
    struct ParsedSedExpression {
        bool parsedSuccessfully;
        int findBeginPos;
        int findEndPos;
        int replaceBeginPos;
        int replaceEndPos;
        QChar delimiter;
    };
    /**
        * The "range expression" is the (optional) expression before the command that describes
        * the range over which the command should be run e.g. '<,'>.  @see CommandRangeExpressionParser
        */
    CommandMode::ParsedSedExpression parseAsSedExpression();
    void replaceCommandBeforeCursorWith(const QString &newCommand);
    int commandBeforeCursorBegin();
    QLineEdit *m_edit;
    InputModeManager *m_viInputModeManager = nullptr;
    KTextEditor::ViewPrivate *m_view;
    InteractiveSedReplaceMode *m_interactiveSedReplaceMode;
    Completer *m_completer;
    KCompletion m_cmdCompletion;
    QHash<QString, KTextEditor::Command *> m_cmdDict;
    KTextEditor::Command *queryCommand(const QString &cmd) const;
};
}

#endif
