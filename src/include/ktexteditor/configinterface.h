/*
    SPDX-FileCopyrightText: 2006 Matt Broadstone <mbroadst@gmail.com>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef KTEXTEDITOR_CONFIGINTERFACE_H
#define KTEXTEDITOR_CONFIGINTERFACE_H

#include <QStringList>
#include <QVariant>
#include <ktexteditor_export.h>

namespace KTextEditor
{
/**
 * \class ConfigInterface configinterface.h <KTextEditor/ConfigInterface>
 *
 * \brief Config interface extension for the Document and View.
 *
 * \ingroup kte_group_view_extensions
 * \ingroup kte_group_doc_extensions
 *
 * \section config_intro Introduction
 *
 * The ConfigInterface provides methods to access and modify the low level
 * config information for a given Document or View. Examples of this config data can be
 * displaying the icon bar, showing line numbers, etc. This generally allows
 * access to settings that otherwise are only accessible during runtime.
 *
 * \section config_access Accessing the Interface
 *
 * The ConfigInterface is supposed to be an extension interface for a Document or View,
 * i.e. the Document or View inherits the interface \e provided that the
 * KTextEditor library in use implements the interface. Use qobject_cast to access
 * the interface:
 * \code
 * // ptr is of type KTextEditor::Document* or KTextEditor::View*
 * auto iface = qobject_cast<KTextEditor::ConfigInterface*>(ptr);
 *
 * if (iface) {
 *     // the implementation supports the interface
 *     // do stuff
 * } else {
 *     // the implementation does not support the interface
 * }
 * \endcode
 *
 * \section config_data Accessing Data
 *
 * A list of available config variables (or keys) can be obtained by calling
 * configKeys(). For all available keys configValue() returns the corresponding
 * value as QVariant. A value for a given key can be set by calling
 * setConfigValue(). Right now, when using KatePart as editor component,
 * KTextEditor::View has support for the following tuples:
 *  - line-numbers [bool], show/hide line numbers
 *  - icon-bar [bool], show/hide icon bar
 *  - folding-bar [bool], show/hide the folding bar
 *  - folding-preview [bool], enable/disable folding preview when mouse hovers
 *    on folded region
 *  - dynamic-word-wrap [bool], enable/disable dynamic word wrap
 *  - background-color [QColor], read/set the default background color
 *  - selection-color [QColor], read/set the default color for selections
 *  - search-highlight-color [QColor], read/set the background color for search
 *  - replace-highlight-color [QColor], read/set the background color for replaces
 *  - default-mark-type [uint], read/set the default mark type
 *  - allow-mark-menu [bool], enable/disable the menu shown when right clicking
 *    on the left gutter. When disabled, click on the gutter will always set
 *    or clear the mark of default type.
 *  - icon-border-color [QColor] read/set the icon border color (on the left,
 *    with the line numbers)
 *  - folding-marker-color [QColor] read/set folding marker colors (in the icon border)
 *  - line-number-color [QColor] read/set line number colors (in the icon border)
 *  - current-line-number-color [QColor] read/set current line number color (in the icon border)
 *  - modification-markers [bool] read/set whether the modification markers are shown
 *  - word-count [bool] enable/disable the counting of words and characters in the statusbar
 *  - line-count [bool] show/hide the total number of lines in the status bar (@since 5.66)
 *  - scrollbar-minimap [bool] enable/disable scrollbar minimap
 *  - scrollbar-preview [bool] enable/disable scrollbar text preview on hover
 *  - font [QFont] change the font
 *  - theme [QString] change the theme
 *
 * KTextEditor::Document has support for the following:
 *  - backup-on-save-local [bool], enable/disable backup when saving local files
 *  - backup-on-save-remote [bool], enable/disable backup when saving remote files
 *  - backup-on-save-suffix [string], set the suffix for file backups, e.g. "~"
 *  - backup-on-save-prefix [string], set the prefix for file backups, e.g. "."
 *  - replace-tabs [bool], whether to replace tabs
 *  - indent-pasted-text [bool], whether to indent pasted text
 *  - tab-width [int], read/set the width for tabs
 *  - indent-width [int], read/set the indentation width
 *  - on-the-fly-spellcheck [bool], enable/disable on the fly spellcheck
 *
 * The KTextEditor::Document or KTextEditor::View objects will emit the \p configChanged signal when appropriate.
 *
 * For instance, if you want to enable dynamic word wrap of a KTextEditor::View
 * simply call
 * \code
 * iface->setConfigValue("dynamic-word-wrap", true);
 * \endcode
 *
 * \see KTextEditor::View, KTextEditor::Document
 * \author Matt Broadstone \<mbroadst@gmail.com\>
 */
class KTEXTEDITOR_EXPORT ConfigInterface
{
public:
    ConfigInterface();

    /**
     * Virtual destructor.
     */
    virtual ~ConfigInterface();

public:
    /**
     * Get a list of all available keys.
     */
    virtual QStringList configKeys() const = 0;
    /**
     * Get a value for the \p key.
     */
    virtual QVariant configValue(const QString &key) = 0;
    /**
     * Set a the \p key's value to \p value.
     */
    virtual void setConfigValue(const QString &key, const QVariant &value) = 0;

private:
    class ConfigInterfacePrivate *const d = nullptr;
};

}

Q_DECLARE_INTERFACE(KTextEditor::ConfigInterface, "org.kde.KTextEditor.ConfigInterface")

#endif
