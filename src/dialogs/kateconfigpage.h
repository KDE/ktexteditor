/*
    SPDX-FileCopyrightText: 2019 KDE Developers

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef KATE_CONFIG_PAGE_H
#define KATE_CONFIG_PAGE_H

#include <ktexteditor/configpage.h>

class KFontRequester;
class KUrlRequester;
class QAbstractButton;
class QAbstractSlider;
class QAbstractSpinBox;
class QComboBox;
class QGroupBox;
class QLineEdit;

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

    void observeChanges(KateConfigPage *page);
    void observeChanges(KUrlRequester *requester);
    void observeChanges(QAbstractButton *button);
    void observeChanges(QAbstractSlider *slider);
    void observeChanges(QAbstractSpinBox *spinBox);
    void observeChanges(QComboBox *comboBox);
    void observeChanges(QGroupBox *groupBox);
    void observeChanges(QLineEdit *lineEdit);
    void observeChanges(KFontRequester *chooser);

protected Q_SLOTS:
    void slotChanged();

private Q_SLOTS:
    void somethingHasChanged();

protected:
    bool m_changed = false;
};

#endif
