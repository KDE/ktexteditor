/* This file is part of the KDE project
   Copyright (C) 2009 Milian Wolff <mail@milianw.de>
   Copyright (C) 2014 Dominik Haumann <dhaumann@kde.org>

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

#ifndef KDELIBS_KTEXTEDITOR_HIGHLIGHTINTERFACE_H
#define KDELIBS_KTEXTEDITOR_HIGHLIGHTINTERFACE_H

#include <ktexteditor_export.h>

#include <ktexteditor/attribute.h>
#include <ktexteditor/cursor.h>

namespace KTextEditor
{

class Document;

//
// TODO: KDE5
// The highlight information depends on the chosen Schema.
// Currently, each view can have a different Schema.
// Thus, for KDE5, this interface should be a view_extension and not a document_extension.
// Discussed by: Milian, Dominik
//

/**
 * \brief Highlighting information interface for the Document.
 *
 * \ingroup kte_group_doc_extensions
 *
 * \section highlightiface_intro Introduction
 *
 * The HighlightInterface provides methods to access the Attributes
 * used for highlighting the Document.
 *
 * \section highlightiface_access Accessing the HighlightInterface
 *
 * The HighlightInterface is supposed to be an extension interface for a
 * Document, i.e. the Document inherits the interface \e provided that the
 * used KTextEditor library implements the interface. Use qobject_cast to
 * access the interface:
 * \code
 * // doc is of type KTextEditor::Document*
 * KTextEditor::HighlightInterface *iface =
 *     qobject_cast<KTextEditor::HighlightInterface*>( doc );
 *
 * if( iface ) {
 *     // the implementation supports the interface
 *     // do stuff
 * }
 * \endcode
 *
 * \see KTextEditor::Document
 * \author Milian Wolff \<mail@milianw.de\>
 * \since 4.4
 */
class KTEXTEDITOR_EXPORT HighlightInterface
{
public:
    /**
     * The following list all valid default styles that is used for the syntax highlighting
     * xml files in the itemData's defStyleNum attribute.
     * Not all default styles are used by a syntax highlighting file.
     */
    enum DefaultStyle {
        //
        // normal text
        //
        /** Default for normal text and source code. */
        dsNormal = 0,
        /** Used for language keywords. */
        dsKeyword,
        /** Used for function definitions and function calls. */
        dsFunction,
        /** Used for variables, if applicable. */
        dsVariable,
        /** Used for control flow highlighting, e.g., if, then, else, return, continue. */
        dsControlFlow,
        /** Used for operators such as +, -, *, / and :: etc. */
        dsOperator,
        /** Used for built-in language classes and functions. */
        dsBuiltIn,
        /** Used for extensions, such as Qt or boost. */
        dsExtension,
        /** Used for preprocessor statements. */
        dsPreprocessor,
        /** Used for attributes of a function, e.g. \@override in Java. */
        dsAttribute,

        //
        // Strings & Characters
        //
        /** Used for a single character. */
        dsChar,
        /** Used for an escaped character. */
        dsSpecialChar,
        /** Used for strings. */
        dsString,
        /** Used for verbatim strings such as HERE docs. */
        dsVerbatimString,
        /** Used for special strings such as regular expressions or LaTeX math mode. */
        dsSpecialString,
        /** Used for includes, imports and modules. */
        dsImport,

        //
        // Number, Types & Constants
        //
        /** Used for data types such as int, char, float etc. */
        dsDataType,
        /** Used for decimal values. */
        dsDecVal,
        /** Used for numbers with base other than 10. */
        dsBaseN,
        /** Used for floating point numbers. */
        dsFloat,
        /** Used for language constants. */
        dsConstant,

        //
        // Comments & Documentation
        //
        /** Used for normal comments. */
        dsComment,
        /** Used for comments that reflect API documentation. */
        dsDocumentation,
        /** Used for annotations in comments, e.g. \@param in Doxygen or JavaDoc. */
        dsAnnotation,
        /** Used to refer to variables in a comment, e.g. after \@param in Doxygen or JavaDoc. */
        dsCommentVar,
        /** Used for region merkers, typically defined by BEGIN/END. */
        dsRegionMarker,
        /** Used for information, e.g. the keyword \@note in Doxygen. */
        dsInformation,
        /** Used for warnings, e.g. the keyword \@warning in Doxygen. */
        dsWarning,
        /** Used for comment specials TODO and WARNING in comments. */
        dsAlert,

        //
        // Misc
        //
        /** Used for attributes that do not match any of the other default styles. */
        dsOthers,
        /** Used to indicate wrong syntax. */
        dsError,

        // number of default styles, insert new default styles before this line
        DS_COUNT
    };

    /**
     * Constructor.
     */
    HighlightInterface();

    /**
     * Virtual destructor.
     */
    virtual ~HighlightInterface();

    /**
     * Returns the attribute used for the style \p ds.
     */
    virtual Attribute::Ptr defaultStyle(const DefaultStyle ds) const = 0;

    /// An AttributeBlock represents an Attribute with its
    /// dimension in a given line.
    ///
    /// \see lineAttributes()
    ///
    /// TODO: KDE5 mark as movable
    struct AttributeBlock {
        AttributeBlock(const int _start, const int _length, const Attribute::Ptr &_attribute)
            : start(_start), length(_length), attribute(_attribute) {
        }
        /// The column this attribute starts at.
        int start;
        /// The number of columns this attribute spans.
        int length;
        /// The attribute for the current range.
        Attribute::Ptr attribute;
    };

    /**
     * Get the list of AttributeBlocks for a given \p line in the document.
     *
     * \return list of AttributeBlocks for given \p line.
     */
    virtual QList<AttributeBlock> lineAttributes(const unsigned int line) = 0;
};

}

Q_DECLARE_INTERFACE(KTextEditor::HighlightInterface, "org.kde.KTextEditor.HighlightInterface")

#endif // KDELIBS_KTEXTEDITOR_HIGHLIGHTINTERFACE_H

