/* This file is part of the KDE project
 *  Copyright (C) 2001-2014 Christoph Cullmann <cullmann@kde.org>
 *  Copyright (C) 2005-2014 Dominik Haumann <dhaumann@kde.org>
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

#ifndef KTEXTEDITOR_CONFIGPAGE_H
#define KTEXTEDITOR_CONFIGPAGE_H

#include <ktexteditor_export.h>

#include <QWidget>

namespace KTextEditor
{

/**
 * \brief Config page interface for the Editor and Plugin%s.
 *
 * \section configpage_intro Introduction
 *
 * The class ConfigPage represents a config page.
 * The config pages are usually embedded into a dialog that shows
 * buttons like \e Defaults, \e Reset and \e Apply. If one of the buttons is
 * clicked and the config page sent the signal changed() beforehand the
 * Editor will call the corresponding slot, either defaults(), reset() or
 * apply().
 *
 * To obtain a useful navigation information for displaying to a user see name(),
 * fullName() and icon() functions.
 *
 * \section configpage_config Saving and Loading Config Data
 *
 * Saving and loading the configuration data can either be achieved by using
 * the host application's KSharedConfig::openConfig() object, or by using an
 * own configuration file.
 *
 * \see KTextEditor::Editor, KTextEditor::Plugin
 * \author Christoph Cullmann \<cullmann@kde.org\>
 */
class KTEXTEDITOR_EXPORT ConfigPage : public QWidget
{
    Q_OBJECT

public:
    /**
     * Constructor.
     *
     * Create a new config page with \p parent.
     * \param parent parent widget
     */
    ConfigPage(QWidget *parent);
    /**
     * Virtual destructor.
     */
    virtual ~ConfigPage();

    /**
     * Get a readable name for the config page. The name should be translated.
     * \return name of given page index
     * \see fullName(), icon()
     */
    virtual QString name() const = 0;

    /**
     * Get a readable full name for the config page. The name
     * should be translated.
     *
     * Example: If the name is "Filetypes", the full name could be
     * "Filetype Specific Settings". For "Shortcuts" the full name would be
     * something like "Shortcut Configuration".
     * \return full name of given page index, default implementation returns name()
     * \see name(), icon()
     */
    virtual QString fullName() const;

    /**
     * Get an icon for the config page.
     * \return icon for the given page index
     * \see name(), fullName()
     */
    virtual QIcon icon() const;

public Q_SLOTS:
    /**
     * This slot is called whenever the button \e Apply or \e OK was clicked.
     * Apply the changed settings made in the config page now.
     */
    virtual void apply() = 0;

    /**
     * This slot is called whenever the button \e Reset was clicked.
     * Reset the config page settings to the initial state.
     */
    virtual void reset() = 0;

    /**
     * Sets default options
     * This slot is called whenever the button \e Defaults was clicked.
     * Set the config page settings to the default values.
     */
    virtual void defaults() = 0;

Q_SIGNALS:
    /**
     * Emit this signal whenever a config option changed.
     */
    void changed();

private:
    class ConfigPagePrivate *const d;
};

}

#endif

