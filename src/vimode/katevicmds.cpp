/*  This file is part of the KDE libraries and the Kate part.
 *
 *  Copyright (C) 2003-2005 Anders Lund <anders@alweb.dk>
 *  Copyright (C) 2001-2010 Christoph Cullmann <cullmann@kde.org>
 *  Copyright (C) 2001 Charles Samuels <charles@kde.org>
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

#include "katevicmds.h"

#include "katedocument.h"
#include "kateview.h"
#include "kateglobal.h"
#include "kateviglobal.h"
#include "katevinormalmode.h"
#include "kateviemulatedcommandbar.h"
#include "katecmd.h"
#include "katepartdebug.h"

#include <KLocalizedString>

#include <QDir>
#include <QRegExp>
#include <QUrl>

//BEGIN ViCommands
KateViCommands::ViCommands *KateViCommands::ViCommands::m_instance = 0;

const QStringList &KateViCommands::ViCommands::cmds()
{
    static QStringList l;

    if (l.isEmpty())
        l << mappingCommands() << QLatin1String("d") << QLatin1String("delete") << QLatin1String("j") << QLatin1String("c") << QLatin1String("change") << QLatin1String("<") << QLatin1String(">") << QLatin1String("y") << QLatin1String("yank") <<
          QLatin1String("ma") << QLatin1String("mark") << QLatin1String("k");

    return l;
}

bool KateViCommands::ViCommands::exec(KTextEditor::View *view,
                                    const QString &_cmd,
                                    QString &msg)
{
    return exec(view, _cmd, msg, KTextEditor::Range::invalid());
}

bool KateViCommands::ViCommands::exec(KTextEditor::View *view,
                                    const QString &_cmd,
                                    QString &msg,
                                    const KTextEditor::Range &range)
{
    Q_UNUSED(range)
    // cast it hardcore, we know that it is really a kateview :)
    KTextEditor::ViewPrivate *v = static_cast<KTextEditor::ViewPrivate *>(view);

    if (!v) {
        msg = i18n("Could not access view");
        return false;
    }

    //create a list of args
    QStringList args(_cmd.split(QRegExp(QLatin1String("\\s+")), QString::SkipEmptyParts));
    QString cmd(args.takeFirst());

    // ALL commands that takes no arguments.
    if (mappingCommands().contains(cmd)) {
        if (cmd.endsWith(QLatin1String("unmap"))) {
            if (args.count() == 1) {
                KTextEditor::EditorPrivate::self()->viInputModeGlobal()->removeMapping(modeForMapCommand(cmd), args.at(0));
                return true;
            } else {
                msg = i18n("Missing argument. Usage: %1 <from>",  cmd);
                return false;
            }
        }
        if (args.count() == 1) {
            msg = KTextEditor::EditorPrivate::self()->viInputModeGlobal()->getMapping(modeForMapCommand(cmd), args.at(0), true);
            if (msg.isEmpty()) {
                msg = i18n("No mapping found for \"%1\"", args.at(0));
                return false;
            } else {
                msg = i18n("\"%1\" is mapped to \"%2\"", args.at(0), msg);
            }
        } else if (args.count() == 2) {
            KateViGlobal::MappingRecursion mappingRecursion = (isMapCommandRecursive(cmd)) ? KateViGlobal::Recursive : KateViGlobal::NonRecursive;
            KTextEditor::EditorPrivate::self()->viInputModeGlobal()->addMapping(modeForMapCommand(cmd), args.at(0), args.at(1), mappingRecursion);
        } else {
            msg = i18n("Missing argument(s). Usage: %1 <from> [<to>]",  cmd);
            return false;
        }

        return true;
    }

    KateViNormalMode *nm = v->getViInputModeManager()->getViNormalMode();

    if (cmd == QLatin1String("d") || cmd == QLatin1String("delete") || cmd == QLatin1String("j") ||
            cmd == QLatin1String("c") || cmd == QLatin1String("change") ||  cmd == QLatin1String("<") || cmd == QLatin1String(">") ||
            cmd == QLatin1String("y") || cmd == QLatin1String("yank")) {

        KTextEditor::Cursor start_cursor_position = v->cursorPosition();

        int count = 1;
        if (range.isValid()) {
            count = qAbs(range.end().line() - range.start().line()) + 1;
            v->setCursorPosition(KTextEditor::Cursor(qMin(range.start().line(),
                                 range.end().line()), 0));
        }

        QRegExp number(QLatin1String("^(\\d+)$"));
        for (int i = 0; i < args.count(); i++) {
            if (number.indexIn(args.at(i)) != -1) {
                count += number.cap().toInt() - 1;
            }

            QChar r = args.at(i).at(0);
            if (args.at(i).size() == 1 && ((r >= QLatin1Char('a') && r <= QLatin1Char('z')) || r == QLatin1Char('_') || r == QLatin1Char('+') || r == QLatin1Char('*'))) {
                nm->setRegister(r);
            }
        }

        nm->setCount(count);

        if (cmd == QLatin1String("d") || cmd == QLatin1String("delete")) {
            nm->commandDeleteLine();
        }
        if (cmd == QLatin1String("j")) {
            nm->commandJoinLines();
        }
        if (cmd == QLatin1String("c") || cmd == QLatin1String("change")) {
            nm->commandChangeLine();
        }
        if (cmd == QLatin1String("<")) {
            nm->commandUnindentLine();
        }
        if (cmd == QLatin1String(">")) {
            nm->commandIndentLine();
        }
        if (cmd == QLatin1String("y") || cmd == QLatin1String("yank")) {
            nm->commandYankLine();
            v->setCursorPosition(start_cursor_position);
        }

        // TODO - should we resetParser, here? We'd have to make it public, if so.
        // Or maybe synthesise a KateViCommand to execute instead ... ?
        nm->setCount(0);

        return true;
    }

    if (cmd == QLatin1String("mark") || cmd == QLatin1String("ma") || cmd == QLatin1String("k")) {
        if (args.count() == 0) {
            if (cmd == QLatin1String("mark")) {
                // TODO: show up mark list;
            } else {
                msg = i18n("Wrong arguments");
                return false;
            }
        } else if (args.count() == 1) {

            QChar r = args.at(0).at(0);
            int line;
            if ((r >= QLatin1Char('a') && r <= QLatin1Char('z')) || r == QLatin1Char('_') || r == QLatin1Char('+') || r == QLatin1Char('*')) {
                if (range.isValid()) {
                    line = qMax(range.end().line(), range.start().line());
                } else {
                    line = v->cursorPosition().line();
                }

                v->getViInputModeManager()->addMark(v->doc(), r, KTextEditor::Cursor(line, 0));
            }
        } else {
            msg = i18n("Wrong arguments");
            return false;
        }
        return true;
    }

    // should not happen :)
    msg = i18n("Unknown command '%1'", cmd);
    return false;
}

bool KateViCommands::ViCommands::supportsRange(const QString &range)
{
    static QStringList l;

    if (l.isEmpty())
        l << QLatin1String("d") << QLatin1String("delete") << QLatin1String("j") << QLatin1String("c") << QLatin1String("change") << QLatin1String("<") <<
          QLatin1String(">") << QLatin1String("y") << QLatin1String("yank") << QLatin1String("ma") << QLatin1String("mark") << QLatin1String("k");

    return l.contains(range.split(QLatin1String(" ")).at(0));
}

KCompletion *KateViCommands::ViCommands::completionObject(KTextEditor::View *view, const QString &cmd)
{
    Q_UNUSED(view)

    KTextEditor::ViewPrivate *v = static_cast<KTextEditor::ViewPrivate *>(view);

    if (v && (cmd == QLatin1String("nn") || cmd == QLatin1String("nnoremap"))) {
        QStringList l = KTextEditor::EditorPrivate::self()->viInputModeGlobal()->getMappings(KateViGlobal::NormalModeMapping);

        KateCmdShellCompletion *co = new KateCmdShellCompletion();
        co->setItems(l);
        co->setIgnoreCase(false);
        return co;
    }
    return 0L;
}

const QStringList &KateViCommands::ViCommands::mappingCommands()
{
    static QStringList mappingsCommands;
    if (mappingsCommands.isEmpty()) {
        mappingsCommands << QLatin1String("nmap")  << QLatin1String("nm")  << QLatin1String("noremap") << QLatin1String("nnoremap") << QLatin1String("nn") << QLatin1String("no")
                         << QLatin1String("vmap") << QLatin1String("vm") << QLatin1String("vnoremap") << QLatin1String("vn")
                         << QLatin1String("imap") << QLatin1String("im") << QLatin1String("inoremap") << QLatin1String("ino")
                         << QLatin1String("cmap") << QLatin1String("cm") << QLatin1String("cnoremap") << QLatin1String("cno");

        mappingsCommands << QLatin1String("nunmap") << QLatin1String("vunmap") << QLatin1String("iunmap") << QLatin1String("cunmap");
    }
    return mappingsCommands;
}

KateViGlobal::MappingMode KateViCommands::ViCommands::modeForMapCommand(const QString &mapCommand)
{
    static QMap<QString, KateViGlobal::MappingMode> modeForMapCommand;
    if (modeForMapCommand.isEmpty()) {
        // Normal is the default.
        modeForMapCommand.insert(QLatin1String("vmap"), KateViGlobal::VisualModeMapping);
        modeForMapCommand.insert(QLatin1String("vm"), KateViGlobal::VisualModeMapping);
        modeForMapCommand.insert(QLatin1String("vnoremap"), KateViGlobal::VisualModeMapping);
        modeForMapCommand.insert(QLatin1String("vn"), KateViGlobal::VisualModeMapping);
        modeForMapCommand.insert(QLatin1String("imap"), KateViGlobal::InsertModeMapping);
        modeForMapCommand.insert(QLatin1String("im"), KateViGlobal::InsertModeMapping);
        modeForMapCommand.insert(QLatin1String("inoremap"), KateViGlobal::InsertModeMapping);
        modeForMapCommand.insert(QLatin1String("ino"), KateViGlobal::InsertModeMapping);
        modeForMapCommand.insert(QLatin1String("cmap"), KateViGlobal::CommandModeMapping);
        modeForMapCommand.insert(QLatin1String("cm"), KateViGlobal::CommandModeMapping);
        modeForMapCommand.insert(QLatin1String("cnoremap"), KateViGlobal::CommandModeMapping);
        modeForMapCommand.insert(QLatin1String("cno"), KateViGlobal::CommandModeMapping);

        modeForMapCommand.insert(QLatin1String("nunmap"), KateViGlobal::NormalModeMapping);
        modeForMapCommand.insert(QLatin1String("vunmap"), KateViGlobal::VisualModeMapping);
        modeForMapCommand.insert(QLatin1String("iunmap"), KateViGlobal::InsertModeMapping);
        modeForMapCommand.insert(QLatin1String("cunmap"), KateViGlobal::CommandModeMapping);
    }
    return modeForMapCommand[mapCommand];
}

bool KateViCommands::ViCommands::isMapCommandRecursive(const QString &mapCommand)
{
    static QMap<QString, bool> isMapCommandRecursive;
    {
        isMapCommandRecursive.insert(QLatin1String("nmap"), true);
        isMapCommandRecursive.insert(QLatin1String("nm"), true);
        isMapCommandRecursive.insert(QLatin1String("vmap"), true);
        isMapCommandRecursive.insert(QLatin1String("vm"), true);
        isMapCommandRecursive.insert(QLatin1String("imap"), true);
        isMapCommandRecursive.insert(QLatin1String("im"), true);
        isMapCommandRecursive.insert(QLatin1String("cmap"), true);
        isMapCommandRecursive.insert(QLatin1String("cm"), true);
    }
    return isMapCommandRecursive[mapCommand];
}

//END ViCommands

//BEGIN AppCommands
KateViCommands::AppCommands *KateViCommands::AppCommands::m_instance = 0;

KateViCommands::AppCommands::AppCommands()
    : KTextEditor::Command()
{
    re_write.setPattern(QLatin1String("w")); // temporarily add :w
    //re_write.setPattern("w(a)?");
    //re_quit.setPattern("(w)?q?(a)?");
    //re_exit.setPattern("x(a)?");
    //re_changeBuffer.setPattern("b(n|p)");
    //re_edit.setPattern("e(dit)?");
    //re_new.setPattern("(v)?new");
}

const QStringList &KateViCommands::AppCommands::cmds()
{
    static QStringList l;

    if (l.empty()) {
        //l << "q" << "qa" << "w" << "wq" << "wa" << "wqa" << "x" << "xa"
        //<< "bn" << "bp" << "new" << "vnew" << "e" << "edit" << "enew";
        l << QLatin1String("w");
    }

    return l;
}

// commands that don't need to live in the hosting application should be
// implemented here. things such as quitting and splitting the view can not be
// done from the editor part and needs to be implemented in the hosting
// application.
bool KateViCommands::AppCommands::exec(KTextEditor::View *view,
                                     const QString &cmd, QString &msg)
{
    QStringList args(cmd.split(QRegExp(QLatin1String("\\s+")), QString::SkipEmptyParts));
    QString command(args.takeFirst());
    QString file = args.join(QLatin1Char(' '));

    if (re_write.exactMatch(command)) { // w, wa
        /*        if (!re_write.cap(1).isEmpty()) { // [a]ll
            view->document()->saveAll();
            msg = i18n("All documents written to disk");
        } else { // w*/
        // Save file
        if (file.isEmpty()) {
            view->document()->documentSave();
        } else {
            QUrl base = view->document()->url();
            if (!base.isValid()) {
                base = QUrl(QDir::homePath());
            }

            QUrl url = base.resolved(QUrl(file));

            view->document()->saveAs(url);
        }
        msg = i18n("Document written to disk");
    }

    return true;
}

bool KateViCommands::AppCommands::help(KTextEditor::View *view, const QString &cmd, QString &msg)
{
    Q_UNUSED(view);

    if (re_write.exactMatch(cmd)) {
        msg = i18n("<p><b>w/wa &mdash; write document(s) to disk</b></p>"
                   "<p>Usage: <tt><b>w[a]</b></tt></p>"
                   "<p>Writes the current document(s) to disk. "
                   "It can be called in two ways:<br />"
                   " <tt>w</tt> &mdash; writes the current document to disk<br />"
                   " <tt>wa</tt> &mdash; writes all document to disk.</p>"
                   "<p>If no file name is associated with the document, "
                   "a file dialog will be shown.</p>");
        return true;
    }
    return false;
}
//END AppCommands

//BEGIN SedReplace
KateViCommands::SedReplace *KateViCommands::SedReplace::m_instance = 0;

static int backslashString(const QString &haystack, const QString &needle, int index)
{
    int len = haystack.length();
    int searchlen = needle.length();
    bool evenCount = true;
    while (index < len) {
        if (haystack[index] == QLatin1Char('\\')) {
            evenCount = !evenCount;
        } else {
            // isn't a slash
            if (!evenCount) {
                if (haystack.mid(index, searchlen) == needle) {
                    return index - 1;
                }
            }
            evenCount = true;
        }
        ++index;

    }

    return -1;
}

// exchange "\t" for the actual tab character, for example
static void exchangeAbbrevs(QString &str)
{
    // the format is (findreplace)*[nullzero]
    const char *magic = "a\x07t\tn\n";

    while (*magic) {
        int index = 0;
        char replace = magic[1];
        while ((index = backslashString(str, QString(QChar::fromLatin1(*magic)), index)) != -1) {
            str.replace(index, 2, QChar::fromLatin1(replace));
            ++index;
        }
        ++magic;
        ++magic;
    }
}

bool KateViCommands::SedReplace::exec(KTextEditor::View *view, const QString &cmd, QString &msg)
{
    return exec(view, cmd, msg, KTextEditor::Range::invalid());
}

bool KateViCommands::SedReplace::exec(class KTextEditor::View *view, const QString &cmd,
                                    QString &msg, const KTextEditor::Range &r)
{
    qCDebug(LOG_PART) << "SedReplace::execCmd( " << cmd << " )";
    if (r.isValid()) {
        qCDebug(LOG_PART) << "Range: " << r;
    }

    int findBeginPos = -1;
    int findEndPos = -1;
    int replaceBeginPos = -1;
    int replaceEndPos = -1;
    QString delimiter;
    if (!parse(cmd, delimiter, findBeginPos, findEndPos, replaceBeginPos, replaceEndPos)) {
        return false;
    }

    const QString searchParamsString = cmd.mid(cmd.lastIndexOf(delimiter));
    const bool noCase = searchParamsString.contains(QLatin1Char('i'));
    const bool repeat = searchParamsString.contains(QLatin1Char('g'));
    const bool interactive = searchParamsString.contains(QLatin1Char('c'));

    QString find = cmd.mid(findBeginPos, findEndPos - findBeginPos + 1);
    qCDebug(LOG_PART) << "SedReplace: find =" << find;

    QString replace = cmd.mid(replaceBeginPos, replaceEndPos - replaceBeginPos + 1);
    exchangeAbbrevs(replace);
    qCDebug(LOG_PART) << "SedReplace: replace =" << replace;

    if (find.isEmpty()) {
        // Nothing to do.
        return true;
    }

    KTextEditor::ViewPrivate *kateView = static_cast<KTextEditor::ViewPrivate *>(view);
    KTextEditor::DocumentPrivate *doc = kateView->doc();
    if (!doc) {
        return false;
    }
    // Only current line ...
    int startLine = kateView->cursorPosition().line();
    int endLine = kateView->cursorPosition().line();
    // ... unless a range was provided.
    if (r.isValid()) {
        startLine = r.start().line();
        endLine = r.end().line();
    }

    QSharedPointer<InteractiveSedReplacer> interactiveSedReplacer(new InteractiveSedReplacer(doc, find, replace, !noCase, !repeat, startLine, endLine));

    if (interactive) {
        if (kateView->viInputMode()) {
            KateViEmulatedCommandBar *emulatedCommandBar = kateView->viModeEmulatedCommandBar();
            emulatedCommandBar->startInteractiveSearchAndReplace(interactiveSedReplacer);
            return true;
        } else {
            qCDebug(LOG_PART) << "Interactive sedreplace is only currently supported with Vi mode plus Vi emulated command bar.";
            return false;
        }
    }

    kateView->setSearchPattern(find);

    interactiveSedReplacer->replaceAllRemaining();
    msg = interactiveSedReplacer->finalStatusReportMessage();

    return true;
}

bool KateViCommands::SedReplace::parse(const QString &sedReplaceString, QString &destDelim, int &destFindBeginPos, int &destFindEndPos, int &destReplaceBeginPos, int &destReplaceEndPos)
{
    // valid delimiters are all non-word, non-space characters plus '_'
    QRegExp delim(QLatin1String("^s\\s*([^\\w\\s]|_)"));
    if (delim.indexIn(sedReplaceString) < 0) {
        return false;
    }

    QString d = delim.cap(1);
    qCDebug(LOG_PART) << "SedReplace: delimiter is '" << d << "'";

    QRegExp splitter(QString::fromLatin1("^s\\s*") + d + QLatin1String("((?:[^\\\\\\") + d + QLatin1String("]|\\\\.)*)\\")
                     + d + QLatin1String("((?:[^\\\\\\") + d + QLatin1String("]|\\\\.)*)(\\") + d + QLatin1String("[igc]{0,3})?$"));
    if (splitter.indexIn(sedReplaceString) < 0) {
        return false;
    }

    const QString find = splitter.cap(1);
    const QString replace = splitter.cap(2);

    destDelim = d;
    destFindBeginPos = splitter.pos(1);
    destFindEndPos = splitter.pos(1) + find.length() - 1;
    destReplaceBeginPos = splitter.pos(2);
    destReplaceEndPos = splitter.pos(2) + replace.length() - 1;

    return true;
}

KateViCommands::SedReplace::InteractiveSedReplacer::InteractiveSedReplacer(KTextEditor::DocumentPrivate *doc, const QString &findPattern, const QString &replacePattern, bool caseSensitive, bool onlyOnePerLine, int startLine, int endLine)
    : m_findPattern(findPattern),
      m_replacePattern(replacePattern),
      m_onlyOnePerLine(onlyOnePerLine),
      m_startLine(startLine),
      m_endLine(endLine),
      m_doc(doc),
      m_regExpSearch(doc, caseSensitive ? Qt::CaseSensitive : Qt::CaseInsensitive),
      m_numReplacementsDone(0),
      m_numLinesTouched(0),
      m_lastChangedLineNum(-1)
{
    m_currentSearchPos = Cursor(startLine, 0);
}

Range KateViCommands::SedReplace::InteractiveSedReplacer::currentMatch()
{
    QVector<Range> matches = fullCurrentMatch();

    if (matches.isEmpty()) {
        return Range::invalid();
    }

    if (matches.first().start().line() > m_endLine) {
        return Range::invalid();
    }

    return matches.first();
}

void KateViCommands::SedReplace::InteractiveSedReplacer::skipCurrentMatch()
{
    const Range currentMatch = this->currentMatch();
    m_currentSearchPos = currentMatch.end();
    if (m_onlyOnePerLine && currentMatch.start().line() == m_currentSearchPos.line()) {
        m_currentSearchPos = Cursor(m_currentSearchPos.line() + 1, 0);
    }
}

void KateViCommands::SedReplace::InteractiveSedReplacer::replaceCurrentMatch()
{
    const Range currentMatch = this->currentMatch();
    const QString currentMatchText = m_doc->text(currentMatch);
    const QString replacementText = replacementTextForCurrentMatch();

    m_doc->editBegin();
    m_doc->removeText(currentMatch);
    m_doc->insertText(currentMatch.start(), replacementText);
    m_doc->editEnd();

    // Begin next search from directly after replacement.
    if (!replacementText.contains(QLatin1Char('\n'))) {
        const int moveChar = currentMatch.isEmpty() ? 1 : 0; // if the search was for \s*, make sure we advance a char
        const int col = currentMatch.start().column() + replacementText.length() + moveChar;

        m_currentSearchPos = Cursor(currentMatch.start().line(), col);
    } else {
        m_currentSearchPos = Cursor(currentMatch.start().line() + replacementText.count(QLatin1Char('\n')),
                                    replacementText.length() - replacementText.lastIndexOf(QLatin1Char('\n')) - 1);
    }
    if (m_onlyOnePerLine) {
        // Drop down to next line.
        m_currentSearchPos = Cursor(m_currentSearchPos.line() + 1, 0);
    }

    // Adjust end line down by the number of new newlines just added, minus the number taken away.
    m_endLine += replacementText.count(QLatin1Char('\n'));
    m_endLine -= currentMatchText.count(QLatin1Char('\n'));

    m_numReplacementsDone++;
    if (m_lastChangedLineNum != currentMatch.start().line()) {
        // Counting "swallowed" lines as being "touched".
        m_numLinesTouched += currentMatchText.count(QLatin1Char('\n')) + 1;
    }
    m_lastChangedLineNum = m_currentSearchPos.line();
}

void KateViCommands::SedReplace::InteractiveSedReplacer::replaceAllRemaining()
{
    m_doc->editBegin();
    while (currentMatch().isValid()) {
        replaceCurrentMatch();
    }
    m_doc->editEnd();
}

QString KateViCommands::SedReplace::InteractiveSedReplacer::currentMatchReplacementConfirmationMessage()
{
    return i18n("replace with %1?", replacementTextForCurrentMatch().replace(QLatin1Char('\n'), QLatin1String("\\n")));
}

QString KateViCommands::SedReplace::InteractiveSedReplacer::finalStatusReportMessage()
{
    return i18ncp("%2 is the translation of the next message",
                  "1 replacement done on %2", "%1 replacements done on %2", m_numReplacementsDone,
                  i18ncp("substituted into the previous message",
                         "1 line", "%1 lines", m_numLinesTouched));

}

const QVector< Range > KateViCommands::SedReplace::InteractiveSedReplacer::fullCurrentMatch()
{
    if (m_currentSearchPos > m_doc->documentEnd()) {
        return QVector<Range>();
    }

    return m_regExpSearch.search(m_findPattern, Range(m_currentSearchPos, m_doc->documentEnd()));
}

QString KateViCommands::SedReplace::InteractiveSedReplacer::replacementTextForCurrentMatch()
{
    const QVector<KTextEditor::Range> captureRanges = fullCurrentMatch();
    QStringList captureTexts;
    foreach (const Range &captureRange, captureRanges) {
        captureTexts << m_doc->text(captureRange);
    }
    const QString replacementText = m_regExpSearch.buildReplacement(m_replacePattern, captureTexts, 0);
    return replacementText;

}

//END SedReplace
