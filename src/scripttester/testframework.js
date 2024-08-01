/*
    SPDX-FileCopyrightText: 2024 Jonathan Poelen <jonathan.poelen@gmail.com>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

/// Value used in blockSelection configuration to run the test first with block
/// selection disabled, then, if the test does not fail, with block selection enabled.
export const DUAL_MODE = 2;

/// Same as DUAL_MODE, but the 2 modes are always executed.
export const ALWAYS_DUAL_MODE = 3;

/// Special value for \c expectedOutput from \c cmd() which indicates that
/// the expected output is the same as the input value.
export const EXPECTED_OUTPUT_AS_INPUT = Symbol('EXPECTED_OUTPUT_AS_INPUT');


let clipboardHistory = [];

function setClipboardHistory(newHistory) {
    clipboardHistory = newHistory.slice(0, 10);
}

function clearClipboardHistory(newHistory) {
    clipboardHistory = [];
}

// editor object of KTextEditor
export const editor = {
    clipboardText: function() {
        return clipboardHistory[0] || '';
    },

    clipboardHistory: function() {
        return clipboardHistory;
    },

    setClipboardText: function(text) {
        if (clipboardHistory.length === 10) {
            clipboardHistory.shift();
        }
        clipboardHistory.unshift(text);
    },
};

/**
 * Convert a value to a String
 * @param value Any: value to convert
 * @param codeFragments[out] Array[String]: array containing the strings representing the value
 * @param ancestors used for cyclic reference detection
 * @param mustBeStable Boolean: indicates whether the result should be stable.
 *                              For example, by sorting object keys.
 * @return undefined
 *
 * This function also has 4 members for storing conversion functions:
 *  - \c .registerClassName(name, addString) when `value.constructor.name == name`
 *  - \c .registerClass(type, addString) when `value instanceof type`
 *  - \c .registerSymbol(symbol, addString) when `value[symbol]`
 *  - \c .register(addString) when `addString(...) == true`
 * The parameters of \c addString are the same as those of \c addStringCodeFragments.
 */
export const addStringCodeFragments = (function(){
    const replacement = function(c) {
        return c === '\n' ? '\\n'
             : c === '\t' ? '\\t'
             : '\\' + c
    };

    // format a string as a literal string
    const pushJSString = function(codeFragments, str) {
        // when no quote or single and double quote, prefer single quote
        if (str.indexOf("'") === -1 || str.indexOf('"') !== -1) {
            codeFragments.push("'");
            codeFragments.push(str.replace(/[\n\t\'\\]/g, replacement));
            codeFragments.push("'");
        }
        else {
            codeFragments.push('"');
            codeFragments.push(str.replace(/[\n\t\"\\]/g, replacement));
            codeFragments.push('"');
        }
    };

    const addStringCodeFragmentByName = {};
    const addStringCodeFragmentByBehavior = [];

    const addStringCodeFragments = function(value, codeFragments, ancestors, mustBeStable) {
        const t = typeof value;
        // String
        if (t === 'string')
            pushJSString(codeFragments, value);
        // undefined
        else if (value === undefined)
            codeFragments.push('undefined');
        // null
        else if (value === null)
            codeFragments.push('null');
        // Function
        else if (t === 'function')
            // .toString() is always `function funcname() { [native code] }`.
            // Ignore body function and add number of parameter
            codeFragments.push(value.name
                ? `function ${value.name}(${value.length} params)`
                : `function(${value.length} params)`
            );
        // Number, Boolean, Symbol, BigInt (not implemented in QJSEngine)
        else if (t === 'symbol')
            // force toString() so as not to have an error with Symbol type
            codeFragments.push(value.toString());
        else if (t !== 'object')
            codeFragments.push(value);
        // circular call
        else if (ancestors.includes(value))
            codeFragments.push("[Circular]");
        else {
            // convert object if registered
            const name = value.constructor?.name;
            const addString = addStringCodeFragmentByName[name];
            if (addString) {
                addString(value, codeFragments, ancestors, mustBeStable);
                return;
            }
            for (const addString of addStringCodeFragmentByBehavior) {
                if (addString(value, codeFragments, ancestors, mustBeStable)) {
                    return;
                }
            }

            // assume array or array like
            if (value[Symbol.iterator]) {
                ancestors.push(value);

                const isArray = Array.isArray(value);
                if (isArray)
                    codeFragments.push('[');
                else {
                    const name = value.constructor.name;
                    codeFragments.push('new ');
                    codeFragments.push(value.constructor.name);
                    codeFragments.push('([');
                }

                const codeFragments2 = [];
                const codeFragmentsLen = codeFragments.length;
                for (const o of value) {
                    const subCodeFragment = [];
                    addStringCodeFragments(o, subCodeFragment, ancestors, mustBeStable);
                    codeFragments2.push(subCodeFragment.join(''));
                }
                if (mustBeStable && !isArray) {
                    codeFragments2.sort();
                }
                codeFragments.push(codeFragments2.join(', '));

                codeFragments.push(isArray ? ']' : '])');

                ancestors.pop();
            }
            // Error
            else if (value instanceof Error) {
                codeFragments.push(`${value.name}: ${value.message}`);
            }
            // RegExp
            else if (value instanceof RegExp) {
                codeFragments.push(value);
            }
            // other Object
            else {
                ancestors.push(value);

                codeFragments.push('{');
                const codeFragmentsLen = codeFragments.length;
                const keys = Object.keys(value);
                for (const key of mustBeStable ? keys.sort() : keys) {
                    if (/^[a-zA-Z$][\w\d$]*/.test(key))
                        codeFragments.push(key);
                    else
                        pushJSString(codeFragments, key);
                    codeFragments.push(': ');
                    addStringCodeFragments(value[key], codeFragments, ancestors, mustBeStable);
                    codeFragments.push(', ');
                }
                // remove last ', '
                if (codeFragmentsLen !== codeFragments.length)
                    codeFragments.pop();
                codeFragments.push('}');

                ancestors.pop();
            }
        }
    };

    // Register function for unknown object
    // @{
    addStringCodeFragments.registerClassName = function(name, addString) {
        addStringCodeFragmentByName[name] = addString;
    };

    addStringCodeFragments.registerClass = function(type, addString) {
        addStringCodeFragmentByBehavior.push((value, codeFragments, ancestors, mustBeStable) => {
            if (value instanceof type) {
                addString(value, codeFragments, ancestors, mustBeStable);
                return true;
            }
            return false;
        });
    };

    addStringCodeFragments.registerSymbol = function(symbol, addString) {
        addStringCodeFragmentByBehavior.push((value, codeFragments, ancestors, mustBeStable) => {
            if (value[symbol]) {
                addString(value, codeFragments, ancestors, mustBeStable);
                return true;
            }
            return false;
        });
    };

    addStringCodeFragments.register = function(addString) {
        addStringCodeFragmentByBehavior.push(addString);
    };
    // @}

    return addStringCodeFragments;
})();

/**
 * Uses \c addStringCodeFragments() on all parameters and returns them
 * separated with \p sep.
 * @param args Iterator
 * @param sep String
 * @return String
 */
export function formatArgs(args, sep) {
    const codeFragments = [];
    for (const arg of args) {
        if (typeof arg === 'string') {
            codeFragments.push(arg);
        }
        else {
            addStringCodeFragments(arg, codeFragments, []);
        }
        codeFragments.push(sep);
    }
    codeFragments.pop();
    return codeFragments.join('');
}

/**
 * Convert a value to a String.
 * @param value Any: value to convert
 * @return String
 */
export function toComparableString(value) {
    const codeFragments = [];
    addStringCodeFragments(value, codeFragments, [], true);
    return codeFragments.join('');
}

/**
 * Convert a function and its arguments to a String.
 * @param fn Function
 * @param args Array
 * @return String
 */
export function functionCallToString(fn, args) {
    // the Fonction object has a name, but may be empty
    const name = (typeof fn === 'function') ? (fn.name || '/*Unamed Function*/')
        : (fn === undefined) ? 'undefined'
        : (fn === null) ? 'null'
        : fn.toString();

    if (args.length === 0) {
        return name + '()';
    }

    const codeFragments = [name, '('];
    for (const arg of args) {
        addStringCodeFragments(arg, codeFragments, []);
        codeFragments.push(', ');
    }
    codeFragments[codeFragments.length - 1] = ')';
    return codeFragments.join('');
}

// As `===` but with `NaN === NaN` evaluated to true
// https://developer.mozilla.org/en-US/docs/Web/JavaScript/Equality_comparisons_and_sameness#comparing_equality_methods
function sameValueZero(x, y) {
  if (typeof x === "number" && typeof y === "number") {
    // x and y are equal (may be -0 and 0) or they are both NaN
    return x === y || (x !== x && y !== y);
  }
  return x === y;
}

/**
 * Operators available for \c TestExpression.test.
 * The key corresponds to the name used for the variable \c op of \c TestExpression.test.
 * The value is a comparison function of the form:
 * (expected: Any) -> {
 *          \c error: Boolean: indicates that the operator is expecting an exception.
 *          \c opname: name of the operator displayed, otherwise the key name is used
 *          \c expr: represents the test expression. If a sub-string '{input}' exists,
 *                   it will be replaced by the command.
 *          \c comp: (result: Any, expected: Any, exception: Any) -> Boolean | {
 *              \c ok: Boolean: indicates whether the comparison is a success or a failure
 *              \c result: represents the result of the command that will be displayed
 *              \c expected: represents the expected result that will be displayed
 *          }
 *      }
 *      All properties expect \c comp are optional
 */
export const operators = {
    /// Comparison on values converted to String.
    eqv: (_) => ({
        comp: (result, expected, exception) => {
            result = toComparableString(result);
            expected = toComparableString(expected);
            return {result, expected, ok: !exception && result === expected};
        },
    }),

    /// Double not ; equivalent to `Boolean(expr)`.
    '!!': (expected) => expected
        ? {
            expr: '!!({input})',
            comp: (result, expected, exception) => !exception && !!result,
        } : {
            expr: '!({input})',
            comp: (result, expected, exception) => !exception && !result,
        },

    /// Javascript operators with `===` and `!==` treating \c NaN as equivalent.
    /// (https://developer.mozilla.org/en-US/docs/Web/JavaScript/Equality_comparisons_and_sameness#same-value-zero_equality)
    /// @{
    '===': (_) => ({
        comp: (result, expected, exception) => !exception && sameValueZero(result, expected)
    }),
    '==': (_) => ({
        comp: (result, expected, exception) => !exception && result == expected
    }),
    '!==': (_) => ({
        comp: (result, expected, exception) => !exception && !sameValueZero(result, expected)
    }),
    '!=': (_) => ({
        comp: (result, expected, exception) => !exception && result != expected
    }),
    '<': (_) => ({
        comp: (result, expected, exception) => !exception && result < expected
    }),
    '>': (_) => ({
        comp: (result, expected, exception) => !exception && result > expected
    }),
    '<=': (_) => ({
        comp: (result, expected, exception) => !exception && result <= expected
    }),
    '>=': (_) => ({
        comp: (result, expected, exception) => !exception && result >= expected
    }),

    'instanceof': (_) => ({
        comp: (result, expected, exception) => !exception && result instanceof expected
    }),
    'in': (_) => ({
        comp: (result, expected, exception) => {
            try {
                return !exception && result in expected;
            }
            catch (_) {
                return false;
            }
        },
    }),
    /// @}

    /// typeof operator.
    'is type': (_) => ({
        comp: (result, expected, exception) => !exception && typeof result === expected
    }),

    /// Object.is
    'is': (_) => ({
        expr: 'Object.is({input}, {expected})',
        comp: (result, expected, exception) => !exception && Object.is(result, expected),
    }),

    /// Check that the \c exception matches \c expected.
    /// If \p exception is an \c Error type, then \p result must be an \c Error
    /// type with the identical \c name and \c message properties. Otherwise \c === is used.
    error: (_) => ({
        error: true,
        expr: 'error of {input} === {expected}',
        comp: (result, expected, exception) => {
            const isError = expected instanceof Error;
            return exception && isError === (exception instanceof Error)
                && (isError
                    ? exception.name === expected.name && exception.message === expected.message
                    : exception === expected)
        },
    }),
    /// Check that the \c exception message matches \c expected.
    /// If \c exception is an \c Error type, \c exception.message is used.
    /// Otherwise `exception.toString()` used.
    errorMsg: (_) => ({
        error: true,
        expr: 'error message of {input} === {expected}',
        comp: (result, expected, exception) =>
            exception && (exception instanceof Error
                ? exception.message === expected
                : exception.toString() === expected),
    }),
    /// Check that the \c exception type matches \c expected.
    /// If \c exception is an \c Error type, \c exception.name is used.
    /// Otherwise `typeof exception` is used.
    errorType: (_) => ({
        error: true,
        expr: 'error type of {input} === {expected}',
        comp: (result, expected, exception) =>
            exception && (exception instanceof Error
                ? exception.name === expected
                : typeof exception === expected)
    }),
    /// Check that an exception is thrown.
    hasError: (expected) => ({
        error: expected,
        expr: expected ? 'check error for {input}' : 'check not error for {input}',
        comp: (result, expected, exception) => ({
            expected: `has error = ${expected}`,
            ok: !exception === !expected,
        }),
    }),

    /// Equivalent to `expected[0] <= result && result < expected[1]`.
    between: (_) => ({
        expr: '{expected[0]} <= {input} < {expected[1]}',
        comp: (result, expected, exception) =>
            !exception && expected[0] <= result && result < expected[1]
    }),

    /// Equivalent to `expected[0] <= result && result <= expected[1]`.
    inclusiveBetween: (_) => ({
        expr: '{expected[0]} <= {input} <= {expected[1]}',
        comp: (result, expected, exception) =>
            !exception && expected[0] <= result && result <= expected[1]
    }),

    /// Equivalent to `expected in result`.
    key: (_) => ({
        expr: '{expected} in {input}',
        comp: (result, expected, exception) => {
            try {
                return !exception && expected in result;
            } catch (_) {
                return false;
            }
        },
    }),
};

export const BREAK_CASE_ERROR = Symbol('BREAK_CASE_ERROR');
export const STOP_CASE_ERROR = Symbol('STOP_CASE_ERROR');

/**
 * @param internal KTextEditorTester::TestFramework
 * @param op Function | String: key of \c operators or \c Function with
 *                              optional \p opname property. See the description
 *                              of \c comp of \c operators for the function signature.
 * @return Object: see \c operators
 */
function getComparatorInfo(internal, op, expected) {
    if (typeof op === 'function') {
        return {comp: op, opname: op.name};
    }

    const mkcomp = operators[op];
    if (!mkcomp) {
        internal.incrementError();
        const names = Object.keys(operators).join(', ');
        throw new ReferenceError(`Unknown operator '${op}', expected: ${names} or Function`);
    }
    return mkcomp(expected);
}

/**
 * transforms \p command into 2 functions: the first to execute the program,
 * the second to retrieve it as a string.
 * @param internal KTextEditorTester::TestFramework
 * @param command Function | String | Array[Function | String, ...arguments]
 * @return Array[program: () -> Any, programToString: () -> String]
 */
function toProgram(internal, command) {
    if (typeof command === 'function') {
        return [
            command,
            () => command.name ? command.name + '()' : '/*Unamed Function*/',
        ];
    }

    if (typeof command === 'string') {
        return [
            () => internal.evaluate(command),
            () => command,
        ];
    }

    if (Array.isArray(command)) {
        const args = command.slice(1);
        let fn = command[0];

        /**
         * Transform string to function.
         * Useful with the form `obj.func` which requires that the parameter
         * be `obj.func.bind(obj)` or `(...arg) => obj.func(...arg)` so as not
         * to lose test which makes it impossible to display the function used
         * in the test message.
         */
        if (typeof fn === 'string') {
            fn = internal.evaluate(`(function(){ return ${fn}(...arguments); })`)
        }

        return [
            () => fn.apply(undefined, args),
            () => functionCallToString(command[0], args),
        ];
    }

    internal.incrementError();
    throw new TypeError("Invalid command type. Expected Function, String or Array");
}

/**
 * Execute \p program.
 * @param program Function() -> Any
 * @return Array[result: Any, exception: Any]
 */
function executeProgram(program) {
    const savedClipboardHistory = clipboardHistory.slice();
    let r;
    try {
        r = [program(), undefined];
    } catch (e) {
        r = [undefined, e];
    }
    clipboardHistory = savedClipboardHistory;
    return r;
}

/**
 * Return the program expression
 * @param info undefined | {
 *          \c expr: Optional[String]
 *          \c program: Optional[String]
 *      }
 * @param compInfo undefined | {
 *          \c expr: Optional[String]
 *          \c opname: Optional[String]
 *      }
 * @param opname String
 * @param getProgram () -> String
 * @param useExpectedResultAsPlaceholder Boolean: replace '{expected}' with '{expectedResult}'
 * @return String
 */
function getInputExpression(info, compInfo, opname, getProgram, useExpectedResultAsPlaceholder) {
    const program = info?.program || getProgram();
    let expr = info?.expr || compInfo?.expr;
    if (expr) {
        if (useExpectedResultAsPlaceholder) {
            expr = expr.replace('{expected}', '{expectedResult}')
        }
        return expr.replace('{input}', program);
    }

    const placeholder = useExpectedResultAsPlaceholder ? 'expectedResult' : 'expected';
    return `${program} ${compInfo?.opname || opname} {${placeholder}}`;
}

/**
 * @param msgOrInfo String | {\c msg: String}
 * @return String
 */
function getMessage(msgOrInfo) {
    return typeof msgOrInfo === 'string' ? msgOrInfo : msgOrInfo?.msg;
}

const OUTPUT_IS_OK = (1 << 0);
const CONTAINS_RESULT_OR_ERROR = 1 << 1;
const EXPECTED_ERROR_BUT_NO_ERROR = (1 << 2) | CONTAINS_RESULT_OR_ERROR;
const EXPECTED_NO_ERROR_BUT_ERROR = (1 << 3) | CONTAINS_RESULT_OR_ERROR;
const IS_RESULT_NOT_ERROR = (1 << 4) | CONTAINS_RESULT_OR_ERROR;
const IS_ERROR_NOT_RESULT = CONTAINS_RESULT_OR_ERROR;
const SAME_RESULT_OR_ERROR = (1 << 5) | CONTAINS_RESULT_OR_ERROR;
const IGNORE_INPUT_OUTPUT = (1 << 6);
const RESULT_OK = (1 << 5);

/**
 * Return the program expression
 * @param cmpRes undefined | {expected: Optional[String], result: Optional[String], error: Boolean}:
 * @param result Any
 * @param expected Any
 * @param exception Any
 * @return {
 *      \c displableExpected: String
 *      \c displayableResult: String
 *      \c opt: Number
 * }
 */
function comparatorResultToPrintableInfo(cmpRes, result, expected, exception) {
    return {
        displableExpected: cmpRes?.expected || toComparableString(expected),
        displayableResult: cmpRes?.result || exception || toComparableString(result),
        opt: cmpRes?.error
            ? (exception ? IS_ERROR_NOT_RESULT : EXPECTED_ERROR_BUT_NO_ERROR)
            : (exception ? EXPECTED_NO_ERROR_BUT_ERROR : IS_RESULT_NOT_ERROR),
    };
}

/**
 * Check \p command result and display the result if it fails.
 * @param internal KTextEditorTester::TestFramework
 * @param name String: test name
 * @param program value for \c executeProgram()
 * @param getProgram Function() -> String
 * @param compInfo undefined | Object: see operator \c getComparatorInfo()
 * @param op undefined | String: value for \c getInputExpression()
 * @param expectedResult: Any
 * @param msgOrInfo value for \c getInputExpression() and \c getMessage()
 * @return Boolean: \c true when no errors, otherwise \c false
 */
function checkCommand(internal, name, program, getProgram, compInfo, op, expectedResult, msgOrInfo) {
    const displayMode = internal.startTest();
    if (displayMode & 2) {
        internal.writeTestExpression(
            name, 'cmd', 3, compInfo
                ? getInputExpression(msgOrInfo, compInfo, op, getProgram, true)
                : (msgOrInfo?.program || getProgram())
        );
    }

    const [result, exception] = executeProgram(program);
    const isSame = internal.checkOutput();
    const compRes = compInfo
        ? compInfo.comp(result, expectedResult, exception)
        : !exception;
    const expectedResultIsOk = (compRes === true || compRes?.ok);

    /*
     * Displays a message if there is an error
     */
    const ok = internal.incrementCounter(isSame && expectedResultIsOk);
    if (!ok || displayMode & 1) {
        const inputExpr = program || getProgram();
        const printableInfo = compInfo
            ? comparatorResultToPrintableInfo(compRes, result, expectedResult, exception)
            : {};
        const opt = ok
            ? OUTPUT_IS_OK | (compInfo ? RESULT_OK : 0)
            : printableInfo.opt
            | (isSame ? OUTPUT_IS_OK : 0)
            | (compInfo
                ? (expectedResultIsOk ? SAME_RESULT_OR_ERROR : 0)
                : (exception ? EXPECTED_NO_ERROR_BUT_ERROR : 0)
            );
        internal.writeTestResult(
            name, 'cmd', 3, compInfo
                ? getInputExpression(msgOrInfo, compInfo, op, getProgram, true)
                : (msgOrInfo?.program || getProgram()),
            getMessage(msgOrInfo), exception,
            printableInfo.displayableResult,
            printableInfo.displableExpected,
            opt,
        );
    }

    internal.endTest(ok, true);
    return ok;
}

/**
 * Set the expected output then execute and check \p command.
 * The command can be executed 2 times if the \c blockSelection configuration contains a \c *DUAL_MODE value.
 * @param test TestCase | TestCaseWithInput
 * @param command value for \c toProgram()
 * @param expectedOutput String | EXPECTED_OUTPUT_AS_INPUT
 * @param msgOrExpectedResultInfo undefined | String | {
 *          \c msg: undefined | String: a message displayed in case of error
 *          \c program undefined | String: string representing the command to be displayed
 *                                         rather than the one automatically calculated
 *          \c expr: represents the test expression. If a sub-string '{input}' exists,
 *                   it will be replaced by the command.
 *          \c op String | Function: see \c getComparatorInfo(). Default is '==='
 *          \c expected: Any
 *          \c error: Boolean: check if an exception is thrown
 *                  | String: check exception message
 *                  | Any: check exception message and class name
 *                  Overwrite property op
 *          \c blockSelection: Boolean
 *      }
 */
function testCommand(test, command, expectedOutput, msgOrExpectedResultInfo) {
    let op;
    let compInfo;
    let expectedResult;
    let hasBlockSelectionConfig = false;
    if (typeof msgOrExpectedResultInfo === 'object') {
        /*
         * Extract result operator comparison
         */
        const hasExpected = ('expected' in msgOrExpectedResultInfo);
        const hasError = ('error' in msgOrExpectedResultInfo);
        if (hasExpected || hasError) {
            if (hasExpected && hasError) {
                test.internal.incrementError();
                throw Error('expected and error property are both defined');
            }

            if (hasExpected) {
                expectedResult = msgOrExpectedResultInfo.expected;
                op = msgOrExpectedResultInfo.op || '===';
            }
            else /*if (hasError)*/ {
                expectedResult = msgOrExpectedResultInfo.error;
                op = msgOrExpectedResultInfo.op;
                if (!opname) {
                    const t = typeof expectedResult;
                    op = (t === 'string') ? 'errorMsg'
                       : (t === 'boolean') ? 'hasError'
                       : 'error';
                }
            }
            compInfo = getComparatorInfo(test.internal, op, expectedResult);
        }

        hasBlockSelectionConfig = 'blockSelection' in msgOrExpectedResultInfo;
    }

    const blockSelection = test.blockSelection;
    const blockSelectionOutput = hasBlockSelectionConfig
        ? msgOrExpectedResultInfo.blockSelection
        : (blockSelection == 1);

    /*
     * Init exepected output
     */
    if (expectedOutput === EXPECTED_OUTPUT_AS_INPUT) {
        test.internal.copyInputToExpectedOutput(blockSelectionOutput);
    }
    else {
        test.internal.setExpectedOutput(expectedOutput, blockSelectionOutput);
    }

    /*
     * Run and check command
     */
    const [program, getProgram] = toProgram(test.internal, command);

    let ok = true;
    // when dual mode
    if (blockSelection === DUAL_MODE || blockSelection === ALWAYS_DUAL_MODE) {
        ok = checkCommand(test.internal, test.name, program, getProgram, compInfo, op, expectedResult, msgOrExpectedResultInfo);
        if (!ok && blockSelection === DUAL_MODE) {
            test._checkBreakOnError();
            test.internal.writeDualModeAborted(test.name, 2);
            return;
        }
        if (!test.internal.reuseInputWithBlockSelection()) {
            return;
        }
        if (!hasBlockSelectionConfig) {
            test.internal.reuseExpectedOutput(true);
        }
    }
    if (!(checkCommand(test.internal, test.name, program, getProgram, compInfo, op, expectedResult, msgOrExpectedResultInfo) && ok)) {
        test._checkBreakOnError();
    }
}


const defaultConfig = {
    syntax: "None",
    indentationMode: "none",
    blockSelection: DUAL_MODE,
    indentationWidth: 4,
    tabWidth: 4,
    replaceTabs: false,
    overrideMode: false,
};

// apply framework config (placeholder) and kate config
function setConfig(test, config) {
    test.internal.setConfig(config);
    if ('blockSelection' in config) {
        test.blockSelection = config.blockSelection;
    }
    if (config.clipboardHistory) {
        setClipboardHistory(config.clipboardHistory);
    }
}


/**
 * Class to test function results.
 * Tests are performed on the current state of the document and view.
 */
class TestExpression {
    constructor(name, breakOnError, internal, blockSelection) {
        this.name = name;
        this.internal = internal;
        this.breakOnError = breakOnError;
        this.blockSelection = blockSelection;
    }

    /**
     * @param config Object: {
     *          // kate config
     *          \c syntax: String,
     *          \c indentationMode: String,
     *          \c blockSelection: Boolean | DUAL_MODE | ALWAYS_DUAL_MODE,
     *          \c indentationWidth: Number,
     *          \c tabWidth: Number,
     *          \c replaceTabs: Boolean,
     *          \c overrideMode: Boolean,
     *          \c indentPastedTest: Boolean,
     *
     *          // editor state
     *          \c clipboardHistory: Array[String],
     *
     *          // Placeholders in input, output and expected output.
     *          // Only the first letter is used.
     *          // If the string is empty, the placeholder is deactivated.
     *          \c cursor: String,
     *          \c secondaryCursor: String,
     *          \c virtualText: String,
     *
     *          // Placeholders in input, output and expected output.
     *          // The first letter corresponds to selection opening, the second to closing.
     *          // If there is only one letter, opening and closing will have the same value.
     *          // If the string is empty, the placeholder is deactivated.
     *          \c selection: String,
     *          \c secondarySelection: String,
     *
     *          // Placeholders used only in display when normal versions are
     *          // disabled or there is a display conflict (same value to
     *          // represent the same thing)
     *          // Empty strings are replaced by the default value.
     *          \c cursor2: String,
     *          \c secondaryCursor2: String,
     *          \c virtualText2: String,
     *          \c selection2: String,
     *          \c secondarySelection2: String,
     *      }
     * @return Self
     */
    config(config) {
        setConfig(this, config);
        return this;
    }

    /**
     * Executes \p command and uses the \p op operator to compare it to \p expected.
     * @param command value for \c toProgram()
     * @param op String | Function: key of \c operators or \c Function with
     *                              optional \p opname property. See the description
     *                              of \c operators for the function signature.
     * @param msgOrInfo undefined | String | {
     *          \c msg: Optional[String]
     *          \c expr: Optional[String]
     *          \c program: Optional[String]
     *      }
     */
    test(command, op, expected, msgOrInfo) { return this._test(command, op, expected, msgOrInfo); }

    /// Shortcut for test() function.
    /// @{
    eqvTrue(command, msgOrInfo) { return this._test(command, '!!', true, msgOrInfo); }
    eqvFalse(command, msgOrInfo) { return this._test(command, '!!', false, msgOrInfo); }
    eqTrue(command, msgOrInfo) { return this._test(command, '===', true, msgOrInfo); }
    eqFalse(command, msgOrInfo) { return this._test(command, '===', false, msgOrInfo); }

    error(command, expected, msgOrInfo) { return this._test(command, 'error', expected, msgOrInfo); }
    errorMsg(command, expected, msgOrInfo) { return this._test(command, 'errorMsg', expected, msgOrInfo); }
    errorType(command, expected, msgOrInfo) { return this._test(command, 'errorType', expected, msgOrInfo); }
    hasError(command, msgOrInfo) { return this._test(command, 'hasError', true, msgOrInfo); }

    eqv(command, expected, msgOrInfo) { return this._test(command, 'eqv', expected, msgOrInfo); }
    is(command, expected, msgOrInfo) { return this._test(command, 'is', expected, msgOrInfo); }
    eq(command, expected, msgOrInfo) { return this._test(command, '===', expected, msgOrInfo); }
    ne(command, expected, msgOrInfo) { return this._test(command, '!=', expected, msgOrInfo); }
    lt(command, expected, msgOrInfo) { return this._test(command, '<', expected, msgOrInfo); }
    gt(command, expected, msgOrInfo) { return this._test(command, '>', expected, msgOrInfo); }
    le(command, expected, msgOrInfo) { return this._test(command, '<=', expected, msgOrInfo); }
    ge(command, expected, msgOrInfo) { return this._test(command, '>=', expected, msgOrInfo); }
    /// @}

    _test(command, op, expected, msgOrInfo) {
        const compInfo = getComparatorInfo(this.internal, op, expected);
        const [program, getProgram] = toProgram(this.internal, command);
        const displayMode = this.internal.startTest();
        if (displayMode & 2) {
            this.internal.writeTestExpression(
                this.name, 'test', 2, getInputExpression(msgOrInfo, compInfo, op, getProgram)
            );
        }

        const [result, exception] = executeProgram(program);
        const res = compInfo.comp(result, expected, exception);
        const ok = this.internal.incrementCounter(res === true || res?.ok);
        if (!ok || displayMode & 1) {
            const printableInfo = comparatorResultToPrintableInfo(res, result, expected, exception);
            this.internal.writeTestResult(
                this.name, 'test', 2,
                getInputExpression(msgOrInfo, compInfo, op, getProgram),
                getMessage(msgOrInfo), exception,
                printableInfo.displayableResult,
                printableInfo.displableExpected,
                IGNORE_INPUT_OUTPUT | (ok ? RESULT_OK : CONTAINS_RESULT_OR_ERROR | printableInfo.opt),
            );
        }

        this.internal.endTest(ok);

        if (!ok) {
            this._checkBreakOnError();
        }

        return this;
    }

    _checkBreakOnError() {
        if (this.internal.hasTooManyErrors()) {
            throw STOP_CASE_ERROR;
        }
        if (this.breakOnError) {
            this.internal.incrementBreakOnError();
            throw BREAK_CASE_ERROR;
        }
    }
}


/**
 * Class to test document text and function results.
 */
class TestCase extends TestExpression {
    constructor(name, breakOnError, internal, env, blockSelection) {
        super(name, breakOnError, internal, blockSelection);
        this.env = env;
    }

    /**
     * @param command Function | String | Array[Function | String, ...args]
     * @param input String
     * @param expectedOutput String | EXPECTED_OUTPUT_AS_INPUT
     * @param msgOrExpectedResultInfo undefined | String | {
     *          \c msg: undefined | String: a message displayed in case of error
     *          \c program undefined | String: string representing the command to be displayed
     *                                         rather than the one automatically calculated
     *          \c op String | Function: see \c getComparatorInfo(). Default is '==='
     *          \c expected: Any
     *          \c error: Boolean: check if an exception is thrown
     *                  | String: check exception message
     *                  | Any: check exception message and class name
     *                  Overwrite property op
     *          \c blockSelection: Boolean
     *      }
     * @return Self
     */
    cmd(command, input, expectedOutput, msgOrExpectedResultInfo) {
        this.internal.setInput(input, this.blockSelection == 1);
        testCommand(this, command, expectedOutput, msgOrExpectedResultInfo);
        return this;
    }
}

/**
 * Class to test document text and function results.
 * The \c input is reused each time \c cmd() is called.
 */
class TestCaseWithInput extends TestExpression {
    constructor(name, breakOnError, input, internal, env, blockSelection) {
        super(name, breakOnError, internal);
        this.env = env;
        this.input = input;
        this.reuseInput = false;
        this.blockSelection = blockSelection;
    }

    /**
     * @param input String
     * @return Self
     */
    setInput(input) {
        this.input = input;
        this.reuseInput = false;
        return this;
    }

    /**
     * @param command: Function | String | Array[Function | String, ...args]
     * @param expectedOutput String | EXPECTED_OUTPUT_AS_INPUT
     * @param msgOrExpectedResultInfo: see \c TestCase.cmd()
     * @return Self
     */
    cmd(command, expectedOutput, msgOrExpectedResultInfo) {
        this._initInput();
        testCommand(this, command, expectedOutput, msgOrExpectedResultInfo);
        return this;
    }

    _initInput() {
        if (this.reuseInput) {
            this.internal.reuseInput(this.blockSelection == 1);
        }
        else {
            this.internal.setInput(this.input, this.blockSelection == 1);
            this.reuseInput = true;
        }
    }
}

/**
 * Class to test document text and function results.
 * The \c expectedOutput text will be used as \c input the next time \c cmd() is called.
 */
class TestCaseChain extends TestCaseWithInput {
    _initInput() {
        if (this.input !== undefined) {
            this.internal.setInput(this.input, this.blockSelection == 1);
            this.input = undefined;
        } else {
            this.internal.moveExpectedOutputToInput(this.blockSelection == 1);
        }
    }
}


export class TestFramework {
    constructor(internal, env) {
        this.internal = internal;
        this.env = env;
        this._breakOnError = false;
        this.blockSelection = DUAL_MODE;
    }

    /**
     * See \c TestExpression.config()
     * @return Self
     */
    config(config) {
        setConfig(this, config);
        return this;
    }

    /**
     * @param on Boolean
     * @return Self
     */
    breakOnError(on) {
        this._breakOnError = on;
        return this;
    }

    /**
     * Starts a test set of type TestCase
     * @param name String | {name: String, breakOnError: Boolean}:
     *      name is used for filter test.
     * @param fn Function(TestCase, globalThis) with this as TestCase
     * @return Self
     */
    testCase(name, fn) {
        this._execTestCase(name, fn, TestCase);
        return this;
    }

    /**
     * Starts a test set of type TestCaseWithInput
     * @param name String | {name: String, breakOnError: Boolean}:
     *      name is used for filter test.
     * @param input String
     * @param fn Function(TestCaseWithInput, globalThis) with this as TestCaseWithInput
     * @return Self
     */
    testCaseWithInput(name, input, fn) {
        this._execTestCase(name, fn, TestCaseWithInput, input);
        return this;
    }

    /**
     * Starts a test set of type TestCaseChain
     * @param name String | {name: String, breakOnError: Boolean}:
     *      name is used for filter test.
     * @param input String
     * @param fn Function(TestCaseChain, globalThis) with this as TestCaseChain
     * @return Self
     */
    testCaseChain(name, input, fn) {
        this._execTestCase(name, fn, TestCaseChain, input);
        return this;
    }

    /**
     * Uses \c formatArgs(args, ' ') and displays with global \c debug() function.
     */
    debug(...args) {
        this.internal.debug(formatArgs(args, ' '));
    }

    /**
     * Uses \c formatArgs(args, ' ') and displays.
     */
    print(...args) {
        this.internal.print(formatArgs(args, ' '));
    }

    /**
     * Uses \c formatArgs(args, sep) and displays.
     */
    printSep(sep, ...args) {
        this.internal.print(formatArgs(args, sep));
    }

    /**
     * @param nameOrConfig String | {name: String, breakOnError: Boolean}
     * @param fn Function(TestCaseChain, globalThis) with \p Class
     * @param Class class object
     */
    _execTestCase(nameOrConfig, fn, Class, ...args) {
        let name, breakOnError;
        if (typeof nameOrConfig === 'string') {
            name = nameOrConfig;
            breakOnError = this._breakOnError;
        }
        else {
            name = nameOrConfig.name;
            breakOnError = ('breakOnError' in nameOrConfig)
                ? nameOrConfig.breakOnError
                : this._breakOnError;
        }

        if (!this.internal.startTestCase(name, 2)) {
            return;
        }

        this.internal.saveConfig();
        const testCase = new Class(name, breakOnError, ...args,
                                   this.internal, this.env, this.blockSelection);
        try {
            fn.call(testCase, testCase, this.env);
        }
        catch (e) {
            if (e !== BREAK_CASE_ERROR) {
                throw e;
            }
        }
        this.internal.restoreConfig();
    }
}

/**
 * Ensures that each method has `${name}.${funcName}` as its name property value.
 * @param name String: name of \p obj
 * @param obj Object: wrapped object
 * @return Object
 */
export function calleeWrapper(name, obj) {
    return new Proxy(obj, {
        get(target, prop) {
            const o = target[prop];
            if (typeof o === 'function') {
                const funcName = `${name}.${prop.toString()}`;
                // create a function with function.name = funcName and this preservation
                return {[funcName]: (...args) => target[prop](...args)}[funcName];
            }
            return target[propertyName];
        }
    });
}
