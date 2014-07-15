/*
 *  This file is part of the KDE libraries
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
 *
 */

#include "lastchangerecorder.h"
#include "kateviinputmodemanager.h"
#include "katevikeyparser.h"
#include "completionreplayer.h"

using namespace KateVi;

LastChangeRecorder::LastChangeRecorder(KateViInputModeManager *viInputModeManager)
    : m_viInputModeManager(viInputModeManager)
    , m_isReplaying(false)
{
}

LastChangeRecorder::~LastChangeRecorder()
{
}

void LastChangeRecorder::record(const QKeyEvent &e)
{
    if (e.key() != Qt::Key_Shift && e.key() != Qt::Key_Control && e.key() != Qt::Key_Meta && e.key() != Qt::Key_Alt) {
        m_changeLog.append(e);
    }
}

void LastChangeRecorder::dropLast()
{
    Q_ASSERT(!m_changeLog.isEmpty());
    m_changeLog.pop_back();
}

void LastChangeRecorder::clear()
{
    m_changeLog.clear();
}

QString LastChangeRecorder::encodedChanges() const
{
    QString result;

    QList<QKeyEvent> keyLog = m_changeLog;

    for (int i = 0; i < keyLog.size(); i++) {
        int keyCode = keyLog.at(i).key();
        QString text = keyLog.at(i).text();
        int mods = keyLog.at(i).modifiers();
        QChar key;

        if (text.length() > 0) {
            key = text.at(0);
        }

        if (text.isEmpty() || (text.length() == 1 && text.at(0) < 0x20)
                || (mods != Qt::NoModifier && mods != Qt::ShiftModifier)) {
            QString keyPress;

            keyPress.append(QLatin1Char('<'));
            keyPress.append((mods & Qt::ShiftModifier)   ? QLatin1String("s-") : QString());
            keyPress.append((mods & Qt::ControlModifier) ? QLatin1String("c-") : QString());
            keyPress.append((mods & Qt::AltModifier)     ? QLatin1String("a-") : QString());
            keyPress.append((mods & Qt::MetaModifier)    ? QLatin1String("m-") : QString());
            keyPress.append(keyCode <= 0xFF ? QChar(keyCode) : KateViKeyParser::self()->qt2vi(keyCode));
            keyPress.append(QLatin1Char('>'));

            key = KateViKeyParser::self()->encodeKeySequence(keyPress).at(0);
        }

        result.append(key);
    }

    return result;
}

bool LastChangeRecorder::isReplaying() const
{
    return m_isReplaying;
}

void LastChangeRecorder::replay(const QString &commands, const CompletionList &completions)
{
    m_isReplaying = true;
    m_viInputModeManager->completionReplayer()->start(completions);
    m_viInputModeManager->feedKeyPresses(commands);
    m_viInputModeManager->completionReplayer()->stop();
    m_isReplaying = false;
}
