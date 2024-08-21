var katescript = {
    "author": "Dominik Haumann <dhdev@gmx.de>, Milian Wolff <mail@milianw.de>, Gerald Senarclens de Grancy <oss@senarclens.eu>, Alex Turbov <i.zaufi@gmail.com>, Pablo Rauzy <r_NOSPAM_@uzy.me>, Henri Kaustinen <heka1@protonmail.com>",
    "license": "LGPL-2.1+",
    "revision": 12,
    "kate-version": "5.1",
    "functions": ["moveLinesDown", "moveLinesUp", "rtrim", "ltrim", "trim", "join", "rmblank", "alignon", "unwrap", "each", "filter", "map", "duplicateLinesUp", "duplicateLinesDown", "duplicateSelection", "rewrap", "encodeURISelection", "decodeURISelection", "fsel", "bsel"],
    "actions": [
        {   "function": "rtrim",
            "name": "Remove Trailing Spaces",
            "category": "Editing"
        },
        {   "function": "ltrim",
            "name": "Remove Leading Spaces",
            "category": "Editing"
        },
        {   "function": "trim",
            "name": "Remove Trailing and Leading Spaces",
            "category": "Editing"
        },
        {   "function": "join",
            "name": "Join Lines",
            "category": "Editing"
        },
        {   "function": "moveLinesDown",
            "name": "Move Lines Down",
            "shortcut": "Ctrl+Shift+Down",
            "category": "Editing"
        },
        {   "function": "moveLinesUp",
            "name": "Move Lines Up",
            "shortcut": "Ctrl+Shift+Up",
            "category": "Editing"
        },
        {   "function": "duplicateLinesDown",
            "name": "Duplicate Selected Lines Down",
            "category": "Editing"
        },
        {   "function": "duplicateLinesUp",
            "name": "Duplicate Selected Lines Up",
            "category": "Editing"
        },
        {   "function": "duplicateSelection",
            "name": "Duplicate selected text",
            "category": "Editing"
        },
        {   "function": "encodeURISelection",
            "name": "URI-encode Selected Text",
            "category": "Editing"
        },
        {   "function": "decodeURISelection",
            "name": "URI-decode Selected Text",
            "category": "Editing"
        },
        {   "function": "rmblank",
            "name": "Remove Empty Lines",
            "category": "Editing"
        }
    ]
}; // kate-script-header, must be at the start of the file without comments, pure json

// required katepart js libraries
require ("range.js");

function rtrim()
{
    map(function(l){ return l.replace(/\s+$/, ''); });
}

function ltrim()
{
    map(function(l){ return l.replace(/^\s+/, ''); });
}

function trim()
{
    map(function(l){ return l.replace(/^\s+|\s+$/, ''); });
}

function rmblank()
{
    filter(function(l) { return l.length > 0; });
}

function alignon(pattern)
{
    if (typeof pattern == "undefined") {
        pattern = "";
    }
    var selection = view.selection();
    if (!selection.isValid()) {
        selection = document.documentRange();
    }
    view.alignOn(selection, pattern);
}

function join(separator)
{
    if (typeof(separator) != "string") {
        separator = "";
    }
    each(function(lines){
        return [lines.join(separator)];
    });
}

// unwrap does the opposite of the script word wrap
function unwrap ()
{
    var selectionRange = view.selection();
    if (selectionRange.isValid()) {
        // unwrap all paragraphs in the selection range
        var currentLine = selectionRange.start.line;
        var count = selectionRange.end.line - selectionRange.start.line;

        document.editBegin();
        while (count >= 0) {
            // skip empty lines
            while (count >= 0 && document.firstColumn(currentLine) == -1) {
                --count;
                ++currentLine;
            }

            // find block of text lines to join
            var anchorLine = currentLine;
            while (count >= 0 && document.firstColumn(currentLine) != -1) {
                --count;
                ++currentLine;
            }

            if (currentLine != anchorLine) {
                document.joinLines(anchorLine, currentLine - 1);
                currentLine -= currentLine - anchorLine - 1;
            }
        }
        document.editEnd();
    } else {
        // unwrap paragraph under the cursor
        var cursorPosition = view.cursorPosition();
        if (document.firstColumn(cursorPosition.line) != -1) {
            var startLine = cursorPosition.line;
            while (startLine > 0) {
                if (document.firstColumn(startLine - 1) == -1) {
                    break;
                }
                --startLine;
            }

            var endLine = cursorPosition.line;
            var lineCount = document.lines();
            while (endLine < lineCount) {
                if (document.firstColumn(endLine + 1) == -1) {
                    break;
                }
                ++endLine;
            }

            if (startLine != endLine) {
                document.editBegin();
                document.joinLines(startLine, endLine);
                document.editEnd();
            }
        }
    }
}

/// \note Range contains a block selected by lines (\b ONLY)!
/// And it is an open range! I.e. <em>[start, end)</em> (like in STL).
function _getBlockForAction()
{
    // Check if selection present in a view...
    var blockRange = Range(view.selection());
    var cursorPosition = view.cursorPosition();
    if (blockRange.isValid()) {
        blockRange.start.column = 0;
        if (blockRange.end.column != 0)
            blockRange.end.line++;
        blockRange.end.column = 0;
    } else {
        // No, it doesn't! Ok, lets select the current line only
        // from current position to the end
        blockRange = new Range(cursorPosition.line, 0, cursorPosition.line + 1, 0);
    }
    return blockRange;
}

// Adjusts ("moves") the current selection by offset.
// Positive offsets move down, negatives up.
function _adjustSelection(selection, offset)
{
    if (selection.end.line + offset < document.lines()) {
        selection = new Range(selection.start.line + offset, selection.start.column,
                              selection.end.line + offset, selection.end.column);
    } else {
        selection = new Range(selection.start.line + offset, selection.start.column,
                              selection.end.line + offset - 1,
                              document.lineLength(selection.end.line + offset - 1));
    }
    view.setSelection(selection);
}

function moveLinesDown()
{
    var selection = view.selection();
    var blockRange = _getBlockForAction();
    // Check is there a space to move?
    if (blockRange.end.line < document.lines()) {
        document.editBegin();
        // Move a block to one line down:
        // 0) take one line after the block
        var text = document.line(blockRange.end.line);
        // 1) remove the line after the block
        document.removeLine(blockRange.end.line);
        // 2) insert a line before the block
        document.insertLine(blockRange.start.line, text);
        document.editEnd();
        if (view.hasSelection())
            _adjustSelection(selection, 1);
    }
}

function moveLinesUp()
{
    var cursor = view.cursorPosition();
    var selection = view.selection();
    var blockRange = _getBlockForAction();
    // Check is there a space to move?
    if (0 < blockRange.start.line) {
        document.editBegin();
        // Move a block to one line up:
        // 0) take one line before the block,
        var text = document.line(blockRange.start.line - 1);
        // 1) and insert it after the block
        document.insertLine(blockRange.end.line, text);
        // 2) remove the original line
        document.removeLine(blockRange.start.line - 1);
        view.setCursorPosition(cursor.line - 1, cursor.column);
        if (view.hasSelection())
            _adjustSelection(selection, -1);
        document.editEnd();
    }
}

function duplicateLinesDown()
{
    var selection = view.selection();
    var blockRange = _getBlockForAction();
    document.editBegin();
    document.insertText(blockRange.start, document.text(blockRange));
    _adjustSelection(selection, blockRange.end.line - blockRange.start.line);
    document.editEnd();
}

function duplicateLinesUp()
{
    var cursor = view.cursorPosition();
    var selection = view.selection();
    var blockRange = _getBlockForAction();
    document.editBegin();
    if (blockRange.end.line == document.lines()) {
        var lastLine = document.lines() - 1;
        var lastCol = document.lineLength(document.lines() - 1);
        blockRange.end.line = lastLine;
        blockRange.end.column = lastCol;
        document.insertText(lastLine, lastCol, document.text(blockRange));
        document.wrapLine(lastLine, lastCol);
    } else {
        document.insertText(blockRange.end, document.text(blockRange));
    }
    view.setCursorPosition(cursor.line, cursor.column);
    _adjustSelection(selection, 0);
    document.editEnd();
}


// Duplicate selected text at start of selection. Place cursor after selection.
function duplicateSelection()
{
    var selection = view.selectedText();

    if (!selection) {
        return
    }

    var range = view.selection();
    document.insertText(range.start, selection);
    range = view.selection();
    view.setCursorPosition(range.end.line, range.end.column);
}

function rewrap()
{
    // initialize line span
    var fromLine = view.cursorPosition().line;
    var toLine = fromLine;
    var hasSelection = view.hasSelection();

    // if a text selection is present, use it to reformat the paragraph
    if (hasSelection) {
        var range = view.selection();
        fromLine = range.start.line;
        toLine = range.end.line;
    } else {
        // abort, if the cursor is in an empty line
        if (document.firstColumn(fromLine) == -1)
            return;

        // no text selection present: search for start & end of paragraph
        while (fromLine > 0 &&
            document.prevNonEmptyLine(fromLine-1) == fromLine - 1) --fromLine;
        while (toLine < document.lines() - 1 &&
            document.nextNonEmptyLine(toLine+1) == toLine + 1) ++toLine;
    }

    // initialize wrap columns
    var wrapColumn = 80;
    var softWrapColumn = 82;
    var exceptionColumn = 70;

    document.editBegin();

    if (fromLine < toLine) {
        document.joinLines(fromLine, toLine);
    }

    var line = fromLine;
    // as long as the line is too long...
    while (document.lastColumn(line) > wrapColumn) {
        // ...search for current word boundaries...
        var range = document.wordRangeAt(line, wrapColumn);
        if (!range.isValid()) {
            break;
        }

        // ...and wrap at a 'smart' position
        var wrapCursor = range.start;
        if (range.start.column < exceptionColumn && range.end.column <= softWrapColumn) {
            wrapCursor = range.end;
        }
        if (!document.wrapLine(wrapCursor))
            break;
        ++line;
    }

    document.editEnd();
}

function _uri_transform_selection(transformer)
{
    var selection = view.selection();
    var cursor = view.cursorPosition();
    // TODO Multiline conversions are meaningless!?
    if (selection.isValid() && selection.onSingleLine()) {
        var text = document.text(selection);
        var coded_text = transformer(text);
        document.editBegin();
        document.removeText(selection);
        document.insertText(selection.start, coded_text);
        document.editEnd();
        var size_diff = coded_text.length - text.length;
        selection.end.column += size_diff;
        view.setSelection(selection);
        if (selection.start.column < cursor.column) {
            cursor.column += size_diff;
        }
        view.setCursorPosition(cursor);
    }
}

function encodeURISelection()
{
    _uri_transform_selection(encodeURIComponent);
}

function decodeURISelection()
{
    _uri_transform_selection(decodeURIComponent);
}

function fsel(target) // forward select
{
    startSel = view.cursorPosition();
    if (typeof target == "undefined") { // by default, select til the end of the current line
        endSel = new Cursor(startSel.line, document.lastColumn(startSel.line) + 1);
    } else { // otherwise, select to the first occurrence of the given target (including it)
        match = view.searchText(new Range(startSel, document.documentRange().end), target);
        if (!match.isValid()) return false;
        else endSel = match.end;
    }
    view.setCursorPosition(endSel);
    view.setSelection(new Range(startSel, endSel));
}

function bsel(target) // backward select
{
    endSel = view.cursorPosition();
    if (typeof target == "undefined") { // by default, select from the beginning of the current line
        startSel = new Cursor(endSel.line, 0);
    } else { // otherwise, select from the last occurrence of the given target (including it)
        match = view.searchText(new Range(document.documentRange().start, endSel), target, true);
        if (!match.isValid()) return false;
        else startSel = match.start;
    }
    view.setCursorPosition(startSel);
    view.setSelection(new Range(startSel, endSel));
}

function help(cmd)
{
    if (cmd == "sort") {
        return i18n("Sort the selected text or whole document.");
    } else if (cmd == "moveLinesDown") {
        return i18n("Move selected lines down.");
    } else if (cmd == "moveLinesUp") {
        return i18n("Move selected lines up.");
    } else if (cmd == "uniq") {
        return i18n("Remove duplicate lines from the selected text or whole document.");
    } else if (cmd == "natsort") {
        return i18n("Sort the selected text or whole document in natural order.<br>Here is an example to show the difference to the normal sort method:<br>sort(a10, a1, a2) => a1, a10, a2<br>natsort(a10, a1, a2) => a1, a2, a10");
    } else if (cmd == "rtrim") {
        return i18n("Trims trailing whitespace from selection or whole document.");
    } else if (cmd == "ltrim") {
        return i18n("Trims leading whitespace from selection or whole document.");
    } else if (cmd == "trim") {
        return i18n("Trims leading and trailing whitespace from selection or whole document.");
    } else if (cmd == "join") {
        return i18n("Joins selected lines or whole document. Optionally pass a separator to put between each line:<br><code>join ', '</code> will e.g. join lines and separate them by a comma.");
    } else if (cmd == "rmblank") {
        return i18n("Removes empty lines from selection or whole document.");
    } else if (cmd == "alignon") {
        return i18n("This command aligns lines in the selected block or whole document on the column given by a regular expression given as an argument.<br><br>If you give an empty pattern it will align on the first non-blank character by default.<br>If the pattern has a capture it will indent on the captured match.<br><br><i>Examples</i>:<br>'<code>alignon -</code>' will insert spaces before the first '-' of each lines to align them all on the same column.<br>'<code>alignon :\\s+(.)</code>' will insert spaces before the first non-blank character that occurs after a colon to align them all on the same column.");
    } else if (cmd == "unwrap") {
        return "Unwraps all paragraphs in the text selection, or the paragraph under the text cursor if there is no selected text.";
    } else if (cmd == "each") {
        return i18n("Given a JavaScript function as argument, call that for the list of (selected) lines and replace them with the return value of that callback.<br>Example (join selected lines):<br><code>each 'function(lines){return lines.join(\", \");}'</code><br>To save you some typing, you can also do this to achieve the same:<br><code>each 'lines.join(\", \")'</code>");
    } else if (cmd == "filter") {
        return i18n("Given a JavaScript function as argument, call that for the list of (selected) lines and remove those where the callback returns false.<br>Example (see also <code>rmblank</code>):<br><code>filter 'function(l){return l.length > 0;}'</code><br>To save you some typing, you can also do this to achieve the same:<br><code>filter 'line.length > 0'</code>");
    } else if (cmd == "map") {
        return i18n("Given a JavaScript function as argument, call that for the list of (selected) lines and replace the line with the return value of the callback.<br>Example (see also <code>ltrim</code>):<br><code>map 'function(line){return line.replace(/^\\s+/, \"\");}'</code><br>To save you some typing, you can also do this to achieve the same:<br><code>map 'line.replace(/^\\s+/, \"\")'</code>");
    } else if (cmd == "duplicateLinesUp") {
        return i18n("Duplicates the selected lines up.");
    } else if (cmd == "duplicateLinesDown") {
        return i18n("Duplicates the selected lines down.");
    } else if (cmd == 'duplicateSelection') {
        return i18n('Duplicate the selected text. Place cursor after selection.');
    } else if (cmd == "encodeURISelection") {
        return i18n("Encode special chars in a single line selection, so the result text can be used as URI.");
    } else if (cmd == "decodeURISelection") {
        return i18n("Reverse action of URI encode.");
    } else if (cmd == "fsel") {
        return i18n("Select text forward from current cursor position to the first occurrence of the given argument after it (or the end of the current line by default).");
    } else if (cmd == "bsel") {
        return i18n("Select text backward from current cursor position to the last occurrence of the given argument before it (or the beginning of the current line by default).");
    }
}

/// helper code below:

function __toFunc(func, defaultArgName)
{
    if ( typeof(func) != "function" ) {
        try {
            func = eval("(" + func + ")");
        } catch(e) {}
        debug(func, typeof(func))
        if ( typeof(func) != "function" ) {
            try {
                // one more try to support e.g.:
                // map 'l+l'
                // or:
                // each 'lines.join("\n")'
                func = eval("(function(" + defaultArgName + "){ return " + func + ";})");
                debug(func, typeof(func))
            } catch(e) {}
            if ( typeof(func) != "function" ) {
                throw "parameter is not a valid JavaScript callback function: " + typeof(func);
            }
        }
    }
    return func;
}

function each(func)
{
    func = __toFunc(func, 'lines');

    var cursor = view.cursorPosition();
    var selection = view.selection();
    var hasSelection = selection.isValid();
    if (!hasSelection) {
        // use whole range
        selection = document.documentRange();
    } else {
        selection.start.column = 0;
        selection.end.column = document.lineLength(selection.end.line);
    }

    var text = document.text(selection);

    var lines = text.split("\n");
    oldLineCount = lines.length;
    oldCurrentLineLength = document.lineLength(cursor.line);
    lines = func(lines);
    if ( typeof(lines) == "object" ) {
      text = lines.join("\n");
      newLineCount = lines.length;
    } else if ( typeof(lines) == "string" ) {
      text = lines
      newLineCount = (lines.match('\n') || []).length;
    } else {
      throw "callback function for each has to return object or array of lines";
    }

    view.clearSelection();

    document.editBegin();
    if (!hasSelection) {
        document.setText(text);
    } else {
        document.removeText(selection);
        document.insertText(selection.start, text);
    }
    document.editEnd();
    if (newLineCount == oldLineCount) {
        if (document.lineLength(cursor.line) != oldCurrentLineLength) {
            cursor.column = document.lastColumn(cursor.line) + 1;
        }
        view.setCursorPosition(cursor);
    }
}

function filter(func)
{
    each(function(lines) { return lines.filter(__toFunc(func, 'line')); });
}

function map(func)
{
    each(function(lines) { return lines.map(__toFunc(func, 'line')); });
}

// kate: space-indent on; indent-width 4; replace-tabs on;
