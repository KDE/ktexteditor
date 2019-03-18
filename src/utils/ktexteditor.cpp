/* This file is part of the KDE project
   Copyright (C) 2001 Christoph Cullmann (cullmann@kde.org)

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#include "kateview.h"

#include "cursor.h"

#include "configpage.h"

#include "editor.h"

#include "document.h"

#include "view.h"

#include "plugin.h"

#include "command.h"
#include "inlinenoteinterface.h"
#include "inlinenote.h"
#include "inlinenotedata.h"
#include "inlinenoteprovider.h"
#include "markinterface.h"
#include "modificationinterface.h"
#include "sessionconfiginterface.h"
#include "texthintinterface.h"
#include "variable.h"

#include "annotationinterface.h"
#include "abstractannotationitemdelegate.h"

#include "kateglobal.h"
#include "kateconfig.h"
#include "katecmd.h"
#include "katemacroexpander.h"

using namespace KTextEditor;

Cursor Cursor::fromString(const QStringRef& str) Q_DECL_NOEXCEPT
{
    // parse format "(line, column)"
    const int startIndex = str.indexOf(QLatin1Char('('));
    const int endIndex = str.indexOf(QLatin1Char(')'));
    const int commaIndex = str.indexOf(QLatin1Char(','));

    if (startIndex < 0 || endIndex < 0 || commaIndex < 0 ||
        commaIndex < startIndex || endIndex < commaIndex ||
        endIndex < startIndex)
    {
        return invalid();
    }

    bool ok1 = false;
    bool ok2 = false;

    const int line = str.mid(startIndex + 1, commaIndex - startIndex - 1).toInt(&ok1);
    const int column = str.mid(commaIndex + 1, endIndex - commaIndex - 1).toInt(&ok2);

    if (!ok1 || !ok2) {
        return invalid();
    }

    return {line, column};
}

Editor::Editor(EditorPrivate *impl)
    : QObject ()
    , d (impl)
{
}

Editor::~Editor()
{
}

Editor *KTextEditor::Editor::instance()
{
    /**
     * Just use internal KTextEditor::EditorPrivate::self()
     */
    return KTextEditor::EditorPrivate::self();
}

QString Editor::defaultEncoding () const
{
    /**
     * return default encoding in global config object
     */
    return d->documentConfig()->encoding ();
}

bool Editor::registerVariableMatch(const QString& name, const QString& description, ExpandFunction expansionFunc)
{
    if (name.isEmpty() || expansionFunc == nullptr)
        return false;

    if (d->m_variableExactMatches.contains(name))
        return false;

    d->m_variableExactMatches.insert(name, Variable(name, description, expansionFunc));
    return true;
}

bool Editor::registerVariablePrefix(const QString& prefix, const QString& description, ExpandFunction expansionFunc)
{
    if (prefix.isEmpty() || expansionFunc == nullptr)
        return false;

    if (d->m_variablePrefixMatches.contains(prefix))
        return false;

    if (!prefix.contains(QLatin1Char(':')))
        return false;

    d->m_variablePrefixMatches.insert(prefix, Variable(prefix, description, expansionFunc));
    return true;
}

bool Editor::unregisterVariableMatch(const QString& variable)
{
    auto it = d->m_variableExactMatches.find(variable);
    if (it != d->m_variableExactMatches.end()) {
        d->m_variableExactMatches.erase(it);
        return true;
    }
    return false;
}

bool Editor::unregisterVariablePrefix(const QString& variable)
{
    auto it = d->m_variablePrefixMatches.find(variable);
    if (it != d->m_variablePrefixMatches.end()) {
        d->m_variablePrefixMatches.erase(it);
        return true;
    }
    return false;
}

bool Editor::expandVariable(const QString& variable, KTextEditor::View* view, QString& output) const
{
    // first try exact matches
    const auto it = d->m_variableExactMatches.find(variable);
    if (it != d->m_variableExactMatches.end()) {
        output = it->evaluate(variable, view);
        return true;
    }

    // try prefix matching
    const int colonIndex = variable.indexOf(QLatin1Char(':'));
    if (colonIndex >= 0) {
        const QString prefix = variable.left(colonIndex + 1);
        const auto itPrefix = d->m_variablePrefixMatches.find(prefix);
        if (itPrefix != d->m_variablePrefixMatches.end()) {
            output = itPrefix->evaluate(variable, view);
            return true;
        }
    }
    return false;
}

void Editor::expandText(const QString& text, KTextEditor::View* view, QString& output) const
{
    output = KateMacroExpander::expandMacro(text, view);
}

bool View::insertText(const QString &text)
{
    KTextEditor::Document *doc = document();
    if (!doc) {
        return false;
    }
    return doc->insertText(cursorPosition(), text, blockSelection());
}

bool View::isStatusBarEnabled() const
{
    /**
     * is the status bar around?
     */
    return !!d->statusBar();
}

void View::setStatusBarEnabled(bool enable)
{
    /**
     * no state change, do nothing
     */
    if (enable == !!d->statusBar())
        return;

    /**
     * else toggle it
     */
    d->toggleStatusBar ();
}

bool View::insertTemplate(const KTextEditor::Cursor& insertPosition,
                          const QString& templateString,
                          const QString& script)
{
    return d->insertTemplateInternal(insertPosition, templateString, script);
}

void View::setViewInputMode(InputMode inputMode)
{
    d->setInputMode(inputMode);
}

ConfigPage::ConfigPage(QWidget *parent)
    : QWidget(parent)
    , d(nullptr)
{}

ConfigPage::~ConfigPage()
{}

QString ConfigPage::fullName() const
{
    return name();
}

QIcon ConfigPage::icon() const
{
    return QIcon::fromTheme(QStringLiteral("document-properties"));
}

View::View (ViewPrivate *impl, QWidget *parent)
    : QWidget (parent), KXMLGUIClient()
    , d(impl)
{}

View::~View()
{}

Plugin::Plugin(QObject *parent)
    : QObject(parent)
    , d(nullptr)
{}

Plugin::~Plugin()
{}

int Plugin::configPages() const
{
    return 0;
}

ConfigPage *Plugin::configPage(int, QWidget *)
{
    return nullptr;
}

MarkInterface::MarkInterface()
{}

MarkInterface::~MarkInterface()
{}

ModificationInterface::ModificationInterface()
{}

ModificationInterface::~ModificationInterface()
{}

SessionConfigInterface::SessionConfigInterface()
{}

SessionConfigInterface::~SessionConfigInterface()
{}

TextHintInterface::TextHintInterface()
{}

TextHintInterface::~TextHintInterface()
{}

TextHintProvider::TextHintProvider()
{}

TextHintProvider::~TextHintProvider()
{}

InlineNoteInterface::InlineNoteInterface()
{}

InlineNoteInterface::~InlineNoteInterface()
{}

InlineNoteProvider::InlineNoteProvider()
{}

InlineNoteProvider::~InlineNoteProvider()
{}

KateInlineNoteData::KateInlineNoteData(KTextEditor::InlineNoteProvider* provider,
                                       const KTextEditor::View* view,
                                       const KTextEditor::Cursor& position,
                                       int index,
                                       bool underMouse,
                                       const QFont& font,
                                       int lineHeight)
    : m_provider(provider)
    , m_view(view)
    , m_position(position)
    , m_index(index)
    , m_underMouse(underMouse)
    , m_font(font)
    , m_lineHeight(lineHeight)
{}

InlineNote::InlineNote(const KateInlineNoteData& data)
    : d(data)
{
}

qreal InlineNote::width() const
{
    return d.m_provider->inlineNoteSize(*this).width();
}

bool KTextEditor::InlineNote::underMouse() const
{
    return d.m_underMouse;
}

void KTextEditor::InlineNoteProvider::inlineNoteActivated(const InlineNote& note, Qt::MouseButtons buttons, const QPoint& globalPos)
{
    Q_UNUSED(note);
    Q_UNUSED(buttons);
    Q_UNUSED(globalPos);
}

void KTextEditor::InlineNoteProvider::inlineNoteFocusInEvent(const KTextEditor::InlineNote& note, const QPoint& globalPos)
{
    Q_UNUSED(note);
    Q_UNUSED(globalPos);
}

void KTextEditor::InlineNoteProvider::inlineNoteFocusOutEvent(const KTextEditor::InlineNote& note)
{
    Q_UNUSED(note);
}

void KTextEditor::InlineNoteProvider::inlineNoteMouseMoveEvent(const KTextEditor::InlineNote& note, const QPoint& globalPos)
{
    Q_UNUSED(note);
    Q_UNUSED(globalPos);
}

KTextEditor::InlineNoteProvider* InlineNote::provider() const
{
    return d.m_provider;
}

const KTextEditor::View* InlineNote::view() const
{
    return d.m_view;
}

QFont InlineNote::font() const
{
    return d.m_font;
}

int InlineNote::index() const
{
    return d.m_index;
}

int InlineNote::lineHeight() const
{
    return d.m_lineHeight;
}

KTextEditor::Cursor InlineNote::position() const
{
    return d.m_position;
}

Command::Command(const QStringList &cmds, QObject *parent)
    : QObject(parent)
    , m_cmds (cmds)
    , d(nullptr)
{
    // register this command
    static_cast<KTextEditor::EditorPrivate *> (KTextEditor::Editor::instance())->cmdManager()->registerCommand (this);
}

Command::~Command()
{
    // unregister this command, if instance is still there!
    if (KTextEditor::Editor::instance())
        static_cast<KTextEditor::EditorPrivate *> (KTextEditor::Editor::instance())->cmdManager()->unregisterCommand (this);
}

bool Command::supportsRange(const QString &)
{
    return false;
}

KCompletion *Command::completionObject(KTextEditor::View *, const QString &)
{
    return nullptr;
}

bool Command::wantsToProcessText(const QString &)
{
    return false;
}

void Command::processText(KTextEditor::View *, const QString &)
{
}

void View::setScrollPosition(KTextEditor::Cursor &cursor)
{
    d->setScrollPositionInternal(cursor);
}

void View::setHorizontalScrollPosition(int x)
{
    d->setHorizontalScrollPositionInternal(x);
}

KTextEditor::Cursor View::maxScrollPosition() const
{
    return d->maxScrollPositionInternal();
}

int View::firstDisplayedLine(LineType lineType) const
{
    return d->firstDisplayedLineInternal(lineType);
}

int View::lastDisplayedLine(LineType lineType) const
{
    return d->lastDisplayedLineInternal(lineType);
}

QRect View::textAreaRect() const
{
    return d->textAreaRectInternal();
}

StyleOptionAnnotationItem::StyleOptionAnnotationItem()
    : contentFontMetrics(QFont())
{}

StyleOptionAnnotationItem::StyleOptionAnnotationItem(const StyleOptionAnnotationItem &other)
    : QStyleOption(Version, Type)
    , contentFontMetrics(QFont())
{
    *this = other;
}

StyleOptionAnnotationItem::StyleOptionAnnotationItem(int version)
    : QStyleOption(version, Type)
    , contentFontMetrics(QFont())
{}

AbstractAnnotationItemDelegate::AbstractAnnotationItemDelegate(QObject *parent)
    : QObject(parent)
{}

AbstractAnnotationItemDelegate::~AbstractAnnotationItemDelegate() = default;
