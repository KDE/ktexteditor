/* This file is part of the KDE libraries
   Copyright (C) 2002, 2003 Anders Lund <anders.lund@lund.tdcadsl.dk>
   Copyright (C) 2003 Christoph Cullmann <cullmann@kde.org>
   Copyright (C) 2001 Joseph Wenninger <jowenn@kde.org>
   Copyright (C) 2006-2016 Dominik Haumann <dhaumann@kde.org>
   Copyright (C) 2007 Mirko Stocker <me@misto.ch>
   Copyright (C) 2009 Michel Ludwig <michel.ludwig@kdemail.net>
   Copyright (C) 1999 Jochen Wilhelmy <digisnap@cs.tu-berlin.de>
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Library General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Library General Public License for more details.
 *
 *  You should have received a copy of the GNU Library General Public License
 *  along with this library; see the file COPYING.LIB.  If not, write to
 *  the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 *  Boston, MA 02110-1301, USA.
 */

#ifndef KATE_DIALOGS_H
#define KATE_DIALOGS_H

#include "katehighlight.h"
#include "kateviewhelpers.h"
#include "kateconfigpage.h"

#include <ktexteditor/attribute.h>
#include <ktexteditor/modificationinterface.h>
#include <ktexteditor/document.h>

#include <sonnet/configwidget.h>
#include <sonnet/dictionarycombobox.h>

#include <QColor>
#include <QDialog>
#include <QTabWidget>
#include <QTreeWidget>

class ModeConfigPage;
namespace KTextEditor { class DocumentPrivate; }
namespace KTextEditor { class ViewPrivate; }
namespace KTextEditor { class Message; }

namespace KIO
{
class Job;
class TransferJob;
}

class KComboBox;
class KShortcutsEditor;
class QSpinBox;
class KPluginSelector;
class KPluginInfo;
class KProcess;

class QCheckBox;
class QLabel;
class QCheckBox;
class QKeyEvent;
class QTemporaryFile;
class QTableWidget;

namespace Ui
{
class TextareaAppearanceConfigWidget;
class BordersAppearanceConfigWidget;
class NavigationConfigWidget;
class EditConfigWidget;
class IndentationConfigWidget;
class OpenSaveConfigWidget;
class OpenSaveConfigAdvWidget;
class CompletionConfigTab;
class SpellCheckConfigWidget;
}

class KateGotoBar : public KateViewBarWidget
{
    Q_OBJECT

public:
    explicit KateGotoBar(KTextEditor::View *view, QWidget *parent = nullptr);

    void closed() override;

public Q_SLOTS:
    void updateData();

protected Q_SLOTS:
    void gotoLine();
    void gotoClipboard();

protected:
    void keyPressEvent(QKeyEvent *event) override;
    bool eventFilter(QObject *, QEvent *) override;
    void showEvent(QShowEvent *event) override;

private:
    KTextEditor::View *const m_view;
    QSpinBox *m_gotoRange = nullptr;
    QToolButton *m_modifiedUp = nullptr;
    QToolButton *m_modifiedDown = nullptr;
    int m_wheelDelta = 0; // To accumulate "wheel-deltas" to become e.g. a touch-pad usable
};

class KateDictionaryBar : public KateViewBarWidget
{
    Q_OBJECT

public:
    explicit KateDictionaryBar(KTextEditor::ViewPrivate *view, QWidget *parent = nullptr);
    virtual ~KateDictionaryBar();

public Q_SLOTS:
    void updateData();

protected Q_SLOTS:
    void dictionaryChanged(const QString &dictionary);

private:
    KTextEditor::ViewPrivate *m_view;
    Sonnet::DictionaryComboBox *m_dictionaryComboBox;
};

class KateIndentConfigTab : public KateConfigPage
{
    Q_OBJECT

public:
    explicit KateIndentConfigTab(QWidget *parent);
    ~KateIndentConfigTab() override;
    QString name() const override;

protected:
    Ui::IndentationConfigWidget *ui;

public Q_SLOTS:
    void apply() override;
    void reload() override;
    void reset() override {}
    void defaults() override {}

private Q_SLOTS:
    void slotChanged();
    void showWhatsThis(const QString &text);
};

class KateCompletionConfigTab : public KateConfigPage
{
    Q_OBJECT

public:
    explicit KateCompletionConfigTab(QWidget *parent);
    ~KateCompletionConfigTab() override;
    QString name() const override;

protected:
    Ui::CompletionConfigTab *ui;

public Q_SLOTS:
    void apply() override;
    void reload() override;
    void reset() override {}
    void defaults() override {}

private Q_SLOTS:
    void showWhatsThis(const QString &text);
};

class KateEditGeneralConfigTab : public KateConfigPage
{
    Q_OBJECT

public:
    explicit KateEditGeneralConfigTab(QWidget *parent);
    ~KateEditGeneralConfigTab() override;
    QString name() const override;

private:
    Ui::EditConfigWidget *ui;

public Q_SLOTS:
    void apply() override;
    void reload() override;
    void reset() override {}
    void defaults() override {}
};

class KateNavigationConfigTab : public KateConfigPage
{
    Q_OBJECT

public:
    explicit KateNavigationConfigTab(QWidget *parent);
    ~KateNavigationConfigTab() override;
    QString name() const override;

private:
    Ui::NavigationConfigWidget *ui;

public Q_SLOTS:
    void apply() override;
    void reload() override;
    void reset() override {}
    void defaults() override {}
};

class KateSpellCheckConfigTab : public KateConfigPage
{
    Q_OBJECT

public:
    explicit KateSpellCheckConfigTab(QWidget *parent);
    ~KateSpellCheckConfigTab() override;
    QString name() const override;

protected:
    Ui::SpellCheckConfigWidget *ui;
    Sonnet::ConfigWidget *m_sonnetConfigWidget;

public Q_SLOTS:
    void apply() override;
    void reload() override;
    void reset() override {}
    void defaults() override {}

private Q_SLOTS:
    void showWhatsThis(const QString &text);
};

class KateEditConfigTab : public KateConfigPage
{
    Q_OBJECT

public:
    explicit KateEditConfigTab(QWidget *parent);
    ~KateEditConfigTab() override;
    QString name() const override;
    QString fullName() const override;
    QIcon icon() const override;

public Q_SLOTS:
    void apply() override;
    void reload() override;
    void reset() override;
    void defaults() override;

private:
    KateEditGeneralConfigTab *editConfigTab;
    KateNavigationConfigTab *navigationConfigTab;
    KateIndentConfigTab *indentConfigTab;
    KateCompletionConfigTab *completionConfigTab;
    KateSpellCheckConfigTab *spellCheckConfigTab;
    QList<KateConfigPage *> m_inputModeConfigTabs;
};

class KateViewDefaultsConfig : public KateConfigPage
{
    Q_OBJECT

public:
    explicit KateViewDefaultsConfig(QWidget *parent);
    ~KateViewDefaultsConfig() override;
    QString name() const override;
    QString fullName() const override;
    QIcon icon() const override;

public Q_SLOTS:
    void apply() override;
    void reload() override;
    void reset() override;
    void defaults() override;

private:
    Ui::TextareaAppearanceConfigWidget *const textareaUi;
    Ui::BordersAppearanceConfigWidget *const bordersUi;
};

class KateSaveConfigTab : public KateConfigPage
{
    Q_OBJECT

public:
    explicit KateSaveConfigTab(QWidget *parent);
    ~KateSaveConfigTab() override;
    QString name() const override;
    QString fullName() const override;
    QIcon icon() const override;

public Q_SLOTS:
    void apply() override;
    void reload() override;
    void reset() override;
    void defaults() override;
    void swapFileModeChanged(int);

protected:
    //why?
    //KComboBox *m_encoding, *m_encodingDetection, *m_eol;
    QCheckBox *cbLocalFiles, *cbRemoteFiles;
    QCheckBox *replaceTabs, *removeSpaces, *allowEolDetection;
    class QSpinBox *blockCount;
    class QLabel *blockCountLabel;

private:
    Ui::OpenSaveConfigWidget *ui;
    Ui::OpenSaveConfigAdvWidget *uiadv;
    ModeConfigPage *modeConfigPage;
};

/**
 * This dialog will prompt the user for what do with a file that is
 * modified on disk.
 * If the file wasn't deleted, it has a 'diff' button, which will create
 * a diff file (uing diff(1)) and launch that using KRun.
 */
class KateModOnHdPrompt : public QObject
{
    Q_OBJECT
public:
    enum Status {
        Reload = 1, // 0 is QDialog::Rejected
        Save,
        Overwrite,
        Ignore,
        Close
    };
    KateModOnHdPrompt(KTextEditor::DocumentPrivate *doc,
                      KTextEditor::ModificationInterface::ModifiedOnDiskReason modtype,
                      const QString &reason);
    ~KateModOnHdPrompt();

Q_SIGNALS:
    void saveAsTriggered();
    void ignoreTriggered();
    void reloadTriggered();

private Q_SLOTS:
    /**
     * Show a diff between the document text and the disk file.
     */
    void slotDiff();

private Q_SLOTS:
    void slotDataAvailable(); ///< read data from the process
    void slotPDone(); ///< Runs the diff file when done

private:
    KTextEditor::DocumentPrivate *m_doc;
    QPointer<KTextEditor::Message> m_message;
    KTextEditor::ModificationInterface::ModifiedOnDiskReason m_modtype;
    KProcess *m_proc;
    QTemporaryFile *m_diffFile;
    QAction *m_diffAction;
};

#endif
