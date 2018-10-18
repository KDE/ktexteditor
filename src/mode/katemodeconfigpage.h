/*  This file is part of the KDE libraries and the Kate part.
 *
 *  Copyright (C) 2001-2010 Christoph Cullmann <cullmann@kde.org>
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

#ifndef KATE_MODECONFIGPAGE_H
#define KATE_MODECONFIGPAGE_H

#include <QPointer>
#include <QHash>

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
