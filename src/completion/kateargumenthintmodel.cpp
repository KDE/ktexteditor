/*
    SPDX-FileCopyrightText: 2007 David Nolden <david.nolden.kdevelop@art-master.de>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "kateargumenthintmodel.h"

#include "kateargumenthinttree.h"
#include "katecompletiontree.h"
#include "katecompletionwidget.h"
#include "katepartdebug.h"

#include <ktexteditor/codecompletionmodel.h>

#include <QApplication>
#include <QGridLayout>
#include <QTextFormat>

using namespace KTextEditor;

void KateArgumentHintModel::clear()
{
    m_rows.clear();
    clearExpanding();
}

QModelIndex KateArgumentHintModel::mapToSource(const QModelIndex &index) const
{
    if (index.row() < 0 || index.row() >= m_rows.count()) {
        return QModelIndex();
    }

    if (m_rows[index.row()] < 0 || m_rows[index.row()] >= (int)group()->filtered.size()) {
        return QModelIndex();
    }

    KateCompletionModel::ModelRow source = group()->filtered[m_rows[index.row()]].sourceRow();
    if (!source.first) {
        qCDebug(LOG_KTE) << "KateArgumentHintModel::data: Row does not exist in source";
        return QModelIndex();
    }

    QModelIndex sourceIndex = source.second.sibling(source.second.row(), index.column());

    return sourceIndex;
}

void KateArgumentHintModel::parentModelReset()
{
    clear();
    buildRows();
}

void KateArgumentHintModel::buildRows()
{
    beginResetModel();

    m_rows.clear();
    QMap<int, QList<int>> m_depths; // Map each hint-depth to a list of functions of that depth
    for (int a = 0; a < (int)group()->filtered.size(); a++) {
        KateCompletionModel::ModelRow source = group()->filtered[a].sourceRow();
        QModelIndex sourceIndex = source.second.sibling(source.second.row(), 0);
        QVariant v = sourceIndex.data(CodeCompletionModel::ArgumentHintDepth);
        if (v.type() == QVariant::Int) {
            QList<int> &lst(m_depths[v.toInt()]);
            lst << a;
        }
    }

    for (QMap<int, QList<int>>::const_iterator it = m_depths.constBegin(); it != m_depths.constEnd(); ++it) {
        for (int row : *it) {
            m_rows.push_front(row); // Insert filtered in reversed order
        }
        m_rows.push_front(-it.key());
    }

    endResetModel();

    Q_EMIT contentStateChanged(!m_rows.isEmpty());
}

KateArgumentHintModel::KateArgumentHintModel(KateCompletionWidget *parent)
    : ExpandingWidgetModel(parent)
    , m_parent(parent)
{
    connect(parent->model(), &KateCompletionModel::modelReset, this, &KateArgumentHintModel::parentModelReset);
    connect(parent->model(), &KateCompletionModel::argumentHintsChanged, this, &KateArgumentHintModel::parentModelReset);
}

QVariant KateArgumentHintModel::data(const QModelIndex &index, int role) const
{
    if (index.row() < 0 || index.row() >= m_rows.count()) {
        // qCDebug(LOG_KTE) << "KateArgumentHintModel::data: index out of bound: " << index.row() << " total filtered: " << m_rows.count();
        return QVariant();
    }

    if (m_rows[index.row()] < 0) {
        // Show labels
        if (role == Qt::DisplayRole && index.column() == 0) {
            return QString(); // QString("Depth %1").arg(-m_rows[index.row()]);
        } else if (role == Qt::BackgroundRole) {
            return QApplication::palette().toolTipBase().color();
        } else if (role == Qt::ForegroundRole) {
            return QApplication::palette().toolTipText().color();
        } else {
            return QVariant();
        }
    }

    if (m_rows[index.row()] < 0 || m_rows[index.row()] >= (int)group()->filtered.size()) {
        qCDebug(LOG_KTE) << "KateArgumentHintModel::data: index out of bound: " << m_rows[index.row()] << " total filtered: " << (int)group()->filtered.size();
        return QVariant();
    }

    KateCompletionModel::ModelRow source = group()->filtered[m_rows[index.row()]].sourceRow();
    if (!source.first) {
        qCDebug(LOG_KTE) << "KateArgumentHintModel::data: Row does not exist in source";
        return QVariant();
    }

    if (index.column() == 0) {
        switch (role) {
        case Qt::DecorationRole: {
            // Show the expand-handle
            model()->cacheIcons();

            if (!isExpanded(index)) {
                return QVariant(model()->m_collapsedIcon);
            } else {
                return QVariant(model()->m_expandedIcon);
            }
        }
        case Qt::DisplayRole:
            // Ignore text in the first column(we create our own compound text in the second)
            return QVariant();
        }
    }

    QModelIndex sourceIndex = source.second.sibling(source.second.row(), index.column());

    if (!sourceIndex.isValid()) {
        qCDebug(LOG_KTE) << "KateArgumentHintModel::data: Source-index is not valid";
        return QVariant();
    }

    switch (role) {
    case Qt::DisplayRole: {
        // Construct the text
        QString totalText;
        for (int a = CodeCompletionModel::Prefix; a <= CodeCompletionModel::Postfix; a++) {
            if (a != CodeCompletionModel::Scope) { // Skip the scope
                totalText += source.second.sibling(source.second.row(), a).data(Qt::DisplayRole).toString() + QLatin1Char(' ');
            }
        }

        return QVariant(totalText);
    }
    case CodeCompletionModel::HighlightingMethod: {
        // Return that we are doing custom-highlighting of one of the sub-strings does it
        for (int a = CodeCompletionModel::Prefix; a <= CodeCompletionModel::Postfix; a++) {
            QVariant method = source.second.sibling(source.second.row(), a).data(CodeCompletionModel::HighlightingMethod);
            if (method.type() == QVariant::Int && method.toInt() == CodeCompletionModel::CustomHighlighting) {
                return QVariant(CodeCompletionModel::CustomHighlighting);
            }
        }

        return QVariant();
    }
    case CodeCompletionModel::CustomHighlight: {
        QStringList strings;

        // Collect strings
        for (int a = CodeCompletionModel::Prefix; a <= CodeCompletionModel::Postfix; a++) {
            strings << source.second.sibling(source.second.row(), a).data(Qt::DisplayRole).toString();
        }

        QList<QVariantList> highlights;

        // Collect custom-highlightings
        for (int a = CodeCompletionModel::Prefix; a <= CodeCompletionModel::Postfix; a++) {
            highlights << source.second.sibling(source.second.row(), a).data(CodeCompletionModel::CustomHighlight).toList();
        }

        // Replace invalid QTextFormats with match-quality color or yellow.
        for (QList<QVariantList>::iterator it = highlights.begin(); it != highlights.end(); ++it) {
            QVariantList &list(*it);

            for (int a = 2; a < list.count(); a += 3) {
                if (list[a].canConvert<QTextFormat>()) {
                    QTextFormat f = list[a].value<QTextFormat>();

                    if (!f.isValid()) {
                        f = QTextFormat(QTextFormat::CharFormat);
                        uint color = matchColor(index);

                        if (color) {
                            f.setBackground(QBrush(color));
                        } else {
                            f.setBackground(Qt::yellow);
                        }

                        list[a] = QVariant(f);
                    }
                }
            }
        }

        return mergeCustomHighlighting(strings, highlights, 1);
    }
    case Qt::DecorationRole: {
        // Redirect the decoration to the decoration of the item-column
        return source.second.sibling(source.second.row(), CodeCompletionModel::Icon).data(role);
    }
    }

    QVariant v = ExpandingWidgetModel::data(index, role);
    if (v.isValid()) {
        return v;
    } else {
        return sourceIndex.data(role);
    }
}

int KateArgumentHintModel::rowCount(const QModelIndex &parent) const
{
    if (!parent.isValid()) {
        return m_rows.count();
    } else {
        return 0;
    }
}

int KateArgumentHintModel::columnCount(const QModelIndex & /*parent*/) const
{
    return 2; // 2 Columns, one for the expand-handle, one for the signature
}

KateCompletionModel::Group *KateArgumentHintModel::group() const
{
    return model()->m_argumentHints;
}

QModelIndex KateArgumentHintModel::index(int row, int column, const QModelIndex &parent) const
{
    if (row < 0 || row > rowCount() || column < 0 || column > columnCount() || parent.isValid()) {
        return {};
    }
    return createIndex(row, column);
}

QModelIndex KateArgumentHintModel::parent(const QModelIndex & /*parent*/) const
{
    return {};
}

KateCompletionModel *KateArgumentHintModel::model() const
{
    return m_parent->model();
}

QTreeView *KateArgumentHintModel::treeView() const
{
    return m_parent->argumentHintTree();
}

void KateArgumentHintModel::emitDataChanged(const QModelIndex &start, const QModelIndex &end)
{
    Q_EMIT dataChanged(start, end);
}

bool KateArgumentHintModel::indexIsItem(const QModelIndex &index) const
{
    return index.row() >= 0 && index.row() < m_rows.count() && m_rows[index.row()] >= 0;
}

int KateArgumentHintModel::contextMatchQuality(const QModelIndex &index) const
{
    int row = index.row();
    if (row < 0 || row >= m_rows.count()) {
        return -1;
    }

    if (m_rows[row] < 0 || m_rows[row] >= (int)group()->filtered.size()) {
        return -1; // Probably a label
    }

    KateCompletionModel::ModelRow source = group()->filtered[m_rows[row]].sourceRow();
    if (!source.first) {
        return -1;
    }

    QModelIndex sourceIndex = source.second.sibling(source.second.row(), 0);

    if (!sourceIndex.isValid()) {
        return -1;
    }

    int depth = sourceIndex.data(CodeCompletionModel::ArgumentHintDepth).toInt();

    switch (depth) {
    case 1: {
        // This argument-hint is on the lowest level, match it with the currently selected item in the completion-widget
        QModelIndex row = m_parent->treeView()->currentIndex();
        if (!row.isValid()) {
            return -1;
        }

        QModelIndex selectedIndex = m_parent->model()->mapToSource(row);
        if (!selectedIndex.isValid()) {
            return -1;
        }

        if (selectedIndex.model() != sourceIndex.model()) {
            return -1; // We can only match items from the same source-model
        }

        sourceIndex.data(CodeCompletionModel::SetMatchContext);

        QVariant v = selectedIndex.data(CodeCompletionModel::MatchQuality);
        if (v.type() == QVariant::Int) {
            return v.toInt();
        }
    } break;
    default:
        // Do some other nice matching here in future
        break;
    }

    return -1;
}
