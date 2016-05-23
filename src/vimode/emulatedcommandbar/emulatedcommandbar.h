/*  This file is part of the KDE libraries and the Kate part.
 *
 *  Copyright (C) 2013 Simon St James <kdedevel@etotheipiplusone.com>
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

#ifndef KATEVI_EMULATED_COMMAND_BAR_H
#define KATEVI_EMULATED_COMMAND_BAR_H

#include "kateviewhelpers.h"
#include <vimode/cmds.h>

#include <ktexteditor/range.h>
#include <ktexteditor/movingrange.h>
#include "../searcher.h"
#include "completer.h"

namespace KTextEditor {
    class ViewPrivate;
    class Command;
}

class QLabel;
class QCompleter;
class QStringListModel;

namespace KateVi
{
class MatchHighlighter;

/**
 * A KateViewBarWidget that attempts to emulate some of the features of Vim's own command bar,
 * including insertion of register contents via ctr-r<registername>; dismissal via
 * ctrl-c and ctrl-[; bi-directional incremental searching, with SmartCase; interactive sed-replace;
 * plus a few extensions such as completion from document and navigable sed search and sed replace history.
 */
class KTEXTEDITOR_EXPORT EmulatedCommandBar : public KateViewBarWidget
{
    Q_OBJECT

public:
    enum Mode { NoMode, SearchForward, SearchBackward, Command };
    explicit EmulatedCommandBar(InputModeManager *viInputModeManager, QWidget *parent = 0);
    virtual ~EmulatedCommandBar();
    void init(Mode mode, const QString &initialText = QString());
    bool isActive();
    void setCommandResponseMessageTimeout(long commandResponseMessageTimeOutMS);
    bool handleKeyPress(const QKeyEvent *keyEvent);
    bool isSendingSyntheticSearchCompletedKeypress();

    void startInteractiveSearchAndReplace(QSharedPointer<SedReplace::InteractiveSedReplacer> interactiveSedReplace);
    QString executeCommand(const QString &commandToExecute);

    void setViInputModeManager(InputModeManager *viInputModeManager);

private:

    InputModeManager *m_viInputModeManager;
    bool m_isActive = false;
    Mode m_mode = NoMode;
    KTextEditor::ViewPrivate *m_view;
    QLineEdit *m_edit;
    QLabel *m_barTypeIndicator;
    void showBarTypeIndicator(Mode mode);
    bool m_wasAborted = true;
    bool m_suspendEditEventFiltering = false;
    bool m_waitingForRegister = false ;
    QLabel *m_waitingForRegisterIndicator;
    bool m_insertedTextShouldBeEscapedForSearchingAsLiteral = false;

    void hideAllWidgetsExcept(QWidget* widgetToKeepVisible);

    QScopedPointer<MatchHighlighter> m_matchHighligher;

    class ActiveMode;
    class Completer
    {
    public:
        enum class CompletionInvocation { ExtraContext, NormalContext };
        Completer(EmulatedCommandBar* emulatedCommandBar, KTextEditor::ViewPrivate* view, QLineEdit* edit);
        void startCompletion(const CompletionStartParams& completionStartParams);
        void deactivateCompletion();
        bool isCompletionActive() const;
        bool isNextTextChangeDueToCompletionChange() const;
        bool completerHandledKeypress(const QKeyEvent* keyEvent);
        void editTextChanged(const QString &newText);
        void setCurrentMode(ActiveMode* currentMode);
    private:
        QLineEdit *m_edit;
        KTextEditor::ViewPrivate *m_view;
        ActiveMode *m_currentMode = nullptr;

        void setCompletionIndex(int index);
        void currentCompletionChanged();
        void updateCompletionPrefix();
        CompletionStartParams activateWordFromDocumentCompletion();
        QString wordBeforeCursor();
        int wordBeforeCursorBegin();
        void abortCompletionAndResetToPreCompletion();

        QCompleter *m_completer;
        QStringListModel *m_completionModel;
        QString m_textToRevertToIfCompletionAborted;
        int m_cursorPosToRevertToIfCompletionAborted;
        bool m_isNextTextChangeDueToCompletionChange = false;
        CompletionStartParams m_currentCompletionStartParams;
        CompletionStartParams::CompletionType m_currentCompletionType = CompletionStartParams::None;
    };
    QScopedPointer<Completer> m_completer;

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
        virtual CompletionStartParams completionInvoked(Completer::CompletionInvocation invocationType)
        {
            Q_UNUSED(invocationType);
            return CompletionStartParams();
        };
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
        EmulatedCommandBar *m_emulatedCommandBar;
    private:
        MatchHighlighter *m_matchHighligher;
    };
    friend ActiveMode;

    class InteractiveSedReplaceMode : public ActiveMode
    {
    public:
        InteractiveSedReplaceMode(EmulatedCommandBar* emulatedCommandBar, MatchHighlighter* matchHighlighter);
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


    class SearchMode : public ActiveMode
    {
    public:
        SearchMode(EmulatedCommandBar* emulatedCommandBar, MatchHighlighter* matchHighlighter, KTextEditor::ViewPrivate* view, QLineEdit* edit);
        virtual ~SearchMode()
        {
        };
        enum class SearchDirection { Forward, Backward };
        void init(SearchDirection);
        void setViInputModeManager(InputModeManager *viInputModeManager);
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
        KTextEditor::ViewPrivate *m_view = nullptr;
        InputModeManager *m_viInputModeManager = nullptr;
        QLineEdit *m_edit = nullptr;
        SearchDirection m_searchDirection;
        KTextEditor::Cursor m_startingCursorPos;
        KateVi::Searcher::SearchParams m_currentSearchParams;
        CompletionStartParams activateSearchHistoryCompletion();
        enum BarBackgroundStatus { Normal, MatchFound, NoMatchFound };
        void setBarBackground(BarBackgroundStatus status);
        bool m_isSendingSyntheticSearchCompletedKeypress = false;
    };

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
    QScopedPointer<InteractiveSedReplaceMode> m_interactiveSedReplaceMode;
    QScopedPointer<SearchMode> m_searchMode;
    QScopedPointer<CommandMode> m_commandMode;

    void moveCursorTo(const KTextEditor::Cursor &cursorPos);

    void switchToMode(ActiveMode *newMode);
    ActiveMode *m_currentMode = nullptr;

    bool barHandledKeypress(const QKeyEvent* keyEvent);
    void insertRegisterContents(const QKeyEvent *keyEvent);
    bool eventFilter(QObject *object, QEvent *event) Q_DECL_OVERRIDE;
    void deleteSpacesToLeftOfCursor();
    void deleteWordCharsToLeftOfCursor();
    bool deleteNonWordCharsToLeftOfCursor();


    void closed() Q_DECL_OVERRIDE;
    void closeWithStatusMessage(const QString& exitStatusMessage);
    QTimer *m_exitStatusMessageDisplayHideTimer;
    QLabel *m_exitStatusMessageDisplay;
    long m_exitStatusMessageHideTimeOutMS = 4000;
private Q_SLOTS:
    void editTextChanged(const QString &newText);
    void startHideExitStatusMessageTimer();
};

}

#endif /* KATEVI_EMULATED_COMMAND_BAR_H */
