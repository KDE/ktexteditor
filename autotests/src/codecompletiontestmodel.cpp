/*
    This file is part of the KDE project
    SPDX-FileCopyrightText: 2005 Hamish Rodda <rodda@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "codecompletiontestmodel.h"

#include <kateglobal.h>
#include <katewordcompletion.h>
#include <ktexteditor/codecompletioninterface.h>
#include <ktexteditor/document.h>
#include <ktexteditor/view.h>

CodeCompletionTestModel::CodeCompletionTestModel(KTextEditor::View *parent, const QString &startText)
    : KTextEditor::CodeCompletionModel(parent)
    , m_startText(startText)
    , m_autoStartText(m_startText.isEmpty())
{
    setRowCount(40);

    Q_ASSERT(cc());

    cc()->setAutomaticInvocationEnabled(true);
    cc()->unregisterCompletionModel(KTextEditor::EditorPrivate::self()->wordCompletionModel()); // would add additional items, we don't want that in tests
    cc()->registerCompletionModel(this);
}

// Fake a series of completions
QVariant CodeCompletionTestModel::data(const QModelIndex &index, int role) const
{
    switch (role) {
    case Qt::DisplayRole:
        if (index.row() < rowCount() / 2) {
            switch (index.column()) {
            case Prefix:
                switch (index.row() % 3) {
                default:
                    return "void ";
                case 1:
                    return "const QString& ";
                case 2:
                    if (index.row() % 6) {
                        return "inline virtual bool ";
                    }
                    return "virtual bool ";
                }

            case Scope:
                switch (index.row() % 4) {
                default:
                    return QString();
                case 1:
                    return "KTextEditor::";
                case 2:
                    return "::";
                case 3:
                    return "std::";
                }

            case Name:
                return QString(m_startText + QString("%1%2%3").arg(QChar('a' + (index.row() % 3))).arg(QChar('a' + index.row())).arg(index.row()));

            case Arguments:
                switch (index.row() % 5) {
                default:
                    return "()";
                case 1:
                    return "(bool trigger)";
                case 4:
                    return "(const QString& name, Qt::CaseSensitivity cs)";
                case 5:
                    return "(int count)";
                }

            case Postfix:
                switch (index.row() % 3) {
                default:
                    return " const";
                case 1:
                    return " KDE_DEPRECATED";
                case 2:
                    return "";
                }
            }
        } else {
            switch (index.column()) {
            case Prefix:
                switch (index.row() % 3) {
                default:
                    return "void ";
                case 1:
                    return "const QString ";
                case 2:
                    return "bool ";
                }

            case Scope:
                switch (index.row() % 4) {
                default:
                    return QString();
                case 1:
                    return "KTextEditor::";
                case 2:
                    return "::";
                case 3:
                    return "std::";
                }

            case Name:
                return QString(m_startText + QString("%1%2%3").arg(QChar('a' + (index.row() % 3))).arg(QChar('a' + index.row())).arg(index.row()));

            default:
                return "";
            }
        }
        break;

    case Qt::DecorationRole:
        break;

    case CompletionRole: {
        CompletionProperties p;
        if (index.row() < rowCount() / 2) {
            p |= Function;
        } else {
            p |= Variable;
        }
        switch (index.row() % 3) {
        case 0:
            p |= Const | Public;
            break;
        case 1:
            p |= Protected;
            break;
        case 2:
            p |= Private;
            break;
        }
        return (int)p;
    }

    case ScopeIndex:
        return (index.row() % 4) - 1;
    }

    return QVariant();
}

KTextEditor::View *CodeCompletionTestModel::view() const
{
    return static_cast<KTextEditor::View *>(const_cast<QObject *>(QObject::parent()));
}

KTextEditor::CodeCompletionInterface *CodeCompletionTestModel::cc() const
{
    return dynamic_cast<KTextEditor::CodeCompletionInterface *>(const_cast<QObject *>(QObject::parent()));
}

void CodeCompletionTestModel::completionInvoked(KTextEditor::View *view, const KTextEditor::Range &range, InvocationType invocationType)
{
    Q_UNUSED(invocationType)

    if (m_autoStartText) {
        m_startText = view->document()->text(KTextEditor::Range(range.start(), view->cursorPosition()));
    }
    qDebug() << m_startText;
}

AbbreviationCodeCompletionTestModel::AbbreviationCodeCompletionTestModel(KTextEditor::View *parent, const QString &startText)
    : CodeCompletionTestModel(parent, startText)
{
    m_items << "SomeCoolAbbreviation"
            << "someCoolAbbreviation"
            << "sca"
            << "SCA";
    m_items << "some_cool_abbreviation"
            << "Some_Cool_Abbreviation";
    m_items << "thisContainsSomeWord"
            << "this_contains_some_word"
            << "thiscontainssomeword";
    m_items << "notmatchedbecausemissingcaps"
            << "not_m_atch_ed_because_underscores";
    setRowCount(m_items.size());
}

QVariant AbbreviationCodeCompletionTestModel::data(const QModelIndex &index, int role) const
{
    if (index.column() == Name && role == Qt::DisplayRole) {
        return m_items[index.row()];
    }
    return QVariant();
}

AsyncCodeCompletionTestModel::AsyncCodeCompletionTestModel(KTextEditor::View *parent, const QString &startText)
    : CodeCompletionTestModel(parent, startText)
{
    setRowCount(0);
}

QVariant AsyncCodeCompletionTestModel::data(const QModelIndex &index, int role) const
{
    if (index.column() == Name && role == Qt::DisplayRole) {
        return m_items[index.row()];
    }
    return QVariant();
}

void AsyncCodeCompletionTestModel::setItems(const QStringList &items)
{
    beginResetModel();
    m_items = items;
    setRowCount(m_items.size());
    endResetModel();
}

void AsyncCodeCompletionTestModel::completionInvoked(KTextEditor::View *view, const KTextEditor::Range &range, InvocationType invocationType)
{
    Q_UNUSED(invocationType)

    Q_EMIT waitForReset();
    CodeCompletionTestModel::completionInvoked(view, range, invocationType);
}
