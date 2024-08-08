/*
    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef SCRIPT_TEST_H
#define SCRIPT_TEST_H

#include <QObject>
#include <QPair>
#include <QStringList>

class TestScriptEnv;
namespace KTextEditor
{
class DocumentPrivate;
}
namespace KTextEditor
{
class ViewPrivate;
}
class QMainWindow;

class ScriptTestBase : public QObject
{
    Q_OBJECT

protected:
    void initTestCase();
    void cleanupTestCase();
    typedef QPair<const char *, const char *> Failure;
    typedef QList<Failure> ExpectedFailures;
    void getTestData(const QString &script);
    void runTest(const ExpectedFailures &failures);

    TestScriptEnv *m_env;
    KTextEditor::DocumentPrivate *m_document;
    QMainWindow *m_toplevel;
    bool m_outputWasCustomised;
    QStringList m_commands;
    KTextEditor::ViewPrivate *m_view;
    QString m_section; // dir name in testdata/
    QString m_script_dir; // dir name in part/script/data/

public:
    static QtMessageHandler m_msgHandler;
};

#endif // SCRIPT_TEST_H
