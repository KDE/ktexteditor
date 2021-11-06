v.setCursorPosition(3, document.lineLength(3));
v.enter();
v.paste("def internal():\n    a = '''multi\nline\n    string\n    '''\n    if True:\n        return True");
