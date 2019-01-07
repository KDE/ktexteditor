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

#include "%{APPNAMELC}plugin.h"

#include "%{APPNAMELC}view.h"

// KF headers
#include <KTextEditor/MainWindow>

#include <KPluginFactory>
#include <KLocalizedString>

K_PLUGIN_CLASS_WITH_JSON(%{APPNAME}Plugin, "%{APPNAMELC}.json")


%{APPNAME}Plugin::%{APPNAME}Plugin(QObject* parent, const QVariantList& /*args*/)
    : KTextEditor::Plugin(parent)
{
}

%{APPNAME}Plugin::~%{APPNAME}Plugin()
{
}

QObject* %{APPNAME}Plugin::createView(KTextEditor::MainWindow* mainwindow)
{
    return new %{APPNAME}View(this, mainwindow);
}


// needed for K_PLUGIN_CLASS_WITH_JSON
#include <%{APPNAMELC}plugin.moc>
