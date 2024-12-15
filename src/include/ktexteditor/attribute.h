/*
    SPDX-FileCopyrightText: 2003-2005 Hamish Rodda <rodda@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef KTEXTEDITOR_ATTRIBUTE_H
#define KTEXTEDITOR_ATTRIBUTE_H

#include <QExplicitlySharedDataPointer>
#include <QSharedData>
#include <QTextCharFormat>

#include <KSyntaxHighlighting/Theme>

#include <ktexteditor_export.h>

class QAction;

namespace KTextEditor
{

/*!
 * \class KTextEditor::Attribute
 * \inmodule KTextEditor
 * \inheaderfile KTextEditor/Attribute
 *
 * \brief A class which provides customized text decorations.
 *
 * The Attribute class extends QTextCharFormat, the class which Qt
 * uses internally to provide formatting information to characters
 * in a text document.
 *
 * In addition to its inherited properties, it provides support for:
 * \list
 * \li several customized text formatting properties
 * \li dynamic highlighting of associated ranges of text
 * \li binding of actions with associated ranges of text (note: not currently implemented)
 * \endlist
 *
 * Implementations are not required to support all properties.
 * In particular, several properties are not supported for dynamic
 * highlighting (notably: font() and fontBold()).
 *
 * Unfortunately, as QTextFormat's setProperty() is not virtual,
 * changes that are made to this attribute cannot automatically be
 * redrawn.  Once you have finished changing properties, you should
 * call changed() to force redrawing of affected ranges of text.
 *
 * \sa MovingInterface
 */
class KTEXTEDITOR_EXPORT Attribute : public QTextCharFormat, public QSharedData
{
public:
    /*!
     * \typedef KTextEditor::Attribute::Ptr
     * Shared data pointer for Attribute
     */
    typedef QExplicitlySharedDataPointer<Attribute> Ptr;

    /*!
     * Default constructor.
     *
     * The resulting Attribute has no properties set to begin with.
     */
    Attribute();

    /*!
     * Construct attribute with given name & default style properties.
     *
     * \a name is the attribute name
     *
     * \a style is the attribute default style
     *
     */
    Attribute(const QString &name, KSyntaxHighlighting::Theme::TextStyle style);

    Attribute(const Attribute &a);

    virtual ~Attribute();

    // BEGIN custom properties

    /*
     * \name Custom properties
     *
     * The following functions provide custom properties which can be set for
     * rendering by editor implementations.
     * \{
     */

    /*!
     * Returns the attribute name.
     */
    QString name() const;

    /*!
     * Sets the attribute name
     *
     * \a name is the new attribute name
     *
     */
    void setName(const QString &name);

    /*!
     * Default style of this attribute
     *
     * Returns default style
     */
    KSyntaxHighlighting::Theme::TextStyle defaultStyle() const;

    /*!
     * Sets the default text style of this attribute.
     *
     * \a style is the new default text style
     *
     */
    void setDefaultStyle(KSyntaxHighlighting::Theme::TextStyle style);

    /*!
     * Should spellchecking be skipped?
     *
     * Returns true if spellchecking should be skipped
     */
    bool skipSpellChecking() const;

    /*!
     * Set if we should spellchecking be skipped?
     *
     * \a skipspellchecking if true then spellchecking should be skipped
     *
     */
    void setSkipSpellChecking(bool skipspellchecking);

    /*!
     * Find out if the font weight is set to QFont::Bold.
     *
     * Returns \c true if the font weight is exactly QFont::Bold, otherwise \c false
     *
     * \sa QTextCharFormat::fontWeight()
     */
    bool fontBold() const;

    /*!
     * Set the font weight to QFont::Bold.  If bold is false, the weight will be set to 0 (normal).
     *
     * \a bold specifies whether the font weight should be bold or not.
     *
     * \sa QTextCharFormat::setFontWeight()
     */
    void setFontBold(bool bold = true);

    /*!
     * Get the brush used to draw an outline around text, if any.
     *
     * Returns brush to be used to draw an outline, or Qt::NoBrush if no outline is set.
     */
    QBrush outline() const;

    /*!
     * Set a brush to be used to draw an outline around text.
     *
     * Use clearProperty(Outline) to clear.
     *
     * \a brush is the brush to be used to draw an outline.
     *
     */
    void setOutline(const QBrush &brush);

    /*!
     * Get the brush used to draw text when it is selected, if any.
     *
     * Returns brush to be used to draw selected text, or Qt::NoBrush if not set
     */
    QBrush selectedForeground() const;

    /*!
     * Set a brush to be used to draw selected text.
     *
     * \sa clearProperty()
     *
     * \a foreground is the brush to be used to draw selected text.
     *
     */
    void setSelectedForeground(const QBrush &foreground);

    /*!
     * Get the brush used to draw the background of selected text, if any.
     *
     * Returns brush to be used to draw the background of selected text, or Qt::NoBrush if not set
     */
    QBrush selectedBackground() const;

    /*!
     * Set a brush to be used to draw the background of selected text, if any.
     *
     * Use clearProperty(SelectedBackground) to clear.
     *
     * \a brush is the brush to be used to draw the background of selected text
     *
     */
    void setSelectedBackground(const QBrush &brush);

    /*!
     * Determine whether background color is drawn over whitespace. Defaults to true if not set.
     *
     * Returns whether the background color should be drawn over whitespace
     */
    bool backgroundFillWhitespace() const;

    /*!
     * Set whether background color is drawn over whitespace. Defaults to true if not set.
     *
     * Use clearProperty(BackgroundFillWhitespace) to clear.
     *
     * \a fillWhitespace specifies whether the background should be drawn over whitespace.
     *
     */
    void setBackgroundFillWhitespace(bool fillWhitespace);

    /*!
     * Clear all set properties.
     */
    void clear();

    /*!
     * Determine if any properties are set.
     *
     * Returns \e true if any properties are set, otherwise \e false
     */
    bool hasAnyProperty() const;

    // END

    // BEGIN Dynamic highlighting

    /*
     * Dynamic highlighting
     *
     * The following functions allow for text to be highlighted dynamically based on
     * several events.
     * \{
     */

    /*!
       \enum KTextEditor::Attribute::ActivationType

       Several automatic activation mechanisms exist for associated attributes.
       Using this you can conveniently have your ranges highlighted when either
       the mouse or cursor enter the range.

       \value ActivateMouseIn
       Activate attribute on mouse in

       \value ActivateCaretIn
       Activate attribute on caret in
     */
    enum ActivationType {
        ActivateMouseIn = 0,
        ActivateCaretIn
    };

    /*!
     * Returns the attribute to use when the event referred to by \a type occurs.
     *
     * \a type the activation type for which to return the Attribute.
     *
     */
    Attribute::Ptr dynamicAttribute(ActivationType type) const;

    /*!
     * Sets the attribute to use when the event referred to by type occurs.
     *
     * \note Nested dynamic attributes are ignored.
     *
     * \a type is the activation type to set the attribute for
     *
     * \a attribute is the attribute to assign. As attribute is refcounted, ownership is not an issue.
     *
     */
    void setDynamicAttribute(ActivationType type, Attribute::Ptr attribute);

    //!\}

    // END

    /*!
     * Addition assignment operator.  Use this to merge another Attribute with this Attribute.
     * Where both attributes have a particular property set, the property in \a a will
     * be used.
     *
     * \a a is the attribute to merge into this attribute.
     *
     */
    Attribute &operator+=(const Attribute &a);

    /*!
     * Replacement assignment operator.  Use this to overwrite this Attribute with another Attribute.
     *
     * \a a is the attribute to assign to this attribute.
     *
     */
    Attribute &operator=(const Attribute &a);

private:
    /*
     * Private d-pointer
     */
    class AttributePrivate *const d;
};

/*
 * Attribute%s of a part of a line.
 *
 * An AttributeBlock represents an Attribute spanning the interval
 * [start, start + length) of a given line. An AttributeBlock is
 * obtained by calling KTextEditor::View::lineAttributes().
 *
 * See KTextEditor::View::lineAttributes()
 */
class AttributeBlock
{
public:
    /*
     * Constructor of AttributeBlock.
     */
    AttributeBlock(int _start, int _length, const Attribute::Ptr &_attribute)
        : start(_start)
        , length(_length)
        , attribute(_attribute)
    {
    }

    /*
     * The column this attribute starts at.
     */
    int start;

    /*
     * The number of columns this attribute spans.
     */
    int length;

    /*
     * The attribute for the current range.
     */
    Attribute::Ptr attribute;
};

}

Q_DECLARE_TYPEINFO(KTextEditor::AttributeBlock, Q_RELOCATABLE_TYPE);

#endif
