/*
    SPDX-FileCopyrightText: 2002-2010 Anders Lund <anders@alweb.dk>

    Rewritten based on code of:
    SPDX-FileCopyrightText: 2002 Michael Goffioul <kdeprint@swing.be>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "kateprinter.h"

#include "kateconfig.h"
#include "katedocument.h"
#include "kateglobal.h"
#include "kateview.h"

#include <KConfigGroup>
#include <KSharedConfig>

#include <QApplication>
#include <QMarginsF>
#include <QPageLayout>
#include <QPrintDialog>
#include <QPrintPreviewDialog>
#include <QPrinter>

#include "printconfigwidgets.h"
#include "printpainter.h"

using namespace KatePrinter;

static void writeSettings(QPrinter *printer)
{
    KSharedConfigPtr config = KTextEditor::EditorPrivate::config();
    KConfigGroup group(config, QStringLiteral("Kate Print Settings"));
    KConfigGroup margins(&group, QStringLiteral("Margins"));

    QMarginsF m = printer->pageLayout().margins(QPageLayout::Millimeter);
    margins.writeEntry("left", m.left());
    margins.writeEntry("top", m.top());
    margins.writeEntry("right", m.right());
    margins.writeEntry("bottom", m.bottom());
}

static void readSettings(QPrinter *printer)
{
    KSharedConfigPtr config = KTextEditor::EditorPrivate::config();
    KConfigGroup group(config, QStringLiteral("Kate Print Settings"));
    KConfigGroup margins(&group, QStringLiteral("Margins"));

    qreal left{};
    qreal right{};
    qreal top{};
    qreal bottom{};
    left = margins.readEntry("left", left);
    top = margins.readEntry("top", top);
    right = margins.readEntry("right", right);
    bottom = margins.readEntry("bottom", bottom);
    QMarginsF m = QMarginsF(left, top, right, bottom);
    printer->setPageMargins(m, QPageLayout::Millimeter);
}

static bool print(KTextEditor::DocumentPrivate *doc, KTextEditor::ViewPrivate *view, QPrinter *printer, QWidget *parentWidget)
{
    // docname is now always there, including the right Untitled name
    printer->setDocName(doc->documentName());

    KatePrintTextSettings *kpts = new KatePrintTextSettings;
    KatePrintHeaderFooter *kphf = new KatePrintHeaderFooter;
    KatePrintLayout *kpl = new KatePrintLayout;

    QList<QWidget *> tabs;
    tabs << kpts;
    tabs << kphf;
    tabs << kpl;

    readSettings(printer);

    QPointer<QPrintDialog> printDialog(new QPrintDialog(printer, parentWidget));
    printDialog->setOptionTabs(tabs);

    if (view && view->selection()) {
        printer->setPrintRange(QPrinter::Selection);
        printDialog->setOption(QAbstractPrintDialog::PrintSelection, true);
    }

    printDialog->setOption(QAbstractPrintDialog::PrintPageRange, true);

    const int dlgCode = printDialog->exec();
    if (dlgCode != QDialog::Accepted || !printDialog) {
        delete printDialog;
        return false;
    }

    writeSettings(printer);

    PrintPainter painter(doc, view);

    // configure the painter
    painter.setPrintGuide(kpts->printGuide());
    painter.setPrintLineNumbers(kpts->printLineNumbers());
    painter.setDontPrintFoldedCode(kpts->dontPrintFoldedCode());

    painter.setColorScheme(kpl->colorScheme());
    painter.setTextFont(kpl->textFont());
    painter.setUseBackground(kpl->useBackground());
    painter.setUseBox(kpl->useBox());
    painter.setBoxMargin(kpl->boxMargin());
    painter.setBoxWidth(kpl->boxWidth());
    painter.setBoxColor(kpl->boxColor());

    painter.setHeadersFont(kphf->font());

    painter.setUseHeader(kphf->useHeader());
    painter.setHeaderBackground(kphf->headerBackground());
    painter.setHeaderForeground(kphf->headerForeground());
    painter.setUseHeaderBackground(kphf->useHeaderBackground());
    painter.setHeaderFormat(kphf->headerFormat());

    painter.setUseFooter(kphf->useFooter());
    painter.setFooterBackground(kphf->footerBackground());
    painter.setFooterForeground(kphf->footerForeground());
    painter.setUseFooterBackground(kphf->useFooterBackground());
    painter.setFooterFormat(kphf->footerFormat());

    delete printDialog;
    painter.paint(printer);

    return true;
}

// END KatePrinterPrivate

// BEGIN KatePrinter

bool KatePrinter::print(KTextEditor::DocumentPrivate *doc, KTextEditor::ViewPrivate *view)
{
    QPrinter printer;
    return print(doc, view, &printer, view ? view : QApplication::activeWindow());
}

bool KatePrinter::printPreview(KTextEditor::DocumentPrivate *doc, KTextEditor::ViewPrivate *view)
{
    QPrinter printer;

    // ensure proper parent & sizing
    auto dialogParent = view ? view : QApplication::activeWindow();
    QPrintPreviewDialog preview(&printer, dialogParent);
    if (dialogParent && dialogParent->window()) {
        preview.resize(dialogParent->window()->size() * 0.75);
    }

    QObject::connect(&preview, &QPrintPreviewDialog::paintRequested, &preview, [doc, view](QPrinter *printer) {
        PrintPainter p(doc, view);
        p.setColorScheme(QStringLiteral("Printing"));
        p.paint(printer);
    });
    return preview.exec();
}

// END KatePrinter
