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

#ifndef KATEVI_APP_COMMANDS_H
#define KATEVI_APP_COMMANDS_H

#include <KTextEditor/Command>
#include <QObject>

namespace KTextEditor {
    class MainWindow;
}

namespace KateVi
{

class AppCommands : public KTextEditor::Command
{
    Q_OBJECT

    AppCommands();
    static AppCommands* m_instance;

public:
    virtual ~AppCommands();
    virtual bool exec(KTextEditor::View *view, const QString &cmd, QString &msg, const KTextEditor::Range &range = KTextEditor::Range::invalid());
    virtual bool help(KTextEditor::View *view, const QString &cmd, QString &msg);

    static AppCommands* self() {
        if (m_instance == 0) {
            m_instance = new AppCommands();
        }
        return m_instance;
    }

private:
    /**
     * @returns a view in the given \p window that does not share a split
     * view with the given \p view. If such view could not be found, then
     * nullptr is returned.
     */
    KTextEditor::View * findViewInDifferentSplitView(KTextEditor::MainWindow *window,
                                                     KTextEditor::View *view);

private Q_SLOTS:
    void closeCurrentDocument();
    void closeCurrentView();
    void closeCurrentSplitView();
    void closeOtherSplitViews();
    void quit();

private:
    QRegExp re_write;
    QRegExp re_close;
    QRegExp re_quit;
    QRegExp re_exit;
    QRegExp re_edit;
    QRegExp re_new;
    QRegExp re_split;
    QRegExp re_vsplit;
    QRegExp re_vclose;
    QRegExp re_only;
};

class BufferCommands : public KTextEditor::Command
{
    Q_OBJECT

    BufferCommands();
    static BufferCommands* m_instance;

public:
    virtual ~BufferCommands();
    virtual bool exec(KTextEditor::View *view, const QString &cmd, QString &msg, const KTextEditor::Range &range = KTextEditor::Range::invalid());
    virtual bool help(KTextEditor::View *view, const QString &cmd, QString &msg);

    static BufferCommands* self() {
        if (m_instance == 0) {
            m_instance = new BufferCommands();
        }
        return m_instance;
    }

private:
    void switchDocument(KTextEditor::View *, const QString &doc);
    void prevBuffer(KTextEditor::View *);
    void nextBuffer(KTextEditor::View *);
    void firstBuffer(KTextEditor::View *);
    void lastBuffer(KTextEditor::View *);
    void prevTab(KTextEditor::View *);
    void nextTab(KTextEditor::View *);
    void firstTab(KTextEditor::View *);
    void lastTab(KTextEditor::View *);

    void activateDocument(KTextEditor::View *, KTextEditor::Document *);
    QList<KTextEditor::Document *> documents();
};

}

#endif /* KATEVI_APP_COMMANDS_H */
