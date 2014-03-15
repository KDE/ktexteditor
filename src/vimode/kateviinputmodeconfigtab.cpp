/*  This file is part of the KDE libraries and the Kate part.
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

#include "kateviinputmodeconfigtab.h"
#include "kateconfig.h"
#include "kateviglobal.h"
#include "katevikeyparser.h"
#include "kateglobal.h"

#include <KMessageBox>
#include <KLocalizedString>

#include <QFileDialog>
#include <QTableWidgetItem>
#include <QWhatsThis>

#include "ui_viinputmodeconfigwidget.h"

KateViInputModeConfigTab::KateViInputModeConfigTab(QWidget *parent, KateViGlobal *viGlobal)
    : KateConfigPage(parent)
    , m_viGlobal(viGlobal)
{

    // This will let us have more separation between this page and
    // the QTabWidget edge (ereslibre)
    QVBoxLayout *layout = new QVBoxLayout;
    QWidget *newWidget = new QWidget(this);

    ui = new Ui::ViInputModeConfigWidget();
    ui->setupUi(newWidget);

    // Make the header take all the width in equal parts.
    ui->tblNormalModeMappings->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    ui->tblInsertModeMappings->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    ui->tblVisualModeMappings->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);

    // What's This? help can be found in the ui file
    reload();

    //
    // after initial reload, connect the stuff for the changed () signal
    //

    connect(ui->chkViInputModeDefault, SIGNAL(toggled(bool)), this, SLOT(slotChanged()));
    connect(ui->chkViCommandsOverride, SIGNAL(toggled(bool)), this, SLOT(slotChanged()));
    connect(ui->chkViRelLineNumbers, SIGNAL(toggled(bool)), this, SLOT(slotChanged()));
    connect(ui->tblNormalModeMappings, SIGNAL(cellChanged(int,int)), this, SLOT(slotChanged()));
    connect(ui->btnAddNewRow, SIGNAL(clicked()), this, SLOT(addMappingRow()));
    connect(ui->btnAddNewRow, SIGNAL(clicked()), this, SLOT(slotChanged()));
    connect(ui->btnRemoveSelectedRows, SIGNAL(clicked()), this, SLOT(removeSelectedMappingRows()));
    connect(ui->btnRemoveSelectedRows, SIGNAL(clicked()), this, SLOT(slotChanged()));
    connect(ui->btnImportNormal, SIGNAL(clicked()), this, SLOT(importNormalMappingRow()));
    connect(ui->btnImportNormal, SIGNAL(clicked()), this, SLOT(slotChanged()));

    layout->addWidget(newWidget);
    setLayout(layout);
}

KateViInputModeConfigTab::~KateViInputModeConfigTab()
{
    delete ui;
}

void KateViInputModeConfigTab::applyTab(QTableWidget *mappingsTable, KateViGlobal::MappingMode mode)
{
    for (int i = 0; i < mappingsTable->rowCount(); i++) {
        QTableWidgetItem *from = mappingsTable->item(i, 0);
        QTableWidgetItem *to = mappingsTable->item(i, 1);
        QTableWidgetItem *recursive = mappingsTable->item(i, 2);

        if (from && to && recursive) {
            const KateViGlobal::MappingRecursion recursion = recursive->checkState() == Qt::Checked ?
                    KateViGlobal::Recursive :
                    KateViGlobal::NonRecursive;
            m_viGlobal->addMapping(mode, from->text(), to->text(), recursion);
        }
    }
}

void KateViInputModeConfigTab::reloadTab(QTableWidget *mappingsTable, KateViGlobal::MappingMode mode)
{
    QStringList l = m_viGlobal->getMappings(mode);
    mappingsTable->setRowCount(l.size());

    int i = 0;
    foreach (const QString &f, l) {
        QTableWidgetItem *from = new QTableWidgetItem(KateViKeyParser::self()->decodeKeySequence(f));
        QString s = m_viGlobal->getMapping(mode, f);
        QTableWidgetItem *to = new QTableWidgetItem(KateViKeyParser::self()->decodeKeySequence(s));
        QTableWidgetItem *recursive = new QTableWidgetItem();
        recursive->setFlags(Qt::ItemIsEnabled | Qt::ItemIsUserCheckable | Qt::ItemIsSelectable);
        const bool isRecursive = m_viGlobal->isMappingRecursive(mode, f);
        recursive->setCheckState(isRecursive ? Qt::Checked : Qt::Unchecked);

        mappingsTable->setItem(i, 0, from);
        mappingsTable->setItem(i, 1, to);
        mappingsTable->setItem(i, 2, recursive);

        i++;
    }
}

void KateViInputModeConfigTab::apply()
{
    // nothing changed, no need to apply stuff
    if (!hasChanged()) {
        return;
    }
    m_changed = false;

    KateViewConfig::global()->configStart();

    // General options.
    KateViewConfig::global()->setViInputMode(ui->chkViInputModeDefault->isChecked());
    KateViewConfig::global()->setViRelativeLineNumbers(ui->chkViRelLineNumbers->isChecked());
    KateViewConfig::global()->setViInputModeStealKeys(ui->chkViCommandsOverride->isChecked());

    // Mappings.
    m_viGlobal->clearMappings(KateViGlobal::NormalModeMapping);
    applyTab(ui->tblNormalModeMappings, KateViGlobal::NormalModeMapping);
    applyTab(ui->tblInsertModeMappings, KateViGlobal::InsertModeMapping);
    applyTab(ui->tblVisualModeMappings, KateViGlobal::VisualModeMapping);

    KateViewConfig::global()->configEnd();
}

void KateViInputModeConfigTab::reload()
{
    // General options.
    ui->chkViInputModeDefault->setChecked(KateViewConfig::global()->viInputMode());
    ui->chkViRelLineNumbers->setChecked( KateViewConfig::global()->viRelativeLineNumbers () );
    ui->chkViCommandsOverride->setChecked(KateViewConfig::global()->viInputModeStealKeys());
    ui->chkViCommandsOverride->setEnabled(ui->chkViInputModeDefault->isChecked());

    // Mappings.
    reloadTab(ui->tblNormalModeMappings, KateViGlobal::NormalModeMapping);
    reloadTab(ui->tblInsertModeMappings, KateViGlobal::InsertModeMapping);
    reloadTab(ui->tblVisualModeMappings, KateViGlobal::VisualModeMapping);
}

void KateViInputModeConfigTab::showWhatsThis(const QString &text)
{
    QWhatsThis::showText(QCursor::pos(), text);
}

void KateViInputModeConfigTab::addMappingRow()
{
    // Pick the current widget.
    QTableWidget *mappingsTable = ui->tblNormalModeMappings;
    if (ui->tabMappingModes->currentIndex() == 1) {
        mappingsTable = ui->tblInsertModeMappings;
    } else if (ui->tabMappingModes->currentIndex() == 2) {
        mappingsTable = ui->tblVisualModeMappings;
    }

    // And add a new row.
    int rows = mappingsTable->rowCount();
    mappingsTable->insertRow(rows);
    QTableWidgetItem *recursive = new QTableWidgetItem();
    recursive->setFlags(Qt::ItemIsEnabled | Qt::ItemIsUserCheckable | Qt::ItemIsSelectable);
    recursive->setCheckState(Qt::Unchecked);
    mappingsTable->setItem(rows, 2, recursive);
    mappingsTable->setCurrentCell(rows, 0);
    mappingsTable->editItem(mappingsTable->currentItem());
}

void KateViInputModeConfigTab::removeSelectedMappingRows()
{
    // Pick the current widget.
    QTableWidget *mappingsTable = ui->tblNormalModeMappings;
    if (ui->tabMappingModes->currentIndex() == 1) {
        mappingsTable = ui->tblInsertModeMappings;
    } else if (ui->tabMappingModes->currentIndex() == 2) {
        mappingsTable = ui->tblVisualModeMappings;
    }

    // And remove the selected rows.
    QList<QTableWidgetSelectionRange> l = mappingsTable->selectedRanges();
    foreach (const QTableWidgetSelectionRange &range, l) {
        for (int i = 0; i < range.bottomRow() - range.topRow() + 1; i++) {
            mappingsTable->removeRow(range.topRow());
        }
    }
}

void KateViInputModeConfigTab::importNormalMappingRow()
{
    QString fileName = QFileDialog::getOpenFileName(this);

    if (fileName.isEmpty()) {
        return;
    }

    QFile configFile(fileName);
    if (! configFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        KMessageBox::error(this, i18n("Unable to open the config file for reading."), i18n("Unable to open file"));
        return;
    }
    QTextStream stream(&configFile);
    while (! stream.atEnd()) {
        QStringList line = stream.readLine().split(QLatin1String(" "));

        // TODO - allow recursive mappings to be read.
        if (line.size() > 2 && (line[0] == QLatin1String("noremap") || line[0] == QLatin1String("no")
                                || line[0] == QLatin1String("nnoremap") || line [0] == QLatin1String("nn"))) {
            int rows = ui->tblNormalModeMappings->rowCount();
            ui->tblNormalModeMappings->insertRow(rows);
            ui->tblNormalModeMappings->setItem(rows, 0, new QTableWidgetItem(line[1]));
            ui->tblNormalModeMappings->setItem(rows, 1, new QTableWidgetItem(line[2]));
            QTableWidgetItem *recursive = new QTableWidgetItem();
            recursive->setFlags(Qt::ItemIsEnabled | Qt::ItemIsUserCheckable | Qt::ItemIsSelectable);
            recursive->setCheckState(Qt::Unchecked);
            ui->tblNormalModeMappings->setItem(rows, 2, recursive);
        }
    }
}
