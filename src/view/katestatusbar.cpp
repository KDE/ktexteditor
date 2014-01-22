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

#include <KIconUtils>
#include <KLocalizedString>

#include <QHBoxLayout>
#include <QFontDatabase>
#include <QSizeGrip>

KateStatusBar::KateStatusBar(KTextEditor::ViewPrivate *view)
    : KateViewBarWidget(false)
    , m_view(view)
{
    setFocusProxy(m_view);

    /**
     * just add our status bar to central widget, full sized
     */
    QHBoxLayout *topLayout = new QHBoxLayout(centralWidget());
    topLayout->setMargin(0);
    topLayout->setSpacing(4);
    topLayout->addSpacing(4);

    m_modifiedLabel = new QLabel( this );
    m_modifiedLabel->setFixedSize( 16, 16 );
    topLayout->addWidget( m_modifiedLabel, 0 );
    m_modifiedLabel->setAlignment( Qt::AlignCenter );
    m_modifiedLabel->setFocusProxy(m_view);

    /**
     * add mode button which allows user to switch mode of document
     * this will reuse the mode action menu of the view
     */
    m_mode = new QPushButton( QString(), this );
    m_mode->setFlat(true);
    m_mode->setFont(QFontDatabase::systemFont(QFontDatabase::SmallestReadableFont));
    topLayout->addWidget( m_mode, 0 );
    m_mode->setMenu(m_view->modeAction()->menu());
    m_mode->setFocusProxy(m_view);

    /**
     * add encoding button which allows user to switch encoding of document
     * this will reuse the encoding action menu of the view
     */
    m_encoding = new QPushButton( QString(), this );
    m_encoding->setFlat(true);
    m_encoding->setFont(QFontDatabase::systemFont(QFontDatabase::SmallestReadableFont));
    topLayout->addWidget( m_encoding, 0 );
    m_encoding->setMenu(m_view->encodingAction()->menu());
    m_encoding->setFocusProxy(m_view);

    m_selectModeLabel = new QLabel( i18n(" LINE "), this );
    topLayout->addWidget( m_selectModeLabel, 0 );
    m_selectModeLabel->setAlignment( Qt::AlignCenter );
    m_selectModeLabel->setFocusProxy(m_view);

    m_insertModeLabel = new QLabel( i18n(" INS "), this );
    topLayout->addWidget( m_insertModeLabel, 0 );
    m_insertModeLabel->setAlignment( Qt::AlignVCenter | Qt::AlignLeft );
    m_insertModeLabel->setFocusProxy(m_view);

    m_infoLabel = new KSqueezedTextLabel( this );
    topLayout->addWidget( m_infoLabel, 1 );
    m_infoLabel->setTextFormat(Qt::PlainText);
    m_infoLabel->setMinimumSize( 0, 0 );
    m_infoLabel->setSizePolicy(QSizePolicy( QSizePolicy::Ignored, QSizePolicy::Fixed ));
    m_infoLabel->setAlignment(Qt::AlignVCenter | Qt::AlignRight);
    m_infoLabel->setFocusProxy(m_view);

    m_lineColLabel = new QLabel( this );
    topLayout->addWidget( m_lineColLabel, 0 );
    m_lineColLabel->setFocusProxy(m_view);

    /**
     * size grip for window resizing
     */
    topLayout->addWidget(new QSizeGrip(this));
    
    m_modPm = QIcon::fromTheme(QStringLiteral("document-save")).pixmap(16);
    m_modDiscPm = QIcon::fromTheme(QStringLiteral("dialog-warning")).pixmap(16);
    QIcon icon = KIconUtils::addOverlay(QIcon::fromTheme(QStringLiteral("document-save")),
                                        QIcon::fromTheme(QStringLiteral("emblem-important")),
                                        Qt::TopLeftCorner);
    m_modmodPm = icon.pixmap(16);

    // signals for the statusbar
    connect(m_view, SIGNAL(cursorPositionChanged(KTextEditor::View*,KTextEditor::Cursor)), this, SLOT(cursorPositionChanged()));
    connect(m_view, SIGNAL(viewModeChanged(KTextEditor::View*)), this, SLOT(viewModeChanged()));
    connect(m_view, SIGNAL(selectionChanged(KTextEditor::View*)), this, SLOT(selectionChanged()));
    connect(m_view, SIGNAL(informationMessage(KTextEditor::View*,QString)), this, SLOT(informationMessage(KTextEditor::View*,QString)));
    connect(m_view->document(), SIGNAL(modifiedChanged(KTextEditor::Document*)), this, SLOT(modifiedChanged()));
    connect(m_view->document(), SIGNAL(modifiedOnDisk(KTextEditor::Document*,bool,KTextEditor::ModificationInterface::ModifiedOnDiskReason)), this, SLOT(modifiedChanged()) );
    connect(m_view->document(), SIGNAL(configChanged()), this, SLOT(documentConfigChanged()));
    connect(m_view->document(), SIGNAL(modeChanged(KTextEditor::Document*)), this, SLOT(modeChanged()));

    updateStatus ();
}

void KateStatusBar::updateStatus ()
{
    viewModeChanged ();
    cursorPositionChanged ();
    selectionChanged ();
    modifiedChanged ();
    m_infoLabel->clear ();
    documentConfigChanged();
    modeChanged();
}

void KateStatusBar::viewModeChanged ()
{
    m_insertModeLabel->setText( m_view->viewMode() );
}

void KateStatusBar::cursorPositionChanged ()
{
    KTextEditor::Cursor position (m_view->cursorPositionVirtual());

    m_lineColLabel->setText(
        i18n("Line %1, Col %2"
            , QLocale().toString(position.line() + 1)
            , QLocale().toString(position.column() + 1)
            )
        );
}

void KateStatusBar::selectionChanged ()
{
    m_selectModeLabel->setText( m_view->blockSelection() ? i18n(" BLOCK ") : i18n(" LINE ") );
}

void KateStatusBar::informationMessage (KTextEditor::View *, const QString &message)
{
    m_infoLabel->setText( message );

    // timer to reset this after 4 seconds
    QTimer::singleShot(4000, this, SLOT(updateStatus()));
}

void KateStatusBar::modifiedChanged()
{
    const bool mod = m_view->doc()->isModified();
    const bool modOnHD = m_view->doc()->isModifiedOnDisc();

    m_modifiedLabel->setPixmap(
        mod ?
        modOnHD ?
        m_modmodPm :
    m_modPm :
        modOnHD ?
        m_modDiscPm :
        QPixmap()
        );
}

void KateStatusBar::documentConfigChanged ()
{
    m_encoding->setText( m_view->document()->encoding() );
}

void KateStatusBar::modeChanged ()
{
    m_mode->setText( KTextEditor::EditorPrivate::self()->modeManager()->fileType(m_view->document()->mode()).nameTranslated() );
}
