/*
    SPDX-FileCopyrightText: 2001-2003 Christoph Cullmann <cullmann@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef KATE_HIGHLIGHTMENU_H
#define KATE_HIGHLIGHTMENU_H

#include <QHash>
#include <QPointer>
#include <QStringList>

#include <KActionMenu>

namespace KTextEditor
{
class DocumentPrivate;
}

class KateHighlightingMenu : public KActionMenu
{
    Q_OBJECT

public:
    KateHighlightingMenu(const QString &text, QObject *parent)
        : KActionMenu(text, parent)
    {
        init();
        setPopupMode(QToolButton::InstantPopup);
    }

    ~KateHighlightingMenu();

    void updateMenu(KTextEditor::DocumentPrivate *doc);

private:
    void init();

    QPointer<KTextEditor::DocumentPrivate> m_doc;
    QStringList subMenusName;
    QStringList names;
    QList<QMenu *> subMenus;
    QList<QAction *> subActions;
    QActionGroup *m_actionGroup;

public Q_SLOTS:
    void slotAboutToShow();

private Q_SLOTS:
    void setHl();
};

#endif
