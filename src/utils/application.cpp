/*
 *  This file is part of the KDE project.
 *
 *  Copyright (C) 2013 Christoph Cullmann <cullmann@kde.org>
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

#include <ktexteditor/application.h>
#include <ktexteditor/mainwindow.h>
#include <ktexteditor/plugin.h>

#include "kateglobal.h"

namespace KTextEditor
{

Application::Application(QObject *parent)
    : QObject(parent)
    , d(Q_NULLPTR)
{
}

Application::~Application()
{
}

bool Application::quit()
{
    /**
     * null check
     */
    if (!parent()) {
        return false;
    }

    /**
     * dispatch to parent
     */
    bool success = false;
    QMetaObject::invokeMethod(parent()
                              , "quit"
                              , Qt::DirectConnection
                              , Q_RETURN_ARG(bool, success));

    return success;
}

QList<KTextEditor::MainWindow *> Application::mainWindows()
{
    /**
     * null check
     */
    if (!parent()) {
        return QList<KTextEditor::MainWindow *> ();
    }

    /**
     * dispatch to parent
     */
    QList<KTextEditor::MainWindow *> mainWindow;
    QMetaObject::invokeMethod(parent()
                              , "mainWindows"
                              , Qt::DirectConnection
                              , Q_RETURN_ARG(QList<KTextEditor::MainWindow *>, mainWindow));
    return mainWindow;
}

KTextEditor::MainWindow *Application::activeMainWindow()
{
    /**
     * null check => return dummy main window to have a usable API
     */
    if (!parent()) {
        return KTextEditor::EditorPrivate::self()->dummyMainWindow();
    }

    /**
     * dispatch to parent
     */
    KTextEditor::MainWindow *window = Q_NULLPTR;
    QMetaObject::invokeMethod(parent()
                              , "activeMainWindow"
                              , Qt::DirectConnection
                              , Q_RETURN_ARG(KTextEditor::MainWindow *, window));

    /**
     * always return some kind of window to not need to check for valid pointer
     */
    return window ? window : KTextEditor::EditorPrivate::self()->dummyMainWindow();
}

QList<KTextEditor::Document *> Application::documents()
{
    /**
     * null check
     */
    if (!parent()) {
        return QList<KTextEditor::Document *> ();
    }

    /**
     * dispatch to parent
     */
    QList<KTextEditor::Document *> documents;
    QMetaObject::invokeMethod(parent()
                              , "documents"
                              , Qt::DirectConnection
                              , Q_RETURN_ARG(QList<KTextEditor::Document *>, documents));
    return documents;
}

KTextEditor::Document *Application::findUrl(const QUrl &url)
{
    /**
     * null check
     */
    if (!parent()) {
        return Q_NULLPTR;
    }

    /**
     * dispatch to parent
     */
    KTextEditor::Document *document = Q_NULLPTR;
    QMetaObject::invokeMethod(parent()
                              , "findUrl"
                              , Qt::DirectConnection
                              , Q_RETURN_ARG(KTextEditor::Document *, document)
                              , Q_ARG(const QUrl &, url));
    return document;
}

KTextEditor::Document *Application::openUrl(const QUrl &url, const QString &encoding)
{
    /**
     * null check
     */
    if (!parent()) {
        return Q_NULLPTR;
    }

    /**
     * dispatch to parent
     */
    KTextEditor::Document *document = Q_NULLPTR;
    QMetaObject::invokeMethod(parent()
                              , "openUrl"
                              , Qt::DirectConnection
                              , Q_RETURN_ARG(KTextEditor::Document *, document)
                              , Q_ARG(const QUrl &, url)
                              , Q_ARG(const QString &, encoding));
    return document;
}

bool Application::closeDocument(KTextEditor::Document *document)
{
    /**
     * null check
     */
    if (!parent()) {
        return false;
    }

    /**
     * dispatch to parent
     */
    bool success = false;
    QMetaObject::invokeMethod(parent()
                              , "closeDocument"
                              , Qt::DirectConnection
                              , Q_RETURN_ARG(bool, success)
                              , Q_ARG(KTextEditor::Document *, document));
    return success;
}

bool Application::closeDocuments(const QList<KTextEditor::Document *> &documents)
{
    /**
     * null check
     */
    if (!parent()) {
        return false;
    }

    /**
     * dispatch to parent
     */
    bool success = false;
    QMetaObject::invokeMethod(parent()
                              , "closeDocuments"
                              , Qt::DirectConnection
                              , Q_RETURN_ARG(bool, success)
                              , Q_ARG(const QList<KTextEditor::Document *> &, documents));
    return success;
}

KTextEditor::Plugin *Application::plugin(const QString &name)
{
    /**
     * null check
     */
    if (!parent()) {
        return Q_NULLPTR;
    }

    /**
     * dispatch to parent
     */
    Plugin *plugin = Q_NULLPTR;
    QMetaObject::invokeMethod(parent()
                              , "plugin"
                              , Qt::DirectConnection
                              , Q_RETURN_ARG(Plugin *, plugin)
                              , Q_ARG(const QString &, name));
    return plugin;
}

} // namespace KTextEditor
