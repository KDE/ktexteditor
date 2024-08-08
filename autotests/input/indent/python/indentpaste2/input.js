v.setCursorPosition(3, document.lineLength(3));
v.enter();
v.paste("def internal():\n    '''test\n    docstring\n    \n    example\n        foo\n'''\n    if True:\n        return True");
