/* This file is part of the KDE libraries
   Copyright (C) 2001 Joseph Wenninger <jowenn@kde.org>
   Copyright (C) 2013-2014 Dominik Haumann <dhaumann@kde.org>

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
#ifndef KTEXTEDITOR_TEXTHINTINTERFACE_H
#define KTEXTEDITOR_TEXTHINTINTERFACE_H

#include <QString>

#include <ktexteditor_export.h>

#include <ktexteditor/cursor.h>

namespace KTextEditor
{

class TextHintProvider;
class View;

/**
 * \brief Text hint interface showing tool tips under the mouse for the View.
 *
 * \ingroup kte_group_view_extensions
 *
 * \section texthint_introduction Introduction
 *
 * The text hint interface provides a way to show tool tips for text located
 * under the mouse. Possible applications include showing a value of a variable
 * when debugging an application, or showing a complete path of an include
 * directive.
 *
 * \image html texthint.png "Text hint showing the contents of a variable"
 *
 * To register as text hint provider, call registerTextHintProvider() with an
 * instance that inherits TextHintProvider. Finally, make sure you remove your
 * text hint provider by calling unregisterTextHintProvider().
 *
 * Text hints are shown after the user hovers with the mouse for a delay of
 * textHintDelay() milliseconds over the same word. To change the delay, call
 * setTextHintDelay().
 *
 * \section texthint_access Accessing the TextHintInterface
 *
 * The TextHintInterface is an extension interface for a
 * View, i.e. the View inherits the interface. Use qobject_cast to access the
 * interface:
 * \code
 * // view is of type KTextEditor::View*
 * auto iface = qobject_cast<KTextEditor::TextHintInterface*>(view);
 *
 * if (iface) {
 *     // the implementation supports the interface
 *     // myProvider inherits KTextEditor::TextHintProvider
 *     iface->registerTextHintProvider(myProvider);
 * } else {
 *     // the implementation does not support the interface
 * }
 * \endcode
 *
 * \see TextHintProvider
 * \since 4.11
 */
class KTEXTEDITOR_EXPORT TextHintInterface
{
public:
    TextHintInterface();
    virtual ~TextHintInterface();

    /**
     * Register the text hint provider \p provider.
     *
     * Whenever the user hovers over text, \p provider will be asked for
     * a text hint. When the provider is about to be destroyed, make
     * sure to call unregisterTextHintProvider() to avoid a dangling pointer.
     *
     * @param provider text hint provider
     * @see unregisterTextHintProvider(), TextHintProvider
     */
    virtual void registerTextHintProvider(KTextEditor::TextHintProvider *provider) = 0;

    /**
     * Unregister the text hint provider \p provider.
     *
     * @param provider text hint provider to unregister
     * @see registerTextHintProvider(), TextHintProvider
     */
    virtual void unregisterTextHintProvider(KTextEditor::TextHintProvider * provider) = 0;

    /**
     * Set the text hint delay to \p delay milliseconds.
     *
     * The delay specifies the time the user needs to hover over the text
     * before the tool tip is shown. Therefore, \p delay should not be
     * too large, a value of 500 milliseconds is recommended and set by
     * default.
     *
     * If \p delay is <= 0, the default delay will be set.
     *
     * \param delay tool tip delay in milliseconds
     */
    virtual void setTextHintDelay(int delay) = 0;

    /**
     * Get the text hint delay in milliseconds.
     * By default, the text hint delay is set to 500 milliseconds.
     * It can be changed by calling \p setTextHintDelay().
     */
    virtual int textHintDelay() const = 0;

private:
    class TextHintInterfacePrivate * const d = nullptr;
};

/**
 * \brief Class to provide text hints for a View.
 *
 * The class TextHintProvider is used in combination with TextHintInterface.
 * TextHintProvider allows to provide text hint information for text under
 * the mouse cursor.
 *
 * To use this class, derive your provider from TextHintProvider and register
 * it with TextHintInterface::registerTextHintProvider(). When not needed
 * anymore, make sure to remove your provider by calling
 * TextHintInterface::unregisterTextHintProvider(), otherwise the View will
 * contain a dangling pointer to your potentially deleted provider.
 *
 * Detailed information about how to use the TextHintInterface can be found
 * in the documentation about the TextHintInterface.
 *
 * \see TextHintInterface
 * \p since 5.0
 */
class KTEXTEDITOR_EXPORT TextHintProvider
{
public:
    /**
     * Default constructor.
     */
    TextHintProvider();

    /**
     * Virtual destructor to allow inheritance.
     */
    virtual ~TextHintProvider();

    /**
     * This function is called whenever the users hovers over text such
     * that the text hint delay passes. Then, textHint() is called
     * for each registered TextHintProvider.
     *
     * Return the text hint (possibly Qt richtext) for @p view at @p position.
     *
     * If you do not have any contents to show, just return an empty QString().
     *
     * \param view the view that requests the text hint
     * \param position text cursor under the mouse position
     * \return text tool tip to be displayed, may be Qt richtext
     */
    virtual QString textHint(KTextEditor::View *view,
                             const KTextEditor::Cursor &position) = 0;

private:
    class TextHintProviderPrivate * const d = nullptr;
};

}

Q_DECLARE_INTERFACE(KTextEditor::TextHintInterface, "org.kde.KTextEditor.TextHintInterface")

#endif
