/* This file is part of the KDE libraries
   Copyright (C) 2001-2014 Christoph Cullmann <cullmann@kde.org>
   Copyright (C) 2005-2014 Dominik Haumann (dhaumann@kde.org)

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

#ifndef KTEXTEDITOR_DOCUMENT_H
#define KTEXTEDITOR_DOCUMENT_H

#include <ktexteditor_export.h>

#include <ktexteditor/attribute.h>
#include <ktexteditor/cursor.h>
#include <ktexteditor/range.h>

// our main baseclass of the KTextEditor::Document
#include <KParts/ReadWritePart>

// the list of views
#include <QList>
#include <QMetaType>

class KConfigGroup;

namespace KTextEditor
{

class DocumentPrivate;
class EditingTransactionPrivate;
class MainWindow;
class Message;
class View;

/**
 * \brief Search flags for use with searchText.
 *
 * Modifies the behavior of searchText.
 * By default it is searched for a case-sensitive plaintext pattern,
 * without processing of escape sequences, with "whole words" off,
 * in forward direction, within a non-block-mode text range.
 *
 * \author Sebastian Pipping \<webmaster@hartwork.org\>
 */
enum SearchOption {
    Default             = 0,      ///< Default settings

    // modes
    Regex               = 1 << 1, ///< Treats the pattern as a regular expression

    // options for all modes
    CaseInsensitive     = 1 << 4, ///< Ignores cases, e.g. "a" matches "A"
    Backwards           = 1 << 5, ///< Searches in backward direction

    // options for plaintext
    EscapeSequences     = 1 << 10, ///< Plaintext mode: Processes escape sequences
    WholeWords          = 1 << 11, ///< Plaintext mode: Whole words only, e.g. @em not &quot;amp&quot; in &quot;example&quot;

    MaxSearchOption     = 1 << 31  ///< Placeholder for binary compatibility
};

Q_DECLARE_FLAGS(SearchOptions, SearchOption)
Q_DECLARE_OPERATORS_FOR_FLAGS(SearchOptions)

/**
 * \brief A KParts derived class representing a text document.
 *
 * Topics:
 *  - \ref doc_intro
 *  - \ref doc_manipulation
 *  - \ref doc_views
 *  - \ref doc_readwrite
 *  - \ref doc_notifications
 *  - \ref doc_recovery
 *  - \ref doc_extensions
 *
 * \section doc_intro Introduction
 *
 * The Document class represents a pure text document providing methods to
 * modify the content and create views. A document can have any number
 * of views, each view representing the same content, i.e. all views are
 * synchronized. Support for text selection is handled by a View and text
 * format attributes by the Attribute class.
 *
 * To load a document call KParts::ReadOnlyPart::openUrl().
 * To reload a document from a file call documentReload(), to save the
 * document call documentSave() or documentSaveAs(). Whenever the modified
 * state of the document changes the signal modifiedChanged() is emitted.
 * Check the modified state with KParts::ReadWritePart::isModified().
 * Further signals are documentUrlChanged(). The encoding can be specified
 * with setEncoding(), however this will only take effect on file reload and
 * file save.
 *
 * \section doc_manipulation Text Manipulation
 *
 * Get the whole content with text() and set new content with setText().
 * Call insertText() or insertLine() to insert new text or removeText()
 * and removeLine() to remove content. Whenever the document's content
 * changed the signal textChanged() is emitted. Additional signals are
 * textInserted() and textRemoved(). Note, that the first line in the
 * document is line 0.
 *
 * A Document provides full undo/redo history.
 * Text manipulation actions can be grouped together to one undo/redo action by
 * using an the class EditingTransaction. You can stack multiple EditingTransaction%s.
 * Internally, the Document has a reference counter. If this reference counter
 * is increased the first time (by creating an instance of EditingTransaction),
 * the signal editingStarted() is emitted. Only when the internal reference counter
 * reaches zero again, the signal editingFinished() and optionally the signal
 * textChanged() are emitted. Whether an editing transaction is currently active
 * can be checked by calling isEditingTransactionRunning().
 *
 * @note The signal editingFinished() is always emitted when the last instance
 *       of EditingTransaction is destroyed. Contrary, the signal textChanged()
 *       is emitted only if text changed. Hence, textChanged() is more accurate
 *       with respect to changes in the Document.
 *
 * Every text editing transaction is also available through the signals
 * lineWrapped(), lineUnwrapped(), textInserted() and textRemoved().
 * However, these signals should be used with care. Please be aware of the
 * following warning:
 *
 * @warning Never change the Document's contents when edit actions are active,
 *          i.e. in between of (foreign) editing transactions. In case you
 *          violate this, the currently active edit action may perform edits
 *          that lead to undefined behavior.
 *
 * \section doc_views Document Views
 *
 * A View displays the document's content. As already mentioned, a document
 * can have any number of views, all synchronized. Get a list of all views
 * with views(). Create a new view with createView(). Every time a new view
 * is created the signal viewCreated() is emitted.
 *
 * \section doc_readwrite Read-Only Mode
 *
 * A Document may be in read-only mode, for instance due to missing file
 * permissions. The read-only mode can be checked with isReadWrite(). Further,
 * the signal readWriteChanged() is emitted whenever the state changes either
 * to read-only mode or to read/write mode. The read-only mode can be controlled
 * with setReadWrite().
 *
 * \section doc_notifications Notifications in Documents and Views
 *
 * A Document has the ability to show a Message to the user in a View.
 * The Message then is shown either the specified View if Message::setView()
 * was called, or in all View%s of the Document.
 *
 * To post a message just create a new Message and send it with postMessage().
 * Further information is available in the API documentation of Message.
 *
 * @see Message
 *
 * \section doc_recovery Crash Recovery for Documents
 *
 * When the system or the application using the editor component crashed
 * with unsaved changes in the Document, the View notifies the user about
 * the lost data and asks, whether the data should be recovered.
 *
 * This Document gives you control over the data recovery process. Use
 * isDataRecoveryAvailable() to check for lost data. If you do not want the
 * editor component to handle the data recovery process automatically, you can
 * either trigger the data recovery by calling recoverData() or discard it
 * through discardDataRecovery().
 *
 * \section doc_extensions Document Extension Interfaces
 *
 * A simple document represents text and provides text manipulation methods.
 * However, a real text editor should support advanced concepts like session
 * support, textsearch support, bookmark/general mark support etc. That is why
 * the KTextEditor library provides several additional interfaces to extend
 * a document's capabilities via multiple inheritance.
 *
 * More information about interfaces for the document can be found in
 * \ref kte_group_doc_extensions.
 *
 * \see KParts::ReadWritePart, KTextEditor::Editor, KTextEditor::View,
 *      KTextEditor::MarkInterface, KTextEditor::ModificationInterface,
 *      KTextEditor::MovingInterface
 * \author Christoph Cullmann \<cullmann@kde.org\>
 */
class KTEXTEDITOR_EXPORT Document : public KParts::ReadWritePart
{
    Q_OBJECT

protected:
    /**
     * Constructor.
     *
     * Create a new document with \p parent.
     *
     * Pass it the internal implementation to store a d-pointer.
     *
     * \param impl d-pointer to use
     * \param parent parent object
     * \see Editor::createDocument()
     */
    Document(DocumentPrivate *impl, QObject *parent);

public:
    /**
     * Virtual destructor.
     */
    virtual ~Document();

    /**
     * \name Manage View%s of this Document
     *
     * \{
     */
public:
    /**
     * Create a new view attached to @p parent.
     * @param parent parent widget
     * @param mainWindow the main window responsible for this view, if any
     * @return the new view
     */
    virtual View *createView(QWidget *parent, KTextEditor::MainWindow *mainWindow = nullptr) = 0;

    /**
     * Returns the views pre-casted to KTextEditor::View%s
     */
    virtual QList<View *> views() const = 0;

Q_SIGNALS:
    /**
     * This signal is emitted whenever the \p document creates a new \p view.
     * It should be called for every view to help applications / plugins to
     * attach to the \p view.
     * \attention This signal should be emitted after the view constructor is
     *            completed, e.g. in the createView() method.
     * \param document the document for which a new view is created
     * \param view the new view
     * \see createView()
     */
    void viewCreated(KTextEditor::Document *document, KTextEditor::View *view);

    //!\}

    /**
     * \name General Information about this Document
     *
     * \{
     */
public:
    /**
     * Get this document's name.
     * The editor part should provide some meaningful name, like some unique
     * "Untitled XYZ" for the document - \e without URL or basename for
     * documents with url.
     * \return readable document name
     */
    virtual QString documentName() const = 0;

    /**
     * Get this document's mimetype.
     * \return mimetype
     */
    virtual QString mimeType() = 0;

    /**
     * Get the git hash of the Document's contents on disk.
     * The returned hash equals the git hash of the file written to disk.
     * If the document is a remote document, the checksum may not be
     * available. In this case, QByteArray::isNull() returns \e true.
     *
     * git hash is defined as:
     *
     * sha1("blob " + filesize + "\0" + filecontent)
     *
     * \return the git hash of the document
     */
    virtual QByteArray checksum() const = 0;

    /*
     * SIGNALS
     * following signals should be emitted by the editor document.
     */
Q_SIGNALS:
    /**
     * This signal is emitted whenever the \p document name changes.
     * \param document document which changed its name
     * \see documentName()
     */
    void documentNameChanged(KTextEditor::Document *document);

    /**
     * This signal is emitted whenever the \p document URL changes.
     * \param document document which changed its URL
     * \see KParts::ReadOnlyPart::url()
     */
    void documentUrlChanged(KTextEditor::Document *document);

    /**
     * This signal is emitted whenever the \p document's buffer changed from
     * either state \e unmodified to \e modified or vice versa.
     *
     * \param document document which changed its modified state
     * \see KParts::ReadWritePart::isModified().
     * \see KParts::ReadWritePart::setModified()
     */
    void modifiedChanged(KTextEditor::Document *document);

    /**
     * This signal is emitted whenever the readWrite state of a document
     * changes.
     * \param document the document whose read/write property changed
     * \see KParts::ReadWritePart::setReadWrite()
     */
    void readWriteChanged(KTextEditor::Document *document);

    /*
     * VERY IMPORTANT: Methods to set and query the current encoding of the
     * document
     */
public:
    /**
     * Set the encoding for this document. This encoding will be used
     * while loading and saving files, it will \e not affect the already
     * existing content of the document, e.g. if the file has already been
     * opened without the correct encoding, this will \e not fix it, you
     * would for example need to trigger a reload for this.
     * \param encoding new encoding for the document, the name must be
     *        accepted by QTextCodec, if an empty encoding name is given, the
     *        part should fallback to its own default encoding, e.g. the
     *        system encoding or the global user settings
     * \return \e true on success, or \e false, if the encoding could not be set.
     * \see encoding()
     */
    virtual bool setEncoding(const QString &encoding) = 0;

    /**
     * Get the current chosen encoding. The return value is an empty string,
     * if the document uses the default encoding of the editor and no own
     * special encoding.
     * \return current encoding of the document
     * \see setEncoding()
     */
    virtual QString encoding() const = 0;

    //!\}

    /**
     * \name File Loading and Saving
     *
     * All this actions cause user interaction in some cases.
     * \{
     */
public:
    /**
     * Reload the current file.
     * The user will be prompted by the part on changes and more and can
     * cancel this action if it can harm.
     * \return \e true if the reload has been done, otherwise \e false. If
     *         the document has no url set, it will just return \e false.
     */
    virtual bool documentReload() = 0;

    /**
     * Save the current file.
     * The user will be asked for a filename if needed and more.
     * \return \e true on success, i.e. the save has been done, otherwise
     *         \e false
     */
    virtual bool documentSave() = 0;

    /**
     * Save the current file to another location.
     * The user will be asked for a filename and more.
     * \return \e true on success, i.e. the save has been done, otherwise
     *         \e false
     */
    virtual bool documentSaveAs() = 0;

    /**
     * True, eg if the file for opening could not be read
     * This doesn't have to handle the KPart job canceled cases.
     * @return was there some problem loading the file?
     */
    bool openingError() const;

    /**
     * Error message if any problem occurred on last load.
     * @return error message what went wrong on loading
     */
    // TODO KF6: Not needed anymore since we show load trouble as KTextEditor::Message.
    //      Remove all code which set m_openingErrorMessage
    QString openingErrorMessage() const;

    /*
     * SIGNALS
     * Following signals should be emitted by the document if the text content
     * is changed.
     */
Q_SIGNALS:
    /**
     * This signal should be emitted after a document has been saved to disk or for remote files uploaded.
     * saveAs should be set to true, if the operation is a save as operation
     */
    void documentSavedOrUploaded(KTextEditor::Document *document, bool saveAs);

    /**
     * Warn anyone listening that the current document is about to close.
     * At this point all of the information is still accessible, such as the text,
     * cursors and ranges.
     *
     * Any modifications made to the document at this point will be lost.
     *
     * \param document the document being closed
     */
    void aboutToClose(KTextEditor::Document *document);

    /**
     * Warn anyone listening that the current document is about to reload.
     * At this point all of the information is still accessible, such as the text,
     * cursors and ranges.
     *
     * Any modifications made to the document at this point will be lost.
     *
     * \param document the document being reloaded
     */
    void aboutToReload(KTextEditor::Document *document);

    /**
     * Emitted after the current document was reloaded.
     * At this point, some information might have been invalidated, like
     * for example the editing history.
     *
     * \param document the document that was reloaded.
     *
     * @since 4.6
     */
    void reloaded(KTextEditor::Document *document);

    //!\}

    /**
     * \name Text Manipulation
     *
     * \{
     */
public:
    /**
     * Editing transaction support.
     *
     * Edit commands during this sequence will be bunched together so that
     * they represent a single undo command in the editor, and so that
     * repaint events do not occur in between.
     *
     * Your application should \e not return control to the event loop while
     * it has an unterminated (i.e. this object is not destructed) editing
     * sequence (result undefined) - so do all of your work in one go!
     *
     * Using this class typically looks as follows:
     * @code
     * void foo() {
     *     KTextEditor::Document::EditingTransaction transaction(document);
     *     // now call editing functions
     *     document->removeText(...)
     *     document->insertText(...)
     * }
     * @endcode
     *
     * Although usually not required, the EditingTransaction additionally
     * allows to manually call finish() and start() in between.
     *
     * @see editingStarted(), editingFinished()
     */
    class KTEXTEDITOR_EXPORT EditingTransaction {
        public:
            /**
             * Constructs the object and starts an editing transaction by
             * calling start().
             *
             * @param document document for the transaction
             * @see start()
             */
            explicit EditingTransaction(Document *document);

            /**
             * Destructs the object and, if needed, finishes a running editing
             * transaction by calling finish().
             *
             * @see finish()
             */
            ~EditingTransaction();

            /**
             * By calling start(), the editing transaction can be started again.
             * This function only is of use in combination with finish().
             *
             * @see finish()
             */
            void start();

            /**
             * By calling finish(), the editing transaction can be finished
             * already before destruction of this instance.
             *
             * @see start()
             */
            void finish();

        private:
            /**
             * no copying allowed
             */
            Q_DISABLE_COPY(EditingTransaction)

            /**
             * private d-pointer
             */
            EditingTransactionPrivate *const d;
    };

    /**
     * Check whether an editing transaction is currently running.
     *
     * @see EditingTransaction
     */
    virtual bool isEditingTransactionRunning() const = 0;

    /*
     * General access to the document's text content.
     */
public:
    /**
     * Get the document content.
     * \return the complete document content
     * \see setText()
     */
    virtual QString text() const = 0;

    /**
     * Get the document content within the given \p range.
     * \param range the range of text to retrieve
     * \param block Set this to \e true to receive text in a visual block,
     *        rather than everything inside \p range.
     * \return the requested text part, or QString() for invalid ranges.
     * \see setText()
     */
    virtual QString text(const Range &range, bool block = false) const = 0;

    /**
     * Get the character at text position \p cursor.
     * \param position the location of the character to retrieve
     * \return the requested character, or QChar() for invalid cursors.
     * \see setText()
     */
    virtual QChar characterAt(const Cursor &position) const = 0;

    /**
     * Get the word at the text position \p cursor.
     * The returned word is defined by the word boundaries to the left and
     * right starting at \p cursor. The algorithm takes highlighting information
     * into account, e.g. a dash ('-') in C++ is interpreted as word boundary,
     * whereas e.g. CSS allows identifiers with dash ('-').
     *
     * If \p cursor is not a valid text position or if there is no word
     * under the requested position \p cursor, an empty string is returned.
     *
     * \param cursor requested cursor position for the word
     * \return the word under the cursor or an empty string if there is no word.
     *
     * \see wordRangeAt(), characterAt()
     */
    virtual QString wordAt(const KTextEditor::Cursor &cursor) const = 0;

    /**
     * Get the text range for the word located under the text position \p cursor.
     * The returned word is defined by the word boundaries to the left and
     * right starting at \p cursor. The algorithm takes highlighting information
     * into account, e.g. a dash ('-') in C++ is interpreted as word boundary,
     * whereas e.g. CSS allows identifiers with dash ('-').
     *
     * If \p cursor is not a valid text position or if there is no word
     * under the requested position \p cursor, an invalid text range is returned.
     * If the text range is valid, it is \e always on a single line.
     *
     * \param cursor requested cursor position for the word
     * \return the Range spanning the word under the cursor or an invalid range if there is no word.
     *
     * \see wordAt(), characterAt(), KTextEditor::Range::isValid()
     */
    virtual KTextEditor::Range wordRangeAt(const KTextEditor::Cursor &cursor) const = 0;

    /**
     * Get whether \p cursor is a valid text position.
     * A cursor position at (line, column) is valid, if
     * - line >= 0 and line < lines() holds, and
     * - column >= 0 and column <= lineLength(column).
     *
     * The text position \p cursor is also invalid if it is inside a Unicode surrogate.
     * Therefore, use this function when iterating over the characters of a line.
     *
     * \param cursor cursor position to check for validity
     * \return true, if \p cursor is a valid text position, otherwise \p false
     *
     * \since 5.0
     */
    virtual bool isValidTextPosition(const KTextEditor::Cursor& cursor) const = 0;

    /**
     * Get the document content within the given \p range.
     * \param range the range of text to retrieve
     * \param block Set this to \e true to receive text in a visual block,
     *        rather than everything inside \p range.
     * \return the requested text lines, or QStringList() for invalid ranges.
     *         no end of line termination is included.
     * \see setText()
     */
    virtual QStringList textLines(const Range &range, bool block = false) const = 0;

    /**
     * Get a single text line.
     * \param line the wanted line
     * \return the requested line, or "" for invalid line numbers
     * \see text(), lineLength()
     */
    virtual QString line(int line) const = 0;

    /**
     * Get the count of lines of the document.
     * \return the current number of lines in the document
     * \see length()
     */
    virtual int lines() const = 0;

    /**
     * Check whether \p line currently contains unsaved data.
     * If \p line contains unsaved data, \e true is returned, otherwise \e false.
     * When the user saves the file, a modified line turns into a \e saved line.
     * In this case isLineModified() returns \e false and in its stead isLineSaved()
     * returns \e true.
     * \param line line to query
     * \see isLineSaved(), isLineTouched()
     * \since 5.0
     */
    virtual bool isLineModified(int line) const = 0;

    /**
     * Check whether \p line currently contains only saved text.
     * Saved text in this case implies that a line was touched at some point
     * by the user and then then changes were either undone or the user saved
     * the file.
     *
     * In case \p line was touched and currently contains only saved data,
     * \e true is returned, otherwise \e false.
     * \param line line to query
     * \see isLineModified(), isLineTouched()
     * \since 5.0
     */
    virtual bool isLineSaved(int line) const = 0;

    /**
     * Check whether \p line was touched since the file was opened.
     * This equals the statement isLineModified() || isLineSaved().
     * \param line line to query
     * \see isLineModified(), isLineSaved()
     * \since 5.0
     */
    virtual bool isLineTouched(int line) const = 0;

    /**
     * End position of the document.
     * \return The last column on the last line of the document
     * \see all()
     */
    virtual Cursor documentEnd() const = 0;

    /**
     * A Range which encompasses the whole document.
     * \return A range from the start to the end of the document
     */
    inline Range documentRange() const {
        return Range(Cursor::start(), documentEnd());
    }

    /**
     * Get the count of characters in the document. A TAB character counts as
     * only one character.
     * \return the number of characters in the document
     * \see lines()
     */
    virtual int totalCharacters() const = 0;

    /**
     * Returns if the document is empty.
     */
    virtual bool isEmpty() const;

    /**
     * Get the length of a given line in characters.
     * \param line line to get length from
     * \return the number of characters in the line or -1 if the line was
     *         invalid
     * \see line()
     */
    virtual int lineLength(int line) const = 0;

    /**
     * Get the end cursor position of line \p line.
     * \param line line
     * \see lineLength(), line()
     */
    inline Cursor endOfLine(int line) const {
        return Cursor(line, lineLength(line));
    }

    /**
     * Set the given text as new document content.
     * \param text new content for the document
     * \return \e true on success, otherwise \e false
     * \see text()
     */
    virtual bool setText(const QString &text) = 0;

    /**
     * Set the given text as new document content.
     * \param text new content for the document
     * \return \e true on success, otherwise \e false
     * \see text()
     */
    virtual bool setText(const QStringList &text) = 0;

    /**
     * Remove the whole content of the document.
     * \return \e true on success, otherwise \e false
     * \see removeText(), removeLine()
     */
    virtual bool clear() = 0;

    /**
     * Insert \p text at \p position.
     * \param position position to insert the text
     * \param text text to insert
     * \param block insert this text as a visual block of text rather than a linear sequence
     * \return \e true on success, otherwise \e false
     * \see setText(), removeText()
     */
    virtual bool insertText(const Cursor &position, const QString &text, bool block = false) = 0;

    /**
     * Insert \p text at \p position.
     * \param position position to insert the text
     * \param text text to insert
     * \param block insert this text as a visual block of text rather than a linear sequence
     * \return \e true on success, otherwise \e false
     * \see setText(), removeText()
     */
    virtual bool insertText(const Cursor &position, const QStringList &text, bool block = false) = 0;

    /**
     * Replace text from \p range with specified \p text.
     * \param range range of text to replace
     * \param text text to replace with
     * \param block replace text as a visual block of text rather than a linear sequence
     * \return \e true on success, otherwise \e false
     * \see setText(), removeText(), insertText()
     */
    virtual bool replaceText(const Range &range, const QString &text, bool block = false);

    /**
     * Replace text from \p range with specified \p text.
     * \param range range of text to replace
     * \param text text to replace with
     * \param block replace text as a visual block of text rather than a linear sequence
     * \return \e true on success, otherwise \e false
     * \see setText(), removeText(), insertText()
     */
    virtual bool replaceText(const Range &range, const QStringList &text, bool block = false);

    /**
     * Remove the text specified in \p range.
     * \param range range of text to remove
     * \param block set this to true to remove a text block on the basis of columns, rather than everything inside \p range
     * \return \e true on success, otherwise \e false
     * \see setText(), insertText()
     */
    virtual bool removeText(const Range &range, bool block = false) = 0;

    /**
     * Insert line(s) at the given line number. The newline character '\\n'
     * is treated as line delimiter, so it is possible to insert multiple
     * lines. To append lines at the end of the document, use
     * \code
     * insertLine( lines(), text )
     * \endcode
     * \param line line where to insert the text
     * \param text text which should be inserted
     * \return \e true on success, otherwise \e false
     * \see insertText()
     */
    virtual bool insertLine(int line, const QString &text) = 0;

    /**
     * Insert line(s) at the given line number. The newline character '\\n'
     * is treated as line delimiter, so it is possible to insert multiple
     * lines. To append lines at the end of the document, use
     * \code
     * insertLine( lines(), text )
     * \endcode
     * \param line line where to insert the text
     * \param text text which should be inserted
     * \return \e true on success, otherwise \e false
     * \see insertText()
     */
    virtual bool insertLines(int line, const QStringList &text) = 0;

    /**
     * Remove \p line from the document.
     * \param line line to remove
     * \return \e true on success, otherwise \e false
     * \see removeText(), clear()
     */
    virtual bool removeLine(int line) = 0;

    /**
     * \brief Searches the given input range for a text pattern.
     *
     * Searches for a text pattern within the given input range.
     * The kind of search performed depends on the \p options
     * used. Use this function for plaintext searches as well as
     * regular expression searches. If no match is found the first
     * (and only) element in the vector return is the invalid range.
     * When searching for regular expressions, the first element holds
     * the range of the full match, the subsequent elements hold
     * the ranges of the capturing parentheses.
     *
     * \param range    Input range to search in
     * \param pattern  Text pattern to search for
     * \param options  Combination of search flags
     * \return         List of ranges (length >=1)
     *
     * \author Sebastian Pipping \<webmaster@hartwork.org\>
     *
     * \since 5.11
     */
    QVector<KTextEditor::Range> searchText(const KTextEditor::Range &range,
                                           const QString &pattern,
                                           const SearchOptions options = Default) const;

    /*
     * SIGNALS
     * Following signals should be emitted by the document if the text content
     * is changed.
     */
Q_SIGNALS:
    /**
     * Editing transaction has started.
     * \param document document which emitted this signal
     */
    void editingStarted(KTextEditor::Document *document);

    /**
     * Editing transaction has finished.
     *
     * @note This signal is emitted also for editing actions that maybe do not
     *       modify the @p document contents (think of having an empty
     *       EditingTransaction). If you want to get notified only
     *       after text really changed, connect to the signal textChanged().
     *
     * \param document document which emitted this signal
     * @see textChanged()
     */
    void editingFinished(KTextEditor::Document *document);

    /**
     * A line got wrapped.
     * \param document document which emitted this signal
     * @param position position where the wrap occurred
     */
    void lineWrapped(KTextEditor::Document *document, const KTextEditor::Cursor &position);

    /**
     * A line got unwrapped.
     * \param document document which emitted this signal
     * @param line line where the unwrap occurred
     */
    void lineUnwrapped(KTextEditor::Document *document, int line);

    /**
     * Text got inserted.
     * \param document document which emitted this signal
     * @param position position where the insertion occurred
     * @param text inserted text
     */
    void textInserted(KTextEditor::Document *document, const KTextEditor::Cursor &position, const QString &text);

    /**
     * Text got removed.
     * \param document document which emitted this signal
     * @param range range where the removal occurred
     * @param text removed text
     */
    void textRemoved(KTextEditor::Document *document, const KTextEditor::Range &range, const QString &text);

    /**
     * The \p document emits this signal whenever its text changes.
     * \param document document which emitted this signal
     * \see text(), textLine()
     */
    void textChanged(KTextEditor::Document *document);

    //!\}

    /**
     * \name Highlighting and Related Information
     *
     * \{
     */
public:
    /**
     * Get the default style of the character located at @p position.
     * If @p position is not a valid text position, the default style
     * DefaultStyle::dsNormal is returned.
     *
     * @note Further information about the colors of default styles depend on
     *       the currently chosen schema. Since each View may have a different
     *       color schema, the color information can be obtained through
     *       View::defaultStyleAttribute() and View::lineAttributes().
     *
     * @param position text position
     * @return default style, see enum KTextEditor::DefaultStyle
     * @see View::defaultStyleAttribute(), View::lineAttributes()
     */
    virtual DefaultStyle defaultStyleAt(const KTextEditor::Cursor &position) const = 0;

    /**
     * Return the name of the currently used mode
     * \return name of the used mode
     * \see modes(), setMode()
     */
    virtual QString mode() const = 0;

    /**
     * Return the name of the currently used mode
     * \return name of the used mode
     * \see highlightingModes(), setHighlightingMode()
     */
    virtual QString highlightingMode() const = 0;

    /**
     * \brief Get all available highlighting modes for the current document.
     *
     * Each document can be highlighted using an arbitrary number of highlighting
     * contexts. This method will return the names for each of the used modes.
     *
     * Example: The "PHP (HTML)" mode includes the highlighting for PHP, HTML, CSS and JavaScript.
     *
     * \return Returns a list of embedded highlighting modes for the current Document.
     *
     * \see KTextEditor::Document::highlightingMode()
     */
    virtual QStringList embeddedHighlightingModes() const = 0;

    /**
     * \brief Get the highlight mode used at a given position in the document.
     *
     * Retrieve the name of the applied highlight mode at a given \p position
     * in the current document.
     *
     * Calling this might trigger re-highlighting up to the given line.
     * Therefore this is not const.
     *
     * \see highlightingModes()
     */
    virtual QString highlightingModeAt(const Cursor &position) = 0;

    /**
     * Return a list of the names of all possible modes
     * \return list of mode names
     * \see mode(), setMode()
     */
    virtual QStringList modes() const = 0;

    /**
     * Return a list of the names of all possible modes
     * \return list of mode names
     * \see highlightingMode(), setHighlightingMode()
     */
    virtual QStringList highlightingModes() const = 0;

    /**
     * Set the current mode of the document by giving its name
     * \param name name of the mode to use for this document
     * \return \e true on success, otherwise \e false
     * \see mode(), modes(), modeChanged()
     */
    virtual bool setMode(const QString &name) = 0;

    /**
     * Set the current mode of the document by giving its name
     * \param name name of the mode to use for this document
     * \return \e true on success, otherwise \e false
     * \see highlightingMode(), highlightingModes(), highlightingModeChanged()
     */
    virtual bool setHighlightingMode(const QString &name) = 0;

    /**
     * Returns the name of the section for a highlight given its index in the highlight
     * list (as returned by highlightModes()).
     *
     * You can use this function to build a tree of the highlight names, organized in sections.
     *
     * \param index the index of the highlight in the list returned by modes()
     */
    virtual QString highlightingModeSection(int index) const = 0;

    /**
     * Returns the name of the section for a mode given its index in the highlight
     * list (as returned by modes()).
     *
     * You can use this function to build a tree of the mode names, organized in sections.
     *
     * \param index the index of the highlight in the list returned by modes()
     */
    virtual QString modeSection(int index) const = 0;

    /*
     * SIGNALS
     * Following signals should be emitted by the document if the mode
     * of the document changes
     */
Q_SIGNALS:
    /**
     * Warn anyone listening that the current document's mode has
     * changed.
     *
     * \param document the document whose mode has changed
     * \see setMode()
     */
    void modeChanged(KTextEditor::Document *document);

    /**
     * Warn anyone listening that the current document's highlighting mode has
     * changed.
     *
     * \param document the document which's mode has changed
     * \see setHighlightingMode()
     */
    void highlightingModeChanged(KTextEditor::Document *document);

    //!\}

    /**
     * \name Printing
     *
     * \{
     */
public:
    /**
     * Print the document. This should result in showing the print dialog.
     *
     * @returns true if document was printed
     */
    virtual bool print() = 0;

    /**
     * Shows the print preview dialog/
     */
    virtual void printPreview() = 0;

    //!\}

    /**
     * \name Showing Interactive Notifications
     *
     * \{
     */
public:
    /**
     * Post @p message to the Document and its View%s.
     * If multiple Message%s are posted, the one with the highest priority
     * is shown first.
     *
     * Usually, you can simply forget the pointer, as the Message is deleted
     * automatically, once it is processed or the document gets closed.
     *
     * If the Document does not have a View yet, the Message is queued and
     * shown, once a View for the Document is created.
     *
     * @param message the message to show
     * @return @e true, if @p message was posted. @e false, if message == 0.
     */
    virtual bool postMessage(Message *message) = 0;

    //!\}

    /**
     * \name Session Configuration
     *
     * \{
     */
public:
    /**
     * Read session settings from the given \p config.
     *
     * Known flags:
     * - \p SkipUrl => do not save/restore the file
     * - \p SkipMode => do not save/restore the mode
     * - \p SkipHighlighting => do not save/restore the highlighting
     * - \p SkipEncoding => do not save/restore the encoding
     *
     * \param config read the session settings from this KConfigGroup
     * \param flags additional flags
     * \see writeSessionConfig()
     */
    virtual void readSessionConfig(const KConfigGroup &config, const QSet<QString> &flags = QSet<QString>()) = 0;

    /**
     * Write session settings to the \p config.
     * See readSessionConfig() for more details about available \p flags.
     *
     * \param config write the session settings to this KConfigGroup
     * \param flags additional flags
     * \see readSessionConfig()
     */
    virtual void writeSessionConfig(KConfigGroup &config, const QSet<QString> &flags = QSet<QString>()) = 0;

    //!\}

    /**
     * \name Crash Recovery
     *
     * \{
     */
public:
    /**
     * Returns whether a recovery is available for the current document.
     *
     * \see recoverData(), discardDataRecovery()
     */
    virtual bool isDataRecoveryAvailable() const = 0;

    /**
     * If recover data is available, calling recoverData() will trigger the
     * recovery of the data. If isDataRecoveryAvailable() returns \e false,
     * calling this function does nothing.
     *
     * \see isDataRecoveryAvailable(), discardDataRecovery()
     */
    virtual void recoverData() = 0;

    /**
     * If recover data is available, calling discardDataRecovery() will discard
     * the recover data and the recover data is lost.
     * If isDataRecoveryAvailable() returns \e false, calling this function
     * does nothing.
     *
     * \see isDataRecoveryAvailable(), recoverData()
     */
    virtual void discardDataRecovery() = 0;

    //!\}

private:
    /**
     * private d-pointer, pointing to the internal implementation
     */
    DocumentPrivate *const d;
};

}

Q_DECLARE_METATYPE(KTextEditor::Document *)

#endif

