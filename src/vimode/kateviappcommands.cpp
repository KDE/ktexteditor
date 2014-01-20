/* This file is part of the KDE libraries
   Copyright (C) 2009 Erlend Hamberg <ehamberg@gmail.com>
   Copyright (C) 2011 Svyatoslav Kuzmich <svatoslav1@gmail.com>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#include "kateviappcommands.h"

#include <KTextEditor/Application>
#include <KTextEditor/Document>
#include <KTextEditor/Editor>
#include <KTextEditor/MainWindow>
#include <KTextEditor/View>

#include <KLocalizedString>

#include <QDir>
#include <QTimer>

KateViAppCommands *KateViAppCommands::m_instance = 0;

KateViAppCommands::KateViAppCommands()
    : QObject()
    , KTextEditor::Command()
{
    KTextEditor::CommandInterface *iface = qobject_cast<KTextEditor::CommandInterface*>(KTextEditor::Editor::instance());

    if (iface) {
        iface->registerCommand(this);
    }

    re_write.setPattern(QLatin1String("w(a)?"));
    re_close.setPattern(QLatin1String("bd(elete)?|tabc(lose)?"));
    re_quit.setPattern(QLatin1String("(w)?q(a|all)?(!)?"));
    re_exit.setPattern(QLatin1String("x(a)?"));
    re_edit.setPattern(QLatin1String("e(dit)?|tabe(dit)?|tabnew"));
    re_new.setPattern(QLatin1String("(v)?new"));
    re_split.setPattern(QLatin1String("sp(lit)?"));
    re_vsplit.setPattern(QLatin1String("vs(plit)?"));
    re_only.setPattern(QLatin1String("on(ly)?"));
}

KateViAppCommands::~KateViAppCommands()
{
    KTextEditor::CommandInterface *iface = qobject_cast<KTextEditor::CommandInterface*>(KTextEditor::Editor::instance());

    if (iface) {
        iface->unregisterCommand(this);
    }

    m_instance = 0;
}

const QStringList& KateViAppCommands::cmds()
{
    static QStringList l;

    if (l.empty()) {
        l << QLatin1String("q") << QLatin1String("qa") << QLatin1String("qall") << QLatin1String("q!") << QLatin1String("qa!") << QLatin1String("qall!")
        << QLatin1String("wq") << QLatin1String("wa") << QLatin1String("wqa") << QLatin1String("x") << QLatin1String("xa") << QLatin1String("new")
        << QLatin1String("vnew") << QLatin1String("e") << QLatin1String("edit") << QLatin1String("enew") << QLatin1String("sp") << QLatin1String("split") << QLatin1String("vs")
        << QLatin1String("vsplit") << QLatin1String("only") << QLatin1String("tabe") << QLatin1String("tabedit") << QLatin1String("tabnew") << QLatin1String("bd")
        << QLatin1String("bdelete") << QLatin1String("tabc") << QLatin1String("tabclose");
    }

    return l;
}

bool KateViAppCommands::exec(KTextEditor::View *view, const QString &cmd, QString &msg)
{
    QStringList args(cmd.split( QRegExp(QLatin1String("\\s+")), QString::SkipEmptyParts)) ;
    QString command( args.takeFirst() );

    KTextEditor::MainWindow *mainWin = view->mainWindow();
    KTextEditor::Application *app = KTextEditor::Editor::instance()->application();

    if (re_write.exactMatch(command)) {  //TODO: handle writing to specific file
        if (!re_write.cap(1).isEmpty()) { // [a]ll
            Q_FOREACH (KTextEditor::Document *doc, app->documents()) {
                doc->save();
            }
            msg = i18n("All documents written to disk");
        } else {
            view->document()->documentSave();
            msg = i18n("Document written to disk");
        }
    }
    // Other buffer commands are implemented by the KateFileTree plugin
    else if (re_close.exactMatch(command)) {
        app->closeDocument(view->document());
    }
    else if (re_quit.exactMatch(command)) {
        const bool save = !re_quit.cap(1).isEmpty(); // :[w]q
        const bool allDocuments = !re_quit.cap(2).isEmpty(); // :q[all]
        const bool doNotPromptForSave = !re_quit.cap(3).isEmpty(); // :q[!]

        if (allDocuments) {
            if (save) {
                Q_FOREACH (KTextEditor::Document *doc, app->documents()) {
                    doc->save();
                }
            }

            if (doNotPromptForSave) {
                Q_FOREACH (KTextEditor::Document *doc, app->documents()) {
                    if (doc->isModified()) {
                        doc->setModified(false);
                    }
                }
            }

            QTimer::singleShot(0, this, SLOT(quit()));
        } else {
            if (save && view->document()->isModified()) {
                view->document()->documentSave();
            }

            if (doNotPromptForSave) {
                view->document()->setModified(false);
            }

            if (mainWin->views().size() > 1) {
                QTimer::singleShot(0, this, SLOT(closeCurrentView()));
            } else {
                if (app->documents().size() > 1) {
                    QTimer::singleShot(0, this, SLOT(closeCurrentDocument()));
                } else {
                    QTimer::singleShot(0, this, SLOT(quit()));
                }
            }
        }
    } else if (re_exit.exactMatch(command)) {
        if (!re_exit.cap(1).isEmpty()) { // a[ll]
            Q_FOREACH (KTextEditor::Document *doc, app->documents()) {
                doc->save();
            }
            QTimer::singleShot(0, this, SLOT(quit()));
        } else {
            if (view->document()->isModified()) {
                view->document()->documentSave();
            }

            if (app->documents().size() > 1) {
                QTimer::singleShot(0, this, SLOT(closeCurrentDocument()));
            } else {
                QTimer::singleShot(0, this, SLOT(quit()));
            }
        }
    }
    else if (re_edit.exactMatch(command)) {
        QString argument = args.join(QLatin1Char(' '));
        if (argument.isEmpty() || argument == QLatin1String("!")) {
            view->document()->documentReload();
        } else {
            QUrl base = view->document()->url();
            QUrl url;
            QUrl arg2path(argument);
            if (base.isValid()) { // first try to use the same path as the current open document has
                url = QUrl(base.resolved(arg2path));  //resolved handles the case where the args is a relative path, and is the same as using QUrl(args) elsewise
            } else { // else use the cwd
                url = QUrl(QUrl(QDir::currentPath() + QLatin1String("/")).resolved(arg2path)); // + "/" is needed because of http://lists.qt.nokia.com/public/qt-interest/2011-May/033913.html
            }
            QFileInfo file(url.toLocalFile());

            KTextEditor::Document *doc = app->findUrl(url);

            if (doc) {
                mainWin->activateView(doc);
            } else if (file.exists()) {
                app->openUrl(url);
            } else {
                app->openUrl(QUrl())->saveAs(url);
            }
        }
    }
    else if (re_new.exactMatch(command)) {
        if (re_new.cap(1) == QLatin1String("v")) { // vertical split
            mainWin->splitView(Qt::Vertical);
        } else {                    // horizontal split
            mainWin->splitView(Qt::Horizontal);
        }
        mainWin->openUrl(QUrl());
    }
    else if (command == QLatin1String("enew")) {
        mainWin->openUrl(QUrl());
    }
    else if (re_split.exactMatch(command)) {
        mainWin->splitView(Qt::Horizontal);
    }
    else if (re_vsplit.exactMatch(command)) {
        mainWin->splitView(Qt::Vertical);
    } else if (re_only.exactMatch(command)) {
        Q_FOREACH (KTextEditor::View *it, mainWin->views()) {
            if (it != view) {
                mainWin->closeView(it);
            }
        }
    }

    return true;
}

bool KateViAppCommands::help(KTextEditor::View *view, const QString &cmd, QString &msg)
{
    Q_UNUSED(view);

    if (re_write.exactMatch(cmd)) {
        msg = i18n("<p><b>w/wa &mdash; write document(s) to disk</b></p>"
        "<p>Usage: <tt><b>w[a]</b></tt></p>"
        "<p>Writes the current document(s) to disk. "
        "It can be called in two ways:<br />"
        " <tt>w</tt> &mdash; writes the current document to disk<br />"
        " <tt>wa</tt> &mdash; writes all documents to disk.</p>"
        "<p>If no file name is associated with the document, "
        "a file dialog will be shown.</p>");
        return true;
    }
    else if (re_quit.exactMatch(cmd)) {
        msg = i18n("<p><b>q/qa/wq/wqa &mdash; [write and] quit</b></p>"
        "<p>Usage: <tt><b>[w]q[a]</b></tt></p>"
        "<p>Quits the application. If <tt>w</tt> is prepended, it also writes"
        " the document(s) to disk. This command "
        "can be called in several ways:<br />"
        " <tt>q</tt> &mdash; closes the current view.<br />"
        " <tt>qa</tt> &mdash; closes all views, effectively quitting the application.<br />"
        " <tt>wq</tt> &mdash; writes the current document to disk and closes its view.<br />"
        " <tt>wqa</tt> &mdash; writes all documents to disk and quits.</p>"
        "<p>In all cases, if the view being closed is the last view, the application quits. "
        "If no file name is associated with the document and it should be written to disk, "
        "a file dialog will be shown.</p>");
        return true;
    }
    else if (re_exit.exactMatch(cmd)) {
        msg = i18n("<p><b>x/xa &mdash; write and quit</b></p>"
        "<p>Usage: <tt><b>x[a]</b></tt></p>"
        "<p>Saves document(s) and quits (e<b>x</b>its). This command "
        "can be called in two ways:<br />"
        " <tt>x</tt> &mdash; closes the current view.<br />"
        " <tt>xa</tt> &mdash; closes all views, effectively quitting the application.</p>"
        "<p>In all cases, if the view being closed is the last view, the application quits. "
        "If no file name is associated with the document and it should be written to disk, "
        "a file dialog will be shown.</p>"
        "<p>Unlike the 'w' commands, this command only writes the document if it is modified."
        "</p>");
        return true;
    }
    else if (re_split.exactMatch(cmd)) {
        msg = i18n("<p><b>sp,split&mdash; Split horizontally the current view into two</b></p>"
        "<p>Usage: <tt><b>sp[lit]</b></tt></p>"
        "<p>The result is two views on the same document.</p>");
        return true;
    }
    else if (re_vsplit.exactMatch(cmd)) {
        msg = i18n("<p><b>vs,vsplit&mdash; Split vertically the current view into two</b></p>"
        "<p>Usage: <tt><b>vs[plit]</b></tt></p>"
        "<p>The result is two views on the same document.</p>");
        return true;
    }
    else if (re_new.exactMatch(cmd)) {
        msg = i18n("<p><b>[v]new &mdash; split view and create new document</b></p>"
        "<p>Usage: <tt><b>[v]new</b></tt></p>"
        "<p>Splits the current view and opens a new document in the new view."
        " This command can be called in two ways:<br />"
        " <tt>new</tt> &mdash; splits the view horizontally and opens a new document.<br />"
        " <tt>vnew</tt> &mdash; splits the view vertically and opens a new document.<br />"
        "</p>");
        return true;
    }
    else if (re_edit.exactMatch(cmd)) {
        msg = i18n("<p><b>e[dit] &mdash; reload current document</b></p>"
        "<p>Usage: <tt><b>e[dit]</b></tt></p>"
        "<p>Starts <b>e</b>diting the current document again. This is useful to re-edit"
        " the current file, when it has been changed by another program.</p>");
        return true;
    }

    return false;
}

void KateViAppCommands::closeCurrentDocument()
{
    KTextEditor::Application *app = KTextEditor::Editor::instance()->application();
    KTextEditor::Document *doc = app->activeMainWindow()->activeView()->document();
    app->closeDocument(doc);
    qDebug() << "called close";
}

void KateViAppCommands::closeCurrentView()
{
    KTextEditor::Application *app = KTextEditor::Editor::instance()->application();
    KTextEditor::MainWindow *mw = app->activeMainWindow();
    mw->closeView(mw->activeView());
    qDebug() << "called close view";
}

void KateViAppCommands::quit()
{
    KTextEditor::Editor::instance()->application()->quit();
}
