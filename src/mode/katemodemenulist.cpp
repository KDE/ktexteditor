/*  This file is part of the KDE libraries and the Kate part.
 *
 *  Copyright (C) 2019 Nibaldo González S. <nibgonz@gmail.com>
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

//BEGIN Includes
#include "katemodemenulist.h"

#include "katedocument.h"
#include "kateconfig.h"
#include "kateview.h"
#include "kateglobal.h"
#include "katesyntaxmanager.h"
#include "katepartdebug.h"

#include <QWidgetAction>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFrame>
//END Includes


namespace
{
/**
 * Detect words delimiters:
 *      ! " # $ % & ' ( ) * + , - . / : ;
 *      < = > ? [ \ ] ^ ` { | } ~ « »
 */
static bool isDelimiter(const ushort c)
{
    return (c <= 126 && c >= 33 && (c >= 123 || c <= 47 ||
           (c <= 96 && c >= 58 && c != 95 && (c >= 91 || c <= 63)))) ||
           c == 171 || c == 187;
}
}


void KateModeMenuList::init(const SearchBarPosition searchBarPos)
{
    /*
     * Load list widget, scroll bar and items.
     */
    m_list = new ModeListWidget(this);
    m_scroll = new QScrollBar(Qt::Vertical, this);
    m_list->setVerticalScrollBar(m_scroll);

    // The vertical scroll bar will be added in another layout
    m_list->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_list->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_list->setIconSize(QSize(m_iconSize, m_iconSize));
    m_list->setResizeMode(QListView::Adjust);
    // Initial size of the list widget, this can be modified later
    m_list->setSizeList(428);

    loadHighlightingList();

    // Add scroll bar and set margin
    QHBoxLayout *layoutScrollBar = new QHBoxLayout();
    layoutScrollBar->addWidget(m_scroll);
    layoutScrollBar->setMargin(2);

    /*
     * Search bar widget.
     */
    m_searchBar = new ModeLineEdit(this, m_list);
    m_searchBar->setCaseSensitivity(Qt::CaseInsensitive);
    m_searchBar->setPlaceholderText(i18nc("Placeholder in search bar", "Search..."));

    m_list->setFocusProxy(m_searchBar);

    /*
     * Set layouts and widgets.
     */
    QWidget *container = new QWidget();
    QVBoxLayout *layoutContainer = new QVBoxLayout();
    m_layoutList = new QGridLayout();
    QHBoxLayout *layoutSearchBar = new QHBoxLayout();

    // Ooverlap scroll bar above the list widget
    m_layoutList->addWidget(m_list, 0, 0, Qt::AlignLeft);
    m_layoutList->addLayout(layoutScrollBar, 0, 0, Qt::AlignRight);

    layoutSearchBar->addWidget(m_searchBar);

    if (searchBarPos == Top) {
        layoutContainer->addLayout(layoutSearchBar);
    }
    layoutContainer->addLayout(m_layoutList);
    if (searchBarPos == Bottom) {
        layoutContainer->addLayout(layoutSearchBar);
    }
    container->setLayout(layoutContainer);

    QWidgetAction *widAct = new QWidgetAction(this);
    widAct->setDefaultWidget(container);
    this->addAction(widAct);

    // Detect selected item with one click.
    // This also applies to double-clicks.
    connect(m_list, &KateModeMenuList::ModeListWidget::itemClicked,
            this, &KateModeMenuList::selectHighlighting);

    return;
}


void KateModeMenuList::loadHighlightingList()
{
    QString *prevHlSection = nullptr;
    /*
     * The width of the text container in the item, in pixels. This is used to make
     * a custom word wrap and prevent the item's text from passing under the scroll bar.
     * NOTE: 12 = the edges
     */
    const int maxWidthText = m_list->sizeHint().width() - m_scroll->sizeHint().width() - m_iconSize - 12;

    /*
     * Get list of modes from KateModeManager::list().
     * We assume that the modes are arranged according to sections, alphabetically;
     * and the attribute "translatedSection" isn't empty if "section" has a value.
     */
    for (auto *hl : KTextEditor::EditorPrivate::self()->modeManager()->list()) {
        /*
         * Detects a new section.
         */
        if ( !hl->translatedSection.isEmpty() && (prevHlSection == nullptr || hl->translatedSection != *prevHlSection) ) {
            QPixmap transparent = QPixmap(m_iconSize / 2, m_iconSize / 2);
            transparent.fill(Qt::transparent);

            /*
             * Add a separator to the list.
             */
            ModeListWidgetItem *separator = new ModeListWidgetItem();
            separator->setFlags(Qt::NoItemFlags);
            separator->setSizeHint(QSize(separator->sizeHint().width() - 4, 4));
            separator->setBackground(QBrush(transparent));

            QFrame *line = new QFrame();
            line->setFrameStyle(QFrame::HLine);

            m_list->addItem(separator);
            m_list->setItemWidget(separator, line);

            /*
             * Add the section name to the list.
             */
            ModeListWidgetItem *section = new ModeListWidgetItem();
            section->setFlags(Qt::NoItemFlags);

            QLabel *label = new QLabel(hl->sectionTranslated());
            if (m_list->layoutDirection() == Qt::RightToLeft) {
                label->setAlignment(Qt::AlignRight);
            }
            label->setTextFormat(Qt::RichText);
            label->setIndent(6);

            // NOTE: Names of sections in bold. The font color
            // should change according to Kate's color theme.
            QFont font = label->font();
            font.setWeight(QFont::Bold);
            label->setFont(font);

            section->setBackground(QBrush(transparent));
            m_list->addItem(section);
            m_list->setItemWidget(section, label);

            m_list->addItem(section);
        }
        prevHlSection = hl->translatedSection.isNull() ? nullptr : &hl->translatedSection;

        /*
         * Create item in the list with the language name.
         */
        ModeListWidgetItem *item = new ModeListWidgetItem();
        item->setText(setWordWrap( hl->nameTranslated(), maxWidthText, m_list->fontMetrics() ));
        item->setMode(hl);
        //item->generateSearchName( item->getMode()->translatedName.isEmpty() ? &item->getMode()->name : &item->getMode()->translatedName );

        m_list->addDefaultItem(item);
    }
    return;
}


void KateModeMenuList::setButton(QPushButton* button, const bool bAutoUpdateTextButton, AlignmentButton position)
{
    if (position == Inverse) {
        if (layoutDirection() == Qt::RightToLeft) {
            m_position = KateModeMenuList::Left;
        } else {
            m_position = KateModeMenuList::Right;
        }
    } else if (position == Left && layoutDirection() != Qt::RightToLeft) {
        m_position = KateModeMenuList::Default;
    } else {
        m_position = position;
    }

    m_pushButton = button;
    m_bAutoUpdateTextButton = bAutoUpdateTextButton;
    return;
}


void KateModeMenuList::showEvent(QShowEvent* event)
{
    Q_UNUSED(event);

    // Set the menu position
    if (m_pushButton && m_pushButton->isVisible()) {
        if (m_position == Right) {
            // New menu position
            int newMenu_x = pos().x() - geometry().width() + m_pushButton->geometry().width();
            // Get position of the right edge of the toggle button
            const int buttonPositionRight = m_pushButton->mapToGlobal(QPoint(0, 0)).x()
                                            + m_pushButton->geometry().width();
            if (newMenu_x < 0) {
                newMenu_x = 0;
            } else if ( newMenu_x + geometry().width() < buttonPositionRight ) {
                newMenu_x = buttonPositionRight - geometry().width();
            }
            move(newMenu_x, pos().y());
        }
        else if (m_position == Left) {
            move(m_pushButton->mapToGlobal(QPoint(0, 0)).x(), pos().y());
        }
    }

    /* TODO: Add keyboard shortcut to show the menu. Put the menu
     *       in the center of the window if the status bar is hidden.
     */

    // Select text from the search bar
    if (!m_searchBar->text().isEmpty()) {
        if (m_searchBar->text().simplified().isEmpty()) {
            m_searchBar->clear();
        } else {
            m_searchBar->selectAll();
        }
    }

    // Set focus on the list. The list widget uses focus proxy to the search bar.
    m_list->setFocus(Qt::ActiveWindowFocusReason);

    KTextEditor::DocumentPrivate *doc = m_doc;

    // First show
    if (!m_selectedItem) {
        /*
         * Generate search names in items.
         * This could be in "loadHighlightingList()".
         */
        for (int i = 0; i < m_list->count(); ++i) {
            ModeListWidgetItem *item = static_cast<ModeListWidgetItem *>(m_list->item(i));
            if (item->hasMode() && !item->getSearchName()) {
                item->generateSearchName( item->getMode()->translatedName.isEmpty() ? &item->getMode()->name : &item->getMode()->translatedName );
            }
        }
        if (doc) {
            selectHighlightingFromExternal(doc->fileType());
        }
    } else if ( doc && m_selectedItem->hasMode() && m_selectedItem->getMode()->name != doc->fileType() ) {
        selectHighlightingFromExternal(doc->fileType());
    }
    return;
}


void KateModeMenuList::updateSelectedItem(ModeListWidgetItem *item)
{
    // Change the previously selected item to empty icon
    if (m_selectedItem) {
        QPixmap emptyIcon(m_iconSize, m_iconSize);
        emptyIcon.fill(Qt::transparent);
        m_selectedItem->setIcon(QIcon(emptyIcon));
    }

    // Update the selected item
    item->setIcon(m_checkIcon);
    m_selectedItem = item;
    m_list->setCurrentItem(item, QItemSelectionModel::ClearAndSelect);

    // Change text of the trigger button
    if (m_bAutoUpdateTextButton && m_pushButton && item->hasMode()) {
        m_pushButton->setText(item->getMode()->nameTranslated());
    }
    return;
}


void KateModeMenuList::selectHighlightingSetVisibility(QListWidgetItem *pItem, const bool bHideMenu)
{
    ModeListWidgetItem *item = static_cast<ModeListWidgetItem *>(pItem);

    updateSelectedItem(item);

    // Hide menu
    if (bHideMenu) {
        this->hide();
    }

    // Apply syntax highlighting
    KTextEditor::DocumentPrivate *doc = m_doc;
    if (doc && item->hasMode()) {
        doc->updateFileType(item->getMode()->name, true);
    }
    return;
}


void KateModeMenuList::selectHighlighting(QListWidgetItem *pItem)
{
    selectHighlightingSetVisibility(pItem, true);
    return;
}


void KateModeMenuList::selectHighlightingFromExternal(const QString &nameMode)
{
    for (int i = 0; i < m_list->count(); ++i) {
        ModeListWidgetItem *item = static_cast<ModeListWidgetItem *>( m_list->item(i) );

        if (!item->hasMode() || m_list->item(i)->text().isEmpty()) {
            continue;
        }
        if (item->getMode()->name == nameMode || ( nameMode.isEmpty() && item->getMode()->name == QLatin1String("Normal") )) {
            updateSelectedItem(item);

            // Clear search
            if (!m_searchBar->text().isEmpty()) {
                // Prevent the empty list message from being seen over the items for a short time
                if (m_emptyListMsg) {
                    m_emptyListMsg->hide();
                }

                // NOTE: This calls updateSearch(), it's scrolled to the selected item
                m_searchBar->clear();
            } else {
                m_list->scrollToItem(item, QAbstractItemView::PositionAtCenter);
            }
            return;
        }
    }
    return;
}


void KateModeMenuList::selectHighlightingFromExternal()
{
    KTextEditor::DocumentPrivate *doc = m_doc;
    if (doc) {
        selectHighlightingFromExternal(doc->fileType());
    }
    return;
}


void KateModeMenuList::loadEmptyMsg()
{
    m_emptyListMsg = new QLabel(i18nc("A search yielded no results", "No items matching your search"));
    m_emptyListMsg->setMargin(15);
    m_emptyListMsg->setWordWrap(true);

    QColor color = m_emptyListMsg->palette().color(QPalette::Text);
    m_emptyListMsg->setStyleSheet( QLatin1String("font-size: 14pt; color: rgba(") + QString::number(color.red()) + QLatin1Char(',') + QString::number(color.green()) + QLatin1Char(',') + QString::number(color.blue()) + QLatin1String(", 0.3);") );

    m_emptyListMsg->setAlignment(Qt::AlignCenter);
    m_layoutList->addWidget(m_emptyListMsg, 0, 0, Qt::AlignCenter);
    return;
}


QString KateModeMenuList::setWordWrap(const QString &text, const int maxWidth, const QFontMetrics &fontMetrics) const
{
    // Get the length of the text, in pixels, and compare it with the container
    if (fontMetrics.boundingRect(text).width() <= maxWidth) {
        return text;
    }

    // Add line breaks in the text to fit in the container
    QStringList words = text.split(QLatin1Char(' '));
    if (words.count() < 1) {
        return text;
    }
    QString newText = QString();
    QString tmpLineText = QString();

    for (int i = 0; i < words.count() - 1; ++i) {
        tmpLineText += words[i];

        // This prevents the last line of text from having only one word with 1 or 2 chars
        if ( i == words.count() - 3 && words[i + 2].length() <= 2 &&
            fontMetrics.boundingRect( tmpLineText + QLatin1Char(' ') + words[i + 1] + QLatin1Char(' ') + words[i + 2] ).width() > maxWidth ) {
            newText += tmpLineText + QLatin1Char('\n');
            tmpLineText.clear();
        }
        // Add line break if the maxWidth is exceeded with the next word
        else if ( fontMetrics.boundingRect( tmpLineText + QLatin1Char(' ') + words[i + 1] ).width() > maxWidth ) {
            newText += tmpLineText + QLatin1Char('\n');
            tmpLineText.clear();
        }
        else {
            tmpLineText.append(QLatin1Char(' '));
        }
    }

    // Add line breaks in delimiters, if the last word is greater than the container
    if (fontMetrics.boundingRect( words[words.count() - 1] ).width() > maxWidth) {
        const int lastw = words.count() - 1;
        for (int c = words[lastw].length() - 1; c >= 0; --c) {
            if (isDelimiter(words[lastw][c].unicode()) && fontMetrics.boundingRect( words[lastw].mid(0, c + 1) ).width() <= maxWidth) {
                words[lastw].insert(c + 1, QLatin1Char('\n'));
                break;
            }
        }
    }

    if (!tmpLineText.isEmpty()) {
        newText += tmpLineText;
    }
    newText += words[words.count() - 1];
    return newText;
}


void KateModeMenuList::ModeListWidget::addDefaultItem(QListWidgetItem *item)
{
    // Set empty icon
    QPixmap emptyIcon(m_parentMenu->m_iconSize, m_parentMenu->m_iconSize);
    emptyIcon.fill(Qt::transparent);
    item->setIcon(QIcon(emptyIcon));
    // Add item
    this->addItem(item);
    return;
}


void KateModeMenuList::ModeListWidget::setSizeList(const int height, const int width)
{
    this->setMinimumWidth(width);
    this->setMaximumWidth(width);
    this->setMinimumHeight(height);
    this->setMaximumHeight(height);
    return;
}


void KateModeMenuList::ModeListWidget::keyPressEvent(QKeyEvent *event)
{
    // Ctrl/Alt/Shift/Meta + Return selects an item, but without hiding the menu
    if (( event->key() == Qt::Key_Enter || event->key() == Qt::Key_Return ) &&
        ( event->modifiers().testFlag(Qt::ControlModifier) || event->modifiers().testFlag(Qt::AltModifier) ||
          event->modifiers().testFlag(Qt::ShiftModifier) || event->modifiers().testFlag(Qt::MetaModifier) )) {
        m_parentMenu->selectHighlightingSetVisibility(currentItem(), false);
    }
    // Return/Enter selects an item and hide the menu
    else if (event->key() == Qt::Key_Return || event->key() == Qt::Key_Enter) {
        m_parentMenu->selectHighlightingSetVisibility(currentItem(), true);
    } else {
        QListWidget::keyPressEvent(event);
    }
    return;
}


void KateModeMenuList::ModeLineEdit::clear()
{
    m_bSearchStateClear = false;
    m_bSearchStateAutoScroll = ( text().trimmed().isEmpty() ) ? false : true;
    KListWidgetSearchLine::clear();
    return;
}


inline void KateModeMenuList::ModeLineEdit::updateSearch(const QString &s)
{
    if (m_parentMenu->m_emptyListMsg) {
        m_parentMenu->m_emptyListMsg->hide();
    }
    if (m_parentMenu->m_scroll->isHidden()) {
        m_parentMenu->m_scroll->show();
    }

    KateModeMenuList::ModeListWidget *listWidget = m_parentMenu->m_list;

    const QString searchText = (s.isNull() ? text() : s).simplified();

    /*
     * Empty search bar.
     * Show all items and scroll to the selected item.
     */
    if ( searchText.isEmpty() || (searchText.size() == 1 && searchText[0].isSpace()) ) {
        // This indicates that the search has already been cleared by clear() and
        // that all the items are already visible. Also see: KListWidgetSearchLine::clear()
        if (m_bSearchStateClear) {
            for (int i = 0; i < listWidget->count(); ++i) {
                if (listWidget->item(i)->isHidden()) {
                    listWidget->item(i)->setHidden(false);
                }
            }
        }

        // Don't auto-scroll if the search is already clear
        if (m_bSearchStateAutoScroll) {
            listWidget->setCurrentItem(m_parentMenu->m_selectedItem);
            listWidget->scrollToItem(m_parentMenu->m_selectedItem, QAbstractItemView::PositionAtCenter);
        }
        m_bSearchStateClear = true;
        m_bSearchStateAutoScroll = false;
        return;
    }

    /*
     * Prepare item filter.
     */
    int lastItem = -1;
    int lastSection = -1;
    bool bEmptySection = true;
    bool bSectionSeparator = false;
    bool bSectionName = false;
    bool bSearchExtensions = true;
    bool bExactMatch = false; // If the search name will not be used
    /*
     * It's used for two purposes, it's true if searchText is a
     * single alphanumeric character or if it starts with a point.
     * Both cases don't conflict, so a single bool is used.
     */
    bool bIsAlphaOrPointExt = false;

    /*
     * Don't search for extensions if the search text has only one character,
     * to avoid unwanted results. In this case, the items that start with
     * that character are displayed.
     */
    if (searchText.length() < 2) {
        bSearchExtensions = false;
        if (searchText[0].isLetterOrNumber()) {
            bIsAlphaOrPointExt = true;
        }
    }
    // If the search text has a point at the beginning, match extensions
    else if (searchText.length() > 1 && searchText[0].toLatin1() == 46) {
        bIsAlphaOrPointExt = true;
        bSearchExtensions = true;
        bExactMatch = true;
    }
    // Two characters: search using the normal name of the items
    else if (searchText.length() == 2) {
        bExactMatch = true;
        // if it contains the '*' character, don't match extensions
        if (searchText[1].toLatin1() == 42 || searchText[0].toLatin1() == 42) {
            bSearchExtensions = false;
        }
    }
    /*
     * Don't use the search name if the search text has delimiters.
     * Don't search in extensions if it contains the '*' character.
     */
    else {
        for (const auto &cc : searchText) {
            const ushort c = cc.unicode();
            if (c == 42) {
                bSearchExtensions = false;
                bExactMatch = true;
                break;
            }
            if (!bExactMatch && isDelimiter(c)) {
                bExactMatch = true;
            }
        }
    }

    /*
     * Filter items.
     */
    for (int i = 0; i < listWidget->count(); ++i) {
        QString itemName = listWidget->item(i)->text();

        /*
         * Hide/show the name of the section. If the text of the item
         * is empty, then it corresponds to the name of the section.
         */
        if (itemName.isEmpty()) {
            listWidget->item(i)->setHidden(false);

            if (bSectionSeparator) {
                bSectionName = true;
            } else {
                bSectionSeparator = true;
            }

            /*
             * This hides the name of the previous section
             * (and the separator) if this section has no items.
             */
            if (bSectionName && bEmptySection && lastSection > 0) {
                listWidget->item(lastSection)->setHidden(true);
                listWidget->item(lastSection - 1)->setHidden(true);
            }

            // Find the section name
            if (bSectionName) {
                bSectionName = false;
                bSectionSeparator = false;
                bEmptySection = true;
                lastSection = i;
            }
            continue;
        }

        /*
         * Start filtering items.
         */
        ModeListWidgetItem *item = static_cast<ModeListWidgetItem *>(listWidget->item(i));

        // Only a character is written in the search bar
        if (searchText.length() == 1) {
            if (bIsAlphaOrPointExt) {
                // CASE 1: All the items that start with that character will be displayed.
                if (item->getSearchName()->startsWith(searchText, Qt::CaseInsensitive) ) {
                    setSearchResult(i, bEmptySection, lastSection, lastItem);
                    continue;
                }

                // CASE 2: Matches considering delimiters. For example, when writing "c",
                //         "Objective-C" will be displayed in the results, but not "Yacc/Bison".
                if (QString( QLatin1Char(' ') + *(item->getSearchName()) + QLatin1Char(' ') ).contains( QLatin1Char(' ') + searchText + QLatin1Char(' '), Qt::CaseInsensitive )) {
                    setSearchResult(i, bEmptySection, lastSection, lastItem);
                    continue;
                }
            }
            // CASE 3: The character isn't a letter or number, do an exact search.
            else if ( item->getMode()->nameTranslated().contains(searchText[0], Qt::CaseInsensitive) ) {
                setSearchResult(i, bEmptySection, lastSection, lastItem);
                continue;
            }
        }
        // CASE 4: Search text, using the search name or the normal name.
        else if (!bExactMatch && item->getSearchName()->contains(searchText, Qt::CaseInsensitive)) {
            setSearchResult(i, bEmptySection, lastSection, lastItem);
            continue;
        }
        else if (bExactMatch && item->getMode()->nameTranslated().contains(searchText, Qt::CaseInsensitive)) {
            setSearchResult(i, bEmptySection, lastSection, lastItem);
            continue;
        }

        // CASE 5: Exact matches in extensions.
        if (bSearchExtensions) {
            if (bIsAlphaOrPointExt && item->matchExtension(searchText.mid(1))) {
                setSearchResult(i, bEmptySection, lastSection, lastItem);
                continue;
            }
            else if (item->matchExtension(searchText)) {
                setSearchResult(i, bEmptySection, lastSection, lastItem);
                continue;
            }
        }

        // Item not found, hide
        listWidget->item(i)->setHidden(true);
    }

    // Remove last section name, if it's empty.
    if ( bEmptySection && lastSection > 0 && !listWidget->item( listWidget->count() - 1 )->text().isEmpty() ) {
        listWidget->item(lastSection)->setHidden(true);
        listWidget->item(lastSection - 1)->setHidden(true);
    }

    listWidget->scrollToTop();

    // Show message of empty list
    if (lastItem == -1) {
        if (m_parentMenu->m_emptyListMsg == nullptr) {
            m_parentMenu->loadEmptyMsg();
        }
        m_parentMenu->m_scroll->hide();
        m_parentMenu->m_emptyListMsg->show();
    }
    // Hide scroll bar if it isn't necessary
    else if ( listWidget->visualItemRect( listWidget->item(lastItem) ).bottom() <= listWidget->geometry().height() ) {
        m_parentMenu->m_scroll->hide();
    }

    m_bSearchStateClear = true;
    m_bSearchStateAutoScroll = true;
    return;
}


void KateModeMenuList::ModeLineEdit::setSearchResult(const int rowItem, bool &bEmptySection, int &lastSection, int &lastItem)
{
    if (lastItem == -1) {
        /*
         * Detect the first result of the search and "select" it.
         * This allows you to scroll through the list using
         * the Up/Down keys after entering a search.
         */
        m_parentMenu->m_list->setCurrentItem( m_parentMenu->m_list->item(rowItem) );
        /*
         * This avoids showing the separator line in the name
         * of the first section, in the search results.
         */
        if (lastSection > 0) {
            m_parentMenu->m_list->item(lastSection - 1)->setHidden(true);
        }
    }
    if (bEmptySection) {
        bEmptySection = false;
    }

    lastItem = rowItem;
    if ( m_parentMenu->m_list->item(rowItem)->isHidden() ) {
        m_parentMenu->m_list->item(rowItem)->setHidden(false);
    }
    return;
}


bool KateModeMenuList::ModeListWidgetItem::generateSearchName(const QString *itemName)
{
    QString searchName = QString(*itemName);
    bool bNewName = false;

    // Replace word delimiters with spaces
    for (int i = searchName.length() - 1; i >= 0; --i) {
        if (isDelimiter( searchName[i].unicode() )) {
            searchName.replace(i, 1, QLatin1Char(' '));
            if (!bNewName) {
                bNewName = true;
            }
        }
        // Avoid duplicate delimiters/spaces
        if (bNewName && i < searchName.length() - 1 && searchName[i].isSpace() && searchName[i + 1].isSpace()) {
            searchName.remove(i + 1, 1);
        }
    }

    if (bNewName) {
        if (searchName[searchName.length() - 1].isSpace()) {
            searchName.remove(searchName.length() - 1, 1);
        }
        if (searchName[0].isSpace()) {
            searchName.remove(0, 1);
        }
        m_searchName = new QString(searchName);
        return true;
    } else {
        m_searchName = itemName;
    }
    return false;
}


bool KateModeMenuList::ModeListWidgetItem::matchExtension(const QString &text)
{
    if (!hasMode() || m_type->wildcards.count() == 0) {
        return false;
    }

    /*
     * Only file extensions and full names are matched. Files like "Kconfig*"
     * aren't considered. It's also assumed that "text" doesn't contain '*'.
     */
    for (auto &ext : m_type->wildcards) {
        // File extension
        if (ext.startsWith(QLatin1String("*."))) {
            if (text.length() == ext.length() - 2 && text.compare(ext.mid(2), Qt::CaseInsensitive) == 0) {
                return true;
            }
        } else if (text.length() != ext.length() || ext.endsWith(QLatin1Char('*'))) {
            continue;
        // Full name
        } else if (text.compare(&ext, Qt::CaseInsensitive) == 0) {
            return true;
        }
    }
    return false;
}


void KateModeMenuList::updateMenu(KTextEditor::Document *doc)
{
    m_doc = static_cast<KTextEditor::DocumentPrivate *>(doc);
}
