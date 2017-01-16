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

#include <QDir>
#include <QTimer>

#include <KLocalizedString>
#include <KTextEditor/Application>
#include <KTextEditor/Document>
#include <KTextEditor/Editor>
#include <KTextEditor/MainWindow>
#include <KTextEditor/View>

#include <vimode/appcommands.h>

using namespace KateVi;

//BEGIN AppCommands
AppCommands *AppCommands::m_instance = nullptr;

AppCommands::AppCommands()
    : KTextEditor::Command(QStringList() << QStringLiteral("q") << QStringLiteral("qa") << QStringLiteral("qall") << QStringLiteral("q!") << QStringLiteral("qa!") << QStringLiteral("qall!")
          << QStringLiteral("w") << QStringLiteral("wq") << QStringLiteral("wa") << QStringLiteral("wqa") << QStringLiteral("x") << QStringLiteral("xa") << QStringLiteral("new")
          << QStringLiteral("vnew") << QStringLiteral("e") << QStringLiteral("edit") << QStringLiteral("enew") << QStringLiteral("sp") << QStringLiteral("split") << QStringLiteral("vs")
          << QStringLiteral("vsplit") << QStringLiteral("only") << QStringLiteral("tabe") << QStringLiteral("tabedit") << QStringLiteral("tabnew") << QStringLiteral("bd")
          << QStringLiteral("bdelete") << QStringLiteral("tabc") << QStringLiteral("tabclose") << QStringLiteral("clo") << QStringLiteral("close"))
{
    re_write.setPattern(QStringLiteral("w(a)?"));
    re_close.setPattern(QStringLiteral("bd(elete)?|tabc(lose)?"));
    re_quit.setPattern(QStringLiteral("(w)?q(a|all)?(!)?"));
    re_exit.setPattern(QStringLiteral("x(a)?"));
    re_edit.setPattern(QStringLiteral("e(dit)?|tabe(dit)?|tabnew"));
    re_new.setPattern(QStringLiteral("(v)?new"));
    re_split.setPattern(QStringLiteral("sp(lit)?"));
    re_vsplit.setPattern(QStringLiteral("vs(plit)?"));
    re_vclose.setPattern(QStringLiteral("clo(se)?"));
    re_only.setPattern(QStringLiteral("on(ly)?"));
}

AppCommands::~AppCommands()
{
    m_instance = nullptr;
}

bool AppCommands::exec(KTextEditor::View *view, const QString &cmd, QString &msg, const KTextEditor::Range &)
{
    QStringList args(cmd.split(QRegExp(QLatin1String("\\s+")), QString::SkipEmptyParts)) ;
    QString command(args.takeFirst());

    KTextEditor::MainWindow *mainWin = view->mainWindow();
    KTextEditor::Application *app = KTextEditor::Editor::instance()->application();

    if (re_write.exactMatch(command)) {  //TODO: handle writing to specific file
        if (!re_write.cap(1).isEmpty()) { // [a]ll
            Q_FOREACH(KTextEditor::Document *doc, app->documents()) {
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
    } else if (re_quit.exactMatch(command)) {
        const bool save = !re_quit.cap(1).isEmpty(); // :[w]q
        const bool allDocuments = !re_quit.cap(2).isEmpty(); // :q[all]
        const bool doNotPromptForSave = !re_quit.cap(3).isEmpty(); // :q[!]

        if (allDocuments) {
            if (save) {
                Q_FOREACH(KTextEditor::Document *doc, app->documents()) {
                    doc->save();
                }
            }

            if (doNotPromptForSave) {
                Q_FOREACH(KTextEditor::Document *doc, app->documents()) {
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
            Q_FOREACH(KTextEditor::Document *doc, app->documents()) {
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
    } else if (re_edit.exactMatch(command)) {
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
    } else if (re_new.exactMatch(command)) {
        if (re_new.cap(1) == QLatin1String("v")) { // vertical split
            mainWin->splitView(Qt::Horizontal);
        } else {                    // horizontal split
            mainWin->splitView(Qt::Vertical);
        }
        mainWin->openUrl(QUrl());
    } else if (command == QLatin1String("enew")) {
        mainWin->openUrl(QUrl());
    } else if (re_split.exactMatch(command)) {
        mainWin->splitView(Qt::Horizontal);
    } else if (re_vsplit.exactMatch(command)) {
        mainWin->splitView(Qt::Vertical);
    } else if (re_vclose.exactMatch(command)) {
        QTimer::singleShot(0, this, SLOT(closeCurrentSplitView()));
    } else if (re_only.exactMatch(command)) {
        QTimer::singleShot(0, this, SLOT(closeOtherSplitViews()));
    }

    return true;
}

bool AppCommands::help(KTextEditor::View *view, const QString &cmd, QString &msg)
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
    } else if (re_quit.exactMatch(cmd)) {
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
    } else if (re_exit.exactMatch(cmd)) {
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
    } else if (re_split.exactMatch(cmd)) {
        msg = i18n("<p><b>sp,split&mdash; Split horizontally the current view into two</b></p>"
                   "<p>Usage: <tt><b>sp[lit]</b></tt></p>"
                   "<p>The result is two views on the same document.</p>");
        return true;
    } else if (re_vsplit.exactMatch(cmd)) {
        msg = i18n("<p><b>vs,vsplit&mdash; Split vertically the current view into two</b></p>"
                   "<p>Usage: <tt><b>vs[plit]</b></tt></p>"
                   "<p>The result is two views on the same document.</p>");
        return true;
    } else if (re_vclose.exactMatch(cmd)) {
        msg = i18n("<p><b>clo[se]&mdash; Close the current view</b></p>"
                   "<p>Usage: <tt><b>clo[se]</b></tt></p>"
                   "<p>After executing it, the current view will be closed.</p>");
        return true;
    } else if (re_new.exactMatch(cmd)) {
        msg = i18n("<p><b>[v]new &mdash; split view and create new document</b></p>"
                   "<p>Usage: <tt><b>[v]new</b></tt></p>"
                   "<p>Splits the current view and opens a new document in the new view."
                   " This command can be called in two ways:<br />"
                   " <tt>new</tt> &mdash; splits the view horizontally and opens a new document.<br />"
                   " <tt>vnew</tt> &mdash; splits the view vertically and opens a new document.<br />"
                   "</p>");
        return true;
    } else if (re_edit.exactMatch(cmd)) {
        msg = i18n("<p><b>e[dit] &mdash; reload current document</b></p>"
                   "<p>Usage: <tt><b>e[dit]</b></tt></p>"
                   "<p>Starts <b>e</b>diting the current document again. This is useful to re-edit"
                   " the current file, when it has been changed by another program.</p>");
        return true;
    }

    return false;
}

KTextEditor::View * AppCommands::findViewInDifferentSplitView(KTextEditor::MainWindow *window,
                                                                    KTextEditor::View *view)
{
    Q_FOREACH (KTextEditor::View *it, window->views()) {
        if (!window->viewsInSameSplitView(it, view)) {
            return it;
        }
    }
    return Q_NULLPTR;
}

void AppCommands::closeCurrentDocument()
{
    KTextEditor::Application *app = KTextEditor::Editor::instance()->application();
    KTextEditor::Document *doc = app->activeMainWindow()->activeView()->document();
    app->closeDocument(doc);
}

void AppCommands::closeCurrentView()
{
    KTextEditor::Application *app = KTextEditor::Editor::instance()->application();
    KTextEditor::MainWindow *mw = app->activeMainWindow();
    mw->closeView(mw->activeView());
}

void AppCommands::closeCurrentSplitView()
{
    KTextEditor::Application *app = KTextEditor::Editor::instance()->application();
    KTextEditor::MainWindow *mw = app->activeMainWindow();
    mw->closeSplitView(mw->activeView());
}

void AppCommands::closeOtherSplitViews()
{
    KTextEditor::Application *app = KTextEditor::Editor::instance()->application();
    KTextEditor::MainWindow *mw = app->activeMainWindow();
    KTextEditor::View *view = mw->activeView();
    KTextEditor::View *viewToRemove = Q_NULLPTR;

    while ((viewToRemove = findViewInDifferentSplitView(mw, view))) {
        mw->closeSplitView(viewToRemove);
    }
}

void AppCommands::quit()
{
    KTextEditor::Editor::instance()->application()->quit();
}

//END AppCommands

//BEGIN KateViBufferCommand
BufferCommands *BufferCommands::m_instance = nullptr;

BufferCommands::BufferCommands()
    : KTextEditor::Command(QStringList() << QStringLiteral("ls")
           << QStringLiteral("b") << QStringLiteral("buffer")
           << QStringLiteral("bn") << QStringLiteral("bnext") << QStringLiteral("bp") << QStringLiteral("bprevious")
           << QStringLiteral("tabn") << QStringLiteral("tabnext") << QStringLiteral("tabp") << QStringLiteral("tabprevious")
           << QStringLiteral("bf") << QStringLiteral("bfirst") << QStringLiteral("bl") << QStringLiteral("blast")
           << QStringLiteral("tabf") << QStringLiteral("tabfirst") << QStringLiteral("tabl") << QStringLiteral("tablast"))
{
}

BufferCommands::~BufferCommands()
{
    m_instance = nullptr;
}

bool BufferCommands::exec(KTextEditor::View *view, const QString &cmd, QString &, const KTextEditor::Range &)
{
    // create list of args
    QStringList args(cmd.split(QLatin1Char(' '), QString::KeepEmptyParts));
    QString command = args.takeFirst(); // same as cmd if split failed
    QString argument = args.join(QLatin1Char(' '));

    if (command == QLatin1String("ls")) {
        // TODO: open quickview
    } else if (command == QLatin1String("b") || command == QLatin1String("buffer")) {
        switchDocument(view, argument);
    } else if (command == QLatin1String("bp") || command == QLatin1String("bprevious")) {
        prevBuffer(view);
    } else if (command == QLatin1String("bn") || command == QLatin1String("bnext")) {
        nextBuffer(view);
    } else if (command == QLatin1String("bf") || command == QLatin1String("bfirst")) {
        firstBuffer(view);
    } else if (command == QLatin1String("bl") || command == QLatin1String("blast")) {
        lastBuffer(view);
    } else if (command == QLatin1String("tabn") || command == QLatin1String("tabnext")) {
        nextTab(view);
    } else if (command == QLatin1String("tabp") || command == QLatin1String("tabprevious")) {
        prevTab(view);
    } else if (command == QLatin1String("tabf") || command == QLatin1String("tabfirst")) {
        firstTab(view);
    } else if (command == QLatin1String("tabl") || command == QLatin1String("tablast")) {
        lastTab(view);
    }
    return true;
}

void BufferCommands::switchDocument(KTextEditor::View *view, const QString &address)
{

    if (address.isEmpty()) {
        // no argument: switch to the previous document
        prevBuffer(view);
        return;
    }

    const int idx = address.toInt();
    QList<KTextEditor::Document *> docs = documents();

    if (idx > 0 && idx <= docs.size()) {
        // numerical argument: switch to the nth document
        activateDocument(view, docs.at(idx - 1));
    } else {
        // string argument: switch to the given file
        KTextEditor::Document *doc = nullptr;

        Q_FOREACH(KTextEditor::Document *it, docs) {
            if (it->documentName() == address) {
                doc = it;
                break;
            }
        }

        if (doc) {
            activateDocument(view, docs.at(idx - 1));
        }
    }
}

void BufferCommands::prevBuffer(KTextEditor::View *view)
{
    QList<KTextEditor::Document *> docs = documents();
    const int idx = docs.indexOf(view->document());

    if (idx > 0) {
        activateDocument(view, docs.at(idx - 1));
    } else { // wrap
        activateDocument(view, docs.last());
    }
}

void BufferCommands::nextBuffer(KTextEditor::View *view)
{
    QList<KTextEditor::Document *> docs = documents();
    const int idx = docs.indexOf(view->document());

    if (idx + 1 < docs.size()) {
        activateDocument(view, docs.at(idx + 1));
    } else { // wrap
        activateDocument(view, docs.first());
    }
}

void BufferCommands::firstBuffer(KTextEditor::View *view)
{
    activateDocument(view, documents().at(0));
}

void BufferCommands::lastBuffer(KTextEditor::View *view)
{
    activateDocument(view, documents().last());
}

void BufferCommands::prevTab(KTextEditor::View *view)
{
    prevBuffer(view); // TODO: implement properly, when interface is added
}

void BufferCommands::nextTab(KTextEditor::View *view)
{
    nextBuffer(view); // TODO: implement properly, when interface is added
}

void BufferCommands::firstTab(KTextEditor::View *view)
{
    firstBuffer(view); // TODO: implement properly, when interface is added
}

void BufferCommands::lastTab(KTextEditor::View *view)
{
    lastBuffer(view); // TODO: implement properly, when interface is added
}

void BufferCommands::activateDocument(KTextEditor::View *view, KTextEditor::Document *doc)
{
    KTextEditor::MainWindow *mainWindow = view->mainWindow();
    mainWindow->activateView(doc);
}

QList< KTextEditor::Document * > BufferCommands::documents()
{
    KTextEditor::Application *app = KTextEditor::Editor::instance()->application();
    return app->documents();
}

bool BufferCommands::help(KTextEditor::View * /*view*/, const QString &cmd, QString &msg)
{
    if (cmd == QLatin1String("b") || cmd == QLatin1String("buffer")) {
        msg = i18n("<p><b>b,buffer &mdash; Edit document N from the document list</b></p>"
                   "<p>Usage: <tt><b>b[uffer] [N]</b></tt></p>");
        return true;
    } else if (cmd == QLatin1String("bp") || cmd == QLatin1String("bprevious") ||
               cmd == QLatin1String("tabp") || cmd == QLatin1String("tabprevious")) {
        msg = i18n("<p><b>bp,bprev &mdash; previous buffer</b></p>"
                   "<p>Usage: <tt><b>bp[revious] [N]</b></tt></p>"
                   "<p>Goes to <b>[N]</b>th previous document (\"<b>b</b>uffer\") in document list. </p>"
                   "<p> <b>[N]</b> defaults to one. </p>"
                   "<p>Wraps around the start of the document list.</p>");
        return true;
    } else if (cmd == QLatin1String("bn") || cmd == QLatin1String("bnext") ||
               cmd == QLatin1String("tabn") || cmd == QLatin1String("tabnext")) {
        msg = i18n("<p><b>bn,bnext &mdash; switch to next document</b></p>"
                   "<p>Usage: <tt><b>bn[ext] [N]</b></tt></p>"
                   "<p>Goes to <b>[N]</b>th next document (\"<b>b</b>uffer\") in document list."
                   "<b>[N]</b> defaults to one. </p>"
                   "<p>Wraps around the end of the document list.</p>");
        return true;
    } else if (cmd == QLatin1String("bf") || cmd == QLatin1String("bfirst") ||
               cmd == QLatin1String("tabf") || cmd == QLatin1String("tabfirst")) {
        msg = i18n("<p><b>bf,bfirst &mdash; first document</b></p>"
                   "<p>Usage: <tt><b>bf[irst]</b></tt></p>"
                   "<p>Goes to the <b>f</b>irst document (\"<b>b</b>uffer\") in document list.</p>");
        return true;
    } else if (cmd == QLatin1String("bl") || cmd == QLatin1String("blast") ||
               cmd == QLatin1String("tabl") || cmd == QLatin1String("tablast")) {
        msg = i18n("<p><b>bl,blast &mdash; last document</b></p>"
                   "<p>Usage: <tt><b>bl[ast]</b></tt></p>"
                   "<p>Goes to the <b>l</b>ast document (\"<b>b</b>uffer\") in document list.</p>");
        return true;
    } else if (cmd == QLatin1String("ls")) {
        msg = i18n("<p><b>ls</b></p>"
                   "<p>list current buffers<p>");
    }

    return false;
}
//END KateViBufferCommand
