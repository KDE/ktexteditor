/*
    SPDX-FileCopyrightText: 2001-2014 Christoph Cullmann <cullmann@kde.org>
    SPDX-FileCopyrightText: 2005-2014 Dominik Haumann <dhaumann@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef KTEXTEDITOR_CONFIGPAGE_H
#define KTEXTEDITOR_CONFIGPAGE_H

#include <ktexteditor_export.h>

#include <QWidget>

namespace KTextEditor
{
/*!
 * \class KTextEditor::ConfigPage
 * \inmodule KTextEditor
 * \inheaderfile KTextEditor/ConfigPage
 *
 * \brief Config page interface for the Editor and Plugin%s.
 *
 * Introduction
 *
 * The class ConfigPage represents a config page.
 * The config pages are usually embedded into a dialog that shows
 * buttons like \c Defaults, \c Reset and \c Apply. If one of the buttons is
 * clicked and the config page sent the signal changed() beforehand the
 * Editor will call the corresponding slot, either defaults(), reset() or
 * apply().
 *
 * To obtain a useful navigation information for displaying to a user see name(),
 * fullName() and icon() functions.
 *
 * Saving and Loading Config Data
 *
 * Saving and loading the configuration data can either be achieved by using
 * the host application's KSharedConfig::openConfig() object, or by using an
 * own configuration file.
 *
 * \sa KTextEditor::Editor, KTextEditor::Plugin
 */
class KTEXTEDITOR_EXPORT ConfigPage : public QWidget
{
    Q_OBJECT

public:
    /*!
     * Constructor.
     *
     * Create a new config page with the specified parent.
     *
     * \a parent is the parent widget
     */
    ConfigPage(QWidget *parent);

    ~ConfigPage() override;

    /*!
     * Get a readable name for the config page. The name should be translated.
     * Returns name of given page index
     * \sa fullName(), icon()
     */
    virtual QString name() const = 0;

    /*!
     * Get a readable full name for the config page. The name
     * should be translated.
     *
     * Example: If the name is "Filetypes", the full name could be
     * "Filetype Specific Settings". For "Shortcuts" the full name would be
     * something like "Shortcut Configuration".
     * Returns full name of given page index, default implementation returns name()
     * \sa name(), icon()
     */
    virtual QString fullName() const;

    /*!
     * Get an icon for the config page.
     * Returns icon for the given page index
     * \sa name(), fullName()
     */
    virtual QIcon icon() const;

public Q_SLOTS:
    /*!
     * This slot is called whenever the button \c Apply or \c OK was clicked.
     * Apply the changed settings made in the config page now.
     */
    virtual void apply() = 0;

    /*!
     * This slot is called whenever the button \c Reset was clicked.
     * Reset the config page settings to the initial state.
     */
    virtual void reset() = 0;

    /*!
     * Sets default options
     * This slot is called whenever the button \c Defaults was clicked.
     * Set the config page settings to the default values.
     */
    virtual void defaults() = 0;

Q_SIGNALS:
    /*!
     * Emit this signal whenever a config option changed.
     */
    void changed();

private:
    class ConfigPagePrivate *const d;
};

}

#endif
