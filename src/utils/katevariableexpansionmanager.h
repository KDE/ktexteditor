/* This file is part of the KDE project
 *
 *  Copyright 2019 Dominik Haumann <dhaumann@kde.org>
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
#ifndef KTEXTEDITOR_VARIABLE_MANAGER
#define KTEXTEDITOR_VARIABLE_MANAGER

#include <QString>
#include <QVector>

#include "variable.h"

namespace KTextEditor
{
class View;
}

/**
 * Manager class for variable expansion.
 */
class KateVariableExpansionManager : public QObject
{
public:
    /**
     * Constructor with @p parent that takes ownership.
     */
    KateVariableExpansionManager(QObject *parent);

    /**
     * Adds @p variable to the expansion list view.
     */
    bool addVariable(const KTextEditor::Variable &variable);

    /**
     * Removes variable @p name.
     */
    bool removeVariable(const QString &name);

    /**
     * Returns the variable called @p name.
     */
    KTextEditor::Variable variable(const QString &name) const;

    /**
     * Returns all registered variables.
     */
    const QVector<KTextEditor::Variable> &variables() const;

    bool expandVariable(const QString &variable, KTextEditor::View *view, QString &output) const;

    QString expandText(const QString &text, KTextEditor::View *view) const;

    void showDialog(const QVector<QWidget *> &widgets, const QStringList &names) const;

private:
    QVector<KTextEditor::Variable> m_variables;
};

#endif // KTEXTEDITOR_VARIABLE_MANAGER

// kate: space-indent on; indent-width 4; replace-tabs on;
