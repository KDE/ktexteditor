/* This file is part of the KDE libraries
   Copyright (C) 2002, 2003 Anders Lund <anders.lund@lund.tdcadsl.dk>
   Copyright (C) 2003 Christoph Cullmann <cullmann@kde.org>
   Copyright (C) 2001 Joseph Wenninger <jowenn@kde.org>
   Copyright (C) 2006 Dominik Haumann <dhdev@gmx.de>
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

#ifndef __KATE_DIALOGS_H__
#define __KATE_DIALOGS_H__

#include "katehighlight.h"
#include "kateviewhelpers.h"
#include "kateconfigpage.h"

#include <ktexteditor/attribute.h>
#include <ktexteditor/modificationinterface.h>
#include <ktexteditor/document.h>

#include <sonnet/configwidget.h>
#include <sonnet/dictionarycombobox.h>

#include <QStringList>
#include <QColor>
#include <QDialog>
#include <QTabWidget>
#include <QTreeWidget>

class ModeConfigPage;
namespace KTextEditor { class DocumentPrivate; }
namespace KTextEditor { class ViewPrivate; }

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
class ModOnHdWidget;
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
    explicit KateGotoBar(KTextEditor::View *view, QWidget *parent = 0);

    void updateData();

protected Q_SLOTS:
    void gotoLine();

protected:
    void keyPressEvent(QKeyEvent *event) Q_DECL_OVERRIDE;

private:
    KTextEditor::View *const m_view;
    QSpinBox *gotoRange;
};

class KateDictionaryBar : public KateViewBarWidget
{
    Q_OBJECT

public:
    explicit KateDictionaryBar(KTextEditor::ViewPrivate *view, QWidget *parent = NULL);
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
    KateIndentConfigTab(QWidget *parent);
    ~KateIndentConfigTab();
    QString name() const Q_DECL_OVERRIDE;

protected:
    Ui::IndentationConfigWidget *ui;

public Q_SLOTS:
    void apply() Q_DECL_OVERRIDE;
    void reload() Q_DECL_OVERRIDE;
    void reset() Q_DECL_OVERRIDE {}
    void defaults() Q_DECL_OVERRIDE {}

private Q_SLOTS:
    void slotChanged();
    void showWhatsThis(const QString &text);
};

class KateCompletionConfigTab : public KateConfigPage
{
    Q_OBJECT

public:
    KateCompletionConfigTab(QWidget *parent);
    ~KateCompletionConfigTab();
    QString name() const Q_DECL_OVERRIDE;

protected:
    Ui::CompletionConfigTab *ui;

public Q_SLOTS:
    void apply() Q_DECL_OVERRIDE;
    void reload() Q_DECL_OVERRIDE;
    void reset() Q_DECL_OVERRIDE {}
    void defaults() Q_DECL_OVERRIDE {}

private Q_SLOTS:
    void showWhatsThis(const QString &text);
};

class KateEditGeneralConfigTab : public KateConfigPage
{
    Q_OBJECT

public:
    KateEditGeneralConfigTab(QWidget *parent);
    ~KateEditGeneralConfigTab();
    QString name() const Q_DECL_OVERRIDE;

private:
    Ui::EditConfigWidget *ui;

public Q_SLOTS:
    void apply() Q_DECL_OVERRIDE;
    void reload() Q_DECL_OVERRIDE;
    void reset() Q_DECL_OVERRIDE {}
    void defaults() Q_DECL_OVERRIDE {}
};

class KateNavigationConfigTab : public KateConfigPage
{
    Q_OBJECT

public:
    KateNavigationConfigTab(QWidget *parent);
    ~KateNavigationConfigTab();
    QString name() const Q_DECL_OVERRIDE;

private:
    Ui::NavigationConfigWidget *ui;

public Q_SLOTS:
    void apply() Q_DECL_OVERRIDE;
    void reload() Q_DECL_OVERRIDE;
    void reset() Q_DECL_OVERRIDE {}
    void defaults() Q_DECL_OVERRIDE {}
};

class KateSpellCheckConfigTab : public KateConfigPage
{
    Q_OBJECT

public:
    KateSpellCheckConfigTab(QWidget *parent);
    ~KateSpellCheckConfigTab();
    QString name() const Q_DECL_OVERRIDE;

protected:
    Ui::SpellCheckConfigWidget *ui;
    Sonnet::ConfigWidget *m_sonnetConfigWidget;

public Q_SLOTS:
    void apply() Q_DECL_OVERRIDE;
    void reload() Q_DECL_OVERRIDE;
    void reset() Q_DECL_OVERRIDE {}
    void defaults() Q_DECL_OVERRIDE {}

private Q_SLOTS:
    void showWhatsThis(const QString &text);
};

class KateEditConfigTab : public KateConfigPage
{
    Q_OBJECT

public:
    KateEditConfigTab(QWidget *parent);
    ~KateEditConfigTab();
    QString name() const Q_DECL_OVERRIDE;
    QString fullName() const Q_DECL_OVERRIDE;
    QIcon icon() const Q_DECL_OVERRIDE;

public Q_SLOTS:
    void apply() Q_DECL_OVERRIDE;
    void reload() Q_DECL_OVERRIDE;
    void reset() Q_DECL_OVERRIDE;
    void defaults() Q_DECL_OVERRIDE;

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
    KateViewDefaultsConfig(QWidget *parent);
    ~KateViewDefaultsConfig();
    QString name() const Q_DECL_OVERRIDE;
    QString fullName() const Q_DECL_OVERRIDE;
    QIcon icon() const Q_DECL_OVERRIDE;

public Q_SLOTS:
    void apply() Q_DECL_OVERRIDE;
    void reload() Q_DECL_OVERRIDE;
    void reset() Q_DECL_OVERRIDE;
    void defaults() Q_DECL_OVERRIDE;

private:
    Ui::TextareaAppearanceConfigWidget *const textareaUi;
    Ui::BordersAppearanceConfigWidget *const bordersUi;
};

class KateSaveConfigTab : public KateConfigPage
{
    Q_OBJECT

public:
    KateSaveConfigTab(QWidget *parent);
    ~KateSaveConfigTab();
    QString name() const Q_DECL_OVERRIDE;
    QString fullName() const Q_DECL_OVERRIDE;
    QIcon icon() const Q_DECL_OVERRIDE;

public Q_SLOTS:
    void apply() Q_DECL_OVERRIDE;
    void reload() Q_DECL_OVERRIDE;
    void reset() Q_DECL_OVERRIDE;
    void defaults() Q_DECL_OVERRIDE;
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

class KateHlDownloadDialog: public QDialog
{
    Q_OBJECT

public:
    KateHlDownloadDialog(QWidget *parent, const char *name, bool modal);
    ~KateHlDownloadDialog();

private:
    static unsigned parseVersion(const QString &);
    class QTreeWidget  *list;
    class QString listData;
    KIO::TransferJob *transferJob;
    QPushButton *m_installButton;

private Q_SLOTS:
    void listDataReceived(KIO::Job *, const QByteArray &data);
    void slotInstall();
};

/**
 * This dialog will prompt the user for what do with a file that is
 * modified on disk.
 * If the file wasn't deleted, it has a 'diff' button, which will create
 * a diff file (uing diff(1)) and launch that using KRun.
 */
class KateModOnHdPrompt : public QDialog
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
                      const QString &reason, QWidget *parent);
    ~KateModOnHdPrompt();

public Q_SLOTS:
    /**
     * Show a diff between the document text and the disk file.
     * This will not close the dialog, since we still need a
     * decision from the user.
     */
    void slotDiff();

private Q_SLOTS:
    void slotOk();
    void slotApply();
    void slotOverwrite();
    void slotClose();
    void slotDataAvailable(); ///< read data from the process
    void slotPDone(); ///< Runs the diff file when done

private:
    Ui::ModOnHdWidget *ui;
    KTextEditor::DocumentPrivate *m_doc;
    KTextEditor::ModificationInterface::ModifiedOnDiskReason m_modtype;
    KProcess *m_proc;
    QTemporaryFile *m_diffFile;
};

#endif
