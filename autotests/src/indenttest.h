/*
    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef INDENTTEST_H
#define INDENTTEST_H

#include <QObject>

#include "script_test_base.h"

class IndentTest : public ScriptTestBase
{
    Q_OBJECT
private Q_SLOTS:
    void initTestCase();

    void testPython_data();
    void testPython();

    void testJulia_data();
    void testJulia();

    void testCstyle_data();
    void testCstyle();

    void testCppstyle_data();
    void testCppstyle();

    void testCMake_data();
    void testCMake();

    void testRuby_data();
    void testRuby();

    void testHaskell_data();
    void testHaskell();

    void testLatex_data();
    void testLatex();

    void testPascal_data();
    void testPascal();

    void testAda_data();
    void testAda();

    void testXml_data();
    void testXml();

    void testNormal_data();
    void testNormal();

    void testReplicode_data();
    void testReplicode();

    void testR_data();
    void testR();
};

#endif // INDENTTEST_H
