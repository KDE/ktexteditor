/*
    SPDX-FileCopyrightText: 2008 Andreas Pakulat <apaku@gmx.de>
    SPDX-FileCopyrightText: 2008-2018 Dominik Haumann <dhaumann@kde.org>
    SPDX-FileCopyrightText: 2017-2018 Friedrich W. H. Kossebau <kossebau@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef KTEXTEDITOR_ANNOTATIONINTERFACE_H
#define KTEXTEDITOR_ANNOTATIONINTERFACE_H

#include <ktexteditor_export.h>

#include <QObject>

class QMenu;

namespace KTextEditor
{
class View;
class AbstractAnnotationItemDelegate;

/*!
 * \class KTextEditor::AnnotationModel
 * \inmodule KTextEditor
 * \inheaderfile KTextEditor/AnnotationInterface
 *
 * \brief A model for providing line annotation information.
 *
 * AnnotationModel is a model-like interface that can be implemented
 * to provide annotation information for each line in a document. It provides
 * means to retrieve several kinds of data for a given line in the document.
 *
 * The public interface of this class is loosely based on the QAbstractItemModel
 * interfaces. It only has a single method to override which is the data()
 * method to provide the actual data for a line and role combination.
 *
 * \since 4.1
 * \sa KTextEditor::AnnotationInterface, KTextEditor::AnnotationViewInterface
 */
class KTEXTEDITOR_EXPORT AnnotationModel : public QObject
{
    Q_OBJECT
public:
    ~AnnotationModel() override;

    /*!
     * \value GroupIdentifierRole
     */
    enum {
        GroupIdentifierRole = Qt::UserRole
    };
    // KF6: add AnnotationModelUserRole = Qt::UserRole + 0x100

    /*!
     * Retrieves the information needed to present the
     * annotation information from the annotation model. The provider
     * should return useful information for the line and the data role.
     *
     * The following roles are supported:
     * \table
     *    \row
     *       \li Qt::DisplayRole
     *       \li a short display text to be placed in the border
     *    \row
     *       \li Qt::TooltipRole
     *       \li a tooltip information, longer text possible
     *    \row
     *       \li Qt::BackgroundRole
     *       \li a brush to be used to paint the background on the border
     *    \row
     *       \li Qt::ForegroundRole
     *       \li a brush to be used to paint the text on the border
     *    \row
     *       \li AnnotationModel::GroupIdentifierRole
     *       \li a string which identifies a group of items which
     *       will be highlighted on mouseover; return the same
     *       string for all items in a group (KDevelop uses a VCS
     *       revision number, for example)
     * \endtable
     *
     * \a line is the line for which the data is to be retrieved
     *
     * \a role identifies which kind of annotation is to be retrieved
     *
     * Returns a QVariant that contains the data for the given role.
     */
    virtual QVariant data(int line, Qt::ItemDataRole role) const = 0; // KF6: use int for role

Q_SIGNALS:
    /*!
     * The model should emit the signal reset() when the text of almost all
     * lines changes. In most cases it is enough to call lineChanged().
     *
     * \note Kate Part implementation details: Whenever reset() is emitted Kate
     *       Part iterates over all lines of the document. Kate Part searches
     *       for the longest text to determine the annotation border's width.
     *
     * \sa lineChanged()
     */
    void reset();

    /*!
     * The model should emit the signal lineChanged() when a line has to be
     * updated.
     *
     * \note Kate Part implementation details: lineChanged() repaints the whole
     *       annotation border automatically.
     */
    void lineChanged(int line);
};

} // namespace KTextEditor

#endif
