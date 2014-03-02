/* This file is part of the KDE project
   Copyright (C) 2009 Milian Wolff <mail@milianw.de>

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
    ///TODO: Documentation
    enum DefaultStyle {
        // normal text
        dsNormal = 0,
        dsKeyword,
        dsFunction,
        dsVariable,
        dsControlFlow,
        dsOperator,
        dsBuiltIn,
        dsExtension,
        dsPreprocessor,
        dsAttribute,

        // Strings & Characters
        dsChar,
        dsSpecialChar,
        dsString,
        dsVerbatimString,
        dsSpecialString,
        dsImport,

        // Number, Types & Constants
        dsDataType,
        dsDecVal,
        dsBaseN,
        dsFloat,
        dsConstant,

        // Comments & Documentation
        dsComment,
        dsDocumentation,
        dsAnnotation,
        dsCommentVar,
        dsRegionMarker,
        dsInformation,
        dsWarning,
        dsAlert,

        // Misc
        dsOthers,
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

