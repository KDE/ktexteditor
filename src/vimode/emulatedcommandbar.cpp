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

#include <vimode/emulatedcommandbar.h>

#include "kateconfig.h"
#include "katedocument.h"
#include "kateglobal.h"
#include "commandrangeexpressionparser.h"
#include "kateview.h"
#include "globalstate.h"
#include <vimode/keyparser.h>
#include <vimode/inputmodemanager.h>
#include <vimode/modes/normalvimode.h>

#include <vimode/cmds.h>
#include <vimode/modes/visualvimode.h>
#include <vimode/appcommands.h>
#include "history.h"

#include "katecmds.h"
#include "katescriptmanager.h"
#include "registers.h"
#include "searcher.h"

#include <KColorScheme>
#include <KLocalizedString>

#include <QLineEdit>
#include <QVBoxLayout>
#include <QLabel>
#include <QCompleter>
#include <QApplication>
#include <QAbstractItemView>
#include <QWhatsThis>
#include <QStringListModel>

#include <algorithm>

using namespace KateVi;

namespace
{
bool caseInsensitiveLessThan(const QString &s1, const QString &s2)
{
    return s1.toLower() < s2.toLower();
}

bool isCharEscaped(const QString &string, int charPos)
{
    if (charPos == 0) {
        return false;
    }
    int numContiguousBackslashesToLeft = 0;
    charPos--;
    while (charPos >= 0 && string[charPos] == QLatin1Char('\\')) {
        numContiguousBackslashesToLeft++;
        charPos--;
    }
    return ((numContiguousBackslashesToLeft % 2) == 1);
}

QString toggledEscaped(const QString &originalString, QChar escapeChar)
{
    int searchFrom = 0;
    QString toggledEscapedString = originalString;
    do {
        const int indexOfEscapeChar = toggledEscapedString.indexOf(escapeChar, searchFrom);
        if (indexOfEscapeChar == -1) {
            break;
        }
        if (!isCharEscaped(toggledEscapedString, indexOfEscapeChar)) {
            // Escape.
            toggledEscapedString.replace(indexOfEscapeChar, 1, QLatin1String("\\") + escapeChar);
            searchFrom = indexOfEscapeChar + 2;
        } else {
            // Unescape.
            toggledEscapedString.remove(indexOfEscapeChar - 1, 1);
            searchFrom = indexOfEscapeChar;
        }
    } while (true);

    return toggledEscapedString;
}

QString ensuredCharEscaped(const QString &originalString, QChar charToEscape)
{
    QString escapedString = originalString;
    for (int i = 0; i < escapedString.length(); i++) {
        if (escapedString[i] == charToEscape && !isCharEscaped(escapedString, i)) {
            escapedString.replace(i, 1, QLatin1String("\\") + charToEscape);
        }
    }
    return escapedString;
}

QString vimRegexToQtRegexPattern(const QString &vimRegexPattern)
{
    QString qtRegexPattern = vimRegexPattern;
    qtRegexPattern = toggledEscaped(qtRegexPattern, QLatin1Char('('));
    qtRegexPattern = toggledEscaped(qtRegexPattern, QLatin1Char(')'));
    qtRegexPattern = toggledEscaped(qtRegexPattern, QLatin1Char('+'));
    qtRegexPattern = toggledEscaped(qtRegexPattern, QLatin1Char('|'));
    qtRegexPattern = ensuredCharEscaped(qtRegexPattern, QLatin1Char('?'));
    {
        // All curly brackets, except the closing curly bracket of a matching pair where the opening bracket is escaped,
        // must have their escaping toggled.
        bool lookingForMatchingCloseBracket = false;
        QList<int> matchingClosedCurlyBracketPositions;
        for (int i = 0; i < qtRegexPattern.length(); i++) {
            if (qtRegexPattern[i] == QLatin1Char('{') && isCharEscaped(qtRegexPattern, i)) {
                lookingForMatchingCloseBracket = true;
            }
            if (qtRegexPattern[i] == QLatin1Char('}') && lookingForMatchingCloseBracket && qtRegexPattern[i - 1] != QLatin1Char('\\')) {
                matchingClosedCurlyBracketPositions.append(i);
            }
        }
        if (matchingClosedCurlyBracketPositions.isEmpty()) {
            // Escape all {'s and }'s - there are no matching pairs.
            qtRegexPattern = toggledEscaped(qtRegexPattern, QLatin1Char('{'));
            qtRegexPattern = toggledEscaped(qtRegexPattern, QLatin1Char('}'));
        } else {
            // Ensure that every chunk of qtRegexPattern that does *not* contain a curly closing bracket
            // that is matched have their { and } escaping toggled.
            QString qtRegexPatternNonMatchingCurliesToggled;
            int previousNonMatchingClosedCurlyPos = 0; // i.e. the position of the last character which is either
            // not a curly closing bracket, or is a curly closing bracket
            // that is not matched.
            foreach (int matchingClosedCurlyPos, matchingClosedCurlyBracketPositions) {
                QString chunkExcludingMatchingCurlyClosed = qtRegexPattern.mid(previousNonMatchingClosedCurlyPos, matchingClosedCurlyPos - previousNonMatchingClosedCurlyPos);
                chunkExcludingMatchingCurlyClosed = toggledEscaped(chunkExcludingMatchingCurlyClosed, QLatin1Char('{'));
                chunkExcludingMatchingCurlyClosed = toggledEscaped(chunkExcludingMatchingCurlyClosed, QLatin1Char('}'));
                qtRegexPatternNonMatchingCurliesToggled += chunkExcludingMatchingCurlyClosed +
                        qtRegexPattern[matchingClosedCurlyPos];
                previousNonMatchingClosedCurlyPos = matchingClosedCurlyPos + 1;
            }
            QString chunkAfterLastMatchingClosedCurly = qtRegexPattern.mid(matchingClosedCurlyBracketPositions.last() + 1);
            chunkAfterLastMatchingClosedCurly = toggledEscaped(chunkAfterLastMatchingClosedCurly, QLatin1Char('{'));
            chunkAfterLastMatchingClosedCurly = toggledEscaped(chunkAfterLastMatchingClosedCurly, QLatin1Char('}'));
            qtRegexPatternNonMatchingCurliesToggled += chunkAfterLastMatchingClosedCurly;

            qtRegexPattern = qtRegexPatternNonMatchingCurliesToggled;
        }

    }

    // All square brackets, *except* for those that are a) unescaped; and b) form a matching pair, must be
    // escaped.
    bool lookingForMatchingCloseBracket = false;
    int openingBracketPos = -1;
    QList<int> matchingSquareBracketPositions;
    for (int i = 0; i < qtRegexPattern.length(); i++) {
        if (qtRegexPattern[i] == QLatin1Char('[') && !isCharEscaped(qtRegexPattern, i) && !lookingForMatchingCloseBracket) {
            lookingForMatchingCloseBracket = true;
            openingBracketPos = i;
        }
        if (qtRegexPattern[i] == QLatin1Char(']') && lookingForMatchingCloseBracket && !isCharEscaped(qtRegexPattern, i)) {
            lookingForMatchingCloseBracket = false;
            matchingSquareBracketPositions.append(openingBracketPos);
            matchingSquareBracketPositions.append(i);
        }
    }

    if (matchingSquareBracketPositions.isEmpty()) {
        // Escape all ['s and ]'s - there are no matching pairs.
        qtRegexPattern = ensuredCharEscaped(qtRegexPattern, QLatin1Char('['));
        qtRegexPattern = ensuredCharEscaped(qtRegexPattern, QLatin1Char(']'));
    } else {
        // Ensure that every chunk of qtRegexPattern that does *not* contain one of the matching pairs of
        // square brackets have their square brackets escaped.
        QString qtRegexPatternNonMatchingSquaresMadeLiteral;
        int previousNonMatchingSquareBracketPos = 0; // i.e. the position of the last character which is
        // either not a square bracket, or is a square bracket but
        // which is not matched.
        foreach (int matchingSquareBracketPos, matchingSquareBracketPositions) {
            QString chunkExcludingMatchingSquareBrackets = qtRegexPattern.mid(previousNonMatchingSquareBracketPos, matchingSquareBracketPos - previousNonMatchingSquareBracketPos);
            chunkExcludingMatchingSquareBrackets = ensuredCharEscaped(chunkExcludingMatchingSquareBrackets, QLatin1Char('['));
            chunkExcludingMatchingSquareBrackets = ensuredCharEscaped(chunkExcludingMatchingSquareBrackets, QLatin1Char(']'));
            qtRegexPatternNonMatchingSquaresMadeLiteral += chunkExcludingMatchingSquareBrackets + qtRegexPattern[matchingSquareBracketPos];
            previousNonMatchingSquareBracketPos = matchingSquareBracketPos + 1;
        }
        QString chunkAfterLastMatchingSquareBracket = qtRegexPattern.mid(matchingSquareBracketPositions.last() + 1);
        chunkAfterLastMatchingSquareBracket = ensuredCharEscaped(chunkAfterLastMatchingSquareBracket, QLatin1Char('['));
        chunkAfterLastMatchingSquareBracket = ensuredCharEscaped(chunkAfterLastMatchingSquareBracket, QLatin1Char(']'));
        qtRegexPatternNonMatchingSquaresMadeLiteral += chunkAfterLastMatchingSquareBracket;

        qtRegexPattern = qtRegexPatternNonMatchingSquaresMadeLiteral;
    }

    qtRegexPattern = qtRegexPattern.replace(QLatin1String("\\>"), QLatin1String("\\b"));
    qtRegexPattern = qtRegexPattern.replace(QLatin1String("\\<"), QLatin1String("\\b"));

    return qtRegexPattern;
}

/**
 * @return \a originalRegex but escaped in such a way that a Qt regex search for
 * the resulting string will match the string \a originalRegex.
 */
QString escapedForSearchingAsLiteral(const QString &originalQtRegex)
{
    QString escapedForSearchingAsLiteral = originalQtRegex;
    escapedForSearchingAsLiteral.replace(QLatin1Char('\\'), QLatin1String("\\\\"));
    escapedForSearchingAsLiteral.replace(QLatin1Char('$'), QLatin1String("\\$"));
    escapedForSearchingAsLiteral.replace(QLatin1Char('^'), QLatin1String("\\^"));
    escapedForSearchingAsLiteral.replace(QLatin1Char('.'), QLatin1String("\\."));
    escapedForSearchingAsLiteral.replace(QLatin1Char('*'), QLatin1String("\\*"));
    escapedForSearchingAsLiteral.replace(QLatin1Char('/'), QLatin1String("\\/"));
    escapedForSearchingAsLiteral.replace(QLatin1Char('['), QLatin1String("\\["));
    escapedForSearchingAsLiteral.replace(QLatin1Char(']'), QLatin1String("\\]"));
    escapedForSearchingAsLiteral.replace(QLatin1Char('\n'), QLatin1String("\\n"));
    return escapedForSearchingAsLiteral;
}

QStringList reversed(const QStringList &originalList)
{
    QStringList reversedList = originalList;
    std::reverse(reversedList.begin(), reversedList.end());
    return reversedList;
}

QString withCaseSensitivityMarkersStripped(const QString &originalSearchTerm)
{
    // Only \C is handled, for now - I'll implement \c if someone asks for it.
    int pos = 0;
    QString caseSensitivityMarkersStripped = originalSearchTerm;
    while (pos < caseSensitivityMarkersStripped.length()) {
        if (caseSensitivityMarkersStripped.at(pos) == QLatin1Char('C') && isCharEscaped(caseSensitivityMarkersStripped, pos)) {
            caseSensitivityMarkersStripped.replace(pos - 1, 2, QString());
            pos--;
        }
        pos++;
    }
    return caseSensitivityMarkersStripped;
}

int findPosOfSearchConfigMarker(const QString &searchText, const bool isSearchBackwards)
{
    const QChar searchConfigMarkerChar = (isSearchBackwards ? QLatin1Char('?') : QLatin1Char('/'));
    for (int pos = 0; pos < searchText.length(); pos++) {
        if (searchText.at(pos) == searchConfigMarkerChar) {
            if (!isCharEscaped(searchText, pos)) {
                return pos;
            }
        }
    }
    return -1;
}

bool isRepeatLastSearch(const QString &searchText, const bool isSearchBackwards)
{
    const int posOfSearchConfigMarker = findPosOfSearchConfigMarker(searchText, isSearchBackwards);
    if (posOfSearchConfigMarker != -1) {
        if (searchText.leftRef(posOfSearchConfigMarker).isEmpty()) {
            return true;
        }
    }
    return false;
}

bool shouldPlaceCursorAtEndOfMatch(const QString &searchText, const bool isSearchBackwards)
{
    const int posOfSearchConfigMarker = findPosOfSearchConfigMarker(searchText, isSearchBackwards);
    if (posOfSearchConfigMarker != -1) {
        if (searchText.length() > posOfSearchConfigMarker + 1 && searchText.at(posOfSearchConfigMarker + 1) == QLatin1Char('e')) {
            return true;
        }
    }
    return false;
}

QString withSearchConfigRemoved(const QString &originalSearchText, const bool isSearchBackwards)
{
    const int posOfSearchConfigMarker = findPosOfSearchConfigMarker(originalSearchText, isSearchBackwards);
    if (posOfSearchConfigMarker == -1) {
        return originalSearchText;
    } else {
        return originalSearchText.left(posOfSearchConfigMarker);
    }
}
}

EmulatedCommandBar::EmulatedCommandBar(InputModeManager *viInputModeManager, QWidget *parent)
    : KateViewBarWidget(false, parent)
    , m_viInputModeManager(viInputModeManager)
    , m_view(viInputModeManager->view())
{
    QHBoxLayout *layout = new QHBoxLayout();
    layout->setMargin(0);
    centralWidget()->setLayout(layout);
    m_barTypeIndicator = new QLabel(this);
    m_barTypeIndicator->setObjectName(QStringLiteral("bartypeindicator"));
    layout->addWidget(m_barTypeIndicator);

    m_edit = new QLineEdit(this);
    m_edit->setObjectName(QStringLiteral("commandtext"));
    layout->addWidget(m_edit);

    m_exitStatusMessageDisplay = new QLabel(this);
    m_exitStatusMessageDisplay->setObjectName(QStringLiteral("commandresponsemessage"));
    m_exitStatusMessageDisplay->setAlignment(Qt::AlignLeft);
    layout->addWidget(m_exitStatusMessageDisplay);

    m_waitingForRegisterIndicator = new QLabel(this);
    m_waitingForRegisterIndicator->setObjectName(QStringLiteral("waitingforregisterindicator"));
    m_waitingForRegisterIndicator->setVisible(false);
    m_waitingForRegisterIndicator->setText(QStringLiteral("\""));
    layout->addWidget(m_waitingForRegisterIndicator);

    m_interactiveSedReplaceMode.reset(new InteractiveSedReplaceMode(this));
    layout->addWidget(m_interactiveSedReplaceMode->label());

    updateMatchHighlightAttrib();
    m_highlightedMatch = m_view->doc()->newMovingRange(KTextEditor::Range::invalid(), Kate::TextRange::DoNotExpand);
    m_highlightedMatch->setView(m_view); // Show only in this view.
    m_highlightedMatch->setAttributeOnlyForViews(true);
    // Use z depth defined in moving ranges interface.
    m_highlightedMatch->setZDepth(-10000.0);
    m_highlightedMatch->setAttribute(m_highlightMatchAttribute);
    connect(m_view, SIGNAL(configChanged()),
            this, SLOT(updateMatchHighlightAttrib()));

    m_edit->installEventFilter(this);
    connect(m_edit, SIGNAL(textChanged(QString)), this, SLOT(editTextChanged(QString)));

    m_completer = new QCompleter(QStringList(), m_edit);
    // Can't find a way to stop the QCompleter from auto-completing when attached to a QLineEdit,
    // so don't actually set it as the QLineEdit's completer.
    m_completer->setWidget(m_edit);
    m_completer->setObjectName(QStringLiteral("completer"));
    m_completionModel = new QStringListModel(this);
    m_completer->setModel(m_completionModel);
    m_completer->setCaseSensitivity(Qt::CaseInsensitive);
    m_completer->popup()->installEventFilter(this);

    m_exitStatusMessageDisplayHideTimer = new QTimer(this);
    m_exitStatusMessageDisplayHideTimer->setSingleShot(true);
    connect(m_exitStatusMessageDisplayHideTimer, SIGNAL(timeout()),
            this, SIGNAL(hideMe()));
    // Make sure the timer is stopped when the user switches views. If not, focus will be given to the
    // wrong view when KateViewBar::hideCurrentBarWidget() is called as a result of m_commandResponseMessageDisplayHide
    // timing out.
    connect(m_view, SIGNAL(focusOut(KTextEditor::View*)), m_exitStatusMessageDisplayHideTimer, SLOT(stop()));
    // We can restart the timer once the view has focus again, though.
    connect(m_view, SIGNAL(focusIn(KTextEditor::View*)), this, SLOT(startHideExitStatusMessageTimer()));

    QList<KTextEditor::Command *> cmds;

    cmds.push_back(KateCommands::CoreCommands::self());
    cmds.push_back(Commands::self());
    cmds.push_back(AppCommands::self());
    cmds.push_back(SedReplace::self());
    cmds.push_back(BufferCommands::self());

    Q_FOREACH (KTextEditor::Command *cmd, KateScriptManager::self()->commandLineScripts()) {
        cmds.push_back(cmd);
    }

    Q_FOREACH (KTextEditor::Command *cmd, cmds) {
        QStringList l = cmd->cmds();

        for (int z = 0; z < l.count(); z++) {
            m_cmdDict.insert(l[z], cmd);
        }

        m_cmdCompletion.insertItems(l);
    }
}

EmulatedCommandBar::~EmulatedCommandBar()
{
    delete m_highlightedMatch;
}

void EmulatedCommandBar::init(EmulatedCommandBar::Mode mode, const QString &initialText)
{
    m_currentCompletionType = None;
    m_mode = mode;
    m_isActive = true;
    m_wasAborted = true;

    showBarTypeIndicator(mode);

    setBarBackground(Normal);

    m_startingCursorPos = m_view->cursorPosition();

    m_edit->setFocus();
    m_edit->setText(initialText);
    m_edit->show();

    m_exitStatusMessageDisplay->hide();
    m_exitStatusMessageDisplayHideTimer->stop();

    // A change in focus will have occurred: make sure we process it now, instead of having it
    // occur later and stop() m_commandResponseMessageDisplayHide.
    // This is generally only a problem when feeding a sequence of keys without human intervention,
    // as when we execute a mapping, macro, or test case.
    while (QApplication::hasPendingEvents()) {
        QApplication::processEvents();
    }
}

bool EmulatedCommandBar::isActive()
{
    return m_isActive;
}

void EmulatedCommandBar::setCommandResponseMessageTimeout(long int commandResponseMessageTimeOutMS)
{
    m_exitStatusMessageHideTimeOutMS = commandResponseMessageTimeOutMS;
}

void EmulatedCommandBar::closed()
{
    // Close can be called multiple times between init()'s, so only reset the cursor once!
    if (m_startingCursorPos.isValid()) {
        if (m_wasAborted) {
            moveCursorTo(m_startingCursorPos);
        }
    }
    m_startingCursorPos = KTextEditor::Cursor::invalid();
    updateMatchHighlight(KTextEditor::Range::invalid());
    m_completer->popup()->hide();
    m_isActive = false;
    m_interactiveSedReplaceMode->deactivate();

    if (m_mode == SearchForward || m_mode == SearchBackward) {
        // Send a synthetic keypress through the system that signals whether the search was aborted or
        // not.  If not, the keypress will "complete" the search motion, thus triggering it.
        // We send to KateViewInternal as it updates the status bar and removes the "?".
        const Qt::Key syntheticSearchCompletedKey = (m_wasAborted ? static_cast<Qt::Key>(0) : Qt::Key_Enter);
        QKeyEvent syntheticSearchCompletedKeyPress(QEvent::KeyPress, syntheticSearchCompletedKey, Qt::NoModifier);
        m_isSendingSyntheticSearchCompletedKeypress = true;
        QApplication::sendEvent(m_view->focusProxy(), &syntheticSearchCompletedKeyPress);
        m_isSendingSyntheticSearchCompletedKeypress = false;
        if (!m_wasAborted) {
            // Search was actually executed, so store it as the last search.
            m_viInputModeManager->searcher()->setLastSearchParams(m_currentSearchParams);
        }
        // Append the raw text of the search to the search history (i.e. without conversion
        // from Vim-style regex; without case-sensitivity markers stripped; etc.
        // Vim does this even if the search was aborted, so we follow suit.
        m_viInputModeManager->globalState()->searchHistory()->append(m_edit->text());
    } else {
        if (m_wasAborted) {
            // Appending the command to the history when it is executed is handled elsewhere; we can't
            // do it inside closed() as we may still be showing the command response display.
            m_viInputModeManager->globalState()->commandHistory()->append(m_edit->text());
            // With Vim, aborting a command returns us to Normal mode, even if we were in Visual Mode.
            // If we switch from Visual to Normal mode, we need to clear the selection.
            m_view->clearSelection();
        }
    }
}

void EmulatedCommandBar::updateMatchHighlightAttrib()
{
    const QColor &matchColour = m_view->renderer()->config()->searchHighlightColor();
    if (!m_highlightMatchAttribute) {
        m_highlightMatchAttribute = new KTextEditor::Attribute;
    }
    m_highlightMatchAttribute->setBackground(matchColour);
    KTextEditor::Attribute::Ptr mouseInAttribute(new KTextEditor::Attribute());
    m_highlightMatchAttribute->setDynamicAttribute(KTextEditor::Attribute::ActivateMouseIn, mouseInAttribute);
    m_highlightMatchAttribute->dynamicAttribute(KTextEditor::Attribute::ActivateMouseIn)->setBackground(matchColour);
}

void EmulatedCommandBar::updateMatchHighlight(const KTextEditor::Range &matchRange)
{
    // Note that if matchRange is invalid, the highlight will not be shown, so we
    // don't need to check for that explicitly.
    m_highlightedMatch->setRange(matchRange);
}

void EmulatedCommandBar::setBarBackground(EmulatedCommandBar::BarBackgroundStatus status)
{
    QPalette barBackground(m_edit->palette());
    switch (status) {
    case MatchFound: {
        KColorScheme::adjustBackground(barBackground, KColorScheme::PositiveBackground);
        break;
    }
    case NoMatchFound: {
        KColorScheme::adjustBackground(barBackground, KColorScheme::NegativeBackground);
        break;
    }
    case Normal: {
        barBackground = QPalette();
        break;
    }
    }
    m_edit->setPalette(barBackground);
}

bool EmulatedCommandBar::eventFilter(QObject *object, QEvent *event)
{
    Q_ASSERT(object == m_edit || object == m_completer->popup());
    if (m_suspendEditEventFiltering) {
        return false;
    }
    Q_UNUSED(object);
    if (event->type() == QEvent::KeyPress) {
        // Re-route this keypress through Vim's central keypress handling area, so that we can use the keypress in e.g.
        // mappings and macros.
        return m_viInputModeManager->handleKeypress(static_cast<QKeyEvent *>(event));
    }
    return false;
}

void EmulatedCommandBar::deleteSpacesToLeftOfCursor()
{
    while (m_edit->cursorPosition() != 0 && m_edit->text().at(m_edit->cursorPosition() - 1) == QLatin1Char(' ')) {
        m_edit->backspace();
    }
}

void EmulatedCommandBar::deleteWordCharsToLeftOfCursor()
{
    while (m_edit->cursorPosition() != 0) {
        const QChar charToTheLeftOfCursor = m_edit->text().at(m_edit->cursorPosition() - 1);
        if (!charToTheLeftOfCursor.isLetterOrNumber() && charToTheLeftOfCursor != QLatin1Char('_')) {
            break;
        }

        m_edit->backspace();
    }
}

bool EmulatedCommandBar::deleteNonWordCharsToLeftOfCursor()
{
    bool deletionsMade = false;
    while (m_edit->cursorPosition() != 0) {
        const QChar charToTheLeftOfCursor = m_edit->text().at(m_edit->cursorPosition() - 1);
        if (charToTheLeftOfCursor.isLetterOrNumber() || charToTheLeftOfCursor == QLatin1Char('_') || charToTheLeftOfCursor == QLatin1Char(' ')) {
            break;
        }

        m_edit->backspace();
        deletionsMade = true;
    }
    return deletionsMade;
}

QString EmulatedCommandBar::wordBeforeCursor()
{
    int wordBeforeCursorBegin = m_edit->cursorPosition() - 1;
    while (wordBeforeCursorBegin >= 0 && (m_edit->text()[wordBeforeCursorBegin].isLetterOrNumber() || m_edit->text()[wordBeforeCursorBegin] == QLatin1Char('_'))) {
        wordBeforeCursorBegin--;
    }
    wordBeforeCursorBegin++;
    return m_edit->text().mid(wordBeforeCursorBegin, m_edit->cursorPosition() - wordBeforeCursorBegin);
}

QString EmulatedCommandBar::commandBeforeCursor()
{
    const QString textWithoutRangeExpression = withoutRangeExpression();
    const int cursorPositionWithoutRangeExpression = m_edit->cursorPosition() - rangeExpression().length();
    int commandBeforeCursorBegin = cursorPositionWithoutRangeExpression - 1;
    while (commandBeforeCursorBegin >= 0 && (textWithoutRangeExpression[commandBeforeCursorBegin].isLetterOrNumber() || textWithoutRangeExpression[commandBeforeCursorBegin] == QLatin1Char('_') || textWithoutRangeExpression[commandBeforeCursorBegin] == QLatin1Char('-'))) {
        commandBeforeCursorBegin--;
    }
    commandBeforeCursorBegin++;
    return textWithoutRangeExpression.mid(commandBeforeCursorBegin, cursorPositionWithoutRangeExpression - commandBeforeCursorBegin);

}

void EmulatedCommandBar::replaceWordBeforeCursorWith(const QString &newWord)
{
    const int wordBeforeCursorStart = m_edit->cursorPosition() - wordBeforeCursor().length();
    const QString newText = m_edit->text().left(m_edit->cursorPosition() - wordBeforeCursor().length()) +
                            newWord +
                            m_edit->text().mid(m_edit->cursorPosition());
    m_edit->setText(newText);
    m_edit->setCursorPosition(wordBeforeCursorStart + newWord.length());
}

void EmulatedCommandBar::replaceCommandBeforeCursorWith(const QString &newCommand)
{
    const QString newText = m_edit->text().left(m_edit->cursorPosition() - commandBeforeCursor().length()) +
                            newCommand +
                            m_edit->text().mid(m_edit->cursorPosition());
    m_edit->setText(newText);
}

void EmulatedCommandBar::activateSearchHistoryCompletion()
{
    m_currentCompletionType = SearchHistory;
    m_completionModel->setStringList(reversed(m_viInputModeManager->globalState()->searchHistory()->items()));
    updateCompletionPrefix();
    m_completer->complete();
}

void EmulatedCommandBar::activateWordFromDocumentCompletion()
{
    m_currentCompletionType = WordFromDocument;
    QRegExp wordRegEx(QLatin1String("\\w{1,}"));
    QStringList foundWords;
    // Narrow the range of lines we search around the cursor so that we don't die on huge files.
    const int startLine = qMax(0, m_view->cursorPosition().line() - 4096);
    const int endLine = qMin(m_view->document()->lines(), m_view->cursorPosition().line() + 4096);
    for (int lineNum = startLine; lineNum < endLine; lineNum++) {
        const QString line = m_view->document()->line(lineNum);
        int wordSearchBeginPos = 0;
        while (wordRegEx.indexIn(line, wordSearchBeginPos) != -1) {
            const QString foundWord = wordRegEx.cap(0);
            foundWords << foundWord;
            wordSearchBeginPos = wordRegEx.indexIn(line, wordSearchBeginPos) + wordRegEx.matchedLength();
        }
    }
    foundWords = QSet<QString>::fromList(foundWords).toList();
    qSort(foundWords.begin(), foundWords.end(), caseInsensitiveLessThan);
    m_completionModel->setStringList(foundWords);
    updateCompletionPrefix();
    m_completer->complete();
}

void EmulatedCommandBar::activateCommandCompletion()
{
    m_completionModel->setStringList(m_cmdCompletion.items());
    m_currentCompletionType = Commands;
}

void EmulatedCommandBar::activateCommandHistoryCompletion()
{
    m_currentCompletionType = CommandHistory;
    m_completionModel->setStringList(reversed(m_viInputModeManager->globalState()->commandHistory()->items()));
    updateCompletionPrefix();
    m_completer->complete();
}

EmulatedCommandBar::CompletionStartParams EmulatedCommandBar::activateSedFindHistoryCompletion()
{
    CompletionStartParams completionStartParams;
    completionStartParams.shouldStart = false;
    if (!m_viInputModeManager->globalState()->searchHistory()->isEmpty()) {
        completionStartParams.completions = reversed(m_viInputModeManager->globalState()->searchHistory()->items());
        completionStartParams.shouldStart = true;
        ParsedSedExpression parsedSedExpression = parseAsSedExpression();
        completionStartParams.wordStartPos = parsedSedExpression.findBeginPos;
        m_currentCompletionType = SedFindHistory;
    }
    return completionStartParams;
}

void EmulatedCommandBar::activateSedReplaceHistoryCompletion()
{
    if (!m_viInputModeManager->globalState()->replaceHistory()->isEmpty()) {
        m_currentCompletionType = SedReplaceHistory;
        m_completionModel->setStringList(reversed(m_viInputModeManager->globalState()->replaceHistory()->items()));
        m_completer->setCompletionPrefix(sedReplaceTerm());
        m_completer->complete();
    }
}

void EmulatedCommandBar::deactivateCompletion()
{
    m_completer->popup()->hide();
    m_currentCompletionType = None;
}

void EmulatedCommandBar::abortCompletionAndResetToPreCompletion()
{
    deactivateCompletion();
    m_isNextTextChangeDueToCompletionChange = true;
    m_edit->setText(m_textToRevertToIfCompletionAborted);
    m_edit->setCursorPosition(m_cursorPosToRevertToIfCompletionAborted);
    m_isNextTextChangeDueToCompletionChange = false;
}

void EmulatedCommandBar::updateCompletionPrefix()
{
    // TODO - switch on type is not very OO - consider making a polymorphic "completion" class.
    if (m_currentCompletionType == WordFromDocument) {
        m_completer->setCompletionPrefix(wordBeforeCursor());
    } else if (m_currentCompletionType == SearchHistory) {
        m_completer->setCompletionPrefix(m_edit->text());
    } else if (m_currentCompletionType == CommandHistory) {
        m_completer->setCompletionPrefix(m_edit->text());
    } else if (m_currentCompletionType == Commands) {
        m_completer->setCompletionPrefix(commandBeforeCursor());
    } else {
        Q_ASSERT(false && "Unhandled completion type");
    }
    // Seem to need a call to complete() else the size of the popup box is not altered appropriately.
    m_completer->complete();
}

void EmulatedCommandBar::currentCompletionChanged()
{
    // TODO - switch on type is not very OO - consider making a polymorphic "completion" class.
    const QString newCompletion = m_completer->currentCompletion();
    if (newCompletion.isEmpty()) {
        return;
    }
    m_isNextTextChangeDueToCompletionChange = true;
    if (m_currentCompletionType == WordFromDocument) {
        replaceWordBeforeCursorWith(newCompletion);
    } else if (m_currentCompletionType == SearchHistory) {
        m_edit->setText(newCompletion);
    } else if (m_currentCompletionType == CommandHistory) {
        m_edit->setText(newCompletion);
    } else if (m_currentCompletionType == Commands) {
        const int newCursorPosition = m_edit->cursorPosition() + (newCompletion.length() - commandBeforeCursor().length());
        replaceCommandBeforeCursorWith(newCompletion);
        m_edit->setCursorPosition(newCursorPosition);
    } else if (m_currentCompletionType == SedFindHistory) {
        m_edit->setText(withSedFindTermReplacedWith(withCaseSensitivityMarkersStripped(withSedDelimiterEscaped(newCompletion))));
        ParsedSedExpression parsedSedExpression = parseAsSedExpression();
        m_edit->setCursorPosition(parsedSedExpression.findEndPos + 1);
    } else if (m_currentCompletionType == SedReplaceHistory) {
        m_edit->setText(withSedReplaceTermReplacedWith(withSedDelimiterEscaped(newCompletion)));
        ParsedSedExpression parsedSedExpression = parseAsSedExpression();
        m_edit->setCursorPosition(parsedSedExpression.replaceEndPos + 1);
    } else {
        Q_ASSERT(false && "Something went wrong, here - completion with unrecognised completion type");
    }
    m_isNextTextChangeDueToCompletionChange = false;
}

void EmulatedCommandBar::setCompletionIndex(int index)
{
    const QModelIndex modelIndex = m_completer->popup()->model()->index(index, 0);
    // Need to set both of these, for some reason.
    m_completer->popup()->setCurrentIndex(modelIndex);
    m_completer->setCurrentRow(index);

    m_completer->popup()->scrollTo(modelIndex);

    currentCompletionChanged();
}

EmulatedCommandBar::ParsedSedExpression EmulatedCommandBar::parseAsSedExpression()
{
    const QString commandWithoutRangeExpression = withoutRangeExpression();
    ParsedSedExpression parsedSedExpression;
    QString delimiter;
    parsedSedExpression.parsedSuccessfully = SedReplace::parse(commandWithoutRangeExpression, delimiter, parsedSedExpression.findBeginPos, parsedSedExpression.findEndPos, parsedSedExpression.replaceBeginPos, parsedSedExpression.replaceEndPos);
    if (parsedSedExpression.parsedSuccessfully) {
        parsedSedExpression.delimiter = delimiter.at(0);
        if (parsedSedExpression.replaceBeginPos == -1) {
            if (parsedSedExpression.findBeginPos != -1) {
                // The replace term was empty, and a quirk of the regex used is that replaceBeginPos will be -1.
                // It's actually the position after the first occurrence of the delimiter after the end of the find pos.
                parsedSedExpression.replaceBeginPos = commandWithoutRangeExpression.indexOf(delimiter, parsedSedExpression.findEndPos) + 1;
                parsedSedExpression.replaceEndPos = parsedSedExpression.replaceBeginPos - 1;
            } else {
                // Both find and replace terms are empty; replace term is at the third occurrence of the delimiter.
                parsedSedExpression.replaceBeginPos = 0;
                for (int delimiterCount = 1; delimiterCount <= 3; delimiterCount++) {
                    parsedSedExpression.replaceBeginPos = commandWithoutRangeExpression.indexOf(delimiter, parsedSedExpression.replaceBeginPos + 1);
                }
                parsedSedExpression.replaceEndPos = parsedSedExpression.replaceBeginPos - 1;
            }
        }
        if (parsedSedExpression.findBeginPos == -1) {
            // The find term was empty, and a quirk of the regex used is that findBeginPos will be -1.
            // It's actually the position after the first occurrence of the delimiter.
            parsedSedExpression.findBeginPos = commandWithoutRangeExpression.indexOf(delimiter) + 1;
            parsedSedExpression.findEndPos = parsedSedExpression.findBeginPos - 1;
        }

    }

    if (parsedSedExpression.parsedSuccessfully) {
        parsedSedExpression.findBeginPos += rangeExpression().length();
        parsedSedExpression.findEndPos += rangeExpression().length();
        parsedSedExpression.replaceBeginPos += rangeExpression().length();
        parsedSedExpression.replaceEndPos += rangeExpression().length();
    }
    return parsedSedExpression;
}

QString EmulatedCommandBar::withSedFindTermReplacedWith(const QString &newFindTerm)
{
    const QString command = m_edit->text();
    ParsedSedExpression parsedSedExpression = parseAsSedExpression();
    Q_ASSERT(parsedSedExpression.parsedSuccessfully);
    return command.mid(0, parsedSedExpression.findBeginPos) +
           newFindTerm +
           command.mid(parsedSedExpression.findEndPos + 1);

}

QString EmulatedCommandBar::withSedReplaceTermReplacedWith(const QString &newReplaceTerm)
{
    const QString command = m_edit->text();
    ParsedSedExpression parsedSedExpression = parseAsSedExpression();
    Q_ASSERT(parsedSedExpression.parsedSuccessfully);
    return command.mid(0, parsedSedExpression.replaceBeginPos) +
           newReplaceTerm +
           command.mid(parsedSedExpression.replaceEndPos + 1);
}

QString EmulatedCommandBar::sedFindTerm()
{
    const QString command = m_edit->text();
    ParsedSedExpression parsedSedExpression = parseAsSedExpression();
    Q_ASSERT(parsedSedExpression.parsedSuccessfully);
    return command.mid(parsedSedExpression.findBeginPos, parsedSedExpression.findEndPos - parsedSedExpression.findBeginPos + 1);
}

QString EmulatedCommandBar::sedReplaceTerm()
{
    const QString command = m_edit->text();
    ParsedSedExpression parsedSedExpression = parseAsSedExpression();
    Q_ASSERT(parsedSedExpression.parsedSuccessfully);
    return command.mid(parsedSedExpression.replaceBeginPos, parsedSedExpression.replaceEndPos - parsedSedExpression.replaceBeginPos + 1);
}

QString EmulatedCommandBar::withSedDelimiterEscaped(const QString &text)
{
    ParsedSedExpression parsedSedExpression = parseAsSedExpression();
    QString delimiterEscaped = ensuredCharEscaped(text, parsedSedExpression.delimiter);
    return delimiterEscaped;
}

bool EmulatedCommandBar::isCursorInFindTermOfSed()
{
    ParsedSedExpression parsedSedExpression = parseAsSedExpression();
    return parsedSedExpression.parsedSuccessfully && (m_edit->cursorPosition() >= parsedSedExpression.findBeginPos && m_edit->cursorPosition() <= parsedSedExpression.findEndPos + 1);
}

bool EmulatedCommandBar::isCursorInReplaceTermOfSed()
{
    ParsedSedExpression parsedSedExpression = parseAsSedExpression();
    return parsedSedExpression.parsedSuccessfully && m_edit->cursorPosition() >= parsedSedExpression.replaceBeginPos && m_edit->cursorPosition() <= parsedSedExpression.replaceEndPos + 1;
}

QString EmulatedCommandBar::withoutRangeExpression()
{
    const QString originalCommand = m_edit->text();
    return originalCommand.mid(rangeExpression().length());
}

QString EmulatedCommandBar::rangeExpression()
{
    const QString command = m_edit->text();
    return CommandRangeExpressionParser(m_viInputModeManager).parseRangeString(command);
}

bool EmulatedCommandBar::handleKeyPress(const QKeyEvent *keyEvent)
{
    if (keyEvent->modifiers() == Qt::ControlModifier && (keyEvent->key() == Qt::Key_C || keyEvent->key() == Qt::Key_BracketLeft) && !m_waitingForRegister) {
        if (m_currentCompletionType == None || !m_completer->popup()->isVisible()) {
            emit hideMe();
        } else {
            abortCompletionAndResetToPreCompletion();
        }
        return true;
    }
    if (m_interactiveSedReplaceMode->isActive()) {
        return m_interactiveSedReplaceMode->handleKeyPress(keyEvent);
    }
    if (keyEvent->modifiers() == Qt::ControlModifier && keyEvent->key() == Qt::Key_Space) {
        activateWordFromDocumentCompletion();
        return true;
    }
    if ((keyEvent->modifiers() == Qt::ControlModifier && keyEvent->key() == Qt::Key_P) || keyEvent->key() == Qt::Key_Down) {
        if (!m_completer->popup()->isVisible()) {
            CompletionStartParams completionStartParams;
            completionStartParams.shouldStart = false;
            if (m_mode == Command) {
                if (isCursorInFindTermOfSed()) {
                    completionStartParams = activateSedFindHistoryCompletion();
                } else if (isCursorInReplaceTermOfSed()) {
                    activateSedReplaceHistoryCompletion();
                } else {
                    activateCommandHistoryCompletion();
                }
            } else {
                activateSearchHistoryCompletion();
            }
            if (completionStartParams.shouldStart)
            {
                m_completionModel->setStringList(completionStartParams.completions);
                const QString completionPrefix = m_edit->text().mid(completionStartParams.wordStartPos, m_edit->cursorPosition() - completionStartParams.wordStartPos);
                m_completer->setCompletionPrefix(completionPrefix);
                m_completer->complete();
            }
            if (m_currentCompletionType != None) {
                setCompletionIndex(0);
            }
        } else {
            // Descend to next row, wrapping around if necessary.
            if (m_completer->currentRow() + 1 == m_completer->completionCount()) {
                setCompletionIndex(0);
            } else {
                setCompletionIndex(m_completer->currentRow() + 1);
            }
        }
        return true;
    }
    if ((keyEvent->modifiers() == Qt::ControlModifier && keyEvent->key() == Qt::Key_N) || keyEvent->key() == Qt::Key_Up) {
        if (!m_completer->popup()->isVisible()) {
            if (m_mode == Command) {
                activateCommandHistoryCompletion();
            } else {
                activateSearchHistoryCompletion();
            }
            setCompletionIndex(m_completer->completionCount() - 1);
        } else {
            // Ascend to previous row, wrapping around if necessary.
            if (m_completer->currentRow() == 0) {
                setCompletionIndex(m_completer->completionCount() - 1);
            } else {
                setCompletionIndex(m_completer->currentRow() - 1);
            }
        }
        return true;
    }
    if (m_waitingForRegister) {
        if (keyEvent->key() != Qt::Key_Shift && keyEvent->key() != Qt::Key_Control) {
            const QChar key = KeyParser::self()->KeyEventToQChar(*keyEvent).toLower();

            const int oldCursorPosition = m_edit->cursorPosition();
            QString textToInsert;
            if (keyEvent->modifiers() == Qt::ControlModifier && keyEvent->key() == Qt::Key_W) {
                textToInsert = m_view->doc()->wordAt(m_view->cursorPosition());
            } else {
                textToInsert = m_viInputModeManager->globalState()->registers()->getContent(key);
            }
            if (m_insertedTextShouldBeEscapedForSearchingAsLiteral) {
                textToInsert = escapedForSearchingAsLiteral(textToInsert);
                m_insertedTextShouldBeEscapedForSearchingAsLiteral = false;
            }
            m_edit->setText(m_edit->text().insert(m_edit->cursorPosition(), textToInsert));
            m_edit->setCursorPosition(oldCursorPosition + textToInsert.length());
            m_waitingForRegister = false;
            m_waitingForRegisterIndicator->setVisible(false);
        }
    } else if ((keyEvent->modifiers() == Qt::ControlModifier && keyEvent->key() == Qt::Key_H) || keyEvent->key() == Qt::Key_Backspace) {
        if (m_edit->text().isEmpty()) {
            emit hideMe();
        }
        m_edit->backspace();
        return true;
    } else if (keyEvent->modifiers() == Qt::ControlModifier) {
        if (keyEvent->key() == Qt::Key_B) {
            m_edit->setCursorPosition(0);
            return true;
        } else if (keyEvent->key() == Qt::Key_E) {
            m_edit->setCursorPosition(m_edit->text().length());
            return true;
        } else if (keyEvent->key() == Qt::Key_W) {
            deleteSpacesToLeftOfCursor();
            if (!deleteNonWordCharsToLeftOfCursor()) {
                deleteWordCharsToLeftOfCursor();
            }
            return true;
        } else if (keyEvent->key() == Qt::Key_R || keyEvent->key() == Qt::Key_G) {
            m_waitingForRegister = true;
            m_waitingForRegisterIndicator->setVisible(true);
            if (keyEvent->key() == Qt::Key_G) {
                m_insertedTextShouldBeEscapedForSearchingAsLiteral = true;
            }
            return true;
        } else if (keyEvent->key() == Qt::Key_D || keyEvent->key() == Qt::Key_F) {
            if (m_mode == Command) {
                ParsedSedExpression parsedSedExpression = parseAsSedExpression();
                if (parsedSedExpression.parsedSuccessfully) {
                    const bool clearFindTerm = (keyEvent->key() == Qt::Key_D);
                    if (clearFindTerm) {
                        m_edit->setSelection(parsedSedExpression.findBeginPos, parsedSedExpression.findEndPos - parsedSedExpression.findBeginPos + 1);
                        m_edit->insert(QString());
                    } else {
                        // Clear replace term.
                        m_edit->setSelection(parsedSedExpression.replaceBeginPos, parsedSedExpression.replaceEndPos - parsedSedExpression.replaceBeginPos + 1);
                        m_edit->insert(QString());
                    }
                }
            }
            return true;
        }
        return false;
    } else if (keyEvent->key() == Qt::Key_Enter || keyEvent->key() == Qt::Key_Return) {
        if (m_completer->popup()->isVisible() && m_currentCompletionType == EmulatedCommandBar::WordFromDocument) {
            deactivateCompletion();
        } else {
            m_wasAborted = false;
            deactivateCompletion();
            if (m_mode == Command) {
                QString commandToExecute = m_edit->text();
                ParsedSedExpression parsedSedExpression = parseAsSedExpression();
                if (parsedSedExpression.parsedSuccessfully) {
                    const QString originalFindTerm = sedFindTerm();
                    const QString convertedFindTerm = vimRegexToQtRegexPattern(originalFindTerm);
                    const QString commandWithSedSearchRegexConverted = withSedFindTermReplacedWith(convertedFindTerm);
                    m_viInputModeManager->globalState()->searchHistory()->append(originalFindTerm);
                    const QString replaceTerm = sedReplaceTerm();
                    m_viInputModeManager->globalState()->replaceHistory()->append(replaceTerm);
                    commandToExecute = commandWithSedSearchRegexConverted;
                }

                const QString commandResponseMessage = executeCommand(commandToExecute);
                if (!m_interactiveSedReplaceMode->isActive()) {
                    if (commandResponseMessage.isEmpty()) {
                        emit hideMe();
                    } else {
                        closeWithStatusMessage(commandResponseMessage);
                    }
                }
                m_viInputModeManager->globalState()->commandHistory()->append(m_edit->text());
            } else {
                emit hideMe();
            }
        }
        return true;
    } else {
        m_suspendEditEventFiltering = true;
        // Send the keypress back to the QLineEdit.  Ideally, instead of doing this, we would simply return "false"
        // and let Qt re-dispatch the event itself; however, there is a corner case in that if the selection
        // changes (as a result of e.g. incremental searches during Visual Mode), and the keypress that causes it
        // is not dispatched from within KateViInputModeHandler::handleKeypress(...)
        // (so KateViInputModeManager::isHandlingKeypress() returns false), we lose information about whether we are
        // in Visual Mode, Visual Line Mode, etc.  See VisualViMode::updateSelection( ).
        QKeyEvent keyEventCopy(keyEvent->type(), keyEvent->key(), keyEvent->modifiers(), keyEvent->text(), keyEvent->isAutoRepeat(), keyEvent->count());
        if (!m_interactiveSedReplaceMode->isActive()) {
            qApp->notify(m_edit, &keyEventCopy);
        }
        m_suspendEditEventFiltering = false;
    }
    return true;
}

bool EmulatedCommandBar::isSendingSyntheticSearchCompletedKeypress()
{
    return m_isSendingSyntheticSearchCompletedKeypress;
}

void EmulatedCommandBar::startInteractiveSearchAndReplace(QSharedPointer<SedReplace::InteractiveSedReplacer> interactiveSedReplace)
{
    Q_ASSERT_X(interactiveSedReplace->currentMatch().isValid(), "startInteractiveSearchAndReplace", "KateCommands shouldn't initiate an interactive sed replace with no initial match");
    m_interactiveSedReplaceMode->activate(interactiveSedReplace);
}

void EmulatedCommandBar::showBarTypeIndicator(EmulatedCommandBar::Mode mode)
{
    QChar barTypeIndicator = QChar::Null;
    switch (mode) {
    case SearchForward:
        barTypeIndicator = QLatin1Char('/');
        break;
    case SearchBackward:
        barTypeIndicator = QLatin1Char('?');
        break;
    case Command:
        barTypeIndicator = QLatin1Char(':');
        break;
    default:
        Q_ASSERT(false && "Unknown mode!");
    }
    m_barTypeIndicator->setText(barTypeIndicator);
    m_barTypeIndicator->show();
}

QString EmulatedCommandBar::executeCommand(const QString &commandToExecute)
{
    // silently ignore leading space characters and colon characters (for vi-heads)
    uint n = 0;
    const uint textlen = commandToExecute.length();
    while ((n < textlen) && commandToExecute[n].isSpace()) {
        n++;
    }

    if (n >= textlen) {
        return QString();
    }

    QString commandResponseMessage;
    QString cmd = commandToExecute.mid(n);

    KTextEditor::Range range = CommandRangeExpressionParser(m_viInputModeManager).parseRange(cmd, cmd);

    if (cmd.length() > 0) {
        KTextEditor::Command *p = queryCommand(cmd);

        if (p) {
            KateViCommandInterface *ci = dynamic_cast<KateViCommandInterface*>(p);
            if (ci) {
                ci->setViInputModeManager(m_viInputModeManager);
                ci->setViGlobal(m_viInputModeManager->globalState());
            }

            // the following commands changes the focus themselves, so bar should be hidden before execution.

            // we got a range and a valid command, but the command does not inherit the RangeCommand
            // extension. bail out.
            if (range.isValid() && !p->supportsRange(cmd)) {
                commandResponseMessage = i18n("Error: No range allowed for command \"%1\".",  cmd);
            } else {

                if (p->exec(m_view, cmd, commandResponseMessage, range)) {

                    if (commandResponseMessage.length() > 0) {
                        commandResponseMessage = i18n("Success: ") + commandResponseMessage;
                    }
                } else {
                    if (commandResponseMessage.length() > 0) {
                        if (commandResponseMessage.contains(QLatin1Char('\n'))) {
                            // multiline error, use widget with more space
                            QWhatsThis::showText(mapToGlobal(QPoint(0, 0)), commandResponseMessage);
                        }
                    } else {
                        commandResponseMessage = i18n("Command \"%1\" failed.",  cmd);
                    }
                }
            }
        } else {
            commandResponseMessage = i18n("No such command: \"%1\"",  cmd);
        }
    }

    // the following commands change the focus themselves
    if (!QRegExp(QLatin1String("buffer|b|new|vnew|bp|bprev|bn|bnext|bf|bfirst|bl|blast|edit|e")).exactMatch(cmd.split(QLatin1String(" ")).at(0))) {
        m_view->setFocus();
    }

    m_viInputModeManager->reset();
    return commandResponseMessage;
}

void EmulatedCommandBar::closeWithStatusMessage(const QString &exitStatusMessage)
{
    // Display the message for a while.  Become inactive, so we don't steal keys in the meantime.
    m_isActive = false;
    m_interactiveSedReplaceMode->deactivate();
    m_exitStatusMessageDisplay->show();
    m_exitStatusMessageDisplay->setText(exitStatusMessage);
    hideAllWidgetsExcept(m_exitStatusMessageDisplay);
    m_exitStatusMessageDisplayHideTimer->start(m_exitStatusMessageHideTimeOutMS);
}

void EmulatedCommandBar::moveCursorTo(const KTextEditor::Cursor &cursorPos)
{
    m_view->setCursorPosition(cursorPos);
    if (m_viInputModeManager->getCurrentViMode() == ViMode::VisualMode ||
        m_viInputModeManager->getCurrentViMode() == ViMode::VisualLineMode) {

        m_viInputModeManager->getViVisualMode()->goToPos(cursorPos);
    }
}

void EmulatedCommandBar::editTextChanged(const QString &newText)
{
    Q_ASSERT(!m_interactiveSedReplaceMode->isActive());
    if (!m_isNextTextChangeDueToCompletionChange) {
        m_textToRevertToIfCompletionAborted = newText;
        m_cursorPosToRevertToIfCompletionAborted = m_edit->cursorPosition();
    }
    if (m_mode == SearchForward || m_mode == SearchBackward) {
        QString qtRegexPattern = newText;
        const bool searchBackwards = (m_mode == SearchBackward);
        const bool placeCursorAtEndOfMatch = shouldPlaceCursorAtEndOfMatch(qtRegexPattern, searchBackwards);
        if (isRepeatLastSearch(qtRegexPattern, searchBackwards)) {
            qtRegexPattern = m_viInputModeManager->searcher()->getLastSearchPattern();
        } else {
            qtRegexPattern = withSearchConfigRemoved(qtRegexPattern, searchBackwards);
            qtRegexPattern = vimRegexToQtRegexPattern(qtRegexPattern);
        }

        // Decide case-sensitivity via SmartCase (note: if the expression contains \C, the "case-sensitive" marker, then
        // we will be case-sensitive "by coincidence", as it were.).
        bool caseSensitive = true;
        if (qtRegexPattern.toLower() == qtRegexPattern) {
            caseSensitive = false;
        }

        qtRegexPattern = withCaseSensitivityMarkersStripped(qtRegexPattern);

        m_currentSearchParams.pattern = qtRegexPattern;
        m_currentSearchParams.isCaseSensitive = caseSensitive;
        m_currentSearchParams.isBackwards = searchBackwards;
        m_currentSearchParams.shouldPlaceCursorAtEndOfMatch = placeCursorAtEndOfMatch;

        // The "count" for the current search is not shared between Visual & Normal mode, so we need to pick
        // the right one to handle the counted search.
        int c = m_viInputModeManager->getCurrentViModeHandler()->getCount();
        KTextEditor::Range match = m_viInputModeManager->searcher()->findPattern(m_currentSearchParams, m_startingCursorPos, c, false /* Don't add incremental searches to search history */);

        if (match.isValid()) {
            // The returned range ends one past the last character of the match, so adjust.
            KTextEditor::Cursor realMatchEnd = KTextEditor::Cursor(match.end().line(), match.end().column() - 1);
            if (realMatchEnd.column() == -1) {
                realMatchEnd = KTextEditor::Cursor(realMatchEnd.line() - 1, m_view->doc()->lineLength(realMatchEnd.line() - 1));
            }
            moveCursorTo(placeCursorAtEndOfMatch ? realMatchEnd :  match.start());
            setBarBackground(MatchFound);
        } else {
            moveCursorTo(m_startingCursorPos);
            if (!m_edit->text().isEmpty()) {
                setBarBackground(NoMatchFound);
            } else {
                setBarBackground(Normal);
            }
        }

        updateMatchHighlight(match);
    }

    // Command completion doesn't need to be manually invoked.
    if (m_mode == Command && m_currentCompletionType == None && !withoutRangeExpression().isEmpty()) {
        activateCommandCompletion();
    }

    // Command completion mode should be automatically invoked if we are in Command mode, but
    // only if this is the leading word in the text edit (it gets annoying if completion pops up
    // after ":s/se" etc).
    const bool commandBeforeCursorIsLeading = (m_edit->cursorPosition() - commandBeforeCursor().length() == rangeExpression().length());
    if (m_mode == Command && !commandBeforeCursorIsLeading && m_currentCompletionType == Commands && !m_isNextTextChangeDueToCompletionChange) {
        deactivateCompletion();
    }

    // If we edit the text after having selected a completion, this means we implicitly accept it,
    // and so we should dismiss it.
    if (!m_isNextTextChangeDueToCompletionChange && m_completer->popup()->currentIndex().row() != -1) {
        deactivateCompletion();
    }

    if (m_currentCompletionType != None && !m_isNextTextChangeDueToCompletionChange) {
        updateCompletionPrefix();
    }
}

void EmulatedCommandBar::startHideExitStatusMessageTimer()
{
    if (m_exitStatusMessageDisplay->isVisible() && !m_exitStatusMessageDisplayHideTimer->isActive()) {
        m_exitStatusMessageDisplayHideTimer->start(m_exitStatusMessageHideTimeOutMS);
    }
}

KTextEditor::Command *EmulatedCommandBar::queryCommand(const QString &cmd) const
{
    // a command can be named ".*[\w\-]+" with the constrain that it must
    // contain at least one letter.
    int f = 0;
    bool b = false;

    // special case: '-' and '_' can be part of a command name, but if the
    // command is 's' (substitute), it should be considered the delimiter and
    // should not be counted as part of the command name
    if (cmd.length() >= 2 && cmd.at(0) == QLatin1Char('s') && (cmd.at(1) == QLatin1Char('-') || cmd.at(1) == QLatin1Char('_'))) {
        return m_cmdDict.value(QStringLiteral("s"));
    }

    for (; f < cmd.length(); f++) {
        if (cmd[f].isLetter()) {
            b = true;
        }
        if (b && (! cmd[f].isLetterOrNumber() && cmd[f] != QLatin1Char('-') && cmd[f] != QLatin1Char('_'))) {
            break;
        }
    }
    return m_cmdDict.value(cmd.left(f));
}

void EmulatedCommandBar::setViInputModeManager(InputModeManager *viInputModeManager)
{
    m_viInputModeManager = viInputModeManager;
}

void EmulatedCommandBar::hideAllWidgetsExcept(QWidget* widgetToKeepVisible)
{
    QList<QWidget*> widgets = centralWidget()->findChildren<QWidget*>();
    foreach(QWidget* widget, widgets)
    {
        if (widget != widgetToKeepVisible)
            widget->hide();
    }

}

EmulatedCommandBar::ActiveMode::~ActiveMode()
{

}

void EmulatedCommandBar::ActiveMode::hideAllWidgetsExcept(QWidget* widgetToKeepVisible)
{
    m_emulatedCommandBar->hideAllWidgetsExcept(widgetToKeepVisible);
}

void EmulatedCommandBar::ActiveMode::moveCursorTo(const KTextEditor::Cursor &cursorPos)
{
    m_emulatedCommandBar->moveCursorTo(cursorPos);
}

void EmulatedCommandBar::ActiveMode::updateMatchHighlight(const KTextEditor::Range& matchRange)
{
    m_emulatedCommandBar->updateMatchHighlight(matchRange);
}

void EmulatedCommandBar::ActiveMode::closeWithStatusMessage(const QString& exitStatusMessage)
{
    m_emulatedCommandBar->closeWithStatusMessage(exitStatusMessage);
}

EmulatedCommandBar::InteractiveSedReplaceMode::InteractiveSedReplaceMode(EmulatedCommandBar* emulatedCommandBar)
    : ActiveMode(emulatedCommandBar), m_isActive(false)
{
    m_interactiveSedReplaceLabel = new QLabel();
    m_interactiveSedReplaceLabel->setObjectName(QStringLiteral("interactivesedreplace"));
}

void EmulatedCommandBar::InteractiveSedReplaceMode::activate(QSharedPointer<SedReplace::InteractiveSedReplacer> interactiveSedReplace)
{
    Q_ASSERT_X(interactiveSedReplace->currentMatch().isValid(), "startInteractiveSearchAndReplace", "KateCommands shouldn't initiate an interactive sed replace with no initial match");

    m_isActive = true;
    m_interactiveSedReplacer = interactiveSedReplace;

    hideAllWidgetsExcept(m_interactiveSedReplaceLabel);
    m_interactiveSedReplaceLabel->show();
    updateInteractiveSedReplaceLabelText();

    updateMatchHighlight(interactiveSedReplace->currentMatch());
    moveCursorTo(interactiveSedReplace->currentMatch().start());
}

bool EmulatedCommandBar::InteractiveSedReplaceMode::handleKeyPress(const QKeyEvent* keyEvent)
{
    qDebug() << "InteractiveSedReplaceMode::handleKeypress";
    // TODO - it would be better to use e.g. keyEvent->key() == Qt::Key_Y instead of keyEvent->text() == "y",
    // but this would require some slightly dicey changes to the "feed key press" code in order to make it work
    // with mappings and macros.
    if (keyEvent->text() == QLatin1String("y") || keyEvent->text() == QLatin1String("n")) {
        const KTextEditor::Cursor cursorPosIfFinalMatch = m_interactiveSedReplacer->currentMatch().start();
        if (keyEvent->text() == QLatin1String("y")) {
            m_interactiveSedReplacer->replaceCurrentMatch();
        } else {
            m_interactiveSedReplacer->skipCurrentMatch();
        }
        updateMatchHighlight(m_interactiveSedReplacer->currentMatch());
        updateInteractiveSedReplaceLabelText();
        moveCursorTo(m_interactiveSedReplacer->currentMatch().start());

        if (!m_interactiveSedReplacer->currentMatch().isValid()) {
            moveCursorTo(cursorPosIfFinalMatch);
            finishInteractiveSedReplace();
        }
        return true;
    } else if (keyEvent->text() == QLatin1String("l")) {
        m_interactiveSedReplacer->replaceCurrentMatch();
        finishInteractiveSedReplace();
        return true;
    } else if (keyEvent->text() == QLatin1String("q")) {
        finishInteractiveSedReplace();
        return true;
    } else if (keyEvent->text() == QLatin1String("a")) {
        m_interactiveSedReplacer->replaceAllRemaining();
        finishInteractiveSedReplace();
        return true;
    }
    return false;
}

void EmulatedCommandBar::InteractiveSedReplaceMode::deactivate()
{
    m_isActive = false;
    m_interactiveSedReplaceLabel->hide();
}

QWidget* EmulatedCommandBar::InteractiveSedReplaceMode::label()
{
    return m_interactiveSedReplaceLabel;
}

void EmulatedCommandBar::InteractiveSedReplaceMode::updateInteractiveSedReplaceLabelText()
{
    m_interactiveSedReplaceLabel->setText(m_interactiveSedReplacer->currentMatchReplacementConfirmationMessage() + QLatin1String(" (y/n/a/q/l)"));
}

void EmulatedCommandBar::InteractiveSedReplaceMode::finishInteractiveSedReplace()
{
    deactivate();
    closeWithStatusMessage(m_interactiveSedReplacer->finalStatusReportMessage());
    m_interactiveSedReplacer.clear();
}
