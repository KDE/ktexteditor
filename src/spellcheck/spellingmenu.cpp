/*
    SPDX-FileCopyrightText: 2009-2010 Michel Ludwig <michel.ludwig@kdemail.net>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "spellingmenu.h"

#include "katedocument.h"
#include "kateglobal.h"
#include "kateview.h"
#include "spellcheck/spellcheck.h"

#include "katepartdebug.h"
#include <KLocalizedString>

KateSpellingMenu::KateSpellingMenu(KTextEditor::ViewPrivate *view)
    : QObject(view)
    , m_view(view)
    , m_spellingMenuAction(nullptr)
    , m_ignoreWordAction(nullptr)
    , m_addToDictionaryAction(nullptr)
    , m_spellingMenu(nullptr)
    , m_currentMisspelledRange(nullptr)
    , // we have to use 'm_currentMisspelledRange'
    // as QSignalMapper doesn't work with pairs of objects;
    // it just points to the object pointed to by either
    // 'm_currentMouseMisspelledRange' or 'm_currentCaretMisspelledRange'
    // TODO: pass the range into the lambda instead
    m_currentMouseMisspelledRange(nullptr)
    , m_currentCaretMisspelledRange(nullptr)
    , m_useMouseForMisspelledRange(false)
{
}

KateSpellingMenu::~KateSpellingMenu()
{
    m_currentMisspelledRange = nullptr; // it shouldn't be accessed anymore as it could
    // point to a non-existing object (bug 226724)
    // (for example, when it pointed to m_currentCaretMisspelledRange
    // and that range got deleted after the caret had left)
    m_currentCaretMisspelledRange = nullptr;
    m_currentMouseMisspelledRange = nullptr;
}

bool KateSpellingMenu::isEnabled() const
{
    if (!m_spellingMenuAction) {
        return false;
    }
    return m_spellingMenuAction->isEnabled();
}

void KateSpellingMenu::setEnabled(bool b)
{
    if (m_spellingMenuAction) {
        m_spellingMenuAction->setEnabled(b);
    }
    if (m_ignoreWordAction) {
        m_ignoreWordAction->setEnabled(b);
    }
    if (m_addToDictionaryAction) {
        m_addToDictionaryAction->setEnabled(b);
    }
}

void KateSpellingMenu::setVisible(bool b)
{
    if (m_spellingMenuAction) {
        m_spellingMenuAction->setVisible(b);
    }
    if (m_ignoreWordAction) {
        m_ignoreWordAction->setVisible(b);
    }
    if (m_addToDictionaryAction) {
        m_addToDictionaryAction->setVisible(b);
    }
}

void KateSpellingMenu::createActions(KActionCollection *ac)
{
    m_spellingMenuAction = new KActionMenu(i18n("Spelling"), this);
    ac->addAction(QStringLiteral("spelling_suggestions"), m_spellingMenuAction);
    m_spellingMenu = m_spellingMenuAction->menu();
    connect(m_spellingMenu, &QMenu::aboutToShow, this, &KateSpellingMenu::populateSuggestionsMenu);

    m_ignoreWordAction = new QAction(i18n("Ignore Word"), this);
    connect(m_ignoreWordAction, &QAction::triggered, this, &KateSpellingMenu::ignoreCurrentWord);

    m_addToDictionaryAction = new QAction(i18n("Add to Dictionary"), this);
    connect(m_addToDictionaryAction, &QAction::triggered, this, &KateSpellingMenu::addCurrentWordToDictionary);

    setEnabled(false);
    setVisible(false);
}

void KateSpellingMenu::caretEnteredMisspelledRange(KTextEditor::MovingRange *range)
{
    if (m_currentCaretMisspelledRange == range) {
        return;
    }
    m_currentCaretMisspelledRange = nullptr;
    setEnabled(true);
    m_currentCaretMisspelledRange = range;
}

void KateSpellingMenu::caretExitedMisspelledRange(KTextEditor::MovingRange *range)
{
    if (range != m_currentCaretMisspelledRange) { // order of 'exit' and 'entered' signals
        return; // was wrong
    }
    setEnabled(false);
    m_currentCaretMisspelledRange = nullptr;
}

void KateSpellingMenu::mouseEnteredMisspelledRange(KTextEditor::MovingRange *range)
{
    if (m_currentMouseMisspelledRange == range) {
        return;
    }
    m_currentMouseMisspelledRange = range;
}

void KateSpellingMenu::mouseExitedMisspelledRange(KTextEditor::MovingRange *range)
{
    if (range != m_currentMouseMisspelledRange) { // order of 'exit' and 'entered' signals
        return; // was wrong
    }
    m_currentMouseMisspelledRange = nullptr;
}

void KateSpellingMenu::rangeDeleted(KTextEditor::MovingRange *range)
{
    if (m_currentCaretMisspelledRange == range) {
        m_currentCaretMisspelledRange = nullptr;
    }
    if (m_currentMouseMisspelledRange == range) {
        m_currentMouseMisspelledRange = nullptr;
    }
    if (m_currentMisspelledRange == range) {
        m_currentMisspelledRange = nullptr;
    }
    setEnabled(m_currentCaretMisspelledRange != nullptr);
}

void KateSpellingMenu::setUseMouseForMisspelledRange(bool b)
{
    m_useMouseForMisspelledRange = b;
    if (m_useMouseForMisspelledRange) {
        setEnabled(m_currentMouseMisspelledRange != nullptr);
    } else if (!m_useMouseForMisspelledRange) {
        setEnabled(m_currentCaretMisspelledRange != nullptr);
    }
}

void KateSpellingMenu::populateSuggestionsMenu()
{
    m_spellingMenu->clear();
    if ((m_useMouseForMisspelledRange && !m_currentMouseMisspelledRange) || (!m_useMouseForMisspelledRange && !m_currentCaretMisspelledRange)) {
        return;
    }
    m_currentMisspelledRange = (m_useMouseForMisspelledRange ? m_currentMouseMisspelledRange : m_currentCaretMisspelledRange);
    m_spellingMenu->addAction(m_ignoreWordAction);
    m_spellingMenu->addAction(m_addToDictionaryAction);
    m_spellingMenu->addSeparator();
    const QString &misspelledWord = m_view->doc()->text(*m_currentMisspelledRange);
    const QString dictionary = m_view->doc()->dictionaryForMisspelledRange(*m_currentMisspelledRange);
    m_currentSuggestions = KTextEditor::EditorPrivate::self()->spellCheckManager()->suggestions(misspelledWord, dictionary);

    int counter = 0;
    for (QStringList::const_iterator i = m_currentSuggestions.cbegin(); i != m_currentSuggestions.cend() && counter < 10; ++i) {
        const QString &suggestion = *i;
        QAction *action = new QAction(suggestion, m_spellingMenu);
        connect(action, &QAction::triggered, this, [suggestion, this]() {
            replaceWordBySuggestion(suggestion);
        });
        m_spellingMenu->addAction(action);
        ++counter;
    }
}

void KateSpellingMenu::replaceWordBySuggestion(const QString &suggestion)
{
    KTextEditor::DocumentPrivate *doc = m_view->doc();
    KTextEditor::EditorPrivate::self()->spellCheckManager()->replaceCharactersEncodedIfNecessary(suggestion, doc, *m_currentMisspelledRange);
}

void KateSpellingMenu::addCurrentWordToDictionary()
{
    if (!m_currentMisspelledRange) {
        return;
    }
    const QString &misspelledWord = m_view->doc()->text(*m_currentMisspelledRange);
    const QString dictionary = m_view->doc()->dictionaryForMisspelledRange(*m_currentMisspelledRange);
    KTextEditor::EditorPrivate::self()->spellCheckManager()->addToDictionary(misspelledWord, dictionary);
    m_view->doc()->clearMisspellingForWord(misspelledWord); // WARNING: 'm_currentMisspelledRange' is deleted here!
}

void KateSpellingMenu::ignoreCurrentWord()
{
    if (!m_currentMisspelledRange) {
        return;
    }
    const QString &misspelledWord = m_view->doc()->text(*m_currentMisspelledRange);
    const QString dictionary = m_view->doc()->dictionaryForMisspelledRange(*m_currentMisspelledRange);
    KTextEditor::EditorPrivate::self()->spellCheckManager()->ignoreWord(misspelledWord, dictionary);
    m_view->doc()->clearMisspellingForWord(misspelledWord); // WARNING: 'm_currentMisspelledRange' is deleted here!
}
