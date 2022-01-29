#ifndef KTEXTEDITOR_DOC_TIP_H
#define KTEXTEDITOR_DOC_TIP_H

#include <QPlainTextEdit>

class DocTip : public QPlainTextEdit
{
    Q_OBJECT
public:
    explicit DocTip(QWidget *parent = nullptr);
    void updatePosition();
};

#endif
