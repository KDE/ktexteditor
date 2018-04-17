/*  This file is part of the KDE libraries and the Kate part.
 *
 *  Copyright (C) 2008 - 2009 Erlend Hamberg <ehamberg@gmail.com>
 *  Copyright (C) 2009 Paul Gideon Dann <pdgiddie@gmail.com>
 *  Copyright (C) 2011 Svyatoslav Kuzmich <svatoslav1@gmail.com>
 *  Copyright (C) 2012 - 2013 Simon St James <kdedevel@etotheipiplusone.com>
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

#ifndef KATEVI_NORMAL_VI_MODE_H
#define KATEVI_NORMAL_VI_MODE_H

#include <vimode/range.h>
#include <vimode/modes/modebase.h>

#include <QVector>
#include <QStack>
#include <QHash>
#include <QRegExp>

#include <ktexteditor/range.h>
#include <ktexteditor_export.h>

class QKeyEvent;
class KateViInputMode;

namespace KateVi
{
class Command;
class Motion;
class KeyParser;
class InputModeManager;

/**
 * Commands for the vi normal mode
 */
class KTEXTEDITOR_EXPORT NormalViMode : public ModeBase
{
    Q_OBJECT

    friend KateViInputMode;

public:
    explicit NormalViMode(InputModeManager *viInputModeManager, KTextEditor::ViewPrivate *view, KateViewInternal *viewInternal);
    ~NormalViMode() Q_DECL_OVERRIDE;

    bool handleKeypress(const QKeyEvent *e) Q_DECL_OVERRIDE;

    bool commandEnterInsertMode();
    bool commandEnterInsertModeAppend();
    bool commandEnterInsertModeAppendEOL();
    bool commandEnterInsertModeBeforeFirstNonBlankInLine();
    bool commandEnterInsertModeLast();

    bool commandEnterVisualMode();
    bool commandEnterVisualLineMode();
    bool commandEnterVisualBlockMode();
    bool commandReselectVisual();
    bool commandToOtherEnd();

    bool commandEnterReplaceMode();

    bool commandDelete();
    bool commandDeleteToEOL();
    bool commandDeleteLine();

    bool commandMakeLowercase();
    bool commandMakeLowercaseLine();
    bool commandMakeUppercase();
    bool commandMakeUppercaseLine();
    bool commandChangeCase();
    bool commandChangeCaseRange();
    bool commandChangeCaseLine();

    bool commandOpenNewLineUnder();
    bool commandOpenNewLineOver();

    bool commandJoinLines();

    bool commandChange();
    bool commandChangeLine();
    bool commandChangeToEOL();
    bool commandSubstituteChar();
    bool commandSubstituteLine();

    bool commandYank();
    bool commandYankLine();
    bool commandYankToEOL();

    bool commandPaste();
    bool commandPasteBefore();

    bool commandgPaste();
    bool commandgPasteBefore();

    bool commandIndentedPaste();
    bool commandIndentedPasteBefore();

    bool commandDeleteChar();
    bool commandDeleteCharBackward();

    bool commandReplaceCharacter();

    bool commandSwitchToCmdLine();
    bool commandSearchBackward();
    bool commandSearchForward();
    bool commandUndo();
    bool commandRedo();

    bool commandSetMark();

    bool commandIndentLine();
    bool commandUnindentLine();
    bool commandIndentLines();
    bool commandUnindentLines();

    bool commandScrollPageDown();
    bool commandScrollPageUp();
    bool commandScrollHalfPageUp();
    bool commandScrollHalfPageDown();

    bool commandCenterView(bool onFirst);
    bool commandCenterViewOnNonBlank();
    bool commandCenterViewOnCursor();
    bool commandTopView(bool onFirst);
    bool commandTopViewOnNonBlank();
    bool commandTopViewOnCursor();
    bool commandBottomView(bool onFirst);
    bool commandBottomViewOnNonBlank();
    bool commandBottomViewOnCursor();

    bool commandAbort();

    bool commandPrintCharacterCode();

    bool commandRepeatLastChange();

    bool commandAlignLine();
    bool commandAlignLines();

    bool commandAddToNumber();
    bool commandSubtractFromNumber();

    bool commandPrependToBlock();
    bool commandAppendToBlock();

    bool commandGoToNextJump();
    bool commandGoToPrevJump();

    bool commandSwitchToLeftView();
    bool commandSwitchToUpView();
    bool commandSwitchToDownView();
    bool commandSwitchToRightView();
    bool commandSwitchToNextView();

    bool commandSplitHoriz();
    bool commandSplitVert();
    bool commandCloseView();

    bool commandSwitchToNextTab();
    bool commandSwitchToPrevTab();

    bool commandFormatLine();
    bool commandFormatLines();

    bool commandCollapseToplevelNodes();
    bool commandCollapseLocal();
    bool commandExpandAll();
    bool commandExpandLocal();
    bool commandToggleRegionVisibility();

    bool commandStartRecordingMacro();
    bool commandReplayMacro();

    bool commandCloseWrite();
    bool commandCloseNocheck();

    // MOTIONS

    Range motionLeft();
    Range motionRight();
    Range motionDown();
    Range motionUp();

    Range motionPageDown();
    Range motionPageUp();
    Range motionHalfPageDown();
    Range motionHalfPageUp();

    Range motionUpToFirstNonBlank();
    Range motionDownToFirstNonBlank();

    Range motionWordForward();
    Range motionWordBackward();
    Range motionWORDForward();
    Range motionWORDBackward();

    Range motionToEndOfWord();
    Range motionToEndOfWORD();
    Range motionToEndOfPrevWord();
    Range motionToEndOfPrevWORD();

    Range motionFindChar();
    Range motionFindCharBackward();
    Range motionToChar();
    Range motionToCharBackward();
    Range motionRepeatlastTF();
    Range motionRepeatlastTFBackward();

    Range motionToEOL();
    Range motionToColumn0();
    Range motionToFirstCharacterOfLine();

    Range motionToLineFirst();
    Range motionToLineLast();

    Range motionToScreenColumn();

    Range motionToMark();
    Range motionToMarkLine();

    Range motionToMatchingItem();

    Range motionToPreviousBraceBlockStart();
    Range motionToNextBraceBlockStart();
    Range motionToPreviousBraceBlockEnd();
    Range motionToNextBraceBlockEnd();

    Range motionToNextOccurrence();
    Range motionToPrevOccurrence();

    Range motionToFirstLineOfWindow();
    Range motionToMiddleLineOfWindow();
    Range motionToLastLineOfWindow();

    Range motionToNextVisualLine();
    Range motionToPrevVisualLine();

    Range motionToPreviousSentence();
    Range motionToNextSentence();

    Range motionToBeforeParagraph();
    Range motionToAfterParagraph();

    Range motionToIncrementalSearchMatch();

    // TEXT OBJECTS

    Range textObjectAWord();
    Range textObjectInnerWord();
    Range textObjectAWORD();
    Range textObjectInnerWORD();

    Range textObjectInnerSentence();
    Range textObjectASentence();

    Range textObjectInnerParagraph();
    Range textObjectAParagraph();

    Range textObjectAQuoteDouble();
    Range textObjectInnerQuoteDouble();

    Range textObjectAQuoteSingle();
    Range textObjectInnerQuoteSingle();

    Range textObjectABackQuote();
    Range textObjectInnerBackQuote();

    Range textObjectAParen();
    Range textObjectInnerParen();

    Range textObjectABracket();
    Range textObjectInnerBracket();

    Range textObjectACurlyBracket();
    Range textObjectInnerCurlyBracket();

    Range textObjectAInequalitySign();
    Range textObjectInnerInequalitySign();

    Range textObjectAComma();
    Range textObjectInnerComma();

    virtual void reset();

    void beginMonitoringDocumentChanges();

protected:
    void resetParser();
    void initializeCommands();
    QRegExp generateMatchingItemRegex() const;
    void executeCommand(const Command *cmd);
    OperationMode getOperationMode() const;

    void highlightYank(const Range &range, const OperationMode mode = CharWise);
    void addHighlightYank(const KTextEditor::Range &range);

    bool motionWillBeUsedWithCommand() const
    {
        return !m_awaitingMotionOrTextObject.isEmpty();
    };

    void joinLines(unsigned int from, unsigned int to) const;
    void reformatLines(unsigned int from, unsigned int to) const;
    bool executeKateCommand(const QString &command);

    /**
     * Get the index of the first non-blank character from the given line.
     *
     * @param line The line to be picked. The current line will picked instead
     * if this parameter is set to a negative value.
     * @returns the index of the first non-blank character from the given line.
     * If a non-space character cannot be found, the 0 is returned.
     */
    int getFirstNonBlank(int line = -1) const;

    Range textObjectComma(bool inner) const;
    void shrinkRangeAroundCursor(Range &toShrink, const Range &rangeToShrinkTo) const;
    KTextEditor::Cursor findSentenceStart();
    KTextEditor::Cursor findSentenceEnd();
    KTextEditor::Cursor findParagraphStart();
    KTextEditor::Cursor findParagraphEnd();

protected:
    // The 'current position' is the current cursor position for non-linewise pastes, and the current
    // line for linewise.
    enum PasteLocation { AtCurrentPosition, AfterCurrentPosition };
    bool paste(NormalViMode::PasteLocation pasteLocation, bool isgPaste, bool isIndentedPaste);
    KTextEditor::Cursor cursorPosAtEndOfPaste(const KTextEditor::Cursor &pasteLocation, const QString &pastedText) const;

protected:
    QString m_keys;
    QString m_lastTFcommand; // holds the last t/T/f/F command so that it can be repeated with ;/,

    unsigned int m_countTemp;
    int m_motionOperatorIndex;
    int m_scroll_count_limit;

    QVector<Command *> m_commands;
    QVector<Motion *> m_motions;
    QVector<int> m_matchingCommands;
    QVector<int> m_matchingMotions;
    QStack<int> m_awaitingMotionOrTextObject;

    bool m_findWaitingForChar;
    bool m_isRepeatedTFcommand;
    bool m_linewiseCommand;
    bool m_commandWithMotion;
    bool m_lastMotionWasLinewiseInnerBlock;
    bool m_motionCanChangeWholeVisualModeSelection;
    bool m_commandShouldKeepSelection;
    bool m_deleteCommand;
    // Ctrl-c or ESC have been pressed, leading to a call to reset().
    bool m_pendingResetIsDueToExit;
    bool m_isUndo;
    bool waitingForRegisterOrCharToSearch();

    // item matching ('%' motion)
    QHash<QString, QString> m_matchingItems;
    QRegExp m_matchItemRegex;

    KeyParser *m_keyParser;

    KTextEditor::Attribute::Ptr m_highlightYankAttribute;
    QSet<KTextEditor::MovingRange *> m_highlightedYanks;
    QSet<KTextEditor::MovingRange *> &highlightedYankForDocument();

    KTextEditor::Cursor m_currentChangeEndMarker;
    KTextEditor::Cursor m_positionWhenIncrementalSearchBegan;

private Q_SLOTS:
    void textInserted(KTextEditor::Document *document, KTextEditor::Range range);
    void textRemoved(KTextEditor::Document *, KTextEditor::Range);
    void undoBeginning();
    void undoEnded();

    void updateYankHighlightAttrib();
    void clearYankHighlight();
    void aboutToDeleteMovingInterfaceContent();
};

}

#endif /* KATEVI_NORMAL_VI_MODE_H */
