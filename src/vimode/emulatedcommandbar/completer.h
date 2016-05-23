#ifndef KATEVI_EMULATED_COMMAND_BAR_COMPLETER_H
#define KATEVI_EMULATED_COMMAND_BAR_COMPLETER_H

#include <functional>

namespace KateVi
{
    struct CompletionStartParams
    {
        static CompletionStartParams createModeSpecific(const QStringList& completions, int wordStartPos, std::function<QString(const QString&)> completionTransform = std::function<QString(const QString&)>())
        {
            CompletionStartParams completionStartParams;
            completionStartParams.completionType = ModeSpecific;
            completionStartParams.completions = completions;
            completionStartParams.wordStartPos = wordStartPos;
            completionStartParams.completionTransform = completionTransform;
            return completionStartParams;
        }
        static CompletionStartParams invalid()
        {
            CompletionStartParams completionStartParams;
            completionStartParams.completionType = None;
            return completionStartParams;
        }
        enum CompletionType { None, ModeSpecific, WordFromDocument };
        CompletionType completionType = None;
        int wordStartPos = -1;
        QStringList completions;
        std::function<QString(const QString&)> completionTransform;
    };
}
#endif

