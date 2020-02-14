/*  SPDX-License-Identifier: LGPL-2.0-or-later

    Copyright (C) 2019 KDE Developers

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

#include "kateconfigpage.h"

#include <KUrlRequester>

#include <QAbstractButton>
#include <QAbstractSlider>
#include <QAbstractSpinBox>
#include <QComboBox>
#include <QGroupBox>
#include <QLineEdit>

KateConfigPage::KateConfigPage(QWidget *parent, const char *)
    : KTextEditor::ConfigPage(parent)
{
    connect(this, &KateConfigPage::changed, this, &KateConfigPage::somethingHasChanged);
}

KateConfigPage::~KateConfigPage()
{
}

void KateConfigPage::observeChanges(KateConfigPage *page)
{
    connect(page, &KateConfigPage::changed, this, &KateConfigPage::slotChanged);
}

void KateConfigPage::observeChanges(KUrlRequester *requester)
{
    connect(requester, &KUrlRequester::textChanged, this, &KateConfigPage::slotChanged);
}

void KateConfigPage::observeChanges(QAbstractButton *button)
{
    connect(button, &QAbstractButton::toggled, this, &KateConfigPage::slotChanged);
}

void KateConfigPage::observeChanges(QAbstractSlider *slider)
{
    connect(slider, &QAbstractSlider::valueChanged, this, &KateConfigPage::slotChanged);
}

void KateConfigPage::observeChanges(QAbstractSpinBox *spinBox)
{
    connect(spinBox, &QAbstractSpinBox::editingFinished, this, &KateConfigPage::slotChanged);
}

void KateConfigPage::observeChanges(QComboBox *comboBox)
{
#if QT_VERSION < QT_VERSION_CHECK(5, 15, 0)
    connect(comboBox, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &KateConfigPage::slotChanged);
#else
    connect(comboBox, QOverload<int, const QString &>::of(&QComboBox::currentIndexChanged), this, &KateConfigPage::slotChanged);
#endif
}

void KateConfigPage::observeChanges(QGroupBox *groupBox)
{
    connect(groupBox, &QGroupBox::toggled, this, &KateConfigPage::slotChanged);
}

void KateConfigPage::observeChanges(QLineEdit *lineEdit)
{
    connect(lineEdit, &QLineEdit::textChanged, this, &KateConfigPage::slotChanged);
}

void KateConfigPage::slotChanged()
{
    emit changed();
}

void KateConfigPage::somethingHasChanged()
{
    m_changed = true;
}
