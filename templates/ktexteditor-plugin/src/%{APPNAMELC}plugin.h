/*
 *   Copyright (C) %{CURRENT_YEAR} by %{AUTHOR} <%{EMAIL}>
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 2 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program; if not, write to the
 *   Free Software Foundation, Inc.,
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA .
 */

#ifndef %{APPNAMEUC}PLUGIN_H
#define %{APPNAMEUC}PLUGIN_H

// KF headers
#include <KTextEditor/Plugin>

class %{APPNAME}Plugin : public KTextEditor::Plugin
{
    Q_OBJECT

public:
    /**
     * Default constructor, with arguments as expected by KPluginFactory
     */
    %{APPNAME}Plugin(QObject* parent, const QVariantList& args);

    ~%{APPNAME}Plugin() override;

public: // KTextEditor::Plugin API
    QObject* createView(KTextEditor::MainWindow* mainWindow) override;
};

#endif // %{APPNAMEUC}PLUGIN_H
