/*  This file is part of the KDE libraries and the Kate part.
 *
 *  Copyright (C) 2004,2010 Joseph Wenninger <jowenn@kde.org>
 *  Copyright (C) 2009 Milian Wolff <mail@milianw.de>
 *  Copyright (C) 2014 Sven Brauch <svenbrauch@gmail.com>
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Library General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Library General Public License for more details.
 *
 *  You should have received a copy of the GNU Library General Public License
 *  along with this library; see the file COPYING.LIB.  If not, write to
 *  the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 *  Boston, MA 02110-1301, USA.
 */

#include <QQueue>
#include <QRegularExpression>
#include <QVector>

#include <ktexteditor/movingcursor.h>
#include <ktexteditor/movingrange.h>

#include "katetemplatehandler.h"
#include "katedocument.h"
#include "kateview.h"
#include "kateconfig.h"
#include "katerenderer.h"
#include "kateundomanager.h"
#include "kateregexpsearch.h"
#include "kateglobal.h"
#include "script/katescriptmanager.h"
#include "katepartdebug.h"

using namespace KTextEditor;

#define ifDebug(x)

KateTemplateHandler::KateTemplateHandler(KTextEditor::ViewPrivate *view,
        Cursor position,
        const QString& templateString,
        const QString& script,
        KateUndoManager* undoManager)
    : QObject(view)
    , m_view(view)
    , m_undoManager(undoManager)
    , m_wholeTemplateRange(nullptr)
    , m_internalEdit(false)
    , m_templateScript(script, KateScript::InputSCRIPT)
{
    Q_ASSERT(m_view);

    m_templateScript.setView(m_view);

    // remember selection, it will be lost when inserting the template
    QScopedPointer<MovingRange> selection(doc()->newMovingRange(m_view->selectionRange(), MovingRange::DoNotExpand));

    m_undoManager->setAllowComplexMerge(true);

    {
        connect(doc(), &KTextEditor::DocumentPrivate::textInserted,
                this, &KateTemplateHandler::slotTemplateInserted);
        KTextEditor::Document::EditingTransaction t(doc());
        // insert the raw template string
        if (!doc()->insertText(position, templateString)) {
            deleteLater();
            return;
        }
        // now there must be a range, caught by the textInserted slot
        Q_ASSERT(m_wholeTemplateRange);
    }

    // before initialization, restore selection (if any) so user scripts can retrieve it
    m_view->setSelection(selection->toRange());
    initializeTemplate();
    // then delete the selected text (if any); it was replaced by the template
    doc()->removeText(selection->toRange());

    bool have_editable_field = false;
    Q_FOREACH ( const auto& field, m_fields ) {
        if ( field.kind == TemplateField::Editable ) {
            have_editable_field = true;
            break;
        }
    }
    // only do complex stuff when required
    if ( have_editable_field ) {
        foreach (View *view, doc()->views()) {
            setupEventHandler(view);
        }

        connect(doc(), &KTextEditor::Document::viewCreated,
                this, &KateTemplateHandler::slotViewCreated);
        connect(doc(), &KTextEditor::DocumentPrivate::textInserted,
                this, &KateTemplateHandler::updateDependentFields);
        connect(doc(), &KTextEditor::DocumentPrivate::textRemoved,
                this, &KateTemplateHandler::updateDependentFields);
        connect(doc(), &KTextEditor::Document::aboutToReload,
                this, &KateTemplateHandler::deleteLater);

    } else {
        // when no interesting ranges got added, we can terminate directly
        jumpToFinalCursorPosition();
        deleteLater();
    }
}

KateTemplateHandler::~KateTemplateHandler()
{
    m_undoManager->setAllowComplexMerge(false);
}

void KateTemplateHandler::sortFields()
{
    qSort(m_fields.begin(), m_fields.end(), [](const TemplateField& l, const TemplateField& r) {
        // always sort the final cursor pos last
        if ( l.kind == TemplateField::FinalCursorPosition ) {
            return false;
        }
        if ( r.kind == TemplateField::FinalCursorPosition ) {
            return true;
        }
        // sort by range
        return l.range->toRange() < r.range->toRange();
    });
}

void KateTemplateHandler::jumpToNextRange()
{
    jump(+1);
}

void KateTemplateHandler::jumpToPreviousRange()
{
    jump(-1);
}

void KateTemplateHandler::jump(int by, bool initial)
{
    Q_ASSERT(by == 1 || by == -1);
    sortFields();

    // find (editable) field index of current cursor position
    int pos = -1;
    auto cursor = view()->cursorPosition();
    // if initial is not set, should start from the beginning (field -1)
    if ( ! initial ) {
        pos = m_fields.indexOf(fieldForRange(KTextEditor::Range(cursor, cursor)));
    }

    // modulo field count and make positive
    auto wrap = [this](int x) -> unsigned int {
        x %= m_fields.size();
        return x + (x < 0 ? m_fields.size() : 0);
    };

    pos = wrap(pos);
    // choose field to jump to, including wrap-around
    auto choose_next_field = [this, by, wrap](unsigned int from_field_index) {
        for ( int i = from_field_index + by; ; i += by ) {
            auto wrapped_i = wrap(i);
            auto kind = m_fields.at(wrapped_i).kind;
            if ( kind == TemplateField::Editable || kind == TemplateField::FinalCursorPosition ) {
                // found an editable field by walking into the desired direction
                return wrapped_i;
            }
            if ( wrapped_i == from_field_index ) {
                // nothing found, do nothing (i.e. keep cursor in current field)
                break;
            }
        }
        return from_field_index;
    };

    // jump
    auto jump_to_field = m_fields.at(choose_next_field(pos));
    view()->setCursorPosition(jump_to_field.range->toRange().start());
    if ( ! jump_to_field.touched ) {
        // field was never edited by the user, so select its contents
        view()->setSelection(jump_to_field.range->toRange());
    }
}

void KateTemplateHandler::jumpToFinalCursorPosition()
{
    Q_FOREACH ( const auto& field, m_fields ) {
        if ( field.kind == TemplateField::FinalCursorPosition ) {
            view()->setCursorPosition(field.range->toRange().start());
            return;
        }
    }
    view()->setCursorPosition(m_wholeTemplateRange->end());
}

void KateTemplateHandler::slotTemplateInserted(Document* /*document*/, const Range&range)
{
    m_wholeTemplateRange.reset(doc()->newMovingRange(range, MovingRange::ExpandLeft | MovingRange::ExpandRight));

    disconnect(doc(), &KTextEditor::DocumentPrivate::textInserted,
               this, &KateTemplateHandler::slotTemplateInserted);
}

KTextEditor::DocumentPrivate* KateTemplateHandler::doc() const
{
    return m_view->doc();
}

void KateTemplateHandler::slotViewCreated(Document *document, View *view)
{
    Q_ASSERT(document == doc());
    Q_UNUSED(document)
    setupEventHandler(view);
}

void KateTemplateHandler::setupEventHandler(View *view)
{
    view->focusProxy()->installEventFilter(this);
}

bool KateTemplateHandler::eventFilter(QObject *object, QEvent *event)
{
    // prevent indenting by eating the keypress event for TAB
    if (event->type() == QEvent::KeyPress || event->type() == QEvent::KeyRelease) {
        QKeyEvent* keyEvent = static_cast<QKeyEvent*>(event);
        if (keyEvent->key() == Qt::Key_Tab || keyEvent->key() == Qt::Key_Backtab) {
            if (!m_view->isCompletionActive()) {
                return true;
            }
        }
    }

    // actually offer shortcuts for navigation
    if (event->type() == QEvent::ShortcutOverride) {
        QKeyEvent* keyEvent = static_cast<QKeyEvent*>(event);

        if (keyEvent->key() == Qt::Key_Return && keyEvent->modifiers() & Qt::AltModifier) {
            // terminate
            jumpToFinalCursorPosition();
            deleteLater();
            keyEvent->accept();
            return true;
        } else if (keyEvent->key() == Qt::Key_Escape) {
            if (!m_view->selection()) {
                // terminate
                jumpToFinalCursorPosition();
                deleteLater();
                keyEvent->accept();
                return true;
            }
        } else if (keyEvent->key() == Qt::Key_Tab && !m_view->isCompletionActive()) {
            if (keyEvent->modifiers() & Qt::ShiftModifier) {
                jumpToPreviousRange();
            } else {
                jumpToNextRange();
            }
            keyEvent->accept();
            return true;
        } else if (keyEvent->key() == Qt::Key_Backtab && !m_view->isCompletionActive()) {
            jumpToPreviousRange();
            keyEvent->accept();
            return true;
        }
    }

    return QObject::eventFilter(object, event);
}

/**
 * Returns an attribute with \p color as background with @p alpha alpha value.
 */
Attribute::Ptr getAttribute(QColor color, int alpha = 230)
{
    Attribute::Ptr attribute(new Attribute());
    color.setAlpha(alpha);
    attribute->setBackground(QBrush(color));
    return attribute;
}

void KateTemplateHandler::parseFields(const QString& templateText)
{
    // matches any field, i.e. the three forms ${foo}, ${foo=expr}, ${func()}
    QRegularExpression field(QStringLiteral("\\${([^}]+)}"));
    // matches the "foo=expr" form within a match of the above expression
    QRegularExpression defaultField(QStringLiteral("\\w+=([^\\}]*)"));

    // create a moving range spanning the given field
    auto createMovingRangeForMatch = [this, &templateText](const QRegularExpressionMatch& match) {
        const auto offset = match.capturedStart(0);
        const auto left = templateText.left(offset);
        const auto nl = QLatin1Char('\n');
        const auto rel_lineno = left.count(nl);
        const auto start = m_wholeTemplateRange->start().toCursor();
        auto matchStart = Cursor(start.line(), rel_lineno == 0 ? start.column() : 0) +
                          Cursor(rel_lineno, offset - left.lastIndexOf(nl) - 1);
        return doc()->newMovingRange({matchStart, matchStart + Cursor(0, match.capturedLength(0))},
                                     MovingRange::ExpandLeft | MovingRange::ExpandRight);
    };

    auto fieldMatch = field.globalMatch(templateText);
    while ( fieldMatch.hasNext() ) {
        auto match = fieldMatch.next();
        auto defaultMatch = defaultField.match(match.captured(0));
        auto contents = match.captured(1);
        TemplateField f;
        f.range.reset(createMovingRangeForMatch(match));
        f.identifier = contents;
        f.kind = TemplateField::Editable;
        // the field has a default value, i.e. ${foo=3}
        if ( defaultMatch.hasMatch() ) {
            f.defaultValue = defaultMatch.captured(1);
            f.identifier = contents.split(QLatin1Char('=')).at(0).trimmed();
        }
        else if ( f.identifier.contains(QLatin1Char('(')) ) {
            // treat the field as function call when it contains an opening parenthesis
            f.kind = TemplateField::FunctionCall;
        }
        else if ( f.identifier == QStringLiteral("cursor") ) {
            // field marks the final cursor position
            f.kind = TemplateField::FinalCursorPosition;
        }
        Q_FOREACH ( const auto& other, m_fields ) {
            if ( ! (f == other) && other.identifier == f.identifier ) {
                f.kind = TemplateField::Mirror;
                break;
            }
        }
        m_fields.append(f);
    }
}

void KateTemplateHandler::setupFieldRanges()
{
    auto config = m_view->renderer()->config();
    auto editableAttribute = getAttribute(config->templateEditablePlaceholderColor(), 255);
    editableAttribute->setDynamicAttribute(
        Attribute::ActivateCaretIn, getAttribute(config->templateFocusedEditablePlaceholderColor(), 255)
    );
    auto notEditableAttribute = getAttribute(config->templateNotEditablePlaceholderColor(), 255);

    // color the whole template
    m_wholeTemplateRange->setAttribute(getAttribute(config->templateBackgroundColor(), 200));

    // color all the template fields
    Q_FOREACH ( const auto& field, m_fields ) {
        field.range->setAttribute(field.kind == TemplateField::Editable ? editableAttribute : notEditableAttribute);
    }
}

void KateTemplateHandler::setupDefaultValues()
{
    Q_FOREACH ( const auto& field, m_fields ) {
        if ( field.kind != TemplateField::Editable ) {
            continue;
        }
        QString value;
        if ( field.defaultValue.isEmpty() ) {
            // field has no default value specified; use its identifier
            value = field.identifier;
        }
        else {
            // field has a default value; evaluate it with the JS engine
            value = m_templateScript.evaluate(field.defaultValue).toString();
        }
        doc()->replaceText(field.range->toRange(), value);
    }
}

void KateTemplateHandler::initializeTemplate()
{
    auto templateString = doc()->text(*m_wholeTemplateRange);
    parseFields(templateString);
    setupFieldRanges();
    setupDefaultValues();

    // call update for each field to set up the initial stuff
    for ( int i = 0; i < m_fields.size(); i++ ) {
        auto& field = m_fields[i];
        ifDebug(qCDebug(LOG_PART) << "update field:" << field.range->toRange();)
        updateDependentFields(doc(), field.range->toRange());
        // remove "user edited field" mark set by the above call since it's not a real edit
        field.touched = false;
    }

    // place the cursor at the first field and select stuff if necessary
    if ( m_fields.count() > 0 ) {
        jump(1, true);
    }
}

const KateTemplateHandler::TemplateField KateTemplateHandler::fieldForRange(const KTextEditor::Range& range) const
{
    Q_FOREACH ( const auto& field, m_fields ) {
        if ( field.range->contains(range.start()) || field.range->end() == range.start() ) {
            return field;
        }
    }
    return {};
}

void KateTemplateHandler::updateDependentFields(Document *document, const Range &range)
{
    Q_ASSERT(document == doc());
    Q_UNUSED(document);
    if ( ! m_undoManager->isActive() ) {
        // currently undoing stuff; don't update fields
        return;
    }

    bool in_range = m_wholeTemplateRange->toRange().contains(range.start());
    bool at_end = m_wholeTemplateRange->toRange().end() == range.end() || m_wholeTemplateRange->toRange().end() == range.start();
    if ( m_wholeTemplateRange->toRange().isEmpty() || (!in_range && !at_end) ) {
        // edit outside template range, abort
        ifDebug(qCDebug(LOG_PART) << "edit outside template range, exiting";)
        deleteLater();
        return;
    }

    if ( m_internalEdit || range.isEmpty() ) {
        // internal or null edit; for internal edits, don't do anything
        // to prevent unwanted recursion
        return;
    }

    ifDebug(qCDebug(LOG_PART) << "text changed" << document << range;)

    // group all the changes into one undo transaction
    KTextEditor::Document::EditingTransaction t(doc());

    // find the field which was modified, if any
    sortFields();
    const auto changedField = fieldForRange(range);
    if ( changedField.kind == TemplateField::Invalid ) {
        // edit not within a field, nothing to do
        ifDebug(qCDebug(LOG_PART) << "edit not within a field:" << range;)
        return;
    }
    if ( changedField.kind == TemplateField::FinalCursorPosition && doc()->text(changedField.range->toRange()).isEmpty() ) {
        // text changed at final cursor position: the user is done, so exit
        // this is not executed when the field's range is not empty: in that case this call
        // is for initial setup and we have to continue below
        ifDebug(qCDebug(LOG_PART) << "final cursor changed:" << range;)
        deleteLater();
        return;
    }

    // turn off expanding left/right for all ranges except @p current;
    // this prevents ranges from overlapping each other when they are adjacent
    auto dontExpandOthers = [this](const TemplateField& current) {
        for ( int i = 0; i < m_fields.size(); i++ ) {
            if ( current.range != m_fields.at(i).range ) {
                m_fields.at(i).range->setInsertBehaviors(MovingRange::DoNotExpand);
            }
            else {
                m_fields.at(i).range->setInsertBehaviors(MovingRange::ExpandLeft | MovingRange::ExpandRight);
            }
        }
    };

    // new contents of the changed template field
    const auto& newText = doc()->text(changedField.range->toRange());
    m_internalEdit = true;
    // go through all fields and update the contents of the dependent ones
    for ( auto field = m_fields.begin(); field != m_fields.end(); field++ ) {
        if ( field->kind == TemplateField::FinalCursorPosition ) {
            // only relevant on first run
            doc()->replaceText(field->range->toRange(), QString());
        }

        if ( *field == changedField ) {
            // mark that the user changed this field
            field->touched = true;
        }

        // If this is mirrored field with the same identifier as the
        // changed one and the changed one is editable, mirror changes
        // edits to non-editable mirror fields are ignored
        if ( field->kind == TemplateField::Mirror &&
             changedField.kind == TemplateField::Editable &&
             field->identifier == changedField.identifier )
        {
            // editable field changed, mirror changes
            dontExpandOthers(*field);
            doc()->replaceText(field->range->toRange(), newText);
        }
        else if ( field->kind == TemplateField::FunctionCall ) {
            // replace field by result of function call
            dontExpandOthers(*field);
            // build map of objects in the scope to pass to the function
            auto map = fieldMap();
            const auto& callResult = m_templateScript.evaluate(field->identifier, map);
            doc()->replaceText(field->range->toRange(), callResult.toString());
        }
    }
    m_internalEdit = false;
    updateRangeBehaviours();
}

void KateTemplateHandler::updateRangeBehaviours()
{
    KTextEditor::Cursor last = {-1, -1};
    for ( int i = 0; i < m_fields.size(); i++ ) {
        auto field = m_fields.at(i);
        auto end = field.range->end().toCursor();
        auto start = field.range->start().toCursor();
        if ( field.kind == TemplateField::FinalCursorPosition ) {
            // final cursor position never grows
            field.range->setInsertBehaviors(MovingRange::DoNotExpand);
        }
        else if ( start <= last ) {
            // ranges are adjacent, only expand to the right to prevent overlap
            field.range->setInsertBehaviors(MovingRange::ExpandRight);
        }
        else {
            // ranges are not adjacent, can grow in both directions
            field.range->setInsertBehaviors(MovingRange::ExpandLeft | MovingRange::ExpandRight);
        }
        last = end;
    }
}

KateScript::FieldMap KateTemplateHandler::fieldMap() const
{
    KateScript::FieldMap map;
    Q_FOREACH ( const auto& field, m_fields ) {
        if ( field.kind != TemplateField::Editable ) {
            // only editable fields are of interest to the scripts
            continue;
        }
        map.insert(field.identifier, QScriptValue(doc()->text(field.range->toRange())));
    }
    return map;
}

KTextEditor::ViewPrivate *KateTemplateHandler::view() const
{
    return m_view;
}

