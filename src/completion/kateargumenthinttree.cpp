/*
    SPDX-FileCopyrightText: 2007 David Nolden <david.nolden.kdevelop@art-master.de>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "kateargumenthinttree.h"

#include "expandingtree/expandingwidgetmodel.h"
#include "kateargumenthintmodel.h"
#include "katecompletiondelegate.h"
#include "katecompletionwidget.h"
#include "kateview.h"

#include <QApplication>
#include <QHeaderView>
#include <QModelIndex>
#include <QPainter>
#include <QScreen>
#include <QScrollBar>

#include <KWindowSystem>

class ArgumentHintDelegate : public KateCompletionDelegate
{
public:
    using KateCompletionDelegate::KateCompletionDelegate;

protected:
    bool editorEvent(QEvent *event, QAbstractItemModel *, const QStyleOptionViewItem &, const QModelIndex &index) override
    {
        if (event->type() == QEvent::MouseButtonRelease) {
            event->accept();
            model()->setExpanded(index, !model()->isExpanded(index));

            return true;
        } else {
            event->ignore();
        }

        return false;
    }

    KateArgumentHintTree *tree() const
    {
        return static_cast<KateArgumentHintTree *>(parent());
    }

    KateArgumentHintModel *model() const
    {
        auto tree = this->tree();
        return tree->model();
    }

    QSize sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const override
    {
        QSize s = QStyledItemDelegate::sizeHint(option, index);
        if (model()->isExpanded(index) && model()->expandingWidget(index)) {
            QWidget *widget = model()->expandingWidget(index);
            QSize widgetSize = widget->size();

            s.setHeight(widgetSize.height() + s.height()
                        + 10); // 10 is the sum that must match exactly the offsets used in ExpandingWidgetModel::placeExpandingWidget
        }
        return s;
    }

    void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const override
    {
        if (index.column() == 0) {
            model()->placeExpandingWidget(index);
        }
        // paint the text at top if there is a widget below
        m_alignTop = model()->isExpanded(index);

        KateCompletionDelegate::paint(painter, option, index);
    }
};

KateArgumentHintTree::KateArgumentHintTree(KateCompletionWidget *parent)
    : QTreeView(parent)
    , m_parent(parent) // Do not use the completion-widget as widget-parent, because the argument-hint-tree will be rendered separately
{
    setFrameStyle(QFrame::Box | QFrame::Raised);

    setUniformRowHeights(false);
    header()->setMinimumSectionSize(0);

    setFocusPolicy(Qt::NoFocus);
    setWindowFlags(Qt::Tool | Qt::FramelessWindowHint);
    setUniformRowHeights(false);
    setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
    header()->hide();
    setRootIsDecorated(false);
    setIndentation(0);
    setAllColumnsShowFocus(true);
    setAlternatingRowColors(true);
    setItemDelegate(new ArgumentHintDelegate(this));
}

void KateArgumentHintTree::clearCompletion()
{
    setCurrentIndex(QModelIndex());
}

KateArgumentHintModel *KateArgumentHintTree::model() const
{
    return m_parent->argumentHintModel();
}

void KateArgumentHintTree::paintEvent(QPaintEvent *event)
{
    QTreeView::paintEvent(event);
    updateGeometry(); ///@todo delay this. It is needed here, because visualRect(...) returns an invalid rect in updateGeometry before the content is painted
}

void KateArgumentHintTree::rowsInserted(const QModelIndex &parent, int start, int end)
{
    QTreeView::rowsInserted(parent, start, end);
    updateGeometry();
}

int KateArgumentHintTree::sizeHintForColumn(int column) const
{
    return QTreeView::sizeHintForColumn(column);
}

unsigned int KateArgumentHintTree::rowHeight(const QModelIndex &index) const
{
    uint max = sizeHintForIndex(index).height();

    for (int a = 0; a < index.model()->columnCount(index.parent()); ++a) {
        QModelIndex i = index.sibling(index.row(), a);
        uint cSize = sizeHintForIndex(i).height();
        if (cSize > max) {
            max = cSize;
        }
    }
    return max;
}

void KateArgumentHintTree::updateGeometry(QRect geom)
{
    // Avoid recursive calls of updateGeometry
    static bool updatingGeometry = false;
    if (updatingGeometry) {
        return;
    }
    updatingGeometry = true;

    if (model()->rowCount(QModelIndex()) == 0) {
        /*  qCDebug(LOG_KTE) << "KateArgumentHintTree:: empty model";*/
        hide();
        setGeometry(geom);
        updatingGeometry = false;
        return;
    }

    int bottom = geom.bottom();
    int totalWidth = std::max(geom.width(), resizeColumns());
    int totalHeight = 0;
    for (int a = 0; a < model()->rowCount(QModelIndex()); ++a) {
        QModelIndex index(model()->index(a, 0));
        totalHeight += rowHeight(index);
        for (int b = 0; b < model()->rowCount(index); ++b) {
            QModelIndex childIndex = model()->index(b, 0, index);
            totalHeight += rowHeight(childIndex);
        }
    }

    totalHeight += frameWidth() * 2;

    geom.setHeight(totalHeight);

    geom.moveBottom(bottom);
    //   if( totalWidth > geom.width() )
    geom.setWidth(totalWidth);

    bool enableScrollBars = false;

    // Resize and move so it fits the screen horizontally
    const auto screenGeometry = m_parent->view()->screen()->availableGeometry();
    const int maxWidth = (screenGeometry.width() * 3) / 4;
    if (geom.width() > maxWidth) {
        geom.setWidth(maxWidth);
        geom.setHeight(geom.height() + horizontalScrollBar()->height() + 2);
        geom.moveBottom(bottom);
        enableScrollBars = true;
    }

    bool resized = false;
    if (!KWindowSystem::isPlatformWayland()) {
        if (geom.right() > screenGeometry.right()) {
            geom.moveRight(screenGeometry.right());
        }

        if (geom.left() < screenGeometry.left()) {
            geom.moveLeft(screenGeometry.left());
        }

        // Resize and move so it fits the screen vertically
        if (geom.top() < screenGeometry.top()) {
            const int offset = screenGeometry.top() - geom.top();
            geom.setBottom(geom.bottom() - offset);
            geom.moveTo(geom.left(), screenGeometry.top());
            resized = true;
        }
    }

    if (geom != geometry()) {
        setUpdatesEnabled(false);
        setAnimated(false);

        setHorizontalScrollBarPolicy(enableScrollBars ? Qt::ScrollBarAlwaysOn : Qt::ScrollBarAlwaysOff);

        /*  qCDebug(LOG_KTE) << "KateArgumentHintTree::updateGeometry: updating geometry to " << geom;*/
        setGeometry(geom);

        if (resized && currentIndex().isValid()) {
            scrollTo(currentIndex());
        }

        setUpdatesEnabled(true);
    }

    updatingGeometry = false;
}

int KateArgumentHintTree::resizeColumns()
{
    int totalSize = 0;
    for (int a = 0; a < header()->count(); a++) {
        int columnSize = sizeHintForColumn(a);
        setColumnWidth(a, columnSize);
        totalSize += columnSize;
    }
    return totalSize;
}

void KateArgumentHintTree::updateGeometry()
{
    updateGeometry(geometry());
}

bool KateArgumentHintTree::nextCompletion()
{
    QModelIndex current;
    QModelIndex firstCurrent = currentIndex();

    do {
        QModelIndex oldCurrent = currentIndex();

        current = moveCursor(MoveDown, Qt::NoModifier);

        if (current != oldCurrent && current.isValid()) {
            setCurrentIndex(current);

        } else {
            if (firstCurrent.isValid()) {
                setCurrentIndex(firstCurrent);
            }
            return false;
        }

    } while (!model()->indexIsItem(current));

    return true;
}

bool KateArgumentHintTree::previousCompletion()
{
    QModelIndex current;
    QModelIndex firstCurrent = currentIndex();

    do {
        QModelIndex oldCurrent = currentIndex();

        current = moveCursor(MoveUp, Qt::NoModifier);

        if (current != oldCurrent && current.isValid()) {
            setCurrentIndex(current);

        } else {
            if (firstCurrent.isValid()) {
                setCurrentIndex(firstCurrent);
            }
            return false;
        }

    } while (!model()->indexIsItem(current));

    return true;
}

bool KateArgumentHintTree::pageDown()
{
    QModelIndex old = currentIndex();
    QModelIndex current = moveCursor(MovePageDown, Qt::NoModifier);

    if (current.isValid()) {
        setCurrentIndex(current);
        if (!model()->indexIsItem(current)) {
            if (!nextCompletion()) {
                previousCompletion();
            }
        }
    }

    return current != old;
}

bool KateArgumentHintTree::pageUp()
{
    QModelIndex old = currentIndex();
    QModelIndex current = moveCursor(MovePageUp, Qt::NoModifier);

    if (current.isValid()) {
        setCurrentIndex(current);
        if (!model()->indexIsItem(current)) {
            if (!previousCompletion()) {
                nextCompletion();
            }
        }
    }
    return current != old;
}

void KateArgumentHintTree::top()
{
    QModelIndex current = moveCursor(MoveHome, Qt::NoModifier);
    setCurrentIndex(current);

    if (current.isValid()) {
        setCurrentIndex(current);
        if (!model()->indexIsItem(current)) {
            nextCompletion();
        }
    }
}

void KateArgumentHintTree::bottom()
{
    QModelIndex current = moveCursor(MoveEnd, Qt::NoModifier);
    setCurrentIndex(current);

    if (current.isValid()) {
        setCurrentIndex(current);
        if (!model()->indexIsItem(current)) {
            previousCompletion();
        }
    }
}
