/* This file is part of the KDE libraries
   Copyright (C) 2007 Matthew Woehlke <mw_triad@users.sourceforge.net>
   Copyright (C) 2003, 2004 Anders Lund <anders@alweb.dk>
   Copyright (C) 2003 Hamish Rodda <rodda@kde.org>
   Copyright (C) 2001,2002 Joseph Wenninger <jowenn@kde.org>
   Copyright (C) 2001 Christoph Cullmann <cullmann@kde.org>
   Copyright (C) 1999 Jochen Wilhelmy <digisnap@cs.tu-berlin.de>

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

//BEGIN INCLUDES
#include "katesyntaxmanager.h"

#include "katetextline.h"
#include "katedocument.h"
#include "katerenderer.h"
#include "kateglobal.h"
#include "kateschema.h"
#include "kateconfig.h"
#include "kateextendedattribute.h"
#include "katehighlight.h"
#include "katepartdebug.h"
#include "katedefaultcolors.h"

#include <KConfigGroup>
#include <KColorUtils>
//END

using namespace KTextEditor;

//BEGIN KateHlManager
KateHlManager::KateHlManager()
    : QObject()
    , m_config(KTextEditor::EditorPrivate::unitTestMode() ? QString() :QStringLiteral("katesyntaxhighlightingrc")
    , KTextEditor::EditorPrivate::unitTestMode() ? KConfig::SimpleConfig : KConfig::NoGlobals) // skip config for unit tests!
{
}

KateHlManager *KateHlManager::self()
{
    return KTextEditor::EditorPrivate::self()->hlManager();
}

KateHighlighting *KateHlManager::getHl(int n)
{
    // default to "None", must exist
    if (n < 0 || n >= modeList().count()) {
        n = nameFind(QStringLiteral("None"));
        Q_ASSERT(n >= 0);
    }

    // construct it on demand
    if (!m_hlDict.contains(modeList().at(n).name())) {
        m_hlDict[modeList().at(n).name()] = std::make_shared<KateHighlighting>(modeList().at(n));
    }
    return m_hlDict[modeList().at(n).name()].get();
}

int KateHlManager::nameFind(const QString &name)
{
    for (int i = 0; i < modeList().count(); ++i) {
        if (modeList().at(i).name().compare(name, Qt::CaseInsensitive) == 0) {
            return i;
        }
    }

    return -1;
}

int KateHlManager::defaultStyleCount()
{
    return KTextEditor::dsError + 1;
}

QString KateHlManager::defaultStyleName(int n, bool translateNames)
{
    static QStringList names;
    static QStringList translatedNames;

    if (names.isEmpty()) {
        names << QStringLiteral("Normal");
        names << QStringLiteral("Keyword");
        names << QStringLiteral("Function");
        names << QStringLiteral("Variable");
        names << QStringLiteral("Control Flow");
        names << QStringLiteral("Operator");
        names << QStringLiteral("Built-in");
        names << QStringLiteral("Extension");
        names << QStringLiteral("Preprocessor");
        names << QStringLiteral("Attribute");

        names << QStringLiteral("Character");
        names << QStringLiteral("Special Character");
        names << QStringLiteral("String");
        names << QStringLiteral("Verbatim String");
        names << QStringLiteral("Special String");
        names << QStringLiteral("Import");

        names << QStringLiteral("Data Type");
        names << QStringLiteral("Decimal/Value");
        names << QStringLiteral("Base-N Integer");
        names << QStringLiteral("Floating Point");
        names << QStringLiteral("Constant");

        names << QStringLiteral("Comment");
        names << QStringLiteral("Documentation");
        names << QStringLiteral("Annotation");
        names << QStringLiteral("Comment Variable");
        // this next one is for denoting the beginning/end of a user defined folding region
        names << QStringLiteral("Region Marker");
        names << QStringLiteral("Information");
        names << QStringLiteral("Warning");
        names << QStringLiteral("Alert");

        names << QStringLiteral("Others");
        // this one is for marking invalid input
        names << QStringLiteral("Error");

        translatedNames << i18nc("@item:intable Text context", "Normal");
        translatedNames << i18nc("@item:intable Text context", "Keyword");
        translatedNames << i18nc("@item:intable Text context", "Function");
        translatedNames << i18nc("@item:intable Text context", "Variable");
        translatedNames << i18nc("@item:intable Text context", "Control Flow");
        translatedNames << i18nc("@item:intable Text context", "Operator");
        translatedNames << i18nc("@item:intable Text context", "Built-in");
        translatedNames << i18nc("@item:intable Text context", "Extension");
        translatedNames << i18nc("@item:intable Text context", "Preprocessor");
        translatedNames << i18nc("@item:intable Text context", "Attribute");

        translatedNames << i18nc("@item:intable Text context", "Character");
        translatedNames << i18nc("@item:intable Text context", "Special Character");
        translatedNames << i18nc("@item:intable Text context", "String");
        translatedNames << i18nc("@item:intable Text context", "Verbatim String");
        translatedNames << i18nc("@item:intable Text context", "Special String");
        translatedNames << i18nc("@item:intable Text context", "Imports, Modules, Includes");

        translatedNames << i18nc("@item:intable Text context", "Data Type");
        translatedNames << i18nc("@item:intable Text context", "Decimal/Value");
        translatedNames << i18nc("@item:intable Text context", "Base-N Integer");
        translatedNames << i18nc("@item:intable Text context", "Floating Point");
        translatedNames << i18nc("@item:intable Text context", "Constant");

        translatedNames << i18nc("@item:intable Text context", "Comment");
        translatedNames << i18nc("@item:intable Text context", "Documentation");
        translatedNames << i18nc("@item:intable Text context", "Annotation");
        translatedNames << i18nc("@item:intable Text context", "Comment Variable");
        // this next one is for denoting the beginning/end of a user defined folding region
        translatedNames << i18nc("@item:intable Text context", "Region Marker");
        translatedNames << i18nc("@item:intable Text context", "Information");
        translatedNames << i18nc("@item:intable Text context", "Warning");
        translatedNames << i18nc("@item:intable Text context", "Alert");

        translatedNames << i18nc("@item:intable Text context", "Others");
        // this one is for marking invalid input
        translatedNames << i18nc("@item:intable Text context", "Error");
    }

    // sanity checks
    Q_ASSERT(n >= 0);
    Q_ASSERT(n < names.size());

    return translateNames ? translatedNames[n] : names[n];
}

int KateHlManager::defaultStyleNameToIndex(const QString &name)
{
    //
    // Normal text and source code
    //
    if (name == QLatin1String("dsNormal")) {
        return KTextEditor::dsNormal;
    } else if (name == QLatin1String("dsKeyword")) {
        return KTextEditor::dsKeyword;
    } else if (name == QLatin1String("dsFunction")) {
        return KTextEditor::dsFunction;
    } else if (name == QLatin1String("dsVariable")) {
        return KTextEditor::dsVariable;
    } else if (name == QLatin1String("dsControlFlow")) {
        return KTextEditor::dsControlFlow;
    } else if (name == QLatin1String("dsOperator")) {
        return KTextEditor::dsOperator;
    } else if (name == QLatin1String("dsBuiltIn")) {
        return KTextEditor::dsBuiltIn;
    } else if (name == QLatin1String("dsExtension")) {
        return KTextEditor::dsExtension;
    } else if (name == QLatin1String("dsPreprocessor")) {
        return KTextEditor::dsPreprocessor;
    } else if (name == QLatin1String("dsAttribute")) {
        return KTextEditor::dsAttribute;
    }

    //
    // Strings & Characters
    //
    if (name == QLatin1String("dsChar")) {
        return KTextEditor::dsChar;
    } else if (name == QLatin1String("dsSpecialChar")) {
        return KTextEditor::dsSpecialChar;
    } else if (name == QLatin1String("dsString")) {
        return KTextEditor::dsString;
    } else if (name == QLatin1String("dsVerbatimString")) {
        return KTextEditor::dsVerbatimString;
    } else if (name == QLatin1String("dsSpecialString")) {
        return KTextEditor::dsSpecialString;
    } else if (name == QLatin1String("dsImport")) {
        return KTextEditor::dsImport;
    }

    //
    // Numbers, Types & Constants
    //
    if (name == QLatin1String("dsDataType")) {
        return KTextEditor::dsDataType;
    } else if (name == QLatin1String("dsDecVal")) {
        return KTextEditor::dsDecVal;
    } else if (name == QLatin1String("dsBaseN")) {
        return KTextEditor::dsBaseN;
    } else if (name == QLatin1String("dsFloat")) {
        return KTextEditor::dsFloat;
    } else if (name == QLatin1String("dsConstant")) {
        return KTextEditor::dsConstant;
    }

    //
    // Comments & Documentation
    //
    if (name == QLatin1String("dsComment")) {
        return KTextEditor::dsComment;
    } else if (name == QLatin1String("dsDocumentation")) {
        return KTextEditor::dsDocumentation;
    } else if (name == QLatin1String("dsAnnotation")) {
        return KTextEditor::dsAnnotation;
    } else if (name == QLatin1String("dsCommentVar")) {
        return KTextEditor::dsCommentVar;
    } else if (name == QLatin1String("dsRegionMarker")) {
        return KTextEditor::dsRegionMarker;
    } else if (name == QLatin1String("dsInformation")) {
        return KTextEditor::dsInformation;
    } else if (name == QLatin1String("dsWarning")) {
        return KTextEditor::dsWarning;
    } else if (name == QLatin1String("dsAlert")) {
        return KTextEditor::dsAlert;
    }

    //
    // Misc
    //
    if (name == QLatin1String("dsOthers")) {
        return KTextEditor::dsOthers;
    } else if (name == QLatin1String("dsError")) {
        return KTextEditor::dsError;
    }

    return KTextEditor::dsNormal;
}

void KateHlManager::getDefaults(const QString &schema, KateAttributeList &list, KConfig *cfg)
{
    const KColorScheme &scheme(KTextEditor::EditorPrivate::self()->defaultColors().view());
    const KColorScheme &schemeSelected(KTextEditor::EditorPrivate::self()->defaultColors().selection());

    ///NOTE: it's important to append in the order of the KTextEditor::DefaultStyle
    ///      enum, to make KTextEditor::DocumentPrivate::defaultStyle() work properly.

    {
        // dsNormal
        Attribute::Ptr attrib(new KTextEditor::Attribute());
        attrib->setForeground(scheme.foreground().color());
        attrib->setSelectedForeground(schemeSelected.foreground().color());
        list.append(attrib);
    }
    {
        // dsKeyword
        Attribute::Ptr attrib(new KTextEditor::Attribute());
        attrib->setForeground(scheme.foreground().color());
        attrib->setSelectedForeground(schemeSelected.foreground().color());
        attrib->setFontBold(true);
        list.append(attrib);
    }
    {
        // dsFunction
        Attribute::Ptr attrib(new KTextEditor::Attribute());
        attrib->setForeground(scheme.foreground(KColorScheme::NeutralText).color());
        attrib->setSelectedForeground(schemeSelected.foreground(KColorScheme::NeutralText).color());
        list.append(attrib);
    }
    {
        // dsVariable
        Attribute::Ptr attrib(new KTextEditor::Attribute());
        attrib->setForeground(scheme.foreground(KColorScheme::LinkText).color());
        attrib->setSelectedForeground(schemeSelected.foreground(KColorScheme::LinkText).color());
        list.append(attrib);
    }
    {
        // dsControlFlow
        Attribute::Ptr attrib(new KTextEditor::Attribute());
        attrib->setForeground(scheme.foreground().color());
        attrib->setSelectedForeground(schemeSelected.foreground().color());
        attrib->setFontBold(true);
        list.append(attrib);
    }
    {
        // dsOperator
        Attribute::Ptr attrib(new KTextEditor::Attribute());
        attrib->setForeground(scheme.foreground().color());
        attrib->setSelectedForeground(schemeSelected.foreground().color());
        list.append(attrib);
    }
    {
        // dsBuiltIn
        Attribute::Ptr attrib(new KTextEditor::Attribute());
        attrib->setForeground(scheme.foreground(KColorScheme::VisitedText).color());
        attrib->setSelectedForeground(schemeSelected.foreground(KColorScheme::VisitedText).color());
        list.append(attrib);
    }
    {
        // dsExtension
        Attribute::Ptr attrib(new KTextEditor::Attribute());
        attrib->setForeground(QColor(0, 149, 255));
        attrib->setSelectedForeground(schemeSelected.foreground().color());
        attrib->setFontBold(true);
        list.append(attrib);
    }
    {
        // dsPreprocessor
        Attribute::Ptr attrib(new KTextEditor::Attribute());
        attrib->setForeground(scheme.foreground(KColorScheme::PositiveText).color());
        attrib->setSelectedForeground(schemeSelected.foreground(KColorScheme::PositiveText).color());
        list.append(attrib);
    }
    {
        // dsAttribute
        Attribute::Ptr attrib(new KTextEditor::Attribute());
        attrib->setForeground(scheme.foreground(KColorScheme::LinkText).color());
        attrib->setSelectedForeground(schemeSelected.foreground(KColorScheme::LinkText).color());
        list.append(attrib);
    }

    {
        // dsChar
        Attribute::Ptr attrib(new KTextEditor::Attribute());
        attrib->setForeground(scheme.foreground(KColorScheme::ActiveText).color());
        attrib->setSelectedForeground(schemeSelected.foreground(KColorScheme::ActiveText).color());
        list.append(attrib);
    }
    {
        // dsSpecialChar
        Attribute::Ptr attrib(new KTextEditor::Attribute());
        attrib->setForeground(scheme.foreground(KColorScheme::ActiveText).color());
        attrib->setSelectedForeground(schemeSelected.foreground(KColorScheme::ActiveText).color());
        list.append(attrib);
    }
    {
        // dsString
        Attribute::Ptr attrib(new KTextEditor::Attribute());
        attrib->setForeground(scheme.foreground(KColorScheme::NegativeText).color());
        attrib->setSelectedForeground(schemeSelected.foreground(KColorScheme::NegativeText).color());
        list.append(attrib);
    }
    {
        // dsVerbatimString
        Attribute::Ptr attrib(new KTextEditor::Attribute());
        attrib->setForeground(scheme.foreground(KColorScheme::NegativeText).color());
        attrib->setSelectedForeground(schemeSelected.foreground(KColorScheme::NegativeText).color());
        list.append(attrib);
    }
    {
        // dsSpecialString
        Attribute::Ptr attrib(new KTextEditor::Attribute());
        attrib->setForeground(scheme.foreground(KColorScheme::NegativeText).color());
        attrib->setSelectedForeground(schemeSelected.foreground(KColorScheme::NegativeText).color());
        list.append(attrib);
    }
    {
        // dsImport
        Attribute::Ptr attrib(new KTextEditor::Attribute());
        attrib->setForeground(scheme.foreground(KColorScheme::VisitedText).color());
        attrib->setSelectedForeground(schemeSelected.foreground(KColorScheme::VisitedText).color());
        list.append(attrib);
    }

    {
        // dsDataType
        Attribute::Ptr attrib(new KTextEditor::Attribute());
        attrib->setForeground(scheme.foreground(KColorScheme::LinkText).color());
        attrib->setSelectedForeground(schemeSelected.foreground(KColorScheme::LinkText).color());
        list.append(attrib);
    }
    {
        // dsDecVal
        Attribute::Ptr attrib(new KTextEditor::Attribute());
        attrib->setForeground(scheme.foreground(KColorScheme::NeutralText).color());
        attrib->setSelectedForeground(schemeSelected.foreground(KColorScheme::NeutralText).color());
        list.append(attrib);
    }
    {
        // dsBaseN
        Attribute::Ptr attrib(new KTextEditor::Attribute());
        attrib->setForeground(scheme.foreground(KColorScheme::NeutralText).color());
        attrib->setSelectedForeground(schemeSelected.foreground(KColorScheme::NeutralText).color());
        list.append(attrib);
    }
    {
        // dsFloat
        Attribute::Ptr attrib(new KTextEditor::Attribute());
        attrib->setForeground(scheme.foreground(KColorScheme::NeutralText).color());
        attrib->setSelectedForeground(schemeSelected.foreground(KColorScheme::NeutralText).color());
        list.append(attrib);
    }
    {
        // dsConstant
        Attribute::Ptr attrib(new KTextEditor::Attribute());
        attrib->setForeground(scheme.foreground().color());
        attrib->setSelectedForeground(schemeSelected.foreground().color());
        attrib->setFontBold(true);
        list.append(attrib);
    }

    {
        // dsComment
        Attribute::Ptr attrib(new KTextEditor::Attribute());
        attrib->setForeground(scheme.foreground(KColorScheme::InactiveText).color());
        attrib->setSelectedForeground(schemeSelected.foreground(KColorScheme::InactiveText).color());
        list.append(attrib);
    }
    {
        // dsDocumentation
        Attribute::Ptr attrib(new KTextEditor::Attribute());
        attrib->setForeground(scheme.foreground(KColorScheme::NegativeText).color());
        attrib->setSelectedForeground(schemeSelected.foreground(KColorScheme::NegativeText).color());
        list.append(attrib);
    }
    {
        // dsAnnotation
        Attribute::Ptr attrib(new KTextEditor::Attribute());
        attrib->setForeground(scheme.foreground(KColorScheme::VisitedText).color());
        attrib->setSelectedForeground(schemeSelected.foreground(KColorScheme::VisitedText).color());
        list.append(attrib);
    }
    {
        // dsCommentVar
        Attribute::Ptr attrib(new KTextEditor::Attribute());
        attrib->setForeground(scheme.foreground(KColorScheme::VisitedText).color());
        attrib->setSelectedForeground(schemeSelected.foreground(KColorScheme::VisitedText).color());
        list.append(attrib);
    }
    {
        // dsRegionMarker
        Attribute::Ptr attrib(new KTextEditor::Attribute());
        attrib->setForeground(scheme.foreground(KColorScheme::LinkText).color());
        attrib->setSelectedForeground(schemeSelected.foreground(KColorScheme::LinkText).color());
        attrib->setBackground(scheme.background(KColorScheme::LinkBackground).color());
        list.append(attrib);
    }
    {
        // dsInformation
        Attribute::Ptr attrib(new KTextEditor::Attribute());
        attrib->setForeground(scheme.foreground(KColorScheme::NeutralText).color());
        attrib->setSelectedForeground(schemeSelected.foreground(KColorScheme::NeutralText).color());
        list.append(attrib);
    }
    {
        // dsWarning
        Attribute::Ptr attrib(new KTextEditor::Attribute());
        attrib->setForeground(scheme.foreground(KColorScheme::NegativeText).color());
        attrib->setSelectedForeground(schemeSelected.foreground(KColorScheme::NegativeText).color());
        list.append(attrib);
    }
    {
        // dsAlert
        Attribute::Ptr attrib(new KTextEditor::Attribute());
        attrib->setForeground(scheme.foreground(KColorScheme::NegativeText).color());
        attrib->setSelectedForeground(schemeSelected.foreground(KColorScheme::NegativeText).color());
        attrib->setFontBold(true);
        attrib->setBackground(scheme.background(KColorScheme::NegativeBackground).color());
        list.append(attrib);
    }

    {
        // dsOthers
        Attribute::Ptr attrib(new KTextEditor::Attribute());
        attrib->setForeground(scheme.foreground(KColorScheme::PositiveText).color());
        attrib->setSelectedForeground(schemeSelected.foreground(KColorScheme::PositiveText).color());
        list.append(attrib);
    }
    {
        // dsError
        Attribute::Ptr attrib(new KTextEditor::Attribute());
        attrib->setForeground(scheme.foreground(KColorScheme::NegativeText));
        attrib->setSelectedForeground(schemeSelected.foreground(KColorScheme::NegativeText).color());
        attrib->setFontUnderline(true);
        list.append(attrib);
    }

    KConfigGroup config(cfg ? cfg : KateHlManager::self()->self()->getKConfig(),
                        QLatin1String("Default Item Styles - Schema ") + schema);

    for (int z = 0; z < defaultStyleCount(); z++) {
        KTextEditor::Attribute::Ptr i = list.at(z);
        QStringList s = config.readEntry(defaultStyleName(z), QStringList());
        if (!s.isEmpty()) {
            while (s.count() < 9) {
                s << QString();
            }

            QString tmp;
            QRgb col;

            tmp = s[0]; if (!tmp.isEmpty()) {
                col = tmp.toUInt(nullptr, 16); i->setForeground(QColor(col));
            }

            tmp = s[1]; if (!tmp.isEmpty()) {
                col = tmp.toUInt(nullptr, 16); i->setSelectedForeground(QColor(col));
            }

            tmp = s[2]; if (!tmp.isEmpty()) {
                i->setFontBold(tmp != QLatin1String("0"));
            }

            tmp = s[3]; if (!tmp.isEmpty()) {
                i->setFontItalic(tmp != QLatin1String("0"));
            }

            tmp = s[4]; if (!tmp.isEmpty()) {
                i->setFontStrikeOut(tmp != QLatin1String("0"));
            }

            tmp = s[5]; if (!tmp.isEmpty()) {
                i->setFontUnderline(tmp != QLatin1String("0"));
            }

            tmp = s[6]; if (!tmp.isEmpty()) {
                if (tmp != QLatin1String("-")) {
                    col = tmp.toUInt(nullptr, 16);
                    i->setBackground(QColor(col));
                } else {
                    i->clearBackground();
                }
            }
            tmp = s[7]; if (!tmp.isEmpty()) {
                if (tmp != QLatin1String("-")) {
                    col = tmp.toUInt(nullptr, 16);
                    i->setSelectedBackground(QColor(col));
                } else {
                    i->clearProperty(SelectedBackground);
                }
            }
            tmp = s[8]; if (!tmp.isEmpty() && tmp != QLatin1String("---")) {
                i->setFontFamily(tmp);
            }
        }
    }
}

void KateHlManager::setDefaults(const QString &schema, KateAttributeList &list, KConfig *cfg)
{
    cfg = cfg ? cfg : KateHlManager::self()->self()->getKConfig();
    KConfigGroup config(cfg,
                        QLatin1String("Default Item Styles - Schema ") + schema);

    const QString zero = QStringLiteral("0");
    const QString one = QStringLiteral("1");
    const QString dash = QStringLiteral("-");

    for (int z = 0; z < defaultStyleCount(); z++) {
        QStringList settings;
        KTextEditor::Attribute::Ptr p = list.at(z);

        settings << (p->hasProperty(QTextFormat::ForegroundBrush) ? QString::number(p->foreground().color().rgb(), 16) : QString());
        settings << (p->hasProperty(SelectedForeground) ? QString::number(p->selectedForeground().color().rgb(), 16) : QString());
        settings << (p->hasProperty(QTextFormat::FontWeight) ? (p->fontBold() ? one : zero) : QString());
        settings << (p->hasProperty(QTextFormat::FontItalic) ? (p->fontItalic() ? one : zero) : QString());
        settings << (p->hasProperty(QTextFormat::FontStrikeOut) ? (p->fontStrikeOut() ? one : zero) : QString());
        settings << (p->hasProperty(QTextFormat::TextUnderlineStyle) ? (p->fontUnderline() ? one : zero) : QString());
        settings << (p->hasProperty(QTextFormat::BackgroundBrush) ? QString::number(p->background().color().rgb(), 16) : dash);
        settings << (p->hasProperty(SelectedBackground) ? QString::number(p->selectedBackground().color().rgb(), 16) : dash);
        settings << (p->hasProperty(QTextFormat::FontFamily) ? (p->fontFamily()) : QString());
        settings << QStringLiteral("---");

        config.writeEntry(defaultStyleName(z), settings);
    }

    emit changed();
}

void KateHlManager::reload()
{
    /**
     * copy current loaded hls from hash to trigger recreation
     */
    auto oldHls = m_hlDict;
    m_hlDict.clear();

    /**
     * recreate repository
     * this might even remove highlighting modes known before
     */
    m_repository.reload();

    /**
     * let all documents use the new highlighters
     * will be created on demand
     * if old hl not found, use none
     */
    for (KTextEditor::DocumentPrivate* doc : KTextEditor::EditorPrivate::self()->kateDocuments()) {
        auto hlMode = doc->highlightingMode();
        if (nameFind(hlMode) < 0) {
            hlMode = QStringLiteral("None");
        }
        doc->setHighlightingMode(hlMode);
    }
}
//END

