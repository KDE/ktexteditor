var katescript = {
    "author": "Alan Prescott",
    "license": "LGPL-2.0-or-later",
    "revision": 3,
    "kate-version": "5.1",
    "functions": ["jumpIndentUp", "jumpIndentDown", "jumpToNextBlankLine", "jumpToPrevBlankLine"],
    "actions": [
        {   "function": "jumpIndentUp",
            "name": "Move Cursor to Previous Matching Indent",
            "shortcut": "Alt+Shift+Up",
            "category": "Navigation"
        },
        {   "function": "jumpIndentDown",
            "name": "Move Cursor to Next Matching Indent",
            "shortcut": "Alt+Shift+Down",
            "category": "Navigation"
        },
        {   "function": "jumpToNextBlankLine",
            "name": "Move Cursor to Next Blank Line",
            "shortcut": "",
            "category": "Navigation"
        },
        {   "function": "jumpToPrevBlankLine",
            "name": "Move Cursor to Previous Blank Line",
            "shortcut": "",
            "category": "Navigation"
        }
    ]
}; // kate-script-header, must be at the start of the file without comments, pure json

/**
 * Move cursor to next/previous line with an equal indent - especially useful
 * for moving around blocks of Python code.
 *
 * A first attempt at a kate script created by modifying the jump.js script
 * written by Dzikri Aziz.
 * I've been looking for this feature in an editor ever since I came across it
 * in an old MS-DOS editor called Edwin.
 */

// required katepart js libraries
require ("range.js");

function jumpIndentDown() {
  return _jumpIndent();
}

function jumpIndentUp() {
  return _jumpIndent( true );
}

function jumpToNextBlankLine() {
  var iniLine = view.cursorPosition().line;
  var iniCol = view.cursorPosition().column;
  var lines  = document.lines();
  if (iniLine < lines) {
    iniLine++;
    while (iniLine < lines && document.lineLength(iniLine) != 0) {
      iniLine++;
    }
    view.setCursorPosition(iniLine, iniCol);
  }
}

function jumpToPrevBlankLine() {
  var iniLine = view.cursorPosition().line;
  var iniCol = view.cursorPosition().column;
  var lines  = document.lines();
  if (iniLine > 0) {
    iniLine--;
    while (iniLine >= 0 && document.lineLength(iniLine) != 0) {
      iniLine--;
    }
    view.setCursorPosition(iniLine, iniCol);
  }
}

function help( cmd ) {
  if (cmd == 'jumpIndentUp') {
    return i18n("Move cursor to previous matching indent");
  }
  else if (cmd == 'jumpIndentDown') {
    return i18n("Move cursor to next matching indent");
  } else if (cmd == 'jumpToNextBlankLine') {
    return i18n("Move cursor to next blank line");
  } else if (cmd == 'jumpToPrevBlankLine') {
    return i18n("Move cursor to previous blank line");
  }
}

function _jumpIndent( up ) {
  var iniLine = view.cursorPosition().line,
      iniCol = view.cursorPosition().column,
      lines  = document.lines(),
      indent, target;

  indent = document.firstColumn(iniLine);
  target = iniLine;

  if ( up === true ) {
    if ( target > 0 ) {
      target--;
      while ( target > 0 && document.firstColumn(target) != indent ) {
        target--;
      }
    }
  }

  else {
    if ( target < lines ) {
      target++;
      while ( target < lines && document.firstColumn(target) != indent ) {
        target++;
      }
    }
  }

  view.setCursorPosition(target, iniCol);
}
