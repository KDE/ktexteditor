/**
 * This file is part of the KDE libraries
 * Copyright (C) 2009 Milian Wolff <mail@milianw.de>
 * Copyright (C) 2002 John Firebaugh <jfirebaugh@kde.org>
 * Copyright (C) 2001 Christoph Cullmann <cullmann@kde.org>
 * Copyright (C) 2001 Joseph Wenninger <jowenn@kde.org>
 * Copyright (C) 1999 Jochen Wilhelmy <digisnap@cs.tu-berlin.de>
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

#include "exporter.h"
#include "abstractexporter.h"
#include "htmlexporter.h"

#include <ktexteditor/document.h>
#include <ktexteditor/view.h>

#include <KLocalizedString>

#include <QMimeData>
#include <QApplication>
#include <QClipboard>
#include <QTextCodec>
#include <QFileDialog>

void KateExporter::exportToClipboard()
{
    if (!m_view->selection()) {
        return;
    }

    QMimeData *data = new QMimeData();

    QString s;
    QTextStream output(&s, QIODevice::WriteOnly);
    exportData(true, output);

    data->setHtml(s);
    data->setText(s);

    QApplication::clipboard()->setMimeData(data);
}

void KateExporter::exportToFile(const QString &file)
{
    QFile savefile(file);
    if (!savefile.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        return;
    }
    
    QTextStream outputStream(&savefile);
    exportData(false, outputStream);
}

void KateExporter::exportData(const bool useSelection, QTextStream &output)
{
    const KTextEditor::Range range = useSelection ? m_view->selectionRange() : m_view->document()->documentRange();
    const bool blockwise = useSelection ? m_view->blockSelection() : false;

    if ((blockwise || range.onSingleLine()) && (range.start().column() > range.end().column())) {
        return;
    }

    output.setCodec(QTextCodec::codecForName("UTF-8"));

    ///TODO: add more exporters
    QScopedPointer<AbstractExporter> exporter;

    exporter.reset(new HTMLExporter(m_view, output, !useSelection));

    const KTextEditor::Attribute::Ptr noAttrib(nullptr);

    for (int i = range.start().line(); (i <= range.end().line()) && (i < m_view->document()->lines()); ++i) {
        const QString &line = m_view->document()->line(i);

        QList<KTextEditor::AttributeBlock> attribs = m_view->lineAttributes(i);

        int lineStart = 0;
        int remainingChars = line.length();
        if (blockwise || range.onSingleLine()) {
            lineStart = range.start().column();
            remainingChars = range.columnWidth();
        } else if (i == range.start().line()) {
            lineStart = range.start().column();
        } else if (i == range.end().line()) {
            remainingChars = range.end().column();
        }

        int handledUntil = lineStart;

        foreach (const KTextEditor::AttributeBlock & block, attribs) {
            // honor (block-) selections
            if (block.start + block.length <= lineStart) {
                continue;
            } else if (block.start >= lineStart + remainingChars) {
                break;
            }
            int start = qMax(block.start, lineStart);
            if (start > handledUntil) {
                exporter->exportText(line.mid(handledUntil, start - handledUntil), noAttrib);
            }
            int length = qMin(block.length, remainingChars);
            exporter->exportText(line.mid(start, length), block.attribute);
            handledUntil = start + length;
        }

        if (handledUntil < lineStart + remainingChars) {
            exporter->exportText(line.mid(handledUntil, remainingChars), noAttrib);
        }

        exporter->closeLine(i == range.end().line());
    }

    output.flush();
}
