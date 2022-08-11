view.alignOn(document.documentRange(), '-');
view.alignOn(document.documentRange(), ':\\s(.)');
view.setBlockSelection(true);
view.alignOn(new Range(1, 14, 2, 30), ',\\s(.)');
view.alignOn(new Range(1, 19, 2, 30), ',\\s(.)');
view.alignOn(new Range(1, 24, 2, 30), ',\\s(.)');
