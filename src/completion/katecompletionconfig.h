/*
    SPDX-FileCopyrightText: 2006 Hamish Rodda <rodda@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef KATECOMPLETIONCONFIG_H
#define KATECOMPLETIONCONFIG_H

#include <QDialog>

#include "kateconfig.h"

namespace Ui
{
class CompletionConfigWidget;
}

class QTreeWidgetItem;
class KateCompletionModel;

/**
 * @author Hamish Rodda <rodda@kde.org>
 */
class KateCompletionConfig : public QDialog, public KateConfig
{
    Q_OBJECT

public:
    explicit KateCompletionConfig(KateCompletionModel *model, QWidget *parent = nullptr);
    ~KateCompletionConfig() override;

    /**
     * Read config from object
     */
    void readConfig(const KConfigGroup &config);

    /**
     * Write config to object
     */
    void writeConfig(KConfigGroup &config);

public Q_SLOTS:
    void apply();

protected:
    void updateConfig() override;

private Q_SLOTS:
    void moveColumnUp();
    void moveColumnDown();
    void moveGroupingUp();
    void moveGroupingDown();
    void moveGroupingOrderUp();
    void moveGroupingOrderDown();

private:
    void applyInternal();

    Ui::CompletionConfigWidget *ui;
    KateCompletionModel *m_model;

    QTreeWidgetItem *m_groupingScopeType;
    QTreeWidgetItem *m_groupingScope;
    QTreeWidgetItem *m_groupingAccessType;
    QTreeWidgetItem *m_groupingItemType;
};

#endif
