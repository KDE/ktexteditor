/*  This file is part of the KDE libraries and the Kate part.
 *
 *  Copyright (C) 2009-2010 by Michel Ludwig <michel.ludwig@kdemail.net>
 *  Copyright (C) 2008 Mirko Stocker <me@misto.ch>
 *  Copyright (C) 2004-2005 Anders Lund <anders@alweb.dk>
 *  Copyright (C) 2002 John Firebaugh <jfirebaugh@kde.org>
 *  Copyright (C) 2001-2004 Christoph Cullmann <cullmann@kde.org>
 *  Copyright (C) 2001 Joseph Wenninger <jowenn@kde.org>
 *  Copyright (C) 1999 Jochen Wilhelmy <digisnap@cs.tu-berlin.de>
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

#ifndef KATE_SPELLCHECKDIALOG_H
#define KATE_SPELLCHECKDIALOG_H

#include <QObject>

namespace KTextEditor { class ViewPrivate; }

class QAction;
class KActionCollection;

namespace Sonnet {
    class BackgroundChecker;
    class Speller;
}

#include "ktexteditor/range.h"

namespace KTextEditor
{
class MovingRange;
}

class SpellCheckBar;

class KateSpellCheckDialog : public QObject
{
    Q_OBJECT

public:
    explicit KateSpellCheckDialog(KTextEditor::ViewPrivate *);
    ~KateSpellCheckDialog();

    void createActions(KActionCollection *);

    // spellcheck from cursor, selection
private Q_SLOTS:
    void spellcheckFromCursor();

    // defined here in anticipation of per view selections ;)
    void spellcheckSelection();

    void spellcheck();

    /**
    * Spellcheck a defined portion of the text.
    *
    * @param from Where to start the check
    * @param to Where to end. If this is (0,0), it will be set to the end of the document.
    */
    void spellcheck(const KTextEditor::Cursor &from, const KTextEditor::Cursor &to = KTextEditor::Cursor());

    void misspelling(const QString &, int);
    void corrected(const QString &, int, const QString &);

    void performSpellCheck(const KTextEditor::Range &range);
    void installNextSpellCheckRange();

    void cancelClicked();

    void objectDestroyed(QObject *object);

    void languageChanged(const QString &language);

private:
    KTextEditor::Cursor locatePosition(int pos);

    KTextEditor::ViewPrivate *m_view;

    Sonnet::Speller *m_speller;
    Sonnet::BackgroundChecker *m_backgroundChecker;

    SpellCheckBar *m_sonnetDialog;

    // define the part of the text that is to be checked
    KTextEditor::Range m_currentSpellCheckRange;
    KTextEditor::MovingRange *m_globalSpellCheckRange;

    QList<QPair<int, int> > m_currentDecToEncOffsetList;

    QList<QPair<KTextEditor::Range, QString> > m_languagesInSpellCheckRange;
    QList<QPair<KTextEditor::Range, QString> >::iterator m_currentLanguageRangeIterator;

    // keep track of where we are.
    KTextEditor::Cursor m_spellPosCursor;
    uint m_spellLastPos;

    bool m_spellCheckCancelledByUser;

    QString m_userSpellCheckLanguage, m_previousGivenSpellCheckLanguage;

    void spellCheckDone();
};

#endif

