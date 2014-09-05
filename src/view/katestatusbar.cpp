/* This file is part of the KDE and the Kate project
 *
 *   Copyright (C) 2013 Dominik Haumann <dhaumann@kde.org>
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

#include "katestatusbar.h"

#include "katemodemenu.h"
#include "kateglobal.h"
#include "katemodemanager.h"
#include "katedocument.h"
#include "kateconfig.h"
#include "kateabstractinputmode.h"

#include <KLocalizedString>
#include <KIconLoader>
#include <KAcceleratorManager>

#include <QHBoxLayout>
#include <QInputDialog>

//BEGIN menu
KateStatusBarOpenUpMenu::KateStatusBarOpenUpMenu(QWidget *parent) : QMenu(parent) {}
KateStatusBarOpenUpMenu::~KateStatusBarOpenUpMenu(){}

void KateStatusBarOpenUpMenu::setVisible(bool visibility) {
    if (visibility) {
        QRect geo=geometry();
        QPoint pos=((QPushButton*)parent())->mapToGlobal(QPoint(0,0));
        geo.moveTopLeft(QPoint(pos.x(),pos.y()-geo.height()));
        if (geo.top()<0) geo.moveTop(0);
        setGeometry(geo);
    }
    
    QMenu::setVisible(visibility);
}
//END menu

KateStatusBar::KateStatusBar(KTextEditor::ViewPrivate *view)
    : KateViewBarWidget(false)
    , m_view(view)
    , m_modifiedStatus (-1)
    , m_selectionMode (-1)
{
    KAcceleratorManager::setNoAccel(this);
    setFocusProxy(m_view);

    /**
     * just add our status bar to central widget, full sized
     */
    QHBoxLayout *topLayout = new QHBoxLayout(centralWidget());
    topLayout->setMargin(0);
    topLayout->setSpacing(4);

    /**
     * add a bit space
     */
    topLayout->addSpacing (4);
    
    /**
     * show Line XXX, Column XXX
     */
    m_lineColLabel = new QLabel( this );
    m_lineColLabel->installEventFilter(this); // register for doubleclick
    topLayout->addWidget( m_lineColLabel, 0 );
    m_lineColLabel->setAlignment( Qt::AlignVCenter | Qt::AlignLeft );
    m_lineColLabel->setFocusProxy(m_view);
    m_lineColLabel->setWhatsThis(i18n("Current cursor position. Doubleclick to go to specific line."));

    /**
     * show the current mode, like INSERT, OVERWRITE, VI + modifiers like [BLOCK]
     */
    m_insertModeLabel = new QLabel( this );
    m_insertModeLabel->installEventFilter(this); // register for doubleclick
    topLayout->addWidget( m_insertModeLabel, 1000 /* this one should strech */ );
    m_insertModeLabel->setAlignment( Qt::AlignVCenter | Qt::AlignRight );
    m_insertModeLabel->setFocusProxy(m_view);
    m_insertModeLabel->setWhatsThis(i18n("Insert mode and VI input mode indicator"));

    /**
     * allow to change indentation configuration
     */
    m_spacesOnly = ki18n("Soft Tabs: %1");
    m_spacesOnlyShowTabs = ki18n("Soft Tabs: %1 (%2)");
    m_tabsOnly = ki18n("Tab Size: %1");
    m_tabSpacesMixed = ki18n("Indent/Tab: %1/%2");

    QAction *action;
    m_tabGroup = new QActionGroup(this);
    m_indentGroup = new QActionGroup(this);

    m_tabsIndent = new QPushButton(QString(), this);
    m_tabsIndent->setFlat(true);
    topLayout->addWidget(m_tabsIndent, 0);
    m_tabsIndent->setFocusProxy(m_view);

    m_indentSettingsMenu = new KateStatusBarOpenUpMenu(m_tabsIndent);
    m_indentSettingsMenu->addSection(i18n("Tab Width"));
    addNumberAction(m_tabGroup, m_indentSettingsMenu, -1);
    addNumberAction(m_tabGroup, m_indentSettingsMenu, 8);
    addNumberAction(m_tabGroup, m_indentSettingsMenu, 4);
    addNumberAction(m_tabGroup, m_indentSettingsMenu, 2);
    m_indentSettingsMenu->addSection(i18n("Indentation Width"));
    addNumberAction(m_indentGroup, m_indentSettingsMenu, -1);
    addNumberAction(m_indentGroup, m_indentSettingsMenu, 8);
    addNumberAction(m_indentGroup, m_indentSettingsMenu, 4);
    addNumberAction(m_indentGroup, m_indentSettingsMenu, 2);

    m_indentSettingsMenu->addSection(i18n("Indentation Mode"));
    QActionGroup *radioGroup = new QActionGroup(m_indentSettingsMenu);
    action = m_indentSettingsMenu->addAction(i18n("Tabulators && Spaces"));
    action->setCheckable(true);
    action->setActionGroup(radioGroup);
    m_mixedAction = action;
    action = m_indentSettingsMenu->addAction(i18n("Tabulators"));
    action->setCheckable(true);
    action->setActionGroup(radioGroup);
    m_hardAction = action;
    action = m_indentSettingsMenu->addAction(i18n("Spaces"));
    action->setCheckable(true);
    action->setActionGroup(radioGroup);
    m_softAction = action;

    m_tabsIndent->setMenu(m_indentSettingsMenu);

    /**
     * add encoding button which allows user to switch encoding of document
     * this will reuse the encoding action menu of the view
     */
    m_encoding = new QPushButton( QString(), this );
    m_encoding->setFlat(true);
    topLayout->addWidget( m_encoding, 0 );
    m_encoding->setMenu(m_view->encodingAction()->menu());
    m_encoding->setFocusProxy(m_view);
    m_encoding->setWhatsThis(i18n("Encoding"));

    /**
     * add mode button which allows user to switch mode of document
     * this will reuse the mode action menu of the view
     */
    m_mode = new QPushButton( QString(), this );
    m_mode->setFlat(true);
    topLayout->addWidget( m_mode, 0 );
    m_mode->setMenu(m_view->modeAction()->menu());
    m_mode->setFocusProxy(m_view);
    m_mode->setWhatsThis(i18n("Syntax highlighting"));

    /**
     * show modification state of the document
     */
    m_modifiedLabel = new QToolButton( this );
    m_modifiedLabel->setAutoRaise(true);
    m_modifiedLabel->setEnabled(false);
    topLayout->addWidget( m_modifiedLabel, 0 );
    m_modifiedLabel->setFocusProxy(m_view);

    /**
     * add a bit space
     */
    topLayout->addSpacing (4);
    
    // signals for the statusbar
    connect(m_view, SIGNAL(cursorPositionChanged(KTextEditor::View*,KTextEditor::Cursor)), this, SLOT(cursorPositionChanged()));
    connect(m_view, SIGNAL(viewModeChanged(KTextEditor::View*,KTextEditor::View::ViewMode)), this, SLOT(viewModeChanged()));
    connect(m_view, SIGNAL(selectionChanged(KTextEditor::View*)), this, SLOT(selectionChanged()));
    connect(m_view->document(), SIGNAL(modifiedChanged(KTextEditor::Document*)), this, SLOT(modifiedChanged()));
    connect(m_view->document(), SIGNAL(modifiedOnDisk(KTextEditor::Document*,bool,KTextEditor::ModificationInterface::ModifiedOnDiskReason)), this, SLOT(modifiedChanged()) );
    connect(m_view->document(), SIGNAL(configChanged()), this, SLOT(documentConfigChanged()));
    connect(m_view->document(), SIGNAL(modeChanged(KTextEditor::Document*)), this, SLOT(modeChanged()));

    connect(m_tabGroup,SIGNAL(triggered(QAction*)),this,SLOT(slotTabGroup(QAction*)));
    connect(m_indentGroup,SIGNAL(triggered(QAction*)),this,SLOT(slotIndentGroup(QAction*)));
    connect(radioGroup,SIGNAL(triggered(QAction*)),this,SLOT(slotIndentTabMode(QAction*)));
    updateStatus ();
}

bool KateStatusBar::eventFilter(QObject *obj, QEvent *event)
{
    if (obj == m_insertModeLabel) {
        if (event->type() == QEvent::MouseButtonDblClick) {
            m_view->currentInputMode()->toggleInsert();
            return true;
        }
    } else if (obj == m_lineColLabel) {
        if (event->type() == QEvent::MouseButtonDblClick) {
            m_view->gotoLine();
            return true;
        }
    }

    return KateViewBarWidget::eventFilter(obj, event);
}

void KateStatusBar::updateStatus ()
{
    selectionChanged ();
    viewModeChanged ();
    cursorPositionChanged ();
    modifiedChanged ();
    documentConfigChanged();
    modeChanged();
}

void KateStatusBar::selectionChanged ()
{
    const unsigned int newSelectionMode = m_view->blockSelection();
    if (newSelectionMode == m_selectionMode) {
        return;
    }
    
    // remember new mode and update info
    m_selectionMode = newSelectionMode;
    viewModeChanged();
}

void KateStatusBar::viewModeChanged ()
{
    // prepend BLOCK for block selection mode
    QString text = m_view->viewModeHuman();
    if (m_view->blockSelection())
        text = i18n ("<em>[BLOCK]</em> %1", text);
    
    m_insertModeLabel->setText(text);
}

void KateStatusBar::cursorPositionChanged ()
{
    KTextEditor::Cursor position (m_view->cursorPositionVirtual());

    m_lineColLabel->setText(
        i18n("Line %1, Column %2"
            , QLocale().toString(position.line() + 1)
            , QLocale().toString(position.column() + 1)
            )
        );
}

void KateStatusBar::modifiedChanged()
{
    const bool mod = m_view->doc()->isModified();
    const bool modOnHD = m_view->doc()->isModifiedOnDisc();

    /**
     * combine to modified status, update only if changed
     */
    unsigned int newStatus = (unsigned int)mod | ((unsigned int)modOnHD << 1);
    if (m_modifiedStatus == newStatus)
        return;

    m_modifiedStatus = newStatus;
    switch (m_modifiedStatus) {
        case 0x1:
            m_modifiedLabel->setIcon (SmallIcon(QStringLiteral("document-save")));
            m_modifiedLabel->setWhatsThis(i18n("Meaning of current icon: Document was modified since it was loaded"));
            break;

        case 0x2:
            m_modifiedLabel->setIcon (SmallIcon(QStringLiteral("dialog-warning")));
            m_modifiedLabel->setWhatsThis(i18n("Meaning of current icon: Document was modified or deleted by another program"));
            break;

        case 0x3:
            m_modifiedLabel->setIcon (SmallIcon(QStringLiteral("document-save"), 0, KIconLoader::DefaultState,
                                               QStringList() << QStringLiteral("emblem-important")));
            m_modifiedLabel->setWhatsThis(QString());
            break;

        default:
            m_modifiedLabel->setIcon (SmallIcon(QStringLiteral("text-plain")));
            m_modifiedLabel->setWhatsThis(i18n("Meaning of current icon: Document was not modified since it was loaded"));
            break;
    }
}
        
void KateStatusBar::documentConfigChanged ()
{
    m_encoding->setText( m_view->document()->encoding() );
    KateDocumentConfig *config=((KTextEditor::DocumentPrivate*)m_view->document())->config();
    int tabWidth=config->tabWidth();
    int indentationWidth=config->indentationWidth();
    bool replaceTabsDyn=config->replaceTabsDyn();
    
    if (!replaceTabsDyn) {
        if (tabWidth==indentationWidth) {
            m_tabsIndent->setText(m_tabsOnly.subs(tabWidth).toString());
            m_tabGroup->setEnabled(false);
            m_hardAction->setChecked(true);
        } else {
            m_tabsIndent->setText(m_tabSpacesMixed.subs(indentationWidth).subs(tabWidth).toString());
            m_tabGroup->setEnabled(true);
            m_mixedAction->setChecked(true);
        }
    } else {
         if (tabWidth==indentationWidth) {
             m_tabsIndent->setText(m_spacesOnly.subs(indentationWidth).toString());
             m_tabGroup->setEnabled(true);
             m_softAction->setChecked(true);
         } else {
              m_tabsIndent->setText(m_spacesOnlyShowTabs.subs(indentationWidth).subs(tabWidth).toString());
              m_tabGroup->setEnabled(true);
              m_softAction->setChecked(true);
        }
    }

    updateGroup(m_tabGroup,tabWidth);
    updateGroup(m_indentGroup,indentationWidth);
}

void KateStatusBar::modeChanged ()
{
    m_mode->setText( KTextEditor::EditorPrivate::self()->modeManager()->fileType(m_view->document()->mode()).nameTranslated() );
}

void KateStatusBar::addNumberAction(QActionGroup *group, QMenu *menu, int data)
{
    QAction *a;
    if (data != -1) {
        a = menu->addAction(QStringLiteral("%1").arg(data));
    } else {
        a = menu->addAction(i18n("Other..."));
    }
    a->setData(data);
    a->setCheckable(true);
    a->setActionGroup(group);
}

void KateStatusBar::updateGroup(QActionGroup *group, int w) {
    QAction *m1=0;
    bool found=false;
    //linear search should be fast enough here, no additional hash
    Q_FOREACH(QAction *action, group->actions()) {
        int val=action->data().toInt();
        if (val==-1) m1=action;
        if (val==w) {
            found=true;
            action->setChecked(true);
        }
    }
    if (found) {
        m1->setText(i18n("Other..."));
    } else {
        m1->setText(i18np("Other (%1)", "Other (%1)", w));
        m1->setChecked(true);
    }
}

void KateStatusBar::slotTabGroup(QAction* a) {
    int val=a->data().toInt();
    bool ok;
    KateDocumentConfig *config=((KTextEditor::DocumentPrivate*)m_view->document())->config();
    if (val==-1) {
        val=QInputDialog::getInt(this, i18n("Tab Width"), i18n("Please specify the wanted tab width:"), config->tabWidth(), 1, 16, 1, &ok);
        if (!ok) val=config->tabWidth();
    }
    config->setTabWidth(val);
}

void KateStatusBar::slotIndentGroup(QAction* a) {
    int val=a->data().toInt();
    bool ok;
    KateDocumentConfig *config=((KTextEditor::DocumentPrivate*)m_view->document())->config();
    if (val==-1) {
        val=QInputDialog::getInt(this, i18n("Indentation Width"), i18n("Please specify the wanted indentation width:"), config->indentationWidth(), 1, 16, 1, &ok);
        if (!ok) val=config->indentationWidth();
    }
    config->configStart();
    config->setIndentationWidth(val);
    if (m_hardAction->isChecked()) config->setTabWidth(val);
    config->configEnd();
}

void KateStatusBar::slotIndentTabMode(QAction* a) {
    KateDocumentConfig *config=((KTextEditor::DocumentPrivate*)m_view->document())->config();
    if (a==m_softAction) {
        config->setReplaceTabsDyn(true);   
    } else if (a==m_mixedAction) {
        if (config->replaceTabsDyn())
            config->setReplaceTabsDyn(false);
        m_tabGroup->setEnabled(true);
    } else if (a==m_hardAction) {
        if (config->replaceTabsDyn()) {
            config->configStart();
            config->setReplaceTabsDyn(false);
            config->setTabWidth(config->indentationWidth());
            config->configEnd();
        } else {
            config->setTabWidth(config->indentationWidth());
        }
        m_tabGroup->setEnabled(false);
    }
}
