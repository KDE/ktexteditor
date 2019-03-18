/* This file is part of the KDE project
   Copyright (C) 2019 Dominik Haumann <dhaumann@kde.org>

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

#ifndef KTEXTEDITOR_VARIABLE_H
#define KTEXTEDITOR_VARIABLE_H

#include <QStringList>
#include <QVariant>

namespace KTextEditor
{
    class View;

/**
 * @brief Variable for variable expansion.
 *
 * @section variable_intro Introduction
 *
 * A Variable is used by the KTextEditor::Editor to expand variables, also
 * know as expanding macros. A Variable itself is defined by the variable
 * name() as well as a description() and a function that replaces the variable
 * by its value.
 *
 * To register a Variable in the Editor use either Editor::registerVariableMatch()
 * or Editor::registerPrefixMatch().
 *
 * @see KTextEditor::Editor, KTextEditor::Editor::registerVariableMatch(),
 *      KTextEditor::Editor::registerPrefixMatch()
 * @author Dominik Haumann \<dhaumann@kde.org\>
 */
class Variable
{
public:
    /**
     * Function that is called to expand a variable in @p text.
     * @param text
     */
    using ExpandFunction = QString (*)(const QStringView& text, KTextEditor::View* view);

    /**
     * Constructor defining a Variable by its @p name, its @p description, and
     * its function @p expansionFunc to expand a variable to its corresponding
     * value.
     *
     * @note The @p name should @e not be translated.
     */
    Variable(const QString& name, const QString& description, ExpandFunction expansionFunc);

    /**
     * Returns true, if the name is non-empty and the function provided in the
     * constructor is not a nullptr.
     */
    bool isValid() const;

    /**
     * Returns the @p name that was provided in the constructor.
     * Depending on where the Variable is registered, this name is used to
     * identify an exact match or a prefix match.
     */
    QString name() const;

    /**
     * Returns the description that was provided in the constructor.
     */
    QString description() const;

    /**
     * Expands the Variable to its value.
     *
     * As example for an exact match, a variable "CurerntDocument:Cursor:Line"
     * uses the @p view to return the current line of the text cursor. In this
     * case @p prefix equals the text of the variable itself, i.e.
     * "CurerntDocument:Cursor:Line".
     *
     * As example of a prefix match, a variable "ENV:value" expands the
     * environment value @e value, e.g. "ENV:HOME". In this case, the @p prefix
     * equals the text "ENV:HOME" and @p view would be unused.
     *
     * @return the expanded variable.
     */
    QString evaluate(const QStringView& prefix, KTextEditor::View * view) const;

private:
    QString m_name;
    QString m_description;
    ExpandFunction m_function;
};

}

#endif
