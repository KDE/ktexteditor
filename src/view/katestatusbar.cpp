/* This file is part of the KDE and the Kate project
 *
 *   Copyright (C) 2013 Dominik Haumann <dhaumann@kde.org>
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

#include "katestatusbar.h"

#include "katemodemenu.h"
#include "kateglobal.h"
#include "katemodemanager.h"
#include "katedocument.h"
#include "kateconfig.h"
#include "kateabstractinputmode.h"
#include "wordcounter.h"

#include <KLocalizedString>
#include <KIconUtils>
#include <KAcceleratorManager>
#include <Sonnet/Speller>

#include <QHBoxLayout>
#include <QInputDialog>

//BEGIN menu
KateStatusBarOpenUpMenu::KateStatusBarOpenUpMenu(QWidget *parent) : QMenu(parent) {}
KateStatusBarOpenUpMenu::~KateStatusBarOpenUpMenu() {}

void KateStatusBarOpenUpMenu::setVisible(bool visibility)
{
    if (visibility) {
        QRect geo = geometry();
        QPoint pos = parentWidget()->mapToGlobal(QPoint(0, 0));
        geo.moveTopLeft(QPoint(pos.x(), pos.y() - geo.height()));
        if (geo.top() < 0) geo.moveTop(0);
        setGeometry(geo);
    }

    QMenu::setVisible(visibility);
}
//END menu

//BEGIN StatusBarButton
StatusBarButton::StatusBarButton(KateStatusBar *parent, const QString &text /*= QString()*/)
    : QPushButton(text, parent)
{
    setFlat(true);
    setFocusProxy(parent->m_view);
    setMinimumSize(QSize(1, minimumSizeHint().height()));
}

StatusBarButton::~StatusBarButton()
{
}
//END StatusBarButton

KateStatusBar::KateStatusBar(KTextEditor::ViewPrivate *view)
    : KateViewBarWidget(false)
    , m_view(view)
    , m_modifiedStatus(-1)
    , m_selectionMode(-1)
    , m_wordCounter(nullptr)
{
    KAcceleratorManager::setNoAccel(this);
    setFocusProxy(m_view);

    /**
     * just add our status bar to central widget, full sized
     */
    QHBoxLayout *topLayout = new QHBoxLayout(centralWidget());
    topLayout->setContentsMargins(0, 0, 0, 0);
    topLayout->setSpacing(4);

    /**
     * show modification state of the document
     * TODO Using a (StatusBar)Button is currently pointless but handy due to no "setIcon()" function in QLabel.
     *      Add some useful action when button is clicked, e.g. save document or show tool-tip
     *      or find a way to not show a "focus frame" when hovered by mouse
     */
    m_modified = new StatusBarButton(this);
    topLayout->addWidget(m_modified);

    /**
     * show Line XXX, Column XXX
     */
    m_cursorPosition = new StatusBarButton(this);
    topLayout->addWidget(m_cursorPosition);
    m_cursorPosition->setWhatsThis(i18n("Current cursor position. Click to go to a specific line."));
    connect(m_cursorPosition, &StatusBarButton::clicked, m_view, &KTextEditor::ViewPrivate::gotoLine);

    // Separate the status line in a left and right part
    topLayout->addStretch(1);

    /**
     * show the current mode, like INSERT, OVERWRITE, VI + modifiers like [BLOCK]
     */
    m_inputMode = new StatusBarButton(this);
    topLayout->addWidget(m_inputMode);
    m_inputMode->setWhatsThis(i18n("Insert mode and VI input mode indicator. Click to change the mode."));
    connect(m_inputMode, &StatusBarButton::clicked, [=] { m_view->currentInputMode()->toggleInsert(); });

    /**
     * Add dictionary button which allows user to switch dictionary of the document
     */
    m_dictionary = new StatusBarButton(this);
    topLayout->addWidget(m_dictionary, 0);
    m_dictionary->setWhatsThis(i18n("Change dictionary"));
    m_dictionaryMenu = new KateStatusBarOpenUpMenu(m_dictionary);
    m_dictionaryMenu->addAction(m_view->action("tools_change_dictionary"));
    m_dictionaryMenu->addAction(m_view->action("tools_clear_dictionary_ranges"));
    m_dictionaryMenu->addAction(m_view->action("tools_toggle_automatic_spell_checking"));
    m_dictionaryMenu->addAction(m_view->action("tools_spelling_from_cursor"));
    m_dictionaryMenu->addAction(m_view->action("tools_spelling"));
    m_dictionaryMenu->addSeparator();
    m_dictionaryGroup = new QActionGroup(m_dictionaryMenu);
    QMapIterator<QString, QString> i(Sonnet::Speller().preferredDictionaries());
    while (i.hasNext()) {
        i.next();
        QAction *action = m_dictionaryGroup->addAction(i.key());
        action->setData(i.value());
        action->setToolTip(i.key());
        action->setCheckable(true);
        m_dictionaryMenu->addAction(action);
    }
    m_dictionary->setMenu(m_dictionaryMenu);
    connect(m_dictionaryGroup, &QActionGroup::triggered, this, &KateStatusBar::changeDictionary);

    /**
     * allow to change indentation configuration
     */
    m_tabsIndent = new StatusBarButton(this);
    topLayout->addWidget(m_tabsIndent);

    m_indentSettingsMenu = new KateStatusBarOpenUpMenu(m_tabsIndent);
    m_indentSettingsMenu->addSection(i18n("Tab Width"));
    m_tabGroup = new QActionGroup(this);
    addNumberAction(m_tabGroup, m_indentSettingsMenu, -1);
    addNumberAction(m_tabGroup, m_indentSettingsMenu, 8);
    addNumberAction(m_tabGroup, m_indentSettingsMenu, 4);
    addNumberAction(m_tabGroup, m_indentSettingsMenu, 2);
    connect(m_tabGroup, &QActionGroup::triggered, this, &KateStatusBar::slotTabGroup);

    m_indentSettingsMenu->addSection(i18n("Indentation Width"));
    m_indentGroup = new QActionGroup(this);
    addNumberAction(m_indentGroup, m_indentSettingsMenu, -1);
    addNumberAction(m_indentGroup, m_indentSettingsMenu, 8);
    addNumberAction(m_indentGroup, m_indentSettingsMenu, 4);
    addNumberAction(m_indentGroup, m_indentSettingsMenu, 2);
    connect(m_indentGroup, &QActionGroup::triggered, this, &KateStatusBar::slotIndentGroup);

    m_indentSettingsMenu->addSection(i18n("Indentation Mode"));
    QActionGroup *radioGroup = new QActionGroup(m_indentSettingsMenu);
    m_mixedAction = m_indentSettingsMenu->addAction(i18n("Tabulators && Spaces"));
    m_mixedAction->setCheckable(true);
    m_mixedAction->setActionGroup(radioGroup);
    m_hardAction = m_indentSettingsMenu->addAction(i18n("Tabulators"));
    m_hardAction->setCheckable(true);
    m_hardAction->setActionGroup(radioGroup);
    m_softAction = m_indentSettingsMenu->addAction(i18n("Spaces"));
    m_softAction->setCheckable(true);
    m_softAction->setActionGroup(radioGroup);
    connect(radioGroup, &QActionGroup::triggered, this, &KateStatusBar::slotIndentTabMode);

    m_tabsIndent->setMenu(m_indentSettingsMenu);

    /**
     * add encoding button which allows user to switch encoding of document
     * this will reuse the encoding action menu of the view
     */
    m_encoding = new StatusBarButton(this);
    topLayout->addWidget(m_encoding);
    m_encoding->setMenu(m_view->encodingAction()->menu());
    m_encoding->setWhatsThis(i18n("Encoding"));

    /**
     * add mode button which allows user to switch mode of document
     * this will reuse the mode action menu of the view
     */
    m_mode = new StatusBarButton(this);
    topLayout->addWidget(m_mode);
    m_mode->setMenu(m_view->modeAction()->menu());
    m_mode->setWhatsThis(i18n("Syntax highlighting"));

    // signals for the statusbar
    connect(m_view, &KTextEditor::View::cursorPositionChanged, this, &KateStatusBar::cursorPositionChanged);
    connect(m_view, &KTextEditor::View::viewModeChanged, this, &KateStatusBar::viewModeChanged);
    connect(m_view, &KTextEditor::View::selectionChanged, this, &KateStatusBar::selectionChanged);
    connect(m_view->document(), &KTextEditor::DocumentPrivate::modifiedChanged, this, &KateStatusBar::modifiedChanged);
    connect(m_view->doc(), &KTextEditor::DocumentPrivate::modifiedOnDisk, this, &KateStatusBar::modifiedChanged);
    connect(m_view->doc(), &KTextEditor::DocumentPrivate::configChanged, this, &KateStatusBar::documentConfigChanged);
    connect(m_view->document(), &KTextEditor::DocumentPrivate::modeChanged, this, &KateStatusBar::modeChanged);
    connect(m_view, &KTextEditor::ViewPrivate::configChanged, this, &KateStatusBar::configChanged);
    connect(m_view->doc(), &KTextEditor::DocumentPrivate::defaultDictionaryChanged, this, &KateStatusBar::updateDictionary);
    connect(m_view->doc(), &KTextEditor::DocumentPrivate::dictionaryRangesPresent, this, &KateStatusBar::updateDictionary);
    connect(m_view, &KTextEditor::ViewPrivate::caretChangedRange, this, &KateStatusBar::updateDictionary);

    updateStatus();
    toggleWordCount(KateViewConfig::global()->showWordCount());
}

bool KateStatusBar::eventFilter(QObject *obj, QEvent *event)
{
    return KateViewBarWidget::eventFilter(obj, event);
}

void KateStatusBar::contextMenuEvent(QContextMenuEvent *event)
{
    // TODO Add option "Show Statusbar" and options to show/hide buttons of the status bar
    QMenu menu(this);
    QAction *showLines = menu.addAction(QStringLiteral("Show line count"), this, &KateStatusBar::toggleShowLines);
    showLines->setCheckable(true);
    showLines->setChecked(KateViewConfig::global()->showLineCount());
    QAction *showWords = menu.addAction(QStringLiteral("Show word count"), this, &KateStatusBar::toggleShowWords);
    showWords->setCheckable(true);
    showWords->setChecked(KateViewConfig::global()->showWordCount());
    menu.exec(event->globalPos());

}

void KateStatusBar::toggleShowLines(bool checked)
{
    KateViewConfig::global()->setShowLineCount(checked);
}

void KateStatusBar::toggleShowWords(bool checked)
{
    KateViewConfig::global()->setShowWordCount(checked);
}

void KateStatusBar::updateStatus()
{
    selectionChanged();
    viewModeChanged();
    cursorPositionChanged();
    modifiedChanged();
    documentConfigChanged();
    modeChanged();
    updateDictionary();
}

void KateStatusBar::selectionChanged()
{
    const unsigned int newSelectionMode = m_view->blockSelection();
    if (newSelectionMode == m_selectionMode) {
        return;
    }

    // remember new mode and update info
    m_selectionMode = newSelectionMode;
    viewModeChanged();
}

void KateStatusBar::viewModeChanged()
{
    // prepend BLOCK for block selection mode
    QString text = m_view->viewModeHuman();
    if (m_view->blockSelection())
        text = i18n("[BLOCK] %1", text);

    m_inputMode->setText(text);
}

void KateStatusBar::cursorPositionChanged()
{
    KTextEditor::Cursor position(m_view->cursorPositionVirtual());

    // Update line/column label
    QString text;
    if (KateViewConfig::global()->showLineCount()) {
        text = i18n("Line %1 of %2, Column %3", QLocale().toString(position.line() + 1)
                                              , QLocale().toString(m_view->doc()->lines())
                                              , QLocale().toString(position.column() + 1)
                   );
    } else {
        text = i18n("Line %1, Column %2", QLocale().toString(position.line() + 1)
                                        , QLocale().toString(position.column() + 1)
                   );
    }
    if (m_wordCounter) {
        text.append(QStringLiteral(", ") + m_wordCount);
    }
    m_cursorPosition->setText(text);
}

void KateStatusBar::updateDictionary()
{
    QString newDict;
    // Check if at the current cursor position is a special dictionary in use
    KTextEditor::Cursor position(m_view->cursorPositionVirtual());
    const QList<QPair<KTextEditor::MovingRange *, QString>> dictRanges = m_view->doc()->dictionaryRanges();
    for (auto rangeDictPair : dictRanges) {
        const KTextEditor::MovingRange *range = rangeDictPair.first;
        if (range->contains(position) || range->end() == position) {
            newDict = rangeDictPair.second;
            break;
        }
    }
    // Check if the default dictionary is in use
    if (newDict.isEmpty()) {
        newDict = m_view->doc()->defaultDictionary();
        if (newDict.isEmpty()) {
            newDict = Sonnet::Speller().defaultLanguage();
        }
    }
    // Update button and menu only on a changed dictionary
    if (!m_dictionaryGroup->checkedAction() || (m_dictionaryGroup->checkedAction()->data().toString() != newDict) || m_dictionary->text().isEmpty()) {
        bool found = false;
        // Remove "-w_accents -variant_0" and such from dict-code to keep it small and clean
        m_dictionary->setText(newDict.section(QLatin1Char('-'), 0, 0));
        // For maximum user clearness, change the checked menu option
        m_dictionaryGroup->blockSignals(true);
        for (auto a : m_dictionaryGroup->actions()) {
            if (a->data().toString() == newDict) {
                a->setChecked(true);
                found = true;
                break;
            }
        }
        if (!found) {
            // User has chose some other dictionary from combo box, we need to add that
            QString dictName = Sonnet::Speller().availableDictionaries().key(newDict);
            QAction *action = m_dictionaryGroup->addAction(dictName);
            action->setData(newDict);
            action->setCheckable(true);
            action->setChecked(true);
            m_dictionaryMenu->addAction(action);
        }
        m_dictionaryGroup->blockSignals(false);
    }
}

void KateStatusBar::modifiedChanged()
{
    const bool mod = m_view->doc()->isModified();
    const bool modOnHD = m_view->doc()->isModifiedOnDisc();

    /**
     * combine to modified status, update only if changed
     */
    unsigned int newStatus = (unsigned int)mod | ((unsigned int)modOnHD << 1);
    if (m_modifiedStatus == newStatus)
        return;

    m_modifiedStatus = newStatus;
    switch (m_modifiedStatus) {
    case 0x1:
        m_modified->setIcon(QIcon::fromTheme(QStringLiteral("document-save")));
        m_modified->setWhatsThis(i18n("Meaning of current icon: Document was modified since it was loaded"));
        break;

    case 0x2:
        m_modified->setIcon(QIcon::fromTheme(QStringLiteral("dialog-warning")));
        m_modified->setWhatsThis(i18n("Meaning of current icon: Document was modified or deleted by another program"));
        break;

    case 0x3:
        m_modified->setIcon(QIcon(KIconUtils::addOverlay(QIcon::fromTheme(QStringLiteral("document-save")),
                                       QIcon(QStringLiteral("emblem-important")), Qt::TopLeftCorner)));
        m_modified->setWhatsThis(QString());
        break;

    default:
        m_modified->setIcon(QIcon::fromTheme(QStringLiteral("text-plain")));
        m_modified->setWhatsThis(i18n("Meaning of current icon: Document was not modified since it was loaded"));
        break;
    }
}

void KateStatusBar::documentConfigChanged()
{
    m_encoding->setText(m_view->document()->encoding());
    KateDocumentConfig *config = ((KTextEditor::DocumentPrivate *)m_view->document())->config();
    int tabWidth = config->tabWidth();
    int indentationWidth = config->indentationWidth();
    bool replaceTabsDyn = config->replaceTabsDyn();

    static const KLocalizedString spacesOnly = ki18n("Soft Tabs: %1");
    static const KLocalizedString spacesOnlyShowTabs = ki18n("Soft Tabs: %1 (%2)");
    static const KLocalizedString tabsOnly = ki18n("Tab Size: %1");
    static const KLocalizedString tabSpacesMixed = ki18n("Indent/Tab: %1/%2");

    if (!replaceTabsDyn) {
        if (tabWidth == indentationWidth) {
            m_tabsIndent->setText(tabsOnly.subs(tabWidth).toString());
            m_tabGroup->setEnabled(false);
            m_hardAction->setChecked(true);
        } else {
            m_tabsIndent->setText(tabSpacesMixed.subs(indentationWidth).subs(tabWidth).toString());
            m_tabGroup->setEnabled(true);
            m_mixedAction->setChecked(true);
        }
    } else {
        if (tabWidth == indentationWidth) {
            m_tabsIndent->setText(spacesOnly.subs(indentationWidth).toString());
            m_tabGroup->setEnabled(true);
            m_softAction->setChecked(true);
        } else {
            m_tabsIndent->setText(spacesOnlyShowTabs.subs(indentationWidth).subs(tabWidth).toString());
            m_tabGroup->setEnabled(true);
            m_softAction->setChecked(true);
        }
    }

    updateGroup(m_tabGroup, tabWidth);
    updateGroup(m_indentGroup, indentationWidth);
}

void KateStatusBar::modeChanged()
{
    m_mode->setText(KTextEditor::EditorPrivate::self()->modeManager()->fileType(m_view->document()->mode()).nameTranslated());
}

void KateStatusBar::addNumberAction(QActionGroup *group, QMenu *menu, int data)
{
    QAction *a;
    if (data != -1) {
        a = menu->addAction(QStringLiteral("%1").arg(data));
    } else {
        a = menu->addAction(i18n("Other..."));
    }
    a->setData(data);
    a->setCheckable(true);
    a->setActionGroup(group);
}

void KateStatusBar::updateGroup(QActionGroup *group, int w)
{
    QAction *m1 = nullptr;
    bool found = false;
    //linear search should be fast enough here, no additional hash
    for (QAction *action : group->actions()) {
        int val = action->data().toInt();
        if (val == -1) m1 = action;
        if (val == w) {
            found = true;
            action->setChecked(true);
        }
    }
    if (found) {
        m1->setText(i18n("Other..."));
    } else {
        m1->setText(i18np("Other (%1)", "Other (%1)", w));
        m1->setChecked(true);
    }
}

void KateStatusBar::slotTabGroup(QAction *a)
{
    int val = a->data().toInt();
    bool ok;
    KateDocumentConfig *config = ((KTextEditor::DocumentPrivate *)m_view->document())->config();
    if (val == -1) {
        val = QInputDialog::getInt(this, i18n("Tab Width"), i18n("Please specify the wanted tab width:"), config->tabWidth(), 1, 16, 1, &ok);
        if (!ok) val = config->tabWidth();
    }
    config->setTabWidth(val);
}

void KateStatusBar::slotIndentGroup(QAction *a)
{
    int val = a->data().toInt();
    bool ok;
    KateDocumentConfig *config = ((KTextEditor::DocumentPrivate *)m_view->document())->config();
    if (val == -1) {
        val = QInputDialog::getInt(this, i18n("Indentation Width"), i18n("Please specify the wanted indentation width:"), config->indentationWidth(), 1, 16, 1, &ok);
        if (!ok) val = config->indentationWidth();
    }
    config->configStart();
    config->setIndentationWidth(val);
    if (m_hardAction->isChecked()) config->setTabWidth(val);
    config->configEnd();
}

void KateStatusBar::slotIndentTabMode(QAction *a)
{
    KateDocumentConfig *config = ((KTextEditor::DocumentPrivate *)m_view->document())->config();
    if (a == m_softAction) {
        config->setReplaceTabsDyn(true);
    } else if (a == m_mixedAction) {
        if (config->replaceTabsDyn())
            config->setReplaceTabsDyn(false);
        m_tabGroup->setEnabled(true);
    } else if (a == m_hardAction) {
        if (config->replaceTabsDyn()) {
            config->configStart();
            config->setReplaceTabsDyn(false);
            config->setTabWidth(config->indentationWidth());
            config->configEnd();
        } else {
            config->setTabWidth(config->indentationWidth());
        }
        m_tabGroup->setEnabled(false);
    }
}

void KateStatusBar::toggleWordCount(bool on)
{
    if ((m_wordCounter != nullptr) == on) {
        return;
    }

    if (on) {
        m_wordCounter = new WordCounter(m_view);
        connect(m_wordCounter, &WordCounter::changed, this, &KateStatusBar::wordCountChanged);
    } else {
        delete m_wordCounter;
        m_wordCounter = nullptr;
    }

    wordCountChanged(0, 0, 0, 0);
}

void KateStatusBar::wordCountChanged(int wordsInDocument, int wordsInSelection, int charsInDocument, int charsInSelection)
{
    if (m_wordCounter) {
        m_wordCount = i18n("Words %1/%2, Chars %3/%4", wordsInDocument, wordsInSelection, charsInDocument, charsInSelection);
    } else {
        m_wordCount.clear();
    }

    cursorPositionChanged();
}

void KateStatusBar::configChanged()
{
    toggleWordCount(m_view->config()->showWordCount());
}

void KateStatusBar::changeDictionary(QAction *action)
{
    const QString dictionary = action->data().toString();
    m_dictionary->setText(dictionary);
    // Code stolen from KateDictionaryBar::dictionaryChanged
    KTextEditor::Range selection = m_view->selectionRange();
    if (selection.isValid() && !selection.isEmpty()) {
        m_view->doc()->setDictionary(dictionary, selection);
    } else {
        m_view->doc()->setDefaultDictionary(dictionary);
    }
}
