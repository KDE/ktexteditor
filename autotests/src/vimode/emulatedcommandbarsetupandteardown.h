/*  This file is part of the KDE libraries and the Kate part.
 *
 *  Copyright (C) 2013-2016 Simon St James <kdedevel@etotheipiplusone.com>
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

#ifndef EMULATEDCOMMANDBARSETUPANDTEARDOWN_H
#define EMULATEDCOMMANDBARSETUPANDTEARDOWN_H

#include <QObject>

namespace KTextEditor
{
    class ViewPrivate;
}

class KateViInputMode;
class QMainWindow;

/**
 * This class is used by the EmulatedCommandBarSetUpAndTearDown class so
 * the main window is active all the time.
 */
class WindowKeepActive : public QObject
{
    Q_OBJECT

    public:
        explicit WindowKeepActive(QMainWindow *mainWindow);

        public Q_SLOTS:
            bool eventFilter(QObject *object, QEvent *event) override;

    private:
        QMainWindow *m_mainWindow;
};

/**
 * Helper class that is used to setup and tear down tests affecting
 * the command bar in any way.
 */
class EmulatedCommandBarSetUpAndTearDown
{
    public:
        EmulatedCommandBarSetUpAndTearDown(KateViInputMode *inputMode,
                KTextEditor::ViewPrivate *view,
                QMainWindow *window);

        ~EmulatedCommandBarSetUpAndTearDown();

    private:
        KTextEditor::ViewPrivate *m_view;
        QMainWindow *m_window;
        WindowKeepActive m_windowKeepActive;
        KateViInputMode *m_viInputMode;
};

#endif
