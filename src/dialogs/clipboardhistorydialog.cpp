/*
    SPDX-FileCopyrightText: 2022 Eric Armbruster <eric1@armbruster-online.de>
    SPDX-FileCopyrightText: 2022 Waqar Ahmed <waqar.17a@gmail.com>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "clipboardhistorydialog.h"
#include "katedocument.h"
#include "katerenderer.h"
#include "kateview.h"

#include <QBoxLayout>
#include <QFont>
#include <QItemSelectionModel>
#include <QMimeDatabase>
#include <QSortFilterProxyModel>
#include <QStyledItemDelegate>

#include <KLocalizedString>
#include <KSyntaxHighlighting/Definition>
#include <KSyntaxHighlighting/Repository>
#include <KTextEditor/Editor>

class ClipboardHistoryModel : public QAbstractTableModel
{
    Q_OBJECT
public:
    enum Role { HighlightingRole = Qt::UserRole + 1,  };

    explicit ClipboardHistoryModel(QObject *parent)
        : QAbstractTableModel(parent)
    {
    }

    int rowCount(const QModelIndex &parent) const override
    {
        if (parent.isValid()) {
            return 0;
        }
        return m_modelEntries.size();
    }

    int columnCount(const QModelIndex &parent) const override
    {
        Q_UNUSED(parent);
        return 1;
    }

    QVariant data(const QModelIndex &idx, int role) const override
    {
        if (!idx.isValid()) {
            return {};
        }

        const ClipboardEntry &clipboardEntry = m_modelEntries.at(idx.row());
        if (role == Qt::DisplayRole) {
            return clipboardEntry.text;
        } else if (role == Role::HighlightingRole) {
            return clipboardEntry.fileName;
        } else if (role == Qt::DecorationRole) {
            return clipboardEntry.icon;
        }

        return {};
    }

    void refresh(const QVector<KTextEditor::EditorPrivate::ClipboardEntry> &clipboardEntry)
    {
        QVector<ClipboardEntry> temp;

        for (int i = 0; i < clipboardEntry.size(); ++i) {
            const auto entry = clipboardEntry.at(i);

            auto icon = QIcon::fromTheme(QMimeDatabase().mimeTypeForFile(entry.fileName).iconName());
            if (icon.isNull()) {
                icon = QIcon::fromTheme(QStringLiteral("text-plain"));
            }

            temp.append({entry.text, entry.fileName, icon});
        }

        beginResetModel();
        m_modelEntries = std::move(temp);
        endResetModel();
    }

    void clear()
    {
        beginResetModel();
        QVector<ClipboardEntry>().swap(m_modelEntries);
        endResetModel();
    }

private:
    struct ClipboardEntry {
        QString text;
        QString fileName;
        QIcon icon;
    };

    QVector<ClipboardEntry> m_modelEntries;
};

class SingleLineDelegate : public QStyledItemDelegate
{
    Q_OBJECT

public:
    explicit SingleLineDelegate(const QFont &font)
        : QStyledItemDelegate(nullptr)
        , m_font(font)
        , m_newLineRegExp(QStringLiteral("\\n|\\r|\u2028"), QRegularExpression::UseUnicodePropertiesOption)
    {
    }

    void initStyleOption(QStyleOptionViewItem *option, const QModelIndex &index) const override
    {
        QStyledItemDelegate::initStyleOption(option, index);
        option->font = m_font;
    }

    QString displayText(const QVariant &value, const QLocale &locale) const override
    {
        QString baseText = QStyledItemDelegate::displayText(value, locale);
        baseText = baseText.trimmed();
        auto endOfLine = baseText.indexOf(m_newLineRegExp, 0);
        if (endOfLine != -1) {
            baseText.truncate(endOfLine);
        }

        return baseText;
    }

private:
    QFont m_font;
    QRegularExpression m_newLineRegExp;
};

ClipboardHistoryDialog::ClipboardHistoryDialog(QWidget *window, KTextEditor::ViewPrivate *viewPrivate)
    : QuickDialog(nullptr, window)
    , m_viewPrivate(viewPrivate)
    , m_model(new ClipboardHistoryModel(this))
    , m_proxyModel(new QSortFilterProxyModel(this))
    , m_selectedDoc(new KTextEditor::DocumentPrivate)
{
    m_proxyModel->setSourceModel(m_model);
    m_proxyModel->setFilterCaseSensitivity(Qt::CaseInsensitive);

    const QFont font = viewPrivate->renderer()->config()->baseFont();

    m_treeView.setModel(m_proxyModel);
    m_treeView.setItemDelegate(new SingleLineDelegate(font));
    m_treeView.setTextElideMode(Qt::ElideRight);

    m_selectedDoc->setParent(this);
    m_selectedView = new KTextEditor::ViewPrivate(m_selectedDoc, this);
    m_selectedView->setStatusBarEnabled(false);
    m_selectedView->setLineNumbersOn(false);
    m_selectedView->setFoldingMarkersOn(false);
    m_selectedView->setIconBorder(false);
    m_selectedView->setScrollBarMarks(false);
    m_selectedView->setScrollBarMiniMap(false);

    auto *d_layout = static_cast<QVBoxLayout *>(layout());
    d_layout->setStretchFactor(&m_treeView, 2);
    d_layout->addWidget(m_selectedView, 3);

    m_lineEdit.setFont(font);

    connect(m_treeView.selectionModel(), &QItemSelectionModel::currentRowChanged, this, [this](const QModelIndex &current, const QModelIndex &previous) {
        Q_UNUSED(previous);
        showSelectedText(current);
    });

    connect(&m_lineEdit, &QLineEdit::textChanged, this, [this](const QString &s) {
        m_proxyModel->setFilterFixedString(s);


        const auto bestMatch = m_proxyModel->index(0, 0);
        m_treeView.setCurrentIndex(bestMatch);
        showSelectedText(bestMatch);
    });
}

void ClipboardHistoryDialog::showSelectedText(const QModelIndex &idx)
{
    QString text = m_proxyModel->data(idx, Qt::DisplayRole).toString();
    if (m_selectedDoc->text().isEmpty() || text != m_selectedDoc->text()) {
        QString fileName = m_proxyModel->data(idx, ClipboardHistoryModel::Role::HighlightingRole).toString();
        m_selectedDoc->setReadWrite(true);
        m_selectedDoc->setText(text);
        m_selectedDoc->setReadWrite(false);
        const auto mode = KTextEditor::Editor::instance()->repository().definitionForFileName(fileName).name();
        m_selectedDoc->setHighlightingMode(mode);
    }
}

void ClipboardHistoryDialog::resetValues()
{
    m_lineEdit.setPlaceholderText(i18n("Select text to paste."));
}

void ClipboardHistoryDialog::openDialog(const QVector<KTextEditor::EditorPrivate::ClipboardEntry> &clipboardHistory)
{
    m_model->refresh(clipboardHistory);
    resetValues();

    const auto first = m_proxyModel->index(0, 0);
    m_treeView.setCurrentIndex(first);
    showSelectedText(first);

    exec();
}

void ClipboardHistoryDialog::slotReturnPressed()
{
    const QString text = m_proxyModel->data(m_treeView.currentIndex(), Qt::DisplayRole).toString();
    m_viewPrivate->paste(&text);

    clearLineEdit();
    hide();
}

#include "clipboardhistorydialog.moc"
