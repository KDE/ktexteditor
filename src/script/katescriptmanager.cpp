// This file is part of the KDE libraries
// Copyright (C) 2005 Christoph Cullmann <cullmann@kde.org>
// Copyright (C) 2005 Joseph Wenninger <jowenn@kde.org>
// Copyright (C) 2006, 2009 Dominik Haumann <dhaumann kde org>
// Copyright (C) 2008 Paul Giannaros <paul@giannaros.org>
// Copyright (C) 2010 Joseph Wenninger <jowenn@kde.org>
//
// This library is free software; you can redistribute it and/or
// modify it under the terms of the GNU Library General Public
// License version 2 as published by the Free Software Foundation.
//
// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// Library General Public License for more details.
//
// You should have received a copy of the GNU Library General Public License
// along with this library; see the file COPYING.LIB.  If not, write to
// the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
// Boston, MA 02110-1301, USA.

#include "katescriptmanager.h"

#include <ktexteditor_version.h>

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QStringList>
#include <QMap>
#include <QUuid>
#include <QJsonDocument>
#include <QJsonValue>
#include <QJsonArray>

#include <KLocalizedString>
#include <KConfig>
#include <KConfigGroup>

#include "kateglobal.h"
#include "katecmd.h"
#include "katepartdebug.h"

KateScriptManager *KateScriptManager::m_instance = 0;

KateScriptManager::KateScriptManager()
    : KTextEditor::Command(QStringList() << QLatin1String("reload-scripts"))
{
    // use cached info
    collect();
}

KateScriptManager::~KateScriptManager()
{
    qDeleteAll(m_indentationScripts);
    qDeleteAll(m_commandLineScripts);
    m_instance = 0;
}

KateIndentScript *KateScriptManager::indenter(const QString &language)
{
    KateIndentScript *highestPriorityIndenter = 0;
    foreach (KateIndentScript *indenter, m_languageToIndenters.value(language.toLower())) {
        // don't overwrite if there is already a result with a higher priority
        if (highestPriorityIndenter && indenter->indentHeader().priority() < highestPriorityIndenter->indentHeader().priority()) {
#ifdef DEBUG_SCRIPTMANAGER
            qCDebug(LOG_PART) << "Not overwriting indenter for"
                              << language << "as the priority isn't big enough (" <<
                              indenter->indentHeader().priority() << '<'
                              << highestPriorityIndenter->indentHeader().priority() << ')';
#endif
        } else {
            highestPriorityIndenter = indenter;
        }
    }

#ifdef DEBUG_SCRIPTMANAGER
    if (highestPriorityIndenter) {
        qCDebug(LOG_PART) << "Found indenter" << highestPriorityIndenter->url() << "for" << language;
    } else {
        qCDebug(LOG_PART) << "No indenter for" << language;
    }
#endif

    return highestPriorityIndenter;
}

/**
 * Small helper: QJsonValue to QStringList
 */
static QStringList jsonToStringList (const QJsonValue &value)
{
    QStringList list;

    Q_FOREACH (const QJsonValue &value, value.toArray()) {
        if (value.isString()) {
            list.append(value.toString());
        }
    }
    
    return list;
}

void KateScriptManager::collect()
{
    // clear out the old scripts and reserve enough space
    qDeleteAll(m_indentationScripts);
    qDeleteAll(m_commandLineScripts);
    m_indentationScripts.clear();
    m_commandLineScripts.clear();

    m_languageToIndenters.clear();
    m_indentationScriptMap.clear();

    /**
     * now, we search all kinds of known scripts
     */
    foreach (const QString &type, QStringList() << QLatin1String("indentation") << QLatin1String("commands")) {
        // get a list of all unique .js files for the current type
        const QString basedir = QLatin1String("katepart5/script/") + type;
        const QStringList dirs = QStandardPaths::locateAll(QStandardPaths::GenericDataLocation, basedir, QStandardPaths::LocateDirectory);

        QStringList list;

        foreach (const QString &dir, dirs) {
            const QStringList fileNames = QDir(dir).entryList(QStringList() << QStringLiteral("*.js"));
            foreach (const QString &file, fileNames) {
                list.append(dir + QLatin1Char('/') + file);
            }
        }

        // iterate through the files and read info out of cache or file
        foreach (const QString &fileName, list) {
            /**
             * get file basename
             */
            const QString baseName = QFileInfo(fileName).baseName();

            /**
             * open file or skip it
             */
            QFile file(fileName);
            if (!file.open(QIODevice::ReadOnly)) {
                qCDebug(LOG_PART) << "Script parse error: Cannot open file " << qPrintable(fileName) << '\n';
                continue;
            }
    
            /**
             * search json header or skip this file
             */
            QByteArray fileContent = file.readAll();
            int startOfJson = fileContent.indexOf ('{');
            if (startOfJson < 0) {
                qCDebug(LOG_PART) << "Script parse error: Cannot find start of json header at start of file " << qPrintable(fileName) << '\n';
                continue;
            }
        
            /**
             * parse json header or skip this file
             */
            const QJsonDocument metaInfo (QJsonDocument::fromJson(fileContent.right(fileContent.size()-startOfJson)));
            if (metaInfo.isNull() || !metaInfo.isObject()) {
                qCDebug(LOG_PART) << "Script parse error: Cannot parse json header at start of file " << qPrintable(fileName) << '\n';
                continue;
            }
    
            /**
             * remember type
             */
            KateScriptHeader generalHeader;
            if (type == QLatin1String("indentation")) {
                generalHeader.setScriptType(Kate::IndentationScript);
            } else if (type == QLatin1String("commands")) {
                generalHeader.setScriptType(Kate::CommandLineScript);
            } else {
                // should never happen, we dictate type by directory
                Q_ASSERT(false);
            }

            const QJsonObject metaInfoObject = metaInfo.object();
            generalHeader.setLicense(metaInfoObject.value(QLatin1String("license")).toString());
            generalHeader.setAuthor(metaInfoObject.value(QLatin1String("author")).toString());
            generalHeader.setRevision(metaInfoObject.value(QLatin1String("revision")).toInt());
            generalHeader.setKateVersion(metaInfoObject.value(QLatin1String("kate-version")).toString());
            generalHeader.setCatalog(metaInfoObject.value(QLatin1String("i18n-catalog")).toString());

            // now, cast accordingly based on type
            switch (generalHeader.scriptType()) {
            case Kate::IndentationScript: {
                KateIndentScriptHeader indentHeader;
                indentHeader.setName(metaInfoObject.value(QLatin1String("name")).toString());
                indentHeader.setBaseName(baseName);
                if (indentHeader.name().isNull()) {
                    qCDebug(LOG_PART) << "Script value error: No name specified in script meta data: "
                                      << qPrintable(fileName) << '\n' << "-> skipping indenter" << '\n';
                    continue;
                }

                // required style?
                indentHeader.setRequiredStyle(metaInfoObject.value(QLatin1String("required-syntax-style")).toString());
                // which languages does this support?
                QStringList indentLanguages = jsonToStringList(metaInfoObject.value(QLatin1String("indent-languages")));
                if (!indentLanguages.isEmpty()) {
                    indentHeader.setIndentLanguages(indentLanguages);
                } else {
                    indentHeader.setIndentLanguages(QStringList() << indentHeader.name());

#ifdef DEBUG_SCRIPTMANAGER
                    qCDebug(LOG_PART) << "Script value warning: No indent-languages specified for indent "
                                      << "script " << qPrintable(fileName) << ". Using the name ("
                                      << qPrintable(indentHeader.name()) << ")\n";
#endif
                }
                // priority
                indentHeader.setPriority(metaInfoObject.value(QLatin1String("priority")).toInt());

                KateIndentScript *script = new KateIndentScript(fileName, indentHeader);
                script->setGeneralHeader(generalHeader);
                foreach (const QString &language, indentHeader.indentLanguages()) {
                    m_languageToIndenters[language.toLower()].push_back(script);
                }

                m_indentationScriptMap.insert(indentHeader.baseName(), script);
                m_indentationScripts.append(script);
                break;
            }
            case Kate::CommandLineScript: {
                KateCommandLineScriptHeader commandHeader;
                commandHeader.setFunctions(jsonToStringList(metaInfoObject.value(QLatin1String("functions"))));
                commandHeader.setActions(metaInfoObject.value(QLatin1String("actions")).toArray());
                if (commandHeader.functions().isEmpty()) {
                    qCDebug(LOG_PART) << "Script value error: No functions specified in script meta data: "
                                      << qPrintable(fileName) << '\n' << "-> skipping script" << '\n';
                    continue;
                }
                KateCommandLineScript *script = new KateCommandLineScript(fileName, commandHeader);
                script->setGeneralHeader(generalHeader);
                m_commandLineScripts.push_back(script);
                break;
            }
            case Kate::UnknownScript:
            default:
                qCDebug(LOG_PART) << "Script value warning: Unknown type ('" << qPrintable(type) << "'): "
                                  << qPrintable(fileName) << '\n';
            }
        }
    }

#ifdef DEBUG_SCRIPTMANAGER
// XX Test
    if (indenter("Python")) {
        qCDebug(LOG_PART) << "Python: " << indenter("Python")->global("triggerCharacters").isValid() << "\n";
        qCDebug(LOG_PART) << "Python: " << indenter("Python")->function("triggerCharacters").isValid() << "\n";
        qCDebug(LOG_PART) << "Python: " << indenter("Python")->global("blafldsjfklas").isValid() << "\n";
        qCDebug(LOG_PART) << "Python: " << indenter("Python")->function("indent").isValid() << "\n";
    }
    if (indenter("C")) {
        qCDebug(LOG_PART) << "C: " << qPrintable(indenter("C")->url()) << "\n";
    }
    if (indenter("lisp")) {
        qCDebug(LOG_PART) << "LISP: " << qPrintable(indenter("Lisp")->url()) << "\n";
    }
#endif
}

void KateScriptManager::reload()
{
    collect();
    emit reloaded();
}

/// Kate::Command stuff

bool KateScriptManager::exec(KTextEditor::View *view, const QString &_cmd, QString &errorMsg, const KTextEditor::Range &)
{
    QStringList args(_cmd.split(QRegExp(QLatin1String("\\s+")), QString::SkipEmptyParts));
    QString cmd(args.first());
    args.removeFirst();

    if (!view) {
        errorMsg = i18n("Could not access view");
        return false;
    }

    if (cmd == QLatin1String("reload-scripts")) {
        reload();
        return true;
    } else {
        errorMsg = i18n("Command not found: %1", cmd);
        return false;
    }
}

bool KateScriptManager::help(KTextEditor::View *view, const QString &cmd, QString &msg)
{
    Q_UNUSED(view)

    if (cmd == QLatin1String("reload-scripts")) {
        msg = i18n("Reload all JavaScript files (indenters, command line scripts, etc).");
        return true;
    } else {
        msg = i18n("Command not found: %1", cmd);
        return false;
    }
}

KTextEditor::TemplateScript *KateScriptManager::registerTemplateScript(QObject *owner, const QString &script)
{
    KateTemplateScript *kts = new KateTemplateScript(script);
    m_templateScripts.append(kts);

    connect(owner, SIGNAL(destroyed(QObject*)),
            this, SLOT(slotTemplateScriptOwnerDestroyed(QObject*)));
    m_ownerScript.insertMulti(owner, kts);
    return kts;
}

void KateScriptManager::unregisterTemplateScript(KTextEditor::TemplateScript *templateScript)
{
    int index = m_templateScripts.indexOf(templateScript);
    if (index != -1) {
        QObject *k = m_ownerScript.key(templateScript);
        m_ownerScript.remove(k, templateScript);
        m_templateScripts.removeAt(index);
        delete templateScript;
    }
}

void KateScriptManager::slotTemplateScriptOwnerDestroyed(QObject *owner)
{
    while (m_ownerScript.contains(owner)) {
        KTextEditor::TemplateScript *templateScript = m_ownerScript.take(owner);
        qCDebug(LOG_PART) << "Destroying template script" << templateScript;
        m_templateScripts.removeAll(templateScript);
        delete templateScript;
    }
}

KateTemplateScript *KateScriptManager::templateScript(KTextEditor::TemplateScript *templateScript)
{
    if (m_templateScripts.contains(templateScript)) {
        return static_cast<KateTemplateScript *>(templateScript);
    }

    return 0;
}

