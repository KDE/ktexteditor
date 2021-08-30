/*
    SPDX-FileCopyrightText: 2019 KDE Developers

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "kateconfigpage.h"

#include <KFontRequester>
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
    connect(comboBox, qOverload<int>(&QComboBox::currentIndexChanged), this, &KateConfigPage::slotChanged);
}

void KateConfigPage::observeChanges(QGroupBox *groupBox)
{
    connect(groupBox, &QGroupBox::toggled, this, &KateConfigPage::slotChanged);
}

void KateConfigPage::observeChanges(QLineEdit *lineEdit)
{
    connect(lineEdit, &QLineEdit::textChanged, this, &KateConfigPage::slotChanged);
}

void KateConfigPage::observeChanges(KFontRequester *chooser)
{
    connect(chooser, &KFontRequester::fontSelected, this, &KateConfigPage::slotChanged);
}

void KateConfigPage::slotChanged()
{
    Q_EMIT changed();
}

void KateConfigPage::somethingHasChanged()
{
    m_changed = true;
}
