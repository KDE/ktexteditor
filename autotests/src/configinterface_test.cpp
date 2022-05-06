/*
    This file is part of the KDE libraries
    SPDX-FileCopyrightText: 2017 Dominik Haumann <dhaumann@kde.de>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "configinterface_test.h"
#include "moc_configinterface_test.cpp"

#include <katedocument.h>
#include <kateglobal.h>
#include <kateview.h>
#include <ktexteditor/configinterface.h>
#include <katesyntaxmanager.h>
#include <KSyntaxHighlighting/Repository>
#include <KSyntaxHighlighting/Theme>

#include <QFont>
#include <QTest>

using namespace KTextEditor;

QTEST_MAIN(KateConfigInterfaceTest)

KateConfigInterfaceTest::KateConfigInterfaceTest()
    : QObject()
{
    KTextEditor::EditorPrivate::enableUnitTestMode();
}

KateConfigInterfaceTest::~KateConfigInterfaceTest()
{
}

void KateConfigInterfaceTest::testDocument()
{
    KTextEditor::DocumentPrivate doc(false, false);
    auto iface = qobject_cast<KTextEditor::ConfigInterface *>(&doc);
    QVERIFY(iface);
    QVERIFY(!iface->configKeys().isEmpty());

    iface->setConfigValue(QLatin1String("backup-on-save-local"), true);
    QCOMPARE(iface->configValue(QLatin1String("backup-on-save-local")).toBool(), true);
    iface->setConfigValue(QLatin1String("backup-on-save-local"), false);
    QCOMPARE(iface->configValue(QLatin1String("backup-on-save-local")).toBool(), false);

    iface->setConfigValue(QLatin1String("backup-on-save-remote"), true);
    QCOMPARE(iface->configValue(QLatin1String("backup-on-save-remote")).toBool(), true);
    iface->setConfigValue(QLatin1String("backup-on-save-remote"), false);
    QCOMPARE(iface->configValue(QLatin1String("backup-on-save-remote")).toBool(), false);

    iface->setConfigValue(QLatin1String("replace-tabs"), true);
    QCOMPARE(iface->configValue(QLatin1String("replace-tabs")).toBool(), true);
    iface->setConfigValue(QLatin1String("replace-tabs"), false);
    QCOMPARE(iface->configValue(QLatin1String("replace-tabs")).toBool(), false);

    iface->setConfigValue(QLatin1String("indent-pasted-text"), true);
    QCOMPARE(iface->configValue(QLatin1String("indent-pasted-text")).toBool(), true);
    iface->setConfigValue(QLatin1String("indent-pasted-text"), false);
    QCOMPARE(iface->configValue(QLatin1String("indent-pasted-text")).toBool(), false);

    iface->setConfigValue(QLatin1String("on-the-fly-spellcheck"), true);
    QCOMPARE(iface->configValue(QLatin1String("on-the-fly-spellcheck")).toBool(), true);
    iface->setConfigValue(QLatin1String("on-the-fly-spellcheck"), false);
    QCOMPARE(iface->configValue(QLatin1String("on-the-fly-spellcheck")).toBool(), false);

    iface->setConfigValue(QLatin1String("indent-width"), 13);
    QCOMPARE(iface->configValue(QLatin1String("indent-width")).toInt(), 13);
    iface->setConfigValue(QLatin1String("indent-width"), 4);
    QCOMPARE(iface->configValue(QLatin1String("indent-width")).toInt(), 4);

    iface->setConfigValue(QLatin1String("tab-width"), 13);
    QCOMPARE(iface->configValue(QLatin1String("tab-width")).toInt(), 13);
    iface->setConfigValue(QLatin1String("tab-width"), 4);
    QCOMPARE(iface->configValue(QLatin1String("tab-width")).toInt(), 4);

    iface->setConfigValue(QLatin1String("backup-on-save-suffix"), QLatin1String("_tmp"));
    QCOMPARE(iface->configValue(QLatin1String("backup-on-save-suffix")).toString(), QLatin1String("_tmp"));

    iface->setConfigValue(QLatin1String("backup-on-save-prefix"), QLatin1String("abc_"));
    QCOMPARE(iface->configValue(QLatin1String("backup-on-save-prefix")).toString(), QLatin1String("abc_"));
}

void KateConfigInterfaceTest::testView()
{
    KTextEditor::DocumentPrivate doc(false, false);
    auto view = static_cast<KTextEditor::View *>(doc.createView(nullptr));
    QVERIFY(view);
    auto iface = qobject_cast<KTextEditor::ConfigInterface *>(view);
    QVERIFY(iface);
    QVERIFY(!iface->configKeys().isEmpty());

    iface->setConfigValue(QLatin1String("line-numbers"), true);
    QCOMPARE(iface->configValue(QLatin1String("line-numbers")).toBool(), true);
    iface->setConfigValue(QLatin1String("line-numbers"), false);
    QCOMPARE(iface->configValue(QLatin1String("line-numbers")).toBool(), false);

    iface->setConfigValue(QLatin1String("icon-bar"), true);
    QCOMPARE(iface->configValue(QLatin1String("icon-bar")).toBool(), true);
    iface->setConfigValue(QLatin1String("icon-bar"), false);
    QCOMPARE(iface->configValue(QLatin1String("icon-bar")).toBool(), false);

    iface->setConfigValue(QLatin1String("folding-bar"), true);
    QCOMPARE(iface->configValue(QLatin1String("folding-bar")).toBool(), true);
    iface->setConfigValue(QLatin1String("folding-bar"), false);
    QCOMPARE(iface->configValue(QLatin1String("folding-bar")).toBool(), false);

    iface->setConfigValue(QLatin1String("folding-preview"), true);
    QCOMPARE(iface->configValue(QLatin1String("folding-preview")).toBool(), true);
    iface->setConfigValue(QLatin1String("folding-preview"), false);
    QCOMPARE(iface->configValue(QLatin1String("folding-preview")).toBool(), false);

    iface->setConfigValue(QLatin1String("dynamic-word-wrap"), true);
    QCOMPARE(iface->configValue(QLatin1String("dynamic-word-wrap")).toBool(), true);
    iface->setConfigValue(QLatin1String("dynamic-word-wrap"), false);
    QCOMPARE(iface->configValue(QLatin1String("dynamic-word-wrap")).toBool(), false);

    iface->setConfigValue(QLatin1String("background-color"), QColor(0, 255, 0));
    QCOMPARE(iface->configValue(QLatin1String("background-color")).value<QColor>(), QColor(0, 255, 0));

    iface->setConfigValue(QLatin1String("selection-color"), QColor(0, 255, 0));
    QCOMPARE(iface->configValue(QLatin1String("selection-color")).value<QColor>(), QColor(0, 255, 0));

    iface->setConfigValue(QLatin1String("search-highlight-color"), QColor(0, 255, 0));
    QCOMPARE(iface->configValue(QLatin1String("search-highlight-color")).value<QColor>(), QColor(0, 255, 0));

    iface->setConfigValue(QLatin1String("replace-highlight-color"), QColor(0, 255, 0));
    QCOMPARE(iface->configValue(QLatin1String("replace-highlight-color")).value<QColor>(), QColor(0, 255, 0));

    iface->setConfigValue(QLatin1String("default-mark-type"), 6);
    QCOMPARE(iface->configValue(QLatin1String("default-mark-type")).toInt(), 6);

    iface->setConfigValue(QLatin1String("allow-mark-menu"), true);
    QCOMPARE(iface->configValue(QLatin1String("allow-mark-menu")).toBool(), true);
    iface->setConfigValue(QLatin1String("allow-mark-menu"), false);
    QCOMPARE(iface->configValue(QLatin1String("allow-mark-menu")).toBool(), false);

    iface->setConfigValue(QLatin1String("icon-border-color"), QColor(0, 255, 0));
    QCOMPARE(iface->configValue(QLatin1String("icon-border-color")).value<QColor>(), QColor(0, 255, 0));

    iface->setConfigValue(QLatin1String("folding-marker-color"), QColor(0, 255, 0));
    QCOMPARE(iface->configValue(QLatin1String("folding-marker-color")).value<QColor>(), QColor(0, 255, 0));

    iface->setConfigValue(QLatin1String("line-number-color"), QColor(0, 255, 0));
    QCOMPARE(iface->configValue(QLatin1String("line-number-color")).value<QColor>(), QColor(0, 255, 0));

    iface->setConfigValue(QLatin1String("current-line-number-color"), QColor(0, 255, 0));
    QCOMPARE(iface->configValue(QLatin1String("current-line-number-color")).value<QColor>(), QColor(0, 255, 0));

    iface->setConfigValue(QLatin1String("modification-markers"), true);
    QCOMPARE(iface->configValue(QLatin1String("modification-markers")).toBool(), true);
    iface->setConfigValue(QLatin1String("modification-markers"), false);
    QCOMPARE(iface->configValue(QLatin1String("modification-markers")).toBool(), false);

    iface->setConfigValue(QLatin1String("word-count"), true);
    QCOMPARE(iface->configValue(QLatin1String("word-count")).toBool(), true);
    iface->setConfigValue(QLatin1String("word-count"), false);
    QCOMPARE(iface->configValue(QLatin1String("word-count")).toBool(), false);

    iface->setConfigValue(QLatin1String("line-count"), true);
    QCOMPARE(iface->configValue(QLatin1String("line-count")).toBool(), true);
    iface->setConfigValue(QLatin1String("line-count"), false);
    QCOMPARE(iface->configValue(QLatin1String("line-count")).toBool(), false);

    iface->setConfigValue(QLatin1String("scrollbar-minimap"), true);
    QCOMPARE(iface->configValue(QLatin1String("scrollbar-minimap")).toBool(), true);
    iface->setConfigValue(QLatin1String("scrollbar-minimap"), false);
    QCOMPARE(iface->configValue(QLatin1String("scrollbar-minimap")).toBool(), false);

    iface->setConfigValue(QLatin1String("scrollbar-preview"), true);
    QCOMPARE(iface->configValue(QLatin1String("scrollbar-preview")).toBool(), true);
    iface->setConfigValue(QLatin1String("scrollbar-preview"), false);
    QCOMPARE(iface->configValue(QLatin1String("scrollbar-preview")).toBool(), false);

    iface->setConfigValue(QLatin1String("font"), QFont("Times", 10, QFont::Bold));
    QCOMPARE(iface->configValue(QLatin1String("font")).value<QFont>(), QFont("Times", 10, QFont::Bold));

    {
        KSyntaxHighlighting::Repository& repository = KTextEditor::EditorPrivate::self()->hlManager()->repository();

        auto lightTheme = repository.defaultTheme(KSyntaxHighlighting::Repository::LightTheme);
        iface->setConfigValue(QLatin1String("theme"), lightTheme.name());
        QCOMPARE(iface->configValue(QLatin1String("theme")).value<QString>(), lightTheme.name());

        auto darkTheme = repository.defaultTheme(KSyntaxHighlighting::Repository::DarkTheme);
        iface->setConfigValue(QLatin1String("theme"), darkTheme.name());
        QCOMPARE(iface->configValue(QLatin1String("theme")).value<QString>(), darkTheme.name());
    }
}

// kate: indent-mode cstyle; indent-width 4; replace-tabs on;
