/*
    SPDX-FileCopyrightText: 2007 Matthew Woehlke <mw_triad@users.sourceforge.net>
    SPDX-FileCopyrightText: 2003, 2004 Anders Lund <anders@alweb.dk>
    SPDX-FileCopyrightText: 2003 Hamish Rodda <rodda@kde.org>
    SPDX-FileCopyrightText: 2001, 2002 Joseph Wenninger <jowenn@kde.org>
    SPDX-FileCopyrightText: 2001 Christoph Cullmann <cullmann@kde.org>
    SPDX-FileCopyrightText: 1999 Jochen Wilhelmy <digisnap@cs.tu-berlin.de>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "katesyntaxmanager.h"

#include "kateconfig.h"
#include "katedocument.h"
#include "kateglobal.h"
#include "katehighlight.h"
#include "katepartdebug.h"
#include "katerenderer.h"

#include <algorithm>

KateHlManager *KateHlManager::self()
{
    return KTextEditor::EditorPrivate::self()->hlManager();
}

QVector<KSyntaxHighlighting::Theme> KateHlManager::sortedThemes() const
{
    // get KSyntaxHighlighting themes
    auto themes = repository().themes();

    // sort by translated name
    std::sort(themes.begin(), themes.end(), [](const KSyntaxHighlighting::Theme &left, const KSyntaxHighlighting::Theme &right) { return left.translatedName().compare(right.translatedName(), Qt::CaseInsensitive) < 0; });
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
