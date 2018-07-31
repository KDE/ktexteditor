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

class KateHlContext;
class KateHlItem;
class KateHlIncludeRule;
class KateSyntaxModeListItem;
class KateSyntaxContextData;
namespace KTextEditor {
    class DocumentPrivate;
}

// same as in kmimemagic, no need to feed more data
#define KATE_HL_HOWMANY 1024

// min. x seconds between two dynamic contexts reset
#define KATE_DYNAMIC_CONTEXTS_RESET_DELAY (30 * 1000)

/**
 * describe a modification of the context stack
 */
class KateHlContextModification
{
public:
    enum modType {
        doNothing = 0,
        doPush = 1,
        doPops = 2,
        doPopsAndPush = 3
    };

    /**
     * Constructor
     * @param _newContext new context to push on stack
     * @param _pops number of contexts to remove from stack in advance
     */
    KateHlContextModification(int _newContext = -1, int _pops = 0) : newContext(_newContext), pops(_pops)     //krazy:exclude=explicit
    {
        if (newContext >= 0 && pops == 0) {
            type = doPush;
        } else if (newContext < 0 && pops > 0) {
            type = doPops;
        } else if (newContext >= 0 && pops > 0) {
            type = doPopsAndPush;
        } else {
            type = doNothing;
        }
    }

public:
    /**
     * indicates what this modification does, for speed
     */
    char type = doNothing;

    /**
     * new context to push on the stack
     * if this is < 0, push nothing on the stack
     */
    int newContext;

    /**
     * number of contexts to pop from the stack
     * before pushing a new context on it
     */
    int pops;
};

class KateEmbeddedHlInfo
{
public:
    KateEmbeddedHlInfo()
    {
        loaded = false;
        context0 = -1;
    }
    KateEmbeddedHlInfo(bool l, int ctx0)
    {
        loaded = l;
        context0 = ctx0;
    }

public:
    bool loaded;
    int context0;
};

// some typedefs
typedef QList<KateHlIncludeRule *> KateHlIncludeRules;
typedef QMap<QString, KateEmbeddedHlInfo> KateEmbeddedHlInfos;
typedef QMap<KateHlContextModification *, QString> KateHlUnresolvedCtxRefs;

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
    struct ContextChange {
        KateHlContext* toContext;
        int pos;
    };

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
                     int tabWidth = 0,
                     QVector<ContextChange>* contextChanges = nullptr);
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

    /**
    * @return the attribute for @p context.
    */
    int attribute(int context) const;

    bool attributeRequiresSpellchecking(int attr);

    /**
     * map attribute to its highlighting file.
     * the returned string is used as key for m_additionalData.
     */
    QString hlKeyForAttrib(int attrib) const;
    QString hlKeyForContext(int attrib) const;

    KateHlContext* contextForLocation(KTextEditor::DocumentPrivate* doc, const KTextEditor::Cursor& cursor);

    KTextEditor::DefaultStyle defaultStyleForAttribute(int attr) const;

    void clearAttributeArrays();

    QList<KTextEditor::Attribute::Ptr> attributes(const QString &schema);

    inline bool noHighlighting() const
    {
        return noHl;
    }

    // be carefull: all documents hl should be invalidated after calling this method!
    void dropDynamicContexts();

    QString indentation()
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

    KateHlContext *contextNum(int n) const;

private:
    /**
      * 'encoding' must not contain new line characters, i.e. '\n' or '\r'!
      **/
    void addCharacterEncoding(const QString &key, const QString &encoding, const QChar &c);

private:
    void init();
    void makeContextList();
    void handleKateHlIncludeRules();
    void handleKateHlIncludeRulesRecursive(int index, KateHlIncludeRules *list);
    int addToContextList(const QString &ident, int ctx0);
    void addToKateExtendedAttributeList();
    void createKateExtendedAttribute(QList<KTextEditor::Attribute::Ptr> &list);
    void readGlobalKeywordConfig();
    void readWordWrapConfig();
    void readCommentConfig();
    void readEmptyLineConfig();
    void readIndentationConfig();
    void readFoldingConfig();
    void readSpellCheckingConfig();

    KateHlItem *createKateHlItem(KateSyntaxContextData *data, QList<KTextEditor::Attribute::Ptr> &iDl, QStringList *RegionList, QStringList *ContextList);
    int lookupAttrName(const QString &name, QList<KTextEditor::Attribute::Ptr> &iDl);

    void createContextNameList(QStringList *ContextNameList, int ctx0);
    KateHlContextModification getContextModificationFromString(QStringList *ContextNameList, QString tmpLineEndContext,/*NO CONST*/ QString &unres);

    QList<KTextEditor::Attribute::Ptr> internalIDList;

    QVector<KateHlContext *> m_contexts;

    QMap< QPair<KateHlContext *, QString>, short> dynamicCtxs;

    // make them pointers perhaps
    // NOTE: gets cleaned once makeContextList() finishes
    KateEmbeddedHlInfos embeddedHls;
    // valid once makeContextList() finishes
    QStringList embeddedHighlightingModes;
    KateHlUnresolvedCtxRefs unresolvedContextReferences;
    QStringList RegionList;
    QStringList ContextNameList;

    bool noHl;
    bool folding;
    bool casesensitive;
    QString weakDeliminator;
    QString deliminator;

    QString iName;
    QString iNameTranslated;
    QString iSection;
    bool iHidden;
    QString identifier;
    QString iVersion;
    QString iStyle;
    QString iAuthor;
    QString iLicense;
    QString m_indentation;
    int refCount;
    int startctx, base_startctx;

    QString errorsAndWarnings;
    QString buildIdentifier;
    QString buildPrefix;
    bool building;
    uint buildContext0Offset;
    KateHlIncludeRules includeRules;
    bool m_foldingIndentationSensitive;

    // map schema name to attributes...
    QHash< QString, QList<KTextEditor::Attribute::Ptr> > m_attributeArrays;

    // list of all created items to delete them later
    QList<KateHlItem *> m_hlItemCleanupList;

    /**
     * This class holds the additional properties for one highlight
     * definition, such as comment strings, deliminators etc.
     *
     * When a highlight is added, a instance of this class is appended to
     * m_additionalData, and the current position in the attrib and context
     * arrays are stored in the indexes for look up. You can then use
     * hlKeyForAttrib or hlKeyForContext to find the relevant instance of this
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
    QHash<QString, HighlightPropertyBag *> m_additionalData;

    /**
     * Fast lookup of hl properties, based on attribute index
     * The key is the starting index in the attribute array for each file.
     * @see hlKeyForAttrib
     */
    QMap<int, QString> m_hlIndex;
    QMap<int, QString> m_ctxIndex;

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
    QVector<KSyntaxHighlighting::Format> m_formats;

    /**
     * mapping of format id => index into m_formats
     */
    std::unordered_map<quint16, short> m_formatsIdToIndex;

    /**
     * textline to do updates on during doHighlight
     */
    Kate::TextLineData *m_textLineToHighlight = nullptr;
};

#endif

