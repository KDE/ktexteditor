/*
 * This file is part of the KDE libraries
 *
 * Copyright (C) 2014 Miquel Sabaté Solà <mikisabate@gmail.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 * You should have received a copy of the GNU Library General Public License
 * along with this library; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */


#ifndef KEYS_TEST_H
#define KEYS_TEST_H


#include "base.h"


/**
 * This class is used by the VimStyleCommandBarTestsSetUpAndTearDown class so
 * the main window is active all the time.
 */
class WindowKeepActive : public QObject
{
    Q_OBJECT

public:
    WindowKeepActive(QMainWindow *mainWindow);

public Q_SLOTS:
    bool eventFilter(QObject *object, QEvent *event);

private:
    QMainWindow *m_mainWindow;
};

/**
 * Helper class that is used to setup and tear down tests affecting
 * the command bar in any way.
 */
class VimStyleCommandBarTestsSetUpAndTearDown
{
public:
    VimStyleCommandBarTestsSetUpAndTearDown(KateViInputMode *inputMode,
                                            KTextEditor::ViewPrivate *view,
                                            QMainWindow *window);

    ~VimStyleCommandBarTestsSetUpAndTearDown();

private:
    KTextEditor::ViewPrivate *m_view;
    QMainWindow *m_window;
    WindowKeepActive m_windowKeepActive;
    KateViInputMode *m_viInputMode;
};


class KeysTest : public BaseTest
{
    Q_OBJECT

private Q_SLOTS:
    void MappingTests();
    void LeaderTests();
    void ParsingTests();
    void AltGr();
    void MacroTests();
    void MarkTests();
};


#endif /* KEYS_TEST_H */
