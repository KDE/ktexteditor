/*
    SPDX-FileCopyrightText: 2024 Jonathan Poelen <jonathan.poelen@gmail.com>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "kateconfig.h"
#include "katedocument.h"
#include "katepartdebug.h"
#include "kateview.h"
#include "scripttester_p.h"

#include <algorithm>

#include <QFile>
#include <QFileInfo>
#include <QJSEngine>
#include <QLatin1StringView>
#include <QVarLengthArray>

namespace KTextEditor
{

using namespace Qt::Literals::StringLiterals;

namespace
{

/**
 * UDL for QStringView
 */
constexpr QStringView operator""_sv(const char16_t *str, size_t size) noexcept
{
    return QStringView(str, size);
}

/**
 * Search for a file \p name in the folder list \p dirs.
 * @param engine[out] used for set an exception
 * @param name name if file
 * @param dirs list of folders to search for the file
 * @param error[out] used for set an error
 * @return the path of the file found. Otherwise, an empty string is returned,
 * an error is set in \p error and an exception in \p engine.
 */
static QString getPath(QJSEngine *engine, const QString &name, const QStringList &dirs, QString *error)
{
    for (const QString &dir : dirs) {
        QString path = dir % u'/' % name;
        if (QFile::exists(path)) {
            return path;
        }
    }

    *error = u"file '%1' not found in %2"_sv.arg(name, dirs.join(u", "_sv));
    engine->throwError(QJSValue::URIError, *error);
    return QString();
}

/**
 * Same as \c getPath, but also searches in current working directory.
 * @param engine[out] used for set an exception
 * @param fileName name if file
 * @param dirs list of folders to search for the file
 * @param error[out] used for set an error
 * @return the path of the file found. Otherwise, an empty string is returned,
 * an error is set in \p error and an exception in \p engine.
 */
static QString getModulePath(QJSEngine *engine, const QString &fileName, const QStringList &dirs, QString *error)
{
    if (!dirs.isEmpty()) {
        if (QFileInfo(fileName).isRelative()) {
            for (const QString &dir : dirs) {
                QString path = dir % u'/' % fileName;
                if (QFile::exists(path)) {
                    return path;
                }
            }
        }
    }

    if (QFile::exists(fileName)) {
        return fileName;
    }

    if (dirs.isEmpty()) {
        *error = u"file '%1' not found in working directory"_sv.arg(fileName);
    } else {
        *error = u"file '%1' not found in %2 and working directory"_sv.arg(fileName, dirs.join(u", "_sv));
    }

    engine->throwError(QJSValue::URIError, *error);
    return QString();
}

/**
 * Search for a file \p name in the folder list \p dirs.
 * @param engine[out] used for set an exception
 * @param sourceUrl file path
 * @param content[out] file contents
 * @param error[out] used for set an error
 * @return \c true when reading is complete, otherwise \c false
 */
static bool readFile(QJSEngine *engine, const QString &sourceUrl, QString *content, QString *error)
{
    QFile file(sourceUrl);
    if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QTextStream stream(&file);
        *content = stream.readAll();
        return true;
    }

    *error = u"reading error for '%1': %2"_sv.arg(sourceUrl, file.errorString());
    engine->throwError(QJSValue::URIError, *error);
    return false;
}

/**
 * Write a line with "^~~" at the position \c column.
 */
static void writeCarretLine(QTextStream &stream, const ScriptTester::Colors &colors, int column)
{
    stream.setPadChar(u' ');
    stream.setFieldWidth(column);
    stream << ""_L1;
    stream.setFieldWidth(0);
    stream << colors.carret;
    stream << "^~~"_L1;
    stream << colors.reset;
    stream << '\n';
}

/**
 * Write a label and adds color when \p colored is \c true.
 */
static void writeLabel(QTextStream &stream, const ScriptTester::Colors &colors, bool colored, QLatin1StringView text)
{
    if (colored) {
        stream << colors.labelInfo << text << colors.reset;
    } else {
        stream << text;
    }
}

/**
 * When property \p name of \p obj is set, convert it to a string and call \p setFn.
 */
template<class SetFn>
static void readString(const QJSValue &obj, const QString &name, SetFn &&setFn)
{
    auto value = obj.property(name);
    if (!value.isUndefined()) {
        setFn(value.toString());
    }
}

/**
 * When property \p name of \p obj is set, convert it to a int and call \p setFn.
 */
template<class SetFn>
static void readInt(const QJSValue &obj, const QString &name, SetFn &&setFn)
{
    auto value = obj.property(name);
    if (!value.isUndefined()) {
        setFn(value.toInt());
    }
}

/**
 * When property \p name of \p obj is set, convert it to a bool and call \p setFn.
 */
template<class SetFn>
static void readBool(const QJSValue &obj, const QString &name, SetFn &&setFn)
{
    auto value = obj.property(name);
    if (!value.isUndefined()) {
        setFn(value.toBool());
    }
}

/**
 * @return the position where \p a differs from \p b.
 */
static qsizetype computeOffsetDifference(QStringView a, QStringView b)
{
    qsizetype n = qMin(a.size(), b.size());
    qsizetype i = 0;
    for (; i < n; ++i) {
        if (a[i] != b[i]) {
            if (a[i].isLowSurrogate() && b[i].isLowSurrogate()) {
                return qMax(0, i - 1);
            }
            return i;
        }
    }
    return i;
}

struct VirtualText {
    qsizetype pos1;
    qsizetype pos2;

    qsizetype size() const
    {
        return pos2 - pos1;
    }
};

/**
 * Search for the next element representing virtual text.
 * If none are found, pos1 and pos2 are set to -1.
 */
static VirtualText findVirtualText(QStringView str, qsizetype pos, QChar c)
{
    VirtualText res{str.indexOf(c, pos), -1};
    if (res.pos1 != -1) {
        res.pos2 = res.pos1 + 1;
        while (res.pos2 < str.size() && str[res.pos2] == c) {
            ++res.pos2;
        }
    }
    return res;
};

/**
 * @return the length of the prefix added by Qt when a file is displayed in a js exception.
 */
static qsizetype filePrefixLen(QStringView str)
{
    // skip file prefix
    if (str.startsWith("file://"_L1)) {
        return 7;
    } else if (str.startsWith("file:"_L1)) {
        return 5;
    }
    return 0;
}

/**
 * @return \p str without its file prefix (\see filePrefixLen).
 */
static QStringView skipFilePrefix(QStringView str)
{
    return str.sliced(filePrefixLen(str));
}

/**
 * @return stack property of js Error.
 */
static inline QJSValue getStack(const QJSValue &exception)
{
    return exception.property(u"stack"_s);
}

/**
 * @return stack of \p engine.
 */
static inline QJSValue generateStack(QJSEngine *engine)
{
    engine->throwError(QString());
    return getStack(engine->catchError());
}

struct StackLine {
    QStringView funcName;
    QStringView filePrefixOrMessage;
    QStringView fileName;
    QStringView lineNumber;
    QStringView remaining;
};

/**
 * Parse a line contained in the stack property of a javascript error.
 * @return the first line. The rest is in \c StackLine::remaining
 */
static StackLine parseStackLine(QStringView stack)
{
    // format: funcName? '@file:' '//'? fileName ':' lineNumber '\n'

    StackLine ret;

    // func
    qsizetype pos = stack.indexOf('@'_L1);
    if (pos >= 0) {
        ret.funcName = stack.first(pos);
        stack = stack.sliced(pos + 1);
    }

    // remove file prefix
    pos = filePrefixLen(stack);
    ret.filePrefixOrMessage = stack.first(pos);

    auto endLine = stack.indexOf('\n'_L1, pos);
    auto line = stack.sliced(pos, ((endLine < 0) ? stack.size() : endLine) - pos);
    // fileName and lineNumber
    auto i = line.lastIndexOf(':'_L1);
    if (i > 0) {
        ret.fileName = line.sliced(0, i);
        ret.lineNumber = line.sliced(i + 1);
    } else {
        ret.filePrefixOrMessage = line;
    }

    if (endLine >= 0) {
        ret.remaining = stack.sliced(endLine + 1);
    }

    return ret;
}

/**
 * Add a formatted error stack to \p buffer.
 * @param buffer[out]
 * @param colors
 * @param stack stack of js Error.
 * @param prefix prefix added to each start of line
 */
static void pushException(QString &buffer, ScriptTester::Colors &colors, QStringView stack, QStringView prefix)
{
    // skips the first line that refers to the internal call
    if (!stack.isEmpty() && stack[0] == u'%') {
        auto pos = stack.indexOf('\n'_L1);
        if (pos < 0) {
            buffer += colors.error % prefix % stack % colors.reset % u'\n';
            return;
        }
        stack = stack.sliced(pos + 1);
    }

    // color lines
    while (!stack.isEmpty()) {
        auto stackLine = parseStackLine(stack);
        // clang-format off
        buffer += colors.error % prefix % colors.reset
                % colors.program % stackLine.funcName % colors.reset
                % colors.error % u'@' % stackLine.filePrefixOrMessage % colors.reset
                % colors.fileName % stackLine.fileName % colors.reset
                % colors.error % u':' % colors.reset
                % colors.lineNumber % stackLine.lineNumber % colors.reset
                % u'\n';
        // clang-format on
        stack = stackLine.remaining;
    }
}

static inline bool cursorSameAsSecondary(const ScriptTester::Placeholders &placeholders)
{
    return placeholders.cursor == placeholders.secondaryCursor;
}

static inline bool selectionStartSameAsSecondary(const ScriptTester::Placeholders &placeholders)
{
    return placeholders.selectionStart == placeholders.selectionStart;
}

static inline bool selectionEndSameAsSecondary(const ScriptTester::Placeholders &placeholders)
{
    return placeholders.selectionEnd == placeholders.selectionEnd;
}

static inline bool selectionSameAsSecondary(const ScriptTester::Placeholders &placeholders)
{
    return selectionStartSameAsSecondary(placeholders) || selectionEndSameAsSecondary(placeholders);
}

static ScriptTester::EditorConfig makeEditorConfig()
{
    return {
        .syntax = u"None"_s,
        .indentationMode = u"none"_s,
        .indentationWidth = 4,
        .tabWidth = 4,
        .replaceTabs = false,
        .overrideMode = false,
        .indentPastedTest = false,
    };
}

} // anonymous namespace

/**
 * Represents a textual (new line, etc.) or non-textual (cursor, etc.) element
 * in the text \c DocumentText::text.
 */
struct ScriptTester::TextItem {
    /*
     * *BlockSelection* are the borders of a block selection and are inserted
     * before display.
     *
     * In scenario 1 and 2, the position of BlockSelection* and Selection* is
     * reversed.
     *
     *  Scenario 1: start.column < end.column
     *      input:   ...[ssssss...\n...ssssss...\n...ssssss]...
     *      display: ...[ssssss]...\n...[ssssss]...\n...[ssssss]...
     *
     *      ...[ssssss]...
     *         ~            SelectionStart
     *                ~     BlockSelectionStart
     *      ...[ssssss]...
     *         ~            VirtualBlockSelectionStart
     *                ~     VirtualBlockSelectionEnd
     *      ...[ssssss]...
     *         ~            BlockSelectionEnd
     *                ~     SelectionEnd
     *
     *  Scenario 2: start.column > end.column
     *      input:   ...ssssss[...\n...ssssss...\n...]ssssss...
     *      display: ...[ssssss]...\n...[ssssss]...\n...[ssssss]...
     *
     *      ...[ssssss]...
     *         ~            BlockSelectionStart
     *                ~     SelectionStart
     *      ...[ssssss]...
     *         ~            VirtualBlockSelectionStart
     *                ~     VirtualBlockSelectionEnd
     *      ...[ssssss]...
     *         ~            SelectionEnd
     *                ~     BlockSelectionEnd
     *
     *  Scenario 3: start.column == end.column
     *      input:   ...[...\n......\n...]...
     *      display: ...[...\n...|...\n...]...
     *
     *      ...[...
     *         ~            SelectionStart
     *      ...|...
     *         ~            VirtualBlockCursor
     *      ...]...
     *         ~            SelectionEnd
     */
    enum Kind {
        // ordered by priority for display (cursor before selection start and after selection end)
        SelectionEnd,
        SecondarySelectionEnd,
        VirtualBlockSelectionEnd,
        BlockSelectionEnd,
        Cursor,
        VirtualBlockCursor,
        SecondaryCursor,
        SelectionStart,
        SecondarySelectionStart,
        VirtualBlockSelectionStart,
        BlockSelectionStart,

        // NewLine is the last item in a line. All other items, including those
        // with an identical position and a virtual text, must be placed in front.
        NewLine,

        // only used for output formatting
        //@{
        Tab,
        Backslash,
        DoubleQuote,
        //@}

        MaxElement,
        StartCharacterElement = NewLine,
    };

    qsizetype pos;
    Kind kind;
    int virtualTextLen = 0; // number of virtual characters left

    bool isCharacter() const
    {
        return kind >= StartCharacterElement;
    }

    bool isSelectionStart() const
    {
        return kind == SelectionStart || kind == SecondarySelectionStart;
    }

    bool isSelectionEnd() const
    {
        return kind == SelectionEnd || kind == SecondarySelectionEnd;
    }

    bool isSelection(bool hasVirtualBlockSelection) const
    {
        switch (kind) {
        case SelectionEnd:
        case SecondarySelectionEnd:
        case SelectionStart:
        case SecondarySelectionStart:
            return true;
        case VirtualBlockSelectionEnd:
        case BlockSelectionEnd:
        case VirtualBlockSelectionStart:
        case BlockSelectionStart:
            return hasVirtualBlockSelection;
        default:
            return false;
        }
    }
};

ScriptTester::DocumentText::DocumentText() = default;
ScriptTester::DocumentText::~DocumentText() = default;

/**
 * Extract item from \p str and add them to \c items.
 * @param str
 * @param kind
 * @param c character in \p str representing a \p kind.
 * @return number of items extracted
 */
std::size_t ScriptTester::DocumentText::addItems(QStringView str, int kind, QChar c)
{
    const auto n = items.size();

    qsizetype pos = 0;
    while (-1 != (pos = str.indexOf(c, pos))) {
        items.push_back({pos, TextItem::Kind(kind)});
        ++pos;
    }

    return items.size() - n;
}

/**
 * Extract selection item from \p str and add them to \c items.
 * Addition is done in pairs by searching for \p start then \p end.
 * The next \p start starts after the previous \p end.
 * If \p end is not found, the element is not added.
 * @param str
 * @param kind
 * @param start character in \p str representing a \p kind.
 * @param end character in \p str representing a \p kind.
 * @return number of pairs extracted
 */
std::size_t ScriptTester::DocumentText::addSelectionItems(QStringView str, int kind, QChar start, QChar end)
{
    const auto n = items.size();

    qsizetype pos = 0;
    while (-1 != (pos = str.indexOf(start, pos))) {
        qsizetype pos2 = str.indexOf(end, pos + 1);
        if (pos2 == -1) {
            break;
        }

        items.push_back({pos, TextItem::Kind(kind)});
        constexpr int offset = -7;
        static_assert(TextItem::SelectionStart + offset == TextItem::SelectionEnd);
        static_assert(TextItem::SecondarySelectionStart + offset == TextItem::SecondarySelectionEnd);
        items.push_back({pos2, TextItem::Kind(kind + offset)});

        pos = pos2 + 1;
    }

    return (items.size() - n) / 2;
}

/**
 * Add virtual cursors and selections by deducing them from the primary selection.
 */
void ScriptTester::DocumentText::computeBlockSelectionItems()
{
    /*
     * Check if any virtual cursors or selections need to be added.
     *
     * Example of possible cases (virtual item represented by @):
     *
     * (no item)    (2 items)    (4 items)    (no item)    (1 item)
     * ..[...]..    ..[...@..    ..[...@..      ..[..       ..[..
     * .......      ..@...]..    ..@...@..      ..]..       ..@..
     * .......      .......      ..@...]..      ....        ..]..
     */
    if (selection.start().line() == -1 || selection.numberOfLines() <= (selection.columnWidth() ? 0 : 1)) {
        return;
    }

    const auto nbLine = selection.numberOfLines();
    const auto startCursor = selection.start();
    const auto endCursor = selection.end();

    const auto nbItem = items.size();

    /*
     * Pre-constructed the number of items that will be added
     */
    if (startCursor.column() != endCursor.column()) {
        items.resize(nbItem + nbLine * 2 + 1);
        /*
         * Added NewLine to simplify inserting the last BlockSelectionEnd.
         * It will be removed at the end.
         */
        items[nbItem] = {text.size(), TextItem::NewLine, 0};
    } else {
        items.resize(nbItem + nbLine - 1);
    }

    using Iterator = std::vector<TextItem>::iterator;

    Iterator itemIt = items.begin();
    Iterator itemEnd = itemIt + nbItem;
    // skip the inserted NewLine
    Iterator outIt = itemEnd + (startCursor.column() != endCursor.column());

    auto advanceUntilNewLine = [](Iterator &itemIt) {
        while (itemIt->kind != TextItem::NewLine) {
            ++itemIt;
        }
    };

    int line = 0;
    qsizetype textPos = 0;

    /*
     * Move to start of the selection line
     */
    if (startCursor.line() > 0) {
        for (;; ++itemIt) {
            advanceUntilNewLine(itemIt);
            if (++line == startCursor.line()) {
                textPos = itemIt->pos + 1;
                ++itemIt;
                break;
            }
        }
    }

    /**
     * Advance \p itemIt to \p column, a virtual text or a new line then add the \p kind item.
     * @return virtual text length of the added item
     */
    auto advanceAndPushItem = [&outIt, &textPos](Iterator &itemIt, int column, TextItem::Kind kind) {
        while (!itemIt->virtualTextLen && itemIt->pos - textPos < column && itemIt->kind != TextItem::NewLine) {
            ++itemIt;
        }

        int vlen = 0;

        if (itemIt->pos - textPos >= column) {
            *outIt = {textPos + column, kind};
        } else /*if (first->virtualTextLen || first->kind == TextItem::NewLine)*/ {
            vlen = column - (itemIt->pos - textPos);
            *outIt = {itemIt->pos, kind, vlen};
        }
        ++outIt;

        return vlen;
    };

    /*
     * Insert BlockSelectionStart then go to the next line
     */
    int vlen = 0;
    if (startCursor.column() != endCursor.column()) {
        vlen = advanceAndPushItem(itemIt, endCursor.column(), TextItem::BlockSelectionStart);
    }
    advanceUntilNewLine(itemIt);
    itemIt->virtualTextLen = qMax(itemIt->virtualTextLen, vlen);
    textPos = itemIt->pos + 1;
    ++itemIt;

    int leftColumn = startCursor.column();
    int rightColumn = endCursor.column();
    if (startCursor.column() > endCursor.column()) {
        std::swap(leftColumn, rightColumn);
    }

    /*
     * Insert VirtualBlockSelection* or VirtualBlockCursor
     */
    while (++line < endCursor.line()) {
        if (leftColumn != rightColumn) {
            advanceAndPushItem(itemIt, leftColumn, TextItem::VirtualBlockSelectionStart);
        }

        int vlen = advanceAndPushItem(itemIt, rightColumn, (leftColumn != rightColumn) ? TextItem::VirtualBlockSelectionEnd : TextItem::VirtualBlockCursor);
        advanceUntilNewLine(itemIt);
        itemIt->virtualTextLen = qMax(itemIt->virtualTextLen, vlen);
        textPos = itemIt->pos + 1;
        ++itemIt;
    }

    /*
     * Insert BlockSelectionEnd
     */
    if (startCursor.column() != endCursor.column()) {
        int vlen = advanceAndPushItem(itemIt, startCursor.column(), TextItem::BlockSelectionEnd);
        if (vlen) {
            advanceUntilNewLine(itemIt);
            itemIt->virtualTextLen = qMax(itemIt->virtualTextLen, vlen);
        }

        /*
         * Remove the new line added
         */
        items[nbItem] = items.back();
        items.pop_back();
    }
}

/**
 * Insert items used only for display with ScriptTester::writeDataTest().
 */
void ScriptTester::DocumentText::insertFormattingItems(DocumentTextFormat format)
{
    if (hasFormattingItems) {
        return;
    }
    hasFormattingItems = true;

    const auto nbItem = items.size();

    computeBlockSelectionItems();

    /*
     * Insert text replacement items
     */
    switch (format) {
    case DocumentTextFormat::Raw:
        break;
    case DocumentTextFormat::EscapeForDoubleQuote:
        addItems(text, TextItem::Backslash, u'\\');
        addItems(text, TextItem::DoubleQuote, u'"');
        [[fallthrough]];
    case DocumentTextFormat::ReplaceNewLineAndTabWithLiteral:
    case DocumentTextFormat::ReplaceNewLineAndTabWithPlaceholder:
    case DocumentTextFormat::ReplaceTabWithPlaceholder:
        addItems(text, TextItem::Tab, u'\t');
        break;
    }

    if (nbItem != items.size()) {
        sortItems();
    }
}

/**
 * Sort items by \c TextItem::pos, then \c TextItem::virtualTextLine, then \c TextItem::kind.
 */
void ScriptTester::DocumentText::sortItems()
{
    auto cmp = [](const TextItem &a, const TextItem &b) {
        if (a.pos < b.pos) {
            return true;
        }
        if (a.pos > b.pos) {
            return false;
        }
        if (a.virtualTextLen < b.virtualTextLen) {
            return true;
        }
        if (a.virtualTextLen > b.virtualTextLen) {
            return false;
        }
        return a.kind < b.kind;
    };
    std::sort(items.begin(), items.end(), cmp);
}

/**
 * Initialize DocumentText with text containing placeholders.
 * @param input text with placeholders
 * @param placeholders
 * @return Error or empty string
 */
QString ScriptTester::DocumentText::setText(QStringView input, const Placeholders &placeholders)
{
    items.clear();
    text.clear();
    secondaryCursors.clear();
    secondaryCursorsWithSelection.clear();
    hasFormattingItems = false;
    totalCursor = 0;
    totalSelection = 0;

    totalLine = 1 + addItems(input, TextItem::NewLine, u'\n');

#define RETURN_IF_VIRTUAL_TEXT_CONFLICT(hasItem, placeholderName)                                                                                              \
    if (hasItem && placeholders.hasVirtualText() && placeholders.virtualText == placeholders.placeholderName) {                                                \
        return u"virtualText placeholder conflicts with " #placeholderName ""_s;                                                                               \
    }

    /*
     * Parse cursor and secondary cursors
     */

    // add secondary cursors
    if (placeholders.hasSecondaryCursor()) {
        totalCursor = addItems(input, TextItem::SecondaryCursor, placeholders.secondaryCursor);
        RETURN_IF_VIRTUAL_TEXT_CONFLICT(totalCursor, secondaryCursor);

        // when cursor and secondaryCursor have the same placeholder,
        // the first one found corresponds to the primary cursor
        if (totalCursor && (!placeholders.hasCursor() || cursorSameAsSecondary(placeholders))) {
            items[items.size() - totalCursor].kind = TextItem::Cursor;
        }
    }

    // add primary cursor when the placeholder is different from the secondary cursor
    if (placeholders.hasCursor() && (!placeholders.hasSecondaryCursor() || !cursorSameAsSecondary(placeholders))) {
        const auto nbCursor = addItems(input, TextItem::Cursor, placeholders.cursor);
        if (nbCursor > 1) {
            return u"primary cursor set multiple times"_s;
        }
        RETURN_IF_VIRTUAL_TEXT_CONFLICT(nbCursor, cursor);
        totalCursor += nbCursor;
    }

    /*
     * Parse selection and secondary selections
     */

    // add secondary selections
    if (placeholders.hasSecondarySelection()) {
        totalSelection = addSelectionItems(input, TextItem::SecondarySelectionStart, placeholders.secondarySelectionStart, placeholders.secondarySelectionEnd);
        RETURN_IF_VIRTUAL_TEXT_CONFLICT(totalSelection, secondarySelectionStart);
        RETURN_IF_VIRTUAL_TEXT_CONFLICT(totalSelection, secondarySelectionEnd);

        // when selection and secondarySelection have the same placeholders,
        // the first one found corresponds to the primary selection
        if (totalSelection && (!placeholders.hasSelection() || selectionSameAsSecondary(placeholders))) {
            if (placeholders.hasSelection() && (!selectionStartSameAsSecondary(placeholders) || !selectionEndSameAsSecondary(placeholders))) {
                return u"primary selection placeholder conflicts with secondary selection placeholder"_s;
            }
            items[items.size() - totalSelection * 2 + 0].kind = TextItem::SelectionStart;
            items[items.size() - totalSelection * 2 + 1].kind = TextItem::SelectionEnd;
        }
    }

    // add primary selection when the placeholders are different from the secondary selection
    if (placeholders.hasSelection() && (!placeholders.hasSecondarySelection() || !selectionSameAsSecondary(placeholders))) {
        const auto nbSelection = addSelectionItems(input, TextItem::SelectionStart, placeholders.selectionStart, placeholders.selectionEnd);
        if (nbSelection > 1) {
            return u"primary selection set multiple times"_s;
        }
        RETURN_IF_VIRTUAL_TEXT_CONFLICT(nbSelection, selectionStart);
        RETURN_IF_VIRTUAL_TEXT_CONFLICT(nbSelection, selectionEnd);
        totalSelection += nbSelection;
    }

#undef RETURN_IF_VIRTUAL_TEXT_CONFLICT

    /*
     * Search for the first virtual text
     */

    VirtualText virtualText{-1, -1};

    if (placeholders.hasVirtualText()) {
        virtualText = findVirtualText(input, 0, placeholders.virtualText);
    }

    if (virtualText.pos2 != -1 && (totalCursor > 1 || totalSelection > 1)) {
        return u"virtualText is incompatible with multi-cursor/selection"_s;
    }

    /*
     * Update text member, cursor member, selection member,
     * TextItem::pos and TextItem::virtualTextLen
     */

    sortItems();

    int line = 0;
    int charConsumedInPreviousLines = 0;
    cursor = Cursor::invalid();
    auto selectionStart = Cursor::invalid();
    auto selectionEnd = Cursor::invalid();
    auto secondarySelectionStart = Cursor::invalid();
    const TextItem *selectionEndItem = nullptr;
    qsizetype ignoredChars = 0;
    qsizetype virtualTextLen = 0;
    qsizetype lastPos = -1;

    /*
     * Update TextItem::pos and TextItem::virtualTextLen
     *  "abc@@@[@]|\n..."
     *      ~~~ ~ VirtualText
     *         ~ SelectionStart -> update pos=3 and virtualTextLen=3
     *           ~ SelectionEnd -> update pos=3 and virtualTextLen=4
     *            ~ Cursor -> update pos=3 and virtualTextLen=4
     *             ~~ NewLine -> update pos=3 and virtualTextLen=4
     */
    for (auto &item : items) {
        // when the same character is used with several placeholders, the
        // position does not change and the previous character must not
        // be ignored because it has not yet been consumed in the input
        if (lastPos == item.pos) {
            --ignoredChars;
        }
        lastPos = item.pos;

        /*
         * Update virtual text information
         */

        // item after virtual text
        if (virtualText.pos2 != -1 && virtualText.pos2 <= item.pos) {
            // invalid virtualText input
            if (item.kind == TextItem::NewLine || virtualText.pos2 != item.pos) {
                break;
            }
            const auto pos = text.size() + ignoredChars;
            text.append(input.sliced(pos, item.pos - pos - virtualText.size()));

            ignoredChars += virtualText.size();
            virtualTextLen += virtualText.size();
            virtualText = findVirtualText(input, virtualText.pos2, placeholders.virtualText);
        } else if (virtualTextLen) {
            // text after virtualText but before NewLine
            if (item.pos != text.size() + ignoredChars) {
                break;
            }
        }

        /*
         * Update TextItem, cursor and selection
         */

        item.pos -= ignoredChars;
        item.virtualTextLen = virtualTextLen;

        auto cursorFromCurrentItem = [&] {
            return Cursor(line, item.pos - charConsumedInPreviousLines + item.virtualTextLen);
        };

        switch (item.kind) {
        case TextItem::Cursor:
            cursor = cursorFromCurrentItem();
            break;
        case TextItem::SelectionStart:
            selectionStart = cursorFromCurrentItem();
            break;
        case TextItem::SelectionEnd:
            selectionEndItem = &item;
            selectionEnd = cursorFromCurrentItem();
            break;
        case TextItem::SecondaryCursor:
            secondaryCursors.push_back({cursorFromCurrentItem(), Range::invalid()});
            break;
        case TextItem::SecondarySelectionStart:
            secondarySelectionStart = cursorFromCurrentItem();
            break;
        case TextItem::SecondarySelectionEnd:
            secondaryCursorsWithSelection.push_back({Cursor::invalid(), {secondarySelectionStart, cursorFromCurrentItem()}});
            break;
        // case TextItem::NewLine:
        default:
            charConsumedInPreviousLines = item.pos + 1;
            virtualTextLen = 0;
            ++line;
            continue;
        }

        const auto pos = text.size() + ignoredChars;
        const auto len = item.pos + ignoredChars - pos;
        text.append(input.sliced(pos, len));
        ++ignoredChars;
    }

    // check for invalid virtual text
    if ((virtualText.pos2 != -1 && virtualText.pos2 != text.size()) || (virtualTextLen && text.size() + ignoredChars != input.size())) {
        const auto pos = (virtualText.pos1 != -1) ? virtualText.pos1 : text.size() + ignoredChars - virtualTextLen;
        return u"virtual text found at position %1, but not followed by a cursor or selection then a line break or end of text"_s.arg(pos);
    }
    // check for missing primary selection with secondary selection
    if (!secondaryCursorsWithSelection.isEmpty() && selectionStart.line() == -1) {
        return u"secondary selections are added without any primary selection"_s;
    }
    // check for missing primary cursor with secondary cursor
    if (!secondaryCursors.empty() && cursor.line() == -1) {
        return u"secondary cursors are added without any primary cursor"_s;
    }

    text += input.sliced(text.size() + ignoredChars);

    /*
     * The previous loop changes TextItem::pos and the elements must be
     * reordered so that the cursor is after an end selection.
     * input: `a[b|]c` -> [{1, SelectionStart}, {3, Cursor}, {4, SelectionStop}]
     * update indexes:    [{1, SelectionStart}, {2, Cursor}, {2, SelectionStop}]
     * expected:          [{1, SelectionStart}, {2, SelectionStop}, {2, Cursor}]
     *                    -> `a[b]|c`
     */
    sortItems();

    /*
     * Check for empty or overlapping selections and for overlapping cursors
     */
    int countSelection = 0;
    qsizetype lastCursorPos = -1;
    qsizetype lastSelectionPos = -1;
    for (auto &item : items) {
        if (item.isSelectionStart()) {
            ++countSelection;
            if ((countSelection & 1) && lastSelectionPos != item.pos) {
                lastSelectionPos = item.pos;
                continue;
            }
        } else if (item.isSelectionEnd()) {
            ++countSelection;
            if (!(countSelection & 1) && lastSelectionPos != item.pos) {
                lastSelectionPos = item.pos;
                continue;
            }
        } else if (item.kind == TextItem::Cursor) {
            if (countSelection & 1) {
                return u"cursor inside a selection"_s;
            }
            if (lastCursorPos == item.pos) {
                return u"one or more cursors overlap"_s;
            }
            lastCursorPos = item.pos;
            continue;
        } else {
            continue;
        }
        return u"selection %1 is empty or overlaps"_s.arg(countSelection / 2 + 1);
    }

    /*
     * Merge secondaryCursors in secondaryCursorsWithSelection
     * and init cursor for secondaryCursorsWithSelection
     *
     * secondaryCursors              = [Cursor{1,3}, Cursor{2,3}, Cursor{3,3}]
     * secondaryCursorsWithSelection = [Range{{1,3}, {1,5}}, Range{{3,0}, {3,3}}, Range{{5,0}, {6,0}}]
     *                              => [(Cursor{1,3}, Range{{1,3}, {1,5}})  // merged
     *                                  (Cursor{2,3}, Range::invalid())     // inserted
     *                                  (Cursor{3,3}, Range{{3,0}, {3,3}})  // merged
     *                                  (Cursor{6,0}, Range{{5,0}, {6,0}})] // update
     */
    if (!secondaryCursors.empty() && !secondaryCursorsWithSelection.isEmpty()) {
        auto it = secondaryCursors.begin();
        auto end = secondaryCursors.end();
        auto it2 = secondaryCursorsWithSelection.begin();
        auto end2 = secondaryCursorsWithSelection.end();

        // merge
        while (it != end && it2 != end2) {
            if (it2->range.end() < it->pos) {
                it2->pos = it2->range.end();
                ++it2;
            } else if (it2->range.start() == it->pos || it2->range.end() == it->pos) {
                it2->pos = it->pos;
                ++it2;
                it->pos.setLine(-1);
                ++it;
            } else {
                ++it;
            }
        }

        // update invalid cursor (set to end())
        for (; it2 != end2; ++it2) {
            it2->pos = it2->range.end();
        }

        // insert cursor without selection
        const auto n = secondaryCursorsWithSelection.size();
        for (auto &c : secondaryCursors) {
            if (c.pos.line() != -1) {
                secondaryCursorsWithSelection.append(c);
            }
        }
        if (n != secondaryCursorsWithSelection.size()) {
            std::sort(secondaryCursorsWithSelection.begin(), secondaryCursorsWithSelection.end());
        }
    } else if (!secondaryCursorsWithSelection.isEmpty()) {
        for (auto &c : secondaryCursorsWithSelection) {
            c.pos = c.range.end();
        }
    } else {
        secondaryCursorsWithSelection.assign(secondaryCursors.begin(), secondaryCursors.end());
    }

    /*
     * Init cursor when no specified
     */
    if (cursor.line() == -1) {
        if (selectionEndItem) {
            // add cursor to end of selection
            auto afterSelection = items.begin() + (selectionEndItem - items.data()) + 1;
            items.insert(afterSelection, {selectionEndItem->pos, TextItem::Cursor, selectionEndItem->virtualTextLen});
            cursor = selectionEnd;
        } else {
            // add cursor to end of document
            const auto virtualTextLen = items.empty() ? 0 : items.back().virtualTextLen;
            items.push_back({input.size(), TextItem::Cursor, virtualTextLen});
            cursor = Cursor(line, input.size() - charConsumedInPreviousLines);
        }
    }

    selection = {selectionStart, selectionEnd};

    // check that the cursor is on a selection if one exists
    if (selection.start().line() != -1 && !selection.boundaryAtCursor(cursor)) {
        return u"the cursor is not at the limit of the selection"_s;
    }

    return QString();
}

ScriptTester::ScriptTester(QIODevice *output,
                           const Format &format,
                           const JSPaths &paths,
                           const TestExecutionConfig &executionConfig,
                           Placeholders placeholders,
                           QJSEngine *engine,
                           DocumentPrivate *doc,
                           ViewPrivate *view,
                           QObject *parent)
    : QObject(parent)
    , m_engine(engine)
    , m_doc(doc)
    , m_view(view)
    , m_fallbackPlaceholders(format.fallbackPlaceholders)
    , m_defaultPlaceholders(placeholders)
    , m_placeholders(placeholders)
    , m_editorConfig(makeEditorConfig())
    , m_stream(output)
    , m_format(format)
    , m_paths(paths)
    , m_executionConfig(executionConfig)
{
    // starts a config without ever finishing it: no need to update anything
    doc->config()->configStart();
    view->config()->configStart();

    view->config()->setValue(KateViewConfig::AutoBrackets, false);
    // doc->config()->setIndentPastedText(false);
}

QString ScriptTester::read(const QString &name)
{
    // the error will also be written to this variable,
    // but ignored because QJSEngine will then throw an exception
    QString contentOrError;

    QString fullName = getPath(m_engine, name, m_paths.scripts, &contentOrError);
    if (!fullName.isEmpty()) {
        readFile(m_engine, fullName, &contentOrError, &contentOrError);
    }
    return contentOrError;
}

void ScriptTester::require(const QString &name)
{
    // check include guard
    auto it = m_libraryFiles.find(name);
    if (it != m_libraryFiles.end()) {
        // re-throw previous exception
        if (!it->isEmpty()) {
            m_engine->throwError(QJSValue::URIError, *it);
        }
        return;
    }

    it = m_libraryFiles.insert(name, QString());

    QString fullName = getPath(m_engine, name, m_paths.libraries, &*it);
    if (fullName.isEmpty()) {
        return;
    }

    QString program;
    if (!readFile(m_engine, fullName, &program, &*it)) {
        return;
    }

    // eval in current script engine
    const QJSValue val = m_engine->evaluate(program, fullName);
    if (!val.isError()) {
        return;
    }

    // propagate exception
    *it = val.toString();
    m_engine->throwError(val);
}

void ScriptTester::debug(const QString &message)
{
    const auto err = m_format.debugOptions ? generateStack(m_engine) : QJSValue();
    const auto stack = m_format.debugOptions.testAnyFlags(DebugOption::WriteStackTrace | DebugOption::WriteFunction) ? err.toString() : QString();

    /*
     * Display format:
     *
     * {fileName}:{lineNumber}: {funcName}: DEBUG: {msg}
     * ~~~~~~~~~~~~~~~~~~~~~~~~~                            WriteLocation option
     *                          ~~~~~~~~~~~~                WriteFunction option
     * {stackTrace}                                         WriteStackTrace option
     */

    // add {fileName}:{lineNumber}:
    if (m_format.debugOptions.testAnyFlag(DebugOption::WriteLocation)) {
        const auto fileName = err.property(u"fileName"_s).toString();
        const auto lineNumber = err.property(u"lineNumber"_s).toString();
        m_debugMsg += m_format.colors.fileName % skipFilePrefix(fileName) % m_format.colors.reset % u':' % m_format.colors.lineNumber % lineNumber
            % m_format.colors.reset % m_format.colors.debugMsg % u": "_sv % m_format.colors.reset;
    }

    // add {funcName}:
    if (m_format.debugOptions.testAnyFlag(DebugOption::WriteFunction)) {
        const QStringView stackView = stack;
        const qsizetype pos = stackView.indexOf('@'_L1);
        if (pos > 0) {
            m_debugMsg += m_format.colors.program % stackView.first(pos) % m_format.colors.reset % m_format.colors.debugMsg % ": "_L1 % m_format.colors.reset;
        }
    }

    // add DEBUG: {msg}
    m_debugMsg +=
        m_format.colors.debugMarker % u"DEBUG:"_sv % m_format.colors.reset % m_format.colors.debugMsg % u' ' % message % m_format.colors.reset % u'\n';

    // add {stackTrace}
    if (m_format.debugOptions.testAnyFlag(DebugOption::WriteStackTrace)) {
        pushException(m_debugMsg, m_format.colors, stack, u"| "_sv);
    }

    // flush
    if (m_format.debugOptions.testAnyFlag(DebugOption::ForceFlush)) {
        if (!m_hasDebugMessage && m_format.testFormatOptions.testAnyFlag(TestFormatOption::AlwaysWriteLocation)) {
            m_stream << '\n';
        }
        m_stream << m_debugMsg;
        m_stream.flush();
        m_debugMsg.clear();
    }

    m_hasDebugMessage = true;
}

void ScriptTester::print(const QString &message)
{
    if (m_format.debugOptions.testAnyFlags(DebugOption::WriteLocation | DebugOption::WriteFunction)) {
        /*
         * Display format:
         *
         * {fileName}:{lineNumber}: {funcName}: PRINT: {msg}
         * ~~~~~~~~~~~~~~~~~~~~~~~~~                            WriteLocation option
         *                          ~~~~~~~~~~~~                WriteFunction option
         */

        const auto errStr = generateStack(m_engine).toString();
        QStringView err = errStr;
        auto nl = err.indexOf(u'\n');
        if (nl != -1) {
            auto stackLine = parseStackLine(err.sliced(nl + 1));

            // add {fileName}:{lineNumber}:
            if (m_format.debugOptions.testAnyFlag(DebugOption::WriteLocation)) {
                m_stream << m_format.colors.fileName << skipFilePrefix(stackLine.fileName) << m_format.colors.reset << ':' << m_format.colors.lineNumber
                         << stackLine.lineNumber << m_format.colors.reset << m_format.colors.debugMsg << ": "_L1 << m_format.colors.reset;
            }

            // add {funcName}:
            if (m_format.debugOptions.testAnyFlag(DebugOption::WriteFunction) && !stackLine.funcName.isEmpty()) {
                m_stream << m_format.colors.program << stackLine.funcName << m_format.colors.reset << m_format.colors.debugMsg << ": "_L1
                         << m_format.colors.reset;
            }
        }
    }

    // add PRINT: {msg}
    m_stream << m_format.colors.debugMarker << "PRINT:"_L1 << m_format.colors.reset << m_format.colors.debugMsg << ' ' << message << m_format.colors.reset
             << '\n';
    m_stream.flush();
}

QJSValue ScriptTester::loadModule(const QString &fileName)
{
    QString error;
    const auto path = getModulePath(m_engine, fileName, m_paths.modules, &error);
    if (path.isEmpty()) {
        return QJSValue();
    }

    auto mod = m_engine->importModule(path);
    if (mod.isError()) {
        m_engine->throwError(mod);
    }
    return mod;
}

void ScriptTester::loadScript(const QString &fileName)
{
    QString contentOrError;
    const auto path = getModulePath(m_engine, fileName, m_paths.scripts, &contentOrError);
    if (path.isEmpty()) {
        return;
    }

    if (!readFile(m_engine, path, &contentOrError, &contentOrError)) {
        return;
    }

    // eval in current script engine
    const QJSValue val = m_engine->evaluate(contentOrError, fileName);
    if (!val.isError()) {
        return;
    }

    // propagate exception
    m_engine->throwError(val);
}

bool ScriptTester::startTestCase(const QString &name, int nthStack)
{
    if (m_executionConfig.patternType == PatternType::Inactive) {
        return true;
    }

    const bool hasMatch = m_executionConfig.pattern.matchView(name).hasMatch();
    const bool exclude = m_executionConfig.patternType == PatternType::Exclude;
    if (exclude == hasMatch) {
        return false;
    }

    ++m_skipedCounter;

    /*
     * format with optional testName
     * ${fileName}:${lineNumber}: ${testName}: SKIP
     */

    // ${fileName}:${lineNumber}:
    writeLocation(nthStack);
    // ${testName}: SKIP
    m_stream << m_format.colors.testName << name << m_format.colors.reset << ": "_L1 << m_format.colors.labelInfo << "SKIP"_L1 << m_format.colors.reset << '\n';

    if (m_format.debugOptions.testAnyFlag(DebugOption::ForceFlush)) {
        m_stream.flush();
    }

    return false;
}

void ScriptTester::setConfig(const QJSValue &config)
{
#define READ_CONFIG(fn, name)                                                                                                                                  \
    fn(config, u"" #name ""_s, [&](auto value) {                                                                                                               \
        m_editorConfig.name = std::move(value);                                                                                                                \
    })
    READ_CONFIG(readString, syntax);
    READ_CONFIG(readString, indentationMode);
    READ_CONFIG(readInt, indentationWidth);
    READ_CONFIG(readInt, tabWidth);
    READ_CONFIG(readBool, replaceTabs);
    READ_CONFIG(readBool, overrideMode);
    READ_CONFIG(readBool, indentPastedTest);
#undef READ_CONFIG

#define READ_PLACEHOLDER(name)                                                                                                                                 \
    readString(config, u"" #name ""_s, [&](QString s) {                                                                                                        \
        m_placeholders.name = s.isEmpty() ? u'\0' : s[0];                                                                                                      \
    });                                                                                                                                                        \
    readString(config, u"" #name "2"_s, [&](QString s) {                                                                                                       \
        m_fallbackPlaceholders.name = s.isEmpty() ? m_format.fallbackPlaceholders.name : s[0];                                                                 \
    })
    READ_PLACEHOLDER(cursor);
    READ_PLACEHOLDER(secondaryCursor);
    READ_PLACEHOLDER(virtualText);
#undef READ_PLACEHOLDER

    auto readSelection = [&](QString name, QString fallbackName, QChar Placeholders::*startMem, QChar Placeholders::*endMem) {
        readString(config, name, [&](QString s) {
            switch (s.size()) {
            case 0:
                m_placeholders.*startMem = m_placeholders.*endMem = u'0';
                break;
            case 1:
                m_placeholders.*startMem = m_placeholders.*endMem = s[0];
                break;
            default:
                m_placeholders.*startMem = s[0];
                m_placeholders.*endMem = s[1];
                break;
            }
        });
        readString(config, fallbackName, [&](QString s) {
            switch (s.size()) {
            case 0:
                m_fallbackPlaceholders.*startMem = m_format.fallbackPlaceholders.*startMem;
                m_fallbackPlaceholders.*endMem = m_format.fallbackPlaceholders.*endMem;
                break;
            case 1:
                m_fallbackPlaceholders.*startMem = m_fallbackPlaceholders.*endMem = s[0];
                break;
            default:
                m_fallbackPlaceholders.*startMem = s[0];
                m_fallbackPlaceholders.*endMem = s[1];
                break;
            }
        });
    };
    readSelection(u"selection"_s, u"selection2"_s, &Placeholders::selectionStart, &Placeholders::selectionEnd);
    readSelection(u"secondarySelection"_s, u"secondarySelection2"_s, &Placeholders::secondarySelectionStart, &Placeholders::secondarySelectionEnd);
}

void ScriptTester::resetConfig()
{
    m_fallbackPlaceholders = m_format.fallbackPlaceholders;
    m_placeholders = m_defaultPlaceholders;
    m_editorConfig = makeEditorConfig();
}

void ScriptTester::saveConfig()
{
    m_savedFallbackPlaceholders = m_fallbackPlaceholders;
    m_savedPlaceholders = m_placeholders;
    m_savedEditorConfig = m_editorConfig;
}

void ScriptTester::restoreConfig()
{
    m_fallbackPlaceholders = m_savedFallbackPlaceholders;
    m_placeholders = m_savedPlaceholders;
    m_editorConfig = m_savedEditorConfig;
}

QJSValue ScriptTester::evaluate(const QString &program)
{
    QStringList stack;
    auto err = m_engine->evaluate(program, u"(program)"_s, 1, &stack);
    if (!stack.isEmpty()) {
        m_engine->throwError(err);
    }
    return err;
}

void ScriptTester::setInput(const QString &input, bool blockSelection)
{
    auto err = m_input.setText(input, m_placeholders);
    if (err.isEmpty() && checkMultiCursorCompatibility(m_input, blockSelection, &err)) {
        m_input.blockSelection = blockSelection;
        initInputDoc();
    } else {
        m_engine->throwError(err);
        ++m_errorCounter;
    }
}

void ScriptTester::moveExpectedOutputToInput(bool blockSelection)
{
    // prefer swap to std::move to avoid freeing vector / list memory
    std::swap(m_input, m_expected);
    reuseInput(blockSelection);
}

void ScriptTester::reuseInput(bool blockSelection)
{
    QString err;
    if (checkMultiCursorCompatibility(m_input, blockSelection, &err)) {
        m_input.blockSelection = blockSelection;
        initInputDoc();
    } else {
        m_engine->throwError(err);
        ++m_errorCounter;
    }
}

bool ScriptTester::reuseInputWithBlockSelection()
{
    QString err;
    if (!checkMultiCursorCompatibility(m_input, true, &err)) {
        return false;
    }
    m_input.blockSelection = true;
    initInputDoc();
    return true;
}

bool ScriptTester::checkMultiCursorCompatibility(const DocumentText &doc, bool blockSelection, QString *err)
{
    if (doc.totalSelection > 1 || doc.totalCursor > 1) {
        if (blockSelection) {
            *err = u"blockSelection is incompatible with multi-cursor/selection"_s;
            return false;
        }
        if (m_doc->config()->ovr()) {
            *err = u"overrideMode is incompatible with multi-cursor/selection"_s;
            return false;
        }
    }

    return true;
}

void ScriptTester::initInputDoc()
{
    auto *docConfig = m_doc->config();
    docConfig->setIndentationMode(m_editorConfig.indentationMode);
    docConfig->setIndentationWidth(m_editorConfig.indentationWidth);
    docConfig->setTabWidth(m_editorConfig.tabWidth);
    docConfig->setReplaceTabsDyn(m_editorConfig.replaceTabs);
    docConfig->setOvr(m_editorConfig.overrideMode);

    m_view->clearSecondaryCursors();

    m_doc->setText(m_input.text);
    m_doc->setHighlightingMode(m_editorConfig.syntax);

    m_view->setBlockSelection(m_input.blockSelection);
    m_view->setSelection(m_input.selection);
    m_view->setCursorPosition(m_input.cursor);

    if (!m_input.secondaryCursorsWithSelection.isEmpty()) {
        m_view->addSecondaryCursorsWithSelection(m_input.secondaryCursorsWithSelection);
    }
}

void ScriptTester::setExpectedOutput(const QString &expected, bool blockSelection)
{
    auto err = m_expected.setText(expected, m_placeholders);
    if (err.isEmpty() && checkMultiCursorCompatibility(m_expected, blockSelection, &err)) {
        m_expected.blockSelection = blockSelection;
    } else {
        m_engine->throwError(err);
        ++m_errorCounter;
    }
}

void ScriptTester::reuseExpectedOutput(bool blockSelection)
{
    QString err;
    if (checkMultiCursorCompatibility(m_expected, blockSelection, &err)) {
        m_expected.blockSelection = blockSelection;
    } else {
        m_engine->throwError(err);
        ++m_errorCounter;
    }
}

void ScriptTester::copyInputToExpectedOutput(bool blockSelection)
{
    m_expected = m_input;
    reuseExpectedOutput(blockSelection);
}

bool ScriptTester::checkOutput()
{
    /*
     * Init m_output
     */
    m_output.text = m_doc->text();
    m_output.totalLine = m_doc->lines();
    m_output.blockSelection = m_view->blockSelection();
    m_output.cursor = m_view->cursorPosition();
    m_output.selection = m_view->selectionRange();

    // init secondaryCursors
    {
        const auto &secondaryCursors = m_view->secondaryCursors();
        m_output.secondaryCursors.resize(secondaryCursors.size());
        auto it = m_output.secondaryCursors.begin();
        for (const auto &c : secondaryCursors) {
            *it++ = {
                .pos = c.cursor(),
                .range = c.range ? c.range->toRange() : Range::invalid(),
            };
        }
    }

    /*
     * Check output
     */
    if (m_output.text != m_expected.text || m_output.blockSelection != m_expected.blockSelection) {
        // differ
    } else if (!m_expected.blockSelection) {
        // compare ignoring virtual column
        auto cursorEq = [this](const Cursor &output, const Cursor &expected) {
            if (output.line() != expected.line()) {
                return false;
            }
            int lineLen = m_doc->lineLength(expected.line());
            int column = qMin(lineLen, expected.column());
            return output.column() == column;
        };
        auto rangeEq = [=](const Range &output, const Range &expected) {
            return cursorEq(output.start(), expected.start()) && cursorEq(output.end(), expected.end());
        };
        auto SecondaryEq = [=](const ViewPrivate::PlainSecondaryCursor &c1, const ViewPrivate::PlainSecondaryCursor &c2) {
            if (!cursorEq(c1.pos, c2.pos) || c1.range.isValid() != c2.range.isValid()) {
                return false;
            }
            return !c1.range.isValid() || rangeEq(c1.range, c2.range);
        };

        if (cursorEq(m_output.cursor, m_expected.cursor) && rangeEq(m_output.selection, m_expected.selection)
            && std::equal(m_output.secondaryCursors.begin(),
                          m_output.secondaryCursors.end(),
                          m_expected.secondaryCursorsWithSelection.constBegin(),
                          m_expected.secondaryCursorsWithSelection.constEnd(),
                          SecondaryEq)) {
            return true;
        }
    } else if (m_output.cursor == m_expected.cursor && m_output.selection == m_expected.selection
               && std::equal(m_output.secondaryCursors.begin(),
                             m_output.secondaryCursors.end(),
                             m_expected.secondaryCursorsWithSelection.constBegin(),
                             m_expected.secondaryCursorsWithSelection.constEnd(),
                             [](const ViewPrivate::PlainSecondaryCursor &c1, const ViewPrivate::PlainSecondaryCursor &c2) {
                                 return c1.pos == c2.pos && c1.range == c2.range;
                             })) {
        return true;
    }

    /*
     * Create a list of all cursors in the document sorted by position
     * with their associated type (TextItem::Kind)
     */

    struct CursorItem {
        Cursor cursor;
        TextItem::Kind kind;
    };
    QVarLengthArray<CursorItem, 12> cursorItems;
    cursorItems.resize(m_output.secondaryCursors.size() * 3 + 3);

    auto it = cursorItems.begin();
    if (m_output.cursor.isValid()) {
        *it++ = {m_output.cursor, TextItem::Cursor};
    }
    if (m_output.selection.isValid()) {
        *it++ = {m_output.selection.start(), TextItem::SelectionStart};
        *it++ = {m_output.selection.end(), TextItem::SelectionEnd};
    }
    for (const auto &c : m_output.secondaryCursors) {
        *it++ = {c.pos, TextItem::SecondaryCursor};
        if (c.range.start().line() != -1) {
            *it++ = {c.range.start(), TextItem::SecondarySelectionStart};
            *it++ = {c.range.end(), TextItem::SecondarySelectionEnd};
        }
    }

    const auto end = it;
    std::sort(cursorItems.begin(), end, [](const CursorItem &a, const CursorItem &b) {
        if (a.cursor < b.cursor) {
            return true;
        }
        if (a.cursor > b.cursor) {
            return false;
        }
        return a.kind < b.kind;
    });
    it = cursorItems.begin();

    /*
     * Init m_output.items
     */

    QStringView output = m_output.text;
    m_output.items.clear();
    m_output.hasFormattingItems = false;

    qsizetype line = 0;
    qsizetype pos = 0;
    for (;;) {
        auto nextPos = output.indexOf(u'\n', pos);
        qsizetype lineLen = nextPos == -1 ? output.size() - pos : nextPos - pos;
        int virtualTextLen = 0;
        for (; it != end && it->cursor.line() == line; ++it) {
            virtualTextLen = (it->cursor.column() > lineLen) ? it->cursor.column() - lineLen : 0;
            m_output.items.push_back({pos + it->cursor.column() - virtualTextLen, it->kind, virtualTextLen});
        }
        if (nextPos == -1) {
            break;
        }
        m_output.items.push_back({nextPos, TextItem::NewLine, virtualTextLen});
        pos = nextPos + 1;
        ++line;
    }

    // no sorting, items are inserted in the right order
    // m_output.sortItems();

    return false;
}

bool ScriptTester::incrementCounter(bool isSuccessNotAFailure)
{
    m_successCounter += isSuccessNotAFailure;
    m_failureCounter += !isSuccessNotAFailure;
    return isSuccessNotAFailure;
}

void ScriptTester::incrementError()
{
    ++m_errorCounter;
}

void ScriptTester::incrementBreakOnError()
{
    ++m_breakOnErrorCounter;
}

int ScriptTester::countError() const
{
    return m_errorCounter + m_failureCounter;
}

bool ScriptTester::hasTooManyErrors() const
{
    return m_executionConfig.maxError > 0 && m_executionConfig.maxError >= countError();
}

int ScriptTester::startTest()
{
    m_debugMsg.clear();
    m_hasDebugMessage = false;
    int flags = 0;
    flags |= m_format.testFormatOptions.testAnyFlag(TestFormatOption::AlwaysWriteInputOutput) ? 1 : 0;
    flags |= m_format.testFormatOptions.testAnyFlag(TestFormatOption::AlwaysWriteLocation) ? 2 : 0;
    return flags;
}

void ScriptTester::endTest(bool ok, bool showBlockSelection)
{
    if (!ok) {
        return;
    }

    constexpr auto mask = TestFormatOptions() | TestFormatOption::AlwaysWriteLocation | TestFormatOption::AlwaysWriteInputOutput;
    if ((m_format.testFormatOptions & mask) != TestFormatOption::AlwaysWriteLocation) {
        return;
    }

    if (showBlockSelection) {
        m_stream << m_format.colors.reset << m_format.colors.blockSelectionInfo
                 << (m_input.blockSelection ? " [blockSelection=1]"_L1 : " [blockSelection=0]"_L1);
    }
    m_stream << m_format.colors.reset << m_format.colors.success << " Ok\n"_L1 << m_format.colors.reset;
}

void ScriptTester::writeTestExpression(const QString &name, const QString &type, int nthStack, const QString &program)
{
    /*
     * format with optional testName
     * ${fileName}:${lineNumber}: ${testName}: ${type} `${program}`
     */

    // ${fileName}:${lineNumber}:
    writeLocation(nthStack);
    // ${testName}:
    writeTestName(name);
    // ${type} `${program}`
    writeTypeAndProgram(type, program);

    if (m_format.debugOptions.testAnyFlag(DebugOption::ForceFlush)) {
        m_stream.flush();
    }
}

void ScriptTester::writeDualModeAborted(const QString &name, int nthStack)
{
    ++m_dualModeAbortedCounter;
    writeLocation(nthStack);
    writeTestName(name);
    m_stream << m_format.colors.error << "cmp DUAL_MODE"_L1 << m_format.colors.reset << m_format.colors.blockSelectionInfo << " [blockSelection=1]"_L1
             << m_format.colors.reset << m_format.colors.error << " Aborted\n"_L1 << m_format.colors.reset;
}

void ScriptTester::writeTestName(const QString &name)
{
    if (!m_format.testFormatOptions.testAnyFlag(TestFormatOption::HiddenTestName) && !name.isEmpty()) {
        m_stream << m_format.colors.testName << name << m_format.colors.reset << ": "_L1;
    }
}

void ScriptTester::writeTypeAndProgram(const QString &type, const QString &program)
{
    m_stream << m_format.colors.error << type << " `"_L1 << m_format.colors.reset << m_format.colors.program << program << m_format.colors.reset
             << m_format.colors.error << '`';
}

void ScriptTester::writeTestResult(const QString &name,
                                   const QString &type,
                                   int nthStack,
                                   const QString &program,
                                   const QString &msg,
                                   const QJSValue &exception,
                                   const QString &result,
                                   const QString &expectedResult,
                                   int options)
{
    constexpr int outputIsOk = 1 << 0;
    constexpr int containsResultOrError = 1 << 1;
    constexpr int expectedErrorButNoError = 1 << 2;
    constexpr int expectedNoErrorButError = 1 << 3;
    constexpr int isResultNotError = 1 << 4;
    constexpr int sameResultOrError = 1 << 5;
    constexpr int ignoreInputOutput = 1 << 6;

    const bool alwaysWriteTest = m_format.testFormatOptions.testAnyFlag(TestFormatOption::AlwaysWriteLocation);
    const bool alwaysWriteInputOutput = m_format.testFormatOptions.testAnyFlag(TestFormatOption::AlwaysWriteInputOutput);

    const bool outputDiffer = !(options & (outputIsOk | ignoreInputOutput));
    const bool resultDiffer = options & (containsResultOrError | expectedNoErrorButError);

    /*
     * format with optional testName and msg
     * alwaysWriteTest = false
     *      ${fileName}:${lineNumber}: ${testName}: {Output/Result} differs
     *      ${type} `${program}` -- ${msg} ${blockSelectionMode}:
     *
     * alwaysWriteTest = true
     *      format with optional msg
     *      {Output/Result} differs -- ${msg} ${blockSelectionMode}:
     */
    if (alwaysWriteTest) {
        if (alwaysWriteInputOutput && !outputDiffer && !resultDiffer) {
            m_stream << m_format.colors.success << " OK"_L1;
        } else if (!m_hasDebugMessage) {
            m_stream << '\n';
        }
    } else {
        // ${fileName}:${lineNumber}:
        writeLocation(nthStack);
        // ${testName}:
        writeTestName(name);
    }
    // {Output/Result} differs
    if (outputDiffer && resultDiffer) {
        m_stream << m_format.colors.error << "Output and Result differs"_L1;
    } else if (resultDiffer) {
        m_stream << m_format.colors.error << "Result differs"_L1;
    } else if (outputDiffer) {
        m_stream << m_format.colors.error << "Output differs"_L1;
    } else if (alwaysWriteInputOutput && !alwaysWriteTest) {
        m_stream << m_format.colors.success << "OK"_L1;
    }
    if (!alwaysWriteTest) {
        m_stream << '\n';
        // ${type} `${program}`
        writeTypeAndProgram(type, program);
    }
    // -- ${msg}
    if (!msg.isEmpty()) {
        m_stream << " -- "_L1 << msg;
    }
    // ${blockSelectionMode}:
    m_stream << m_format.colors.reset << m_format.colors.blockSelectionInfo;
    if (m_output.blockSelection == m_expected.blockSelection && m_expected.blockSelection == m_input.blockSelection) {
        m_stream << (m_input.blockSelection ? " [blockSelection=1]"_L1 : " [blockSelection=0]"_L1);
    } else {
        m_stream << " [blockSelection=(input="_L1 << m_input.blockSelection << ", output="_L1 << m_output.blockSelection << ", expected="_L1
                 << m_expected.blockSelection << ")]"_L1;
    }
    m_stream << m_format.colors.reset << ":\n"_L1;

    /*
     * Display buffered debug messages
     */
    m_stream << m_debugMsg;
    m_debugMsg.clear();

    /*
     * Editor result block
     */
    if (!(options & ignoreInputOutput)) {
        writeDataTest(options & outputIsOk);
    }

    /*
     * Function result block (exception caught or return value)
     */
    if (options & (containsResultOrError | sameResultOrError)) {
        if (!(options & ignoreInputOutput)) {
            m_stream << "  ---------\n"_L1;
        }

        // display
        //  result:   ... (optional)
        //  expected: ...
        if (options & expectedErrorButNoError) {
            m_stream << m_format.colors.error << "  An error is expected, but there is none"_L1 << m_format.colors.reset << '\n';
            if (!result.isEmpty()) {
                writeLabel(m_stream, m_format.colors, false, "  result:   "_L1);
                m_stream << m_format.colors.result << result << m_format.colors.reset << '\n';
            }
            m_stream << "  expected: "_L1;
            m_stream << m_format.colors.result << expectedResult << m_format.colors.reset << '\n';
        }
        // display
        //  result:   ... (or error:)
        //  expected: ... (optional)
        else {
            auto label = (options & (isResultNotError | sameResultOrError)) ? "  result:   "_L1 : "  error:    "_L1;
            writeLabel(m_stream, m_format.colors, options & sameResultOrError, label);

            m_stream << m_format.colors.result << result << m_format.colors.reset << '\n';
            if (!(options & sameResultOrError)) {
                auto differPos = computeOffsetDifference(result, expectedResult);
                m_stream << "  expected: "_L1;
                m_stream << m_format.colors.result << expectedResult << m_format.colors.reset << '\n';
                writeCarretLine(m_stream, m_format.colors, differPos + 12);
            }
        }
    }

    /*
     * Uncaught exception block
     */
    if (options & expectedNoErrorButError) {
        m_stream << "  ---------\n"_L1 << m_format.colors.error << "  Uncaught exception: "_L1 << exception.toString() << '\n';
        writeException(exception, u"  | "_sv);
    }

    m_stream << '\n';
}

void ScriptTester::writeException(const QJSValue &exception, QStringView prefix)
{
    const auto stack = getStack(exception);
    if (stack.isUndefined()) {
        m_stream << m_format.colors.error << prefix << "undefined\n"_L1 << m_format.colors.reset;
    } else {
        m_stringBuffer.clear();
        pushException(m_stringBuffer, m_format.colors, stack.toString(), prefix);
        m_stream << m_stringBuffer;
    }
}

void ScriptTester::writeLocation(int nthStack)
{
    const auto errStr = generateStack(m_engine).toString();
    const QStringView err = errStr;

    // skip lines
    qsizetype startIndex = 0;
    while (nthStack-- > 0) {
        qsizetype pos = err.indexOf('\n'_L1, startIndex);
        if (pos <= -1) {
            break;
        }
        startIndex = pos + 1;
    }

    auto stackLine = parseStackLine(err.sliced(startIndex));
    m_stream << m_format.colors.fileName << stackLine.fileName << m_format.colors.reset << ':' << m_format.colors.lineNumber << stackLine.lineNumber
             << m_format.colors.reset << ": "_L1;
}

/**
 * Builds the map to convert \c TextItem::Kind to \c QStringView.
 */
struct ScriptTester::Replacements {
    const QStringView selectionStartPlaceholder;
    const QStringView selectionEndPlaceholder;
    const QStringView secondarySelectionStartPlaceholder;
    const QStringView secondarySelectionEndPlaceholder;
    const QChar virtualTextPlaceholder;

    // index 0 = color; index 1 = replacement text
    QStringView replacements[TextItem::MaxElement][2];
    int tabWidth = 0;

    static constexpr int tabBufferLen = 16;
    QChar tabBuffer[tabBufferLen];

#define GET_PLACEHOLDER(name, b) QStringView(placeholders.name != u'\0' && placeholders.name != u'\n' && b ? &placeholders.name : &fallbackPlaceholders.name, 1)

    Replacements(const Colors &colors, const Placeholders &placeholders, const Placeholders &fallbackPlaceholders)
        : selectionStartPlaceholder(GET_PLACEHOLDER(selectionStart, true))
        , selectionEndPlaceholder(GET_PLACEHOLDER(selectionEnd, true))
        , secondarySelectionStartPlaceholder(GET_PLACEHOLDER(secondarySelectionStart, true))
        , secondarySelectionEndPlaceholder(GET_PLACEHOLDER(secondarySelectionEnd, true))
        , virtualTextPlaceholder(GET_PLACEHOLDER(virtualText,
                                                 placeholders.virtualText != placeholders.cursor && placeholders.virtualText != placeholders.selectionStart
                                                     && placeholders.virtualText != placeholders.selectionEnd)[0])
    {
        replacements[TextItem::SecondarySelectionStart][0] = colors.secondarySelection;
        replacements[TextItem::SecondarySelectionStart][1] = secondarySelectionStartPlaceholder;
        replacements[TextItem::SecondarySelectionEnd][0] = colors.secondarySelection;
        replacements[TextItem::SecondarySelectionEnd][1] = secondarySelectionEndPlaceholder;

        replacements[TextItem::Cursor][0] = colors.cursor;
        replacements[TextItem::Cursor][1] =
            GET_PLACEHOLDER(cursor, selectionStartPlaceholder != placeholders.cursor && selectionEndPlaceholder != placeholders.cursor);

        replacements[TextItem::SecondaryCursor][0] = colors.secondaryCursor;
        replacements[TextItem::SecondaryCursor][1] = GET_PLACEHOLDER(
            secondaryCursor,
            selectionStartPlaceholder != placeholders.secondaryCursor && selectionEndPlaceholder != placeholders.secondaryCursor
                && secondarySelectionStartPlaceholder != placeholders.secondaryCursor && secondarySelectionEndPlaceholder != placeholders.secondaryCursor);

        replacements[TextItem::SelectionStart][0] = colors.selection;
        replacements[TextItem::SelectionEnd][0] = colors.selection;

        replacements[TextItem::BlockSelectionStart][0] = colors.blockSelection;
        replacements[TextItem::BlockSelectionEnd][0] = colors.blockSelection;
        replacements[TextItem::VirtualBlockCursor][0] = colors.blockSelection;
        replacements[TextItem::VirtualBlockSelectionStart][0] = colors.blockSelection;
        replacements[TextItem::VirtualBlockSelectionEnd][0] = colors.blockSelection;
    }

#undef GET_PLACEHOLDER

    void initEscapeForDoubleQuote(const Colors &colors)
    {
        replacements[TextItem::NewLine][0] = colors.resultReplacement;
        replacements[TextItem::NewLine][1] = u"\\n"_sv;
        replacements[TextItem::Tab][0] = colors.resultReplacement;
        replacements[TextItem::Tab][1] = u"\\t"_sv;
        replacements[TextItem::Backslash][0] = colors.resultReplacement;
        replacements[TextItem::Backslash][1] = u"\\\\"_sv;
        replacements[TextItem::DoubleQuote][0] = colors.resultReplacement;
        replacements[TextItem::DoubleQuote][1] = u"\\\""_sv;
    }

    void initReplaceNewLineAndTabWithLiteral(const Colors &colors)
    {
        replacements[TextItem::NewLine][0] = colors.resultReplacement;
        replacements[TextItem::NewLine][1] = u"\\n"_sv;
        replacements[TextItem::Tab][0] = colors.resultReplacement;
        replacements[TextItem::Tab][1] = u"\\t"_sv;
    }

    void initNewLine(const Format &format)
    {
        const auto &newLine = format.textReplacement.newLine;
        replacements[TextItem::NewLine][0] = format.colors.resultReplacement;
        replacements[TextItem::NewLine][1] = newLine.unicode() ? QStringView(&newLine, 1) : u"↵"_sv;
    }

    void initTab(const Format &format, DocumentPrivate *doc)
    {
        const auto &repl = format.textReplacement;
        tabWidth = qMin(doc->config()->tabWidth(), tabBufferLen);
        if (tabWidth > 0) {
            for (int i = 0; i < tabWidth - 1; ++i) {
                tabBuffer[i] = repl.tab1;
            }
            tabBuffer[tabWidth - 1] = repl.tab2;
        }
        replacements[TextItem::Tab][0] = format.colors.resultReplacement;
        replacements[TextItem::Tab][1] = QStringView(tabBuffer, tabWidth);
    }

    void initSelections(bool hasVirtualBlockSelection, bool reverseSelection)
    {
        if (hasVirtualBlockSelection && reverseSelection) {
            replacements[TextItem::SelectionStart][1] = selectionEndPlaceholder;
            replacements[TextItem::SelectionEnd][1] = selectionStartPlaceholder;

            replacements[TextItem::BlockSelectionStart][1] = selectionStartPlaceholder;
            replacements[TextItem::BlockSelectionEnd][1] = selectionEndPlaceholder;
        } else {
            replacements[TextItem::SelectionStart][1] = selectionStartPlaceholder;
            replacements[TextItem::SelectionEnd][1] = selectionEndPlaceholder;
            if (hasVirtualBlockSelection) {
                replacements[TextItem::BlockSelectionStart][1] = selectionEndPlaceholder;
                replacements[TextItem::BlockSelectionEnd][1] = selectionStartPlaceholder;
            }
        }

        if (hasVirtualBlockSelection) {
            replacements[TextItem::VirtualBlockCursor][1] = replacements[TextItem::Cursor][1];
            replacements[TextItem::VirtualBlockSelectionStart][1] = selectionStartPlaceholder;
            replacements[TextItem::VirtualBlockSelectionEnd][1] = selectionEndPlaceholder;
        } else {
            replacements[TextItem::BlockSelectionStart][1] = QStringView();
            replacements[TextItem::BlockSelectionEnd][1] = QStringView();
            replacements[TextItem::VirtualBlockCursor][1] = QStringView();
            replacements[TextItem::VirtualBlockSelectionStart][1] = QStringView();
            replacements[TextItem::VirtualBlockSelectionEnd][1] = QStringView();
        }
    }

    const QStringView (&operator[](TextItem::Kind i) const)[2]
    {
        return replacements[i];
    }
};

void ScriptTester::writeDataTest(bool outputIsOk)
{
    Replacements replacements(m_format.colors, m_placeholders, m_fallbackPlaceholders);

    const auto textFormat = (m_input.blockSelection || m_output.blockSelection || (outputIsOk && m_expected.blockSelection))
        ? m_format.documentTextFormatWithBlockSelection
        : m_format.documentTextFormat;

    bool alignNL = true;

    switch (textFormat) {
    case DocumentTextFormat::Raw:
        break;
    case DocumentTextFormat::EscapeForDoubleQuote:
        replacements.initEscapeForDoubleQuote(m_format.colors);
        alignNL = false;
        break;
    case DocumentTextFormat::ReplaceNewLineAndTabWithLiteral:
        replacements.initReplaceNewLineAndTabWithLiteral(m_format.colors);
        alignNL = false;
        break;
    case DocumentTextFormat::ReplaceNewLineAndTabWithPlaceholder:
        replacements.initNewLine(m_format);
        [[fallthrough]];
    case DocumentTextFormat::ReplaceTabWithPlaceholder:
        replacements.initTab(m_format, m_doc);
        break;
    }

    auto writeText = [&](const DocumentText &docText, qsizetype carretLine = -1, qsizetype carretColumn = -1, bool lastCall = false) {
        const bool hasVirtualBlockSelection = (docText.blockSelection && docText.selection.start().line() != -1);

        const QStringView inSelectionFormat = (hasVirtualBlockSelection && docText.selection.columnWidth() == 0) ? QStringView() : m_format.colors.inSelection;

        replacements.initSelections(hasVirtualBlockSelection, docText.selection.columnWidth() < 0);

        QStringView inSelection;
        bool showCarret = (carretColumn != -1);
        qsizetype line = 0;
        qsizetype previousLinePos = 0;
        qsizetype virtualTabLen = 0;
        qsizetype textPos = 0;
        qsizetype virtualTextLen = 0;
        for (const TextItem &item : docText.items) {
            // displays the text between 2 items
            if (textPos != item.pos) {
                auto textFragment = docText.text.sliced(textPos, item.pos - textPos);
                m_stream << m_format.colors.result << inSelection << textFragment << m_format.colors.reset;
            }

            // insert virtual text symbols
            if (virtualTextLen < item.virtualTextLen && docText.blockSelection) {
                m_stream << m_format.colors.reset << m_format.colors.virtualText << inSelection;
                m_stream.setPadChar(replacements.virtualTextPlaceholder);
                m_stream.setFieldWidth(item.virtualTextLen - virtualTextLen);
                m_stream << ""_L1;
                m_stream.setFieldWidth(0);
                if (!m_format.colors.virtualText.isEmpty() || !inSelection.isEmpty()) {
                    m_stream << m_format.colors.reset;
                }
                textPos = item.pos + item.isCharacter();
                virtualTextLen = item.virtualTextLen;
            }

            // update selection text state (close selection)
            const bool isInSelection = !inSelection.isEmpty();
            if (isInSelection && item.isSelection(hasVirtualBlockSelection)) {
                inSelection = QStringView();
            }

            // display item
            const auto &replacement = replacements[item.kind];
            if (!replacement[1].isEmpty()) {
                m_stream << replacement[0] << inSelection;
                // adapts tab size to be a multiple of tabWidth
                // tab="->" tabWidth=4
                // input:  ab\t\tc
                // output: ab->--->c
                //         ~~~~ = tabWidth
                if (item.kind == TextItem::Tab && replacements.tabWidth) {
                    const auto column = item.pos - previousLinePos + virtualTabLen;
                    const auto skip = column % replacements.tabWidth;
                    virtualTabLen += replacement[1].size() - skip - 1;
                    m_stream << replacement[1].sliced(skip);
                } else {
                    m_stream << replacement[1];
                }
            }

            const bool insertNewLine = (alignNL && item.kind == TextItem::NewLine);
            if (insertNewLine || (!replacement[1].isEmpty() && (!replacement[0].isEmpty() || !inSelection.isEmpty()))) {
                m_stream << m_format.colors.reset;
                if (insertNewLine) {
                    m_stream << '\n';
                    if (showCarret && carretLine == line) {
                        showCarret = false;
                        writeCarretLine(m_stream, m_format.colors, carretColumn);
                    }
                    m_stream << "            "_L1;
                    ++line;
                }
            }
            if (item.kind == TextItem::NewLine) {
                virtualTabLen = 0;
                virtualTextLen = 0;
                previousLinePos = item.pos + 1;
            }

            // update selection text state (open selection)
            if (!isInSelection && item.isSelection(hasVirtualBlockSelection)) {
                inSelection = inSelectionFormat;
            }

            textPos = item.pos + item.isCharacter();
        }

        // display the remaining text
        if (textPos != docText.text.size()) {
            m_stream << m_format.colors.result << docText.text.sliced(textPos) << m_format.colors.reset;
        }

        m_stream << '\n';

        if (showCarret) {
            writeCarretLine(m_stream, m_format.colors, carretColumn);
        } else if (alignNL && docText.totalLine > 1 && !lastCall) {
            m_stream << '\n';
        }
    };

    m_input.insertFormattingItems(textFormat);
    writeLabel(m_stream, m_format.colors, outputIsOk, "  input:    "_L1);
    writeText(m_input);

    m_expected.insertFormattingItems(textFormat);
    writeLabel(m_stream, m_format.colors, outputIsOk, "  output:   "_L1);
    if (outputIsOk) {
        writeText(m_expected);
    } else {
        m_output.insertFormattingItems(textFormat);

        /*
         * Compute carret position
         */
        qsizetype carretLine = 0;
        qsizetype carretColumn = 0;
        qsizetype ignoredLen = 0;
        auto differPos = computeOffsetDifference(m_output.text, m_expected.text);
        auto it1 = m_output.items.begin();
        auto it2 = m_expected.items.begin();
        for (; it1 != m_output.items.end() && it2 != m_expected.items.end() && it1->pos == it2->pos && it1->kind == it2->kind
             && it1->virtualTextLen == (m_expected.blockSelection ? it2->virtualTextLen : 0) && differPos > it1->pos;
             ++it1, ++it2) {
            carretColumn += it1->virtualTextLen + replacements[it1->kind][1].size() - it1->isCharacter();
            if (alignNL && it1->kind == TextItem::NewLine) {
                ++carretLine;
                carretColumn = 0;
                ignoredLen = it1->pos + 1;
            }
        }
        if (it1 != m_output.items.end() && it1->pos < differPos) {
            differPos = it1->pos;
        }
        if (it2 != m_expected.items.end() && it2->pos < differPos) {
            differPos = it2->pos;
        }

        carretColumn += 12 + differPos - ignoredLen;

        /*
         * Display output and expected output
         */
        const bool insertCarretOnOutput = (alignNL && (m_output.totalLine > 1 || m_expected.totalLine > 1));
        writeText(m_output, carretLine, insertCarretOnOutput ? carretColumn : -1);
        m_stream << "  expected: "_L1;
        writeText(m_expected, carretLine, carretColumn, true);
    }
}

void ScriptTester::writeAndResetCounters()
{
    auto &colors = m_format.colors;

    if (m_failureCounter || m_format.testFormatOptions.testAnyFlag(TestFormatOption::AlwaysWriteLocation)) {
        m_stream << '\n';
    }

    if (m_skipedCounter || m_breakOnErrorCounter) {
        m_stream << colors.labelInfo << "Test cases:  Skipped: "_L1 << m_skipedCounter << "  Aborted: "_L1 << m_breakOnErrorCounter << colors.reset << '\n';
    }

    m_stream << "Success: "_L1 << colors.success << m_successCounter << colors.reset << "  Failure: "_L1 << (m_failureCounter ? colors.error : colors.success)
             << m_failureCounter << colors.reset;

    if (m_dualModeAbortedCounter) {
        m_stream << "  DUAL_MODE aborted: "_L1 << colors.error << m_dualModeAbortedCounter << colors.reset;
    }

    if (m_errorCounter) {
        m_stream << "  Error: "_L1 << colors.error << m_errorCounter << colors.reset;
    }

    m_successCounter = 0;
    m_failureCounter = 0;
    m_skipedCounter = 0;
    m_errorCounter = 0;
    m_breakOnErrorCounter = 0;
}

} // namespace KTextEditor

#include "moc_scripttester_p.cpp"
