/* This file is part of the KDE libraries
   Copyright (C) 2001 Joseph Wenninger <jowenn@kde.org>
   Copyright (C) 2000 Scott Manson <sdmanson@alltel.net>

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

#include "katesyntaxdocument.h"

#include "katepartdebug.h"
#include "kateglobal.h"

#include <qplatformdefs.h>

#include <KLocalizedString>
#include <KMessageBox>
#include <KConfig>
#include <KConfigGroup>

#include <QApplication>
#include <QFile>
#include <QDir>
#include <QJsonDocument>

// use this to turn on over verbose debug output...
#undef KSD_OVER_VERBOSE

KateSyntaxDocument::KateSyntaxDocument()
    : QDomDocument()
{
    // Let's build the Mode List
    setupModeList();
}

KateSyntaxDocument::~KateSyntaxDocument()
{
    for (int i = 0; i < myModeList.size(); i++) {
        delete myModeList[i];
    }
}

/** If the open hl file is different from the one needed, it opens
    the new one and assign some other things.
    identifier = File name and path of the new xml needed
*/
bool KateSyntaxDocument::setIdentifier(const QString &identifier)
{
    // if the current file is the same as the new one don't do anything.
    if (currentFile != identifier) {
        // let's open the new file
        QFile f(identifier);

        if (f.open(QIODevice::ReadOnly)) {
            // Let's parse the contets of the xml file
            /* The result of this function should be check for robustness,
               a false returned means a parse error */
            QString errorMsg;
            int line, col;
            bool success = setContent(&f, &errorMsg, &line, &col);

            // Ok, now the current file is the pretended one (identifier)
            currentFile = identifier;

            // Close the file, is not longer needed
            f.close();

            if (!success) {
                KMessageBox::error(QApplication::activeWindow(), i18n("<qt>The error <b>%4</b><br /> has been detected in the file %1 at %2/%3</qt>", identifier,
                                   line, col, i18nc("QXml", errorMsg.toUtf8().data())));
                return false;
            }
        } else {
            // Oh o, we couldn't open the file.
            KMessageBox::error(QApplication::activeWindow(), i18n("Unable to open %1", identifier));
            return false;
        }
    }
    return true;
}

/**
 * Jump to the next group, KateSyntaxContextData::currentGroup will point to the next group
 */
bool KateSyntaxDocument::nextGroup(KateSyntaxContextData *data)
{
    if (!data) {
        return false;
    }

    // No group yet so go to first child
    if (data->currentGroup.isNull()) {
        // Skip over non-elements. So far non-elements are just comments
        QDomNode node = data->parent.firstChild();
        while (node.isComment()) {
            node = node.nextSibling();
        }

        data->currentGroup = node.toElement();
    } else {
        // common case, iterate over siblings, skipping comments as we go
        QDomNode node = data->currentGroup.nextSibling();
        while (node.isComment()) {
            node = node.nextSibling();
        }

        data->currentGroup = node.toElement();
    }

    return !data->currentGroup.isNull();
}

/**
 * Jump to the next item, KateSyntaxContextData::item will point to the next item
 */
bool KateSyntaxDocument::nextItem(KateSyntaxContextData *data)
{
    if (!data) {
        return false;
    }

    if (data->item.isNull()) {
        QDomNode node = data->currentGroup.firstChild();
        while (node.isComment()) {
            node = node.nextSibling();
        }

        data->item = node.toElement();
    } else {
        QDomNode node = data->item.nextSibling();
        while (node.isComment()) {
            node = node.nextSibling();
        }

        data->item = node.toElement();
    }

    return !data->item.isNull();
}

/**
 * This function is used to fetch the atributes of the tags of the item in a KateSyntaxContextData.
 */
QString KateSyntaxDocument::groupItemData(const KateSyntaxContextData *data, const QString &name)
{
    if (!data) {
        return QString();
    }

    // If there's no name just return the tag name of data->item
    if ((!data->item.isNull()) && (name.isEmpty())) {
        return data->item.tagName();
    }

    // if name is not empty return the value of the attribute name
    if (!data->item.isNull()) {
        return data->item.attribute(name);
    }

    return QString();

}

QString KateSyntaxDocument::groupData(const KateSyntaxContextData *data, const QString &name)
{
    if (!data) {
        return QString();
    }

    if (!data->currentGroup.isNull()) {
        return data->currentGroup.attribute(name);
    } else {
        return QString();
    }
}

void KateSyntaxDocument::freeGroupInfo(KateSyntaxContextData *data)
{
    delete data;
}

KateSyntaxContextData *KateSyntaxDocument::getSubItems(KateSyntaxContextData *data)
{
    KateSyntaxContextData *retval = new KateSyntaxContextData;

    if (data != 0) {
        retval->parent = data->currentGroup;
        retval->currentGroup = data->item;
    }

    return retval;
}

bool KateSyntaxDocument::getElement(QDomElement &element, const QString &mainGroupName, const QString &config)
{
#ifdef KSD_OVER_VERBOSE
    qCDebug(LOG_PART) << "Looking for \"" << mainGroupName << "\" -> \"" << config << "\".";
#endif

    QDomNodeList nodes = documentElement().childNodes();

    // Loop over all these child nodes looking for mainGroupName
    for (int i = 0; i < nodes.count(); i++) {
        QDomElement elem = nodes.item(i).toElement();
        if (elem.tagName() == mainGroupName) {
            // Found mainGroupName ...
            QDomNodeList subNodes = elem.childNodes();

            // ... so now loop looking for config
            for (int j = 0; j < subNodes.count(); j++) {
                QDomElement subElem = subNodes.item(j).toElement();
                if (subElem.tagName() == config) {
                    // Found it!
                    element = subElem;
                    return true;
                }
            }

#ifdef KSD_OVER_VERBOSE
            qCDebug(LOG_PART) << "WARNING: \"" << config << "\" wasn't found!";
#endif

            return false;
        }
    }

#ifdef KSD_OVER_VERBOSE
    qCDebug(LOG_PART) << "WARNING: \"" << mainGroupName << "\" wasn't found!";
#endif

    return false;
}

/**
 * Get the KateSyntaxContextData of the QDomElement Config inside mainGroupName
 * KateSyntaxContextData::item will contain the QDomElement found
 */
KateSyntaxContextData *KateSyntaxDocument::getConfig(const QString &mainGroupName, const QString &config)
{
    QDomElement element;
    if (getElement(element, mainGroupName, config)) {
        KateSyntaxContextData *data = new KateSyntaxContextData;
        data->item = element;
        return data;
    }
    return 0;
}

/**
 * Get the KateSyntaxContextData of the QDomElement Config inside mainGroupName
 * KateSyntaxContextData::parent will contain the QDomElement found
 */
KateSyntaxContextData *KateSyntaxDocument::getGroupInfo(const QString &mainGroupName, const QString &group)
{
    QDomElement element;
    if (getElement(element, mainGroupName, group + QLatin1Char('s'))) {
        KateSyntaxContextData *data = new KateSyntaxContextData;
        data->parent = element;
        return data;
    }
    return 0;
}

/**
 * Returns a list with all the keywords inside the list type
 */
QStringList &KateSyntaxDocument::finddata(const QString &mainGroup, const QString &type, bool clearList)
{
#ifdef KSD_OVER_VERBOSE
    qCDebug(LOG_PART) << "Create a list of keywords \"" << type << "\" from \"" << mainGroup << "\".";
#endif

    if (clearList) {
        m_data.clear();
    }

    for (QDomNode node = documentElement().firstChild(); !node.isNull(); node = node.nextSibling()) {
        QDomElement elem = node.toElement();
        if (elem.tagName() == mainGroup) {
#ifdef KSD_OVER_VERBOSE
            qCDebug(LOG_PART) << "\"" << mainGroup << "\" found.";
#endif

            QDomNodeList nodelist1 = elem.elementsByTagName(QLatin1String("list"));

            for (int l = 0; l < nodelist1.count(); l++) {
                if (nodelist1.item(l).toElement().attribute(QLatin1String("name")) == type) {
#ifdef KSD_OVER_VERBOSE
                    qCDebug(LOG_PART) << "List with attribute name=\"" << type << "\" found.";
#endif

                    QDomNodeList childlist = nodelist1.item(l).toElement().childNodes();

                    for (int i = 0; i < childlist.count(); i++) {
                        QString element = childlist.item(i).toElement().text().trimmed();
                        if (element.isEmpty()) {
                            continue;
                        }

#ifdef KSD_OVER_VERBOSE
                        if (i < 6) {
                            qCDebug(LOG_PART) << "\"" << element << "\" added to the list \"" << type << "\"";
                        } else if (i == 6) {
                            qCDebug(LOG_PART) << "... The list continues ...";
                        }
#endif

                        m_data += element;
                    }

                    break;
                }
            }
            break;
        }
    }

    return m_data;
}

// Private
/** Generate the list of hl modes, store them in myModeList
    force: if true forces to rebuild the Mode List from the xml files (instead of katesyntax...rc)
*/
void KateSyntaxDocument::setupModeList()
{
    // If there's something in myModeList the Mode List was already built so, don't do it again
    if (!myModeList.isEmpty()) {
        return;
    }

    // Let's get a list of all the index & xml files for hl
    QStringList dirsWithIndexFiles;
    QStringList xmlFiles;

    const QStringList dirs = QStandardPaths::locateAll(QStandardPaths::GenericDataLocation, QLatin1String("katepart5/syntax"), QStandardPaths::LocateDirectory);
    foreach (const QString &dir, dirs) {
        QDir d(dir);

        // if dir has index json, only take that into account!
        if (d.exists(QStringLiteral("index.json"))) {
            dirsWithIndexFiles.append(dir);
            continue;
        }
        
        // else get all xml files
        const QStringList fileNames = d.entryList(QStringList() << QStringLiteral("*.xml"));
        foreach (const QString &file, fileNames) {
            xmlFiles.append(dir + QLatin1Char('/') + file);
        }
    }
    
    // only allow each name once!
    QSet<QString> hlNames;
    
    // preference: xml files, to allow users to overwrite system files with index!
    Q_FOREACH (const QString xmlFile, xmlFiles) {
        // We're forced to read the xml files or the mode doesn't exist in the katesyntax...rc
        QFile f(xmlFile);
        if (!f.open(QIODevice::ReadOnly))
            continue;
        
        // Ok we opened the file, let's read the contents and close the file
        /* the return of setContent should be checked because a false return shows a parsing error */
        QString errMsg;
        int line, col;
        if (!setContent(&f, &errMsg, &line, &col))
            continue;

        QDomElement root = documentElement();
        if (root.isNull())
            continue;
        
        // If the 'first' tag is language, go on
        if (root.tagName() != QLatin1String("language"))
            continue;
        
        // get name, only allow hls once!
        const QString name = root.attribute(QLatin1String("name"));
        if (hlNames.contains(name))
            continue;
        
        // let's make the mode list item.
        KateSyntaxModeListItem *mli = new KateSyntaxModeListItem;

        mli->name      = name;
        mli->section   = root.attribute(QLatin1String("section"));
        mli->mimetype  = root.attribute(QLatin1String("mimetype"));
        mli->extension = root.attribute(QLatin1String("extensions"));
        mli->version   = root.attribute(QLatin1String("version"));
        mli->priority  = root.attribute(QLatin1String("priority"));
        mli->style     = root.attribute(QLatin1String("style"));
        mli->author    = root.attribute(QLatin1String("author"));
        mli->license   = root.attribute(QLatin1String("license"));
        mli->indenter  = root.attribute(QLatin1String("indenter"));

        QString hidden = root.attribute(QLatin1String("hidden"));
        mli->hidden    = (hidden == QLatin1String("true") || hidden == QLatin1String("TRUE"));

        mli->identifier = xmlFile;
        
        // translate section + name
        mli->section    = i18nc("Language Section", mli->section.toUtf8().data());
        mli->nameTranslated = i18nc("Language", mli->name.toUtf8().data());

        // Append the new item to the list.
        myModeList.append(mli);
        hlNames.insert(name);
    }
    
    // now: index files
    Q_FOREACH (const QString dir, dirsWithIndexFiles) {
        /**
         * open the file for reading, bail out on error!
         */
        QFile file (dir + QStringLiteral("/index.json"));
        if (!file.open (QFile::ReadOnly))
            continue;

        /**
         * parse the whole file, bail out again on error!
         */
        const QJsonDocument index (QJsonDocument::fromBinaryData(file.readAll()));
        if (index.isNull())
            continue;
        
        /**
         * iterate over all hls in the index
         */
        QMapIterator<QString, QVariant> i(index.toVariant().toMap());
        while (i.hasNext()) {
            i.next();
            
            // get map
            QVariantMap map = i.value().toMap();
            
            // get name, only allow hls once!
            const QString name = map[QLatin1String("name")].toString();
            if (hlNames.contains(name))
                continue;
            
            // let's make the mode list item.
            KateSyntaxModeListItem *mli = new KateSyntaxModeListItem;

            mli->name      = name;
            mli->section   = map[QLatin1String("section")].toString();
            mli->mimetype  = map[QLatin1String("mimetype")].toString();
            mli->extension = map[QLatin1String("extensions")].toString();
            mli->version   = map[QLatin1String("version")].toString();
            mli->priority  = map[QLatin1String("priority")].toString();
            mli->style     = map[QLatin1String("style")].toString();
            mli->author    = map[QLatin1String("author")].toString();
            mli->license   = map[QLatin1String("license")].toString();
            mli->indenter  = map[QLatin1String("indenter")].toString();
            mli->hidden    = map[QLatin1String("hidden")].toBool();

            mli->identifier = dir + QLatin1Char('/') + i.key();
            
            // translate section + name
            mli->section    = i18nc("Language Section", mli->section.toUtf8().data());
            mli->nameTranslated = i18nc("Language", mli->name.toUtf8().data());

            // Append the new item to the list.
            myModeList.append(mli);
            hlNames.insert(name);
        }
    }
}

