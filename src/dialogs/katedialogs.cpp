/* This file is part of the KDE libraries
   Copyright (C) 2002, 2003 Anders Lund <anders.lund@lund.tdcadsl.dk>
   Copyright (C) 2003 Christoph Cullmann <cullmann@kde.org>
   Copyright (C) 2001 Joseph Wenninger <jowenn@kde.org>
   Copyright (C) 2006 Dominik Haumann <dhdev@gmx.de>
   Copyright (C) 2007 Mirko Stocker <me@misto.ch>
   Copyright (C) 2009 Michel Ludwig <michel.ludwig@kdemail.net>
   Copyright (C) 2009 Erlend Hamberg <ehamberg@gmail.com>

   Based on work of:
     Copyright (C) 1999 Jochen Wilhelmy <digisnap@cs.tu-berlin.de>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License version 2 as published by the Free Software Foundation.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

//BEGIN Includes
#include "katedialogs.h"

#include <ktexteditor_version.h>
#include <ktexteditor/message.h>

#include "kateautoindent.h"
#include "katebuffer.h"
#include "kateconfig.h"
#include "katedocument.h"
#include "kateglobal.h"
#include "kateschema.h"
#include "katemodeconfigpage.h"
#include "kateview.h"
#include "spellcheck/spellcheck.h"

// auto generated ui files
#include "ui_textareaappearanceconfigwidget.h"
#include "ui_bordersappearanceconfigwidget.h"
#include "ui_navigationconfigwidget.h"
#include "ui_editconfigwidget.h"
#include "ui_indentationconfigwidget.h"
#include "ui_completionconfigtab.h"
#include "ui_opensaveconfigwidget.h"
#include "ui_opensaveconfigadvwidget.h"
#include "ui_spellcheckconfigwidget.h"

#include <KIO/Job>
#include <kjobwidgets.h>

#include <KCharsets>
#include <KColorCombo>
#include <KComboBox>
#include <KIconLoader>
#include "katepartdebug.h"
#include "kateabstractinputmodefactory.h"
#include <KShortcutsDialog>
#include <KLineEdit>
#include <KMessageBox>
#include <KProcess>
#include <KRun>
#include <KSeparator>
#include <KActionCollection>

#include <QFile>
#include <QClipboard>
#include <QMap>
#include <QStringList>
#include <QTextCodec>
#include <QTextStream>
#include <QTemporaryFile>
#include <QKeyEvent>
#include <QPainter>
#include <QCheckBox>
#include <QComboBox>
#include <QDialog>
#include <QFileDialog>
#include <QGroupBox>
#include <QLabel>
#include <QLayout>
#include <QRadioButton>
#include <QSettings>
#include <QSlider>
#include <QSpinBox>
#include <QTabBar>
#include <QTabWidget>
#include <QToolButton>
#include <QWhatsThis>
#include <QDomDocument>

//END

//BEGIN KateIndentConfigTab
KateIndentConfigTab::KateIndentConfigTab(QWidget *parent)
    : KateConfigPage(parent)
{
    // This will let us have more separation between this page and
    // the QTabWidget edge (ereslibre)
    QVBoxLayout *layout = new QVBoxLayout;
    QWidget *newWidget = new QWidget(this);

    ui = new Ui::IndentationConfigWidget();
    ui->setupUi(newWidget);

    ui->cmbMode->addItems(KateAutoIndent::listModes());

    ui->label->setTextInteractionFlags(Qt::LinksAccessibleByMouse | Qt::LinksAccessibleByKeyboard);
    connect(ui->label, SIGNAL(linkActivated(QString)), this, SLOT(showWhatsThis(QString)));

    // What's This? help can be found in the ui file

    reload();

    //
    // after initial reload, connect the stuff for the changed () signal
    //

    connect(ui->cmbMode, SIGNAL(activated(int)), this, SLOT(slotChanged()));
    connect(ui->rbIndentWithTabs, SIGNAL(toggled(bool)), this, SLOT(slotChanged()));
    connect(ui->rbIndentWithSpaces, SIGNAL(toggled(bool)), this, SLOT(slotChanged()));
    connect(ui->rbIndentMixed, SIGNAL(toggled(bool)), this, SLOT(slotChanged()));
    connect(ui->rbIndentWithTabs, SIGNAL(toggled(bool)), ui->sbIndentWidth, SLOT(setDisabled(bool)));

    connect(ui->chkKeepExtraSpaces, SIGNAL(toggled(bool)), this, SLOT(slotChanged()));
    connect(ui->chkIndentPaste, SIGNAL(toggled(bool)), this, SLOT(slotChanged()));
    connect(ui->chkBackspaceUnindents, SIGNAL(toggled(bool)), this, SLOT(slotChanged()));

    connect(ui->sbTabWidth, SIGNAL(valueChanged(int)), this, SLOT(slotChanged()));
    connect(ui->sbIndentWidth, SIGNAL(valueChanged(int)), this, SLOT(slotChanged()));

    connect(ui->rbTabAdvances, SIGNAL(toggled(bool)), this, SLOT(slotChanged()));
    connect(ui->rbTabIndents, SIGNAL(toggled(bool)), this, SLOT(slotChanged()));
    connect(ui->rbTabSmart, SIGNAL(toggled(bool)), this, SLOT(slotChanged()));

    layout->addWidget(newWidget);
    setLayout(layout);
}

KateIndentConfigTab::~KateIndentConfigTab()
{
    delete ui;
}

void KateIndentConfigTab::slotChanged()
{
    if (ui->rbIndentWithTabs->isChecked()) {
        ui->sbIndentWidth->setValue(ui->sbTabWidth->value());
    }

    KateConfigPage::slotChanged();
}

void KateIndentConfigTab::showWhatsThis(const QString &text)
{
    QWhatsThis::showText(QCursor::pos(), text);
}

void KateIndentConfigTab::apply()
{
    // nothing changed, no need to apply stuff
    if (!hasChanged()) {
        return;
    }
    m_changed = false;

    KateDocumentConfig::global()->configStart();

    KateDocumentConfig::global()->setKeepExtraSpaces(ui->chkKeepExtraSpaces->isChecked());
    KateDocumentConfig::global()->setBackspaceIndents(ui->chkBackspaceUnindents->isChecked());
    KateDocumentConfig::global()->setIndentPastedText(ui->chkIndentPaste->isChecked());
    KateDocumentConfig::global()->setIndentationWidth(ui->sbIndentWidth->value());
    KateDocumentConfig::global()->setIndentationMode(KateAutoIndent::modeName(ui->cmbMode->currentIndex()));
    KateDocumentConfig::global()->setTabWidth(ui->sbTabWidth->value());
    KateDocumentConfig::global()->setReplaceTabsDyn(ui->rbIndentWithSpaces->isChecked());

    if (ui->rbTabAdvances->isChecked()) {
        KateDocumentConfig::global()->setTabHandling(KateDocumentConfig::tabInsertsTab);
    } else if (ui->rbTabIndents->isChecked()) {
        KateDocumentConfig::global()->setTabHandling(KateDocumentConfig::tabIndents);
    } else {
        KateDocumentConfig::global()->setTabHandling(KateDocumentConfig::tabSmart);
    }

    KateDocumentConfig::global()->configEnd();
}

void KateIndentConfigTab::reload()
{
    ui->sbTabWidth->setSuffix(ki18np(" character", " characters"));
    ui->sbTabWidth->setValue(KateDocumentConfig::global()->tabWidth());
    ui->sbIndentWidth->setSuffix(ki18np(" character", " characters"));
    ui->sbIndentWidth->setValue(KateDocumentConfig::global()->indentationWidth());
    ui->chkKeepExtraSpaces->setChecked(KateDocumentConfig::global()->keepExtraSpaces());
    ui->chkIndentPaste->setChecked(KateDocumentConfig::global()->indentPastedText());
    ui->chkBackspaceUnindents->setChecked(KateDocumentConfig::global()->backspaceIndents());

    ui->rbTabAdvances->setChecked(KateDocumentConfig::global()->tabHandling() == KateDocumentConfig::tabInsertsTab);
    ui->rbTabIndents->setChecked(KateDocumentConfig::global()->tabHandling() == KateDocumentConfig::tabIndents);
    ui->rbTabSmart->setChecked(KateDocumentConfig::global()->tabHandling() == KateDocumentConfig::tabSmart);

    ui->cmbMode->setCurrentIndex(KateAutoIndent::modeNumber(KateDocumentConfig::global()->indentationMode()));

    if (KateDocumentConfig::global()->replaceTabsDyn()) {
        ui->rbIndentWithSpaces->setChecked(true);
    } else {
        if (KateDocumentConfig::global()->indentationWidth() == KateDocumentConfig::global()->tabWidth()) {
            ui->rbIndentWithTabs->setChecked(true);
        } else {
            ui->rbIndentMixed->setChecked(true);
        }
    }

    ui->sbIndentWidth->setEnabled(!ui->rbIndentWithTabs->isChecked());
}

QString KateIndentConfigTab::name() const
{
    return i18n("Indentation");
}

//END KateIndentConfigTab

//BEGIN KateCompletionConfigTab
KateCompletionConfigTab::KateCompletionConfigTab(QWidget *parent)
    : KateConfigPage(parent)
{
    // This will let us have more separation between this page and
    // the QTabWidget edge (ereslibre)
    QVBoxLayout *layout = new QVBoxLayout;
    QWidget *newWidget = new QWidget(this);

    ui = new Ui::CompletionConfigTab();
    ui->setupUi(newWidget);

    // What's This? help can be found in the ui file

    reload();

    //
    // after initial reload, connect the stuff for the changed () signal
    //

    connect(ui->chkAutoCompletionEnabled, SIGNAL(toggled(bool)), this, SLOT(slotChanged()));
    connect(ui->gbWordCompletion, SIGNAL(toggled(bool)), this, SLOT(slotChanged()));
    connect(ui->gbKeywordCompletion, SIGNAL(toggled(bool)), this, SLOT(slotChanged()));
    connect(ui->minimalWordLength, SIGNAL(valueChanged(int)), this, SLOT(slotChanged()));
    connect(ui->removeTail, SIGNAL(toggled(bool)), this, SLOT(slotChanged()));

    layout->addWidget(newWidget);
    setLayout(layout);
}

KateCompletionConfigTab::~KateCompletionConfigTab()
{
    delete ui;
}

void KateCompletionConfigTab::showWhatsThis(const QString &text)
{
    QWhatsThis::showText(QCursor::pos(), text);
}

void KateCompletionConfigTab::apply()
{
    // nothing changed, no need to apply stuff
    if (!hasChanged()) {
        return;
    }
    m_changed = false;

    KateViewConfig::global()->configStart();
    KateViewConfig::global()->setAutomaticCompletionInvocation(ui->chkAutoCompletionEnabled->isChecked());
    KateViewConfig::global()->setWordCompletion(ui->gbWordCompletion->isChecked());
    KateViewConfig::global()->setWordCompletionMinimalWordLength(ui->minimalWordLength->value());
    KateViewConfig::global()->setWordCompletionRemoveTail(ui->removeTail->isChecked());
    KateViewConfig::global()->setKeywordCompletion(ui->gbKeywordCompletion->isChecked());
    KateViewConfig::global()->configEnd();
}

void KateCompletionConfigTab::reload()
{
    ui->chkAutoCompletionEnabled->setChecked(KateViewConfig::global()->automaticCompletionInvocation());
    ui->gbWordCompletion->setChecked(KateViewConfig::global()->wordCompletion());
    ui->minimalWordLength->setValue(KateViewConfig::global()->wordCompletionMinimalWordLength());
    ui->gbKeywordCompletion->setChecked(KateViewConfig::global()->keywordCompletion());
    ui->removeTail->setChecked(KateViewConfig::global()->wordCompletionRemoveTail());
}

QString KateCompletionConfigTab::name() const
{
    return i18n("Auto Completion");
}

//END KateCompletionConfigTab

//BEGIN KateSpellCheckConfigTab
KateSpellCheckConfigTab::KateSpellCheckConfigTab(QWidget *parent)
    : KateConfigPage(parent)
{
    // This will let us have more separation between this page and
    // the QTabWidget edge (ereslibre)
    QVBoxLayout *layout = new QVBoxLayout;
    QWidget *newWidget = new QWidget(this);

    ui = new Ui::SpellCheckConfigWidget();
    ui->setupUi(newWidget);

    // What's This? help can be found in the ui file
    reload();

    //
    // after initial reload, connect the stuff for the changed () signal

    m_sonnetConfigWidget = new Sonnet::ConfigWidget(this);
    connect(m_sonnetConfigWidget, SIGNAL(configChanged()), this, SLOT(slotChanged()));
    layout->addWidget(m_sonnetConfigWidget);

    layout->addWidget(newWidget);
    setLayout(layout);
}

KateSpellCheckConfigTab::~KateSpellCheckConfigTab()
{
    delete ui;
}

void KateSpellCheckConfigTab::showWhatsThis(const QString &text)
{
    QWhatsThis::showText(QCursor::pos(), text);
}

void KateSpellCheckConfigTab::apply()
{
    if (!hasChanged()) {
        // nothing changed, no need to apply stuff
        return;
    }
    m_changed = false;

    // WARNING: this is slightly hackish, but it's currently the only way to
    //          do it, see also the KTextEdit class
    KateDocumentConfig::global()->configStart();
    m_sonnetConfigWidget->save();
    QSettings settings(QStringLiteral("KDE"), QStringLiteral("Sonnet"));
    KateDocumentConfig::global()->setOnTheFlySpellCheck(settings.value(QStringLiteral("checkerEnabledByDefault"), false).toBool());
    KateDocumentConfig::global()->configEnd();

    foreach (KTextEditor::DocumentPrivate *doc, KTextEditor::EditorPrivate::self()->kateDocuments()) {
        doc->refreshOnTheFlyCheck();
    }
}

void KateSpellCheckConfigTab::reload()
{
    // does nothing
}

QString KateSpellCheckConfigTab::name() const
{
    return i18n("Spellcheck");
}

//END KateSpellCheckConfigTab

//BEGIN KateNavigationConfigTab
KateNavigationConfigTab::KateNavigationConfigTab(QWidget *parent)
    : KateConfigPage(parent)
{
    // This will let us having more separation between this page and
    // the QTabWidget edge (ereslibre)
    QVBoxLayout *layout = new QVBoxLayout;
    QWidget *newWidget = new QWidget(this);

    ui = new Ui::NavigationConfigWidget();
    ui->setupUi(newWidget);

    // What's This? Help is in the ui-files

    reload();

    //
    // after initial reload, connect the stuff for the changed () signal
    //

    connect(ui->cbTextSelectionMode, SIGNAL(currentIndexChanged(int)), this, SLOT(slotChanged()));
    connect(ui->chkSmartHome, SIGNAL(toggled(bool)), this, SLOT(slotChanged()));
    connect(ui->chkPagingMovesCursor, SIGNAL(toggled(bool)), this, SLOT(slotChanged()));
    connect(ui->sbAutoCenterCursor, SIGNAL(valueChanged(int)), this, SLOT(slotChanged()));
    connect(ui->chkScrollPastEnd, SIGNAL(toggled(bool)), this, SLOT(slotChanged()));
    connect(ui->chkBackspaceRemoveComposed, SIGNAL(toggled(bool)), this, SLOT(slotChanged()));

    layout->addWidget(newWidget);
    setLayout(layout);
}

KateNavigationConfigTab::~KateNavigationConfigTab()
{
    delete ui;
}

void KateNavigationConfigTab::apply()
{
    // nothing changed, no need to apply stuff
    if (!hasChanged()) {
        return;
    }
    m_changed = false;

    KateViewConfig::global()->configStart();
    KateDocumentConfig::global()->configStart();

    KateDocumentConfig::global()->setSmartHome(ui->chkSmartHome->isChecked());

    KateViewConfig::global()->setAutoCenterLines(qMax(0, ui->sbAutoCenterCursor->value()));
    KateDocumentConfig::global()->setPageUpDownMovesCursor(ui->chkPagingMovesCursor->isChecked());

    KateViewConfig::global()->setPersistentSelection(ui->cbTextSelectionMode->currentIndex() == 1);

    KateViewConfig::global()->setScrollPastEnd(ui->chkScrollPastEnd->isChecked());

    KateViewConfig::global()->setBackspaceRemoveComposed(ui->chkBackspaceRemoveComposed->isChecked());

    KateDocumentConfig::global()->configEnd();
    KateViewConfig::global()->configEnd();
}

void KateNavigationConfigTab::reload()
{
    ui->cbTextSelectionMode->setCurrentIndex(KateViewConfig::global()->persistentSelection() ? 1 : 0);

    ui->chkSmartHome->setChecked(KateDocumentConfig::global()->smartHome());
    ui->chkPagingMovesCursor->setChecked(KateDocumentConfig::global()->pageUpDownMovesCursor());
    ui->sbAutoCenterCursor->setValue(KateViewConfig::global()->autoCenterLines());
    ui->chkScrollPastEnd->setChecked(KateViewConfig::global()->scrollPastEnd());
    ui->chkBackspaceRemoveComposed->setChecked(KateViewConfig::global()->backspaceRemoveComposed());
}

QString KateNavigationConfigTab::name() const
{
    return i18n("Text Navigation");
}

//END KateNavigationConfigTab

//BEGIN KateEditGeneralConfigTab
KateEditGeneralConfigTab::KateEditGeneralConfigTab(QWidget *parent)
    : KateConfigPage(parent)
{
    QVBoxLayout *layout = new QVBoxLayout;
    QWidget *newWidget = new QWidget(this);
    ui = new Ui::EditConfigWidget();
    ui->setupUi(newWidget);

    QList<KateAbstractInputModeFactory *> inputModes = KTextEditor::EditorPrivate::self()->inputModeFactories();
    Q_FOREACH(KateAbstractInputModeFactory *fact, inputModes) {
        ui->cmbInputMode->addItem(fact->name(), static_cast<int>(fact->inputMode()));
    }

    reload();

    connect(ui->chkStaticWordWrap, SIGNAL(toggled(bool)), this, SLOT(slotChanged()));
    connect(ui->chkShowStaticWordWrapMarker, SIGNAL(toggled(bool)), this, SLOT(slotChanged()));
    connect(ui->sbWordWrap, SIGNAL(valueChanged(int)), this, SLOT(slotChanged()));
    connect(ui->chkAutoBrackets, SIGNAL(toggled(bool)), this, SLOT(slotChanged()));
    connect(ui->chkSmartCopyCut, SIGNAL(toggled(bool)), this, SLOT(slotChanged()));
    connect(ui->chkMousePasteAtCursorPosition, SIGNAL(toggled(bool)), this, SLOT(slotChanged()));
    connect(ui->cmbInputMode, SIGNAL(currentIndexChanged(int)), this, SLOT(slotChanged()));

    // "What's this?" help is in the ui-file

    layout->addWidget(newWidget);
    setLayout(layout);
}

KateEditGeneralConfigTab::~KateEditGeneralConfigTab()
{
    delete ui;
}

void KateEditGeneralConfigTab::apply()
{
    // nothing changed, no need to apply stuff
    if (!hasChanged()) {
        return;
    }
    m_changed = false;

    KateViewConfig::global()->configStart();
    KateDocumentConfig::global()->configStart();

    KateDocumentConfig::global()->setWordWrapAt(ui->sbWordWrap->value());
    KateDocumentConfig::global()->setWordWrap(ui->chkStaticWordWrap->isChecked());

    KateRendererConfig::global()->setWordWrapMarker(ui->chkShowStaticWordWrapMarker->isChecked());

    KateViewConfig::global()->setAutoBrackets(ui->chkAutoBrackets->isChecked());
    KateViewConfig::global()->setSmartCopyCut(ui->chkSmartCopyCut->isChecked());
    KateViewConfig::global()->setMousePasteAtCursorPosition(ui->chkMousePasteAtCursorPosition->isChecked());

    KateViewConfig::global()->setInputModeRaw(ui->cmbInputMode->currentData().toInt());

    KateDocumentConfig::global()->configEnd();
    KateViewConfig::global()->configEnd();
}

void KateEditGeneralConfigTab::reload()
{
    ui->chkStaticWordWrap->setChecked(KateDocumentConfig::global()->wordWrap());
    ui->chkShowStaticWordWrapMarker->setChecked(KateRendererConfig::global()->wordWrapMarker());
    ui->sbWordWrap->setSuffix(ki18ncp("Wrap words at (value is at 20 or larger)", " character", " characters"));
    ui->sbWordWrap->setValue(KateDocumentConfig::global()->wordWrapAt());
    ui->chkAutoBrackets->setChecked(KateViewConfig::global()->autoBrackets());
    ui->chkSmartCopyCut->setChecked(KateViewConfig::global()->smartCopyCut());
    ui->chkMousePasteAtCursorPosition->setChecked(KateViewConfig::global()->mousePasteAtCursorPosition());

    const int id = static_cast<int>(KateViewConfig::global()->inputMode());
    ui->cmbInputMode->setCurrentIndex(ui->cmbInputMode->findData(id));
}

QString KateEditGeneralConfigTab::name() const
{
    return i18n("General");
}

//END KateEditGeneralConfigTab

//BEGIN KateEditConfigTab
KateEditConfigTab::KateEditConfigTab(QWidget *parent)
    : KateConfigPage(parent)
    , editConfigTab(new KateEditGeneralConfigTab(this))
    , navigationConfigTab(new KateNavigationConfigTab(this))
    , indentConfigTab(new KateIndentConfigTab(this))
    , completionConfigTab(new KateCompletionConfigTab(this))
    , spellCheckConfigTab(new KateSpellCheckConfigTab(this))
{
    QVBoxLayout *layout = new QVBoxLayout;
    layout->setContentsMargins(0, 0, 0, 0);
    QTabWidget *tabWidget = new QTabWidget(this);

    // add all tabs
    tabWidget->insertTab(0, editConfigTab, editConfigTab->name());
    tabWidget->insertTab(1, navigationConfigTab, navigationConfigTab->name());
    tabWidget->insertTab(2, indentConfigTab, indentConfigTab->name());
    tabWidget->insertTab(3, completionConfigTab, completionConfigTab->name());
    tabWidget->insertTab(4, spellCheckConfigTab, spellCheckConfigTab->name());

    connect(editConfigTab, SIGNAL(changed()), this, SLOT(slotChanged()));
    connect(navigationConfigTab, SIGNAL(changed()), this, SLOT(slotChanged()));
    connect(indentConfigTab, SIGNAL(changed()), this, SLOT(slotChanged()));
    connect(completionConfigTab, SIGNAL(changed()), this, SLOT(slotChanged()));
    connect(spellCheckConfigTab, SIGNAL(changed()), this, SLOT(slotChanged()));

    int i = tabWidget->count();
    Q_FOREACH(KateAbstractInputModeFactory *factory, KTextEditor::EditorPrivate::self()->inputModeFactories()) {
        KateConfigPage *tab = factory->createConfigPage(this);
        if (tab) {
            m_inputModeConfigTabs << tab;
            tabWidget->insertTab(i, tab, tab->name());
            connect(tab, SIGNAL(changed()), this, SLOT(slotChanged()));
            i++;
        }
    }

    layout->addWidget(tabWidget);
    setLayout(layout);
}

KateEditConfigTab::~KateEditConfigTab()
{
    qDeleteAll(m_inputModeConfigTabs);
}

void KateEditConfigTab::apply()
{
    // try to update the rest of tabs
    editConfigTab->apply();
    navigationConfigTab->apply();
    indentConfigTab->apply();
    completionConfigTab->apply();
    spellCheckConfigTab->apply();
    Q_FOREACH(KateConfigPage *tab, m_inputModeConfigTabs) {
        tab->apply();
    }
}

void KateEditConfigTab::reload()
{
    editConfigTab->reload();
    navigationConfigTab->reload();
    indentConfigTab->reload();
    completionConfigTab->reload();
    spellCheckConfigTab->reload();
    Q_FOREACH(KateConfigPage *tab, m_inputModeConfigTabs) {
        tab->reload();
    }
}

void KateEditConfigTab::reset()
{
    editConfigTab->reset();
    navigationConfigTab->reset();
    indentConfigTab->reset();
    completionConfigTab->reset();
    spellCheckConfigTab->reset();
    Q_FOREACH(KateConfigPage *tab, m_inputModeConfigTabs) {
        tab->reset();
    }
}

void KateEditConfigTab::defaults()
{
    editConfigTab->defaults();
    navigationConfigTab->defaults();
    indentConfigTab->defaults();
    completionConfigTab->defaults();
    spellCheckConfigTab->defaults();
    Q_FOREACH(KateConfigPage *tab, m_inputModeConfigTabs) {
        tab->defaults();
    }
}

QString KateEditConfigTab::name() const
{
    return i18n("Editing");
}

QString KateEditConfigTab::fullName() const
{
    return i18n("Editing Options");
}

QIcon KateEditConfigTab::icon() const
{
    return QIcon::fromTheme(QStringLiteral("accessories-text-editor"));
}

//END KateEditConfigTab

//BEGIN KateViewDefaultsConfig
KateViewDefaultsConfig::KateViewDefaultsConfig(QWidget *parent)
    : KateConfigPage(parent)
    , textareaUi(new Ui::TextareaAppearanceConfigWidget())
    , bordersUi(new Ui::BordersAppearanceConfigWidget())
{
    QLayout *layout = new QVBoxLayout(this);
    QTabWidget *tabWidget = new QTabWidget(this);
    layout->addWidget(tabWidget);
    layout->setContentsMargins(0, 0, 0, 0);

    QWidget *textareaTab = new QWidget(tabWidget);
    textareaUi->setupUi(textareaTab);
    tabWidget->addTab(textareaTab, i18n("General"));

    QWidget *bordersTab = new QWidget(tabWidget);
    bordersUi->setupUi(bordersTab);
    tabWidget->addTab(bordersTab, i18n("Borders"));

    textareaUi->cmbDynamicWordWrapIndicator->addItem(i18n("Off"));
    textareaUi->cmbDynamicWordWrapIndicator->addItem(i18n("Follow Line Numbers"));
    textareaUi->cmbDynamicWordWrapIndicator->addItem(i18n("Always On"));

    // What's This? help is in the ui-file

    reload();

    //
    // after initial reload, connect the stuff for the changed () signal
    //

    connect(textareaUi->gbWordWrap, SIGNAL(toggled(bool)), this, SLOT(slotChanged()));
    connect(textareaUi->chkDynWrapAtStaticMarker, SIGNAL(toggled(bool)), this, SLOT(slotChanged()));
    connect(textareaUi->cmbDynamicWordWrapIndicator, SIGNAL(activated(int)), this, SLOT(slotChanged()));
    connect(textareaUi->sbDynamicWordWrapDepth, SIGNAL(valueChanged(int)), this, SLOT(slotChanged()));
    connect(textareaUi->chkShowTabs, SIGNAL(toggled(bool)), this, SLOT(slotChanged()));
    connect(textareaUi->spacesComboBox, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &KateViewDefaultsConfig::slotChanged);
    connect(textareaUi->chkShowIndentationLines, SIGNAL(toggled(bool)), this, SLOT(slotChanged()));
    connect(textareaUi->sliSetMarkerSize, SIGNAL(valueChanged(int)), this, SLOT(slotChanged()));
    connect(textareaUi->chkShowWholeBracketExpression, SIGNAL(toggled(bool)), this, SLOT(slotChanged()));
    connect(textareaUi->chkAnimateBracketMatching, SIGNAL(toggled(bool)), this, SLOT(slotChanged()));
    connect(textareaUi->chkFoldFirstLine,  SIGNAL(toggled(bool)), this, SLOT(slotChanged()));
    connect(textareaUi->chkShowWordCount,  SIGNAL(toggled(bool)), this, SLOT(slotChanged()));
    connect(textareaUi->chkShowLineCount, SIGNAL(toggled(bool)), this, SLOT(slotChanged()));

    connect(bordersUi->chkIconBorder, SIGNAL(toggled(bool)), this, SLOT(slotChanged()));
    connect(bordersUi->chkScrollbarMarks, SIGNAL(toggled(bool)), this, SLOT(slotChanged()));
    connect(bordersUi->chkScrollbarPreview, SIGNAL(toggled(bool)), this, SLOT(slotChanged()));
    connect(bordersUi->chkScrollbarMiniMap, SIGNAL(toggled(bool)), this, SLOT(slotChanged()));
    connect(bordersUi->chkScrollbarMiniMapAll, SIGNAL(toggled(bool)), this, SLOT(slotChanged()));
    bordersUi->chkScrollbarMiniMapAll->hide(); // this is temporary until the feature is done
    connect(bordersUi->spBoxMiniMapWidth, SIGNAL(valueChanged(int)), this, SLOT(slotChanged()));
    connect(bordersUi->chkLineNumbers, SIGNAL(toggled(bool)), this, SLOT(slotChanged()));
    connect(bordersUi->chkShowLineModification, SIGNAL(toggled(bool)), this, SLOT(slotChanged()));
    connect(bordersUi->chkShowFoldingMarkers, SIGNAL(toggled(bool)), this, SLOT(slotChanged()));
    connect(bordersUi->chkShowFoldingPreview, SIGNAL(toggled(bool)), this, SLOT(slotChanged()));
    connect(bordersUi->rbSortBookmarksByPosition, SIGNAL(toggled(bool)), this, SLOT(slotChanged()));
    connect(bordersUi->rbSortBookmarksByCreation, SIGNAL(toggled(bool)), this, SLOT(slotChanged()));
    connect(bordersUi->cmbShowScrollbars, SIGNAL(activated(int)), this, SLOT(slotChanged()));
}

KateViewDefaultsConfig::~KateViewDefaultsConfig()
{
    delete bordersUi;
    delete textareaUi;
}

void KateViewDefaultsConfig::apply()
{
    // nothing changed, no need to apply stuff
    if (!hasChanged()) {
        return;
    }
    m_changed = false;

    KateViewConfig::global()->configStart();
    KateRendererConfig::global()->configStart();

    KateViewConfig::global()->setDynWordWrap(textareaUi->gbWordWrap->isChecked());
    KateViewConfig::global()->setDynWrapAtStaticMarker(textareaUi->chkDynWrapAtStaticMarker->isChecked());
    KateViewConfig::global()->setDynWordWrapIndicators(textareaUi->cmbDynamicWordWrapIndicator->currentIndex());
    KateViewConfig::global()->setDynWordWrapAlignIndent(textareaUi->sbDynamicWordWrapDepth->value());
    KateDocumentConfig::global()->setShowTabs(textareaUi->chkShowTabs->isChecked());
    KateDocumentConfig::global()->setShowSpaces(KateDocumentConfig::WhitespaceRendering(textareaUi->spacesComboBox->currentIndex()));
    KateDocumentConfig::global()->setMarkerSize(textareaUi->sliSetMarkerSize->value());
    KateViewConfig::global()->setLineNumbers(bordersUi->chkLineNumbers->isChecked());
    KateViewConfig::global()->setIconBar(bordersUi->chkIconBorder->isChecked());
    KateViewConfig::global()->setScrollBarMarks(bordersUi->chkScrollbarMarks->isChecked());
    KateViewConfig::global()->setScrollBarPreview(bordersUi->chkScrollbarPreview->isChecked());
    KateViewConfig::global()->setScrollBarMiniMap(bordersUi->chkScrollbarMiniMap->isChecked());
    KateViewConfig::global()->setScrollBarMiniMapAll(bordersUi->chkScrollbarMiniMapAll->isChecked());
    KateViewConfig::global()->setScrollBarMiniMapWidth(bordersUi->spBoxMiniMapWidth->value());
    KateViewConfig::global()->setFoldingBar(bordersUi->chkShowFoldingMarkers->isChecked());
    KateViewConfig::global()->setFoldingPreview(bordersUi->chkShowFoldingPreview->isChecked());
    KateViewConfig::global()->setLineModification(bordersUi->chkShowLineModification->isChecked());
    KateViewConfig::global()->setShowScrollbars(bordersUi->cmbShowScrollbars->currentIndex());

    KateViewConfig::global()->setBookmarkSort(bordersUi->rbSortBookmarksByPosition->isChecked() ? 0 : 1);
    KateRendererConfig::global()->setShowIndentationLines(textareaUi->chkShowIndentationLines->isChecked());
    KateRendererConfig::global()->setShowWholeBracketExpression(textareaUi->chkShowWholeBracketExpression->isChecked());
    KateRendererConfig::global()->setAnimateBracketMatching(textareaUi->chkAnimateBracketMatching->isChecked());
    KateViewConfig::global()->setFoldFirstLine(textareaUi->chkFoldFirstLine->isChecked());
    KateViewConfig::global()->setShowWordCount(textareaUi->chkShowWordCount->isChecked());
    KateViewConfig::global()->setShowLineCount(textareaUi->chkShowLineCount->isChecked());

    KateRendererConfig::global()->configEnd();
    KateViewConfig::global()->configEnd();
}

void KateViewDefaultsConfig::reload()
{
    textareaUi->gbWordWrap->setChecked(KateViewConfig::global()->dynWordWrap());
    textareaUi->chkDynWrapAtStaticMarker->setChecked(KateViewConfig::global()->dynWrapAtStaticMarker());
    textareaUi->cmbDynamicWordWrapIndicator->setCurrentIndex(KateViewConfig::global()->dynWordWrapIndicators());
    textareaUi->sbDynamicWordWrapDepth->setValue(KateViewConfig::global()->dynWordWrapAlignIndent());
    textareaUi->chkShowTabs->setChecked(KateDocumentConfig::global()->showTabs());
    textareaUi->spacesComboBox->setCurrentIndex(KateDocumentConfig::global()->showSpaces());
    textareaUi->sliSetMarkerSize->setValue(KateDocumentConfig::global()->markerSize());
    bordersUi->chkLineNumbers->setChecked(KateViewConfig::global()->lineNumbers());
    bordersUi->chkIconBorder->setChecked(KateViewConfig::global()->iconBar());
    bordersUi->chkScrollbarMarks->setChecked(KateViewConfig::global()->scrollBarMarks());
    bordersUi->chkScrollbarPreview->setChecked(KateViewConfig::global()->scrollBarPreview());
    bordersUi->chkScrollbarMiniMap->setChecked(KateViewConfig::global()->scrollBarMiniMap());
    bordersUi->chkScrollbarMiniMapAll->setChecked(KateViewConfig::global()->scrollBarMiniMapAll());
    bordersUi->spBoxMiniMapWidth->setValue(KateViewConfig::global()->scrollBarMiniMapWidth());
    bordersUi->chkShowFoldingMarkers->setChecked(KateViewConfig::global()->foldingBar());
    bordersUi->chkShowFoldingPreview->setChecked(KateViewConfig::global()->foldingPreview());
    bordersUi->chkShowLineModification->setChecked(KateViewConfig::global()->lineModification());
    bordersUi->rbSortBookmarksByPosition->setChecked(KateViewConfig::global()->bookmarkSort() == 0);
    bordersUi->rbSortBookmarksByCreation->setChecked(KateViewConfig::global()->bookmarkSort() == 1);
    bordersUi->cmbShowScrollbars->setCurrentIndex(KateViewConfig::global()->showScrollbars());
    textareaUi->chkShowIndentationLines->setChecked(KateRendererConfig::global()->showIndentationLines());
    textareaUi->chkShowWholeBracketExpression->setChecked(KateRendererConfig::global()->showWholeBracketExpression());
    textareaUi->chkAnimateBracketMatching->setChecked(KateRendererConfig::global()->animateBracketMatching());
    textareaUi->chkFoldFirstLine->setChecked(KateViewConfig::global()->foldFirstLine());
    textareaUi->chkShowWordCount->setChecked(KateViewConfig::global()->showWordCount());
    textareaUi->chkShowLineCount->setChecked(KateViewConfig::global()->showLineCount());
}

void KateViewDefaultsConfig::reset()
{
    ;
}

void KateViewDefaultsConfig::defaults()
{
    ;
}

QString KateViewDefaultsConfig::name() const
{
    return i18n("Appearance");
}

QString KateViewDefaultsConfig::fullName() const
{
    return i18n("Appearance");
}

QIcon KateViewDefaultsConfig::icon() const
{
    return QIcon::fromTheme(QStringLiteral("preferences-desktop-theme"));
}

//END KateViewDefaultsConfig

//BEGIN KateSaveConfigTab
KateSaveConfigTab::KateSaveConfigTab(QWidget *parent)
    : KateConfigPage(parent)
    , modeConfigPage(new ModeConfigPage(this))
{
    // FIXME: Is really needed to move all this code below to another class,
    // since it is another tab itself on the config dialog. This means we should
    // initialize, add and work with as we do with modeConfigPage (ereslibre)
    QVBoxLayout *layout = new QVBoxLayout;
    layout->setContentsMargins(0, 0, 0, 0);
    QTabWidget *tabWidget = new QTabWidget(this);

    QWidget *tmpWidget = new QWidget(tabWidget);
    QVBoxLayout *internalLayout = new QVBoxLayout;
    QWidget *newWidget = new QWidget(tabWidget);
    ui = new Ui::OpenSaveConfigWidget();
    ui->setupUi(newWidget);

    QWidget *tmpWidget2 = new QWidget(tabWidget);
    QVBoxLayout *internalLayout2 = new QVBoxLayout;
    QWidget *newWidget2 = new QWidget(tabWidget);
    uiadv = new Ui::OpenSaveConfigAdvWidget();
    uiadv->setupUi(newWidget2);

    // What's this help is added in ui/opensaveconfigwidget.ui
    reload();

    //
    // after initial reload, connect the stuff for the changed () signal
    //

    connect(ui->cmbEncoding, SIGNAL(activated(int)), this, SLOT(slotChanged()));
    connect(ui->cmbEncodingDetection, SIGNAL(activated(int)), this, SLOT(slotChanged()));
    connect(ui->cmbEncodingFallback, SIGNAL(activated(int)), this, SLOT(slotChanged()));
    connect(ui->cmbEOL, SIGNAL(activated(int)), this, SLOT(slotChanged()));
    connect(ui->chkDetectEOL, SIGNAL(toggled(bool)), this, SLOT(slotChanged()));
    connect(ui->chkEnableBOM, SIGNAL(toggled(bool)), this, SLOT(slotChanged()));
    connect(ui->lineLengthLimit, SIGNAL(valueChanged(int)), this, SLOT(slotChanged()));
    connect(ui->cbRemoveTrailingSpaces, SIGNAL(currentIndexChanged(int)), this, SLOT(slotChanged()));
    connect(ui->chkNewLineAtEof, SIGNAL(toggled(bool)), this, SLOT(slotChanged()));
    connect(uiadv->chkBackupLocalFiles, SIGNAL(toggled(bool)), this, SLOT(slotChanged()));
    connect(uiadv->chkBackupRemoteFiles, SIGNAL(toggled(bool)), this, SLOT(slotChanged()));
    connect(uiadv->edtBackupPrefix, SIGNAL(textChanged(QString)), this, SLOT(slotChanged()));
    connect(uiadv->edtBackupSuffix, SIGNAL(textChanged(QString)), this, SLOT(slotChanged()));
    connect(uiadv->cmbSwapFileMode, SIGNAL(currentIndexChanged(int)), this, SLOT(slotChanged()));
    connect(uiadv->cmbSwapFileMode, SIGNAL(currentIndexChanged(int)), this, SLOT(swapFileModeChanged(int)));
    connect(uiadv->kurlSwapDirectory, SIGNAL(textChanged(QString)), this, SLOT(slotChanged()));
    connect(uiadv->spbSwapFileSync, SIGNAL(valueChanged(int)), this, SLOT(slotChanged()));

    internalLayout->addWidget(newWidget);
    tmpWidget->setLayout(internalLayout);
    internalLayout2->addWidget(newWidget2);
    tmpWidget2->setLayout(internalLayout2);

    // add all tabs
    tabWidget->insertTab(0, tmpWidget, i18n("General"));
    tabWidget->insertTab(1, tmpWidget2, i18n("Advanced"));
    tabWidget->insertTab(2, modeConfigPage, modeConfigPage->name());

    connect(modeConfigPage, SIGNAL(changed()), this, SLOT(slotChanged()));

    layout->addWidget(tabWidget);
    setLayout(layout);
}

KateSaveConfigTab::~KateSaveConfigTab()
{
    delete ui;
}

void KateSaveConfigTab::swapFileModeChanged(int idx)
{
    const KateDocumentConfig::SwapFileMode mode = static_cast<KateDocumentConfig::SwapFileMode>(idx);
    switch (mode) {
    case KateDocumentConfig::DisableSwapFile:
        uiadv->lblSwapDirectory->setEnabled(false);
        uiadv->kurlSwapDirectory->setEnabled(false);
        uiadv->lblSwapFileSync->setEnabled(false);
        uiadv->spbSwapFileSync->setEnabled(false);
        break;
    case KateDocumentConfig::EnableSwapFile:
        uiadv->lblSwapDirectory->setEnabled(false);
        uiadv->kurlSwapDirectory->setEnabled(false);
        uiadv->lblSwapFileSync->setEnabled(true);
        uiadv->spbSwapFileSync->setEnabled(true);
        break;
    case KateDocumentConfig::SwapFilePresetDirectory:
        uiadv->lblSwapDirectory->setEnabled(true);
        uiadv->kurlSwapDirectory->setEnabled(true);
        uiadv->lblSwapFileSync->setEnabled(true);
        uiadv->spbSwapFileSync->setEnabled(true);
        break;
    }
}

void KateSaveConfigTab::apply()
{
    modeConfigPage->apply();

    // nothing changed, no need to apply stuff
    if (!hasChanged()) {
        return;
    }
    m_changed = false;

    KateGlobalConfig::global()->configStart();
    KateDocumentConfig::global()->configStart();

    if (uiadv->edtBackupSuffix->text().isEmpty() && uiadv->edtBackupPrefix->text().isEmpty()) {
        KMessageBox::information(
            this,
            i18n("You did not provide a backup suffix or prefix. Using default suffix: '~'"),
            i18n("No Backup Suffix or Prefix")
        );
        uiadv->edtBackupSuffix->setText(QStringLiteral("~"));
    }

    KateDocumentConfig::global()->setBackupOnSaveLocal(uiadv->chkBackupLocalFiles->isChecked());
    KateDocumentConfig::global()->setBackupOnSaveRemote(uiadv->chkBackupRemoteFiles->isChecked());
    KateDocumentConfig::global()->setBackupPrefix(uiadv->edtBackupPrefix->text());
    KateDocumentConfig::global()->setBackupSuffix(uiadv->edtBackupSuffix->text());

    KateDocumentConfig::global()->setSwapFileMode(uiadv->cmbSwapFileMode->currentIndex());
    KateDocumentConfig::global()->setSwapDirectory(uiadv->kurlSwapDirectory->url().toLocalFile());
    KateDocumentConfig::global()->setSwapSyncInterval(uiadv->spbSwapFileSync->value());

    KateDocumentConfig::global()->setRemoveSpaces(ui->cbRemoveTrailingSpaces->currentIndex());

    KateDocumentConfig::global()->setNewLineAtEof(ui->chkNewLineAtEof->isChecked());

    // set both standard and fallback encoding
    KateDocumentConfig::global()->setEncoding(KCharsets::charsets()->encodingForName(ui->cmbEncoding->currentText()));

    KateGlobalConfig::global()->setProberType((KEncodingProber::ProberType)ui->cmbEncodingDetection->currentIndex());
    KateGlobalConfig::global()->setFallbackEncoding(KCharsets::charsets()->encodingForName(ui->cmbEncodingFallback->currentText()));

    KateDocumentConfig::global()->setEol(ui->cmbEOL->currentIndex());
    KateDocumentConfig::global()->setAllowEolDetection(ui->chkDetectEOL->isChecked());
    KateDocumentConfig::global()->setBom(ui->chkEnableBOM->isChecked());

    KateDocumentConfig::global()->setLineLengthLimit(ui->lineLengthLimit->value());

    KateDocumentConfig::global()->configEnd();
    KateGlobalConfig::global()->configEnd();
}

void KateSaveConfigTab::reload()
{
    modeConfigPage->reload();

    // encodings
    ui->cmbEncoding->clear();
    ui->cmbEncodingFallback->clear();
    QStringList encodings(KCharsets::charsets()->descriptiveEncodingNames());
    int insert = 0;
    for (int i = 0; i < encodings.count(); i++) {
        bool found = false;
        QTextCodec *codecForEnc = KCharsets::charsets()->codecForName(KCharsets::charsets()->encodingForName(encodings[i]), found);

        if (found) {
            ui->cmbEncoding->addItem(encodings[i]);
            ui->cmbEncodingFallback->addItem(encodings[i]);

            if (codecForEnc == KateDocumentConfig::global()->codec()) {
                ui->cmbEncoding->setCurrentIndex(insert);
            }

            if (codecForEnc == KateGlobalConfig::global()->fallbackCodec()) {
                // adjust index for fallback config, has no default!
                ui->cmbEncodingFallback->setCurrentIndex(insert);
            }

            insert++;
        }
    }

    // encoding detection
    ui->cmbEncodingDetection->clear();
    bool found = false;
    for (int i = 0; !KEncodingProber::nameForProberType((KEncodingProber::ProberType) i).isEmpty(); ++i) {
        ui->cmbEncodingDetection->addItem(KEncodingProber::nameForProberType((KEncodingProber::ProberType) i));
        if (i == KateGlobalConfig::global()->proberType()) {
            ui->cmbEncodingDetection->setCurrentIndex(ui->cmbEncodingDetection->count() - 1);
            found = true;
        }
    }
    if (!found) {
        ui->cmbEncodingDetection->setCurrentIndex(KEncodingProber::Universal);
    }

    // eol
    ui->cmbEOL->setCurrentIndex(KateDocumentConfig::global()->eol());
    ui->chkDetectEOL->setChecked(KateDocumentConfig::global()->allowEolDetection());
    ui->chkEnableBOM->setChecked(KateDocumentConfig::global()->bom());
    ui->lineLengthLimit->setValue(KateDocumentConfig::global()->lineLengthLimit());

    ui->cbRemoveTrailingSpaces->setCurrentIndex(KateDocumentConfig::global()->removeSpaces());
    ui->chkNewLineAtEof->setChecked(KateDocumentConfig::global()->newLineAtEof());

    // other stuff
    uiadv->chkBackupLocalFiles->setChecked(KateDocumentConfig::global()->backupOnSaveLocal());
    uiadv->chkBackupRemoteFiles->setChecked(KateDocumentConfig::global()->backupOnSaveRemote());
    uiadv->edtBackupPrefix->setText(KateDocumentConfig::global()->backupPrefix());
    uiadv->edtBackupSuffix->setText(KateDocumentConfig::global()->backupSuffix());
    uiadv->cmbSwapFileMode->setCurrentIndex(KateDocumentConfig::global()->swapFileMode());
    uiadv->kurlSwapDirectory->setUrl(QUrl::fromLocalFile(KateDocumentConfig::global()->swapDirectory()));
    uiadv->spbSwapFileSync->setValue(KateDocumentConfig::global()->swapSyncInterval());
    swapFileModeChanged(KateDocumentConfig::global()->swapFileMode());
}

void KateSaveConfigTab::reset()
{
    modeConfigPage->reset();
}

void KateSaveConfigTab::defaults()
{
    modeConfigPage->defaults();

    ui->cbRemoveTrailingSpaces->setCurrentIndex(0);

    uiadv->chkBackupLocalFiles->setChecked(true);
    uiadv->chkBackupRemoteFiles->setChecked(false);
    uiadv->edtBackupPrefix->setText(QString());
    uiadv->edtBackupSuffix->setText(QStringLiteral("~"));
    uiadv->cmbSwapFileMode->setCurrentIndex(1);
    uiadv->kurlSwapDirectory->setDisabled(true);
    uiadv->lblSwapDirectory->setDisabled(true);
    uiadv->spbSwapFileSync->setValue(15);
}

QString KateSaveConfigTab::name() const
{
    return i18n("Open/Save");
}

QString KateSaveConfigTab::fullName() const
{
    return i18n("File Opening & Saving");
}

QIcon KateSaveConfigTab::icon() const
{
    return QIcon::fromTheme(QStringLiteral("document-save"));
}

//END KateSaveConfigTab

//BEGIN KateGotoBar
KateGotoBar::KateGotoBar(KTextEditor::View *view, QWidget *parent)
    : KateViewBarWidget(true, parent)
    , m_view(view)
{
    Q_ASSERT(m_view != nullptr);    // this bar widget is pointless w/o a view

    QHBoxLayout *topLayout = new QHBoxLayout(centralWidget());
    topLayout->setContentsMargins(0, 0, 0, 0);

    QToolButton *btn = new QToolButton(this);
    btn->setAutoRaise(true);
    btn->setMinimumSize(QSize(1, btn->minimumSizeHint().height()));
    btn->setText(i18n("&Line:"));
    btn->setToolTip(i18n("Go to line number from clipboard"));
    connect(btn, &QToolButton::clicked, this, &KateGotoBar::gotoClipboard);
    topLayout->addWidget(btn);

    m_gotoRange = new QSpinBox(this);
    m_gotoRange->setMinimum(1);
    topLayout->addWidget(m_gotoRange, 1);
    topLayout->setStretchFactor(m_gotoRange, 0);

    btn = new QToolButton(this);
    btn->setAutoRaise(true);
    btn->setMinimumSize(QSize(1, btn->minimumSizeHint().height()));
    btn->setText(i18n("Go to"));
    btn->setIcon(QIcon::fromTheme(QStringLiteral("go-jump")));
    btn->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    connect(btn, &QToolButton::clicked, this, &KateGotoBar::gotoLine);
    topLayout->addWidget(btn);

    btn = m_modifiedUp = new QToolButton(this);
    btn->setAutoRaise(true);
    btn->setMinimumSize(QSize(1, btn->minimumSizeHint().height()));
    btn->setDefaultAction(m_view->action("modified_line_up"));
    btn->setIcon(QIcon::fromTheme(QStringLiteral("go-up-search")));
    btn->setText(QString());
    btn->installEventFilter(this);
    topLayout->addWidget(btn);

    btn = m_modifiedDown = new QToolButton(this);
    btn->setAutoRaise(true);
    btn->setMinimumSize(QSize(1, btn->minimumSizeHint().height()));
    btn->setDefaultAction(m_view->action("modified_line_down"));
    btn->setIcon(QIcon::fromTheme(QStringLiteral("go-down-search")));
    btn->setText(QString());
    btn->installEventFilter(this);
    topLayout->addWidget(btn);

    topLayout->addStretch();

    setFocusProxy(m_gotoRange);
}

void KateGotoBar::showEvent(QShowEvent *event)
{
    Q_UNUSED(event)
    // Catch rare cases where the bar is visible while document is edited
    connect(m_view->document(), &KTextEditor::Document::textChanged, this, &KateGotoBar::updateData);
}

void KateGotoBar::closed()
{
    disconnect(m_view->document(), &KTextEditor::Document::textChanged, this, &KateGotoBar::updateData);
}

bool KateGotoBar::eventFilter(QObject *object, QEvent *event)
{
    if (object == m_modifiedUp || object == m_modifiedDown) {
        if (event->type() != QEvent::Wheel) {
            return false;
        }

        int delta = static_cast<QWheelEvent *>(event)->delta();
        // Reset m_wheelDelta when scroll direction change
        if (m_wheelDelta != 0 && (m_wheelDelta < 0) != (delta < 0)) {
            m_wheelDelta = 0;
        }

        m_wheelDelta += delta;

        if (m_wheelDelta >= 120) {
            m_wheelDelta = 0;
            m_modifiedUp->click();
        } else if (m_wheelDelta <= -120) {
            m_wheelDelta = 0;
            m_modifiedDown->click();
        }
    }

    return false;
}

void KateGotoBar::gotoClipboard()
{
    QRegularExpression rx(QStringLiteral("\\d+"));
    int lineNo = rx.match(QApplication::clipboard()->text(QClipboard::Selection)).captured().toInt();
    if (lineNo <= m_gotoRange->maximum() && lineNo >= 1) {
        m_gotoRange->setValue(lineNo);
        gotoLine();
    } else {
        QPointer<KTextEditor::Message> message = new KTextEditor::Message(
            i18n("No valid line number found in clipboard"));
        message->setWordWrap(true);
        message->setAutoHide(2000);
        message->setPosition(KTextEditor::Message::BottomInView);
        message->setView(m_view),
        m_view->document()->postMessage(message);
    }
}

void KateGotoBar::updateData()
{
    m_gotoRange->setMaximum(m_view->document()->lines());
    if (!isVisible()) {
        m_gotoRange->setValue(m_view->cursorPosition().line() + 1);
        m_gotoRange->adjustSize(); // ### does not respect the range :-(
    }

    m_gotoRange->selectAll();
}

void KateGotoBar::keyPressEvent(QKeyEvent *event)
{
    int key = event->key();
    if (key == Qt::Key_Return || key == Qt::Key_Enter) {
        gotoLine();
        return;
    }
    KateViewBarWidget::keyPressEvent(event);
}

void KateGotoBar::gotoLine()
{
    KTextEditor::ViewPrivate *kv = qobject_cast<KTextEditor::ViewPrivate *>(m_view);
    if (kv && kv->selection() && !kv->config()->persistentSelection()) {
        kv->clearSelection();
    }

    m_view->setCursorPosition(KTextEditor::Cursor(m_gotoRange->value() - 1, 0));
    m_view->setFocus();
    emit hideMe();
}
//END KateGotoBar

//BEGIN KateDictionaryBar
KateDictionaryBar::KateDictionaryBar(KTextEditor::ViewPrivate *view, QWidget *parent)
    : KateViewBarWidget(true, parent)
    , m_view(view)
{
    Q_ASSERT(m_view != nullptr); // this bar widget is pointless w/o a view

    QHBoxLayout *topLayout = new QHBoxLayout(centralWidget());
    topLayout->setContentsMargins(0, 0, 0, 0);
    //topLayout->setSpacing(spacingHint());
    m_dictionaryComboBox = new Sonnet::DictionaryComboBox(centralWidget());
    connect(m_dictionaryComboBox, SIGNAL(dictionaryChanged(QString)),
            this, SLOT(dictionaryChanged(QString)));
    connect(view->doc(), SIGNAL(defaultDictionaryChanged(KTextEditor::DocumentPrivate*)),
            this, SLOT(updateData()));
    QLabel *label = new QLabel(i18n("Dictionary:"), centralWidget());
    label->setBuddy(m_dictionaryComboBox);

    topLayout->addWidget(label);
    topLayout->addWidget(m_dictionaryComboBox, 1);
    topLayout->setStretchFactor(m_dictionaryComboBox, 0);
    topLayout->addStretch();
}

KateDictionaryBar::~KateDictionaryBar()
{
}

void KateDictionaryBar::updateData()
{
    KTextEditor::DocumentPrivate *document = m_view->doc();
    QString dictionary = document->defaultDictionary();
    if (dictionary.isEmpty()) {
        dictionary = Sonnet::Speller().defaultLanguage();
    }
    m_dictionaryComboBox->setCurrentByDictionary(dictionary);
}

void KateDictionaryBar::dictionaryChanged(const QString &dictionary)
{
    const KTextEditor::Range selection = m_view->selectionRange();
    if (selection.isValid() && !selection.isEmpty()) {
        const bool blockmode = m_view->blockSelection();
        m_view->doc()->setDictionary(dictionary, selection, blockmode);
    } else {
        m_view->doc()->setDefaultDictionary(dictionary);
    }
}

//END KateGotoBar

//BEGIN KateModOnHdPrompt
KateModOnHdPrompt::KateModOnHdPrompt(KTextEditor::DocumentPrivate *doc,
                                     KTextEditor::ModificationInterface::ModifiedOnDiskReason modtype,
                                     const QString &reason)
    : QObject(doc)
    , m_doc(doc)
    , m_modtype(modtype)
    , m_proc(nullptr)
    , m_diffFile(nullptr)
    , m_diffAction(nullptr)
{
    m_message = new KTextEditor::Message(reason, KTextEditor::Message::Information);
    m_message->setPosition(KTextEditor::Message::AboveView);
    m_message->setWordWrap(true);

    // If the file isn't deleted, present a diff button
    const bool onDiskDeleted = modtype == KTextEditor::ModificationInterface::OnDiskDeleted;
    if (!onDiskDeleted) {
        QAction * aAutoReload = new QAction(i18n("Enable Auto Reload"), this);
        aAutoReload->setIcon(QIcon::fromTheme(QStringLiteral("view-refresh")));
        aAutoReload->setToolTip(i18n("Will never again warn about on disk changes but always reload."));
        m_message->addAction(aAutoReload, false);
        connect(aAutoReload, &QAction::triggered, this, &KateModOnHdPrompt::autoReloadTriggered);

        if (!QStandardPaths::findExecutable(QStringLiteral("diff")).isEmpty()) {
            m_diffAction = new QAction(i18n("View &Difference"), this);
            m_diffAction->setToolTip(i18n("Shows a diff of the changes"));
            m_message->addAction(m_diffAction, false);
            connect(m_diffAction, SIGNAL(triggered()), this, SLOT(slotDiff()));
        }

        QAction * aReload = new QAction(i18n("&Reload"), this);
        aReload->setIcon(QIcon::fromTheme(QStringLiteral("view-refresh")));
        aReload->setToolTip(i18n("Reload the file from disk. Unsaved changes will be lost."));
        m_message->addAction(aReload);
        connect(aReload, SIGNAL(triggered()), this, SIGNAL(reloadTriggered()));
    } else {
        QAction * closeFile = new QAction(i18nc("@action:button closes the opened file", "&Close File"), this);
        closeFile->setIcon(QIcon::fromTheme(QStringLiteral("document-close")));
        closeFile->setToolTip(i18n("Close the file, discarding its content."));
        m_message->addAction(closeFile, false);
        connect(closeFile, &QAction::triggered, this, &KateModOnHdPrompt::closeTriggered);

        QAction * aSaveAs = new QAction(i18n("&Save As..."), this);
        aSaveAs->setIcon(QIcon::fromTheme(QStringLiteral("document-save-as")));
        aSaveAs->setToolTip(i18n("Lets you select a location and save the file again."));
        m_message->addAction(aSaveAs, false);
        connect(aSaveAs, SIGNAL(triggered()), this, SIGNAL(saveAsTriggered()));
    }

    QAction * aIgnore = new QAction(i18n("&Ignore"), this);
    aIgnore->setToolTip(i18n("Ignores the changes on disk without any action."));
    aIgnore->setIcon(KStandardGuiItem::overwrite().icon());
    m_message->addAction(aIgnore);
    connect(aIgnore, SIGNAL(triggered()), this, SIGNAL(ignoreTriggered()));

    m_doc->postMessage(m_message);
}

KateModOnHdPrompt::~KateModOnHdPrompt()
{
    delete m_proc;
    m_proc = nullptr;
    if (m_diffFile) {
        m_diffFile->setAutoRemove(true);
        delete m_diffFile;
        m_diffFile = nullptr;
    }
    delete m_message;
}

void KateModOnHdPrompt::slotDiff()
{
    if (m_diffFile) {
        return;
    }

    m_diffFile = new QTemporaryFile();
    m_diffFile->open();

    // Start a KProcess that creates a diff
    m_proc = new KProcess(this);
    m_proc->setOutputChannelMode(KProcess::MergedChannels);
    *m_proc << QStringLiteral("diff") << QLatin1String("-u")
            << QStringLiteral("-") <<  m_doc->url().toLocalFile();
    connect(m_proc, SIGNAL(readyRead()), this, SLOT(slotDataAvailable()));
    connect(m_proc, SIGNAL(finished(int,QProcess::ExitStatus)), this, SLOT(slotPDone()));

    // disable the diff button, to hinder the user to run it twice.
    m_diffAction->setEnabled(false);

    m_proc->start();

    QTextStream ts(m_proc);
    int lastln = m_doc->lines() - 1;
    for (int l = 0; l < lastln; ++l) {
        ts << m_doc->line(l) << '\n';
    }
    ts << m_doc->line(lastln);
    ts.flush();
    m_proc->closeWriteChannel();
}

void KateModOnHdPrompt::slotDataAvailable()
{
    m_diffFile->write(m_proc->readAll());
}

void KateModOnHdPrompt::slotPDone()
{
    m_diffAction->setEnabled(true);

    const QProcess::ExitStatus es = m_proc->exitStatus();
    delete m_proc;
    m_proc = nullptr;

    if (es != QProcess::NormalExit) {
        KMessageBox::sorry(nullptr,
                           i18n("The diff command failed. Please make sure that "
                                "diff(1) is installed and in your PATH."),
                           i18n("Error Creating Diff"));
        delete m_diffFile;
        m_diffFile = nullptr;
        return;
    }

    if (m_diffFile->size() == 0) {
        KMessageBox::information(nullptr,
                                 i18n("The files are identical."),
                                 i18n("Diff Output"));
        delete m_diffFile;
        m_diffFile = nullptr;
        return;
    }

    m_diffFile->setAutoRemove(false);
    QUrl url = QUrl::fromLocalFile(m_diffFile->fileName());
    delete m_diffFile;
    m_diffFile = nullptr;

    // KRun::runUrl should delete the file, once the client exits
    KRun::runUrl(url, QStringLiteral("text/x-patch"), nullptr, KRun::RunFlags(KRun::DeleteTemporaryFiles));
}

//END KateModOnHdPrompt

