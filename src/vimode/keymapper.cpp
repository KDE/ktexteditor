/*  This file is part of the KDE libraries and the Kate part.
 *
 *  Copyright (C) 2008-2009 Erlend Hamberg <ehamberg@gmail.com>
 *  Copyright (C) 2013 Simon St James <kdedevel@etotheipiplusone.com>
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

#include "kateglobal.h"
#include "globalstate.h"
#include "mappings.h"
#include "katedocument.h"
#include <vimode/keymapper.h>
#include <vimode/keyparser.h>
#include <vimode/inputmodemanager.h>

#include <QTimer>

using namespace KateVi;

KeyMapper::KeyMapper(InputModeManager *kateViInputModeManager, KTextEditor::DocumentPrivate *doc, KTextEditor::ViewPrivate *view)
    : m_viInputModeManager(kateViInputModeManager),
      m_doc(doc),
      m_view(view)
{
    m_mappingTimer = new QTimer(this);
    m_doNotExpandFurtherMappings = false;
    m_timeoutlen = 1000; // FIXME: make configurable
    m_doNotMapNextKeypress = false;
    m_numMappingsBeingExecuted = 0;
    m_isPlayingBackRejectedKeys = false;
    connect(m_mappingTimer, SIGNAL(timeout()), this, SLOT(mappingTimerTimeOut()));
}

void KeyMapper::executeMapping()
{
    m_mappingKeys.clear();
    m_mappingTimer->stop();
    m_numMappingsBeingExecuted++;
    const QString mappedKeypresses = m_viInputModeManager->globalState()->mappings()->get(Mappings::mappingModeForCurrentViMode(m_viInputModeManager->inputAdapter()), m_fullMappingMatch, false, true);
    if (!m_viInputModeManager->globalState()->mappings()->isRecursive(Mappings::mappingModeForCurrentViMode(m_viInputModeManager->inputAdapter()), m_fullMappingMatch)) {
        m_doNotExpandFurtherMappings = true;
    }
    m_doc->editBegin();
    m_viInputModeManager->feedKeyPresses(mappedKeypresses);
    m_doNotExpandFurtherMappings = false;
    m_doc->editEnd();
    m_numMappingsBeingExecuted--;
}

void KeyMapper::playBackRejectedKeys()
{
    m_isPlayingBackRejectedKeys = true;
    const QString mappingKeys = m_mappingKeys;
    m_mappingKeys.clear();
    m_viInputModeManager->feedKeyPresses(mappingKeys);
    m_isPlayingBackRejectedKeys = false;
}

void KeyMapper::setMappingTimeout(int timeoutMS)
{
    m_timeoutlen = timeoutMS;
}

void KeyMapper::mappingTimerTimeOut()
{
    if (!m_fullMappingMatch.isNull()) {
        executeMapping();
    } else {
        playBackRejectedKeys();
    }
    m_mappingKeys.clear();
}

bool KeyMapper::handleKeypress(QChar key)
{
    if (!m_doNotExpandFurtherMappings && !m_doNotMapNextKeypress && !m_isPlayingBackRejectedKeys) {
        m_mappingKeys.append(key);

        bool isPartialMapping = false;
        bool isFullMapping = false;
        m_fullMappingMatch.clear();
        foreach (const QString &mapping, m_viInputModeManager->globalState()->mappings()->getAll(Mappings::mappingModeForCurrentViMode(m_viInputModeManager->inputAdapter()), false, true)) {
            if (mapping.startsWith(m_mappingKeys)) {
                if (mapping == m_mappingKeys) {
                    isFullMapping = true;
                    m_fullMappingMatch = mapping;
                } else {
                    isPartialMapping = true;
                }
            }
        }
        if (isFullMapping && !isPartialMapping) {
            // Great - m_mappingKeys is a mapping, and one that can't be extended to
            // a longer one - execute it immediately.
            executeMapping();
            return true;
        }
        if (isPartialMapping) {
            // Need to wait for more characters (or a timeout) before we decide what to
            // do with this.
            m_mappingTimer->start(m_timeoutlen);
            m_mappingTimer->setSingleShot(true);
            return true;
        }
        // We've been swallowing all the keypresses meant for m_keys for our mapping keys; now that we know
        // this cannot be a mapping, restore them.
        Q_ASSERT(!isPartialMapping && !isFullMapping);
        playBackRejectedKeys();
        return true;
    }
    m_doNotMapNextKeypress = false;
    return false;
}

void KeyMapper::setDoNotMapNextKeypress()
{
    m_doNotMapNextKeypress = true;
}

bool KeyMapper::isExecutingMapping()
{
    return m_numMappingsBeingExecuted > 0;
}

bool KeyMapper::isPlayingBackRejectedKeys()
{
    return m_isPlayingBackRejectedKeys;
}

