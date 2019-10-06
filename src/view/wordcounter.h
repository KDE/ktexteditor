/* This file is part of the KDE libraries

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

#ifndef WORDCOUNTER_H
#define WORDCOUNTER_H

#include <QObject>
#include <QString>
#include <QTimer>
#include <QVector>

namespace KTextEditor
{
class Document;
class DocumentPrivate;
class View;
class ViewPrivate;
class Range;
}

class WordCounter : public QObject
{
    Q_OBJECT

public:
    explicit WordCounter(KTextEditor::ViewPrivate *view);

Q_SIGNALS:
    void changed(int wordsInDocument, int wordsInSelection, int charsInDocument, int charsInSelection);

private Q_SLOTS:
    void textInserted(KTextEditor::Document *document, const KTextEditor::Range &range);
    void textRemoved(KTextEditor::Document *document, const KTextEditor::Range &range, const QString &oldText);
    void recalculate(KTextEditor::Document *document);
    void selectionChanged(KTextEditor::View *view);
    void recalculateLines();

private:
    int countWords(const QString &text) const;

private:
    QVector<int> m_countByLine;
    int m_wordsInDocument, m_wordsInSelection;
    int m_charsInDocument, m_charsInSelection;
    QTimer m_timer;
    int m_startRecalculationFrom;
    KTextEditor::Document *m_document;
};

#endif
