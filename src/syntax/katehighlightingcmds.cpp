/*  This file is part of the KDE libraries and the Kate part.
 *
 *  Copyright (C) 2014 Christoph Rüßler <christoph.ruessler@mailbox.org>
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

#include "katehighlightingcmds.h"

#include "katedocument.h"
#include "kateglobal.h"
#include "katehighlight.h"
#include "katesyntaxmanager.h"
#include "kateview.h"

#include <QUrl>

KateCommands::Highlighting *KateCommands::Highlighting::m_instance = nullptr;

bool KateCommands::Highlighting::exec(KTextEditor::View *view, const QString &cmd, QString &, const KTextEditor::Range &)
{
    if (cmd.startsWith(QLatin1String("reload-highlighting"))) {
        KateHlManager *manager = KTextEditor::EditorPrivate::self()->hlManager();
        manager->reload();

        return true;
    } else if (cmd.startsWith(QLatin1String("edit-highlighting"))) {
        KTextEditor::DocumentPrivate *document = static_cast<KTextEditor::DocumentPrivate *>(view->document());
        KateHighlighting *highlighting = document->highlight();

        if (!highlighting->noHighlighting()) {
            QUrl url = QUrl::fromLocalFile(highlighting->getIdentifier());
            KTextEditor::Application *app = KTextEditor::Editor::instance()->application();
            app->openUrl(url);
        }

        return true;
    }

    return true;
}

bool KateCommands::Highlighting::help(KTextEditor::View *, const QString &, QString &)
{
    return false;
}
