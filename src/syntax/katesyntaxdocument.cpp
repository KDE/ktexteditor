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

// use this to turn on over verbose debug output...
#undef KSD_OVER_VERBOSE

KateSyntaxDocument::KateSyntaxDocument()
{
}

KateSyntaxDocument::~KateSyntaxDocument()
{
    // cleanup, else we have a memory leak!
    clearCache();
}

/** If the open hl file is different from the one needed, it opens
    the new one and assign some other things.
    identifier = File name and path of the new xml needed
*/
bool KateSyntaxDocument::setIdentifier(const QString &identifier)
{
    // already existing in cache? be done
    if (m_domDocuments.contains(identifier)) {
        currentFile = identifier;
        return true;
    }

    // else: try to open
    QFile f(identifier);
    if (!f.open(QIODevice::ReadOnly)) {
         // Oh o, we couldn't open the file.
        KMessageBox::error(QApplication::activeWindow(), i18n("Unable to open %1", identifier));
        return false;
    }

    // try to parse
    QDomDocument *document = new QDomDocument();
    QString errorMsg;
    int line, col;
    if (!document->setContent(&f, &errorMsg, &line, &col)) {
        KMessageBox::error(QApplication::activeWindow(), i18n("<qt>The error <b>%4</b><br /> has been detected in the file %1 at %2/%3</qt>", identifier,
                                   line, col, i18nc("QXml", errorMsg.toUtf8().data())));
        delete document;
        return false;
    }

    // cache and be done
    currentFile = identifier;
    m_domDocuments[currentFile] = document;
    return true;
}

void KateSyntaxDocument::clearCache()
{
    qDeleteAll(m_domDocuments);
    m_domDocuments.clear();
    currentFile.clear();
    m_data.clear();
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

    if (data != nullptr) {
        retval->parent = data->currentGroup;
        retval->currentGroup = data->item;
    }

    return retval;
}

bool KateSyntaxDocument::getElement(QDomElement &element, const QString &mainGroupName, const QString &config)
{
#ifdef KSD_OVER_VERBOSE
    qCDebug(LOG_KTE) << "Looking for \"" << mainGroupName << "\" -> \"" << config << "\".";
#endif

    QDomNodeList nodes;
    if (m_domDocuments.contains(currentFile))
        nodes = m_domDocuments[currentFile]->documentElement().childNodes();

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
            qCDebug(LOG_KTE) << "WARNING: \"" << config << "\" wasn't found!";
#endif

            return false;
        }
    }

#ifdef KSD_OVER_VERBOSE
    qCDebug(LOG_KTE) << "WARNING: \"" << mainGroupName << "\" wasn't found!";
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
    return nullptr;
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
    return nullptr;
}

/**
 * Returns a list with all the keywords inside the list type
 */
QStringList &KateSyntaxDocument::finddata(const QString &mainGroup, const QString &type, bool clearList)
{
#ifdef KSD_OVER_VERBOSE
    qCDebug(LOG_KTE) << "Create a list of keywords \"" << type << "\" from \"" << mainGroup << "\".";
#endif

    if (clearList) {
        m_data.clear();
    }

    if (!m_domDocuments.contains(currentFile))
        return m_data;

    for (QDomNode node = m_domDocuments[currentFile]->documentElement().firstChild(); !node.isNull(); node = node.nextSibling()) {
        QDomElement elem = node.toElement();
        if (elem.tagName() == mainGroup) {
#ifdef KSD_OVER_VERBOSE
            qCDebug(LOG_KTE) << "\"" << mainGroup << "\" found.";
#endif

            QDomNodeList nodelist1 = elem.elementsByTagName(QStringLiteral("list"));

            for (int l = 0; l < nodelist1.count(); l++) {
                if (nodelist1.item(l).toElement().attribute(QStringLiteral("name")) == type) {
#ifdef KSD_OVER_VERBOSE
                    qCDebug(LOG_KTE) << "List with attribute name=\"" << type << "\" found.";
#endif

                    QDomNodeList childlist = nodelist1.item(l).toElement().childNodes();

                    for (int i = 0; i < childlist.count(); i++) {
                        QString element = childlist.item(i).toElement().text().trimmed();
                        if (element.isEmpty()) {
                            continue;
                        }

#ifdef KSD_OVER_VERBOSE
                        if (i < 6) {
                            qCDebug(LOG_KTE) << "\"" << element << "\" added to the list \"" << type << "\"";
                        } else if (i == 6) {
                            qCDebug(LOG_KTE) << "... The list continues ...";
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
