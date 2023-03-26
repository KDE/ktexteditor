/*
    SPDX-FileCopyrightText: 2011 Dominik Haumann <dhaumann@kde.org>
    SPDX-FileCopyrightText: 2009-2010 Bernhard Beschow <bbeschow@cs.tu-berlin.de>
    SPDX-FileCopyrightText: 2002 John Firebaugh <jfirebaugh@kde.org>
    SPDX-FileCopyrightText: 2001 Christoph Cullmann <cullmann@kde.org>
    SPDX-FileCopyrightText: 2001 Joseph Wenninger <jowenn@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef kate_undo_h
#define kate_undo_h

#include <QList>

#include <QBitArray>
#include <kateview.h>
#include <ktexteditor/range.h>

class KateUndoManager;
namespace KTextEditor
{
class DocumentPrivate;
}

/**
 * Base class for Kate undo commands.
 */
class KateUndo
{
public:
    /**
     * Constructor
     * @param document the document the undo item belongs to
     */
    explicit KateUndo() = default;

    /**
     * Destructor
     */
    virtual ~KateUndo() = default;

public:
    /**
     * Types for undo items
     */
    enum UndoType { editInsertText, editRemoveText, editWrapLine, editUnWrapLine, editInsertLine, editRemoveLine, editMarkLineAutoWrapped, editInvalid };

public:
    /**
     * Check whether the item is empty.
     *
     * @return whether the item is empty
     */
    virtual bool isEmpty() const;

    /**
     * merge an undo item
     * Saves a bit of memory and potentially many calls when undo/redoing.
     * Only called for equal types of this object and the passed one.
     * @param undo undo item to merge
     * @return success
     */
    virtual bool mergeWith(const KateUndo *undo);

    /**
     * undo this item
     */
    virtual void undo(KTextEditor::DocumentPrivate *doc) = 0;

    /**
     * redo this item
     */
    virtual void redo(KTextEditor::DocumentPrivate *doc) = 0;

    /**
     * type of item
     * @return type
     */
    virtual KateUndo::UndoType type() const = 0;

private:
    //
    // Line modification system
    //
public:
    enum ModificationFlag {
        UndoLine1Modified = 1,
        UndoLine2Modified = 2,
        UndoLine1Saved = 4,
        UndoLine2Saved = 8,
        RedoLine1Modified = 16,
        RedoLine2Modified = 32,
        RedoLine1Saved = 64,
        RedoLine2Saved = 128
    };

    inline void setFlag(ModificationFlag flag)
    {
        m_lineModFlags |= flag;
    }

    inline void unsetFlag(ModificationFlag flag)
    {
        m_lineModFlags &= (~flag);
    }

    inline bool isFlagSet(ModificationFlag flag) const
    {
        return m_lineModFlags & flag;
    }

    virtual void updateUndoSavedOnDiskFlag(QBitArray &lines)
    {
        Q_UNUSED(lines)
    }
    virtual void updateRedoSavedOnDiskFlag(QBitArray &lines)
    {
        Q_UNUSED(lines)
    }

private:
    uchar m_lineModFlags = 0x0;
};

class KateEditInsertTextUndo : public KateUndo
{
public:
    explicit KateEditInsertTextUndo(int line, int col, const QString &text)
        : m_line(line)
        , m_col(col)
        , m_text(text)
    {
    }

    bool isEmpty() const override;
    void undo(KTextEditor::DocumentPrivate *doc) override;
    void redo(KTextEditor::DocumentPrivate *doc) override;
    bool mergeWith(const KateUndo *undo) override;
    KateUndo::UndoType type() const override
    {
        return KateUndo::editInsertText;
    }

protected:
    inline int len() const
    {
        return m_text.length();
    }
    inline int line() const
    {
        return m_line;
    }

private:
    const int m_line;
    const int m_col;
    QString m_text;
};

class KateEditRemoveTextUndo : public KateUndo
{
public:
    explicit KateEditRemoveTextUndo(int line, int col, const QString &text)
        : m_line(line)
        , m_col(col)
        , m_text(text)
    {
    }

    bool isEmpty() const override;
    void undo(KTextEditor::DocumentPrivate *doc) override;
    void redo(KTextEditor::DocumentPrivate *doc) override;
    bool mergeWith(const KateUndo *undo) override;

    KateUndo::UndoType type() const override
    {
        return KateUndo::editRemoveText;
    }

protected:
    inline int len() const
    {
        return m_text.length();
    }
    inline int line() const
    {
        return m_line;
    }

private:
    const int m_line;
    int m_col;
    QString m_text;
};

class KateEditMarkLineAutoWrappedUndo : public KateUndo
{
public:
    explicit KateEditMarkLineAutoWrappedUndo(int line, bool autowrapped)
        : KateUndo()
        , m_line(line)
        , m_autowrapped(autowrapped)
    {
    }

    void undo(KTextEditor::DocumentPrivate *doc) override;
    void redo(KTextEditor::DocumentPrivate *doc) override;
    KateUndo::UndoType type() const override
    {
        return KateUndo::editMarkLineAutoWrapped;
    }

private:
    const int m_line;
    const bool m_autowrapped;
};

class KateEditWrapLineUndo : public KateUndo
{
public:
    explicit KateEditWrapLineUndo(int line, int col, int len, bool newLine)
        : m_line(line)
        , m_col(col)
        , m_len(len)
        , m_newLine(newLine)
    {
    }

    void undo(KTextEditor::DocumentPrivate *doc) override;
    void redo(KTextEditor::DocumentPrivate *doc) override;
    KateUndo::UndoType type() const override
    {
        return KateUndo::editWrapLine;
    }

protected:
    inline int line() const
    {
        return m_line;
    }

private:
    const int m_line;
    const int m_col;
    const int m_len;
    const bool m_newLine;
};

class KateEditUnWrapLineUndo : public KateUndo
{
public:
    explicit KateEditUnWrapLineUndo(int line, int col, int len, bool removeLine)
        : m_line(line)
        , m_col(col)
        , m_len(len)
        , m_removeLine(removeLine)
    {
    }

    void undo(KTextEditor::DocumentPrivate *doc) override;
    void redo(KTextEditor::DocumentPrivate *doc) override;
    KateUndo::UndoType type() const override
    {
        return KateUndo::editUnWrapLine;
    }

protected:
    inline int line() const
    {
        return m_line;
    }

private:
    const int m_line;
    const int m_col;
    const int m_len;
    const bool m_removeLine;
};

class KateEditInsertLineUndo : public KateUndo
{
public:
    explicit KateEditInsertLineUndo(int line, const QString &text)
        : m_line(line)
        , m_text(text)
    {
    }

    void undo(KTextEditor::DocumentPrivate *doc) override;
    void redo(KTextEditor::DocumentPrivate *doc) override;

    KateUndo::UndoType type() const override
    {
        return KateUndo::editInsertLine;
    }

protected:
    inline int line() const
    {
        return m_line;
    }

private:
    const int m_line;
    const QString m_text;
};

class KateEditRemoveLineUndo : public KateUndo
{
public:
    explicit KateEditRemoveLineUndo(int line, const QString &text)
        : m_line(line)
        , m_text(text)
    {
    }

    void undo(KTextEditor::DocumentPrivate *doc) override;
    void redo(KTextEditor::DocumentPrivate *doc) override;
    KateUndo::UndoType type() const override
    {
        return KateUndo::editRemoveLine;
    }

protected:
    inline int line() const
    {
        return m_line;
    }

private:
    const int m_line;
    const QString m_text;
};

/**
 * Class to manage a group of undo items
 */
class KateUndoGroup
{
public:
    /**
     * Constructor
     * @param manager KateUndoManager this undo group will belong to
     */
    explicit KateUndoGroup(const KTextEditor::Cursor cursorPosition,
                           KTextEditor::Range selection,
                           const QVector<KTextEditor::ViewPrivate::PlainSecondaryCursor> &);

    /**
     * Destructor
     */
    ~KateUndoGroup();

    KateUndoGroup(const KateUndoGroup &) = delete;
    KateUndoGroup &operator=(const KateUndoGroup &) = delete;

    KateUndoGroup(KateUndoGroup &&o) = default;
    KateUndoGroup &operator=(KateUndoGroup &&o) = default;

public:
    /**
     * Undo the contained undo items
     */
    void undo(KateUndoManager *manager, KTextEditor::ViewPrivate *view);

    /**
     * Redo the contained undo items
     */
    void redo(KateUndoManager *manager, KTextEditor::ViewPrivate *view);

    void editEnd(const KTextEditor::Cursor cursorPosition,
                 KTextEditor::Range selectionRange,
                 const QVector<KTextEditor::ViewPrivate::PlainSecondaryCursor> &secondaryCursors);

    /**
     * merge this group with an other
     * @param newGroup group to merge into this one
     * @param complex set if a complex undo
     * @return success
     */
    bool merge(KateUndoGroup *newGroup, bool complex);

    /**
     * set group as as savepoint. the next group will not merge with this one
     */
    void safePoint(bool safePoint = true);

    /**
     * is this undogroup empty?
     */
    bool isEmpty() const
    {
        return m_items.isEmpty();
    }

    /**
     * Change all LineSaved flags to LineModified of the line modification system.
     */
    void flagSavedAsModified();

    void markUndoAsSaved(QBitArray &lines);
    void markRedoAsSaved(QBitArray &lines);

    /**
     * Set the undo cursor to @p cursor.
     */
    inline void setUndoCursor(const KTextEditor::Cursor cursor)
    {
        m_undoCursor = cursor;
    }

    /**
     * Set the redo cursor to @p cursor.
     */
    inline void setRedoCursor(const KTextEditor::Cursor cursor)
    {
        m_redoCursor = cursor;
    }

    inline KTextEditor::Cursor redoCursor() const
    {
        return m_redoCursor;
    }

private:
    /**
     * singleType
     * @return the type if it's only one type, or editInvalid if it contains multiple types.
     */
    KateUndo::UndoType singleType() const;

    /**
     * are we only of this type ?
     * @param type type to query
     * @return we contain only the given type
     */
    bool isOnlyType(KateUndo::UndoType type) const;

public:
    /**
     * add an undo item
     * @param u item to add
     */
    void addItem(KateUndo *u);

private:
    /**
     * list of items contained
     */
    QList<KateUndo *> m_items;

    /**
     * prohibit merging with the next group
     */
    bool m_safePoint = false;
    /*
     * Selection Range of primary cursor
     */
    KTextEditor::Range m_undoSelection;
    /*
     * Selection Range of primary cursor
     */
    KTextEditor::Range m_redoSelection;

    /**
     * the cursor position of the active view before the edit step
     */
    KTextEditor::Cursor m_undoCursor;
    /**
     * the cursor positions of the active view before the edit step
     */
    QVector<KTextEditor::ViewPrivate::PlainSecondaryCursor> m_undoSecondaryCursors;

    /**
     * the cursor position of the active view after the edit step
     */
    KTextEditor::Cursor m_redoCursor;
    /**
     * the cursor positions of the active view before the edit step
     */
    QVector<KTextEditor::ViewPrivate::PlainSecondaryCursor> m_redoSecondaryCursors;
};

#endif
