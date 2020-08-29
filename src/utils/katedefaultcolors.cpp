/*
    SPDX-FileCopyrightText: 2014 Milian Wolff <mail@milianw.de>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "katedefaultcolors.h"
#include "kateglobal.h"

#include <KColorUtils>

using namespace Kate;

KateDefaultColors::KateDefaultColors()
    // for unit testing: avoid global configs!
    : m_view(QPalette::Active,
             KColorScheme::View,
             KSharedConfig::openConfig(KTextEditor::EditorPrivate::unitTestMode() ? QStringLiteral("unittestmoderc") : QString(), KTextEditor::EditorPrivate::unitTestMode() ? KConfig::SimpleConfig : KConfig::FullConfig))
    , m_window(QPalette::Active,
               KColorScheme::Window,
               KSharedConfig::openConfig(KTextEditor::EditorPrivate::unitTestMode() ? QStringLiteral("unittestmoderc") : QString(), KTextEditor::EditorPrivate::unitTestMode() ? KConfig::SimpleConfig : KConfig::FullConfig))
    , m_selection(QPalette::Active,
                  KColorScheme::Selection,
                  KSharedConfig::openConfig(KTextEditor::EditorPrivate::unitTestMode() ? QStringLiteral("unittestmoderc") : QString(), KTextEditor::EditorPrivate::unitTestMode() ? KConfig::SimpleConfig : KConfig::FullConfig))
    , m_inactiveSelection(QPalette::Inactive,
                          KColorScheme::Selection,
                          KSharedConfig::openConfig(KTextEditor::EditorPrivate::unitTestMode() ? QStringLiteral("unittestmoderc") : QString(), KTextEditor::EditorPrivate::unitTestMode() ? KConfig::SimpleConfig : KConfig::FullConfig))

    // init our colors
    , m_background(m_view.background().color())
    , m_foreground(m_view.foreground().color())
    , m_backgroundLuma(KColorUtils::luma(m_background))
    , m_foregroundLuma(KColorUtils::luma(m_foreground))
{
}

QColor KateDefaultColors::color(ColorRole role, const KSyntaxHighlighting::Theme &theme) const
{
    // prefer theme color, if valid
    if (theme.isValid()) {
        return theme.editorColor(role);
    }

    switch (role) {
    case KSyntaxHighlighting::Theme::BackgroundColor:
        return m_background;
    case KSyntaxHighlighting::Theme::TextSelection:
        return m_selection.background().color();
    case KSyntaxHighlighting::Theme::CurrentLine:
        return m_view.background(KColorScheme::AlternateBackground).color();
    case KSyntaxHighlighting::Theme::BracketMatching:
        return KColorUtils::tint(m_background, m_view.decoration(KColorScheme::HoverColor).color());
    case KSyntaxHighlighting::Theme::WordWrapMarker:
        return KColorUtils::shade(m_background, m_backgroundLuma > 0.3 ? -0.15 : 0.03);
    case KSyntaxHighlighting::Theme::TabMarker:
        return KColorUtils::shade(m_background, m_backgroundLuma > 0.7 ? -0.35 : 0.3);
    case KSyntaxHighlighting::Theme::IndentationLine:
        return KColorUtils::shade(m_background, m_backgroundLuma > 0.7 ? -0.35 : 0.3);
    case KSyntaxHighlighting::Theme::IconBorder:
        return m_window.background().color();
    case KSyntaxHighlighting::Theme::CodeFolding:
        return m_inactiveSelection.background().color();
    case KSyntaxHighlighting::Theme::LineNumbers:
    case KSyntaxHighlighting::Theme::CurrentLineNumber:
        return m_window.foreground().color();
    case KSyntaxHighlighting::Theme::Separator:
        return m_view.foreground(KColorScheme::InactiveText).color();
    case KSyntaxHighlighting::Theme::SpellChecking:
        return m_view.foreground(KColorScheme::NegativeText).color();
    case KSyntaxHighlighting::Theme::ModifiedLines:
        return m_view.background(KColorScheme::NegativeBackground).color();
    case KSyntaxHighlighting::Theme::SavedLines:
        return m_view.background(KColorScheme::PositiveBackground).color();
    case KSyntaxHighlighting::Theme::SearchHighlight:
        return adaptToScheme(Qt::yellow, BackgroundColor);
    case KSyntaxHighlighting::Theme::ReplaceHighlight:
        return adaptToScheme(Qt::green, BackgroundColor);
    case KSyntaxHighlighting::Theme::TemplateBackground:
        return m_window.background().color();
    case KSyntaxHighlighting::Theme::TemplateFocusedPlaceholder:
        return m_view.background(KColorScheme::PositiveBackground).color();
    case KSyntaxHighlighting::Theme::TemplatePlaceholder:
        return m_view.background(KColorScheme::PositiveBackground).color();
    case KSyntaxHighlighting::Theme::TemplateReadOnlyPlaceholder:
        return m_view.background(KColorScheme::NegativeBackground).color();
    case KSyntaxHighlighting::Theme::MarkBookmark:
        return adaptToScheme(Qt::blue, BackgroundColor);
    case KSyntaxHighlighting::Theme::MarkBreakpointActive:
        return adaptToScheme(Qt::red, BackgroundColor);
    case KSyntaxHighlighting::Theme::MarkBreakpointReached:
        return adaptToScheme(Qt::yellow, BackgroundColor);
    case KSyntaxHighlighting::Theme::MarkBreakpointDisabled:
        return adaptToScheme(Qt::magenta, BackgroundColor);
    case KSyntaxHighlighting::Theme::MarkExecution:
        return adaptToScheme(Qt::gray, BackgroundColor);
    case KSyntaxHighlighting::Theme::MarkWarning:
        return m_view.foreground(KColorScheme::NeutralText).color();
    case KSyntaxHighlighting::Theme::MarkError:
        return m_view.foreground(KColorScheme::NegativeText).color();
    default:
        // we can arrive here if KSyntaxHighlighting gets new color roles!
        break;
    }

    // we can arrive here if KSyntaxHighlighting gets new color roles!
    return QColor();
}

QColor KateDefaultColors::mark(Mark mark, const KSyntaxHighlighting::Theme &theme) const
{
    // redirect to color roles enum, KSyntaxHighlighting has that all combined!
    return color(static_cast<KSyntaxHighlighting::Theme::EditorColorRole>(mark + KSyntaxHighlighting::Theme::MarkBookmark), theme);
}

QColor KateDefaultColors::mark(int i, const KSyntaxHighlighting::Theme &theme) const
{
    Q_ASSERT(i >= FIRST_MARK && i <= LAST_MARK);
    return mark(static_cast<Mark>(i), theme);
}

QColor KateDefaultColors::adaptToScheme(const QColor &color, ColorType type) const
{
    if (m_foregroundLuma > m_backgroundLuma) {
        // for dark color schemes, produce a fitting color by tinting with the foreground/background color
        return KColorUtils::tint(type == ForegroundColor ? m_foreground : m_background, color, 0.5);
    }
    return color;
}
