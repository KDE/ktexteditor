/*  This file is part of the Kate project.
 *
 *  Copyright (C) 2010-2018 Dominik Haumann <dhaumann@kde.org>
 *  Copyright (C) 2010 Diana-Victoria Tiriplica <diana.tiriplica@gmail.com>
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

#ifndef KATE_SWAPFILE_H
#define KATE_SWAPFILE_H

#include <QObject>
#include <QDataStream>
#include <QFile>
#include <QTimer>

#include <ktexteditor_export.h>
#include "katetextbuffer.h"
#include "katebuffer.h"
#include "katedocument.h"

#include <KTextEditor/Message>

namespace KTextEditor { class ViewPrivate; }

namespace Kate
{

/**
 * Class for tracking editing actions.
 * In case Kate crashes, this can be used to replay all edit actions to
 * recover the lost data.
 */
class KTEXTEDITOR_EXPORT SwapFile : public QObject
{
    Q_OBJECT

public:
    explicit SwapFile(KTextEditor::DocumentPrivate *document);
    ~SwapFile();
    bool shouldRecover() const;

    void fileClosed();
    QString fileName();

    KTextEditor::DocumentPrivate *document();

private:
    void setTrackingEnabled(bool trackingEnabled);
    void removeSwapFile();
    bool updateFileName();
    bool isValidSwapFile(QDataStream &stream, bool checkDigest) const;

private:
    KTextEditor::DocumentPrivate *m_document;
    bool m_trackingEnabled;

protected Q_SLOTS:
    void fileSaved(const QString &filename);
    void fileLoaded(const QString &filename);
    void modifiedChanged();

    void startEditing();
    void finishEditing();

    void wrapLine(const KTextEditor::Cursor &position);
    void unwrapLine(int line);
    void insertText(const KTextEditor::Cursor &position, const QString &text);
    void removeText(const KTextEditor::Range &range);

public Q_SLOTS:
    void discard();
    void recover();
    bool recover(QDataStream &, bool checkDigest = true);
    void configChanged();

private:
    QDataStream m_stream;
    QFile m_swapfile;
    bool m_recovered;
    bool m_needSync;
    static QTimer *s_timer;

protected Q_SLOTS:
    void writeFileToDisk();

private:
    QTimer *syncTimer();

public Q_SLOTS:
    void showSwapFileMessage();
    void showDiff();

private:
    QPointer<KTextEditor::Message> m_swapMessage;
};

}

#endif // KATE_SWAPFILE_H

