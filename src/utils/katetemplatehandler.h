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

#ifndef _KATE_TEMPLATE_HANDLER_H_
#define _KATE_TEMPLATE_HANDLER_H_

#include <QPointer>
#include <QObject>
#include <QVector>
#include <QMap>
#include <QHash>
#include <QString>
#include <QList>
#include <QRegExp>

#include <katescript.h>

namespace KTextEditor { class DocumentPrivate; }

namespace KTextEditor { class ViewPrivate; }

class KateUndoManager;

namespace KTextEditor
{

class MovingCursor;

class MovingRange;
}

/**
 * \brief Inserts a template and offers advanced snippet features, like navigation and mirroring.
 *
 * For each template inserted a new KateTemplateHandler will be created.
 *
 * The handler has the following features:
 *
 * \li It inserts the template string into the document at the requested position.
 * \li When the template contains at least one variable, the cursor will be placed
 *     at the start of the first variable and its range gets selected.
 * \li When more than one variable exists,TAB and SHIFT TAB can be used to navigate
 *     to the next/previous variable.
 * \li When a variable occurs more than once in the template, edits to any of the
 *     occurrences will be mirroed to the other ones.
 * \li When ESC is pressed, the template handler closes.
 * \li When ALT + RETURN is pressed and a \c %{cursor} variable
 *     exists in the template,the cursor will be placed there. Else the cursor will
 *     be placed at the end of the template.
 *
 * \see KTextEditor::DocumentPrivate::insertTemplateTextImplementation(), KTextEditor::TemplateInterface
 *
 * \author Milian Wolff <mail@milianw.de>
 */

class KateTemplateHandler: public QObject
{
    Q_OBJECT

public:
    /**
     * Setup the template handler, insert the template string.
     *
     * NOTE: The handler deletes itself when required, you do not need to
     *       keep track of it.
     */
    KateTemplateHandler(KTextEditor::ViewPrivate *view,
                        const KTextEditor::Cursor &position,
                        const QString &templateString,
                        const QString& script,
                        KateUndoManager *undoManager);

    /**
     * Cancels the template handler and cleans everything up.
     */
    virtual ~KateTemplateHandler();

protected:
    /**
     * \brief Provide keyboard interaction for the template handler.
     *
     * The event filter handles the following shortcuts:
     *
     * TAB: jump to next editable (i.e. not mirrored) range.
     *      NOTE: this prevents indenting via TAB.
     * SHIFT + TAB: jump to previous editable (i.e. not mirrored) range.
     *      NOTE: this prevents un-indenting via SHIFT + TAB.
     * ESC: terminate template handler (only when no completion is active).
     * ALT + RETURN: accept template and jump to the end-cursor.
     *               if %{cursor} was given in the template, that will be the
     *               end-cursor.
     *               else just jump to the end of the inserted text.
     */
    virtual bool eventFilter(QObject *object, QEvent *event);

private:
    /**
     * Inserts the @p text at @p position and sets an undo point.
     */

    void handleTemplateString();

    void parseFields(const QString& templateText);
    void setupFieldRanges();
    // evaluate default values
    void setupDefaultValues();

    /**
     * Install an event filter on the filter proxy of \p view for
     * navigation between the ranges and terminating the KateTemplateHandler.
     *
     * \see eventFilter()
     */
    void setupEventHandler(KTextEditor::View *view);

    /**
     * Jumps to the previous editable range. If there is none, wrap and jump to the first range.
     *
     * \see jumpToNextRange()
     */
    void jumpToPreviousRange();

    /**
     * Jumps to the next editable range. If there is none, wrap and jump to the last range.
     *
     * \see jumpToPreviousRange()
     */
    void jumpToNextRange();

    /// Helper function for jumpTo{Next,Previous}
    /// if initial is set to true, assumes the cursor is before the snippet
    /// and selects the first field
    void jump(int by, bool initial = false);

    /**
     * Set selection to \p range and move the cursor to its beginning.
     */
    void setCurrentRange(KTextEditor::MovingRange *range);

    /**
     * Syncs the contents of all mirrored ranges for a given variable.
     *
     * \param range The range that acts as base. Its contents will be propagated.
     *              Mirrored ranges can be found as child of a child of \p m_templateRange
     */
    void syncMirroredRanges(KTextEditor::MovingRange *range);

private:
    /**
     * Jumps to the final cursor position. This is either \p m_finalCursorPosition, or
     * if that is not set, the end of \p m_templateRange.
     */
    void jumpToFinalCursorPosition();
    void updateRangeBehaviours();
    void sortFields();

    KTextEditor::DocumentPrivate* doc() const;

private Q_SLOTS:
    /**
     * Saves the range of the inserted template. This is required since
     * tabs could get expanded on insert. While we are at it, we can
     * use it to auto-indent the code after insert.
     */
    void slotTemplateInserted(KTextEditor::Document *document, const KTextEditor::Range &range);

    /**
     * Install event filter on new views.
     */
    void slotViewCreated(KTextEditor::Document *document, KTextEditor::View *view);

    /**
     * Update content of all dependent fields, i.e. mirror or script fields.
     */
    void updateDependentFields(KTextEditor::Document *document, const KTextEditor::Range &oldRange);

public:
    KTextEditor::ViewPrivate* view() const;

private:
    /// The document we operate on.
    KTextEditor::ViewPrivate *m_view;
    /// The undo manager associated with our document
    KateUndoManager* const m_undoManager;

    struct TemplateField {
        QSharedPointer<KTextEditor::MovingRange> range;
        // contents of the field, i.e. identifier or function to call
        QString identifier;
        // default value, if applicable; else empty
        QString defaultValue;
        enum Kind {
            Invalid,
            Editable,
            Mirror,
            FunctionCall,
            FinalCursorPosition
        };
        Kind kind = Invalid;
        // true if this field was edited by the user
        bool touched = false;
        bool operator==(const TemplateField& other) {
            return range == other.range;
        }
    };
    QVector<TemplateField> m_fields;

    TemplateField fieldForRange(const KTextEditor::Range& range) const;
    /// Construct a map of master fields and their current value, for use in scripts.
    KateScript::FieldMap fieldMap() const;

    /// A range that occupies the whole range of the inserted template.
    /// When the cursor moves outside it, the template handler gets closed.
    QSharedPointer<KTextEditor::MovingRange> m_wholeTemplateRange;
    /// The last caret position during editing.
    KTextEditor::Cursor m_lastCaretPosition;

    /// Set to true when we are currently mirroring, to prevent recursion.
    bool m_internalEdit;
    /// template script pointer for the template script, which might be used by the current template
    KateScript m_templateScript;
};

#endif

