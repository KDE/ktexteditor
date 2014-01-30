/* This file is part of the KDE libraries
   Copyright (C) 2001 Christoph Cullmann <cullmann@kde.org>
   Copyright (C) 2005 Dominik Haumann (dhdev@gmx.de) (documentation)

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#ifndef KDELIBS_KTEXTEDITOR_PLUGIN_H
#define KDELIBS_KTEXTEDITOR_PLUGIN_H

#include <QObject>

#include <ktexteditor_export.h>

namespace KTextEditor
{

class MainWindow;

/**
 * \brief KTextEditor Plugin interface.
 *
 * Topics:
 *  - \ref plugin_intro
 *  - \ref plugin_config
 *  - \ref plugin_config_pages
 *  - \ref plugin_sessions
 *
 * \section plugin_intro Introduction
 *
 * The Plugin class provides methods to create loadable plugins for the
 * KTextEditor framework. The Plugin class itself only has a single function
 * createView() that is called for each MainWindow. In createView(), the
 * plugin is responsible to create tool views through MainWindow::createToolView(),
 * hook itself into the menus and toolbars through KXMLGuiClient, and attach
 * itself to View%s or Document%s.
 *
 * \section plugin_config Configuration Management
 *
 * @todo write docu about config pages (new with kpluginmanager)
 * @todo write docu about save/load settings (related to config page)
 *
 * \section plugin_config_pages Plugin Config Pages
 *
 * If your plugin needs config pages, inherit your plugin also from
 * KTextEditor::ConfigPageInterface like this:
 * \code
 * class MyPlugin : public KTextEditor::Plugin,
 *                  public KTextEditor::ConfigPageInterface
 * {
 *     Q_OBJECT
 *     Q_INTERFACES(KTextEditor::ConfigPageInterface)
 * public:
 *     // ...
 *     virtual int configPages() const Q_DECL_OVERRIDE;
 *     virtual ConfigPage *configPage(int number, QWidget *parent) Q_DECL_OVERRIDE;
 *     virtual QString configPageName(int number) const Q_DECL_OVERRIDE;
 *     virtual QString configPageFullName(int number) const Q_DECL_OVERRIDE;
 *     virtual QIcon configPageIcon(int number) const Q_DECL_OVERRIDE;
 * };
 * \endcode
 * In the implementation, you can then return the amount of config pages in
 * configPages(). The host application will then call configPage() with parameters
 * 0, ..., \e{configPages() - 1}. In addition, implement configPageName(),
 * configPageFullName() and configPageIcon() to get a nice looking entry in the
 * config dialog.
 *
 * \section plugin_sessions Session Management
 *
 * As an extension a Plugin can implement the SessionConfigInterface. This
 * interface provides functions to read and write session related settings.
 * If you have session dependent data additionally derive your Plugin from
 * this interface and implement the session related functions, for example:
 * \code
 * class MyPlugin : public KTextEditor::Plugin,
 *                  public KTextEditor::SessionConfigInterface
 * {
 *     Q_OBJECT
 *     Q_INTERFACES(KTextEditor::SessionConfigInterface)
 * public:
 *     // ...
 *     void readSessionConfig (const KConfigGroup& config) Q_DECL_OVERRIDE;
 *     void writeSessionConfig (KConfigGroup& config) Q_DECL_OVERRIDE;
 * };
 * \endcode
 *
 * \see KTextEditor::Editor, KTextEditor::Document, KTextEditor::View,
 *      KTextEditor::ConfigPageInterface, KTextEditor::SessionConfigInterface
 * \author Christoph Cullmann \<cullmann@kde.org\>
 */
class KTEXTEDITOR_EXPORT Plugin : public QObject
{
    Q_OBJECT

public:
    /**
     * Constructor.
     *
     * Create a new application plugin.
     * \param parent parent object
     */
    Plugin(QObject *parent);

    /**
     * Virtual destructor.
     */
    virtual ~Plugin();

    /**
     * Create a new View for this plugin for the given KTextEditor::MainWindow.
     * This may be called arbitrary often by the application to create as much
     * views as MainWindow%s exist, the application will take care to delete
     * this views whenever a MainWindow is closed, so you do not need to handle
     * deletion of the view yourself in the plugin.
     *
     * \param mainWindow the MainWindow for which a view should be created
     * \return the new created view or NULL
     */
    virtual QObject *createView(KTextEditor::MainWindow *mainWindow) = 0;

private:
    class PluginPrivate *const d;
};

}

#endif
