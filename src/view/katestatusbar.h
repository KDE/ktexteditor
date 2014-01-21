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

#include <QLabel>
#include <QPixmap>
#include <QPushButton>
#include <QStatusBar>

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

    void informationMessage (KTextEditor::View *view, const QString &message);

    void modeChanged ();

protected:
    virtual bool eventFilter (QObject*, QEvent *);

private:
    KTextEditor::ViewPrivate *const m_view;
    QStatusBar *const m_statusBar;
    QLabel* m_lineColLabel;
    QLabel* m_modifiedLabel;
    QLabel* m_insertModeLabel;
    QLabel* m_selectModeLabel;
    QPushButton* m_mode;
    QPushButton* m_encoding;
    KSqueezedTextLabel* m_infoLabel;
    QPixmap m_modPm, m_modDiscPm, m_modmodPm;
};

#endif
