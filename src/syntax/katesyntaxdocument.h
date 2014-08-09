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

#ifndef __KATE_SYNTAXDOCUMENT_H__
#define __KATE_SYNTAXDOCUMENT_H__

#include <QList>
#include <QStringList>
#include <QDomDocument>
#include <QHash>

/**
 * Class holding the data around the current QDomElement
 */
class KateSyntaxContextData
{
public:
    QDomElement parent;
    QDomElement currentGroup;
    QDomElement item;
};

/**
 * Store and manage the information about Syntax Highlighting.
 */
class KateSyntaxDocument
{
public:
    /**
     * Constructor
     */
    KateSyntaxDocument();

    /**
     * Destructor
     */
    ~KateSyntaxDocument();

    /**
     * If the open hl file is different from the one needed, it opens
     * the new one and assign some other things.
     * @param identifier file name and path of the new xml needed
     * @return success
     */
    bool setIdentifier(const QString &identifier);
    
    /**
     * Clear internal QDomDocument cache
     */
    void clearCache();

    /**
     * Jump to the next group, KateSyntaxContextData::currentGroup will point to the next group
     * @param data context
     * @return success
     */
    bool nextGroup(KateSyntaxContextData *data);

    /**
     * Jump to the next item, KateSyntaxContextData::item will point to the next item
     * @param data context
     * @return success
     */
    bool nextItem(KateSyntaxContextData *data);

    /**
     * This function is used to fetch the atributes of the tags.
     */
    QString groupItemData(const KateSyntaxContextData *data, const QString &name);
    QString groupData(const KateSyntaxContextData *data, const QString &name);

    void freeGroupInfo(KateSyntaxContextData *data);
    KateSyntaxContextData *getSubItems(KateSyntaxContextData *data);

    /**
     * Get the KateSyntaxContextData of the DomElement Config inside mainGroupName
     * It just fills KateSyntaxContextData::item
     */
    KateSyntaxContextData *getConfig(const QString &mainGroupName, const QString &config);

    /**
     * Get the KateSyntaxContextData of the QDomElement Config inside mainGroupName
     * KateSyntaxContextData::parent will contain the QDomElement found
     */
    KateSyntaxContextData *getGroupInfo(const QString &mainGroupName, const QString &group);

    /**
     * Returns a list with all the keywords inside the list type
     */
    QStringList &finddata(const QString &mainGroup, const QString &type, bool clearList = true);

private:
    /**
     * Used by getConfig and getGroupInfo to traverse the xml nodes and
     * evenually return the found element
     */
    bool getElement(QDomElement &element, const QString &mainGroupName, const QString &config);

    /**
     * current parsed filename
     */
    QString currentFile;

    /**
     * last found data out of the xml
     */
    QStringList m_data;
    
    /**
     * internal cache for domdocuments
     */
    QHash<QString, QDomDocument *> m_domDocuments;
};

#endif

