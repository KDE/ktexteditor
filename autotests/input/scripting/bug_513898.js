/*
 * test setVirtualCursorPosition
 * see bug 513898
 */
const testSetVirtualCursorPosition = function() {
    // edit first line
    view.setVirtualCursorPosition(0, 3);
    document.insertText(view.cursorPosition(), "c");
    view.setVirtualCursorPosition(0, 4);
    document.insertText(view.cursorPosition(), "d");

    // edit second line
    view.setVirtualCursorPosition(1, 2);
    document.insertText(view.cursorPosition(), "e");

    // move cursor to the end
    view.setVirtualCursorPosition(1, 5);
};

testCase('bug_513898', () => {
    config({
        syntax: 'None',
        selection: '',
        blockSelection: false,
    });
    cmd(testSetVirtualCursorPosition, "a\tb\na\tb", "ac\tdb\nae\tb");
});
