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

#ifndef KATE_STATUS_BAR_H
#define KATE_STATUS_BAR_H

#include "kateview.h"
#include "kateviewhelpers.h"

#include <KSqueezedTextLabel>
#include <KLocalizedString>

#include <QLabel>
#include <QPushButton>
#include <QToolButton>

class WordCounter;

class KateStatusBarOpenUpMenu: public QMenu
{
    Q_OBJECT
public:
    explicit KateStatusBarOpenUpMenu(QWidget *parent);
    ~KateStatusBarOpenUpMenu() override;
    void setVisible(bool) override;
};

/**
 * For convenience an own button class to ensure a unified look&feel.
 * Should someone dislike the QPushButton at all could he change it
 * to a e.g. QLabel subclass
 */
class StatusBarButton: public QPushButton
{
    Q_OBJECT
public:
    explicit StatusBarButton(KateStatusBar *parent, const QString &text = QString());
    ~StatusBarButton() override;
};

class KateStatusBar : public KateViewBarWidget
{
    Q_OBJECT
    friend class StatusBarButton;

public:
    explicit KateStatusBar(KTextEditor::ViewPrivate *view);

public Q_SLOTS:
    void updateStatus();

    void viewModeChanged();

    void cursorPositionChanged();

    void updateDictionary();

    void selectionChanged();

    void modifiedChanged();

    void documentConfigChanged();

    void modeChanged();

    void wordCountChanged(int, int, int, int);

    void toggleWordCount(bool on);

    void configChanged();

    void changeDictionary(QAction *action);

protected:
    bool eventFilter(QObject *obj, QEvent *event) override;
    void contextMenuEvent(QContextMenuEvent *event) override;

private:
    KTextEditor::ViewPrivate *const m_view;
    StatusBarButton *m_cursorPosition = nullptr;
    QString m_wordCount;
    StatusBarButton *m_modified = nullptr;
    StatusBarButton *m_inputMode = nullptr;
    StatusBarButton *m_mode = nullptr;
    StatusBarButton *m_encoding = nullptr;
    StatusBarButton *m_tabsIndent = nullptr;
    StatusBarButton *m_dictionary = nullptr;
    QActionGroup *m_dictionaryGroup = nullptr;
    KateStatusBarOpenUpMenu *m_dictionaryMenu = nullptr;
    QMenu *m_indentSettingsMenu;
    unsigned int m_modifiedStatus;
    unsigned int m_selectionMode;
    QActionGroup *m_tabGroup;
    QActionGroup *m_indentGroup;
    QAction *m_mixedAction;
    QAction *m_hardAction;
    QAction *m_softAction;
    WordCounter *m_wordCounter;

private:
    void addNumberAction(QActionGroup *group, QMenu *menu, int data);
    void updateGroup(QActionGroup *group, int w);

public Q_SLOTS:
    void slotTabGroup(QAction *);
    void slotIndentGroup(QAction *);
    void slotIndentTabMode(QAction *);
    void toggleShowLines(bool checked);
    void toggleShowWords(bool checked);
};

#endif
