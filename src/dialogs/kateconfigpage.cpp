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

#include "kateconfigpage.h"
#include "katepartdebug.h"

KateConfigPage::KateConfigPage(QWidget *parent, const char *)
    : KTextEditor::ConfigPage(parent)
    , m_changed(false)
{
    connect(this, SIGNAL(changed()), this, SLOT(somethingHasChanged()));
}

KateConfigPage::~KateConfigPage()
{
}

void KateConfigPage::slotChanged()
{
    emit changed();
}

void KateConfigPage::somethingHasChanged()
{
    m_changed = true;
    qCDebug(LOG_PART) << "TEST: something changed on the config page: " << this;
}
