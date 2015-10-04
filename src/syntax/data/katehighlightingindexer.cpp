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
#include <QTextStream>
#include <QVariant>
#include <QXmlStreamReader>
#include <QJsonDocument>
#include <QXmlSchema>
#include <QXmlSchemaValidator>
#include <QRegularExpression>
#include <QDebug>

namespace {

QStringList readListing(const QString &fileName)
{
    QFile file(fileName);
    if (!file.open(QIODevice::ReadOnly)) {
        return QStringList();
    }

    QStringList listing;
    QTextStream stream(&file);
    while (!stream.atEnd()) {
        const QString line = stream.readLine();
        listing << line;
    }
    return listing;
}

}

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

    const QString hlFilenamesListing = app.arguments().value(3);
    if (hlFilenamesListing.isEmpty()) {
        return 1;
    }

    QStringList hlFilenames = readListing(hlFilenamesListing);
    if (hlFilenames.isEmpty()) {
        return 3;
    }

    // text attributes
    const QStringList textAttributes = QStringList() << QLatin1String("name") << QLatin1String("section") << QLatin1String("mimetype")
            << QLatin1String("extensions") << QLatin1String("version") << QLatin1String("priority") << QLatin1String("style")
            << QLatin1String("author") << QLatin1String("license") << QLatin1String("indenter");

    // index all given highlightings
    QVariantMap hls;
    int anyError = 0;
    foreach (const QString &hlFilename, hlFilenames) {
        QFile hlFile(hlFilename);
        if (!hlFile.open(QIODevice::ReadOnly)) {
            anyError = 3;
            continue;
        }

        // validate against schema
        QXmlSchemaValidator validator(schema);
        if (!validator.validate(&hlFile, QUrl::fromLocalFile(hlFile.fileName()))) {
            anyError = 4;
            continue;
        }

        // read the needed attributes from toplevel language tag
        hlFile.reset();
        QXmlStreamReader xml(&hlFile);
        if (xml.readNextStartElement()) {
            if (xml.name() != QLatin1String("language")) {
                anyError = 5;
                continue;
            }
        } else {
            anyError = 6;
            continue;
        }

        // map to store hl info
        QVariantMap hl;

        // transfer text attributes
        Q_FOREACH (QString attribute, textAttributes) {
            hl[attribute] = xml.attributes().value(attribute).toString();
        }

        // add boolean one
        const QString hidden = xml.attributes().value(QLatin1String("hidden")).toString();
        hl[QLatin1String("hidden")] = (hidden == QLatin1String("true") || hidden == QLatin1String("1"));

        // remember hl
        hls[QFileInfo(hlFile).fileName()] = hl;

        // scan for broken regex
        while (!xml.atEnd()) {
            xml.readNext();
            if (!xml.isStartElement() || xml.name() != QLatin1String("RegExpr")) {
                continue;
            }

            const QString string (xml.attributes().value(QLatin1String("String")).toString());
            const QRegularExpression regexp (string);
            if (!regexp.isValid()) {
                qDebug() << hlFilename << "line" << xml.lineNumber() << "broken regex:" << string << "problem:" << regexp.errorString() << "at offset" << regexp.patternErrorOffset();
                anyError = 7;
            }
        }
    }

    // bail out if any problem was seen
    if (anyError)
        return anyError;

    // create outfile, after all has worked!
    QFile outFile(app.arguments().at(1));
    if (!outFile.open(QIODevice::WriteOnly | QIODevice::Truncate))
        return 7;

    // write out json
    outFile.write(QJsonDocument::fromVariant(QVariant(hls)).toBinaryData());

    // be done
    return 0;
}
