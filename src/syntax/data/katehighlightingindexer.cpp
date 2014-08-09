/* This file is part of the KDE libraries
   Copyright (C) 2014 Christoph Cullmann <cullmann@kde.org>

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

#include <QCoreApplication>
#include <QFile>
#include <QFileInfo>
#include <QVariant>
#include <QXmlStreamReader>
#include <QJsonDocument>
#include <QXmlSchema>
#include <QXmlSchemaValidator>

int main(int argc, char *argv[])
{
    // get app instance
    QCoreApplication app(argc, argv);
    
    // ensure enough arguments are passed
    if (app.arguments().size() < 3)
        return 1;
    
    // open schema
    QXmlSchema schema;
    if (!schema.load(QUrl::fromLocalFile(app.arguments().at(2))))
        return 2;
    
    // text attributes
    const QStringList textAttributes = QStringList() << QLatin1String("name") << QLatin1String("section") << QLatin1String("mimetype")
            << QLatin1String("extensions") << QLatin1String("version") << QLatin1String("priority") << QLatin1String("style")
            << QLatin1String("author") << QLatin1String("license") << QLatin1String("indenter");
    
    // index all given highlightings
    QVariantMap hls;
    for (int i = 3; i < app.arguments().size(); ++i) {
        QFile hlFile (app.arguments().at(i));
        if (!hlFile.open(QIODevice::ReadOnly))
            return 3;
        
        // validate against schema
        QXmlSchemaValidator validator(schema);
        if (!validator.validate(&hlFile, QUrl::fromLocalFile(hlFile.fileName())))
            return 4;
        
        // read the needed attributes from toplevel language tag
        hlFile.reset();
        QXmlStreamReader xml(&hlFile);
        if (xml.readNextStartElement()) {
            if (xml.name() != QLatin1String("language"))
                return 5;
        } else {
            return 6;
        }

        // map to store hl info
        QVariantMap hl;
        
        // transfer text attributes
        Q_FOREACH (QString attribute, textAttributes) {
            hl[attribute] = xml.attributes().value(attribute).toString();
        }
        
        // add boolean one
        const QString hidden = xml.attributes().value(QLatin1String("hidden")).toString();
        hl[QLatin1String("hidden")] = (hidden == QLatin1String("true") || hidden == QLatin1String("TRUE"));
            
        // remember hl
        hls[QFileInfo(hlFile).fileName()] = hl;
    }
    
    // create outfile, after all has worked!
    QFile outFile(app.arguments().at(1));
    if (!outFile.open(QIODevice::WriteOnly | QIODevice::Truncate))
        return 7;

    // write out json
    outFile.write(QJsonDocument::fromVariant(QVariant(hls)).toBinaryData());
    
    // be done
    return 0;
}
