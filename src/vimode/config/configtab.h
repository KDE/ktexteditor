/*  SPDX-License-Identifier: LGPL-2.0-or-later

    Copyright (C) KDE Developers

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

#ifndef KATEVI_CONFIG_TAB_H
#define KATEVI_CONFIG_TAB_H

#include <dialogs/kateconfigpage.h>
#include <vimode/mappings.h>

class QTableWidget;

namespace KateVi
{
namespace Ui
{
class ConfigWidget;
}

class ConfigTab : public KateConfigPage
{
    Q_OBJECT

public:
    explicit ConfigTab(QWidget *parent, Mappings *mappings);
    ~ConfigTab() override;

    QString name() const override;

protected:
    Ui::ConfigWidget *ui;

private:
    void applyTab(QTableWidget *mappingsTable, Mappings::MappingMode mode);
    void reloadTab(QTableWidget *mappingsTable, Mappings::MappingMode mode);

public Q_SLOTS:
    void apply() override;
    void reload() override;
    void reset() override;
    void defaults() override;

private Q_SLOTS:
    void showWhatsThis(const QString &text);
    void addMappingRow();
    void removeSelectedMappingRows();
    void importNormalMappingRow();

private:
    Mappings *m_mappings;
};

}

#endif /* KATEVI_CONFIG_TAB_H */
