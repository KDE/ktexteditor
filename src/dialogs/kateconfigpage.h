/* This file is part of the KDE libraries
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

#ifndef KATE_CONFIG_PAGE_H
#define KATE_CONFIG_PAGE_H

#include <ktexteditor/configpage.h>

class KateConfigPage : public KTextEditor::ConfigPage
{
    Q_OBJECT

public:
    explicit KateConfigPage(QWidget *parent = nullptr, const char *name = nullptr);
    virtual ~KateConfigPage();
    virtual void reload() = 0;

public:
    bool hasChanged()
    {
        return m_changed;
    }

protected Q_SLOTS:
    void slotChanged();

private Q_SLOTS:
    void somethingHasChanged();

protected:
    bool m_changed = false;
};

#endif
