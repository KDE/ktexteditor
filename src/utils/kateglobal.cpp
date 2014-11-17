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
#include "config.h"

#include <ktexteditor_version.h>

#include "katedocument.h"
#include "kateview.h"
#include "katerenderer.h"
#include "katecmd.h"
#include "katecmds.h"
#include "katesedcmd.h"
#include "katehighlightingcmds.h"
#include "katemodemanager.h"
#include "kateschema.h"
#include "kateschemaconfig.h"
#include "kateconfig.h"
#include "katescriptmanager.h"
#include "katebuffer.h"
#include "katewordcompletion.h"
#include "katekeywordcompletion.h"
#include "spellcheck/spellcheck.h"
#include "katepartdebug.h"
#include "katedefaultcolors.h"

#include "katenormalinputmodefactory.h"
#include "kateviinputmodefactory.h"

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
#include <QPushButton>
#include <QStringListModel>

#ifdef HAVE_GIT2
#include <git2.h>
#endif

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
    , m_aboutData(QLatin1String("katepart"), i18n("Kate Part"), QLatin1String(KTEXTEDITOR_VERSION_STRING),
                  i18n("Embeddable editor component"), KAboutLicense::LGPL_V2,
                  i18n("(c) 2000-2014 The Kate Authors"), QString(), QLatin1String("http://kate-editor.org"))
    , m_application(Q_NULLPTR)
    , m_defaultColors(new KateDefaultColors())
    , m_searchHistoryModel(Q_NULLPTR)
    , m_replaceHistoryModel(Q_NULLPTR)

{
    // remember this
    staticInstance = this;
    
#ifdef HAVE_GIT2
#ifdef HAVE_GIT2_THREADS
    // init libgit2 if used
    git_threads_init();
#endif
#endif
    
    /**
     * register some datatypes
     */
    qRegisterMetaType<KTextEditor::Cursor>("KTextEditor::Cursor");
    qRegisterMetaType<KTextEditor::Document *>("KTextEditor::Document*");
    qRegisterMetaType<KTextEditor::View *>("KTextEditor::View*");

    //
    // fill about data
    //
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
    // input mode factories
    //
    KateAbstractInputModeFactory *fact;
    fact = new KateNormalInputModeFactory();
    m_inputModeFactories.insert(KTextEditor::View::NormalInputMode, fact);

#ifdef BUILD_VIMODE
    fact = new KateViInputModeFactory();
    m_inputModeFactories.insert(KTextEditor::View::ViInputMode, fact);
#endif

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
    m_cmds.push_back(KateCommands::Character::self());
    m_cmds.push_back(KateCommands::Date::self());
    m_cmds.push_back(KateCommands::SedReplace::self());
    m_cmds.push_back(KateCommands::Highlighting::self());

    // global word completion model
    m_wordCompletionModel = new KateWordCompletionModel(this);

    // global keyword completion model
    m_keywordCompletionModel = new KateKeywordCompletionModel (this);

    // tap to QApplication object for color palette changes
    qApp->installEventFilter(this);
}

KTextEditor::EditorPrivate::~EditorPrivate()
{
    delete m_globalConfig;
    delete m_documentConfig;
    delete m_viewConfig;
    delete m_rendererConfig;

    delete m_modeManager;
    delete m_schemaManager;

    delete m_dirWatch;

    // cu managers
    delete m_scriptManager;
    delete m_hlManager;

    delete m_spellCheckManager;

    // cu model
    delete m_wordCompletionModel;

    // delete the commands before we delete the cmd manager
    qDeleteAll(m_cmds);
    delete m_cmdManager;

    qDeleteAll(m_inputModeFactories);
    
#ifdef HAVE_GIT2
#ifdef HAVE_GIT2_THREADS
    // shutdown libgit2 if used
    git_threads_shutdown();
#endif
#endif
}

KTextEditor::Document *KTextEditor::EditorPrivate::createDocument(QObject *parent)
{
    KTextEditor::DocumentPrivate *doc = new KTextEditor::DocumentPrivate(false, false, 0, parent);

    emit documentCreated(this, doc);

    return doc;
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
        QFrame *page = new QFrame();
        KTextEditor::ConfigPage *cp = configPage(i, page);

        KPageWidgetItem *item = kd->addPage(page, cp->name());
        item->setHeader(cp->fullName());
        item->setIcon(cp->icon());

        QVBoxLayout *topLayout = new QVBoxLayout(page);
        topLayout->setMargin(0);

        connect(kd->button(QDialogButtonBox::Apply), SIGNAL(clicked()), cp, SLOT(apply()));
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
    Q_ASSERT (!m_documents.contains(doc));
    m_documents.insert(doc, doc);
}

void KTextEditor::EditorPrivate::deregisterDocument(KTextEditor::DocumentPrivate *doc)
{
    Q_ASSERT (m_documents.contains(doc));
    m_documents.remove(doc);
}

void KTextEditor::EditorPrivate::registerView(KTextEditor::ViewPrivate *view)
{
    Q_ASSERT (!m_views.contains(view));
    m_views.insert(view);
}

void KTextEditor::EditorPrivate::deregisterView(KTextEditor::ViewPrivate *view)
{
    Q_ASSERT (m_views.contains(view));
    m_views.remove(view);
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

void KTextEditor::EditorPrivate::updateColorPalette()
{
    // update default color cache
    m_defaultColors.reset(new KateDefaultColors());

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

QList< KateAbstractInputModeFactory *> KTextEditor::EditorPrivate::inputModeFactories()
{
    return m_inputModeFactories.values();
}

QStringListModel *KTextEditor::EditorPrivate::searchHistoryModel()
{
    if (!m_searchHistoryModel) {
        KConfigGroup cg(KSharedConfig::openConfig(), "KTextEditor::Search");
        const QStringList history = cg.readEntry(QStringLiteral("Search History"), QStringList());
        m_searchHistoryModel = new QStringListModel(history, this);
    }
    return m_searchHistoryModel;
}

QStringListModel *KTextEditor::EditorPrivate::replaceHistoryModel()
{
    if (!m_replaceHistoryModel) {
        KConfigGroup cg(KSharedConfig::openConfig(), "KTextEditor::Search");
        const QStringList history = cg.readEntry(QStringLiteral("Replace History"), QStringList());
        m_replaceHistoryModel = new QStringListModel(history, this);
    }
    return m_replaceHistoryModel;
}

void KTextEditor::EditorPrivate::saveSearchReplaceHistoryModels()
{
    KConfigGroup cg(KSharedConfig::openConfig(), "KTextEditor::Search");
    if (m_searchHistoryModel) {
        cg.writeEntry(QStringLiteral("Search History"), m_searchHistoryModel->stringList());
    }
    if (m_replaceHistoryModel) {
        cg.writeEntry(QStringLiteral("Replace History"), m_replaceHistoryModel->stringList());
    }
}
