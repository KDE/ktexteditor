/*
    SPDX-FileCopyrightText: 2001-2014 Christoph Cullmann <cullmann@kde.org>
    SPDX-FileCopyrightText: 2005-2014 Dominik Haumann <dhaumann@kde.org>
    SPDX-FileCopyrightText: 2009 Michel Ludwig <michel.ludwig@kdemail.net>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef KTEXTEDITOR_SESSIONCONFIGINTERFACE_H
#define KTEXTEDITOR_SESSIONCONFIGINTERFACE_H

#include <ktexteditor_export.h>

class KConfigGroup;

#include <QObject>

namespace KTextEditor
{
/*!
 * \class KTextEditor::SessionConfigInterface
 * \inmodule KTextEditor
 * \inheaderfile KTextEditor/SessionConfigInterface
 *
 * \brief Session config interface extension for the Plugin and Plugin views.
 *
 * \ingroup kte_group_plugin_extensions
 *
 * \target sessionconfig_intro
 * \section1 Introduction
 *
 * The SessionConfigInterface is an extension for Plugin%s and Plugin views
 * to add support for session-specific configuration settings.
 * readSessionConfig() is called whenever session-specific settings are to be
 * read from the given KConfigGroup and writeSessionConfig() whenever they are to
 * be written, for example when a session changed or was closed.
 *
 * \note A \e session does not have anything to do with an X-session under Unix.
 *       What is meant is rather a context, think of sessions in Kate or
 *       projects in KDevelop for example.
 *
 * \target sessionconfig_support
 * \section1 Adding Session Support
 *
 * To add support for sessions, your Plugin has to inherit the SessionConfigInterface
 * and reimplement readSessionConfig() and writeSessionConfig().
 *
 * \target sessionconfig_access
 * \section1 Accessing the SessionConfigInterface
 *
 * This section is for application developers such as Kate, KDevelop, etc that
 * what to support session configuration for plugins.
 *
 * The SessionConfigInterface is an extension interface for a Plugin or a
 * Plugin view, i.e. Plugin/Plugin view inherits the interface
 * \e provided that it implements the interface. Use qobject_cast to
 * access the interface:
 * \code
 * // object is of type Plugin* or, in case of a plugin view, QObject*
 * KTextEditor::SessionConfigInterface *iface =
 *     qobject_cast<KTextEditor::SessionConfigInterface*>( object );
 *
 * if( iface ) {
 *     // interface is supported
 *     // do stuff
 * }
 * \endcode
 *
 * \sa KTextEditor::Plugin
 */
class KTEXTEDITOR_EXPORT SessionConfigInterface
{
public:
    /*!
     *
     */
    SessionConfigInterface();

    /*!
     * Virtual destructor.
     */
    virtual ~SessionConfigInterface();

public:
    /*!
     * Read session settings from the given \a config.
     *
     * That means for example:
     * \list
     *  \li a Document should reload the file, restore all marks etc...
     *  \li a View should scroll to the last position and restore the cursor
     *    position etc...
     *  \li a Plugin should restore session specific settings
     *  \li If no file is being loaded, because an empty new document is going to be displayed,
     *    this function should emit ReadOnlyPart::completed
     * \endlist
     *
     * \a config is the KConfigGroup to read the session settings from
     *
     * \sa writeSessionConfig()
     */
    virtual void readSessionConfig(const KConfigGroup &config) = 0;

    /*!
     * Write session settings to the \a config.
     * See readSessionConfig() for more details.
     *
     * \a config is the KConfigGroup to write the session settings to
     *
     * \sa readSessionConfig()
     */
    virtual void writeSessionConfig(KConfigGroup &config) = 0;

private:
    class SessionConfigInterfacePrivate *const d = nullptr;
};

}

Q_DECLARE_INTERFACE(KTextEditor::SessionConfigInterface, "org.kde.KTextEditor.SessionConfigInterface")

#endif
