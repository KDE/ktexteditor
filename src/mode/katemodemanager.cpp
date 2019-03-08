/*  This file is part of the KDE libraries and the Kate part.
 *
 *  Copyright (C) 2001-2010 Christoph Cullmann <cullmann@kde.org>
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

//BEGIN Includes
#include "katemodemanager.h"
#include "katewildcardmatcher.h"

#include "katedocument.h"
#include "kateconfig.h"
#include "kateview.h"
#include "kateglobal.h"
#include "katesyntaxmanager.h"
#include "katepartdebug.h"

#include <KConfigGroup>

#include <QMimeDatabase>
//END Includes

static QStringList vectorToList(const QVector<QString> &v)
{
    QStringList l;
    l.reserve(v.size());
    std::copy(v.begin(), v.end(), std::back_inserter(l));
    return l;
}

KateModeManager::KateModeManager()
{
    update();
}

KateModeManager::~KateModeManager()
{
    qDeleteAll(m_types);
}

bool compareKateFileType(const KateFileType *const left, const KateFileType *const right)
{
    int comparison = left->section.compare(right->section, Qt::CaseInsensitive);
    if (comparison == 0) {
        comparison = left->name.compare(right->name, Qt::CaseInsensitive);
    }
    return comparison < 0;
}

//
// read the types from config file and update the internal list
//
void KateModeManager::update()
{
    KConfig config(QStringLiteral("katemoderc"), KConfig::NoGlobals);

    QStringList g(config.groupList());

    qDeleteAll(m_types);
    m_types.clear();
    m_name2Type.clear();
    for (int z = 0; z < g.count(); z++) {
        KConfigGroup cg(&config, g[z]);

        KateFileType *type = new KateFileType();
        type->number = z;
        type->name = g[z];
        type->section = cg.readEntry(QStringLiteral("Section"));
        type->wildcards = cg.readXdgListEntry(QStringLiteral("Wildcards"));
        type->mimetypes = cg.readXdgListEntry(QStringLiteral("Mimetypes"));
        type->priority = cg.readEntry(QStringLiteral("Priority"), 0);
        type->varLine = cg.readEntry(QStringLiteral("Variables"));
        type->indenter = cg.readEntry(QStringLiteral("Indenter"));

        type->hl = cg.readEntry(QStringLiteral("Highlighting"));

        // only for generated types...
        type->hlGenerated = cg.readEntry(QStringLiteral("Highlighting Generated"), false);
        type->version = cg.readEntry(QStringLiteral("Highlighting Version"));

        // insert into the list + hash...
        m_types.append(type);
        m_name2Type.insert(type->name, type);
    }

    // try if the hl stuff is up to date...
    const auto modes = KateHlManager::self()->modeList();
    for (int i = 0; i < modes.size(); ++i) {
        // filter out hidden languages; and
        // filter out "None" hl, we add that later as "normal" mode
        if (modes[i].isHidden() || modes[i].name() == QLatin1String("None")) {
            continue;
        }

        KateFileType *type = nullptr;
        bool newType = false;
        if (m_name2Type.contains(modes[i].name())) {
            type = m_name2Type[modes[i].name()];
        } else {
            newType = true;
            type = new KateFileType();
            type->name = modes[i].name();
            type->priority = 0;
            m_types.append(type);
            m_name2Type.insert(type->name, type);
        }

        if (newType || type->version != QString::number(modes[i].version())) {
            type->name = modes[i].name();
            type->section = modes[i].section();
            type->wildcards = vectorToList(modes[i].extensions());
            type->mimetypes = vectorToList(modes[i].mimeTypes());
            type->priority = modes[i].priority();
            type->version = QString::number(modes[i].version());
            type->indenter = modes[i].indenter();
            type->hl = modes[i].name();
            type->hlGenerated = true;
        }

        type->translatedName = modes[i].translatedName();
        type->translatedSection = modes[i].translatedSection();
    }

    // sort the list...
    std::sort(m_types.begin(), m_types.end(), compareKateFileType);

    // add the none type...
    KateFileType *t = new KateFileType();

    // DO NOT TRANSLATE THIS, DONE LATER, marked by hlGenerated
    t->name = QStringLiteral("Normal");
    t->hl = QStringLiteral("None");
    t->hlGenerated = true;

    m_types.prepend(t);
}

//
// save the given list to config file + update
//
void KateModeManager::save(const QList<KateFileType *> &v)
{
    KConfig katerc(QStringLiteral("katemoderc"), KConfig::NoGlobals);

    QStringList newg;
    foreach (const KateFileType *type, v) {
        KConfigGroup config(&katerc, type->name);

        config.writeEntry("Section", type->section);
        config.writeXdgListEntry("Wildcards", type->wildcards);
        config.writeXdgListEntry("Mimetypes", type->mimetypes);
        config.writeEntry("Priority", type->priority);
        config.writeEntry("Indenter", type->indenter);

        QString varLine = type->varLine;
        if (!varLine.contains(QStringLiteral("kate:"))) {
            varLine.prepend(QLatin1String("kate: "));
        }

        config.writeEntry("Variables", varLine);

        config.writeEntry("Highlighting", type->hl);

        // only for generated types...
        config.writeEntry("Highlighting Generated", type->hlGenerated);
        config.writeEntry("Highlighting Version", type->version);

        newg << type->name;
    }

    foreach (const QString &groupName, katerc.groupList()) {
        if (newg.indexOf(groupName) == -1) {
            katerc.deleteGroup(groupName);
        }
    }

    katerc.sync();

    update();
}

QString KateModeManager::fileType(KTextEditor::DocumentPrivate *doc, const QString &fileToReadFrom)
{
    if (!doc) {
        return QString();
    }

    if (m_types.isEmpty()) {
        return QString();
    }

    QString fileName = doc->url().toString();
    int length = doc->url().toString().length();

    QString result;

    // Try wildcards
    if (! fileName.isEmpty()) {
        static const QStringList commonSuffixes = QStringLiteral(".orig;.new;~;.bak;.BAK").split(QLatin1Char(';'));

        if (!(result = wildcardsFind(fileName)).isEmpty()) {
            return result;
        }

        QString backupSuffix = KateDocumentConfig::global()->backupSuffix();
        if (fileName.endsWith(backupSuffix)) {
            if (!(result = wildcardsFind(fileName.left(length - backupSuffix.length()))).isEmpty()) {
                return result;
            }
        }

        for (QStringList::ConstIterator it = commonSuffixes.begin(); it != commonSuffixes.end(); ++it) {
            if (*it != backupSuffix && fileName.endsWith(*it)) {
                if (!(result = wildcardsFind(fileName.left(length - (*it).length()))).isEmpty()) {
                    return result;
                }
            }
        }
    }


    // either read the file passed to this function (pre-load) or use the normal mimeType() KF KTextEditor API
    QString mtName;
    if (!fileToReadFrom.isEmpty()) {
        mtName = QMimeDatabase().mimeTypeForFile(fileToReadFrom).name();
    } else {
        mtName = doc->mimeType();
    }

    QList<KateFileType *> types;
    foreach (KateFileType *type, m_types) {
        if (type->mimetypes.indexOf(mtName) > -1) {
            types.append(type);
        }
    }

    if (!types.isEmpty()) {
        int pri = -1;
        QString name;

        foreach (KateFileType *type, types) {
            if (type->priority > pri) {
                pri = type->priority;
                name = type->name;
            }
        }

        return name;
    }

    return QString();
}

QString KateModeManager::wildcardsFind(const QString &fileName)
{
    KateFileType *match = nullptr;
    int minPrio = -1;
    foreach (KateFileType *type, m_types) {
        if (type->priority <= minPrio) {
            continue;
        }

        foreach (const QString &wildcard, type->wildcards) {
            if (KateWildcardMatcher::exactMatch(fileName, wildcard)) {
                match = type;
                minPrio = type->priority;
                break;
            }
        }
    }

    return (match == nullptr) ? QString() : match->name;
}

const KateFileType &KateModeManager::fileType(const QString &name) const
{
    for (int i = 0; i < m_types.size(); ++i)
        if (m_types[i]->name == name) {
            return *m_types[i];
        }

    static KateFileType notype;
    return notype;
}

