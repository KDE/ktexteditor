/*
    SPDX-FileCopyrightText: 2011-2018 Dominik Haumann <dhaumann@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef VARIABLE_EDITOR_H
#define VARIABLE_EDITOR_H

#include <QWidget>

class KateHelpButton;

class VariableBoolItem;
class VariableColorItem;
class VariableFontItem;
class VariableItem;
class VariableStringListItem;
class VariableIntItem;
class VariableStringItem;
class VariableSpellCheckItem;
class VariableRemoveSpacesItem;

class KColorCombo;
class QFontComboBox;
class QCheckBox;
class QComboBox;
class QLabel;
class QLineEdit;
class QSpinBox;

namespace Sonnet
{
class DictionaryComboBox;
}

class VariableEditor : public QWidget
{
    Q_OBJECT

public:
    explicit VariableEditor(VariableItem *item, QWidget *parent = nullptr);

    VariableItem *item() const;

Q_SIGNALS:
    void valueChanged();

protected Q_SLOTS:
    void itemEnabled(bool enabled);
    void activateItem();

protected:
    void paintEvent(QPaintEvent *event) override;
    void enterEvent(QEvent *event) override;
    void leaveEvent(QEvent *event) override;

private:
    VariableItem *m_item;

    QCheckBox *m_checkBox;
    QLabel *m_variable;
    QLabel *m_helpText;
    KateHelpButton *m_btnHelp;
};

class VariableIntEditor : public VariableEditor
{
    Q_OBJECT
public:
    VariableIntEditor(VariableIntItem *item, QWidget *parent);

protected Q_SLOTS:
    void setItemValue(int newValue);

private:
    QSpinBox *m_spinBox;
};

class VariableBoolEditor : public VariableEditor
{
    Q_OBJECT
public:
    VariableBoolEditor(VariableBoolItem *item, QWidget *parent);

protected Q_SLOTS:
    void setItemValue(int enabled);

private:
    QComboBox *m_comboBox;
};

class VariableStringListEditor : public VariableEditor
{
    Q_OBJECT
public:
    VariableStringListEditor(VariableStringListItem *item, QWidget *parent);

protected Q_SLOTS:
    void setItemValue(const QString &newValue);

private:
    QComboBox *m_comboBox;
};

class VariableColorEditor : public VariableEditor
{
    Q_OBJECT
public:
    VariableColorEditor(VariableColorItem *item, QWidget *parent);

protected Q_SLOTS:
    void setItemValue(const QColor &newValue);

private:
    KColorCombo *m_comboBox;
};

class VariableFontEditor : public VariableEditor
{
    Q_OBJECT
public:
    VariableFontEditor(VariableFontItem *item, QWidget *parent);

protected Q_SLOTS:
    void setItemValue(const QFont &newValue);

private:
    QFontComboBox *m_comboBox;
};

class VariableStringEditor : public VariableEditor
{
    Q_OBJECT
public:
    VariableStringEditor(VariableStringItem *item, QWidget *parent);

protected Q_SLOTS:
    void setItemValue(const QString &newValue);

private:
    QLineEdit *m_lineEdit;
};

class VariableSpellCheckEditor : public VariableEditor
{
    Q_OBJECT
public:
    VariableSpellCheckEditor(VariableSpellCheckItem *item, QWidget *parent);

protected Q_SLOTS:
    void setItemValue(const QString &newValue);

private:
    Sonnet::DictionaryComboBox *m_dictionaryCombo;
};

class VariableRemoveSpacesEditor : public VariableEditor
{
    Q_OBJECT
public:
    VariableRemoveSpacesEditor(VariableRemoveSpacesItem *item, QWidget *parent);

protected Q_SLOTS:
    void setItemValue(int enabled);

private:
    QComboBox *m_comboBox;
};

#endif
