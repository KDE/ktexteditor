/*
    SPDX-FileCopyrightText: 2013-2016 Simon St James <kdedevel@etotheipiplusone.com>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef KATEVI_EMULATED_COMMAND_BAR_H
#define KATEVI_EMULATED_COMMAND_BAR_H

#include "kateviewhelpers.h"
#include <vimode/cmds.h>

namespace KTextEditor
{
class ViewPrivate;
class Command;
}

class QLabel;
class QLayout;

namespace KateVi
{
class MatchHighlighter;
class InteractiveSedReplaceMode;
class SearchMode;
class CommandMode;
class ActiveMode;
class Completer;

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
    explicit EmulatedCommandBar(KateViInputMode *viInputMode, InputModeManager *viInputModeManager, QWidget *parent = nullptr);
    ~EmulatedCommandBar() override;
    void init(Mode mode, const QString &initialText = QString());
    bool isActive() const;
    void setCommandResponseMessageTimeout(long commandResponseMessageTimeOutMS);
    bool handleKeyPress(const QKeyEvent *keyEvent);
    bool isSendingSyntheticSearchCompletedKeypress();

    void startInteractiveSearchAndReplace(QSharedPointer<SedReplace::InteractiveSedReplacer> interactiveSedReplace);
    QString executeCommand(const QString &commandToExecute);

    void setViInputModeManager(InputModeManager *viInputModeManager);

private:
    KateViInputMode *m_viInputMode;
    InputModeManager *m_viInputModeManager;
    bool m_isActive = false;
    bool m_wasAborted = true;
    Mode m_mode = NoMode;
    KTextEditor::ViewPrivate *m_view = nullptr;
    QLineEdit *m_edit = nullptr;

    QLabel *m_barTypeIndicator = nullptr;
    KTEXTEDITOR_NO_EXPORT
    void showBarTypeIndicator(Mode mode);

    bool m_suspendEditEventFiltering = false;

    bool m_waitingForRegister = false;
    QLabel *m_waitingForRegisterIndicator;
    bool m_insertedTextShouldBeEscapedForSearchingAsLiteral = false;

    KTEXTEDITOR_NO_EXPORT
    void hideAllWidgetsExcept(QWidget *widgetToKeepVisible);

    friend class ActiveMode;
    QScopedPointer<MatchHighlighter> m_matchHighligher;

    QScopedPointer<Completer> m_completer;

    QScopedPointer<InteractiveSedReplaceMode> m_interactiveSedReplaceMode;
    QScopedPointer<SearchMode> m_searchMode;
    QScopedPointer<CommandMode> m_commandMode;

    KTEXTEDITOR_NO_EXPORT
    void switchToMode(ActiveMode *newMode);
    ActiveMode *m_currentMode = nullptr;

    KTEXTEDITOR_NO_EXPORT
    bool barHandledKeypress(const QKeyEvent *keyEvent);
    KTEXTEDITOR_NO_EXPORT
    void insertRegisterContents(const QKeyEvent *keyEvent);
    bool eventFilter(QObject *object, QEvent *event) override;
    KTEXTEDITOR_NO_EXPORT
    void deleteSpacesToLeftOfCursor();
    KTEXTEDITOR_NO_EXPORT
    void deleteWordCharsToLeftOfCursor();
    KTEXTEDITOR_NO_EXPORT
    bool deleteNonWordCharsToLeftOfCursor();

    void closed() override;
    KTEXTEDITOR_NO_EXPORT
    void closeWithStatusMessage(const QString &exitStatusMessage);
    QTimer *m_exitStatusMessageDisplayHideTimer;
    QLabel *m_exitStatusMessageDisplay;
    long m_exitStatusMessageHideTimeOutMS = 4000;

    KTEXTEDITOR_NO_EXPORT
    void createAndAddBarTypeIndicator(QLayout *layout);
    KTEXTEDITOR_NO_EXPORT
    void createAndAddEditWidget(QLayout *layout);
    KTEXTEDITOR_NO_EXPORT
    void createAndAddExitStatusMessageDisplay(QLayout *layout);
    KTEXTEDITOR_NO_EXPORT
    void createAndInitExitStatusMessageDisplayTimer();
    KTEXTEDITOR_NO_EXPORT
    void createAndAddWaitingForRegisterIndicator(QLayout *layout);

private Q_SLOTS:
    void editTextChanged(const QString &newText);
    void startHideExitStatusMessageTimer();
};

}

#endif /* KATEVI_EMULATED_COMMAND_BAR_H */
