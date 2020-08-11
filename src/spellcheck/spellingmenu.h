/*
    SPDX-FileCopyrightText: 2008-2009 Michel Ludwig <michel.ludwig@kdemail.net>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef SPELLINGMENU_H
#define SPELLINGMENU_H

#include <QObject>

#include <KActionCollection>
#include <KActionMenu>
#include <ktexteditor/movingrange.h>
#include <ktexteditor/movingrangefeedback.h>
#include <ktexteditor/range.h>
#include <ktexteditor/view.h>

namespace KTextEditor
{
class ViewPrivate;
}
class KateOnTheFlyChecker;

class KateSpellingMenu : public QObject
{
    Q_OBJECT
    friend class KateOnTheFlyChecker;

public:
    explicit KateSpellingMenu(KTextEditor::ViewPrivate *view);
    virtual ~KateSpellingMenu();

    bool isEnabled() const;

    void createActions(KActionCollection *ac);

    /**
     * This method has to be called before the menu is shown in response to a context
     * menu event. It will trigger that the misspelled range located under the mouse pointer
     * is considered for the spelling suggestions.
     **/
    void setUseMouseForMisspelledRange(bool b);

public Q_SLOTS:
    void setEnabled(bool b);
    void setVisible(bool b);

protected:
    KTextEditor::ViewPrivate *m_view;
    KActionMenu *m_spellingMenuAction;
    QAction *m_ignoreWordAction, *m_addToDictionaryAction;
    QMenu *m_spellingMenu;
    KTextEditor::MovingRange *m_currentMisspelledRange;
    KTextEditor::MovingRange *m_currentMouseMisspelledRange;
    KTextEditor::MovingRange *m_currentCaretMisspelledRange;
    bool m_useMouseForMisspelledRange;
    QStringList m_currentSuggestions;

    // These methods are called from KateOnTheFlyChecker to inform about events involving
    // moving ranges.
    void rangeDeleted(KTextEditor::MovingRange *range);
    void caretEnteredMisspelledRange(KTextEditor::MovingRange *range);
    void caretExitedMisspelledRange(KTextEditor::MovingRange *range);
    void mouseEnteredMisspelledRange(KTextEditor::MovingRange *range);
    void mouseExitedMisspelledRange(KTextEditor::MovingRange *range);

protected Q_SLOTS:
    void populateSuggestionsMenu();
    void replaceWordBySuggestion(const QString &suggestion);

    void addCurrentWordToDictionary();
    void ignoreCurrentWord();
};

#endif
