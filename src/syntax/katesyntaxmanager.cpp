/*
    SPDX-FileCopyrightText: 2007 Matthew Woehlke <mw_triad@users.sourceforge.net>
    SPDX-FileCopyrightText: 2003, 2004 Anders Lund <anders@alweb.dk>
    SPDX-FileCopyrightText: 2003 Hamish Rodda <rodda@kde.org>
    SPDX-FileCopyrightText: 2001, 2002 Joseph Wenninger <jowenn@kde.org>
    SPDX-FileCopyrightText: 2001 Christoph Cullmann <cullmann@kde.org>
    SPDX-FileCopyrightText: 1999 Jochen Wilhelmy <digisnap@cs.tu-berlin.de>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

// BEGIN INCLUDES
#include "katesyntaxmanager.h"

#include "kateconfig.h"
#include "katedocument.h"
#include "kateextendedattribute.h"
#include "kateglobal.h"
#include "katehighlight.h"
#include "katepartdebug.h"
#include "katerenderer.h"
#include "katetextline.h"

#include <KColorUtils>
#include <KConfigGroup>
#include <KLocalizedString>

#include <algorithm>
// END

using namespace KTextEditor;

// BEGIN KateHlManager
KateHlManager *KateHlManager::self()
{
    return KTextEditor::EditorPrivate::self()->hlManager();
}

QVector<KSyntaxHighlighting::Theme> KateHlManager::sortedThemes() const
{
    // get KSyntaxHighlighting themes
    auto themes = repository().themes();

    // sort by translated name
    std::sort(themes.begin(), themes.end(), [](const KSyntaxHighlighting::Theme &left, const KSyntaxHighlighting::Theme &right) {
        return left.translatedName().compare(right.translatedName(), Qt::CaseInsensitive) < 0;
    });
    return themes;
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

void KateHlManager::reload()
{
    // copy current loaded hls from hash to trigger recreation
    auto oldHls = m_hlDict;
    m_hlDict.clear();

    // recreate repository
    // this might even remove highlighting modes known before
    m_repository.reload();

    // let all documents use the new highlighters
    // will be created on demand
    // if old hl not found, use none
    for (auto doc : KTextEditor::EditorPrivate::self()->kateDocuments()) {
        auto hlMode = doc->highlightingMode();
        if (nameFind(hlMode) < 0) {
            hlMode = QStringLiteral("None");
        }
        doc->setHighlightingMode(hlMode);
    }
}
// END
