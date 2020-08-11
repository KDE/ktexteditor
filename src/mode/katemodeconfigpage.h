/*
    SPDX-FileCopyrightText: 2001-2010 Christoph Cullmann <cullmann@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef KATE_MODECONFIGPAGE_H
#define KATE_MODECONFIGPAGE_H

#include <QHash>
#include <QPointer>

#include "katedialogs.h"
#include "katemodemanager.h"

namespace Ui
{
class FileTypeConfigWidget;
}

class ModeConfigPage : public KateConfigPage
{
    Q_OBJECT

public:
    explicit ModeConfigPage(QWidget *parent);
    ~ModeConfigPage() override;
    QString name() const override;

public Q_SLOTS:
    void apply() override;
    void reload() override;
    void reset() override;
    void defaults() override;

private Q_SLOTS:
    void update();
    void deleteType();
    void newType();
    void typeChanged(int type);
    void showMTDlg();
    void save();

private:
    Ui::FileTypeConfigWidget *ui;

    QList<KateFileType *> m_types;
    int m_lastType;
};

#endif
