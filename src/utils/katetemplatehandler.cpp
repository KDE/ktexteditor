/*
    SPDX-FileCopyrightText: 2004, 2010 Joseph Wenninger <jowenn@kde.org>
    SPDX-FileCopyrightText: 2009 Milian Wolff <mail@milianw.de>
    SPDX-FileCopyrightText: 2014 Sven Brauch <svenbrauch@gmail.com>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include <QKeyEvent>
#include <QQueue>
#include <QRegularExpression>
#include <algorithm>

#include <ktexteditor/movingcursor.h>
#include <ktexteditor/movingrange.h>

#include "cursor.h"
#include "kateconfig.h"
#include "katedocument.h"
#include "kateglobal.h"
#include "katepartdebug.h"
#include "kateregexpsearch.h"
#include "katetemplatehandler.h"
#include "kateundomanager.h"
#include "kateview.h"
#include "script/katescriptmanager.h"

using namespace KTextEditor;

#define ifDebug(x)

KateTemplateHandler::KateTemplateHandler(KTextEditor::ViewPrivate *view,
                                         Cursor position,
                                         const QString &templateString,
                                         const QString &script,
                                         KateUndoManager *undoManager)
    : QObject(view)
    , m_view(view)
    , m_undoManager(undoManager)
    , m_wholeTemplateRange()
    , m_internalEdit(false)
    , m_templateScript(script, KateScript::InputSCRIPT)
{
    Q_ASSERT(m_view);

    m_templateScript.setView(m_view);

    // remember selection, it will be lost when inserting the template
    std::unique_ptr<MovingRange> selection(doc()->newMovingRange(m_view->selectionRange(), MovingRange::DoNotExpand));

    m_undoManager->setAllowComplexMerge(true);

    {
        connect(doc(), &KTextEditor::DocumentPrivate::textInsertedRange, this, &KateTemplateHandler::slotTemplateInserted);
        KTextEditor::Document::EditingTransaction t(doc());
        // insert the raw template string
        if (!doc()->insertText(position, templateString)) {
            deleteLater();
            return;
        }
        // now there must be a range, caught by the textInserted slot
        Q_ASSERT(m_wholeTemplateRange);
        doc()->align(m_view, *m_wholeTemplateRange);
    }

    // before initialization, restore selection (if any) so user scripts can retrieve it
    m_view->setSelection(selection->toRange());
    initializeTemplate();
    // then delete the selected text (if any); it was replaced by the template
    doc()->removeText(selection->toRange());

    const bool have_editable_field = std::any_of(m_fields.constBegin(), m_fields.constEnd(), [](const TemplateField &field) {
        return (field.kind == TemplateField::Editable);
    });
    // only do complex stuff when required
    if (have_editable_field) {
        const auto views = doc()->views();
        for (View *view : views) {
            setupEventHandler(view);
        }

        // place the cursor at the first field and select stuff
        jump(1, true);

        connect(doc(), &KTextEditor::Document::viewCreated, this, &KateTemplateHandler::slotViewCreated);
        connect(doc(), &KTextEditor::DocumentPrivate::textInsertedRange, this, [this](KTextEditor::Document *doc, KTextEditor::Range range) {
            updateDependentFields(doc, range, false);
        });
        connect(doc(), &KTextEditor::DocumentPrivate::textRemoved, this, [this](KTextEditor::Document *doc, KTextEditor::Range range, const QString &) {
            updateDependentFields(doc, range, true);
        });
        connect(doc(), &KTextEditor::Document::aboutToReload, this, &KateTemplateHandler::deleteLater);

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
    std::sort(m_fields.begin(), m_fields.end(), [](const auto &l, const auto &r) {
        return l.range->toRange().start() < r.range->toRange().start();
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

    auto start = view()->cursorPosition();

    if (initial) {
        start = Cursor{-1, -1};
    }

    std::stable_sort(m_fields.begin(), m_fields.end(), [&by](const auto &a, const auto &b) {
        if (by > 0) {
            return a.range->toRange().start() < b.range->toRange().start();
        } else {
            return a.range->toRange().start() > b.range->toRange().start();
        }
    });

    std::stable_partition(m_fields.begin(), m_fields.end(), [&by, &start](const auto &a) {
        if (by > 0) {
            return a.range->start() > start;
        } else {
            return a.range->start() < start;
        }
    });

    const auto it = std::find_if(m_fields.begin(), m_fields.end(), [](const auto &a) {
        return (!a.removed && ((a.kind == TemplateField::Editable && !a.range->isEmpty()) || a.kind == TemplateField::FinalCursorPosition));
    });

    if (it != m_fields.end()) {
        // found a valid field, jump to its start position
        const auto &field = *it;

        view()->setCursorPosition(field.range->toRange().start());

        if (!field.touched) {
            // field was never edited by the user, so select its contents
            view()->setSelection(field.range->toRange());
        } else {
            view()->clearSelection();
        }
    } else {
        // found nothing, stay put
        return;
    }
}

void KateTemplateHandler::jumpToFinalCursorPosition()
{
    for (const auto &field : std::as_const(m_fields)) {
        if (field.kind == TemplateField::FinalCursorPosition) {
            view()->setCursorPosition(field.range->toRange().start());
            return;
        }
    }
    view()->setCursorPosition(m_wholeTemplateRange->end());
}

void KateTemplateHandler::slotTemplateInserted(Document * /*document*/, Range range)
{
    m_wholeTemplateRange.reset(doc()->newMovingRange(range, MovingRange::ExpandLeft | MovingRange::ExpandRight));

    disconnect(doc(), &KTextEditor::DocumentPrivate::textInsertedRange, this, &KateTemplateHandler::slotTemplateInserted);
}

KTextEditor::DocumentPrivate *KateTemplateHandler::doc() const
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
        QKeyEvent *keyEvent = static_cast<QKeyEvent *>(event);
        if (keyEvent->key() == Qt::Key_Tab || keyEvent->key() == Qt::Key_Backtab) {
            if (!m_view->isCompletionActive()) {
                return true;
            }
        }
    }

    // actually offer shortcuts for navigation
    if (event->type() == QEvent::ShortcutOverride) {
        QKeyEvent *keyEvent = static_cast<QKeyEvent *>(event);

        if (keyEvent->key() == Qt::Key_Escape || (keyEvent->key() == Qt::Key_Return && keyEvent->modifiers() & Qt::AltModifier)) {
            // terminate
            jumpToFinalCursorPosition();
            view()->clearSelection();
            deleteLater();
            keyEvent->accept();
            return true;
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

void KateTemplateHandler::parseFields(const QString &templateText)
{
    // matches any field, i.e. the three forms ${foo}, ${foo=expr}, ${func()}
    static const QRegularExpression field(QStringLiteral(R"((?<!\\)(?<slash>(?:\\\\)*)\${(?<body>[^}]+)})"), QRegularExpression::UseUnicodePropertiesOption);
    // matches the "foo=expr" form within a match of the above expression
    static const QRegularExpression defaultField(QStringLiteral(R"(\w+=([^}]*))"), QRegularExpression::UseUnicodePropertiesOption);
    // this only captures escaped fields, i.e. \\${foo} etc.
    static const QRegularExpression escapedField(QStringLiteral(R"((?<!\\)(?<slash>\\(?:\\\\)*)\${[^}]+})"), QRegularExpression::UseUnicodePropertiesOption);

    // compute start cursor of a match
    auto startOfMatch = [this, &templateText](const QRegularExpressionMatch &match) {
        const auto offset = match.capturedStart(0);
        const auto left = QStringView(templateText).left(offset);
        const auto nl = QLatin1Char('\n');
        const auto rel_lineno = left.count(nl);
        const auto start = m_wholeTemplateRange->start().toCursor();

        return Cursor(start.line(), rel_lineno == 0 ? start.column() : 0) + Cursor(rel_lineno, offset - left.lastIndexOf(nl) - 1);
    };

    // create a moving range spanning the given field
    auto createMovingRangeForMatch = [this, startOfMatch](const QRegularExpressionMatch &match) {
        auto matchStart = startOfMatch(match);
        auto slashOffset = Cursor{0, static_cast<int>(match.capturedLength("slash"))};
        return doc()->newMovingRange({matchStart + slashOffset, matchStart + Cursor(0, match.capturedLength(0))},
                                     MovingRange::ExpandLeft | MovingRange::ExpandRight);
    };

    // list of escape backslashes to remove after parsing
    QList<KTextEditor::Range> stripBackslashes;

    // process fields
    auto fieldMatch = field.globalMatch(templateText);
    QMap<QString, qsizetype> mainFields;

    while (fieldMatch.hasNext()) {
        const auto match = fieldMatch.next();

        // collect leading escaped backslashes
        if (match.hasCaptured("slash") && match.capturedLength("slash") > 0) {
            auto slashes = match.captured("slash");
            auto start = startOfMatch(match);
            int count = std::floor(match.capturedLength("slash") / 2);
            stripBackslashes.append({start, start + Cursor{0, count}});
        }

        // a template field was found, instantiate a field object and populate it
        auto defaultMatch = defaultField.match(match.captured(0));
        const QString contents = match.captured("body");

        TemplateField f;
        f.range.reset(createMovingRangeForMatch(match));
        f.identifier = contents;
        f.kind = TemplateField::Editable;

        if (defaultMatch.hasMatch()) {
            // the field has a default value, i.e. ${foo=3}
            f.defaultValue = defaultMatch.captured(1);
            f.identifier = QStringView(contents).left(contents.indexOf(QLatin1Char('='))).trimmed().toString();
        } else if (f.identifier.contains(QLatin1Char('('))) {
            // field is a function call when it contains an opening parenthesis
            f.kind = TemplateField::FunctionCall;
        } else if (f.identifier == QLatin1String("cursor")) {
            // field marks the final cursor position
            f.kind = TemplateField::FinalCursorPosition;
        }

        m_fields.append(f);
        auto &storedField = m_fields.last();
        auto index = m_fields.count() - 1;

        if (f.kind != TemplateField::FinalCursorPosition) {
            if (mainFields.contains(f.identifier)) {
                auto &other = m_fields[mainFields[f.identifier]];

                if (defaultMatch.hasMatch() && other.defaultValue.isEmpty()) {
                    other.kind = TemplateField::Mirror;
                    mainFields[storedField.identifier] = index;
                } else {
                    storedField.kind = TemplateField::Mirror;
                }
            } else {
                mainFields[storedField.identifier] = index;
            }
        }
    }

    for (const auto &match : escapedField.globalMatch(templateText)) {
        // $ is escaped, not a field; mark the backslash for removal
        auto start = startOfMatch(match);
        int count = std::floor(match.captured("slash").length() / 2) + 1;
        stripBackslashes.append({start, start + Cursor{0, count}});
    }

    // remove escape characters
    // sort the list so the characters are removed starting from the
    // back and ranges do not move around
    std::sort(stripBackslashes.begin(), stripBackslashes.end(), [](const Range l, const Range r) {
        return l > r;
    });

    for (const auto &backslash : stripBackslashes) {
        doc()->removeText(backslash);
    }
}

void KateTemplateHandler::setupFieldRanges()
{
    auto config = m_view->rendererConfig();
    auto editableAttribute = getAttribute(config->templateEditablePlaceholderColor(), 255);
    editableAttribute->setDynamicAttribute(Attribute::ActivateCaretIn, getAttribute(config->templateFocusedEditablePlaceholderColor(), 255));
    auto notEditableAttribute = getAttribute(config->templateNotEditablePlaceholderColor(), 255);

    // color the whole template
    m_wholeTemplateRange->setAttribute(getAttribute(config->templateBackgroundColor(), 200));

    // color all the template fields
    for (const auto &field : std::as_const(m_fields)) {
        field.range->setAttribute(field.kind == TemplateField::Editable ? editableAttribute : notEditableAttribute);

        // initially set all ranges to be static, as required by setupDefaultValues()
        field.range->setInsertBehaviors(KTextEditor::MovingRange::DoNotExpand);
    }
}

void KateTemplateHandler::setupDefaultValues()
{
    // Evaluate default values and apply them to the field that defined them:
    // ${x='foo'}, ${x=func()}, etc.

    KateScript::FieldMap defaultsMap;

    for (auto &field : m_fields) {
        if (field.kind != TemplateField::Editable) {
            continue;
        }

        if (field.defaultValue.isEmpty() && field.kind != TemplateField::FinalCursorPosition) {
            // field has no default value specified; use its identifier
            field.defaultValue = field.identifier;
        } else {
            // field has a default value; evaluate it with the JS engine
            //
            // The default value may only reference other fields that are defined
            // before the current field.
            //
            // Examples:  ${a} ${b=a}        -> a a
            //            ${a=b} ${b}        -> ReferenceError: b is not defined b
            //            ${a=fields.b} ${b} -> undefined b

            // Make sure a field that depends on itself does not cause a reference
            // error. It uses its own name as value instead.
            defaultsMap[field.identifier] = field.identifier;

            field.defaultValue = m_templateScript.evaluate(field.defaultValue, defaultsMap).toString();
        }

        defaultsMap[field.identifier] = field.defaultValue;
    }

    // Evaluate function calls and mirror fields, and store the results in
    // their defaultValue property.

    for (auto &field : m_fields) {
        if (field.kind == TemplateField::FunctionCall) {
            field.defaultValue = m_templateScript.evaluate(field.identifier, defaultsMap).toString();
        } else if (field.kind == TemplateField::Mirror) {
            field.defaultValue = defaultsMap[field.identifier].toString();
        }
    }

    // group all changes into one undo transaction
    KTextEditor::Document::EditingTransaction t(doc());

    for (const auto &field : m_fields) {
        // All ranges are static by default, as prepared in setupFieldRanges().
        // Dynamic behaviors are set in updateRangeBehaviours() after initialization is finished.
        field.range->setInsertBehaviors(KTextEditor::MovingRange::ExpandLeft | KTextEditor::MovingRange::ExpandRight);
        doc()->replaceText(field.range->toRange(), field.defaultValue);
        field.range->setInsertBehaviors(KTextEditor::MovingRange::DoNotExpand);
    }
}

void KateTemplateHandler::initializeTemplate()
{
    auto templateString = doc()->text(*m_wholeTemplateRange);
    parseFields(templateString);
    setupFieldRanges();
    setupDefaultValues();
    updateRangeBehaviours();
}

const KateTemplateHandler::TemplateField KateTemplateHandler::fieldForRange(KTextEditor::Range range) const
{
    for (const auto &field : m_fields) {
        if (!field.removed && (field.range->contains(range.start()) || field.range->end() == range.start())) {
            return field;
        }
        if (field.kind == TemplateField::FinalCursorPosition && range.end() == field.range->end().toCursor()) {
            return field;
        }
    }
    return {};
}

const QList<KateTemplateHandler::TemplateField> KateTemplateHandler::fieldsForRange(KTextEditor::Range range) const
{
    QList<KateTemplateHandler::TemplateField> collected;

    for (const auto &field : m_fields) {
        if (field.removed) {
            continue;
        }

        if (range.contains(field.range->toRange()) || field.range->contains(range.start()) || field.range->contains(range.end())
            || field.range->end() == range.start() || field.range->end() == range.end()) {
            collected << field;
        } else if (field.kind == TemplateField::FinalCursorPosition && range.end() == field.range->end()) {
            collected << field;
        }
    }

    return collected;
}

void KateTemplateHandler::updateDependentFields(Document *document, Range range, bool textRemoved)
{
    Q_ASSERT(document == doc());
    Q_UNUSED(document);
    if (!m_undoManager->isActive()) {
        // currently undoing stuff; don't update fields
        return;
    }

    bool in_range = m_wholeTemplateRange->toRange().contains(range.start());
    bool at_end = m_wholeTemplateRange->toRange().end() == range.end() || m_wholeTemplateRange->toRange().end() == range.start();
    if (m_wholeTemplateRange->toRange().isEmpty() || (!in_range && !at_end)) {
        // edit outside template range, abort
        ifDebug(qCDebug(LOG_KTE) << "edit outside template range, exiting";) deleteLater();
        return;
    }

    if (m_internalEdit || range.isEmpty()) {
        // internal or null edit; for internal edits, don't do anything
        // to prevent unwanted recursion
        return;
    }

    ifDebug(qCDebug(LOG_KTE) << "text changed" << document << range;)

        // group all the changes into one undo transaction
        KTextEditor::Document::EditingTransaction t(doc());

    // find the field which was modified, if any
    sortFields();
    const auto changedField = fieldForRange(range);
    if (changedField.kind == TemplateField::Invalid) {
        // edit not within a field, nothing to do
        ifDebug(qCDebug(LOG_KTE) << "edit not within a field:" << range;) return;
    }
    if (changedField.kind == TemplateField::FinalCursorPosition && doc()->text(changedField.range->toRange()).isEmpty()) {
        // text changed at final cursor position: the user is done, so exit
        // this is not executed when the field's range is not empty: in that case this call
        // is for initial setup and we have to continue below
        ifDebug(qCDebug(LOG_KTE) << "final cursor changed:" << range;) deleteLater();
        return;
    }

    if (textRemoved && !range.onSingleLine()) {
        for (auto &f : m_fields) {
            if ((f.kind == TemplateField::Editable || f.kind == TemplateField::Mirror) && !f.removed
                && (range.contains(f.range->toRange()) && f.range->isEmpty())) {
                f.removed = true;
            }
        }
    }

    // new contents of the changed template field
    const auto &newText = doc()->text(changedField.range->toRange());
    m_internalEdit = true;
    // go through all fields and update the contents of the dependent ones
    for (auto field = m_fields.begin(); field != m_fields.end(); field++) {
        if (field->kind == TemplateField::FinalCursorPosition) {
            // only relevant on first run
            doc()->replaceText(field->range->toRange(), QString());
        }

        if (*field == changedField) {
            // mark that the user changed this field
            field->touched = true;
        }

        // If this is mirrored field with the same identifier as the
        // changed one and the changed one is editable, mirror changes
        // edits to non-editable mirror fields are ignored
        if (field->kind == TemplateField::Mirror && changedField.kind == TemplateField::Editable && field->identifier == changedField.identifier) {
            // editable field changed, mirror changes
            field->dontExpandOthers(m_fields);
            doc()->replaceText(field->range->toRange(), newText);
        } else if (field->kind == TemplateField::FunctionCall) {
            // replace field by result of function call
            field->dontExpandOthers(m_fields);
            // build map of objects in the scope to pass to the function
            auto map = fieldMap();
            const auto &callResult = m_templateScript.evaluate(field->identifier, map);
            doc()->replaceText(field->range->toRange(), callResult.toString());
        }
    }
    m_internalEdit = false;
    updateRangeBehaviours();
}

void KateTemplateHandler::updateRangeBehaviours()
{
    std::sort(m_fields.begin(), m_fields.end(), [](const auto &l, const auto &r) {
        return l.range->toRange().start() < r.range->toRange().start();
    });

    TemplateField *lastField = nullptr;

    for (auto &field : m_fields) {
        auto last = lastField != nullptr ? *(lastField->range) : Range::invalid();

        if (field.removed) {
            // field is removed: it should not receive any changes and should not
            // be considered in relation to other fields
            field.range->setInsertBehaviors(MovingRange::DoNotExpand);
            continue;
        }

        if (field.kind == TemplateField::FinalCursorPosition) {
            // final cursor position never grows
            field.range->setInsertBehaviors(MovingRange::DoNotExpand);
        } else if (field.range->start() <= last.end()) {
            // ranges are adjacent...
            if (field.range->isEmpty() && lastField != nullptr) {
                if (field.kind != TemplateField::Editable && field.kind != TemplateField::FinalCursorPosition
                    && (lastField->kind == TemplateField::Editable || lastField->kind == TemplateField::FinalCursorPosition)) {
                    // ...do not expand the current field to let the previous, more important field expand instead
                    field.range->setInsertBehaviors(MovingRange::DoNotExpand);

                    // ...let the previous field only expand to the right to prevent overlap
                    lastField->range->setInsertBehaviors(MovingRange::ExpandRight);
                } else {
                    // ...do not expand the previous field to let the empty field expand instead
                    lastField->range->setInsertBehaviors(MovingRange::DoNotExpand);

                    // ...expand to both sides to catch new input while the field is empty
                    field.range->setInsertBehaviors(MovingRange::ExpandLeft | MovingRange::ExpandRight);
                }
            } else {
                // ...only expand to the right to prevent overlap
                field.range->setInsertBehaviors(MovingRange::ExpandRight);
            }
        } else {
            // ranges are not adjacent, can grow in both directions
            field.range->setInsertBehaviors(MovingRange::ExpandLeft | MovingRange::ExpandRight);
        }

        lastField = &field;
    }
}

KateScript::FieldMap KateTemplateHandler::fieldMap() const
{
    KateScript::FieldMap map;
    for (const auto &field : m_fields) {
        if (field.kind != TemplateField::Editable) {
            // only editable fields are of interest to the scripts
            continue;
        }
        map.insert(field.identifier, QJSValue(doc()->text(field.range->toRange())));
    }
    return map;
}

KTextEditor::ViewPrivate *KateTemplateHandler::view() const
{
    return m_view;
}

void KateTemplateHandler::TemplateField::dontExpandOthers(const QList<TemplateField> &others) const
{
    for (const auto &i : others) {
        if (this->range != i.range) {
            i.range->setInsertBehaviors(KTextEditor::MovingRange::DoNotExpand);
        } else {
            i.range->setInsertBehaviors(KTextEditor::MovingRange::ExpandLeft | KTextEditor::MovingRange::ExpandRight);
        }
    }
}

QDebug operator<<(QDebug s, const KateTemplateHandler::TemplateField &field)
{
    QDebugStateSaver saver(s);
    s.nospace() << "{" << qSetFieldWidth(12) << Qt::left << field.identifier % QStringLiteral(":") << qSetFieldWidth(0) << "kind=" << field.kind
                << " removed=" << field.removed;

    if (field.range) {
        s.nospace() << "\t" << *field.range << field.range->insertBehaviors();
    }

    s.nospace() << "}";
    return s;
}

#include "moc_katetemplatehandler.cpp"
