var katescript = {
    "author": "Gregor Petrin",
    "license": "BSD",
    "revision": 3,
    "kate-version": "5.1",
    "functions": ["emmetExpand", "emmetWrap", "emmetSelectTagPairInwards", "emmetSelectTagPairOutwards", "emmetMatchingPair", "emmetToggleComment", "emmetNext", "emmetPrev", "emmetSelectNext", "emmetSelectPrev", "emmetDelete", "emmetSplitJoinTab", "emmetEvaluateMathExpression", "emmetDecrementNumberBy1", "emmetDecrementNumberBy10", "emmetDecrementNumberBy01", "emmetIncrementNumberBy1", "emmetIncrementNumberBy10", "emmetIncrementNumberBy01"],
    "actions": [
        {   "function": "emmetExpand",
            "name": "Expand abbreviation",
            "category": "Emmet"
        },
        {   "function": "emmetWrap",
            "name": "Wrap with tag",
            "category": "Emmet",
            "interactive": 1
        },
        {   "function": "emmetMatchingPair",
            "name": "Move cursor to matching tag",
            "category": "Emmet"
        },
        {   "function": "emmetSelectTagPairInwards",
            "name": "Select HTML/XML tag contents inwards",
            "category": "Emmet"
        },
        {   "function": "emmetSelectTagPairOutwards",
            "name": "Select HTML/XML tag contents outwards",
            "category": "Emmet"
        },
        {   "function": "emmetToggleComment",
            "name": "Toggle comment",
            "category": "Emmet"
        },
        {   "function": "emmetNext",
            "name": "Go to next edit point",
            "category": "Emmet"
        },
        {   "function": "emmetPrev",
            "name": "Go to previous edit point",
            "category": "Emmet"
        },
        {   "function": "emmetSelectNext",
            "name": "Select next edit point",
            "category": "Emmet"
        },
        {   "function": "emmetSelectPrev",
            "name": "Select previous edit point",
            "category": "Emmet"
        },
        {   "function": "emmetDelete",
            "name": "Delete tag under cursor",
            "category": "Emmet"
        },
        {   "function": "emmetSplitJoinTab",
            "name": "Split or join a tag",
            "category": "Emmet"
        },
        {   "function": "emmetEvaluateMathExpression",
            "name": "Evaluate a simple math expression",
            "category": "Emmet"
        },
        {   "function": "emmetDecrementNumberBy1",
            "name": "Decrement number by 1",
            "category": "Emmet"
        },
        {   "function": "emmetDecrementNumberBy10",
            "name": "Decrement number by 10",
            "category": "Emmet"
        },
        {   "function": "emmetDecrementNumberBy01",
            "name": "Decrement number by 0.1",
            "category": "Emmet"
        },
        {   "function": "emmetIncrementNumberBy1",
            "name": "Increment number by 1",
            "category": "Emmet"
        },
        {   "function": "emmetIncrementNumberBy10",
            "name": "Increment number by 10",
            "category": "Emmet"
        },
        {   "function": "emmetIncrementNumberBy01",
            "name": "Increment number by 0.1",
            "category": "Emmet"
        }
    ]
}; // kate-script-header, must be at the start of the file without comments, pure json

// required katepart js libraries
require ("range.js");
require ("emmet/lib.js");
require ("emmet/editor_interface.js");

function help(cmd)
{
	cmds = {
		"emmetExpand" : i18n("Expands the abbreviation using Emmet expressions; see http://code.google.com/p/zen-coding/wiki/ZenHTMLSelectorsEn"),
		"emmetWrap" : i18n("Wraps the selected text in XML tags constructed from the provided Emmet expression (defaults to div)."),
		"emmetMatchingPair" : i18n("Moves the caret to the current tag's pair"),
		"emmetSelectTagPairInwards" : i18n("Select contents of HTML/XML tag, moving inward on continuous invocations"),
		"emmetSelectTagPairOutwards" : i18n("Select contents of HTML/XML tag, moving outwards on continuous invocations"),
		"emmetNext" : i18n("Move to the next edit point (tag or empty attribute)."),
		"emmetPrev" : i18n("Move to the previous edit point (tag or empty attribute)."),
		"emmetSelectNext" : i18n("Select next edit point (tag or empty attribute)."),
		"emmetSelectPrev" : i18n("Select previous edit point (tag or empty attribute)."),
		"emmetToggleComment" : i18n("Toggle comment of current tag or CSS selector"),
		"emmetDelete" : i18n("Deletes tag under cursor"),
		"emmetSplitJoinTab" : i18n("Splits or joins a tag"),
		"emmetEvaluateMathExpression" : i18n("Evaluates a simple math expression"),
		"emmetDecrementNumberBy1" : i18n("Decrement number under cursor by 1"),
		"emmetDecrementNumberBy10" : i18n("Decrement number under cursor by 10"),
		"emmetDecrementNumberBy01" : i18n("Decrement number under cursor by 0.1"),
		"emmetIncrementNumberBy1" : i18n("Increment number under cursor by 1"),
		"emmetIncrementNumberBy10" : i18n("Increment number under cursor by 10"),
		"emmetIncrementNumberBy01" : i18n("Increment number under cursor by 0.1")
	}
	return cmds[cmd];
}

//Returns the kate editor interface
function getInterface() {
	var kateInterface = new zen_editor(document, view);
	return kateInterface;
}

function emmetExpand() {
	emmet.require('actions').run('expand_abbreviation', getInterface());
}   

function emmetWrap(par) {
	emmet.require('actions').run('wrap_with_abbreviation', getInterface(), par || 'div');
}

function emmetMatchingPair() {
	emmet.require('actions').run('matching_pair', getInterface());
}

function emmetSelectTagPairInwards() {
	emmet.require('actions').run('match_pair', getInterface());
}

function emmetSelectTagPairOutwards() {
	emmet.require('actions').run('match_pair', getInterface());
}

function emmetNext() {
	emmet.require('actions').run('next_edit_point', getInterface());
}

function emmetPrev() {
	emmet.require('actions').run('prev_edit_point', getInterface());
}

function emmetSelectNext() {
	emmet.require('actions').run('select_next_item', getInterface());
}

function emmetSelectPrev() {
	emmet.require('actions').run('select_previous_item', getInterface());
}

function emmetToggleComment() {
	emmet.require('actions').run('toggle_comment', getInterface());
}

function emmetDelete() {
	emmet.require('actions').run('remove_tag', getInterface());
}

function emmetSplitJoinTab() {
	emmet.require('actions').run('split_join_tag', getInterface());
}

function emmetEvaluateMathExpression() {
	emmet.require('actions').run('evaluate_math_expression', getInterface());
}

function emmetIncrementNumberBy1() {
	emmet.require('actions').run('increment_number_by_1', getInterface());
}

function emmetIncrementNumberBy10() {
	emmet.require('actions').run('increment_number_by_10', getInterface());
}

function emmetIncrementNumberBy01() {
	emmet.require('actions').run('increment_number_by_01', getInterface());
}

function emmetDecrementNumberBy1() {
	emmet.require('actions').run('decrement_number_by_1', getInterface());
}

function emmetDecrementNumberBy10() {
	emmet.require('actions').run('decrement_number_by_10', getInterface());
}

function emmetDecrementNumberBy01() {
	emmet.require('actions').run('decrement_number_by_01', getInterface());
}
