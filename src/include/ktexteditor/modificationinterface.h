/*
    SPDX-FileCopyrightText: 2005 Christoph Cullmann <cullmann@kde.org>

    Documentation:
    SPDX-FileCopyrightText: 2005 Dominik Haumann <dhdev@gmx.de>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef KTEXTEDITOR_MODIFICATIONINTERFACE_H
#define KTEXTEDITOR_MODIFICATIONINTERFACE_H

#include <ktexteditor_export.h>

#include <QObject>

namespace KTextEditor
{
class Document;
class View;

/**
 * \class ModificationInterface modificationinterface.h <KTextEditor/ModificationInterface>
 *
 * \brief External modification extension interface for the Document.
 *
 * \ingroup kte_group_doc_extensions
 *
 * \section modiface_intro Introduction
 *
 * The class ModificationInterface provides methods to handle modifications
 * of all opened files caused by external programs. Whenever the
 * modified-on-disk state changes the signal modifiedOnDisk() is emitted
 * along with a ModifiedOnDiskReason. Set the state by calling
 * setModifiedOnDisk(). Whether the Editor should show warning dialogs to
 * inform the user about external modified files can be controlled with
 * setModifiedOnDiskWarning(). The slot modifiedOnDisk() is called to ask
 * the user what to do whenever a file was modified.
 *
 * \section modiface_access Accessing the ModificationInterface
 *
 * The ModificationInterface is supposed to be an extension interface for a
 * Document, i.e. the Document inherits the interface \e provided that the
 * used KTextEditor library implements the interface. Use qobject_cast to
 * access the interface:
 * \code
 * // doc is of type KTextEditor::Document*
 * auto iface = qobject_cast<KTextEditor::ModificationInterface*>(doc);
 *
 * if (iface) {
 *     // the implementation supports the interface
 *     // do stuff
 * } else {
 *     // the implementation does not support the interface
 * }
 * \endcode
 *
 * \see KTextEditor::Document
 * \author Christoph Cullmann \<cullmann@kde.org\>
 */
class KTEXTEDITOR_EXPORT ModificationInterface
{
public:
    ModificationInterface();

    /**
     * Virtual destructor.
     */
    virtual ~ModificationInterface();

public:
    /**
     * Reasons why a document is modified on disk.
     */
    enum ModifiedOnDiskReason {
        OnDiskUnmodified = 0, ///< Not modified
        OnDiskModified = 1, ///< The file was modified by another program
        OnDiskCreated = 2, ///< The file was created by another program
        OnDiskDeleted = 3 ///< The file was deleted
    };

public:
    /**
     * Set the document's modified-on-disk state to \p reason.
     * KTextEditor implementations should emit the signal modifiedOnDisk()
     * along with the reason. When the document is in a clean state again the
     * reason should be ModifiedOnDiskReason::OnDiskUnmodified.
     *
     * \param reason the modified-on-disk reason.
     * \see ModifiedOnDiskReason, modifiedOnDisk()
     */
    virtual void setModifiedOnDisk(ModifiedOnDiskReason reason) = 0;

    /**
     * Control, whether the editor should show a warning dialog whenever a file
     * was modified on disk. If \p on is \e true the editor will show warning
     * dialogs.
     * \param on controls, whether the editor should show a warning dialog for
     *        files modified on disk
     */
    virtual void setModifiedOnDiskWarning(bool on) = 0;

    /*
     * These stuff is implemented as SIGNALS in the real document
     */
public:
    /**
     * This signal is emitted whenever the \p document changed its
     * modified-on-disk state.
     * \param document the Document object that represents the file on disk
     * \param isModified if \e true, the file was modified rather than created
     *        or deleted
     * \param reason the reason why the signal was emitted
     * \see setModifiedOnDisk()
     */
    virtual void modifiedOnDisk(KTextEditor::Document *document, bool isModified, KTextEditor::ModificationInterface::ModifiedOnDiskReason reason) = 0;

private:
    class ModificationInterfacePrivate *const d = nullptr;
};

}

Q_DECLARE_INTERFACE(KTextEditor::ModificationInterface, "org.kde.KTextEditor.ModificationInterface")

#endif
