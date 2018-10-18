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

#include <ktexteditor/mainwindow.h>
#include <ktexteditor/plugin.h>

#include <KXMLGUIFactory>

namespace KTextEditor
{

MainWindow::MainWindow(QObject *parent)
    : QObject(parent)
    , d(nullptr)
{
}

MainWindow::~MainWindow()
{
}

QWidget *MainWindow::window()
{
    /**
     * dispatch to parent
     */
    QWidget *window = nullptr;
    QMetaObject::invokeMethod(parent()
                              , "window"
                              , Qt::DirectConnection
                              , Q_RETURN_ARG(QWidget*, window));
    return window;
}

KXMLGUIFactory *MainWindow::guiFactory()
{
    /**
     * dispatch to parent
     */
    KXMLGUIFactory *guiFactory = nullptr;
    QMetaObject::invokeMethod(parent()
                              , "guiFactory"
                              , Qt::DirectConnection
                              , Q_RETURN_ARG(KXMLGUIFactory*, guiFactory));
    return guiFactory;
}

QList<KTextEditor::View *> MainWindow::views()
{
    /**
     * dispatch to parent
     */
    QList<KTextEditor::View *> views;
    QMetaObject::invokeMethod(parent()
                              , "views"
                              , Qt::DirectConnection
                              , Q_RETURN_ARG(QList<KTextEditor::View*>, views));
    return views;
}

KTextEditor::View *MainWindow::activeView()
{
    /**
     * dispatch to parent
     */
    KTextEditor::View *view = nullptr;
    QMetaObject::invokeMethod(parent()
                              , "activeView"
                              , Qt::DirectConnection
                              , Q_RETURN_ARG(KTextEditor::View*, view));
    return view;
}

KTextEditor::View *MainWindow::activateView(KTextEditor::Document *document)
{
    /**
     * dispatch to parent
     */
    KTextEditor::View *view = nullptr;
    QMetaObject::invokeMethod(parent()
                              , "activateView"
                              , Qt::DirectConnection
                              , Q_RETURN_ARG(KTextEditor::View*, view)
                              , Q_ARG(KTextEditor::Document*, document));
    return view;
}

KTextEditor::View *MainWindow::openUrl(const QUrl &url, const QString &encoding)
{
    /**
     * dispatch to parent
     */
    KTextEditor::View *view = nullptr;
    QMetaObject::invokeMethod(parent()
                              , "openUrl"
                              , Qt::DirectConnection
                              , Q_RETURN_ARG(KTextEditor::View*, view)
                              , Q_ARG(QUrl, url)
                              , Q_ARG(QString, encoding));
    return view;
}

bool MainWindow::closeView(KTextEditor::View *view)
{
    /**
     * dispatch to parent
     */
    bool success = false;
    QMetaObject::invokeMethod(parent()
                              , "closeView"
                              , Qt::DirectConnection
                              , Q_RETURN_ARG(bool, success)
                              , Q_ARG(KTextEditor::View*, view));
    return success;
}

void MainWindow::splitView(Qt::Orientation orientation)
{
    /**
     * dispatch to parent
     */
    QMetaObject::invokeMethod(parent()
                              , "splitView"
                              , Qt::DirectConnection
                              , Q_ARG(Qt::Orientation, orientation));
}

bool MainWindow::closeSplitView(KTextEditor::View *view)
{
    /**
     * dispatch to parent
     */
    bool success = false;
    QMetaObject::invokeMethod(parent()
                              , "closeSplitView"
                              , Qt::DirectConnection
                              , Q_RETURN_ARG(bool, success)
                              , Q_ARG(KTextEditor::View*, view));
    return success;
}

bool MainWindow::viewsInSameSplitView(KTextEditor::View *view1,
                                      KTextEditor::View *view2)
{
    /**
     * dispatch to parent
     */
    bool success = false;
    QMetaObject::invokeMethod(parent()
                              , "viewsInSameSplitView"
                              , Qt::DirectConnection
                              , Q_RETURN_ARG(bool, success)
                              , Q_ARG(KTextEditor::View*, view1)
                              , Q_ARG(KTextEditor::View*, view2));
    return success;
}

QWidget *MainWindow::createViewBar(KTextEditor::View *view)
{
    /**
     * dispatch to parent
     */
    QWidget *viewBar = nullptr;
    QMetaObject::invokeMethod(parent()
                              , "createViewBar"
                              , Qt::DirectConnection
                              , Q_RETURN_ARG(QWidget*, viewBar)
                              , Q_ARG(KTextEditor::View*, view));
    return viewBar;
}

void MainWindow::deleteViewBar(KTextEditor::View *view)
{
    /**
     * dispatch to parent
     */
    QMetaObject::invokeMethod(parent()
                              , "deleteViewBar"
                              , Qt::DirectConnection
                              , Q_ARG(KTextEditor::View*, view));
}

void MainWindow::addWidgetToViewBar(KTextEditor::View *view, QWidget *bar)
{
    /**
     * dispatch to parent
     */
    QMetaObject::invokeMethod(parent()
                              , "addWidgetToViewBar"
                              , Qt::DirectConnection
                              , Q_ARG(KTextEditor::View*, view)
                              , Q_ARG(QWidget*, bar));
}

void MainWindow::showViewBar(KTextEditor::View *view)
{
    /**
     * dispatch to parent
     */
    QMetaObject::invokeMethod(parent()
                              , "showViewBar"
                              , Qt::DirectConnection
                              , Q_ARG(KTextEditor::View*, view));
}

void MainWindow::hideViewBar(KTextEditor::View *view)
{
    /**
     * dispatch to parent
     */
    QMetaObject::invokeMethod(parent()
                              , "hideViewBar"
                              , Qt::DirectConnection
                              , Q_ARG(KTextEditor::View*, view));
}

QWidget *MainWindow::createToolView(KTextEditor::Plugin *plugin, const QString &identifier, KTextEditor::MainWindow::ToolViewPosition pos, const QIcon &icon, const QString &text)
{
    /**
     * dispatch to parent
     */
    QWidget *toolView = nullptr;
    QMetaObject::invokeMethod(parent()
                              , "createToolView"
                              , Qt::DirectConnection
                              , Q_RETURN_ARG(QWidget*, toolView)
                              , Q_ARG(KTextEditor::Plugin*, plugin)
                              , Q_ARG(QString, identifier)
                              , Q_ARG(KTextEditor::MainWindow::ToolViewPosition, pos)
                              , Q_ARG(QIcon, icon)
                              , Q_ARG(QString, text));
    return toolView;
}

bool MainWindow::moveToolView(QWidget *widget, KTextEditor::MainWindow::ToolViewPosition pos)
{
    /**
     * dispatch to parent
     */
    bool success = false;
    QMetaObject::invokeMethod(parent()
                              , "moveToolView"
                              , Qt::DirectConnection
                              , Q_RETURN_ARG(bool, success)
                              , Q_ARG(QWidget*, widget)
                              , Q_ARG(KTextEditor::MainWindow::ToolViewPosition, pos));
    return success;
}

bool MainWindow::showToolView(QWidget *widget)
{
    /**
     * dispatch to parent
     */
    bool success = false;
    QMetaObject::invokeMethod(parent()
                              , "showToolView"
                              , Qt::DirectConnection
                              , Q_RETURN_ARG(bool, success)
                              , Q_ARG(QWidget*, widget));
    return success;
}

bool MainWindow::hideToolView(QWidget *widget)
{
    /**
     * dispatch to parent
     */
    bool success = false;
    QMetaObject::invokeMethod(parent()
                              , "hideToolView"
                              , Qt::DirectConnection
                              , Q_RETURN_ARG(bool, success)
                              , Q_ARG(QWidget*, widget));
    return success;
}

QObject *MainWindow::pluginView(const QString &name)
{
    /**
     * dispatch to parent
     */
    QObject *pluginView = nullptr;
    QMetaObject::invokeMethod(parent()
                              , "pluginView"
                              , Qt::DirectConnection
                              , Q_RETURN_ARG(QObject*, pluginView)
                              , Q_ARG(QString, name));
    return pluginView;
}

} // namespace KTextEditor
