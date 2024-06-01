# Kate Script Tester

`ktexteditor-script-tester6` is a program used to run unit tests on javascript scripts built into `KTextEditor` or written by users.

## Getting Started

First, we need to load a script via `loadScript`.
The script will be evaluated in the context of the global object.

```js
loadScript(":/ktexteditor/script/commands/utils.js");
```

The path above refers to scripts integrated directly into `KTextEditor`.
In particular, it contains the `moveLinesDown` function, which will be used in what follows.

Next, we need to write a test case using one of the 3 available functions:

- `testCase`
- `testCaseChain`
- `testCaseWithInput`

Each of these functions takes a name as first parameter and a function as last parameter.
The name is displayed in case of failure, and cli options allow you to filter what will or won't be executed.
The last parameter function contains our tests on `moveLinesDown`.

```js
testCase("moveLinesDown test", (t) => {
    //       command           input           expected
    t.cmd(moveLinesDown, "[1]\n2\n3\n4\n", "2\n[1]\n3\n4\n");
    t.cmd(moveLinesDown, "2\n[1]\n3\n4\n", "2\n3\n[1]\n4\n");
    t.cmd(moveLinesDown, "2\n3\n[1]\n4\n", "2\n3\n4\n[1]\n");
});
```

`t.cmd()` executes a *command* on a document whose state is represented by *input* and checks that the final document is in the state of *expected*.

The document state is the text and the position of cursors and selections represented by characters called *placeholder*. By default, `|` represents a cursor and `[...]` a selection.

If no cursor position is given, it will automatically be set after the selection or at the end of the document.

```js
t.cmd(moveLinesDown, "[1]\n2\n3\n4\n", "2\n[1]\n3\n4\n");
// equivalent to
t.cmd(moveLinesDown, "[1]|\n2\n3\n4\n", "2\n[1]|\n3\n4\n");
```

Running the test gives us:

```
Success: 6  Failure: 0  Duration: 10ms
```

The number of successes is double that expected. This is normal, as by default `cmd()` executes tests twice: once with block selection disabled, a second time with block selection enabled.

With a more verbose display (`ktexteditor-script-tester6 -p test.js`) we can see the following result:

```
test.js:4: moveLinesDown test: cmd `moveLinesDown()` [blockSelection=0] Ok
test.js:4: moveLinesDown test: cmd `moveLinesDown()` [blockSelection=1] Ok
test.js:5: moveLinesDown test: cmd `moveLinesDown()` [blockSelection=0] Ok
test.js:5: moveLinesDown test: cmd `moveLinesDown()` [blockSelection=1] Ok
test.js:6: moveLinesDown test: cmd `moveLinesDown()` [blockSelection=0] Ok
test.js:6: moveLinesDown test: cmd `moveLinesDown()` [blockSelection=1] Ok

Success: 6  Failure: 0  Duration: 11ms
```

The `-B` option disables tests on block selection and `--dual` offers finer control.
It can also be disabled directly in the test file via `config()`:

```js
// disable for all tests
testFramework.config({blockSelection: false});

testCase("moveLinesDown test", (t) => {
    // disable for current test case
    t.config({blockSelection: false});
    // ...
});
```

If we look closely at our test, the expected result corresponds to the next test input. The whole thing can be simplified using `testCaseChain`:

```js
//                                     first input
testCaseChain("moveLinesDown test", "[1]\n2\n3\n4\n", (t) => {
    //       command     expected then next input
    t.cmd(moveLinesDown, "2\n[1]\n3\n4\n");
    t.cmd(moveLinesDown, "2\n3\n[1]\n4\n");
    t.cmd(moveLinesDown, "2\n3\n4\n[1]\n");
});
```

By activating verbose mode (`ktexteditor-script-tester6 -B -V test.js`), we can see that the input to a test takes the expected value from the previous test:

```
test.js:4: moveLinesDown test: OK
cmd `moveLinesDown()` [blockSelection=0]:
  input:    [1]|\n2\n3\n4\n
  output:   2\n[1]|\n3\n4\n

test.js:5: moveLinesDown test: OK
cmd `moveLinesDown()` [blockSelection=0]:
  input:    2\n[1]|\n3\n4\n
  output:   2\n3\n[1]|\n4\n

test.js:6: moveLinesDown test: OK
cmd `moveLinesDown()` [blockSelection=0]:
  input:    2\n3\n[1]|\n4\n
  output:   2\n3\n4\n[1]|\n

Success: 3  Failure: 0  Duration: 8ms
```

Note that a cursor is automatically added to the input.

The default display for block selection is different (controlled by `-F`), here's an extract:

```
file1.js:4: moveLinesDown test: OK
cmd `moveLinesDown()` [blockSelection=1]:
  input:    [1]|↵
            2↵
            3↵
            4↵


  output:   2↵
            [1]|↵
            3↵
            4↵

```

Finally, `testCaseWithInput` has an identical interface to `testCaseChain`, but input doesn't change each time `cmd()` is called.


## Checking return values

There are 2 ways to check the result of a function:

- either `t.test(command, op, expected, msgOrInfo)`

```js
function foo() { return 42; }
t.test(foo, '===', 42);
```

- or `t.cmd(command, input, expectedOutput, msgOrExpectedResultInfo)` (parameter `input` does not exist with `testCaseChain` and `testCaseWithInput`)

```js
t.cmd(foo, 'input', 'expected output', {
    expected: 42, // if specified, the value will be checked
    op: '===', // is default

    error: true, // alias of {op: 'hasError', expected: true}
    error: "...", // alias of {op: 'errorMsg', expected: "..."}
    error: OtherValue, // alias of {op: 'error', expected: OtherValue}
});
```

All operators are stored in the `TestFramework.operators` array, which a script can complete. It contains:

- classic operators: `===`, `!=`, `<`, etc
- more advanced operators:
    - `eqv`: converts to string then compares. Conversion is done via a dedicated function explained in the next chapter.
    - `!!`: a double negation or, more simply, `Boolean(result) == expected`.
    - `between` for `expected[0] <= result < expected[1]`.
    - `inclusiveBetween` for `expected[0] <= result <= expected[1]`
    - `is` for `Object.is(result, expected)`
- some keywords:
    - `instanceof`
    - `in`
    - `is type` for `typeof result === expected`.
    - `key` for `expected in result`.
- error operators: `error`, `errorMsg`, `errorType`, `hasError`.

`t.test()` has different aliases:

- test on Boolean:
    - `eqvTrue(command, msgOrInfo)` = `test(command, '!!', true, msgOrInfo)`
    - `eqvFalse(command, msgOrInfo` = `test(command, '!!', false, msgOrInfo)`
    - `eqTrue(command, msgOrInfo` = `test(command, '===', true, msgOrInfo)`
    - `eqFalse(command, msgOrInfo` = `test(command, '===', false, msgOrInfo)`
- test on errors:
    - `error(command, expected, msgOrInfo` = `test(command, 'error', expected, msgOrInfo)`
    - `errorMsg(command, expected, msgOrInfo` = `test(command, 'errorMsg', expected, msgOrInfo)`
    - `errorType(command, expected, msgOrInfo` = `test(command, 'errorType', expected, msgOrInfo)`
    - `hasError(command, msgOrInfo` = `test(command, 'hasError', true, msgOrInfo)`
- alias for operators:
    - `eqv(command, expected, msgOrInfo` = `test(command, 'eqv', expected, msgOrInfo)`
    - `is(command, expected, msgOrInfo` = `test(command, 'is', expected, msgOrInfo)`
    - `eq(command, expected, msgOrInfo` = `test(command, '===', expected, msgOrInfo)`
    - `ne(command, expected, msgOrInfo` = `test(command, '!=', expected, msgOrInfo)`
    - `lt(command, expected, msgOrInfo` = `test(command, '<', expected, msgOrInfo)`
    - `gt(command, expected, msgOrInfo` = `test(command, '>', expected, msgOrInfo)`
    - `le(command, expected, msgOrInfo` = `test(command, '<=', expected, msgOrInfo)`
    - `ge(command, expected, msgOrInfo` = `test(command, '>=', expected, msgOrInfo)`


### Add a string conversion function

String conversions are used for display and with the `eqv` operator.

The conversion function used is `TestFramework.addStringCodeFragments` which displays basic types or use functions registered with `TestFramework.addStringCodeFragments.{registerClassName, registerClass, registerSymbol, register}`.

See sources in `testframework.js` for details.


## Command parameter

the `command` parameter of `t.cmd()` and `t.test()` can be either

- a function
- a string to be executed in the global context (not recommended)
- an array with a function or string as the first parameter and arguments for the following parameters

```js
t.eq(String, ""); // for String()
t.eq("String()", ""); // equivalent to eval("String()")
t.eq([String, 2e33], "2e+33"); // for String(2e33)
t.eq(["String", 2e33], "2e+33"); // equivalent to eval("String(2e33)")
```

Unfortunately, this doesn't work well with methods because the `this` is lost:

```js
const obj = [1,2,3];
t.eq(obj.join, "[1,2,3]"); // error: Value is undefined
```

To get around this problem, we can use `calleeWrapper()`, which retains the `this` and adds an object name to the function name display:

```js
const obj = calleeWrapper("myArray", [1,2,3]);
t.eq(obj.join, "[1,2,3]"); // obj.join()
t.eq([obj.join, ", "], "1, 2, 3"); // oobj.join(", ")
```


## Placeholders

Placeholders are used with `t.cmd()` to insert cursors or selections in the input and expected output. They are configured via `t.config()` or globally via `testFramework.config()`.

There are a total of 7 placeholders distributed in 5 configuration variables:

- `cursor`: represents the primary cursor. The default is `'|'`.
- `selection`: represents the start and end of a selection. Default is `'[]'`.
- `virtualText`: used with block selection to introduce spaces to place a cursor outside the document text. It must always be followed by a placeholder or new line.
- `secondaryCursor`: secondary cursor in multi-cursor. No default value.
- `secondarySelection`: secondary selection in multi-cursor. No default value.

If the placeholder is an empty string, it will not be used.

If `cursor` and `secondaryCursor` have the same value, then the first cursor found will be the primary cursor. The same applies to selection.

In some cases, a placeholder may not be configured, but used in the result display. This is configured via the `-S` command line option or with the following 5 configuration variables:

- `cursor2`: the default is `'|'`
- `selection2`: the default is `'[]'`
- `virtualText2`: the default is `'·'`
- `secondaryCursor2`: the default is `'┆'`
- `secondarySelection2`: the default is `'❲❳'`

These variables are also used when a placeholder has a display conflict with another placeholder.

Unlike the first 5, setting an empty value reset to default value.


## Constants

There are 3 special constants:

- `DUAL_MODE`: used with the `blockSelection` configuration variable so that `t.cmd()` runs the test first with block selection disabled, then, if the test does not fail, with block selection enabled.
- `ALWAYS_DUAL_MODE`: Same as `DUAL_MODE`, but the 2 modes are always executed.
- `AS_INPUT`: Special value for `expectedOutput` from `t.cmd()` which indicates that the expected output is the same as the input value.


## Interface

- `loadScript`: load a javascript file in the context of the global object.
- `loadModule`: load a javascript file as a module.

### TestFramework

For full documentation, see the functions and classes exported from `testframework.js` (those prefixed with `export`).

- `TestFramework.DUAL_MODE` ; alias: `DUAL_MODE`
- `TestFramework.ALWAYS_DUAL_MODE` ; alias: `ALWAYS_DUAL_MODE`
- `TestFramework.EXPECTED_OUTPUT_AS_INPUT` ; alias: `AS_INPUT`
- `TestFramework.operators`
- `TestFramework.formatArgs(args, sep)`
- `TestFramework.toComparableString(value)`
- `TestFramework.functionCallToString(fn, args)`
- `TestFramework.calleeWrapper(name, obj, newObj)`
- `TestFramework.addStringCodeFragments(value, codeFragments, ancestors, mustBeStable)`
- `TestFramework.addStringCodeFragments.registerClassName(name, addString)`
- `TestFramework.addStringCodeFragments.registerClass(type, addString)`
- `TestFramework.addStringCodeFragments.registerSymbol(symbol, addString)`
- `TestFramework.addStringCodeFragments.register(addString)`

- `testFramework.config(object)`
- `testFramework.breakOnError(bool)`
- `testFramework.testCase(nameOrConfig, fn)` ; alias: `testCase`
- `testFramework.testCaseChain(nameOrConfig, input, fn)` ; alias: `testCaseChain`
- `testFramework.testCaseWithInput(nameOrConfig, input, fn)` ; alias: `testCaseWithInput`
- `testFramework.print(...args)` = `testFramework.printSep(' ', ...args)` ; alias: `print`
- `testFramework.printSep(sep, ...args)` ; alias: `printSep`
