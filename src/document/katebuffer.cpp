/*
    SPDX-FileCopyrightText: 2000 Waldo Bastian <bastian@kde.org>
    SPDX-FileCopyrightText: 2002-2004 Christoph Cullmann <cullmann@kde.org>
    SPDX-FileCopyrightText: 2007 Mirko Stocker <me@misto.ch>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "katebuffer.h"
#include "kateautoindent.h"
#include "kateconfig.h"
#include "katedocument.h"
#include "kateglobal.h"
#include "katehighlight.h"
#include "katepartdebug.h"
#include "katesyntaxmanager.h"

#include <KLocalizedString>

#include <QDate>
#include <QFile>
#include <QFileInfo>
#include <QTextCodec>
#include <QTextStream>
#include <QTimer>

/**
 * Initial value for m_maxDynamicContexts
 */
static const int KATE_MAX_DYNAMIC_CONTEXTS = 512;

/**
 * Create an empty buffer. (with one block with one empty line)
 */
KateBuffer::KateBuffer(KTextEditor::DocumentPrivate *doc)
    : Kate::TextBuffer(doc)
    , m_doc(doc)
    , m_brokenEncoding(false)
    , m_tooLongLinesWrapped(false)
    , m_longestLineLoaded(0)
    , m_highlight(nullptr)
    , m_tabWidth(8)
    , m_lineHighlighted(0)
    , m_maxDynamicContexts(KATE_MAX_DYNAMIC_CONTEXTS)
{
}

/**
 * Cleanup on destruction
 */
KateBuffer::~KateBuffer() = default;

void KateBuffer::editStart()
{
    if (!startEditing()) {
        return;
    }
}

void KateBuffer::editEnd()
{
    // not finished, do nothing
    if (!finishEditing()) {
        return;
    }

    // nothing change, OK
    if (!editingChangedBuffer()) {
        return;
    }

    // if we arrive here, line changed should be OK
    Q_ASSERT(editingMinimalLineChanged() != -1);
    Q_ASSERT(editingMaximalLineChanged() != -1);
    Q_ASSERT(editingMinimalLineChanged() <= editingMaximalLineChanged());

    updateHighlighting();
}

void KateBuffer::updateHighlighting()
{
    // no highlighting, nothing to do
    if (!m_highlight) {
        return;
    }

    // if we don't touch the highlighted area => fine
    if (editingMinimalLineChanged() > m_lineHighlighted) {
        return;
    }

    // really update highlighting
    // look one line too far, needed for linecontinue stuff
    doHighlight(editingMinimalLineChanged(), editingMaximalLineChanged() + 1, true);
}

void KateBuffer::clear()
{
    // call original clear function
    Kate::TextBuffer::clear();

    // reset the state
    m_brokenEncoding = false;
    m_tooLongLinesWrapped = false;
    m_longestLineLoaded = 0;

    // back to line 0 with hl
    m_lineHighlighted = 0;
}

bool KateBuffer::openFile(const QString &m_file, bool enforceTextCodec)
{
    // first: setup fallback and normal encoding
    setEncodingProberType(KateGlobalConfig::global()->proberType());
    setFallbackTextCodec(KateGlobalConfig::global()->fallbackCodec());
    setTextCodec(m_doc->config()->codec());

    // setup eol
    setEndOfLineMode((EndOfLineMode)m_doc->config()->eol());

    // NOTE: we do not remove trailing spaces on load. This was discussed
    //       over the years again and again. bugs: 306926, 239077, ...

    // line length limit
    setLineLengthLimit(m_doc->lineLengthLimit());

    // then, try to load the file
    m_brokenEncoding = false;
    m_tooLongLinesWrapped = false;
    m_longestLineLoaded = 0;

    // allow non-existent files without error, if local file!
    // will allow to do "kate newfile.txt" without error messages but still fail if e.g. you mistype a url
    // and it can't be fetched via fish:// or other strange things in kio happen...
    // just clear() + exit with success!

    QFileInfo fileInfo(m_file);
    if (m_doc->url().isLocalFile() && !fileInfo.exists()) {
        clear();
        KTextEditor::Message *message = new KTextEditor::Message(i18nc("short translation, user created new file", "New file"), KTextEditor::Message::Warning);
        message->setPosition(KTextEditor::Message::TopInView);
        message->setAutoHide(1000);
        m_doc->postMessage(message);

        // remember error
        m_doc->m_openingError = true;
        m_doc->m_openingErrorMessage = i18n("The file %1 does not exist.", m_doc->url().toString());
        return true;
    }

    // check if this is a normal file or not, avoids to open char devices or directories!
    // else clear buffer and break out with error
    if (!fileInfo.isFile()) {
        clear();
        return false;
    }

    // try to load
    if (!load(m_file, m_brokenEncoding, m_tooLongLinesWrapped, m_longestLineLoaded, enforceTextCodec)) {
        return false;
    }

    // save back encoding
    m_doc->config()->setEncoding(QString::fromLatin1(textCodec()->name()));

    // set eol mode, if a eol char was found
    if (m_doc->config()->allowEolDetection()) {
        m_doc->config()->setEol(endOfLineMode());
    }

    // generate a bom?
    if (generateByteOrderMark()) {
        m_doc->config()->setBom(true);
    }

    // okay, loading did work
    return true;
}

bool KateBuffer::canEncode()
{
    QTextCodec *codec = m_doc->config()->codec();

    // hardcode some Unicode encodings which can encode all chars
    if ((QString::fromLatin1(codec->name()) == QLatin1String("UTF-8")) || (QString::fromLatin1(codec->name()) == QLatin1String("ISO-10646-UCS-2"))) {
        return true;
    }

    for (int i = 0; i < lines(); i++) {
        if (!codec->canEncode(line(i)->text())) {
            qCDebug(LOG_KTE) << QLatin1String("ENC NAME: ") << codec->name();
            qCDebug(LOG_KTE) << QLatin1String("STRING LINE: ") << line(i)->text();
            qCDebug(LOG_KTE) << QLatin1String("ENC WORKING: FALSE");

            return false;
        }
    }

    return true;
}

bool KateBuffer::saveFile(const QString &m_file)
{
    // first: setup fallback and normal encoding
    setEncodingProberType(KateGlobalConfig::global()->proberType());
    setFallbackTextCodec(KateGlobalConfig::global()->fallbackCodec());
    setTextCodec(m_doc->config()->codec());

    // setup eol
    setEndOfLineMode((EndOfLineMode)m_doc->config()->eol());

    // generate bom?
    setGenerateByteOrderMark(m_doc->config()->bom());

    // try to save
    if (!save(m_file)) {
        return false;
    }

    // no longer broken encoding, or we don't care
    m_brokenEncoding = false;
    m_tooLongLinesWrapped = false;
    m_longestLineLoaded = 0;

    // okay
    return true;
}

void KateBuffer::ensureHighlighted(int line, int lookAhead)
{
    // valid line at all?
    if (line < 0 || line >= lines()) {
        return;
    }

    // already hl up-to-date for this line?
    if (line < m_lineHighlighted) {
        return;
    }

    // update hl until this line + max lookAhead
    int end = qMin(line + lookAhead, lines() - 1);

    // ensure we have enough highlighted
    doHighlight(m_lineHighlighted, end, false);
}

void KateBuffer::wrapLine(const KTextEditor::Cursor position)
{
    // call original
    Kate::TextBuffer::wrapLine(position);

    if (m_lineHighlighted > position.line() + 1) {
        m_lineHighlighted++;
    }
}

void KateBuffer::unwrapLine(int line)
{
    // reimplemented, so first call original
    Kate::TextBuffer::unwrapLine(line);

    if (m_lineHighlighted > line) {
        --m_lineHighlighted;
    }
}

void KateBuffer::setTabWidth(int w)
{
    if ((m_tabWidth != w) && (m_tabWidth > 0)) {
        m_tabWidth = w;

        if (m_highlight && m_highlight->foldingIndentationSensitive()) {
            invalidateHighlighting();
        }
    }
}

void KateBuffer::setHighlight(int hlMode)
{
    KateHighlighting *h = KateHlManager::self()->getHl(hlMode);

    // aha, hl will change
    if (h != m_highlight) {
        bool invalidate = !h->noHighlighting();

        if (m_highlight) {
            invalidate = true;
        }

        m_highlight = h;

        if (invalidate) {
            invalidateHighlighting();
        }

        // inform the document that the hl was really changed
        // needed to update attributes and more ;)
        m_doc->bufferHlChanged();

        // try to set indentation
        if (!h->indentation().isEmpty()) {
            m_doc->config()->setIndentationMode(h->indentation());
        }
    }
}

void KateBuffer::invalidateHighlighting()
{
    m_lineHighlighted = 0;
}

void KateBuffer::doHighlight(int startLine, int endLine, bool invalidate)
{
    // no hl around, no stuff to do
    if (!m_highlight || m_highlight->noHighlighting()) {
        return;
    }

#ifdef BUFFER_DEBUGGING
    QTime t;
    t.start();
    qCDebug(LOG_KTE) << "HIGHLIGHTED START --- NEED HL, LINESTART: " << startLine << " LINEEND: " << endLine;
    qCDebug(LOG_KTE) << "HL UNTIL LINE: " << m_lineHighlighted;
#endif

    // if possible get previous line, otherwise create 0 line.
    Kate::TextLine prevLine = (startLine >= 1) ? plainLine(startLine - 1) : Kate::TextLine();

    // here we are atm, start at start line in the block
    int current_line = startLine;
    int start_spellchecking = -1;
    int last_line_spellchecking = -1;
    bool ctxChanged = false;
    // loop over the lines of the block, from startline to endline or end of block
    // if stillcontinue forces us to do so
    for (; current_line < qMin(endLine + 1, lines()); ++current_line) {
        // handle one line
        ctxChanged = false;
        Kate::TextLine textLine = plainLine(current_line);
        m_highlight->doHighlight(prevLine.get(), textLine.get(), ctxChanged);
        prevLine = textLine;

#ifdef BUFFER_DEBUGGING
        // debug stuff
        qCDebug(LOG_KTE) << "current line to hl: " << current_line;
        qCDebug(LOG_KTE) << "text length: " << textLine->length() << " attribute list size: " << textLine->attributesList().size();

        const QVector<int> &ml(textLine->attributesList());
        for (int i = 2; i < ml.size(); i += 3) {
            qCDebug(LOG_KTE) << "start: " << ml.at(i - 2) << " len: " << ml.at(i - 1) << " at: " << ml.at(i) << " ";
        }
        qCDebug(LOG_KTE);
#endif

        // need we to continue ?
        bool stillcontinue = ctxChanged;
        if (stillcontinue && start_spellchecking < 0) {
            start_spellchecking = current_line;
        } else if (!stillcontinue && start_spellchecking >= 0) {
            last_line_spellchecking = current_line;
        }
    }

    // perhaps we need to adjust the maximal highlighted line
    int oldHighlighted = m_lineHighlighted;
    if (ctxChanged || current_line > m_lineHighlighted) {
        m_lineHighlighted = current_line;
    }

    // tag the changed lines !
    if (invalidate) {
#ifdef BUFFER_DEBUGGING
        qCDebug(LOG_KTE) << "HIGHLIGHTED TAG LINES: " << startLine << current_line;
#endif

        Q_EMIT tagLines({startLine, qMax(current_line, oldHighlighted)});

        if (start_spellchecking >= 0 && lines() > 0) {
            Q_EMIT respellCheckBlock(start_spellchecking,
                                     qMin(lines() - 1, (last_line_spellchecking == -1) ? qMax(current_line, oldHighlighted) : last_line_spellchecking));
        }
    }

#ifdef BUFFER_DEBUGGING
    qCDebug(LOG_KTE) << "HIGHLIGHTED END --- NEED HL, LINESTART: " << startLine << " LINEEND: " << endLine;
    qCDebug(LOG_KTE) << "HL UNTIL LINE: " << m_lineHighlighted;
    qCDebug(LOG_KTE) << "HL DYN COUNT: " << KateHlManager::self()->countDynamicCtxs() << " MAX: " << m_maxDynamicContexts;
    qCDebug(LOG_KTE) << "TIME TAKEN: " << t.elapsed();
#endif
}

std::pair<bool, bool> KateBuffer::isFoldingStartingOnLine(int startLine)
{
    // ensure valid input
    if (startLine < 0 || startLine >= lines()) {
        return {false, false};
    }

    // no highlighting, no folding, ATM
    if (!m_highlight || m_highlight->noHighlighting()) {
        return {false, false};
    }

    // first: get the wanted start line highlighted
    ensureHighlighted(startLine);
    const auto startTextLine = plainLine(startLine);

    // we prefer token based folding
    if (startTextLine->markedAsFoldingStartAttribute()) {
        return {true, false};
    }

    // check for indentation based folding
    if (m_highlight->foldingIndentationSensitive() && (tabWidth() > 0) && startTextLine->highlightingState().indentationBasedFoldingEnabled()
        && !m_highlight->isEmptyLine(startTextLine.get())) {
        // do some look ahead if this line might be a folding start
        // we limit this to avoid runtime disaster
        int linesVisited = 0;
        while (const auto nextLine = plainLine(++startLine)) {
            if (!m_highlight->isEmptyLine(nextLine.get())) {
                const bool foldingStart = startTextLine->indentDepth(tabWidth()) < nextLine->indentDepth(tabWidth());
                return {foldingStart, foldingStart};
            }

            // ensure some sensible limit of look ahead
            constexpr int lookAheadLimit = 64;
            if (++linesVisited > lookAheadLimit) {
                break;
            }
        }
    }

    // no folding start of any kind
    return {false, false};
}

KTextEditor::Range KateBuffer::computeFoldingRangeForStartLine(int startLine)
{
    // check for start, will trigger highlighting, too, and rule out bad lines
    const auto [foldingStart, foldingIndentationSensitive] = isFoldingStartingOnLine(startLine);
    if (!foldingStart) {
        return KTextEditor::Range::invalid();
    }
    const auto startTextLine = plainLine(startLine);

    // now: decided if indentation based folding or not!
    if (foldingIndentationSensitive) {
        // get our start indentation level
        const int startIndentation = startTextLine->indentDepth(tabWidth());

        // search next line with indentation level <= our one
        int lastLine = startLine + 1;
        for (; lastLine < lines(); ++lastLine) {
            // get line
            Kate::TextLine textLine = plainLine(lastLine);

            // indentation higher than our start line? continue
            if (startIndentation < textLine->indentDepth(tabWidth())) {
                continue;
            }

            // empty line? continue
            if (m_highlight->isEmptyLine(textLine.get())) {
                continue;
            }

            // else, break
            break;
        }

        // lastLine is always one too much
        --lastLine;

        // backtrack all empty lines, we don't want to add them to the fold!
        while (lastLine > startLine) {
            if (m_highlight->isEmptyLine(plainLine(lastLine).get())) {
                --lastLine;
            } else {
                break;
            }
        }

        // we shall not fold one-liners
        if (lastLine == startLine) {
            return KTextEditor::Range::invalid();
        }

        // be done now
        return KTextEditor::Range(KTextEditor::Cursor(startLine, 0), KTextEditor::Cursor(lastLine, plainLine(lastLine)->length()));
    }

    // 'normal' attribute based folding, aka token based like '{' BLUB '}'

    // first step: search the first region type, that stays open for the start line
    short openedRegionType = 0;
    int openedRegionOffset = -1;
    {
        // mapping of type to "first" offset of it and current number of not matched openings
        QHash<short, QPair<int, int>> foldingStartToOffsetAndCount;

        // walk over all attributes of the line and compute the matchings
        const auto &startLineAttributes = startTextLine->foldings();
        for (size_t i = 0; i < startLineAttributes.size(); ++i) {
            // folding close?
            if (startLineAttributes[i].foldingValue < 0) {
                // search for this type, try to decrement counter, perhaps erase element!
                QHash<short, QPair<int, int>>::iterator end = foldingStartToOffsetAndCount.find(-startLineAttributes[i].foldingValue);
                if (end != foldingStartToOffsetAndCount.end()) {
                    if (end.value().second > 1) {
                        --(end.value().second);
                    } else {
                        foldingStartToOffsetAndCount.erase(end);
                    }
                }
            }

            // folding open?
            if (startLineAttributes[i].foldingValue > 0) {
                // search for this type, either insert it, with current offset or increment counter!
                QHash<short, QPair<int, int>>::iterator start = foldingStartToOffsetAndCount.find(startLineAttributes[i].foldingValue);
                if (start != foldingStartToOffsetAndCount.end()) {
                    ++(start.value().second);
                } else {
                    foldingStartToOffsetAndCount.insert(startLineAttributes[i].foldingValue, qMakePair(startLineAttributes[i].offset, 1));
                }
            }
        }

        // compute first type with offset
        QHashIterator<short, QPair<int, int>> hashIt(foldingStartToOffsetAndCount);
        while (hashIt.hasNext()) {
            hashIt.next();
            if (openedRegionOffset == -1 || hashIt.value().first < openedRegionOffset) {
                openedRegionType = hashIt.key();
                openedRegionOffset = hashIt.value().first;
            }
        }
    }

    // no opening region found, bad, nothing to do
    if (openedRegionType == 0) {
        return KTextEditor::Range::invalid();
    }

    // second step: search for matching end region marker!
    int countOfOpenRegions = 1;
    for (int line = startLine + 1; line < lines(); ++line) {
        // ensure line is highlighted
        ensureHighlighted(line);
        Kate::TextLine textLine = plainLine(line);

        // search for matching end marker
        const auto &lineAttributes = textLine->foldings();
        for (size_t i = 0; i < lineAttributes.size(); ++i) {
            // matching folding close?
            if (lineAttributes[i].foldingValue == -openedRegionType) {
                --countOfOpenRegions;

                // end reached?
                // compute resulting range!
                if (countOfOpenRegions == 0) {
                    // Don't return a valid range without content!
                    if (line - startLine == 1) {
                        return KTextEditor::Range::invalid();
                    }

                    // return computed range
                    return KTextEditor::Range(KTextEditor::Cursor(startLine, openedRegionOffset), KTextEditor::Cursor(line, lineAttributes[i].offset));
                }
            }

            // matching folding open?
            if (lineAttributes[i].foldingValue == openedRegionType) {
                ++countOfOpenRegions;
            }
        }
    }

    // if we arrive here, the opened range spans to the end of the document!
    return KTextEditor::Range(KTextEditor::Cursor(startLine, openedRegionOffset), KTextEditor::Cursor(lines() - 1, plainLine(lines() - 1)->length()));
}

#include "moc_katebuffer.cpp"
