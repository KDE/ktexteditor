/*
    SPDX-FileCopyrightText: 2007 David Nolden <david.nolden.kdevelop@art-master.de>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "expandingwidgetmodel.h"

#include <QApplication>
#include <QBrush>
#include <QModelIndex>
#include <QTreeView>

#include <KColorUtils>
#include <KTextEdit>
#include <ktexteditor/codecompletionmodel.h>

#include "expandingdelegate.h"
#include "katepartdebug.h"

using namespace KTextEditor;

inline QModelIndex firstColumn(const QModelIndex &index)
{
    return index.sibling(index.row(), 0);
}

ExpandingWidgetModel::ExpandingWidgetModel(QWidget *parent)
    : QAbstractItemModel(parent)
{
}

ExpandingWidgetModel::~ExpandingWidgetModel()
{
    clearExpanding();
}

static QColor doAlternate(const QColor &color)
{
    QColor background = QApplication::palette().window().color();
    return KColorUtils::mix(color, background, 0.15);
}

uint ExpandingWidgetModel::matchColor(const QModelIndex &index) const
{
    int matchQuality = contextMatchQuality(index.sibling(index.row(), 0));

    if (matchQuality > 0) {
        bool alternate = index.row() & 1;

        QColor badMatchColor(0xff00aa44); // Blue-ish green
        QColor goodMatchColor(0xff00ff00); // Green

        QColor background = treeView()->palette().light().color();

        QColor totalColor = KColorUtils::mix(badMatchColor, goodMatchColor, ((float)matchQuality) / 10.0);

        if (alternate) {
            totalColor = doAlternate(totalColor);
        }

        const qreal dynamicTint = 0.2;
        const qreal minimumTint = 0.2;
        qreal tintStrength = (dynamicTint * matchQuality) / 10;
        if (tintStrength != 0.0) {
            tintStrength += minimumTint; // Some minimum tinting strength, else it's not visible any more
        }

        return KColorUtils::tint(background, totalColor, tintStrength).rgb();
    } else {
        return 0;
    }
}

QVariant ExpandingWidgetModel::data(const QModelIndex &index, int role) const
{
    switch (role) {
    case Qt::BackgroundRole: {
        if (index.column() == 0) {
            // Highlight by match-quality
            uint color = matchColor(index);
            if (color) {
                return QBrush(color);
            }
        }
        // Use a special background-color for expanded items
        if (isExpanded(index)) {
            if (index.row() & 1) {
                return doAlternate(treeView()->palette().toolTipBase().color());
            } else {
                return treeView()->palette().toolTipBase();
            }
        }
    }
    }
    return QVariant();
}

void ExpandingWidgetModel::clearMatchQualities()
{
    m_contextMatchQualities.clear();
}

QModelIndex ExpandingWidgetModel::partiallyExpandedRow() const
{
    if (m_partiallyExpanded.isEmpty()) {
        return QModelIndex();
    } else {
        return m_partiallyExpanded.constBegin().key();
    }
}

void ExpandingWidgetModel::clearExpanding()
{
    clearMatchQualities();
    QMap<QModelIndex, ExpandingWidgetModel::ExpandingType> oldExpandState = m_expandState;
    for (auto &widget : std::as_const(m_expandingWidgets)) {
        if (widget) {
            widget->deleteLater(); // By using deleteLater, we prevent crashes when an action within a widget makes the completion cancel
        }
    }
    m_expandingWidgets.clear();
    m_expandState.clear();
    m_partiallyExpanded.clear();

    for (auto it = oldExpandState.constBegin(); it != oldExpandState.constEnd(); ++it) {
        if (it.value() == Expanded) {
            Q_EMIT dataChanged(it.key(), it.key());
        }
    }
}

ExpandingWidgetModel::ExpansionType ExpandingWidgetModel::isPartiallyExpanded(const QModelIndex &index) const
{
    if (m_partiallyExpanded.contains(firstColumn(index))) {
        return m_partiallyExpanded[firstColumn(index)];
    } else {
        return NotExpanded;
    }
}

void ExpandingWidgetModel::partiallyUnExpand(const QModelIndex &idx_)
{
    QModelIndex index(firstColumn(idx_));
    m_partiallyExpanded.remove(index);
    m_partiallyExpanded.remove(idx_);
}

int ExpandingWidgetModel::partiallyExpandWidgetHeight() const
{
    return 60; ///@todo use font-metrics text-height*2 for 2 lines
}

void ExpandingWidgetModel::rowSelected(const QModelIndex &idx_)
{
    QModelIndex idx(firstColumn(idx_));
    if (!m_partiallyExpanded.contains(idx)) {
        QModelIndex oldIndex = partiallyExpandedRow();
        // Unexpand the previous partially expanded row
        if (!m_partiallyExpanded.isEmpty()) {
            ///@todo allow multiple partially expanded rows
            while (!m_partiallyExpanded.isEmpty()) {
                m_partiallyExpanded.erase(m_partiallyExpanded.begin());
            }
            // partiallyUnExpand( m_partiallyExpanded.begin().key() );
        }
        // Notify the underlying models that the item was selected, and eventually get back the text for the expanding widget.
        if (!idx.isValid()) {
            // All items have been unselected
            if (oldIndex.isValid()) {
                Q_EMIT dataChanged(oldIndex, oldIndex);
            }
        } else {
            QVariant variant = data(idx, CodeCompletionModel::ItemSelected);

            if (!isExpanded(idx) && variant.type() == QVariant::String) {
                // Either expand upwards or downwards, choose in a way that
                // the visible fields of the new selected entry are not moved.
                if (oldIndex.isValid() && (oldIndex < idx || (!(oldIndex < idx) && oldIndex.parent() < idx.parent()))) {
                    m_partiallyExpanded.insert(idx, ExpandUpwards);
                } else {
                    m_partiallyExpanded.insert(idx, ExpandDownwards);
                }

                // Say that one row above until one row below has changed, so no items will need to be moved(the space that is taken from one item is given to
                // the other)
                if (oldIndex.isValid() && oldIndex < idx) {
                    Q_EMIT dataChanged(oldIndex, idx);

                    if (treeView()->verticalScrollMode() == QAbstractItemView::ScrollPerItem) {
                        // Qt fails to correctly scroll in ScrollPerItem mode, so the selected index is completely visible,
                        // so we do the scrolling by hand.
                        QRect selectedRect = treeView()->visualRect(idx);
                        QRect frameRect = treeView()->frameRect();

                        if (selectedRect.bottom() > frameRect.bottom()) {
                            int diff = selectedRect.bottom() - frameRect.bottom();
                            // We need to scroll down
                            QModelIndex newTopIndex = idx;

                            QModelIndex nextTopIndex = idx;
                            QRect nextRect = treeView()->visualRect(nextTopIndex);
                            while (nextTopIndex.isValid() && nextRect.isValid() && nextRect.top() >= diff) {
                                newTopIndex = nextTopIndex;
                                nextTopIndex = treeView()->indexAbove(nextTopIndex);
                                if (nextTopIndex.isValid()) {
                                    nextRect = treeView()->visualRect(nextTopIndex);
                                }
                            }
                            treeView()->scrollTo(newTopIndex, QAbstractItemView::PositionAtTop);
                        }
                    }

                    // This is needed to keep the item we are expanding completely visible. Qt does not scroll the view to keep the item visible.
                    // But we must make sure that it isn't too expensive.
                    // We need to make sure that scrolling is efficient, and the whole content is not repainted.
                    // Since we are scrolling anyway, we can keep the next line visible, which might be a cool feature.

                    // Since this also doesn't work smoothly, leave it for now
                    // treeView()->scrollTo( nextLine, QAbstractItemView::EnsureVisible );
                } else if (oldIndex.isValid() && idx < oldIndex) {
                    Q_EMIT dataChanged(idx, oldIndex);

                    // For consistency with the down-scrolling, we keep one additional line visible above the current visible.

                    // Since this also doesn't work smoothly, leave it for now
                    /*            QModelIndex prevLine = idx.sibling(idx.row()-1, idx.column());
                                if( prevLine.isValid() )
                                    treeView()->scrollTo( prevLine );*/
                } else {
                    Q_EMIT dataChanged(idx, idx);
                }
            } else if (oldIndex.isValid()) {
                // We are not partially expanding a new row, but we previously had a partially expanded row. So signalize that it has been unexpanded.

                Q_EMIT dataChanged(oldIndex, oldIndex);
            }
        }
    } else {
        qCDebug(LOG_KTE) << "ExpandingWidgetModel::rowSelected: Row is already partially expanded";
    }
}

QString ExpandingWidgetModel::partialExpandText(const QModelIndex &idx) const
{
    if (!idx.isValid()) {
        return QString();
    }

    return data(firstColumn(idx), CodeCompletionModel::ItemSelected).toString();
}

QRect ExpandingWidgetModel::partialExpandRect(const QModelIndex &idx_) const
{
    QModelIndex idx(firstColumn(idx_));

    if (!idx.isValid()) {
        return QRect();
    }

    ExpansionType expansion = ExpandDownwards;

    if (m_partiallyExpanded.find(idx) != m_partiallyExpanded.constEnd()) {
        expansion = m_partiallyExpanded[idx];
    }

    // Get the whole rectangle of the row:
    QModelIndex rightMostIndex = idx;
    QModelIndex tempIndex = idx;
    while ((tempIndex = rightMostIndex.sibling(rightMostIndex.row(), rightMostIndex.column() + 1)).isValid()) {
        rightMostIndex = tempIndex;
    }

    QRect rect = treeView()->visualRect(idx);
    QRect rightMostRect = treeView()->visualRect(rightMostIndex);

    rect.setLeft(rect.left() + 20);
    rect.setRight(rightMostRect.right() - 5);

    // These offsets must match exactly those used in ExpandingDelegate::sizeHint()
    int top = rect.top() + 5;
    int bottom = rightMostRect.bottom() - 5;

    if (expansion == ExpandDownwards) {
        top += basicRowHeight(idx);
    } else {
        bottom -= basicRowHeight(idx);
    }

    rect.setTop(top);
    rect.setBottom(bottom);

    return rect;
}

bool ExpandingWidgetModel::isExpandable(const QModelIndex &idx_) const
{
    QModelIndex idx(firstColumn(idx_));

    if (!m_expandState.contains(idx)) {
        m_expandState.insert(idx, NotExpandable);
        QVariant v = data(idx, CodeCompletionModel::IsExpandable);
        if (v.canConvert<bool>() && v.toBool()) {
            m_expandState[idx] = Expandable;
        }
    }

    return m_expandState[idx] != NotExpandable;
}

bool ExpandingWidgetModel::isExpanded(const QModelIndex &idx_) const
{
    QModelIndex idx(firstColumn(idx_));
    return m_expandState.contains(idx) && m_expandState[idx] == Expanded;
}

void ExpandingWidgetModel::setExpanded(QModelIndex idx_, bool expanded)
{
    QModelIndex idx(firstColumn(idx_));

    // qCDebug(LOG_KTE) << "Setting expand-state of row " << idx.row() << " to " << expanded;
    if (!idx.isValid()) {
        return;
    }

    if (isExpandable(idx)) {
        if (!expanded && m_expandingWidgets.contains(idx) && m_expandingWidgets[idx]) {
            m_expandingWidgets[idx]->hide();
        }

        m_expandState[idx] = expanded ? Expanded : Expandable;

        if (expanded) {
            partiallyUnExpand(idx);
        }

        if (expanded && !m_expandingWidgets.contains(idx)) {
            QVariant v = data(idx, CodeCompletionModel::ExpandingWidget);

            if (v.canConvert<QWidget *>()) {
                m_expandingWidgets[idx] = v.value<QWidget *>();
            } else if (v.canConvert<QString>()) {
                // Create a html widget that shows the given string
                KTextEdit *edit = new KTextEdit(v.toString());
                edit->setReadOnly(true);
                edit->resize(200, 50); // Make the widget small so it embeds nicely.
                m_expandingWidgets[idx] = edit;
            } else {
                m_expandingWidgets[idx] = nullptr;
            }
        }

        // Eventually partially expand the row
        if (!expanded && firstColumn(treeView()->currentIndex()) == idx && !isPartiallyExpanded(idx)) {
            rowSelected(idx); // Partially expand the row.
        }

        Q_EMIT dataChanged(idx, idx);

        if (treeView()) {
            treeView()->scrollTo(idx);
        }
    }
}

int ExpandingWidgetModel::basicRowHeight(const QModelIndex &idx_) const
{
    QModelIndex idx(firstColumn(idx_));

    ExpandingDelegate *delegate = dynamic_cast<ExpandingDelegate *>(treeView()->itemDelegate(idx));
    if (!delegate || !idx.isValid()) {
        qCDebug(LOG_KTE) << "ExpandingWidgetModel::basicRowHeight: Could not get delegate";
        return 15;
    }
    return delegate->basicSizeHint(idx).height();
}

void ExpandingWidgetModel::placeExpandingWidget(const QModelIndex &idx_)
{
    QModelIndex idx(firstColumn(idx_));
    if (!idx.isValid() || !isExpanded(idx)) {
        return;
    }

    QWidget *w = m_expandingWidgets.value(idx);
    if (!w) {
        return;
    }

    QRect rect = treeView()->visualRect(idx);

    if (!rect.isValid() || rect.bottom() < 0 || rect.top() >= treeView()->height()) {
        // The item is currently not visible
        w->hide();
        return;
    }

    // Find out the basic width of the row
    rect.setLeft(rect.left() + 20);
    for (int i = 0, numColumns = idx.model()->columnCount(idx.parent()); i < numColumns; ++i) {
        QModelIndex rightMostIndex = idx.sibling(idx.row(), i);
        int right = treeView()->visualRect(rightMostIndex).right();
        if (right > rect.right()) {
            rect.setRight(right);
        }
    }
    rect.setRight(rect.right() - 5);

    // These offsets must match exactly those used in KateCompletionDeleage::sizeHint()
    rect.setTop(rect.top() + basicRowHeight(idx) + 5);
    rect.setHeight(w->height());

    if (w->parent() != treeView()->viewport() || w->geometry() != rect || !w->isVisible()) {
        w->setParent(treeView()->viewport());

        w->setGeometry(rect);
        w->show();
    }
}

void ExpandingWidgetModel::placeExpandingWidgets()
{
    for (QMap<QModelIndex, QPointer<QWidget>>::const_iterator it = m_expandingWidgets.constBegin(); it != m_expandingWidgets.constEnd(); ++it) {
        placeExpandingWidget(it.key());
    }
}

int ExpandingWidgetModel::expandingWidgetsHeight() const
{
    int sum = 0;
    for (QMap<QModelIndex, QPointer<QWidget>>::const_iterator it = m_expandingWidgets.constBegin(); it != m_expandingWidgets.constEnd(); ++it) {
        if (isExpanded(it.key()) && (*it)) {
            sum += (*it)->height();
        }
    }
    return sum;
}

QWidget *ExpandingWidgetModel::expandingWidget(const QModelIndex &idx_) const
{
    QModelIndex idx(firstColumn(idx_));

    if (m_expandingWidgets.contains(idx)) {
        return m_expandingWidgets[idx];
    } else {
        return nullptr;
    }
}

void ExpandingWidgetModel::cacheIcons() const
{
    if (m_expandedIcon.isNull()) {
        m_expandedIcon = QIcon::fromTheme(QStringLiteral("arrow-down"));
    }

    if (m_collapsedIcon.isNull()) {
        m_collapsedIcon = QIcon::fromTheme(QStringLiteral("arrow-right"));
    }
}

QList<QVariant> mergeCustomHighlighting(int leftSize, const QList<QVariant> &left, int rightSize, const QList<QVariant> &right)
{
    QList<QVariant> ret = left;
    if (left.isEmpty()) {
        ret << QVariant(0);
        ret << QVariant(leftSize);
        ret << QTextFormat(QTextFormat::CharFormat);
    }

    if (right.isEmpty()) {
        ret << QVariant(leftSize);
        ret << QVariant(rightSize);
        ret << QTextFormat(QTextFormat::CharFormat);
    } else {
        QList<QVariant>::const_iterator it = right.constBegin();
        while (it != right.constEnd()) {
            {
                QList<QVariant>::const_iterator testIt = it;
                for (int a = 0; a < 2; a++) {
                    ++testIt;
                    if (testIt == right.constEnd()) {
                        qCWarning(LOG_KTE) << "Length of input is not multiple of 3";
                        break;
                    }
                }
            }

            ret << QVariant((*it).toInt() + leftSize);
            ++it;
            ret << QVariant((*it).toInt());
            ++it;
            ret << *it;
            if (!(*it).value<QTextFormat>().isValid()) {
                qCDebug(LOG_KTE) << "Text-format is invalid";
            }
            ++it;
        }
    }
    return ret;
}

// It is assumed that between each two strings, one space is inserted
QList<QVariant> mergeCustomHighlighting(QStringList strings, QList<QVariantList> highlights, int grapBetweenStrings)
{
    if (strings.isEmpty()) {
        qCWarning(LOG_KTE) << "List of strings is empty";
        return QList<QVariant>();
    }

    if (highlights.isEmpty()) {
        qCWarning(LOG_KTE) << "List of highlightings is empty";
        return QList<QVariant>();
    }

    if (strings.count() != highlights.count()) {
        qCWarning(LOG_KTE) << "Length of string-list is " << strings.count() << " while count of highlightings is " << highlights.count() << ", should be same";
        return QList<QVariant>();
    }

    // Merge them together
    QString totalString = strings[0];
    QVariantList totalHighlighting = highlights[0];

    strings.pop_front();
    highlights.pop_front();

    while (!strings.isEmpty()) {
        totalHighlighting = mergeCustomHighlighting(totalString.length(), totalHighlighting, strings[0].length(), highlights[0]);
        totalString += strings[0];

        for (int a = 0; a < grapBetweenStrings; a++) {
            totalString += QLatin1Char(' ');
        }

        strings.pop_front();
        highlights.pop_front();
    }
    // Combine the custom-highlightings
    return totalHighlighting;
}
