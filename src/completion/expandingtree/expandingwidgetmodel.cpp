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

#include "katecompletiondelegate.h"
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

void ExpandingWidgetModel::clearExpanding()
{
    QMap<QModelIndex, ExpandingWidgetModel::ExpandingType> oldExpandState = m_expandState;
    for (auto &widget : std::as_const(m_expandingWidgets)) {
        if (widget) {
            widget->deleteLater(); // By using deleteLater, we prevent crashes when an action within a widget makes the completion cancel
        }
    }
    m_expandingWidgets.clear();
    m_expandState.clear();

    for (auto it = oldExpandState.constBegin(); it != oldExpandState.constEnd(); ++it) {
        if (it.value() == Expanded) {
            Q_EMIT dataChanged(it.key(), it.key());
        }
    }
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

        if (expanded && !m_expandingWidgets.contains(idx)) {
            QVariant v = data(idx, CodeCompletionModel::ExpandingWidget);

            if (v.canConvert<QWidget *>()) {
                m_expandingWidgets[idx] = v.value<QWidget *>();
            } else if (v.canConvert<QString>()) {
                // Create a html widget that shows the given string
                QTextEdit *edit = new QTextEdit(v.toString());
                edit->setReadOnly(true);
                edit->resize(200, 50); // Make the widget small so it embeds nicely.
                m_expandingWidgets[idx] = edit;
            } else {
                m_expandingWidgets[idx] = nullptr;
            }
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

    auto *delegate = (KateCompletionDelegate *)treeView()->itemDelegate(idx);
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
    const int numColumns = idx.model()->columnCount(idx.parent());
    int left = 0;
    for (int i = 0; i < numColumns; ++i) {
        auto index = idx.sibling(idx.row(), i);
        auto text = index.data().toString();
        if (!index.data(Qt::DecorationRole).isNull()) {
            left += 24;
        }

        if (!text.isEmpty()) {
            left += treeView()->visualRect(index).left();
            break;
        }
    }
    rect.setLeft(rect.left() + left);

    for (int i = 0; i < numColumns; ++i) {
        QModelIndex rightMostIndex = idx.sibling(idx.row(), i);
        int right = treeView()->visualRect(rightMostIndex).right();
        if (right > rect.right()) {
            rect.setRight(right);
        }
    }
    rect.setRight(rect.right() - 5);

    // These offsets must match exactly those used in KateCompletionDeleage::sizeHint()
    rect.setTop(rect.top() + basicRowHeight(idx));
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
