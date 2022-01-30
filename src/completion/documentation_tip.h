#ifndef KTEXTEDITOR_DOC_TIP_H
#define KTEXTEDITOR_DOC_TIP_H

#include <QStackedWidget>
#include <QTextBrowser>

class DocTip : public QFrame
{
    Q_OBJECT
public:
    explicit DocTip(QWidget *parent = nullptr);
    void updatePosition();

    QWidget *currentWidget();

    void setText(const QString &);
    void setWidget(QWidget *w);

private:
    QStackedWidget m_stack;
    QTextBrowser *const m_textView;
};

#endif
