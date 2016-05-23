#ifndef KATEVI_EMULATED_COMMAND_BAR_MATCHHIGHLIGHTER_H
#define KATEVI_EMULATED_COMMAND_BAR_MATCHHIGHLIGHTER_H

#include <ktexteditor/attribute.h>

#include <QObject>

namespace KTextEditor
{
    class ViewPrivate;
    class Range;
    class MovingRange;
}

namespace KateVi
{
class MatchHighlighter : public QObject
{
    Q_OBJECT
public:
    MatchHighlighter(KTextEditor::ViewPrivate* view);
    ~MatchHighlighter();
    void updateMatchHighlight(const KTextEditor::Range &matchRange);
private Q_SLOTS:
    void updateMatchHighlightAttrib();
private:
    KTextEditor::ViewPrivate *m_view;
    KTextEditor::Attribute::Ptr m_highlightMatchAttribute;
    KTextEditor::MovingRange *m_highlightedMatch;
};
}
#endif
