/*
 * This file is part of the KDE libraries
 *
 * Copyright (C) 2014 Miquel Sabaté Solà <mikisabate@gmail.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 * You should have received a copy of the GNU Library General Public License
 * along with this library; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */


#ifndef BASE_TEST_H
#define BASE_TEST_H


#include <QMainWindow>
#include <QtTestWidgets>
#include <QVBoxLayout>
#include <katedocument.h>
#include <kateview.h>
#include <inputmodemanager.h>

class QLineEdit;
namespace KateVi { class EmulatedCommandBar; }

/* Syntactic sugar :P */
#define DoTest(...) DoTest_(__LINE__, __VA_ARGS__)
#define FinishTest(...) FinishTest_( __LINE__, __VA_ARGS__ )


/// Helper class that represents a change in a document.
class DocChange
{
public:
    enum ChangeType { TextRemoved, TextInserted };

    DocChange(ChangeType changeType, KTextEditor::Range changeRange, QString newText = QString())
        : m_changeType(changeType), m_changeRange(changeRange), m_newText(newText)
    {
        /* There's nothing to do here. */
    }

    ChangeType changeType() const
    {
        return m_changeType;
    }

    KTextEditor::Range changeRange() const
    {
        return m_changeRange;
    }

    QString newText() const
    {
        return m_newText;
    }

private:
    ChangeType m_changeType;
    KTextEditor::Range m_changeRange;
    QString m_newText;
};


class BaseTest : public QObject
{
    Q_OBJECT

public:
    BaseTest();
    ~BaseTest();

    static void waitForCompletionWidgetToActivate(KTextEditor::ViewPrivate *kate_view);

protected:
    // Begin/Do/Finish test.

    enum Expectation { ShouldPass, ShouldFail };

    void TestPressKey(const QString &str);
    void BeginTest(const QString &original);
    void FinishTest_(int line, const QString &expected, Expectation expectation = ShouldPass,
                    const QString &failureReason = QString());
    void DoTest_(int line, const QString &original, const QString &command,
                 const QString &expected, Expectation expectation = ShouldPass,
                 const QString &failureReason = QString());

    // Parsing keys.

    Qt::Key parseCodedSpecialKey(const QString &string, int startPos, int *destEndOfCodedKey);
    Qt::KeyboardModifier parseCodedModifier(const QString &string, int startPos, int *destEndOfCodedModifier);

    // Emulated command bar.

    KateVi::EmulatedCommandBar * emulatedCommandBar();
    QLineEdit * emulatedCommandBarTextEdit();

    // Macros & mappings.

    void clearAllMappings();
    void clearAllMacros();

    // Other auxiliar functions.

    void ensureKateViewVisible();

protected:
    KTextEditor::DocumentPrivate *kate_document;
    KTextEditor::ViewPrivate *kate_view;
    KateViInputMode *vi_input_mode;
    KateVi::GlobalState *vi_global;
    KateVi::InputModeManager *vi_input_mode_manager;

    bool m_firstBatchOfKeypressesForTest;

    QMainWindow *mainWindow;
    QVBoxLayout *mainWindowLayout;

    QMap<QString, Qt::Key> m_codesToSpecialKeys;
    QMap<QString, Qt::KeyboardModifier> m_codesToModifiers;

    QList<DocChange> m_docChanges;

protected Q_SLOTS:
    void init();

private Q_SLOTS:
    void textInserted(KTextEditor::Document *document, KTextEditor::Range range);
    void textRemoved(KTextEditor::Document *document, KTextEditor::Range range);
};


#endif /* BASE_TEST_H */
