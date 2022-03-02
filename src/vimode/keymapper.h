/*
    SPDX-FileCopyrightText: 2008-2009 Erlend Hamberg <ehamberg@gmail.com>
    SPDX-FileCopyrightText: 2013 Simon St James <kdedevel@etotheipiplusone.com>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef KATEVI_KEY_MAPPER_H
#define KATEVI_KEY_MAPPER_H

#include <QObject>
#include <ktexteditor_export.h>

class QTimer;

namespace KTextEditor
{
class DocumentPrivate;
class ViewPrivate;
}

namespace KateVi
{
class InputModeManager;

class KTEXTEDITOR_EXPORT KeyMapper : public QObject
{
    Q_OBJECT

public:
    KeyMapper(InputModeManager *kateViInputModeManager, KTextEditor::DocumentPrivate *doc, KTextEditor::ViewPrivate *view);
    bool handleKeypress(QChar key);
    void setMappingTimeout(int timeoutMS);
    void setDoNotMapNextKeypress();
    bool isExecutingMapping() const;
    bool isPlayingBackRejectedKeys() const;

public Q_SLOTS:
    void mappingTimerTimeOut();

private:
    // Will be the mapping used if we decide that no extra mapping characters will be
    // typed, either because we have a mapping that cannot be extended to another
    // mapping by adding additional characters, or we have a mapping and timed out waiting
    // for it to be extended to a longer mapping.
    // (Essentially, this allows us to have mappings that extend each other e.g. "'12" and
    // "'123", and to choose between them.)
    QString m_fullMappingMatch;
    QString m_mappingKeys;
    bool m_doNotExpandFurtherMappings;
    QTimer *m_mappingTimer;
    InputModeManager *m_viInputModeManager;
    KTextEditor::DocumentPrivate *m_doc;
    KTextEditor::ViewPrivate *m_view;
    int m_timeoutlen; // time to wait for the next keypress of a multi-key mapping (default: 1000 ms)
    bool m_doNotMapNextKeypress;
    int m_numMappingsBeingExecuted;
    bool m_isPlayingBackRejectedKeys;

private:
    void executeMapping();
    void playBackRejectedKeys();
};

}

#endif /* KATEVI_KEY_MAPPER_H */
