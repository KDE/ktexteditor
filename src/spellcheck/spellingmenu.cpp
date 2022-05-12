/*
    SPDX-FileCopyrightText: 2009-2010 Michel Ludwig <michel.ludwig@kdemail.net>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "spellingmenu.h"

#include "katedocument.h"
#include "kateglobal.h"
#include "kateview.h"
#include "ontheflycheck.h"
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
{
}

KateSpellingMenu::~KateSpellingMenu()
{
    m_currentMisspelledRange = nullptr; // it shouldn't be accessed anymore as it could
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
    if (m_currentMisspelledRange == range) {
        return;
    }
    m_currentMisspelledRange = nullptr;
    m_currentMisspelledRange = range;
}

void KateSpellingMenu::caretExitedMisspelledRange(KTextEditor::MovingRange *range)
{
    if (range != m_currentMisspelledRange) {
        // The order of 'exited' and 'entered' signals was wrong
        return;
    }
    m_currentMisspelledRange = nullptr;
}

void KateSpellingMenu::rangeDeleted(KTextEditor::MovingRange *range)
{
    if (m_currentMisspelledRange == range) {
        m_currentMisspelledRange = nullptr;
    }
}

void KateSpellingMenu::prepareToBeShown()
{
    if (!m_view->doc()->onTheFlySpellChecker()) {
        // Nothing todo!
        return;
    }

    auto sr = m_view->selectionRange();
    if (sr.isValid()) {
        // Selected words need a special handling to work properly
        auto imv = m_view->doc()->onTheFlySpellChecker()->installedMovingRanges(sr);
        for (int i = 0; i < imv.size(); ++i) {
            if (imv.at(i)->toRange() == sr) {
                m_currentMisspelledRange = nullptr;
                m_currentMisspelledRange = imv.at(i);
                break;
            }
        }
    }

    setEnabled(m_currentMisspelledRange != nullptr);
    setVisible(m_currentMisspelledRange != nullptr);
}

void KateSpellingMenu::populateSuggestionsMenu()
{
    m_spellingMenu->clear();
    if (!m_currentMisspelledRange) {
        return;
    }
    m_spellingMenu->addAction(m_ignoreWordAction);
    m_spellingMenu->addAction(m_addToDictionaryAction);
    m_spellingMenu->addSeparator();
    const QString &misspelledWord = m_view->doc()->text(*m_currentMisspelledRange);
    const QString dictionary = m_view->doc()->dictionaryForMisspelledRange(*m_currentMisspelledRange);
    const auto suggestionList = KTextEditor::EditorPrivate::self()->spellCheckManager()->suggestions(misspelledWord, dictionary);

    int counter = 10;
    for (QStringList::const_iterator i = suggestionList.cbegin(); i != suggestionList.cend() && counter > 0; ++i) {
        const QString &suggestion = *i;
        QAction *action = new QAction(suggestion, m_spellingMenu);
        connect(action, &QAction::triggered, this, [suggestion, this]() {
            replaceWordBySuggestion(suggestion);
        });
        m_spellingMenu->addAction(action);
        --counter;
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
