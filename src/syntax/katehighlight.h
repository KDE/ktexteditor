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

#ifndef __KATE_HIGHLIGHT_H__
#define __KATE_HIGHLIGHT_H__

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
#include <QLinkedList>

#include <unordered_map>

class KConfig;

namespace KTextEditor {
    class DocumentPrivate;
}

class KateHighlighting : private KSyntaxHighlighting::AbstractHighlighter
{
public:
    KateHighlighting(const KSyntaxHighlighting::Definition &def);
    ~KateHighlighting();

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
    virtual void applyFormat(int offset, int length, const KSyntaxHighlighting::Format &format) override;

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
    virtual void applyFolding(int offset, int length, KSyntaxHighlighting::FoldingRegion region) override;

private:
    /**
     * this method frees mem ;)
     * used by the destructor and done(), therefor
     * not only delete elements but also reset the array
     * sizes, as we will reuse this object later and refill ;)
     */
    void cleanup();

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
    void setKateExtendedAttributeList(const QString &schema, QList<KTextEditor::Attribute::Ptr> &list,
                                      KConfig *cfg = nullptr /*if 0  standard kate config*/, bool writeDefaultsToo = false);

    const QString &name() const
    {
        return iName;
    }
    const QString &nameTranslated() const
    {
        return iNameTranslated;
    }
    const QString &section() const
    {
        return iSection;
    }
    bool hidden() const
    {
        return iHidden;
    }
    const QString &version() const
    {
        return iVersion;
    }
    const QString &style() const
    {
        return iStyle;
    }
    const QString &author() const
    {
        return iAuthor;
    }
    const QString &license() const
    {
        return iLicense;
    }
    const QString &getIdentifier() const
    {
        return identifier;
    }
    void use();
    void reload();

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
    QLinkedList<QRegularExpression> emptyLines(int attribute = 0) const;

    bool isEmptyLine(const Kate::TextLineData *textline) const;

    /**
    * @return true if @p beginAttr and @p endAttr are members of the same
    * highlight, and there are comment markers of either type in that.
    */
    bool canComment(int startAttr, int endAttr) const;

    /**
    * @return 0 if highlighting which attr is a member of does not
    * define a comment region, otherwise the region is returned
    */
    signed char commentRegion(int attr) const;

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
     * This enum is used for storing the information where a single line comment marker should be inserted
     */
    enum CSLPos { CSLPosColumn0 = 0, CSLPosAfterWhitespace = 1};

    /**
     * @return the single comment marker position for the highlight corresponding
     * to @p attrib.
     */
    CSLPos getCommentSingleLinePosition(int attrib = 0) const;

    bool attributeRequiresSpellchecking(int attr);

    /**
     * map attribute to its highlighting file.
     * the returned string is used as key for m_additionalData.
     */
    QString hlKeyForAttrib(int attrib) const;

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

    QList<KTextEditor::Attribute::Ptr> attributes(const QString &schema);

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

    void getKateExtendedAttributeList(const QString &schema, QList<KTextEditor::Attribute::Ptr> &, KConfig *cfg = nullptr);
    void getKateExtendedAttributeListCopy(const QString &schema, QList<KTextEditor::Attribute::Ptr> &, KConfig *cfg = nullptr);

    const QHash<QString, QChar> &getCharacterEncodings(int attrib) const;
    const KatePrefixStore &getCharacterEncodingsPrefixStore(int attrib) const;
    const QHash<QChar, QString> &getReverseCharacterEncodings(int attrib) const;
    int getEncodedCharactersInsertionPolicy(int attrib) const;

    /**
     * Returns a list of names of embedded modes.
     */
    QStringList getEmbeddedHighlightingModes() const;

private:
    /**
      * 'encoding' must not contain new line characters, i.e. '\n' or '\r'!
      **/
    void addCharacterEncoding(const QString &key, const QString &encoding, const QChar &c);

private:
    void init();

    QStringList embeddedHighlightingModes;
    QStringList RegionList;
    QStringList ContextNameList;

    bool noHl = true;
    bool folding = false;
    QString deliminator;

    QString iName;
    QString iNameTranslated;
    QString iSection;
    bool iHidden = false;
    QString identifier;
    QString iVersion;
    QString iStyle;
    QString iAuthor;
    QString iLicense;

    /**
     * Indentation mode, e.g. c-style, ....
     */
    QString m_indentation;

    int refCount = 0;

    bool m_foldingIndentationSensitive = false;

    // map schema name to attributes...
    QHash< QString, QList<KTextEditor::Attribute::Ptr> > m_attributeArrays;

    /**
     * This class holds the additional properties for one highlight
     * definition, such as comment strings, deliminators etc.
     *
     * When a highlight is added, a instance of this class is appended to
     * m_additionalData, and the current position in the attrib and context
     * arrays are stored in the indexes for look up. You can then use
     * hlKeyForAttrib to find the relevant instance of this
     * class from m_additionalData.
     *
     * If you need to add a property to a highlight, add it here.
     */
    class HighlightPropertyBag
    {
    public:
        QString singleLineCommentMarker;
        QString multiLineCommentStart;
        QString multiLineCommentEnd;
        QString multiLineRegion;
        CSLPos  singleLineCommentPosition;
        QString deliminator;
        QString wordWrapDeliminator;
        QLinkedList<QRegularExpression> emptyLines;
        QHash<QString, QChar> characterEncodings;
        KatePrefixStore characterEncodingsPrefixStore;
        QHash<QChar, QString> reverseCharacterEncodings;
        int encodedCharactersInsertionPolicy;
    };

    /**
     * Highlight properties for each included highlight definition.
     * The key is the identifier
     */
    QHash<QString, HighlightPropertyBag> m_additionalData;

    /**
     * Fast lookup of hl properties, based on attribute index
     * The key is the starting index in the attribute array for each file.
     * @see hlKeyForAttrib
     */
    QMap<int, QString> m_hlIndex;

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
     * all formats for the highlighting definition of this highlighting
     * includes included formats
     */
    std::vector<KSyntaxHighlighting::Format> m_formats;

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
     * constructed on demand!
     */
    QHash<int, int> *m_foldingStartToCount = nullptr;
};

#endif

