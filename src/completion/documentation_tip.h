#ifndef KTEXTEDITOR_DOC_TIP_H
#define KTEXTEDITOR_DOC_TIP_H

#include <QStackedWidget>
#include <QTextEdit>

class DocTip : public QFrame
{
    Q_OBJECT
public:
    explicit DocTip(QWidget *parent = nullptr);
    void updatePosition();

    void setText(const QString &);
    void setWidget(QWidget *w);

private:
    QStackedWidget m_stack;
    QTextEdit *const m_textEdit;
};

#endif
