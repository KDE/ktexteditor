/*
    SPDX-FileCopyrightText: 2021 Igor Kushnir <igorkuo@gmail.com>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "katemodemanager_test_base.h"

#include <kateglobal.h>

#include <QString>
#include <QTest>

namespace
{
struct FileTypeDataRow {
    const char *dataTag;
    const char *m_inputString;
    const char *m_fileTypeName;

    QString inputString() const
    {
        return QString::fromUtf8(m_inputString);
    }
    QString fileTypeName() const
    {
        return QString::fromUtf8(m_fileTypeName);
    }
};
// The two tables below have been copied from syntax-highlighting:autotests/repository_test_base.cpp and adjusted:
// removed all except first definition/fileType names because only the first name is used in KTextEditor.
// The two versions of the tables should be kept in sync.

// Additional adjustments to the syntax-highlighting version of fileTypesForFileNames table:
//  1) used the second, not first, definition/fileType name in the QRPG.ninja row because
//     "ILERPG" < "Ninja" in KSyntaxHighlighting but "Sources/ILERPG" > "Other/Ninja" in KTextEditor.
//  2) used the second, not first, definition/fileType name in the qrpg*.tt row because
//     "ILERPG" < "TT2" in KSyntaxHighlighting but "Sources/ILERPG" > "Markup/TT2" in KTextEditor.
constexpr FileTypeDataRow fileTypesForFileNames[] = {
    {"empty", "", ""},

    {"*.tar.gz", "noMatch.tar.gz", ""},
    {"No match", "a_random#filename", ""},
    {"Long path, no match", "/this/is/a/moderately/long/path/to/no-match", ""},
    {"Prefix in dir name", "Kconfig/no_match", ""},

    {"*.qml", "/bla/foo.qml", "QML"},
    {"*.frag", "flat.frag", "GLSL"},
    {"*.md", "highPriority.md", "Markdown"},
    {"*.octave", "lowPriority.octave", "Octave"},
    {"*.hats", "sameLastLetterPattern.hats", "ATS"},

    {"*.c", "test.c", "C"},
    {"*.fs", "test.fs", "FSharp"},
    {"*.m", "/bla/foo.m", "Objective-C"},

    {"Makefile", "Makefile", "Makefile"},
    {"Path to Makefile", "/some/path/to/Makefile", "Makefile"},
    {"Makefile.*", "Makefile.am", "Makefile"},

    {"not-Makefile.dic", "not-Makefile.dic", "Hunspell Dictionary File"},
    {"*qmakefile.cpp", "test_qmakefile.cpp", "C++"},
    {"*_makefile.mm", "bench_makefile.mm", "Objective-C++"},

    {"xorg.conf", "/etc/literal-pattern/xorg.conf", "x.org Configuration"},
    {".profile", "2-literal-patterns/.profile", "Bash"},

    {"Config.*", "Config.beginning", "Kconfig"},
    {"usr.libexec.*", "usr.libexec.", "AppArmor Security Profile"},
    {"Jam*", "Jam-beginning-no-dot", "Jam"},
    {"usr.li-*.ch", "usr.li-many-partial-prefix-matches.ch", "xHarbour"},
    {"QRPG*.*", "QRPG1u4[+.unusual", "ILERPG"},

    {"*patch", "no-dot-before-ending~patch", "Diff"},
    {"*.cmake.in", "two-dots-after-asterisk.cmake.in", "CMake"},
    {"*.html.mst", "two-dots-priority!=0.html.mst", "Mustache/Handlebars (HTML)"},

    {"*.desktop.cmake", "2_suffixes.desktop.cmake", ".desktop"},
    {"*.per.err", "2_suffixes-but-one-a-better-match.per.err", "4GL"},
    {"*.xml.eex", "2_suffixes-one-lang.xml.eex", "Elixir"},
    {"fishd.*.fish", "fishd.prefix,suffix=one-lang.fish", "Fish"},

    {"usr.bin.*.ftl", "usr.bin.heterogenousPatternMatch.ftl", "AppArmor Security Profile"},
    {"Doxyfile.*.pro", "Doxyfile.heterogenous.Pattern-Match.pro", "QMake"},
    {"Kconfig*.ml", "KconfigHeterogenous_pattern_match.ml", "Objective Caml"},
    {"snap-confine.*.html.rac", "snap-confine.2.-higher-priority.html.rac", "Mustache/Handlebars (HTML)"},
    {"file_contexts_*.fq.gz", "file_contexts_prefix-higher-priority.fq.gz", "SELinux File Contexts"},
    {"QRPG*.ninja", "QRPG.ninja", "Ninja"},
    {"qrpg*.tt", "qrpgTwoUnusualPatterns.tt", "TT2"},
    {"qrpg*.cl", "qrpg$heterogenous~pattern&match.cl", "OpenCL"},
    {".gitignore*.tt*.textile", ".gitignoreHeterogenous3.tt.textile", "Textile"},
};

constexpr FileTypeDataRow fileTypesForMimeTypeNames[] = {
    {"empty", "", ""},

    {"Nonexistent MIME type", "text/nonexistent-mt", ""},
    {"No match", "application/x-bzip-compressed-tar", ""},

    {"High priority", "text/rust", "Rust"},
    {"Negative priority", "text/octave", "Octave"},

    {"Multiple types match", "text/x-chdr", "C++"},
};

template<std::size_t size>
void addFileTypeDataRows(const FileTypeDataRow (&array)[size])
{
    for (const auto &row : array) {
        QTest::newRow(row.dataTag) << row.inputString() << row.fileTypeName();
    }
}
} // unnamed namespace

KateModeManagerTestBase::KateModeManagerTestBase()
    : m_modeManager{KTextEditor::EditorPrivate::self()->modeManager()}
{
}

void KateModeManagerTestBase::wildcardsFindTestData()
{
    QTest::addColumn<QString>("fileName");
    QTest::addColumn<QString>("fileTypeName");
    addFileTypeDataRows(fileTypesForFileNames);
}

void KateModeManagerTestBase::mimeTypesFindTestData()
{
    QTest::addColumn<QString>("mimeTypeName");
    QTest::addColumn<QString>("fileTypeName");
    addFileTypeDataRows(fileTypesForMimeTypeNames);
}
