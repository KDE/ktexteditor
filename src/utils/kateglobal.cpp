/*  This file is part of the KDE libraries and the Kate part.
 *
 *  Copyright (C) 2001-2010 Christoph Cullmann <cullmann@kde.org>
 *  Copyright (C) 2009 Erlend Hamberg <ehamberg@gmail.com>
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

#include "kateglobal.h"

#include <ktexteditor_version.h>

#include "katedocument.h"
#include "kateview.h"
#include "katerenderer.h"
#include "katecmds.h"
#include "katemodemanager.h"
#include "kateschema.h"
#include "kateschemaconfig.h"
#include "kateconfig.h"
#include "katescriptmanager.h"
#include "katecmd.h"
#include "katebuffer.h"
#include "kateviglobal.h"
#include "katewordcompletion.h"
#include "spellcheck/spellcheck.h"
#include "katepartdebug.h"

#include <KServiceTypeTrader>
#include <kdirwatch.h>
#include <KLocalizedString>
#include <KAboutData>
#include <KPageDialog>
#include <KPageWidgetModel>
#include <KIconLoader>
#include <KConfigGroup>

#include <QBoxLayout>
#include <QApplication>
#include <QClipboard>

Q_LOGGING_CATEGORY(LOG_PART, "katepart")

//BEGIN unit test mode
static bool kateUnitTestMode = false;
void KTextEditor::EditorPrivate::enableUnitTestMode()
{
    kateUnitTestMode = true;
}

bool KTextEditor::EditorPrivate::unitTestMode()
{
    return kateUnitTestMode;
}
//END unit test mode

KTextEditor::EditorPrivate::EditorPrivate(QPointer<KTextEditor::EditorPrivate> &staticInstance)
    : KTextEditor::Editor (this)
    , m_aboutData(QLatin1String("katepart"), QString(), i18n("Kate Part"), QLatin1String(KTEXTEDITOR_VERSION_STRING),
                  i18n("Embeddable editor component"), KAboutData::License_LGPL_V2,
                  i18n("(c) 2000-2014 The Kate Authors"), QString(), QLatin1String("http://kate-editor.org"))
    , m_sessionConfig(KSharedConfig::openConfig())
    , m_application(nullptr)
{
    // FIXME KF5
    QLoggingCategory::setFilterRules(QStringLiteral("katepart = true"));

    // remember this
    staticInstance = this;

    /**
     * register some datatypes
     */
    qRegisterMetaType<KTextEditor::Cursor>("KTextEditor::Cursor");
    qRegisterMetaType<KTextEditor::Document *>("KTextEditor::Document*");
    qRegisterMetaType<KTextEditor::View *>("KTextEditor::View*");

    // load the kate part translation catalog
    // FIXME: kf5
    // KLocale::global()->insertCatalog("katepart4");

    //
    // fill about data
    //
    m_aboutData.setProgramIconName(QLatin1String("preferences-plugin"));
    m_aboutData.addAuthor(i18n("Christoph Cullmann"), i18n("Maintainer"), QLatin1String("cullmann@kde.org"), QLatin1String("http://www.cullmann.io"));
    m_aboutData.addAuthor(i18n("Dominik Haumann"), i18n("Core Developer"), QLatin1String("dhaumann@kde.org"));
    m_aboutData.addAuthor(i18n("Milian Wolff"), i18n("Core Developer"), QLatin1String("mail@milianw.de"), QLatin1String("http://milianw.de"));
    m_aboutData.addAuthor(i18n("Joseph Wenninger"), i18n("Core Developer"), QLatin1String("jowenn@kde.org"), QLatin1String("http://stud3.tuwien.ac.at/~e9925371"));
    m_aboutData.addAuthor(i18n("Erlend Hamberg"), i18n("Vi Input Mode"), QLatin1String("ehamberg@gmail.com"), QLatin1String("http://hamberg.no/erlend"));
    m_aboutData.addAuthor(i18n("Bernhard Beschow"), i18n("Developer"), QLatin1String("bbeschow@cs.tu-berlin.de"), QLatin1String("https://user.cs.tu-berlin.de/~bbeschow"));
    m_aboutData.addAuthor(i18n("Anders Lund"), i18n("Core Developer"), QLatin1String("anders@alweb.dk"), QLatin1String("http://www.alweb.dk"));
    m_aboutData.addAuthor(i18n("Michel Ludwig"), i18n("On-the-fly spell checking"), QLatin1String("michel.ludwig@kdemail.net"));
    m_aboutData.addAuthor(i18n("Pascal Létourneau"), i18n("Large scale bug fixing"), QLatin1String("pascal.letourneau@gmail.com"));
    m_aboutData.addAuthor(i18n("Hamish Rodda"), i18n("Core Developer"), QLatin1String("rodda@kde.org"));
    m_aboutData.addAuthor(i18n("Waldo Bastian"), i18n("The cool buffersystem"), QLatin1String("bastian@kde.org"));
    m_aboutData.addAuthor(i18n("Charles Samuels"), i18n("The Editing Commands"), QLatin1String("charles@kde.org"));
    m_aboutData.addAuthor(i18n("Matt Newell"), i18n("Testing, ..."), QLatin1String("newellm@proaxis.com"));
    m_aboutData.addAuthor(i18n("Michael Bartl"), i18n("Former Core Developer"), QLatin1String("michael.bartl1@chello.at"));
    m_aboutData.addAuthor(i18n("Michael McCallum"), i18n("Core Developer"), QLatin1String("gholam@xtra.co.nz"));
    m_aboutData.addAuthor(i18n("Michael Koch"), i18n("KWrite port to KParts"), QLatin1String("koch@kde.org"));
    m_aboutData.addAuthor(i18n("Christian Gebauer"), QString(), QLatin1String("gebauer@kde.org"));
    m_aboutData.addAuthor(i18n("Simon Hausmann"), QString(), QLatin1String("hausmann@kde.org"));
    m_aboutData.addAuthor(i18n("Glen Parker"), i18n("KWrite Undo History, Kspell integration"), QLatin1String("glenebob@nwlink.com"));
    m_aboutData.addAuthor(i18n("Scott Manson"), i18n("KWrite XML Syntax highlighting support"), QLatin1String("sdmanson@alltel.net"));
    m_aboutData.addAuthor(i18n("John Firebaugh"), i18n("Patches and more"), QLatin1String("jfirebaugh@kde.org"));
    m_aboutData.addAuthor(i18n("Andreas Kling"), i18n("Developer"), QLatin1String("kling@impul.se"));
    m_aboutData.addAuthor(i18n("Mirko Stocker"), i18n("Various bugfixes"), QLatin1String("me@misto.ch"), QLatin1String("http://misto.ch/"));
    m_aboutData.addAuthor(i18n("Matthew Woehlke"), i18n("Selection, KColorScheme integration"), QLatin1String("mw_triad@users.sourceforge.net"));
    m_aboutData.addAuthor(i18n("Sebastian Pipping"), i18n("Search bar back- and front-end"), QLatin1String("webmaster@hartwork.org"), QLatin1String("http://www.hartwork.org/"));
    m_aboutData.addAuthor(i18n("Jochen Wilhelmy"), i18n("Original KWrite Author"), QLatin1String("digisnap@cs.tu-berlin.de"));
    m_aboutData.addAuthor(i18n("Gerald Senarclens de Grancy"), i18n("QA and Scripting"), QLatin1String("oss@senarclens.eu"), QLatin1String("http://find-santa.eu/"));

    m_aboutData.addCredit(i18n("Matteo Merli"), i18n("Highlighting for RPM Spec-Files, Perl, Diff and more"), QLatin1String("merlim@libero.it"));
    m_aboutData.addCredit(i18n("Rocky Scaletta"), i18n("Highlighting for VHDL"), QLatin1String("rocky@purdue.edu"));
    m_aboutData.addCredit(i18n("Yury Lebedev"), i18n("Highlighting for SQL"), QString());
    m_aboutData.addCredit(i18n("Chris Ross"), i18n("Highlighting for Ferite"), QString());
    m_aboutData.addCredit(i18n("Nick Roux"), i18n("Highlighting for ILERPG"), QString());
    m_aboutData.addCredit(i18n("Carsten Niehaus"), i18n("Highlighting for LaTeX"), QString());
    m_aboutData.addCredit(i18n("Per Wigren"), i18n("Highlighting for Makefiles, Python"), QString());
    m_aboutData.addCredit(i18n("Jan Fritz"), i18n("Highlighting for Python"), QString());
    m_aboutData.addCredit(i18n("Daniel Naber"));
    m_aboutData.addCredit(i18n("Roland Pabel"), i18n("Highlighting for Scheme"), QString());
    m_aboutData.addCredit(i18n("Cristi Dumitrescu"), i18n("PHP Keyword/Datatype list"), QString());
    m_aboutData.addCredit(i18n("Carsten Pfeiffer"), i18n("Very nice help"), QString());
    m_aboutData.addCredit(i18n("Bruno Massa"), i18n("Highlighting for Lua"), QLatin1String("brmassa@gmail.com"));

    m_aboutData.addCredit(i18n("All people who have contributed and I have forgotten to mention"));

    m_aboutData.setTranslator(i18nc("NAME OF TRANSLATORS", "Your names"), i18nc("EMAIL OF TRANSLATORS", "Your emails"));

    //
    // dir watch
    //
    m_dirWatch = new KDirWatch();

    //
    // command manager
    //
    m_cmdManager = new KateCmd();

    //
    // hl manager
    //
    m_hlManager = new KateHlManager();

    //
    // mode man
    //
    m_modeManager = new KateModeManager();

    //
    // schema man
    //
    m_schemaManager = new KateSchemaManager();

    //
    // vi input mode global
    //
    m_viInputModeGlobal = new KateViGlobal();

    //
    // spell check manager
    //
    m_spellCheckManager = new KateSpellCheckManager();

    // config objects
    m_globalConfig = new KateGlobalConfig();
    m_documentConfig = new KateDocumentConfig();
    m_viewConfig = new KateViewConfig();
    m_rendererConfig = new KateRendererConfig();

    // create script manager (search scripts)
    m_scriptManager = KateScriptManager::self();

    //
    // init the cmds
    //
    m_cmds.push_back(KateCommands::CoreCommands::self());
    m_cmds.push_back(KateCommands::ViCommands::self());
    m_cmds.push_back(KateCommands::AppCommands::self());
    m_cmds.push_back(KateCommands::SedReplace::self());
    m_cmds.push_back(KateCommands::Character::self());
    m_cmds.push_back(KateCommands::Date::self());

    for (QList<KTextEditor::Command *>::iterator it = m_cmds.begin(); it != m_cmds.end(); ++it) {
        m_cmdManager->registerCommand(*it);
    }

    // global word completion model
    m_wordCompletionModel = new KateWordCompletionModel(this);

    // tap to QApplication object
    // TODO: recheck the frameworks, if there is a better way of handling the PaletteChange "signal"
    qApp->installEventFilter(this);

    //required for setting sessionConfig property
    qRegisterMetaType<KSharedConfig::Ptr>("KSharedConfig::Ptr");
}

KTextEditor::EditorPrivate::~EditorPrivate()
{
    delete m_globalConfig;
    delete m_documentConfig;
    delete m_viewConfig;
    delete m_rendererConfig;

    delete m_modeManager;
    delete m_schemaManager;

    delete m_viInputModeGlobal;

    delete m_dirWatch;

    // you too
    qDeleteAll(m_cmds);

    // cu managers
    delete m_scriptManager;
    delete m_hlManager;
    delete m_cmdManager;

    delete m_spellCheckManager;

    // cu model
    delete m_wordCompletionModel;
}

KTextEditor::Document *KTextEditor::EditorPrivate::createDocument(QObject *parent)
{
    KTextEditor::DocumentPrivate *doc = new KTextEditor::DocumentPrivate(false, false, 0, parent);

    emit documentCreated(this, doc);

    return doc;
}

QList<KTextEditor::Document *> KTextEditor::EditorPrivate::documents()
{
    return m_docs;
}

//BEGIN KTextEditor::Editor config stuff
void KTextEditor::EditorPrivate::readConfig(KConfig *config)
{
    if (!config) {
        config = KSharedConfig::openConfig().data();
    }

    KateGlobalConfig::global()->readConfig(KConfigGroup(config, "Kate Part Defaults"));

    KateDocumentConfig::global()->readConfig(KConfigGroup(config, "Kate Document Defaults"));

    KateViewConfig::global()->readConfig(KConfigGroup(config, "Kate View Defaults"));

    KateRendererConfig::global()->readConfig(KConfigGroup(config, "Kate Renderer Defaults"));

    m_viInputModeGlobal->readConfig(KConfigGroup(config, "Kate Vi Input Mode Settings"));
}

void KTextEditor::EditorPrivate::writeConfig(KConfig *config)
{
    if (!config) {
        config = KSharedConfig::openConfig().data();
    }

    KConfigGroup cgGlobal(config, "Kate Part Defaults");
    KateGlobalConfig::global()->writeConfig(cgGlobal);

    KConfigGroup cg(config, "Kate Document Defaults");
    KateDocumentConfig::global()->writeConfig(cg);

    KConfigGroup cgDefault(config, "Kate View Defaults");
    KateViewConfig::global()->writeConfig(cgDefault);

    KConfigGroup cgRenderer(config, "Kate Renderer Defaults");
    KateRendererConfig::global()->writeConfig(cgRenderer);

    KConfigGroup cgViInputMode(config, "Kate Vi Input Mode Settings");
    m_viInputModeGlobal->writeConfig(cgViInputMode);

    config->sync();
}
//END KTextEditor::Editor config stuff

void KTextEditor::EditorPrivate::configDialog(QWidget *parent)
{
    QPointer<KPageDialog> kd = new KPageDialog(parent);

    kd->setWindowTitle(i18n("Configure"));
    kd->setFaceType(KPageDialog::List);
    kd->setStandardButtons(QDialogButtonBox::Ok | QDialogButtonBox::Cancel | QDialogButtonBox::Apply | QDialogButtonBox::Help);

    QList<KTextEditor::ConfigPage *> editorPages;

    for (int i = 0; i < configPages(); ++i) {
        const QString name = configPageName(i);

        QFrame *page = new QFrame();

        KPageWidgetItem *item = kd->addPage(page, name);
        item->setHeader(configPageFullName(i));
        item->setIcon(configPageIcon(i));

        QVBoxLayout *topLayout = new QVBoxLayout(page);
        topLayout->setMargin(0);

        KTextEditor::ConfigPage *cp = configPage(i, page);
        connect(kd, SIGNAL(applyClicked()), cp, SLOT(apply()));
        topLayout->addWidget(cp);
        editorPages.append(cp);
    }

    if (kd->exec() && kd) {
        KateGlobalConfig::global()->configStart();
        KateDocumentConfig::global()->configStart();
        KateViewConfig::global()->configStart();
        KateRendererConfig::global()->configStart();

        for (int i = 0; i < editorPages.count(); ++i) {
            editorPages.at(i)->apply();
        }

        KateGlobalConfig::global()->configEnd();
        KateDocumentConfig::global()->configEnd();
        KateViewConfig::global()->configEnd();
        KateRendererConfig::global()->configEnd();
    }

    delete kd;
}

int KTextEditor::EditorPrivate::configPages() const
{
    return 4;
}

KTextEditor::ConfigPage *KTextEditor::EditorPrivate::configPage(int number, QWidget *parent)
{
    switch (number) {
    case 0:
        return new KateViewDefaultsConfig(parent);

    case 1:
        return new KateSchemaConfigPage(parent);

    case 2:
        return new KateEditConfigTab(parent);

    case 3:
        return new KateSaveConfigTab(parent);

    default:
        break;
    }

    return 0;
}

QString KTextEditor::EditorPrivate::configPageName(int number) const
{
    switch (number) {
    case 0:
        return i18n("Appearance");

    case 1:
        return i18n("Fonts & Colors");

    case 2:
        return i18n("Editing");

    case 3:
        return i18n("Open/Save");

    default:
        break;
    }

    return QString();
}

QString KTextEditor::EditorPrivate::configPageFullName(int number) const
{
    switch (number) {
    case 0:
        return i18n("Appearance");

    case 1:
        return i18n("Font & Color Schemas");

    case 2:
        return i18n("Editing Options");

    case 3:
        return i18n("File Opening & Saving");

    default:
        break;
    }

    return QString();
}

QIcon KTextEditor::EditorPrivate::configPageIcon(int number) const
{
    switch (number) {
    case 0:
        return QIcon::fromTheme(QLatin1String("preferences-desktop-theme"));

    case 1:
        return QIcon::fromTheme(QLatin1String("preferences-desktop-color"));

    case 2:
        return QIcon::fromTheme(QLatin1String("accessories-text-editor"));

    case 3:
        return QIcon::fromTheme(QLatin1String("document-save"));

    default:
        break;
    }

    return QIcon::fromTheme(QLatin1String("document-properties"));
}

/**
 * Cleanup the KTextEditor::EditorPrivate during QCoreApplication shutdown
 */
static void cleanupGlobal()
{
    /**
     * delete if there
     */
    delete KTextEditor::EditorPrivate::self();
}

KTextEditor::EditorPrivate *KTextEditor::EditorPrivate::self()
{
    /**
     * remember the static instance in a QPointer
     */
    static bool inited = false;
    static QPointer<KTextEditor::EditorPrivate> staticInstance;

    /**
     * just return it, if already inited
     */
    if (inited) {
        return staticInstance.data();
    }

    /**
     * start init process
     */
    inited = true;

    /**
     * now create the object and store it
     */
    new KTextEditor::EditorPrivate(staticInstance);

    /**
     * register cleanup
     * let use be deleted during QCoreApplication shutdown
     */
    qAddPostRoutine(cleanupGlobal);

    /**
     * return instance
     */
    return staticInstance.data();
}

void KTextEditor::EditorPrivate::registerDocument(KTextEditor::DocumentPrivate *doc)
{
    m_documents.append(doc);
    m_docs.append(doc);
}

void KTextEditor::EditorPrivate::deregisterDocument(KTextEditor::DocumentPrivate *doc)
{
    m_docs.removeAll(doc);
    m_documents.removeAll(doc);
}

void KTextEditor::EditorPrivate::registerView(KTextEditor::ViewPrivate *view)
{
    m_views.append(view);
}

void KTextEditor::EditorPrivate::deregisterView(KTextEditor::ViewPrivate *view)
{
    m_views.removeAll(view);
}

//BEGIN command interface
bool KTextEditor::EditorPrivate::registerCommand(KTextEditor::Command *cmd)
{
    return m_cmdManager->registerCommand(cmd);
}

bool KTextEditor::EditorPrivate::unregisterCommand(KTextEditor::Command *cmd)
{
    return m_cmdManager->unregisterCommand(cmd);
}

KTextEditor::Command *KTextEditor::EditorPrivate::queryCommand(const QString &cmd) const
{
    return m_cmdManager->queryCommand(cmd);
}

QList<KTextEditor::Command *> KTextEditor::EditorPrivate::commands() const
{
    return m_cmdManager->commands();
}

QStringList KTextEditor::EditorPrivate::commandList() const
{
    return m_cmdManager->commandList();
}
//END command interface

KTextEditor::TemplateScript *KTextEditor::EditorPrivate::registerTemplateScript(QObject *owner, const QString &script)
{
    return scriptManager()->registerTemplateScript(owner, script);
}

void KTextEditor::EditorPrivate::unregisterTemplateScript(KTextEditor::TemplateScript *templateScript)
{
    scriptManager()->unregisterTemplateScript(templateScript);
}

void KTextEditor::EditorPrivate::updateColorPalette()
{
    // reload the global schema (triggers reload for every view as well)
    m_rendererConfig->reloadSchema();

    // force full update of all view caches and colors
    m_rendererConfig->updateConfig();
}

void KTextEditor::EditorPrivate::copyToClipboard(const QString &text)
{
    /**
     * empty => nop
     */
    if (text.isEmpty()) {
        return;
    }

    /**
     * move to clipboard
     */
    QApplication::clipboard()->setText(text, QClipboard::Clipboard);

    /**
     * remember in history
     * cut after 10 entries
     */
    m_clipboardHistory.prepend(text);
    if (m_clipboardHistory.size() > 10) {
        m_clipboardHistory.removeLast();
    }

    /**
     * notify about change
     */
    emit clipboardHistoryChanged();
}

bool KTextEditor::EditorPrivate::eventFilter(QObject *obj, QEvent *event)
{
    Q_UNUSED(obj);

    if (event->type() == QEvent::ApplicationPaletteChange) {
        updateColorPalette();
    }

    return false; // always continue processing
}

