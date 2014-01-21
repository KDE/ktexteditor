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

#include <KIconUtils>
#include <KLocalizedString>

#include <QVBoxLayout>

KateStatusBar::KateStatusBar(KTextEditor::ViewPrivate *view)
    : KateViewBarWidget(false)
    , m_view(view)
    , m_statusBar(new QStatusBar(centralWidget()))
{
    /**
     * just add our status bar to central widget, full sized
     */
    QVBoxLayout *topLayout = new QVBoxLayout(centralWidget());
    topLayout->setMargin(0);
    topLayout->addWidget(m_statusBar);

    QString lineColText = i18n(" Line: %1 Col: %2 ", QLocale().toString(4444), QLocale().toString(44));

    m_lineColLabel = new QLabel( m_statusBar );
    m_lineColLabel->setMinimumWidth( m_lineColLabel->fontMetrics().width( lineColText ) );
    m_statusBar->addWidget( m_lineColLabel, 0 );
    m_lineColLabel->installEventFilter( this );

    m_modifiedLabel = new QLabel( m_statusBar );
    m_modifiedLabel->setFixedSize( 16, 16 );
    m_statusBar->addWidget( m_modifiedLabel, 0 );
    m_modifiedLabel->setAlignment( Qt::AlignCenter );
    m_modifiedLabel->installEventFilter( this );

    m_selectModeLabel = new QLabel( i18n(" LINE "), m_statusBar );
    m_statusBar->addWidget( m_selectModeLabel, 0 );
    m_selectModeLabel->setAlignment( Qt::AlignCenter );
    m_selectModeLabel->installEventFilter( this );

    m_insertModeLabel = new QLabel( i18n(" INS "), m_statusBar );
    m_statusBar->addWidget( m_insertModeLabel, 0 );
    m_insertModeLabel->setAlignment( Qt::AlignVCenter | Qt::AlignLeft );
    m_insertModeLabel->installEventFilter( this );

    m_infoLabel = new KSqueezedTextLabel( m_statusBar );
    m_statusBar->addPermanentWidget( m_infoLabel, 1 );
    m_infoLabel->setTextFormat(Qt::PlainText);
    m_infoLabel->setMinimumSize( 0, 0 );
    m_infoLabel->setSizePolicy(QSizePolicy( QSizePolicy::Ignored, QSizePolicy::Fixed ));
    m_infoLabel->setAlignment(Qt::AlignVCenter | Qt::AlignRight);
    m_infoLabel->installEventFilter( this );


    m_encodingLabel = new QLabel( QString(), m_statusBar );
    m_statusBar->addPermanentWidget( m_encodingLabel, 0 );
    m_encodingLabel->setAlignment( Qt::AlignCenter );
    m_encodingLabel->installEventFilter( this );

#ifdef Q_WS_MAC
    m_statusBar->setSizeGripEnabled( false );
    m_statusBar->addPermanentWidget( new QSizeGrip( m_statusBar ) );
#endif

    installEventFilter( m_statusBar );
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

    updateStatus ();
}

bool KateStatusBar::eventFilter(QObject*, QEvent *e)
{
  if (e->type() == QEvent::MouseButtonPress)
  {
    m_view->setFocus();
    return true;
  }

  return false;
}

void KateStatusBar::updateStatus ()
{
  viewModeChanged ();
  cursorPositionChanged ();
  selectionChanged ();
  modifiedChanged ();
  documentConfigChanged ();
  m_infoLabel->clear ();
}

void KateStatusBar::viewModeChanged ()
{
  m_insertModeLabel->setText( QString::fromLatin1(" %1 ").arg (m_view->viewMode()) );
}

void KateStatusBar::cursorPositionChanged ()
{
  KTextEditor::Cursor position (m_view->cursorPositionVirtual());

  m_lineColLabel->setText(
      i18n(
          " Line: %1 of %2 Col: %3 "
        , QLocale().toString(position.line() + 1)
        , m_view->document()->lines()
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
    bool mod = m_view->document()->isModified();

    bool modOnHD = false;//FIXME KF5 info && info->modifiedOnDisc;

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
    m_encodingLabel->setText( QString::fromLatin1(" %1 ").arg (m_view->document()->encoding()) );
}
