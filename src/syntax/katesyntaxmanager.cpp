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
#include "katesyntaxdocument.h"
#include "katerenderer.h"
#include "kateglobal.h"
#include "kateschema.h"
#include "kateconfig.h"
#include "kateextendedattribute.h"
#include "katehighlight.h"
#include "katepartdebug.h"

#include <KConfigGroup>
#include <KColorScheme>
#include <KColorUtils>
#include <KMessageBox>

#include <QSet>
#include <QAction>
#include <QStringList>
#include <QTextStream>
//END

using namespace KTextEditor;

bool compareKateHighlighting(const KateHighlighting *const left, const KateHighlighting *const right)
{
    int comparison = left->section().compare(right->section(), Qt::CaseInsensitive);
    if (comparison == 0) {
        comparison = left->nameTranslated().compare(right->nameTranslated(), Qt::CaseInsensitive);
    }
    return comparison < 0;
}

//BEGIN KateHlManager
KateHlManager::KateHlManager()
    : QObject()
    , m_config(QLatin1String("katesyntaxhighlightingrc"), KConfig::NoGlobals)
    , commonSuffixes(QString::fromLatin1(".orig;.new;~;.bak;.BAK").split(QLatin1Char(';')))
    , syntax(new KateSyntaxDocument(&m_config))
    , dynamicCtxsCount(0)
    , forceNoDCReset(false)
{
    KateSyntaxModeList modeList = syntax->modeList();
    hlList.reserve(modeList.size() + 1);
    hlDict.reserve(modeList.size() + 1);
    for (int i = 0; i < modeList.count(); i++) {
        KateHighlighting *hl = new KateHighlighting(modeList[i]);

        hlList.insert(qLowerBound(hlList.begin(), hlList.end(), hl, compareKateHighlighting), hl);
        hlDict.insert(hl->name(), hl);
    }

    // Normal HL
    KateHighlighting *hl = new KateHighlighting(0);
    hlList.prepend(hl);
    hlDict.insert(hl->name(), hl);

    lastCtxsReset.start();
}

KateHlManager::~KateHlManager()
{
    delete syntax;
    qDeleteAll(hlList);
}

KateHlManager *KateHlManager::self()
{
    return KTextEditor::EditorPrivate::self()->hlManager();
}

KateHighlighting *KateHlManager::getHl(int n)
{
    if (n < 0 || n >= hlList.count()) {
        n = 0;
    }

    return hlList.at(n);
}

int KateHlManager::nameFind(const QString &name)
{
    for (int i = 0; i < hlList.count(); ++i) {
        if (hlList.at(i)->name().compare(name, Qt::CaseInsensitive) == 0) {
            return i;
        }
    }

    return -1;
}

int KateHlManager::defaultStyleCount()
{
    return HighlightInterface::DS_COUNT;
}

QString KateHlManager::defaultStyleName(int n, bool translateNames)
{
    static QStringList names;
    static QStringList translatedNames;

    if (names.isEmpty()) {
        names << QLatin1String("Normal");
        names << QLatin1String("Keyword");
        names << QLatin1String("Function");
        names << QLatin1String("Variable");
        names << QLatin1String("Control Flow");
        names << QLatin1String("Operator");
        names << QLatin1String("Built-in");
        names << QLatin1String("Extension");
        names << QLatin1String("Preprocessor");
        names << QLatin1String("Attribute");

        names << QLatin1String("Character");
        names << QLatin1String("Special Character");
        names << QLatin1String("String");
        names << QLatin1String("Verbatim String");
        names << QLatin1String("Special String");
        names << QLatin1String("Import");

        names << QLatin1String("Data Type");
        names << QLatin1String("Decimal/Value");
        names << QLatin1String("Base-N Integer");
        names << QLatin1String("Floating Point");
        names << QLatin1String("Constant");

        names << QLatin1String("Comment");
        names << QLatin1String("Documentation");
        names << QLatin1String("Annotation");
        names << QLatin1String("Comment Variable");
        // this next one is for denoting the beginning/end of a user defined folding region
        names << QLatin1String("Region Marker");
        names << QLatin1String("Information");
        names << QLatin1String("Warning");
        names << QLatin1String("Alert");

        names << QLatin1String("Others");
        // this one is for marking invalid input
        names << QLatin1String("Error");

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
        return KTextEditor::HighlightInterface::dsNormal;
    } else if (name == QLatin1String("dsKeyword")) {
        return KTextEditor::HighlightInterface::dsKeyword;
    } else if (name == QLatin1String("dsFunction")) {
        return KTextEditor::HighlightInterface::dsFunction;
    } else if (name == QLatin1String("dsVariable")) {
        return KTextEditor::HighlightInterface::dsVariable;
    } else if (name == QLatin1String("dsControlFlow")) {
        return KTextEditor::HighlightInterface::dsControlFlow;
    } else if (name == QLatin1String("dsOperator")) {
        return KTextEditor::HighlightInterface::dsOperator;
    } else if (name == QLatin1String("dsBuiltIn")) {
        return KTextEditor::HighlightInterface::dsBuiltIn;
    } else if (name == QLatin1String("dsExtension")) {
        return KTextEditor::HighlightInterface::dsExtension;
    } else if (name == QLatin1String("dsPreprocessor")) {
        return KTextEditor::HighlightInterface::dsPreprocessor;
    } else if (name == QLatin1String("dsAttribute")) {
        return KTextEditor::HighlightInterface::dsAttribute;
    }

    //
    // Strings & Characters
    //
    if (name == QLatin1String("dsChar")) {
        return KTextEditor::HighlightInterface::dsChar;
    } else if (name == QLatin1String("dsSpecialChar")) {
        return KTextEditor::HighlightInterface::dsSpecialChar;
    } else if (name == QLatin1String("dsString")) {
        return KTextEditor::HighlightInterface::dsString;
    } else if (name == QLatin1String("dsVerbatimString")) {
        return KTextEditor::HighlightInterface::dsVerbatimString;
    } else if (name == QLatin1String("dsSpecialString")) {
        return KTextEditor::HighlightInterface::dsSpecialString;
    } else if (name == QLatin1String("dsImport")) {
        return KTextEditor::HighlightInterface::dsImport;
    }

    //
    // Numbers, Types & Constants
    //
    if (name == QLatin1String("dsDataType")) {
        return KTextEditor::HighlightInterface::dsDataType;
    } else if (name == QLatin1String("dsDecVal")) {
        return KTextEditor::HighlightInterface::dsDecVal;
    } else if (name == QLatin1String("dsBaseN")) {
        return KTextEditor::HighlightInterface::dsBaseN;
    } else if (name == QLatin1String("dsFloat")) {
        return KTextEditor::HighlightInterface::dsFloat;
    } else if (name == QLatin1String("dsConstant")) {
        return KTextEditor::HighlightInterface::dsConstant;
    }

    //
    // Comments & Documentation
    //
    if (name == QLatin1String("dsComment")) {
        return KTextEditor::HighlightInterface::dsComment;
    } else if (name == QLatin1String("dsDocumentation")) {
        return KTextEditor::HighlightInterface::dsDocumentation;
    } else if (name == QLatin1String("dsAnnotation")) {
        return KTextEditor::HighlightInterface::dsAnnotation;
    } else if (name == QLatin1String("dsCommentVar")) {
        return KTextEditor::HighlightInterface::dsCommentVar;
    } else if (name == QLatin1String("dsRegionMarker")) {
        return KTextEditor::HighlightInterface::dsRegionMarker;
    } else if (name == QLatin1String("dsInformation")) {
        return KTextEditor::HighlightInterface::dsInformation;
    } else if (name == QLatin1String("dsWarning")) {
        return KTextEditor::HighlightInterface::dsWarning;
    } else if (name == QLatin1String("dsAlert")) {
        return KTextEditor::HighlightInterface::dsAlert;
    }

    //
    // Misc
    //
    if (name == QLatin1String("dsOthers")) {
        return KTextEditor::HighlightInterface::dsOthers;
    } else if (name == QLatin1String("dsError")) {
        return KTextEditor::HighlightInterface::dsError;
    }

    return KTextEditor::HighlightInterface::dsNormal;
}

void KateHlManager::getDefaults(const QString &schema, KateAttributeList &list, KConfig *cfg)
{
    KColorScheme scheme(QPalette::Active, KColorScheme::View);
    KColorScheme schemeSelected(QPalette::Active, KColorScheme::Selection);

    ///NOTE: it's important to append in the order of the HighlightInterface::DefaultStyle
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
                col = tmp.toUInt(0, 16); i->setForeground(QColor(col));
            }

            tmp = s[1]; if (!tmp.isEmpty()) {
                col = tmp.toUInt(0, 16); i->setSelectedForeground(QColor(col));
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
                    col = tmp.toUInt(0, 16);
                    i->setBackground(QColor(col));
                } else {
                    i->clearBackground();
                }
            }
            tmp = s[7]; if (!tmp.isEmpty()) {
                if (tmp != QLatin1String("-")) {
                    col = tmp.toUInt(0, 16);
                    i->setSelectedBackground(QColor(col));
                } else {
                    i->clearProperty(KTextEditor::Attribute::SelectedBackground);
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

    for (int z = 0; z < defaultStyleCount(); z++) {
        QStringList settings;
        KTextEditor::Attribute::Ptr p = list.at(z);

        settings << (p->hasProperty(QTextFormat::ForegroundBrush) ? QString::number(p->foreground().color().rgb(), 16) : QString());
        settings << (p->hasProperty(KTextEditor::Attribute::SelectedForeground) ? QString::number(p->selectedForeground().color().rgb(), 16) : QString());
        settings << (p->hasProperty(QTextFormat::FontWeight) ? (p->fontBold() ? QLatin1String("1") : QLatin1String("0")) : QString());
        settings << (p->hasProperty(QTextFormat::FontItalic) ? (p->fontItalic() ? QLatin1String("1") : QLatin1String("0")) : QString());
        settings << (p->hasProperty(QTextFormat::FontStrikeOut) ? (p->fontStrikeOut() ? QLatin1String("1") : QLatin1String("0")) : QString());
        settings << (p->hasProperty(QTextFormat::FontUnderline) ? (p->fontUnderline() ? QLatin1String("1") : QLatin1String("0")) : QString());
        settings << (p->hasProperty(QTextFormat::BackgroundBrush) ? QString::number(p->background().color().rgb(), 16) : QLatin1String("-"));
        settings << (p->hasProperty(KTextEditor::Attribute::SelectedBackground) ? QString::number(p->selectedBackground().color().rgb(), 16) : QLatin1String("-"));
        settings << (p->hasProperty(QTextFormat::FontFamily) ? (p->fontFamily()) : QString());
        settings << QLatin1String("---");

        config.writeEntry(defaultStyleName(z), settings);
    }

    emit changed();
}

int KateHlManager::highlights()
{
    return (int) hlList.count();
}

QString KateHlManager::hlName(int n)
{
    return hlList.at(n)->name();
}

QString KateHlManager::hlNameTranslated(int n)
{
    return hlList.at(n)->nameTranslated();
}

QString KateHlManager::hlSection(int n)
{
    return hlList.at(n)->section();
}

bool KateHlManager::hlHidden(int n)
{
    return hlList.at(n)->hidden();
}

QString KateHlManager::identifierForName(const QString &name)
{
    if (hlDict.contains(name)) {
        return hlDict[name]->getIdentifier();
    }

    return QString();
}

QString KateHlManager::nameForIdentifier(const QString &identifier)
{
    for (QHash<QString, KateHighlighting *>::iterator it = hlDict.begin();
            it != hlDict.end(); ++it) {
        if ((*it)->getIdentifier() == identifier) {
            return it.key();
        }
    }

    return QString();
}

bool KateHlManager::resetDynamicCtxs()
{
    if (forceNoDCReset) {
        return false;
    }

    if (lastCtxsReset.elapsed() < KATE_DYNAMIC_CONTEXTS_RESET_DELAY) {
        return false;
    }

    foreach (KateHighlighting *hl, hlList) {
        hl->dropDynamicContexts();
    }

    dynamicCtxsCount = 0;
    lastCtxsReset.start();

    return true;
}
//END

