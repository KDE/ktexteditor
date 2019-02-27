/* This file is part of the KDE libraries
   Copyright (C) 2001,2002 Joseph Wenninger <jowenn@kde.org>
   Copyright (C) 2001 Christoph Cullmann <cullmann@kde.org>
   Copyright (C) 1999 Jochen Wilhelmy <digisnap@cs.tu-berlin.de>
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

#ifndef KATE_HIGHLIGHT_H
#define KATE_HIGHLIGHT_H

#include <KSyntaxHighlighting/AbstractHighlighter>
#include <KSyntaxHighlighting/Definition>
#include <KSyntaxHighlighting/FoldingRegion>
#include <KSyntaxHighlighting/Format>

#include "katetextline.h"
#include "kateextendedattribute.h"
#include "katesyntaxmanager.h"
#include "spellcheck/prefixstore.h"
#include "range.h"

#include <QVector>
#include <QList>
#include <QHash>
#include <QMap>

#include <QRegularExpression>
#include <QObject>
#include <QStringList>
#include <QPointer>
#include <QDate>

#include <unordered_map>

class KConfig;

namespace KTextEditor {
    class DocumentPrivate;
}

class KateHighlighting : private KSyntaxHighlighting::AbstractHighlighter
{
public:
    explicit KateHighlighting(const KSyntaxHighlighting::Definition &def);

protected:
    /**
     * Reimplement this to apply formats to your output. The provided @p format
     * is valid for the interval [@p offset, @p offset + @p length).
     *
     * @param offset The start column of the interval for which @p format matches
     * @param length The length of the matching text
     * @param format The Format that applies to the range [offset, offset + length)
     *
     * @note Make sure to set a valid Definition, otherwise the parameter
     *       @p format is invalid for the entire line passed to highlightLine()
     *       (cf. Format::isValid()).
     *
     * @see applyFolding(), highlightLine()
     */
    void applyFormat(int offset, int length, const KSyntaxHighlighting::Format &format) override;

    /**
     * Reimplement this to apply folding to your output. The provided
     * FoldingRegion @p region either stars or ends a code folding region in the
     * interval [@p offset, @p offset + @p length).
     *
     * @param offset The start column of the FoldingRegion
     * @param length The length of the matching text that starts / ends a
     *       folding region
     * @param region The FoldingRegion that applies to the range [offset, offset + length)
     *
     * @note The FoldingRegion @p region is @e always either of type
     *       FoldingRegion::Type::Begin or FoldingRegion::Type::End.
     *
     * @see applyFormat(), highlightLine(), FoldingRegion
     */
    void applyFolding(int offset, int length, KSyntaxHighlighting::FoldingRegion region) override;

public:
    /**
     * Parse the text and fill in the context array and folding list array
     *
     * @param prevLine The previous line, the context array is picked up from that if present.
     * @param textLine The text line to parse
     * @param nextLine The next line, to check if indentation changed for indentation based folding.
     * @param ctxChanged will be set to reflect if the context changed
     * @param tabWidth tab width for indentation based folding, if wanted, else 0
     */
    void doHighlight(const Kate::TextLineData *prevLine,
                     Kate::TextLineData *textLine,
                     const Kate::TextLineData *nextLine,
                     bool &ctxChanged,
                     int tabWidth = 0);
    /**
     * Saves the attribute definitions to the config file.
     *
     * @param schema The id of the schema group to save
     * @param list QList<KateExtendedAttribute::Ptr> containing the data to be used
     */
    void setKateExtendedAttributeList(const QString &schema, QVector<KTextEditor::Attribute::Ptr> &list,
                                      KConfig *cfg = nullptr /*if 0  standard kate config*/, bool writeDefaultsToo = false);

    const QString &name() const
    {
        return iName;
    }
    const QString &section() const
    {
        return iSection;
    }
    bool hidden() const
    {
        return iHidden;
    }
    const QString &style() const
    {
        return iStyle;
    }
    const QString &getIdentifier() const
    {
        return identifier;
    }

    /**
     * @return true if the character @p c is not a deliminator character
     *     for the corresponding highlight.
     */
    bool isInWord(QChar c, int attrib = 0) const;

    /**
     * @return true if the character @p c is a wordwrap deliminator as specified
     * in the general keyword section of the xml file.
     */
    bool canBreakAt(QChar c, int attrib = 0) const;
    /**
     *
     */
    const QVector<QRegularExpression> &emptyLines(int attribute = 0) const;

    bool isEmptyLine(const Kate::TextLineData *textline) const;

    /**
    * @return true if @p beginAttr and @p endAttr are members of the same
    * highlight, and there are comment markers of either type in that.
    */
    bool canComment(int startAttr, int endAttr) const;

    /**
     * @return the mulitiline comment start marker for the highlight
     * corresponding to @p attrib.
     */
    QString getCommentStart(int attrib = 0) const;

    /**
     * @return the muiltiline comment end marker for the highlight corresponding
     * to @p attrib.
     */
    QString getCommentEnd(int attrib = 0) const;

    /**
     * @return the single comment marker for the highlight corresponding
     * to @p attrib.
     */
    QString getCommentSingleLineStart(int attrib = 0) const;

    const QHash<QString, QChar> &characterEncodings(int attrib = 0) const;

    /**
     * @return the single comment marker position for the highlight corresponding
     * to @p attrib.
     */
    KSyntaxHighlighting::CommentPosition getCommentSingleLinePosition(int attrib = 0) const;

    bool attributeRequiresSpellchecking(int attr);

    /**
     * map attribute to its name
     * @return name of the attribute
     */
    QString nameForAttrib(int attrib) const;

    /**
     * Get attribute for the given cursor position.
     * @param doc document to use
     * @param cursor cursor position in the given document
     * @return attribute valid at that location, default is 0
     */
    int attributeForLocation(KTextEditor::DocumentPrivate* doc, const KTextEditor::Cursor& cursor);

    /**
     * Get all keywords valid for the given cursor position.
     * @param doc document to use
     * @param cursor cursor position in the given document
     * @return all keywords valid at that location
     */
    QStringList keywordsForLocation(KTextEditor::DocumentPrivate* doc, const KTextEditor::Cursor& cursor);

    /**
     * Is spellchecking required for the tiven cursor position?
     * @param doc document to use
     * @param cursor cursor position in the given document
     * @return spell checking required?
     */
    bool spellCheckingRequiredForLocation(KTextEditor::DocumentPrivate* doc, const KTextEditor::Cursor& cursor);

    /**
     * Get highlighting mode for the given cursor position.
     * @param doc document to use
     * @param cursor cursor position in the given document
     * @return mode valid at that location
     */
    QString higlightingModeForLocation(KTextEditor::DocumentPrivate* doc, const KTextEditor::Cursor& cursor);

    KTextEditor::DefaultStyle defaultStyleForAttribute(int attr) const;

    void clearAttributeArrays();

    QVector<KTextEditor::Attribute::Ptr> attributes(const QString &schema);

    inline bool noHighlighting() const
    {
        return noHl;
    }

    /**
     * Indentation mode, e.g. c-style, ....
     * @return indentation mode
     */
    const QString &indentation() const
    {
        return m_indentation;
    }

    void getKateExtendedAttributeList(const QString &schema, QVector<KTextEditor::Attribute::Ptr> &, KConfig *cfg = nullptr);
    void getKateExtendedAttributeListCopy(const QString &schema, QVector<KTextEditor::Attribute::Ptr> &, KConfig *cfg = nullptr);

    const QHash<QString, QChar> &getCharacterEncodings(int attrib) const;
    const KatePrefixStore &getCharacterEncodingsPrefixStore(int attrib) const;
    const QHash<QChar, QString> &getReverseCharacterEncodings(int attrib) const;

    /**
     * Returns a list of names of embedded modes.
     */
    QStringList getEmbeddedHighlightingModes() const;

private:
    /**
     * create list of attributes from internal formats with properties as defined in syntax file
     * @return attributes list with attributes as defined in syntax file
     */
    QVector<KTextEditor::Attribute::Ptr> attributesForDefinition();

    int sanitizeFormatIndex(int attrib) const;

private:
    QStringList embeddedHighlightingModes;

    bool noHl = true;
    bool folding = false;

    QString iName;
    QString iSection;
    bool iHidden = false;
    QString identifier;
    QString iStyle;

    /**
     * Indentation mode, e.g. c-style, ....
     */
    QString m_indentation;

    bool m_foldingIndentationSensitive = false;

    // map schema name to attributes...
    QHash< QString, QVector<KTextEditor::Attribute::Ptr> > m_attributeArrays;

    /**
     * This class holds the additional properties for one highlight
     * definition, such as comment strings, deliminators etc.
     */
    class HighlightPropertyBag
    {
    public:
        KSyntaxHighlighting::Definition definition;
        QString singleLineCommentMarker;
        QString multiLineCommentStart;
        QString multiLineCommentEnd;
        KSyntaxHighlighting::CommentPosition singleLineCommentPosition;
        QVector<QRegularExpression> emptyLines;
        QHash<QString, QChar> characterEncodings;
        KatePrefixStore characterEncodingsPrefixStore;
        QHash<QChar, QString> reverseCharacterEncodings;
    };

public:
    inline bool foldingIndentationSensitive()
    {
        return m_foldingIndentationSensitive;
    }
    inline bool allowsFolding()
    {
        return folding;
    }

    /**
     * Highlight properties for this definition and each included highlight definition.
     */
    std::vector<HighlightPropertyBag> m_properties;

    /**
     * all formats for the highlighting definition of this highlighting
     * includes included formats
     */
    std::vector<KSyntaxHighlighting::Format> m_formats;

    /**
     * for each format, pointer to the matching HighlightPropertyBag in m_properties
     */
    std::vector<const HighlightPropertyBag *> m_propertiesForFormat;

    /**
     * mapping of format id => index into m_formats
     */
    std::unordered_map<quint16, short> m_formatsIdToIndex;

    /**
     * textline to do updates on during doHighlight
     */
    Kate::TextLineData *m_textLineToHighlight = nullptr;

    /**
     * check if the folding begin/ends are balanced!
     * updated during doHighlight
     */
    QHash<int, int> m_foldingStartToCount;
};

#endif

