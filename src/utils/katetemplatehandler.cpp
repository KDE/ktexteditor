/*
    SPDX-FileCopyrightText: 2004, 2010 Joseph Wenninger <jowenn@kde.org>
    SPDX-FileCopyrightText: 2009 Milian Wolff <mail@milianw.de>
    SPDX-FileCopyrightText: 2014 Sven Brauch <svenbrauch@gmail.com>
    SPDX-FileCopyrightText: 2025 Mirian Margiani <mixosaurus+kde@pm.me>

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
            // insertText() fails if the document is read only. That is already
            // being checked before the template handler is created, so this
            // code should be unreachable.
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
    qsizetype nextId = 0;

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
        f.id = nextId;
        nextId++;
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

        if (f.kind != TemplateField::FinalCursorPosition && f.kind != TemplateField::FunctionCall) {
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
    std::stable_sort(stripBackslashes.begin(), stripBackslashes.end(), [](const Range l, const Range r) {
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

    // initialize static ranges
    for (auto &field : m_fields) {
        field.staticRange = field.range->toRange();
    }
}

const QList<KateTemplateHandler::TemplateField> KateTemplateHandler::fieldsForRange(KTextEditor::Range range, bool compareStaticRanges) const
{
    QList<KateTemplateHandler::TemplateField> collected;

    for (const auto &field : m_fields) {
        if (field.removed) {
            continue;
        }

        const auto &fieldRange = (compareStaticRanges ? field.staticRange : field.range->toRange());

        if (range.contains(fieldRange) || fieldRange.contains(range.start()) || fieldRange.contains(range.end()) || fieldRange.end() == range.start()
            || fieldRange.end() == range.end()) {
            collected << field;
        } else if (field.kind == TemplateField::FinalCursorPosition && range.end() == fieldRange.end()) {
            collected << field;
        }
    }

    return collected;
}

void KateTemplateHandler::reoderEmptyAdjacentFields(const QList<TemplateField> &changedFields)
{
    // This function is an elaborate workaround for adjacent (mirror) fields losing order
    // when their contents are replaced.
    //
    // Consider this:
    //        ${foo}${foo=100}${foo}  =>  100100100
    //   IDs:   0     1         2
    //
    // Field #1 is the only editable field; it is selected when the template
    // is inserted. Its contents will be replaced as soon as you type:
    //
    // 1. contents are deleted: all fields collapse and become empty, sitting at
    //    the same spot. Order: 0, 1, 2
    // 2. new contents are inserted: fiels #1 receives new content and pushes
    //    fields #0 and #2 to the end. Order: 1 (non-empty), 0 (empty), 2 (empty)
    //
    // This function resets the original order to be 0, 1, 2 again.

    // find groups of previously empty fields at the same position
    QList<qsizetype> lastGroup;
    Cursor lastPosition{Cursor::invalid()};

    QMap<qsizetype, TemplateField *> fieldsLookup;
    for (auto &field : m_fields) {
        fieldsLookup.insert(field.id, &field);
    }

    auto processGroup = [&lastPosition, &lastGroup, &fieldsLookup]() {
        auto start = lastPosition;

        for (auto &fieldId : lastGroup) {
            auto &field = *fieldsLookup[fieldId];

            field.range->setRange(start, start + field.range->end() - field.range->start());
            start = field.range->end();
        }

        lastGroup.clear();
    };

    for (qsizetype i = 0; i < changedFields.size(); ++i) {
        auto &field = changedFields[i];

        if (field.staticRange.isEmpty() && field.staticRange.start() == lastPosition) {
            lastGroup.push_back(field.id);
        } else {
            processGroup();
            lastPosition = field.staticRange.start();

            if (field.staticRange.isEmpty()) {
                lastGroup.push_back(field.id);
            }
        }
    }

    processGroup();
}

void KateTemplateHandler::updateDependentFields(Document *document, Range range, bool textRemoved)
{
    Q_ASSERT(document == doc());
    Q_UNUSED(document);
    if (!m_undoManager->isActive()) {
        // currently undoing stuff; don't update fields
        return;
    }

    if (m_internalEdit || range.isEmpty()) {
        // internal or null edit; for internal edits, don't do anything
        // to prevent unwanted recursion
        return;
    }

    bool in_range = m_wholeTemplateRange->toRange().contains(range.start());
    bool at_end = m_wholeTemplateRange->toRange().end() == range.end() || m_wholeTemplateRange->toRange().end() == range.start();
    if (m_wholeTemplateRange->toRange().isEmpty() || (!in_range && !at_end)) {
        // edit outside template range, abort
        ifDebug(qCDebug(LOG_KTE) << "edit outside template range, exiting";) deleteLater();
        return;
    }

    ifDebug(qCDebug(LOG_KTE) << "text changed" << document << range;)

        // find the fields which were modified, if any
        const auto changedFields = fieldsForRange(range, textRemoved);

    if (changedFields.isEmpty()) {
        ifDebug(qCDebug(LOG_KTE) << "edit did not touch any field:" << range;) return;
    } else if (changedFields.length() == 1 && changedFields[0].kind == TemplateField::FinalCursorPosition) {
        // text changed at final cursor position: the user is done, so exit
        ifDebug(qCDebug(LOG_KTE) << "final cursor changed:" << range;) deleteLater();
    }

    // group all changes into one undo transaction
    KTextEditor::Document::EditingTransaction t(doc());
    m_internalEdit = true; // prevent unwanted recursion

    if (textRemoved) {
        // If text was removed, mark all affected fields that are now empty as removed
        for (auto &field : m_fields) {
            if (field.removed) {
                continue;
            }

            if ((range.start() < field.staticRange.start() && range.end() >= field.staticRange.end()) || !field.staticRange.isValid()) {
                field.removed = true;
            }
        }
    } else {
        // If no text was removed (i.e. if text was inserted), make sure empty fields
        // are sorted correctly before continuing.
        reoderEmptyAdjacentFields(changedFields);
    }

    // Collect new values of changed editable fields
    QMap<QString, QString> mainFieldValues;
    for (const auto &field : changedFields) {
        if (field.kind == TemplateField::Editable) {
            if (field.range->toRange().isValid()) {
                mainFieldValues[field.identifier] = doc()->text(field.range->toRange());
            } else {
                mainFieldValues[field.identifier] = QStringLiteral("");
            }
        }
    }

    // Mark all field ranges as static until we are finished with editing
    for (const auto &field : m_fields) {
        field.range->setInsertBehaviors(KTextEditor::MovingRange::DoNotExpand);
    }

    // - Apply changed main values to mirror fields
    // - Mark changed main fields as edited
    // - Re-run all function fields with new values
    auto jsFields = fieldMap();
    for (auto &field : m_fields) {
        Cursor reset = {Cursor::invalid()};

        if (field.range->start() == view()->cursorPosition()) {
            reset = view()->cursorPosition();
        }

        if (mainFieldValues.contains(field.identifier)) {
            if (field.kind == TemplateField::Editable && mainFieldValues[field.identifier] != field.defaultValue) {
                field.touched = true;
            } else if (field.kind == TemplateField::Mirror) {
                field.range->setInsertBehaviors(KTextEditor::MovingRange::ExpandLeft | KTextEditor::MovingRange::ExpandRight);
                doc()->replaceText(field.range->toRange(), mainFieldValues[field.identifier]);
                field.range->setInsertBehaviors(KTextEditor::MovingRange::DoNotExpand);
            }
        } else if (field.kind == TemplateField::FunctionCall) {
            const auto &callResult = m_templateScript.evaluate(field.identifier, jsFields);
            field.range->setInsertBehaviors(KTextEditor::MovingRange::ExpandLeft | KTextEditor::MovingRange::ExpandRight);
            doc()->replaceText(field.range->toRange(), callResult.toString());
            field.range->setInsertBehaviors(KTextEditor::MovingRange::DoNotExpand);
        }

        if (reset.isValid()) {
            view()->setCursorPosition(reset);
        }
    }

    // Re-apply dynamic range behaviors now that we are done editing
    updateRangeBehaviours();

    // Update saved static ranges
    for (auto &field : m_fields) {
        field.staticRange = field.range->toRange();
    }

    m_internalEdit = false; // enable this slot again
}

void KateTemplateHandler::updateRangeBehaviours()
{
    std::stable_sort(m_fields.begin(), m_fields.end(), [](const auto &l, const auto &r) {
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

                    // ...let the previous field expand to the right
                    lastField->range->setInsertBehaviors(lastField->range->insertBehaviors() | MovingRange::ExpandRight);
                } else {
                    // ...do not expand the previous field to let the empty field expand instead
                    lastField->range->setInsertBehaviors(MovingRange::DoNotExpand);

                    // ...expand to both sides to catch new input while the field is empty
                    field.range->setInsertBehaviors(MovingRange::ExpandLeft | MovingRange::ExpandRight);
                }
            } else if (field.kind == TemplateField::Editable && lastField != nullptr && lastField->kind != TemplateField::Editable) {
                // ...expand to both sides as the current, editable field is more important than the previous field
                field.range->setInsertBehaviors(MovingRange::ExpandLeft | MovingRange::ExpandRight);

                // ...do not expand the previous field to the right
                lastField->range->setInsertBehaviors(lastField->range->insertBehaviors() & ~MovingRange::ExpandRight);
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

#include "moc_katetemplatehandler.cpp"
