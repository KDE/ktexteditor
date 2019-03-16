/* This file is part of the KDE libraries
   Copyright (C) 2007 Mirko Stocker <me@misto.ch>
   Copyright (C) 2007 Matthew Woehlke <mw_triad@users.sourceforge.net>
   Copyright (C) 2003, 2004 Anders Lund <anders@alweb.dk>
   Copyright (C) 2003 Hamish Rodda <rodda@kde.org>
   Copyright (C) 2001,2002 Joseph Wenninger <jowenn@kde.org>
   Copyright (C) 2001 Christoph Cullmann <cullmann@kde.org>
   Copyright (C) 1999 Jochen Wilhelmy <digisnap@cs.tu-berlin.de>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License version 2 as published by the Free Software Foundation.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

//BEGIN INCLUDES
#include "katehighlight.h"

#include "katetextline.h"
#include "katedocument.h"
#include "katerenderer.h"
#include "kateglobal.h"
#include "kateschema.h"
#include "kateconfig.h"
#include "kateextendedattribute.h"
#include "katepartdebug.h"
#include "katedefaultcolors.h"

#include <KConfig>
#include <KConfigGroup>
#include <KMessageBox>

#include <QSet>
#include <QStringList>
#include <QTextStream>
#include <QVarLengthArray>
#include <QAction>
#include <QApplication>
//END

//BEGIN STATICS
namespace {

/**
 * convert from KSyntaxHighlighting => KTextEditor type
 * special handle non-1:1 things
 */
inline KTextEditor::DefaultStyle textStyleToDefaultStyle(const KSyntaxHighlighting::Theme::TextStyle textStyle)
{
    // handle deviations
    if (textStyle == KSyntaxHighlighting::Theme::Error) {
        return KTextEditor::dsError;
    }
    if (textStyle == KSyntaxHighlighting::Theme::Others) {
        return KTextEditor::dsOthers;
    }

    // else: simple cast
    return static_cast<KTextEditor::DefaultStyle>(textStyle);
}

}
//END

//BEGIN KateHighlighting
KateHighlighting::KateHighlighting(const KSyntaxHighlighting::Definition &def)
{
    /**
     * get name and section, always works
     */
    iName = def.name();
    iSection = def.translatedSection();

    /**
     * get all included definitions, e.g. PHP for HTML highlighting
     */
    auto definitions = def.includedDefinitions();

    /**
     * handle the "no highlighting" case
     * it's possible to not have any defintions with malformed file
     */
    if (!def.isValid() || (definitions.isEmpty() && def.formats().isEmpty())) {
        // dummy properties + formats
        m_properties.resize(1);
        m_propertiesForFormat.push_back(&m_properties[0]);
        m_formats.resize(1);
        m_formatsIdToIndex.insert(std::make_pair(m_formats[0].id(), 0));

        // be done, all below is just for the real highlighting variants
        return;
    }

    /**
     * handle the real highlighting case
     */
    noHl = false;
    iHidden = def.isHidden();
    identifier = def.filePath();
    iStyle = def.style();
    m_indentation = def.indenter();
    folding = def.foldingEnabled();
    m_foldingIndentationSensitive = def.indentationBasedFoldingEnabled();

    /**
     * tell the AbstractHighlighter the definition it shall use
     */
    setDefinition(def);

    /**
     * first: handle only really included definitions
     */
    for (const auto &includedDefinition : definitions)
        embeddedHighlightingModes.push_back(includedDefinition.name());

    /**
     * now: handle all, including this definition itself
     * create the format => attributes mapping
     * collect embedded highlightings, too
     *
     * we start with our definition as we want to have the default format
     * of the initial definition as attribute with index == 0
     *
     * we collect additional properties in the m_properties and
     * map the formats to the right properties in m_propertiesForFormat
     */
    definitions.push_front(definition());
    m_properties.resize(definitions.size());
    size_t propertiesIndex = 0;
    for (const auto & includedDefinition : definitions) {
        auto &properties = m_properties[propertiesIndex];
        properties.definition = includedDefinition;
        for (const auto &emptyLine : includedDefinition.foldingIgnoreList())
            properties.emptyLines.push_back(QRegularExpression(emptyLine));
        properties.singleLineCommentMarker = includedDefinition.singleLineCommentMarker();
        properties.singleLineCommentPosition = includedDefinition.singleLineCommentPosition();
        const auto multiLineComment = includedDefinition.multiLineCommentMarker();
        properties.multiLineCommentStart = multiLineComment.first;
        properties.multiLineCommentEnd = multiLineComment.second;

        // collect character characters
        for (const auto &enc : includedDefinition.characterEncodings()) {
            properties.characterEncodingsPrefixStore.addPrefix(enc.second);
            properties.characterEncodings[enc.second] = enc.first;
            properties.reverseCharacterEncodings[enc.first] = enc.second;
        }

        // collect formats
        for (const auto & format : includedDefinition.formats()) {
            // register format id => internal attributes, we want no clashs
            const auto nextId = m_formats.size();
            m_formatsIdToIndex.insert(std::make_pair(format.id(), nextId));
            m_formats.push_back(format);
            m_propertiesForFormat.push_back(&properties);
        }

        // advance to next properties
        ++propertiesIndex;
    }
}

void KateHighlighting::doHighlight(const Kate::TextLineData *prevLine,
                                   Kate::TextLineData *textLine,
                                   const Kate::TextLineData *nextLine,
                                   bool &ctxChanged,
                                   int tabWidth)
{
    // default: no context change
    ctxChanged = false;

    // no text line => nothing to do
    if (!textLine) {
        return;
    }

    // in all cases, remove old hl, or we will grow to infinite ;)
    textLine->clearAttributesAndFoldings();

    // reset folding start
    textLine->clearMarkedAsFoldingStart();

    // no hl set, nothing to do more than the above cleaning ;)
    if (noHl) {
        return;
    }

    /**
     * ensure we arrive in clean state
     */
    Q_ASSERT(!m_textLineToHighlight);
    Q_ASSERT(m_foldingStartToCount.isEmpty());

    /**
     * highlight the given line via the abstract highlighter
     * a bit ugly: we set the line to highlight as member to be able to update its stats in the applyFormat and applyFolding member functions
     */
    m_textLineToHighlight = textLine;
    const KSyntaxHighlighting::State initialState (!prevLine ? KSyntaxHighlighting::State() : prevLine->highlightingState());
    const KSyntaxHighlighting::State endOfLineState = highlightLine(textLine->string(), initialState);
    m_textLineToHighlight = nullptr;

    /**
     * update highlighting state if needed
     */
    if (textLine->highlightingState() != endOfLineState) {
        textLine->setHighlightingState(endOfLineState);
        ctxChanged = true;
    }

    /**
     * handle folding info computed and cleanup hash again, if there
     * check if folding is not balanced and we have more starts then ends
     * then this line is a possible folding start!
     */
    if (!m_foldingStartToCount.isEmpty()) {
        /**
         * possible folding start, if imbalanced, aka hash not empty!
         */
        textLine->markAsFoldingStartAttribute();

        /**
         * clear hash for next doHighlight
         */
        m_foldingStartToCount.clear();
    }

    /**
     * check for indentation based folding
     */
    if (m_foldingIndentationSensitive && (tabWidth > 0) && !textLine->markedAsFoldingStartAttribute()) {
        /**
         * compute if we increase indentation in next line
         */
        if (endOfLineState.indentationBasedFoldingEnabled() && !isEmptyLine(textLine) && !isEmptyLine(nextLine)
                && (textLine->indentDepth(tabWidth) < nextLine->indentDepth(tabWidth))) {
            textLine->markAsFoldingStartIndentation();
        }
    }

}

void KateHighlighting::applyFormat(int offset, int length, const KSyntaxHighlighting::Format &format)
{
    // WE ATM assume ascending offset order
    Q_ASSERT(m_textLineToHighlight);
    if (!format.isValid()) {
        return;
    }

    // get internal attribute, must exist
    const auto it = m_formatsIdToIndex.find(format.id());
    Q_ASSERT(it != m_formatsIdToIndex.end());

    // remember highlighting info in our textline
    m_textLineToHighlight->addAttribute(Kate::TextLineData::Attribute(offset, length, it->second));
}

void KateHighlighting::applyFolding(int offset, int length, KSyntaxHighlighting::FoldingRegion region)
{
    // WE ATM assume ascending offset order, we add the length to the offset for the folding ends to have ranges spanning the full folding region
    Q_ASSERT(m_textLineToHighlight);
    Q_ASSERT(region.isValid());
    const int foldingValue = (region.type() == KSyntaxHighlighting::FoldingRegion::Begin) ? int(region.id()) : -int(region.id());
    m_textLineToHighlight->addFolding(offset + (region.type() == KSyntaxHighlighting::FoldingRegion::Begin) ? 0 : length, foldingValue);

    /**
     * for each end region, decrement counter for that type, erase if count reaches 0!
     */
    if (foldingValue < 0) {
        QHash<int, int>::iterator end = m_foldingStartToCount.find(-foldingValue);
        if (end != m_foldingStartToCount.end()) {
            if (end.value() > 1) {
                --(end.value());
            } else {
                m_foldingStartToCount.erase(end);
            }
        }
    }

    /**
     * increment counter for each begin region!
     */
    if (foldingValue > 0) {
        ++m_foldingStartToCount[foldingValue];
    }
}

void KateHighlighting::getKateExtendedAttributeList(const QString &schema, QVector<KTextEditor::Attribute::Ptr> &list, KConfig *cfg)
{
    KConfigGroup config(cfg ? cfg : KateHlManager::self()->getKConfig(),
                        QLatin1String("Highlighting ") + iName + QLatin1String(" - Schema ") + schema);

    list = attributesForDefinition();

    foreach (KTextEditor::Attribute::Ptr p, list) {
        Q_ASSERT(p);

        QStringList s = config.readEntry(p->name(), QStringList());

//    qCDebug(LOG_KTE)<<p->name<<s.count();
        if (!s.isEmpty()) {

            while (s.count() < 10) {
                s << QString();
            }
            QString name = p->name();
            bool spellCheck = !p->skipSpellChecking();
            p->clear();
            p->setName(name);
            p->setSkipSpellChecking(!spellCheck);

            QString tmp = s[0]; if (!tmp.isEmpty()) {
                p->setDefaultStyle(static_cast<KTextEditor::DefaultStyle> (tmp.toInt()));
            }

            QRgb col;

            tmp = s[1]; if (!tmp.isEmpty()) {
                col = tmp.toUInt(nullptr, 16); p->setForeground(QColor(col));
            }

            tmp = s[2]; if (!tmp.isEmpty()) {
                col = tmp.toUInt(nullptr, 16); p->setSelectedForeground(QColor(col));
            }

            tmp = s[3]; if (!tmp.isEmpty()) {
                p->setFontBold(tmp != QLatin1String("0"));
            }

            tmp = s[4]; if (!tmp.isEmpty()) {
                p->setFontItalic(tmp != QLatin1String("0"));
            }

            tmp = s[5]; if (!tmp.isEmpty()) {
                p->setFontStrikeOut(tmp != QLatin1String("0"));
            }

            tmp = s[6]; if (!tmp.isEmpty()) {
                p->setFontUnderline(tmp != QLatin1String("0"));
            }

            tmp = s[7]; if (!tmp.isEmpty()) {
                col = tmp.toUInt(nullptr, 16); p->setBackground(QColor(col));
            }

            tmp = s[8]; if (!tmp.isEmpty()) {
                col = tmp.toUInt(nullptr, 16); p->setSelectedBackground(QColor(col));
            }

            tmp = s[9]; if (!tmp.isEmpty() && tmp != QLatin1String("---")) {
                p->setFontFamily(tmp);
            }

        }
    }
}

void KateHighlighting::getKateExtendedAttributeListCopy(const QString &schema, QVector<KTextEditor::Attribute::Ptr> &list, KConfig *cfg)
{
    QVector<KTextEditor::Attribute::Ptr> attributes;
    getKateExtendedAttributeList(schema, attributes, cfg);

    list.clear();

    foreach (const KTextEditor::Attribute::Ptr &attribute, attributes) {
        list.append(KTextEditor::Attribute::Ptr(new KTextEditor::Attribute(*attribute.data())));
    }
}

void KateHighlighting::setKateExtendedAttributeList(const QString &schema, QVector<KTextEditor::Attribute::Ptr> &list, KConfig *cfg, bool writeDefaultsToo)
{
     KConfigGroup config(cfg ? cfg : KateHlManager::self()->getKConfig(),
                        QLatin1String("Highlighting ") + iName + QLatin1String(" - Schema ") + schema);

    QStringList settings;

    KateAttributeList defList;
    KateHlManager::self()->getDefaults(schema, defList);

    foreach (const KTextEditor::Attribute::Ptr &p, list) {
        Q_ASSERT(p);

        settings.clear();
        KTextEditor::DefaultStyle defStyle = p->defaultStyle();
        KTextEditor::Attribute::Ptr a(defList[defStyle]);
        settings << QString::number(p->defaultStyle(), 10);
        settings << (p->hasProperty(QTextFormat::ForegroundBrush) ? QString::number(p->foreground().color().rgb(), 16) : (writeDefaultsToo ? QString::number(a->foreground().color().rgb(), 16) : QString()));
        settings << (p->hasProperty(SelectedForeground) ? QString::number(p->selectedForeground().color().rgb(), 16) : (writeDefaultsToo ? QString::number(a->selectedForeground().color().rgb(), 16) : QString()));
        settings << (p->hasProperty(QTextFormat::FontWeight) ? (p->fontBold() ? QStringLiteral("1") : QStringLiteral("0")) : (writeDefaultsToo ? (a->fontBold() ? QStringLiteral("1") : QStringLiteral("0")) : QString()));
        settings << (p->hasProperty(QTextFormat::FontItalic) ? (p->fontItalic() ? QStringLiteral("1") : QStringLiteral("0")) : (writeDefaultsToo ? (a->fontItalic() ? QStringLiteral("1") : QStringLiteral("0")) : QString()));
        settings << (p->hasProperty(QTextFormat::FontStrikeOut) ? (p->fontStrikeOut() ? QStringLiteral("1") : QStringLiteral("0")) : (writeDefaultsToo ? (a->fontStrikeOut() ? QStringLiteral("1") : QStringLiteral("0")) : QString()));
        settings << (p->hasProperty(QTextFormat::TextUnderlineStyle) ? (p->fontUnderline() ? QStringLiteral("1") : QStringLiteral("0")) : (writeDefaultsToo ? (a->fontUnderline() ? QStringLiteral("1") : QStringLiteral("0")) : QString()));
        settings << (p->hasProperty(QTextFormat::BackgroundBrush) ? QString::number(p->background().color().rgb(), 16) : ((writeDefaultsToo && a->hasProperty(QTextFormat::BackgroundBrush)) ? QString::number(a->background().color().rgb(), 16) : QString()));
        settings << (p->hasProperty(SelectedBackground) ? QString::number(p->selectedBackground().color().rgb(), 16) : ((writeDefaultsToo && a->hasProperty(SelectedBackground)) ? QString::number(a->selectedBackground().color().rgb(), 16) : QString()));
        settings << (p->hasProperty(QTextFormat::FontFamily) ? (p->fontFamily()) : (writeDefaultsToo ? a->fontFamily() : QString()));
        settings << QStringLiteral("---");
        config.writeEntry(p->name(), settings);
    }
}

int KateHighlighting::sanitizeFormatIndex(int attrib) const
{
    // sanitize, e.g. one could have old hl info with now invalid attribs
    if (attrib < 0 || size_t(attrib) >= m_formats.size()) {
        return 0;
    }
    return attrib;

}

const QHash<QString, QChar> &KateHighlighting::getCharacterEncodings(int attrib) const
{
    return m_propertiesForFormat.at(sanitizeFormatIndex(attrib))->characterEncodings;
}

const KatePrefixStore &KateHighlighting::getCharacterEncodingsPrefixStore(int attrib) const
{
    return m_propertiesForFormat.at(sanitizeFormatIndex(attrib))->characterEncodingsPrefixStore;
}

const QHash<QChar, QString> &KateHighlighting::getReverseCharacterEncodings(int attrib) const
{
    return m_propertiesForFormat.at(sanitizeFormatIndex(attrib))->reverseCharacterEncodings;
}

bool KateHighlighting::attributeRequiresSpellchecking(int attr)
{
    return m_formats[sanitizeFormatIndex(attr)].spellCheck();
}

KTextEditor::DefaultStyle KateHighlighting::defaultStyleForAttribute(int attr) const
{
    return textStyleToDefaultStyle(m_formats[sanitizeFormatIndex(attr)].textStyle());
}

QString KateHighlighting::nameForAttrib(int attrib) const
{
    const auto &format = m_formats.at(sanitizeFormatIndex(attrib));
    return m_propertiesForFormat.at(sanitizeFormatIndex(attrib))->definition.name() + QLatin1Char(':') + QString(format.isValid() ? format.name() : QLatin1String("Normal"));
}

bool KateHighlighting::isInWord(QChar c, int attrib) const
{
    return !m_propertiesForFormat.at(sanitizeFormatIndex(attrib))->definition.isWordDelimiter(c)
           && !c.isSpace()
           && c != QLatin1Char('"') && c != QLatin1Char('\'') && c != QLatin1Char('`');
}

bool KateHighlighting::canBreakAt(QChar c, int attrib) const
{
    return m_propertiesForFormat.at(sanitizeFormatIndex(attrib))->definition.isWordWrapDelimiter(c) && c != QLatin1Char('"') && c != QLatin1Char('\'');
}

const QVector<QRegularExpression> &KateHighlighting::emptyLines(int attrib) const
{
    return m_propertiesForFormat.at(sanitizeFormatIndex(attrib))->emptyLines;
}

bool KateHighlighting::canComment(int startAttrib, int endAttrib) const
{
    const auto startProperties = m_propertiesForFormat.at(sanitizeFormatIndex(startAttrib));
    const auto endProperties = m_propertiesForFormat.at(sanitizeFormatIndex(endAttrib));
    return (startProperties == endProperties &&
            ((!startProperties->multiLineCommentStart.isEmpty() && !startProperties->multiLineCommentEnd.isEmpty()) ||
             !startProperties->singleLineCommentMarker.isEmpty()));
}

QString KateHighlighting::getCommentStart(int attrib) const
{
    return m_propertiesForFormat.at(sanitizeFormatIndex(attrib))->multiLineCommentStart;
}

QString KateHighlighting::getCommentEnd(int attrib) const
{
    return m_propertiesForFormat.at(sanitizeFormatIndex(attrib))->multiLineCommentEnd;
}

QString KateHighlighting::getCommentSingleLineStart(int attrib) const
{
    return m_propertiesForFormat.at(sanitizeFormatIndex(attrib))->singleLineCommentMarker;
}

KSyntaxHighlighting::CommentPosition KateHighlighting::getCommentSingleLinePosition(int attrib) const
{
    return m_propertiesForFormat.at(sanitizeFormatIndex(attrib))->singleLineCommentPosition;
}

const QHash<QString, QChar> &KateHighlighting::characterEncodings(int attrib) const
{
    return  m_propertiesForFormat.at(sanitizeFormatIndex(attrib))->characterEncodings;
}

void KateHighlighting::clearAttributeArrays()
{
    // just clear the hashed attributes, we create them lazy again
    m_attributeArrays.clear();
}

QVector<KTextEditor::Attribute::Ptr> KateHighlighting::attributesForDefinition()
{
     /**
     * create list of all known things
     */
    QVector<KTextEditor::Attribute::Ptr> array;
    for (const auto &format : m_formats) {
        /**
         * FIXME: atm we just set some theme here for later color generation
         */
        setTheme(KateHlManager::self()->repository().defaultTheme(KSyntaxHighlighting::Repository::LightTheme));

        /**
         * create a KTextEditor attribute matching the given format
         */
        KTextEditor::Attribute::Ptr newAttribute(new KTextEditor::Attribute(nameForAttrib(array.size()), textStyleToDefaultStyle(format.textStyle())));

        if (format.hasTextColor(theme())) {
            newAttribute->setForeground(format.textColor(theme()));
            newAttribute->setSelectedForeground(format.selectedTextColor(theme()));
        }

        if (format.hasBackgroundColor(theme())) {
            newAttribute->setBackground(format.backgroundColor(theme()));
            newAttribute->setSelectedBackground(format.selectedBackgroundColor(theme()));
        }

        if (format.isBold(theme())) {
            newAttribute->setFontBold(true);
        }

        if (format.isItalic(theme())) {
            newAttribute->setFontItalic(true);
        }

        if (format.isUnderline(theme())) {
            newAttribute->setFontUnderline(true);
        }

        if (format.isStrikeThrough(theme())) {
            newAttribute->setFontStrikeOut(true);
        }

        newAttribute->setSkipSpellChecking(format.spellCheck());

        array.append(newAttribute);
    }
    return array;
}

QVector<KTextEditor::Attribute::Ptr> KateHighlighting::attributes(const QString &schema)
{
    // found it, already floating around
    if (m_attributeArrays.contains(schema)) {
        return m_attributeArrays[schema];
    }

    // k, schema correct, let create the data
    QVector<KTextEditor::Attribute::Ptr> array;
    KateAttributeList defaultStyleList;

    KateHlManager::self()->getDefaults(schema, defaultStyleList);

    QVector<KTextEditor::Attribute::Ptr> itemDataList;
    getKateExtendedAttributeList(schema, itemDataList);

    uint nAttribs = itemDataList.count();
    for (uint z = 0; z < nAttribs; z++) {
        KTextEditor::Attribute::Ptr itemData = itemDataList.at(z);
        KTextEditor::Attribute::Ptr newAttribute(new KTextEditor::Attribute(*defaultStyleList.at(itemData->defaultStyle())));

        if (itemData && itemData->hasAnyProperty()) {
            *newAttribute += *itemData;
        }

        array.append(newAttribute);
    }

    m_attributeArrays.insert(schema, array);

    return array;
}

QStringList KateHighlighting::getEmbeddedHighlightingModes() const
{
    return embeddedHighlightingModes;
}

bool KateHighlighting::isEmptyLine(const Kate::TextLineData *textline) const
{
    const QString &txt = textline->string();
    if (txt.isEmpty()) {
        return true;
    }

    const auto &l = emptyLines(textline->attribute(0));
    if (l.isEmpty()) {
        return false;
    }

    foreach (const QRegularExpression &re, l) {
        const QRegularExpressionMatch match = re.match (txt, 0, QRegularExpression::NormalMatch, QRegularExpression::AnchoredMatchOption);
        if (match.hasMatch() && match.capturedLength() == txt.length()) {
            return true;
        }
    }

    return false;
}

int KateHighlighting::attributeForLocation(KTextEditor::DocumentPrivate* doc, const KTextEditor::Cursor& cursor)
{
     // Validate parameters to prevent out of range access
    if (cursor.line() < 0 || cursor.line() >= doc->lines() || cursor.column() < 0) {
        return 0;
    }

    // get highlighted line
    Kate::TextLine tl = doc->kateTextLine(cursor.line());

    // make sure the textline is a valid pointer
    if (!tl) {
        return 0;
    }

    /**
     * either get char attribute or attribute of context still active at end of line
     */
    if (cursor.column() < tl->length()) {
        return sanitizeFormatIndex(tl->attribute(cursor.column()));
    } else if (cursor.column() >= tl->length()) {
        if (!tl->attributesList().isEmpty()) {
            return sanitizeFormatIndex(tl->attributesList().back().attributeValue);
        }
    }
    return 0;
}

QStringList KateHighlighting::keywordsForLocation(KTextEditor::DocumentPrivate* doc, const KTextEditor::Cursor& cursor)
{
    // FIXME-SYNTAX: was before more precise, aka context level
    const auto &def =  m_propertiesForFormat.at(attributeForLocation(doc, cursor))->definition;
    QStringList keywords;
    for (const auto &keylist : def.keywordLists()) {
        keywords += def.keywordList(keylist);
    }
    return keywords;
}

bool KateHighlighting::spellCheckingRequiredForLocation(KTextEditor::DocumentPrivate* doc, const KTextEditor::Cursor& cursor)
{
    return m_formats.at(attributeForLocation(doc, cursor)).spellCheck();
}

QString KateHighlighting::higlightingModeForLocation(KTextEditor::DocumentPrivate* doc, const KTextEditor::Cursor& cursor)
{
    return m_propertiesForFormat.at(attributeForLocation(doc, cursor))->definition.name();
}

//END

