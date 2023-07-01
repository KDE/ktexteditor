/*
    This file is part of the KDE project
    SPDX-FileCopyrightText: 2001, 2003 Peter Kelly <pmk@post.com>
    SPDX-FileCopyrightText: 2003, 2004 Stephan Kulow <coolo@kde.org>
    SPDX-FileCopyrightText: 2004 Dirk Mueller <mueller@kde.org>
    SPDX-FileCopyrightText: 2006, 2007 Leo Savernik <l.savernik@aon.at>
    SPDX-FileCopyrightText: 2010 Milian Wolff <mail@milianw.de>
    SPDX-FileCopyrightText: 2013 Gerald Senarclens de Grancy <oss@senarclens.eu>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

// BEGIN Includes
#include "indenttest.h"

#include "kateconfig.h"
#include "katedocument.h"

#include <QtTestWidgets>

#include "testutils.h"

QTEST_MAIN(IndentTest)

#define FAILURE(test, comment) qMakePair<const char *, const char *>((test), (comment))

void IndentTest::initTestCase()
{
    ScriptTestBase::initTestCase();
    m_document->config()->setValue(KateDocumentConfig::IndentOnTextPaste, true);
    m_section = "indent";
    m_script_dir = "indentation";
}

void IndentTest::testCstyle_data()
{
    getTestData("cstyle");
}

void IndentTest::testCstyle()
{
    runTest(
        ExpectedFailures() << FAILURE("using1", "this is insane, those who write such code can cope with it :P")
                           << FAILURE("using2", "this is insane, those who write such code can cope with it :P")
                           << FAILURE("plist14",
                                      "in function signatures it might be wanted to use the indentation of the\n"
                                      "opening paren instead of just increasing the indentation level like in function calls")
                           << FAILURE("switch10", "test for case where cfgSwitchIndent = false; needs proper config-interface")
                           << FAILURE("switch11", "test for case where cfgSwitchIndent = false; needs proper config-interface")
                           << FAILURE("visib2", "test for access modifier where cfgAccessModifiers = 1;needs proper config interface")
                           << FAILURE("visib3", "test for access modifier where cfgAccessModifiers = 1;needs proper config interface")
                           << FAILURE("plist10", "low low prio, maybe wontfix: if the user wants to add a arg, he should do so and press enter afterwards")
                           << FAILURE("switch13", "pure insanity, whoever wrote this test and expects that to be indented properly should stop writing code"));
}

void IndentTest::testCppstyle_data()
{
    getTestData("cppstyle");
}

void IndentTest::testCppstyle()
{
    runTest(ExpectedFailures()
            /// \todo Fix (smth) to make failed test cases really work!
            << FAILURE("parens1", "dunno why it failed in test! in manual mode everything works fine..."));
}

void IndentTest::testCMake_data()
{
    getTestData("cmake");
}

void IndentTest::testCMake()
{
    runTest(ExpectedFailures());
}

void IndentTest::testPython_data()
{
    getTestData("python");
}

void IndentTest::testPython()
{
    runTest(ExpectedFailures());
}

void IndentTest::testJulia_data()
{
    getTestData("julia");
}

void IndentTest::testJulia()
{
    runTest(ExpectedFailures());
}

void IndentTest::testHaskell_data()
{
    getTestData("haskell");
}

void IndentTest::testHaskell()
{
    runTest(ExpectedFailures());
}

void IndentTest::testLatex_data()
{
    getTestData("latex");
}

void IndentTest::testLatex()
{
    runTest(ExpectedFailures());
}

void IndentTest::testPascal_data()
{
    getTestData("pascal");
}

void IndentTest::testPascal()
{
    runTest(ExpectedFailures());
}

void IndentTest::testAda_data()
{
    getTestData("ada");
}

void IndentTest::testAda()
{
    runTest(ExpectedFailures());
}

void IndentTest::testRuby_data()
{
    getTestData("ruby");
}

void IndentTest::testRuby()
{
    runTest(ExpectedFailures() << FAILURE("block01", "Multiline blocks using {} is not supported")
                               << FAILURE("block02", "Multiline blocks using {} is not supported")
                               << FAILURE("singleline01", "Single line defs are not supported") << FAILURE("singleline02", "Single line defs are not supported")
                               << FAILURE("wordlist01", "multiline word list is not supported") << FAILURE("wordlist02", "multiline word list is not supported")
                               << FAILURE("wordlist11", "multiline word list is not supported") << FAILURE("wordlist12", "multiline word list is not supported")
                               << FAILURE("wordlist21", "multiline word list is not supported") << FAILURE("wordlist22", "multiline word list is not supported")
                               << FAILURE("if20", "multi line if assignment is not supported") << FAILURE("if21", "multi line if assignment is not supported")
                               << FAILURE("if30", "single line if is not supported") << FAILURE("if31", "single line if is not supported")
                               << FAILURE("regexp1", "regression, inside already in commit afc551d14225023ce38900ddc49b43ba2a0762a8"));
}

void IndentTest::testXml_data()
{
    getTestData("xml");
}

void IndentTest::testXml()
{
    runTest(ExpectedFailures());
}

void IndentTest::testNormal_data()
{
    getTestData("normal");
}

void IndentTest::testNormal()
{
    runTest(ExpectedFailures());
}

void IndentTest::testReplicode_data()
{
    getTestData("replicode");
}

void IndentTest::testReplicode()
{
    runTest(ExpectedFailures());
}

void IndentTest::testR_data()
{
    getTestData("R");
}

void IndentTest::testR()
{
    runTest(ExpectedFailures());
}

#include "moc_indenttest.cpp"
