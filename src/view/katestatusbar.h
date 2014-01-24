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

class KateStatusBarOpenUpMenu: public QMenu
{
        Q_OBJECT
public:
        KateStatusBarOpenUpMenu(QWidget *parent);
        virtual ~KateStatusBarOpenUpMenu();
        virtual void setVisible(bool);
};

class KateStatusBar : public KateViewBarWidget
{
    Q_OBJECT

public:
    explicit KateStatusBar(KTextEditor::ViewPrivate *view);

public Q_SLOTS:
    void updateStatus ();

    void viewModeChanged ();

    void cursorPositionChanged ();

    void selectionChanged ();

    void modifiedChanged();

    void documentConfigChanged ();

    void modeChanged ();

protected:
    bool eventFilter(QObject *obj, QEvent *event) Q_DECL_OVERRIDE;
    
private:
    KTextEditor::ViewPrivate *const m_view;
    QLabel* m_lineColLabel;
    QToolButton* m_modifiedLabel;
    QLabel* m_insertModeLabel;
    QPushButton* m_mode;
    QPushButton* m_encoding;
    QPushButton* m_tabsIndent;
    KLocalizedString m_spacesOnly;
    KLocalizedString m_tabsOnly;
    KLocalizedString m_tabSpacesMixed;
    KLocalizedString m_spacesOnlyShowTabs;
    QMenu *m_indentSettingsMenu;
    unsigned int m_modifiedStatus;
    unsigned int m_selectionMode;
    QActionGroup *m_tabGroup;
    QActionGroup *m_indentGroup;
    QAction *m_mixedAction;
    QAction *m_hardAction;
    QAction *m_softAction;
    void addNumberAction(QActionGroup *group, QMenu *menu,int data);
    void updateGroup(QActionGroup *group, int w);
    
public Q_SLOTS:
    void slotTabGroup(QAction*);
    void slotIndentGroup(QAction*);
    void slotIndentTabMode(QAction*);
};

#endif
