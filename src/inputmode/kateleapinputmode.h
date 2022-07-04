/*
    SPDX-FileCopyrightText: KDE Developers

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef KATE_LEAP_INPUT_MODE_H
#define KATE_LEAP_INPUT_MODE_H

#include <memory>

#include "kateabstractinputmode.h"

class KTEXTEDITOR_EXPORT KateLeapInputMode : public KateAbstractInputMode
{
    explicit KateLeapInputMode(KateViewInternal *viewInternal);
    friend class KateLeapInputModeFactory;

public:
    KTextEditor::View::ViewMode viewMode() const override;
    QString viewModeHuman() const override;
    KTextEditor::View::InputMode viewInputMode() const override;
    QString viewInputModeHuman() const override;

    void activate() override;
    void deactivate() override;
    void reset() override;

    bool overwrite() const override;
    void overwrittenChar(const QChar &) override;

    void clearSelection() override;
    bool stealKey(QKeyEvent *) override;

    void gotFocus() override;
    void lostFocus() override;

    void readSessionConfig(const KConfigGroup &config) override;
    void writeSessionConfig(KConfigGroup &config) override;
    void updateRendererConfig() override;
    void updateConfig() override;
    void readWriteChanged(bool rw) override;

    void find() override;
    void findSelectedForwards() override;
    void findSelectedBackwards() override;
    void findReplace() override;
    void findNext() override;
    void findPrevious() override;

    void activateCommandLine() override;

    bool keyPress(QKeyEvent *) override;
    bool keyRelease(QKeyEvent *) override;
    bool blinkCaret() const override;
    KateRenderer::caretStyles caretStyle() const override;

    void toggleInsert() override;
    void launchInteractiveCommand(const QString &command) override;

    QString bookmarkLabel(int line) const override;

private:
    struct Private;
    QScopedPointer<Private> d;
};

#endif
