/*
    SPDX-FileCopyrightText: 2007, 2008 Matthew Woehlke <mw_triad@users.sourceforge.net>
    SPDX-FileCopyrightText: 2001-2003 Christoph Cullmann <cullmann@kde.org>
    SPDX-FileCopyrightText: 2002, 2003 Anders Lund <anders.lund@lund.tdcadsl.dk>
    SPDX-FileCopyrightText: 2012-2018 Dominik Haumann <dhaumann@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

// BEGIN Includes
#include "kateschemaconfig.h"

#include "katecolortreewidget.h"
#include "kateconfig.h"
#include "katedocument.h"
#include "kateglobal.h"
#include "katehighlight.h"
#include "katepartdebug.h"
#include "katerenderer.h"
#include "katestyletreewidget.h"
#include "kateview.h"

#include "ui_howtoimportschema.h"

#include <KComboBox>
#include <KConfigGroup>
#include <KMessageBox>

#include <QFileDialog>
#include <QInputDialog>
#include <QMetaEnum>
#include <QProgressDialog>
#include <QPushButton>
#include <QTabWidget>

// END

/**
 * Return the translated name of default style @p n.
 */
static inline QString defaultStyleName(int n)
{
    static QStringList translatedNames;
    if (translatedNames.isEmpty()) {
        translatedNames << i18nc("@item:intable Text context", "Normal");
        translatedNames << i18nc("@item:intable Text context", "Keyword");
        translatedNames << i18nc("@item:intable Text context", "Function");
        translatedNames << i18nc("@item:intable Text context", "Variable");
        translatedNames << i18nc("@item:intable Text context", "Control Flow");
        translatedNames << i18nc("@item:intable Text context", "Operator");
        translatedNames << i18nc("@item:intable Text context", "Built-in");
        translatedNames << i18nc("@item:intable Text context", "Extension");
        translatedNames << i18nc("@item:intable Text context", "Preprocessor");
        translatedNames << i18nc("@item:intable Text context", "Attribute");

        translatedNames << i18nc("@item:intable Text context", "Character");
        translatedNames << i18nc("@item:intable Text context", "Special Character");
        translatedNames << i18nc("@item:intable Text context", "String");
        translatedNames << i18nc("@item:intable Text context", "Verbatim String");
        translatedNames << i18nc("@item:intable Text context", "Special String");
        translatedNames << i18nc("@item:intable Text context", "Imports, Modules, Includes");

        translatedNames << i18nc("@item:intable Text context", "Data Type");
        translatedNames << i18nc("@item:intable Text context", "Decimal/Value");
        translatedNames << i18nc("@item:intable Text context", "Base-N Integer");
        translatedNames << i18nc("@item:intable Text context", "Floating Point");
        translatedNames << i18nc("@item:intable Text context", "Constant");

        translatedNames << i18nc("@item:intable Text context", "Comment");
        translatedNames << i18nc("@item:intable Text context", "Documentation");
        translatedNames << i18nc("@item:intable Text context", "Annotation");
        translatedNames << i18nc("@item:intable Text context", "Comment Variable");
        // this next one is for denoting the beginning/end of a user defined folding region
        translatedNames << i18nc("@item:intable Text context", "Region Marker");
        translatedNames << i18nc("@item:intable Text context", "Information");
        translatedNames << i18nc("@item:intable Text context", "Warning");
        translatedNames << i18nc("@item:intable Text context", "Alert");

        translatedNames << i18nc("@item:intable Text context", "Others");
        // this one is for marking invalid input
        translatedNames << i18nc("@item:intable Text context", "Error");
    }

    // sanity checks
    Q_ASSERT(n >= 0);
    Q_ASSERT(n < translatedNames.size());
    return translatedNames[n];
}

/**
 * Return the number of default styles.
 */
static int defaultStyleCount()
{
    return KTextEditor::dsError + 1;
}

// BEGIN KateSchemaConfigColorTab -- 'Colors' tab
KateSchemaConfigColorTab::KateSchemaConfigColorTab()
{
    QGridLayout *l = new QGridLayout(this);
    setLayout(l);

    ui = new KateColorTreeWidget(this);
    QPushButton *btnUseColorScheme = new QPushButton(i18n("Use Default Colors"), this);

    l->addWidget(ui, 0, 0, 1, 2);
    l->addWidget(btnUseColorScheme, 1, 1);

    l->setColumnStretch(0, 1);
    l->setColumnStretch(1, 0);

    connect(btnUseColorScheme, SIGNAL(clicked()), ui, SLOT(selectDefaults()));
    connect(ui, SIGNAL(changed()), SIGNAL(changed()));
}

KateSchemaConfigColorTab::~KateSchemaConfigColorTab()
{
}

QVector<KateColorItem> KateSchemaConfigColorTab::colorItemList(const KSyntaxHighlighting::Theme &theme) const
{
    QVector<KateColorItem> items;

    //
    // editor background colors
    //
    KateColorItem ci(KSyntaxHighlighting::Theme::BackgroundColor);
    ci.category = i18n("Editor Background Colors");

    ci.name = i18n("Text Area");
    ci.key = QStringLiteral("Color Background");
    ci.whatsThis = i18n("<p>Sets the background color of the editing area.</p>");
    ci.defaultColor = theme.editorColor(ci.role);
    items.append(ci);

    ci.role = KSyntaxHighlighting::Theme::TextSelection;
    ci.name = i18n("Selected Text");
    ci.key = QStringLiteral("Color Selection");
    ci.whatsThis = i18n("<p>Sets the background color of the selection.</p><p>To set the text color for selected text, use the &quot;<b>Configure Highlighting</b>&quot; dialog.</p>");
    ci.defaultColor = theme.editorColor(ci.role);
    items.append(ci);

    ci.role = KSyntaxHighlighting::Theme::CurrentLine;
    ci.name = i18n("Current Line");
    ci.key = QStringLiteral("Color Highlighted Line");
    ci.whatsThis = i18n("<p>Sets the background color of the currently active line, which means the line where your cursor is positioned.</p>");
    ci.defaultColor = theme.editorColor(ci.role);
    items.append(ci);

    ci.role = KSyntaxHighlighting::Theme::SearchHighlight;
    ci.name = i18n("Search Highlight");
    ci.key = QStringLiteral("Color Search Highlight");
    ci.whatsThis = i18n("<p>Sets the background color of search results.</p>");
    ci.defaultColor = theme.editorColor(ci.role);
    items.append(ci);

    ci.role = KSyntaxHighlighting::Theme::ReplaceHighlight;
    ci.name = i18n("Replace Highlight");
    ci.key = QStringLiteral("Color Replace Highlight");
    ci.whatsThis = i18n("<p>Sets the background color of replaced text.</p>");
    ci.defaultColor = theme.editorColor(ci.role);
    items.append(ci);

    //
    // icon border
    //
    ci.category = i18n("Icon Border");

    ci.role = KSyntaxHighlighting::Theme::IconBorder;
    ci.name = i18n("Background Area");
    ci.key = QStringLiteral("Color Icon Bar");
    ci.whatsThis = i18n("<p>Sets the background color of the icon border.</p>");
    ci.defaultColor = theme.editorColor(ci.role);
    items.append(ci);

    ci.role = KSyntaxHighlighting::Theme::LineNumbers;
    ci.name = i18n("Line Numbers");
    ci.key = QStringLiteral("Color Line Number");
    ci.whatsThis = i18n("<p>This color will be used to draw the line numbers (if enabled).</p>");
    ci.defaultColor = theme.editorColor(ci.role);
    items.append(ci);

    ci.role = KSyntaxHighlighting::Theme::CurrentLineNumber;
    ci.name = i18n("Current Line Number");
    ci.key = QStringLiteral("Color Current Line Number");
    ci.whatsThis = i18n("<p>This color will be used to draw the number of the current line (if enabled).</p>");
    ci.defaultColor = theme.editorColor(ci.role);
    items.append(ci);

    ci.role = KSyntaxHighlighting::Theme::Separator;
    ci.name = i18n("Separator");
    ci.key = QStringLiteral("Color Separator");
    ci.whatsThis = i18n("<p>This color will be used to draw the line between line numbers and the icon borders, if both are enabled.</p>");
    ci.defaultColor = theme.editorColor(ci.role);
    items.append(ci);

    ci.role = KSyntaxHighlighting::Theme::WordWrapMarker;
    ci.name = i18n("Word Wrap Marker");
    ci.key = QStringLiteral("Color Word Wrap Marker");
    ci.whatsThis = i18n(
        "<p>Sets the color of Word Wrap-related markers:</p><dl><dt>Static Word Wrap</dt><dd>A vertical line which shows the column where text is going to be wrapped</dd><dt>Dynamic Word Wrap</dt><dd>An arrow shown to the left of "
        "visually-wrapped lines</dd></dl>");
    ci.defaultColor = theme.editorColor(ci.role);
    items.append(ci);

    ci.role = KSyntaxHighlighting::Theme::CodeFolding;
    ci.name = i18n("Code Folding");
    ci.key = QStringLiteral("Color Code Folding");
    ci.whatsThis = i18n("<p>Sets the color of the code folding bar.</p>");
    ci.defaultColor = theme.editorColor(ci.role);
    items.append(ci);

    ci.role = KSyntaxHighlighting::Theme::ModifiedLines;
    ci.name = i18n("Modified Lines");
    ci.key = QStringLiteral("Color Modified Lines");
    ci.whatsThis = i18n("<p>Sets the color of the line modification marker for modified lines.</p>");
    ci.defaultColor = theme.editorColor(ci.role);
    items.append(ci);

    ci.role = KSyntaxHighlighting::Theme::SavedLines;
    ci.name = i18n("Saved Lines");
    ci.key = QStringLiteral("Color Saved Lines");
    ci.whatsThis = i18n("<p>Sets the color of the line modification marker for saved lines.</p>");
    ci.defaultColor = theme.editorColor(ci.role);
    items.append(ci);

    //
    // text decorations
    //
    ci.category = i18n("Text Decorations");

    ci.role = KSyntaxHighlighting::Theme::SpellChecking;
    ci.name = i18n("Spelling Mistake Line");
    ci.key = QStringLiteral("Color Spelling Mistake Line");
    ci.whatsThis = i18n("<p>Sets the color of the line that is used to indicate spelling mistakes.</p>");
    ci.defaultColor = theme.editorColor(ci.role);
    items.append(ci);

    ci.role = KSyntaxHighlighting::Theme::TabMarker;
    ci.name = i18n("Tab and Space Markers");
    ci.key = QStringLiteral("Color Tab Marker");
    ci.whatsThis = i18n("<p>Sets the color of the tabulator marks.</p>");
    ci.defaultColor = theme.editorColor(ci.role);
    items.append(ci);

    ci.role = KSyntaxHighlighting::Theme::IndentationLine;
    ci.name = i18n("Indentation Line");
    ci.key = QStringLiteral("Color Indentation Line");
    ci.whatsThis = i18n("<p>Sets the color of the vertical indentation lines.</p>");
    ci.defaultColor = theme.editorColor(ci.role);
    items.append(ci);

    ci.role = KSyntaxHighlighting::Theme::BracketMatching;
    ci.name = i18n("Bracket Highlight");
    ci.key = QStringLiteral("Color Highlighted Bracket");
    ci.whatsThis = i18n("<p>Sets the bracket matching color. This means, if you place the cursor e.g. at a <b>(</b>, the matching <b>)</b> will be highlighted with this color.</p>");
    ci.defaultColor = theme.editorColor(ci.role);
    items.append(ci);

    //
    // marker colors
    //
    ci.category = i18n("Marker Colors");

    const QString markerNames[KSyntaxHighlighting::Theme::MarkError - KSyntaxHighlighting::Theme::MarkBookmark + 1] = {i18n("Bookmark"), i18n("Active Breakpoint"), i18n("Reached Breakpoint"), i18n("Disabled Breakpoint"), i18n("Execution"), i18n("Warning"), i18n("Error")};

    ci.whatsThis = i18n("<p>Sets the background color of mark type.</p><p><b>Note</b>: The marker color is displayed lightly because of transparency.</p>");
    for (int i = 0; i <= KSyntaxHighlighting::Theme::MarkError - KSyntaxHighlighting::Theme::MarkBookmark; ++i) {
        ci.role = static_cast<KSyntaxHighlighting::Theme::EditorColorRole>(i + KSyntaxHighlighting::Theme::MarkBookmark);
        ci.defaultColor = theme.editorColor(ci.role);
        ci.name = markerNames[i];
        ci.key = QLatin1String("Color MarkType ") + QString::number(i + 1);
        items.append(ci);
    }

    //
    // text templates
    //
    ci.category = i18n("Text Templates & Snippets");

    ci.whatsThis = QString(); // TODO: add whatsThis for text templates

    ci.role = KSyntaxHighlighting::Theme::TemplateBackground;
    ci.name = i18n("Background");
    ci.key = QStringLiteral("Color Template Background");
    ci.defaultColor = theme.editorColor(ci.role);
    items.append(ci);

    ci.role = KSyntaxHighlighting::Theme::TemplatePlaceholder;
    ci.name = i18n("Editable Placeholder");
    ci.key = QStringLiteral("Color Template Editable Placeholder");
    ci.defaultColor = theme.editorColor(ci.role);
    items.append(ci);

    ci.role = KSyntaxHighlighting::Theme::TemplateFocusedPlaceholder;
    ci.name = i18n("Focused Editable Placeholder");
    ci.key = QStringLiteral("Color Template Focused Editable Placeholder");
    ci.defaultColor = theme.editorColor(ci.role);
    items.append(ci);

    ci.role = KSyntaxHighlighting::Theme::TemplateReadOnlyPlaceholder;
    ci.name = i18n("Not Editable Placeholder");
    ci.key = QStringLiteral("Color Template Not Editable Placeholder");
    ci.defaultColor = theme.editorColor(ci.role);
    items.append(ci);

    //
    // finally, add all elements
    //
    return items;
}

void KateSchemaConfigColorTab::schemaChanged(const QString &newSchema)
{
    // save current schema
    if (!m_currentSchema.isEmpty()) {
        if (m_schemas.contains(m_currentSchema)) {
            m_schemas.remove(m_currentSchema); // clear this color schema
        }

        // now add it again
        m_schemas[m_currentSchema] = ui->colorItems();
    }

    if (newSchema == m_currentSchema) {
        return;
    }

    // switch
    m_currentSchema = newSchema;

    // If we havent this schema, read in from config file
    if (!m_schemas.contains(newSchema)) {
        KConfigGroup config;
        QVector<KateColorItem> items = readConfig(config, KateHlManager::self()->repository().theme(newSchema));
        m_schemas[newSchema] = items;
    }

    // first block signals otherwise setColor emits changed
    const bool blocked = blockSignals(true);

    ui->clear();
    ui->addColorItems(m_schemas[m_currentSchema]);

    blockSignals(blocked);
}

QVector<KateColorItem> KateSchemaConfigColorTab::readConfig(KConfigGroup &, const KSyntaxHighlighting::Theme &theme)
{
    QVector<KateColorItem> items = colorItemList(theme);
    for (int i = 0; i < items.count(); ++i) {
        KateColorItem &item(items[i]);
        item.color = theme.editorColor(item.role);
    }
    return items;
}

void KateSchemaConfigColorTab::importSchema(KConfigGroup &config)
{
    m_schemas[m_currentSchema] = readConfig(config, KSyntaxHighlighting::Theme());

    // first block signals otherwise setColor emits changed
    const bool blocked = blockSignals(true);

    ui->clear();
    ui->addColorItems(m_schemas[m_currentSchema]);

    blockSignals(blocked);
}

QJsonObject KateSchemaConfigColorTab::exportJson() const
{
    static const auto idx = KSyntaxHighlighting::Theme::staticMetaObject.indexOfEnumerator("EditorColorRole");
    Q_ASSERT(idx >= 0);
    const auto metaEnum = KSyntaxHighlighting::Theme::staticMetaObject.enumerator(idx);
    QJsonObject colors;
    const QVector<KateColorItem> items = ui->colorItems();
    for (const KateColorItem &item : items) {
        QColor c = item.useDefault ? item.defaultColor : item.color;
        colors[QLatin1String(metaEnum.key(item.role))] = c.name();
    }
    return colors;
}

void KateSchemaConfigColorTab::apply()
{
    schemaChanged(m_currentSchema);

#if 0 // FIXME-THEME
    for (auto it = m_schemas.constBegin(); it != m_schemas.constEnd(); ++it) {
        KConfigGroup config = KTextEditor::EditorPrivate::self()->schemaManager()->schema(it.key());
        for (const KateColorItem &item : it.value()) {
            if (item.useDefault) {
                config.deleteEntry(item.key);
            } else {
                config.writeEntry(item.key, item.color);
            }
        }

        // add dummy entry to prevent the config group from being empty.
        // As if the group is empty, KateSchemaManager will not find it anymore.
        config.writeEntry("dummy", "prevent-empty-group");
    }
#endif
    // all colors are written, so throw away all cached schemas
    m_schemas.clear();
}

void KateSchemaConfigColorTab::reload()
{
    // drop all cached data
    m_schemas.clear();

    // load from config
    KConfigGroup config;
    QVector<KateColorItem> items = readConfig(config, KateHlManager::self()->repository().theme(m_currentSchema));

    // first block signals otherwise setColor emits changed
    const bool blocked = blockSignals(true);

    ui->clear();
    ui->addColorItems(items);

    blockSignals(blocked);
}

QColor KateSchemaConfigColorTab::backgroundColor() const
{
    return ui->findColor(QStringLiteral("Color Background"));
}

QColor KateSchemaConfigColorTab::selectionColor() const
{
    return ui->findColor(QStringLiteral("Color Selection"));
}
// END KateSchemaConfigColorTab

// BEGIN FontColorConfig -- 'Normal Text Styles' tab
KateSchemaConfigDefaultStylesTab::KateSchemaConfigDefaultStylesTab(KateSchemaConfigColorTab *colorTab)
{
    m_colorTab = colorTab;

    // size management
    QGridLayout *grid = new QGridLayout(this);

    m_defaultStyles = new KateStyleTreeWidget(this);
    connect(m_defaultStyles, SIGNAL(changed()), this, SIGNAL(changed()));
    grid->addWidget(m_defaultStyles, 0, 0);

    m_defaultStyles->setWhatsThis(
        i18n("<p>This list displays the default styles for the current color theme and "
             "offers the means to edit them. The style name reflects the current "
             "style settings.</p>"
             "<p>To edit the colors, click the colored squares, or select the color "
             "to edit from the popup menu.</p><p>You can unset the Background and Selected "
             "Background colors from the popup menu when appropriate.</p>"));
}

KateSchemaConfigDefaultStylesTab::~KateSchemaConfigDefaultStylesTab()
{
    qDeleteAll(m_defaultStyleLists);
}

KateAttributeList *KateSchemaConfigDefaultStylesTab::attributeList(const QString &schema)
{
    if (!m_defaultStyleLists.contains(schema)) {
        // get list of all default styles
        KateAttributeList *list = new KateAttributeList();
        const KSyntaxHighlighting::Theme currentTheme = KateHlManager::self()->repository().theme(schema);
        for (int z = 0; z < defaultStyleCount(); z++) {
            KTextEditor::Attribute::Ptr i(new KTextEditor::Attribute());
            const auto style = defaultStyleToTextStyle(static_cast<KTextEditor::DefaultStyle>(z));

            if (const auto col = currentTheme.textColor(style)) {
                i->setForeground(QColor(col));
            }

            if (const auto col = currentTheme.selectedTextColor(style)) {
                i->setSelectedForeground(QColor(col));
            }

            if (const auto col = currentTheme.backgroundColor(style)) {
                i->setBackground(QColor(col));
            } else {
                i->clearBackground();
            }

            if (const auto col = currentTheme.selectedBackgroundColor(style)) {
                i->setSelectedBackground(QColor(col));
            } else {
                i->clearProperty(SelectedBackground);
            }

            i->setFontBold(currentTheme.isBold(style));
            i->setFontItalic(currentTheme.isItalic(style));
            i->setFontUnderline(currentTheme.isUnderline(style));
            i->setFontStrikeOut(currentTheme.isStrikeThrough(style));
            list->append(i);
        }
        m_defaultStyleLists.insert(schema, list);
    }

    return m_defaultStyleLists[schema];
}

void KateSchemaConfigDefaultStylesTab::schemaChanged(const QString &schema)
{
    m_currentSchema = schema;

    m_defaultStyles->clear();

    KateAttributeList *l = attributeList(schema);
    updateColorPalette(l->at(0)->foreground().color());

    // normal text and source code
    QTreeWidgetItem *parent = new QTreeWidgetItem(m_defaultStyles, QStringList() << i18nc("@item:intable", "Normal Text & Source Code"));
    parent->setFirstColumnSpanned(true);
    for (int i = (int)KTextEditor::dsNormal; i <= (int)KTextEditor::dsAttribute; ++i) {
        m_defaultStyles->addItem(parent, defaultStyleName(i), l->at(i));
    }

    // Number, Types & Constants
    parent = new QTreeWidgetItem(m_defaultStyles, QStringList() << i18nc("@item:intable", "Numbers, Types & Constants"));
    parent->setFirstColumnSpanned(true);
    for (int i = (int)KTextEditor::dsDataType; i <= (int)KTextEditor::dsConstant; ++i) {
        m_defaultStyles->addItem(parent, defaultStyleName(i), l->at(i));
    }

    // strings & characters
    parent = new QTreeWidgetItem(m_defaultStyles, QStringList() << i18nc("@item:intable", "Strings & Characters"));
    parent->setFirstColumnSpanned(true);
    for (int i = (int)KTextEditor::dsChar; i <= (int)KTextEditor::dsImport; ++i) {
        m_defaultStyles->addItem(parent, defaultStyleName(i), l->at(i));
    }

    // comments & documentation
    parent = new QTreeWidgetItem(m_defaultStyles, QStringList() << i18nc("@item:intable", "Comments & Documentation"));
    parent->setFirstColumnSpanned(true);
    for (int i = (int)KTextEditor::dsComment; i <= (int)KTextEditor::dsAlert; ++i) {
        m_defaultStyles->addItem(parent, defaultStyleName(i), l->at(i));
    }

    // Misc
    parent = new QTreeWidgetItem(m_defaultStyles, QStringList() << i18nc("@item:intable", "Miscellaneous"));
    parent->setFirstColumnSpanned(true);
    for (int i = (int)KTextEditor::dsOthers; i <= (int)KTextEditor::dsError; ++i) {
        m_defaultStyles->addItem(parent, defaultStyleName(i), l->at(i));
    }

    m_defaultStyles->expandAll();
}

void KateSchemaConfigDefaultStylesTab::updateColorPalette(const QColor &textColor)
{
    QPalette p(m_defaultStyles->palette());
    p.setColor(QPalette::Base, m_colorTab->backgroundColor());
    p.setColor(QPalette::Highlight, m_colorTab->selectionColor());
    p.setColor(QPalette::Text, textColor);
    m_defaultStyles->setPalette(p);
}

void KateSchemaConfigDefaultStylesTab::reload()
{
    m_defaultStyles->clear();
    qDeleteAll(m_defaultStyleLists);
    m_defaultStyleLists.clear();

    schemaChanged(m_currentSchema);
}

void KateSchemaConfigDefaultStylesTab::apply()
{
#if 0 // FIXME-THEME
    QHashIterator<QString, KateAttributeList *> it = m_defaultStyleLists;
    while (it.hasNext()) {
        it.next();
        KateHlManager::self()->setDefaults(it.key(), *it.value());
    }
#endif
}

void KateSchemaConfigDefaultStylesTab::importSchema(const QString &schemaName, const QString &schema, KConfig *cfg)
{
#if 0 // FIXME-THEME
    KateHlManager::self()->getDefaults(schemaName, *(m_defaultStyleLists[schema]), cfg);
#endif
}

QJsonObject KateSchemaConfigDefaultStylesTab::exportJson(const QString &schema) const
{
    static const auto idx = KSyntaxHighlighting::Theme::staticMetaObject.indexOfEnumerator("TextStyle");
    Q_ASSERT(idx >= 0);
    const auto metaEnum = KSyntaxHighlighting::Theme::staticMetaObject.enumerator(idx);
    QJsonObject styles;
    for (int z = 0; z < defaultStyleCount(); z++) {
        QJsonObject style;
        KTextEditor::Attribute::Ptr p = m_defaultStyleLists[schema]->at(z);
        if (p->hasProperty(QTextFormat::ForegroundBrush)) {
            style[QLatin1String("text-color")] = p->foreground().color().name();
        }
        if (p->hasProperty(QTextFormat::BackgroundBrush)) {
            style[QLatin1String("background-color")] = p->background().color().name();
        }
        if (p->hasProperty(SelectedForeground)) {
            style[QLatin1String("selected-text-color")] = p->selectedForeground().color().name();
        }
        if (p->hasProperty(SelectedBackground)) {
            style[QLatin1String("selected-background-color")] = p->selectedBackground().color().name();
        }
        if (p->hasProperty(QTextFormat::FontWeight) && p->fontBold()) {
            style[QLatin1String("bold")] = true;
        }
        if (p->hasProperty(QTextFormat::FontItalic) && p->fontItalic()) {
            style[QLatin1String("italic")] = true;
        }
        if (p->hasProperty(QTextFormat::TextUnderlineStyle) && p->fontUnderline()) {
            style[QLatin1String("underline")] = true;
        }
        if (p->hasProperty(QTextFormat::FontStrikeOut) && p->fontStrikeOut()) {
            style[QLatin1String("strike-through")] = true;
        }
        styles[QLatin1String(metaEnum.key(defaultStyleToTextStyle(static_cast<KTextEditor::DefaultStyle>(z))))] = style;
    }
    return styles;
}

void KateSchemaConfigDefaultStylesTab::showEvent(QShowEvent *event)
{
    if (!event->spontaneous() && !m_currentSchema.isEmpty()) {
        KateAttributeList *l = attributeList(m_currentSchema);
        Q_ASSERT(l != nullptr);
        updateColorPalette(l->at(0)->foreground().color());
    }

    QWidget::showEvent(event);
}
// END FontColorConfig

// BEGIN KateSchemaConfigHighlightTab -- 'Highlighting Text Styles' tab
KateSchemaConfigHighlightTab::KateSchemaConfigHighlightTab(KateSchemaConfigDefaultStylesTab *page, KateSchemaConfigColorTab *colorTab)
{
    m_defaults = page;
    m_colorTab = colorTab;

    m_hl = 0;

    QVBoxLayout *layout = new QVBoxLayout(this);

    QHBoxLayout *headerLayout = new QHBoxLayout;
    layout->addLayout(headerLayout);

    QLabel *lHl = new QLabel(i18n("H&ighlight:"), this);
    headerLayout->addWidget(lHl);

    hlCombo = new KComboBox(this);
    hlCombo->setEditable(false);
    headerLayout->addWidget(hlCombo);

    lHl->setBuddy(hlCombo);
    connect(hlCombo, SIGNAL(activated(int)), this, SLOT(hlChanged(int)));

    QPushButton *btnexport = new QPushButton(i18n("Export..."), this);
    headerLayout->addWidget(btnexport);
    connect(btnexport, SIGNAL(clicked()), this, SLOT(exportHl()));

    QPushButton *btnimport = new QPushButton(i18n("Import..."), this);
    headerLayout->addWidget(btnimport);
    connect(btnimport, SIGNAL(clicked()), this, SLOT(importHl()));

    headerLayout->addStretch();

    for (const auto &hl : KateHlManager::self()->modeList()) {
        const auto section = hl.translatedSection();
        if (!section.isEmpty()) {
            hlCombo->addItem(section + QLatin1Char('/') + hl.translatedName());
        } else {
            hlCombo->addItem(hl.translatedName());
        }
    }
    hlCombo->setCurrentIndex(0);

    // styles listview
    m_styles = new KateStyleTreeWidget(this, true);
    connect(m_styles, SIGNAL(changed()), this, SIGNAL(changed()));
    layout->addWidget(m_styles, 999);

    // get current highlighting from the host application
    int hl = 0;
    KTextEditor::ViewPrivate *kv = qobject_cast<KTextEditor::ViewPrivate *>(KTextEditor::EditorPrivate::self()->application()->activeMainWindow()->activeView());
    if (kv) {
        const QString hlName = kv->doc()->highlight()->name();
        hl = KateHlManager::self()->nameFind(hlName);
        Q_ASSERT(hl >= 0);
    }

    hlCombo->setCurrentIndex(hl);
    hlChanged(hl);

    m_styles->setWhatsThis(
        i18n("<p>This list displays the contexts of the current syntax highlight mode and "
             "offers the means to edit them. The context name reflects the current "
             "style settings.</p><p>To edit using the keyboard, press "
             "<strong>&lt;SPACE&gt;</strong> and choose a property from the popup menu.</p>"
             "<p>To edit the colors, click the colored squares, or select the color "
             "to edit from the popup menu.</p><p>You can unset the Background and Selected "
             "Background colors from the context menu when appropriate.</p>"));
}

KateSchemaConfigHighlightTab::~KateSchemaConfigHighlightTab()
{
}

void KateSchemaConfigHighlightTab::hlChanged(int z)
{
    m_hl = z;
    schemaChanged(m_schema);
}

void KateSchemaConfigHighlightTab::schemaChanged(const QString &schema)
{
    m_schema = schema;

    m_styles->clear();

    if (!m_hlDict.contains(m_schema)) {
        m_hlDict.insert(schema, QHash<int, QVector<KTextEditor::Attribute::Ptr>>());
    }

    if (!m_hlDict[m_schema].contains(m_hl)) {
        m_hlDict[m_schema].insert(m_hl, KateHlManager::self()->getHl(m_hl)->attributesForDefinition(m_schema));
    }

    KateAttributeList *l = m_defaults->attributeList(schema);

    // Set listview colors
    updateColorPalette(l->at(0)->foreground().color());

    QHash<QString, QTreeWidgetItem *> prefixes;
    QVector<KTextEditor::Attribute::Ptr>::ConstIterator it = m_hlDict[m_schema][m_hl].constBegin();
    while (it != m_hlDict[m_schema][m_hl].constEnd()) {
        const KTextEditor::Attribute::Ptr itemData = *it;
        Q_ASSERT(itemData);

        // All stylenames have their language mode prefixed, e.g. HTML:Comment
        // split them and put them into nice substructures.
        int c = itemData->name().indexOf(QLatin1Char(':'));
        if (c > 0) {
            QString prefix = itemData->name().left(c);
            QString name = itemData->name().mid(c + 1);

            QTreeWidgetItem *parent = prefixes[prefix];
            if (!parent) {
                parent = new QTreeWidgetItem(m_styles, QStringList() << prefix);
                m_styles->expandItem(parent);
                prefixes.insert(prefix, parent);
            }
            m_styles->addItem(parent, name, l->at(itemData->defaultStyle()), itemData);
        } else {
            m_styles->addItem(itemData->name(), l->at(itemData->defaultStyle()), itemData);
        }
        ++it;
    }

    m_styles->resizeColumns();
}

void KateSchemaConfigHighlightTab::updateColorPalette(const QColor &textColor)
{
    QPalette p(m_styles->palette());
    p.setColor(QPalette::Base, m_colorTab->backgroundColor());
    p.setColor(QPalette::Highlight, m_colorTab->selectionColor());
    p.setColor(QPalette::Text, textColor);
    m_styles->setPalette(p);
}

void KateSchemaConfigHighlightTab::reload()
{
    m_styles->clear();

    m_hlDict.clear();

    hlChanged(hlCombo->currentIndex());
}

void KateSchemaConfigHighlightTab::apply()
{
#if 0 // FIXME-THEME
    QMutableHashIterator<QString, QHash<int, QVector<KTextEditor::Attribute::Ptr>>> it = m_hlDict;
    while (it.hasNext()) {
        it.next();
        QMutableHashIterator<int, QVector<KTextEditor::Attribute::Ptr>> it2 = it.value();
        while (it2.hasNext()) {
            it2.next();
            KateHlManager::self()->getHl(it2.key())->setKateExtendedAttributeList(it.key(), it2.value());
        }
    }
#endif
}

QList<int> KateSchemaConfigHighlightTab::hlsForSchema(const QString &schema)
{
    return m_hlDict[schema].keys();
}

void KateSchemaConfigHighlightTab::importHl(const QString &fromSchemaName, QString schema, int hl, KConfig *cfg)
{
#if 0 // FIXME-THEME
    QString schemaNameForLoading(fromSchemaName);
    QString hlName;
    bool doManage = (cfg == nullptr);
    if (schema.isEmpty()) {
        schema = m_schema;
    }



    if (doManage) {
        QString srcName =
            QFileDialog::getOpenFileName(this, i18n("Importing colors for single highlighting"), KateHlManager::self()->getHl(hl)->name() + QLatin1String(".katehlcolor"), QStringLiteral("%1 (*.katehlcolor)").arg(i18n("Kate color schema")));

        if (srcName.isEmpty()) {
            return;
        }

        cfg = new KConfig(srcName, KConfig::SimpleConfig);
        KConfigGroup grp(cfg, "KateHLColors");
        hlName = grp.readEntry("highlight", QString());
        schemaNameForLoading = grp.readEntry("schema", QString());
        if ((grp.readEntry("full schema", "true").toUpper() != QLatin1String("FALSE")) || hlName.isEmpty() || schemaNameForLoading.isEmpty()) {
            // ERROR - file format
            KMessageBox::information(this, i18n("File is not a single highlighting color file"), i18n("Fileformat error"));
            hl = -1;
            schemaNameForLoading = QString();
        } else {
            hl = KateHlManager::self()->nameFind(hlName);
            if (hl == -1) {
                // hl not found
                KMessageBox::information(this, i18n("The selected file contains colors for a non existing highlighting:%1", hlName), i18n("Import failure"));
                hl = -1;
                schemaNameForLoading = QString();
            }
        }
    }

    if ((hl != -1) && (!schemaNameForLoading.isEmpty())) {
        QVector<KTextEditor::Attribute::Ptr> list;
        KateHlManager::self()->getHl(hl)->getKateExtendedAttributeListCopy(schemaNameForLoading, list, cfg);
        KateHlManager::self()->getHl(hl)->setKateExtendedAttributeList(schema, list);
        m_hlDict[schema].insert(hl, list);
    }

    if (cfg && doManage) {
        apply();
        delete cfg;
        cfg = nullptr;
        if ((hl != -1) && (!schemaNameForLoading.isEmpty())) {
            hlChanged(m_hl);
            KMessageBox::information(this, i18n("Colors have been imported for highlighting: %1", hlName), i18n("Import has finished"));
        }
    }
#endif
}

void KateSchemaConfigHighlightTab::exportHl(QString schema, int hl, KConfig *cfg)
{
#if 0 // FIXME-THEME
    bool doManage = (cfg == nullptr);
    if (schema.isEmpty()) {
        schema = m_schema;
    }
    if (hl == -1) {
        hl = m_hl;
    }

    QVector<KTextEditor::Attribute::Ptr> items = m_hlDict[schema][hl];
    if (doManage) {
        QString destName = QFileDialog::getSaveFileName(this,
                                                        i18n("Exporting colors for single highlighting: %1", KateHlManager::self()->getHl(hl)->name()),
                                                        KateHlManager::self()->getHl(hl)->name() + QLatin1String(".katehlcolor"),
                                                        QStringLiteral("%1 (*.katehlcolor)").arg(i18n("Kate color schema")));

        if (destName.isEmpty()) {
            return;
        }

        cfg = new KConfig(destName, KConfig::SimpleConfig);
        KConfigGroup grp(cfg, "KateHLColors");
        grp.writeEntry("highlight", KateHlManager::self()->getHl(hl)->name());
        grp.writeEntry("schema", schema);
        grp.writeEntry("full schema", "false");
    }
    KateHlManager::self()->getHl(hl)->setKateExtendedAttributeList(schema, items, cfg, doManage);

    if (doManage) {
        cfg->sync();
        delete cfg;
    }
#endif
}

void KateSchemaConfigHighlightTab::showEvent(QShowEvent *event)
{
    if (!event->spontaneous()) {
        KateAttributeList *l = m_defaults->attributeList(m_schema);
        Q_ASSERT(l != nullptr);
        updateColorPalette(l->at(0)->foreground().color());
    }

    QWidget::showEvent(event);
}
// END KateSchemaConfigHighlightTab

// BEGIN KateSchemaConfigPage -- Main dialog page
KateSchemaConfigPage::KateSchemaConfigPage(QWidget *parent)
    : KateConfigPage(parent)
    , m_currentSchema(-1)
{
    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);

    // header
    QHBoxLayout *headerLayout = new QHBoxLayout;
    layout->addLayout(headerLayout);

    QLabel *lHl = new QLabel(i18n("&Theme:"), this);
    headerLayout->addWidget(lHl);

    schemaCombo = new KComboBox(this);
    schemaCombo->setEditable(false);
    lHl->setBuddy(schemaCombo);
    headerLayout->addWidget(schemaCombo);
    connect(schemaCombo, SIGNAL(currentIndexChanged(int)), this, SLOT(comboBoxIndexChanged(int)));

    QPushButton *btnnew = new QPushButton(i18n("&New..."), this);
    headerLayout->addWidget(btnnew);
    connect(btnnew, SIGNAL(clicked()), this, SLOT(newSchema()));

    btndel = new QPushButton(i18n("&Delete"), this);
    headerLayout->addWidget(btndel);
    connect(btndel, SIGNAL(clicked()), this, SLOT(deleteSchema()));

    QPushButton *btnexport = new QPushButton(i18n("Export..."), this);
    headerLayout->addWidget(btnexport);
    connect(btnexport, SIGNAL(clicked()), this, SLOT(exportFullSchema()));

    QPushButton *btnimport = new QPushButton(i18n("Import..."), this);
    headerLayout->addWidget(btnimport);
    connect(btnimport, SIGNAL(clicked()), this, SLOT(importFullSchema()));

    headerLayout->addStretch();

    // tabs
    QTabWidget *tabWidget = new QTabWidget(this);
    layout->addWidget(tabWidget);

    m_colorTab = new KateSchemaConfigColorTab();
    tabWidget->addTab(m_colorTab, i18n("Colors"));
    connect(m_colorTab, SIGNAL(changed()), SLOT(slotChanged()));

    m_defaultStylesTab = new KateSchemaConfigDefaultStylesTab(m_colorTab);
    tabWidget->addTab(m_defaultStylesTab, i18n("Default Text Styles"));
    connect(m_defaultStylesTab, SIGNAL(changed()), SLOT(slotChanged()));

    m_highlightTab = new KateSchemaConfigHighlightTab(m_defaultStylesTab, m_colorTab);
    tabWidget->addTab(m_highlightTab, i18n("Highlighting Text Styles"));
    connect(m_highlightTab, SIGNAL(changed()), SLOT(slotChanged()));

    QHBoxLayout *footLayout = new QHBoxLayout;
    layout->addLayout(footLayout);

    lHl = new QLabel(i18n("&Default theme for %1:", QCoreApplication::applicationName()), this);
    footLayout->addWidget(lHl);

    defaultSchemaCombo = new KComboBox(this);
    footLayout->addWidget(defaultSchemaCombo);
    defaultSchemaCombo->setEditable(false);
    lHl->setBuddy(defaultSchemaCombo);

    reload();

    connect(defaultSchemaCombo, SIGNAL(currentIndexChanged(int)), this, SLOT(slotChanged()));
}

KateSchemaConfigPage::~KateSchemaConfigPage()
{
}

void KateSchemaConfigPage::exportFullSchema()
{
    // get save destination
    const QString currentSchemaName = m_currentSchema;
    const QString destName = QFileDialog::getSaveFileName(this, i18n("Exporting color schema: %1", currentSchemaName), currentSchemaName + QLatin1String(".kateschema"), QStringLiteral("%1 (*.kateschema *.theme)").arg(i18n("Color theme")));
    if (destName.isEmpty()) {
        return;
    }

    // if we have the .theme ending, export some JSON for KSyntaxHighlighting
    if (destName.endsWith(QLatin1String(".theme"), Qt::CaseInsensitive)) {
        // export to json format
        QJsonObject theme;
        QJsonObject metaData;
        metaData[QLatin1String("revision")] = 1;
        metaData[QLatin1String("name")] = currentSchemaName;
        theme[QLatin1String("metadata")] = metaData;
        theme[QLatin1String("editor-colors")] = m_colorTab->exportJson();
        theme[QLatin1String("text-styles")] = m_defaultStylesTab->exportJson(m_currentSchema);

        // write to file
        QFile saveFile(destName);
        if (!saveFile.open(QIODevice::WriteOnly)) {
            return;
        }
        saveFile.write(QJsonDocument(theme).toJson());
        return;
    }

    // open config file
    KConfig cfg(destName, KConfig::SimpleConfig);

#if 0 // FIXME-THEME OLD EXPORT
    //
    // export editor Colors (background, ...)
    //
    KConfigGroup colorConfigGroup(&cfg, "Editor Colors");
    m_colorTab->exportSchema(colorConfigGroup);

    //
    // export Default Styles
    //
    m_defaultStylesTab->exportSchema(m_currentSchema, &cfg);

    //
    // export Highlighting Text Styles
    //
    // force a load of all Highlighting Text Styles
    QStringList hlList;
    m_highlightTab->loadAllHlsForSchema(m_currentSchema);

    const QList<int> hls = m_highlightTab->hlsForSchema(m_currentSchema);

    int cnt = 0;
    QProgressDialog progress(i18n("Exporting schema"), QString(), 0, hls.count(), this);
    progress.setWindowModality(Qt::WindowModal);
    for (int hl : hls) {
        hlList << KateHlManager::self()->getHl(hl)->name();
        m_highlightTab->exportHl(m_currentSchema, hl, &cfg);
        progress.setValue(++cnt);
        if (progress.wasCanceled()) {
            break;
        }
    }
    progress.setValue(hls.count());

    KConfigGroup grp(&cfg, "KateSchema");
    grp.writeEntry("full schema", "true");
    grp.writeEntry("highlightings", hlList);
    grp.writeEntry("schema", currentSchemaName);
    cfg.sync();

#endif
}

QString KateSchemaConfigPage::requestSchemaName(const QString &suggestedName)
{
    QString schemaName = suggestedName;

    bool reask = true;
    do {
        QDialog howToImportDialog(this);
        Ui_KateHowToImportSchema howToImport;

        QVBoxLayout *mainLayout = new QVBoxLayout;
        howToImportDialog.setLayout(mainLayout);

        QWidget *w = new QWidget(&howToImportDialog);
        mainLayout->addWidget(w);
        howToImport.setupUi(w);

        QDialogButtonBox *buttons = new QDialogButtonBox(&howToImportDialog);
        mainLayout->addWidget(buttons);

        QPushButton *okButton = new QPushButton;
        okButton->setDefault(true);
        KGuiItem::assign(okButton, KStandardGuiItem::ok());
        buttons->addButton(okButton, QDialogButtonBox::AcceptRole);
        connect(okButton, SIGNAL(clicked()), &howToImportDialog, SLOT(accept()));

        QPushButton *cancelButton = new QPushButton;
        KGuiItem::assign(cancelButton, KStandardGuiItem::cancel());
        buttons->addButton(cancelButton, QDialogButtonBox::RejectRole);
        connect(cancelButton, SIGNAL(clicked()), &howToImportDialog, SLOT(reject()));
        //

        // if schema exists, prepare option to replace
        if (KateHlManager::self()->repository().theme(schemaName).isValid()) {
            howToImport.radioReplaceExisting->show();
            howToImport.radioReplaceExisting->setText(i18n("Replace existing theme %1", schemaName));
            howToImport.radioReplaceExisting->setChecked(true);
        } else {
            howToImport.radioReplaceExisting->hide();
            howToImport.newName->setText(schemaName);
        }

        // cancel pressed?
        if (howToImportDialog.exec() == QDialog::Rejected) {
            schemaName.clear();
            reask = false;
        }
        // check what the user wants
        else {
            // replace existing
            if (howToImport.radioReplaceExisting->isChecked()) {
                reask = false;
            }
            // replace current
            else if (howToImport.radioReplaceCurrent->isChecked()) {
                schemaName = m_currentSchema;
                reask = false;
            }
            // new one, check again, whether the schema already exists
            else if (howToImport.radioAsNew->isChecked()) {
                schemaName = howToImport.newName->text();
                if (KateHlManager::self()->repository().theme(schemaName).isValid()) {
                    reask = true;
                } else {
                    reask = false;
                }
            }
            // should never happen
            else {
                reask = true;
            }
        }
    } while (reask);

    return schemaName;
}

void KateSchemaConfigPage::importFullSchema()
{
#if 0 // FIXME-THEME
    const QString srcName = QFileDialog::getOpenFileName(this, i18n("Importing Color Schema"), QString(), QStringLiteral("%1 (*.kateschema)").arg(i18n("Kate color schema")));

    if (srcName.isEmpty()) {
        return;
    }

    // carete config + sanity check for full color schema
    KConfig cfg(srcName, KConfig::SimpleConfig);
    KConfigGroup schemaGroup(&cfg, "KateSchema");
    if (schemaGroup.readEntry("full schema", "false").toUpper() != QLatin1String("TRUE")) {
        KMessageBox::sorry(this, i18n("The file does not contain a full color schema."), i18n("Fileformat error"));
        return;
    }

    // read color schema name
    const QStringList highlightings = schemaGroup.readEntry("highlightings", QStringList());
    const QString fromSchemaName = schemaGroup.readEntry("schema", i18n("Name unspecified"));

    // request valid schema name
    const QString schemaName = requestSchemaName(fromSchemaName);
    if (schemaName.isEmpty()) {
        return;
    }

    // if the schema already exists, select it in the combo box
    if (schemaCombo->findData(schemaName) != -1) {
        schemaCombo->setCurrentIndex(schemaCombo->findData(schemaName));
    } else { // it is really a new schema, easy meat :-)
        newSchema(schemaName);
    }

    // make sure the correct schema is activated
    schemaChanged(schemaName);

    // Finally, the correct schema is activated.
    // Next,  start importing.

    //
    // import editor Colors (background, ...)
    //
    KConfigGroup colorConfigGroup(&cfg, "Editor Colors");
    m_colorTab->importSchema(colorConfigGroup);

    //
    // import Default Styles
    //
    m_defaultStylesTab->importSchema(fromSchemaName, schemaName, &cfg);

    //
    // import all Highlighting Text Styles
    //
    // create mapping from highlighting name to internal id
    const int hlCount = KateHlManager::self()->modeList().size();
    QHash<QString, int> nameToId;
    for (int i = 0; i < hlCount; ++i) {
        nameToId.insert(KateHlManager::self()->modeList().at(i).name(), i);
    }

    // may take some time, as we have > 200 highlightings
    int cnt = 0;
    QProgressDialog progress(i18n("Importing schema"), QString(), 0, highlightings.count(), this);
    progress.setWindowModality(Qt::WindowModal);
    for (const QString &hl : highlightings) {
        if (nameToId.contains(hl)) {
            const int i = nameToId[hl];
            m_highlightTab->importHl(fromSchemaName, schemaName, i, &cfg);
        }
        progress.setValue(++cnt);
    }
    progress.setValue(highlightings.count());
#endif
}

void KateSchemaConfigPage::apply()
{
    // remember name + index
    const QString schemaName = schemaCombo->itemData(schemaCombo->currentIndex()).toString();

    // first apply all tabs
    m_colorTab->apply();
    m_defaultStylesTab->apply();
    m_highlightTab->apply();

    // reload themes DB & clear all attributes
    KateHlManager::self()->reload();
    for (int i = 0; i < KateHlManager::self()->modeList().size(); ++i) {
        KateHlManager::self()->getHl(i)->clearAttributeArrays();
    }

    // than reload the whole stuff, special handle auto selection == empty theme name
    const auto defaultTheme = defaultSchemaCombo->itemData(defaultSchemaCombo->currentIndex()).toString();
    if (defaultTheme.isEmpty()) {
        KateRendererConfig::global()->setValue(KateRendererConfig::AutoColorThemeSelection, true);
    } else {
        KateRendererConfig::global()->setValue(KateRendererConfig::AutoColorThemeSelection, false);
        KateRendererConfig::global()->setSchema(defaultTheme);
    }
    KateRendererConfig::global()->reloadSchema();

    // KateSchemaManager::update() sorts the schema alphabetically, hence the
    // schema indexes change. Thus, repopulate the schema list...
    refillCombos(schemaCombo->itemData(schemaCombo->currentIndex()).toString(), defaultSchemaCombo->itemData(defaultSchemaCombo->currentIndex()).toString());
    schemaChanged(schemaName);
}

void KateSchemaConfigPage::reload()
{
    // reinitialize combo boxes
    refillCombos(KateRendererConfig::global()->schema(), KateRendererConfig::global()->schema());

    // finally, activate the current schema again
    schemaChanged(schemaCombo->itemData(schemaCombo->currentIndex()).toString());

    // all tabs need to reload to discard all the cached data, as the index
    // mapping may have changed
    m_colorTab->reload();
    m_defaultStylesTab->reload();
    m_highlightTab->reload();
}

void KateSchemaConfigPage::refillCombos(const QString &schemaName, const QString &defaultSchemaName)
{
    schemaCombo->blockSignals(true);
    defaultSchemaCombo->blockSignals(true);

    // reinitialize combo boxes
    schemaCombo->clear();
    defaultSchemaCombo->clear();
    defaultSchemaCombo->addItem(i18n("Automatic Selection"), QString());
    const auto themes = KateHlManager::self()->sortedThemes();
    for (const auto &theme : themes) {
        schemaCombo->addItem(theme.translatedName(), theme.name());
        defaultSchemaCombo->addItem(theme.translatedName(), theme.name());
    }

    // set the correct indexes again, fallback to always existing default theme
    int schemaIndex = schemaCombo->findData(schemaName);
    if (schemaIndex == -1) {
        schemaIndex = schemaCombo->findData(KTextEditor::EditorPrivate::self()->hlManager()->repository().defaultTheme(KSyntaxHighlighting::Repository::LightTheme).name());
    }

    // set the correct indexes again, fallback to auto-selection
    int defaultSchemaIndex = 0;
    if (!KateRendererConfig::global()->value(KateRendererConfig::AutoColorThemeSelection).toBool()) {
        defaultSchemaIndex = defaultSchemaCombo->findData(defaultSchemaName);
        if (defaultSchemaIndex == -1) {
            defaultSchemaIndex = 0;
        }
    }

    Q_ASSERT(schemaIndex != -1);
    Q_ASSERT(defaultSchemaIndex != -1);

    defaultSchemaCombo->setCurrentIndex(defaultSchemaIndex);
    schemaCombo->setCurrentIndex(schemaIndex);

    schemaCombo->blockSignals(false);
    defaultSchemaCombo->blockSignals(false);
}

void KateSchemaConfigPage::reset()
{
    reload();
}

void KateSchemaConfigPage::defaults()
{
    reload();
}

void KateSchemaConfigPage::deleteSchema()
{
    const int comboIndex = schemaCombo->currentIndex();
    const QString schemaNameToDelete = schemaCombo->itemData(comboIndex).toString();

    // KSyntaxHighlighting themes can not be deleted, skip invalid themes, too
    const auto theme = KateHlManager::self()->repository().theme(schemaNameToDelete);
    if (!theme.isValid() || theme.isReadOnly()) {
        return;
    }

    // ask the user again, this can't be undone
    if (KMessageBox::warningContinueCancel(this, i18n("Do you really want to delete the theme \"%1\"? This can not be undone.").arg(schemaNameToDelete),
                                                     i18n("Possible Data Loss"),
                                                     KGuiItem(i18n("Delete Nevertheless")),
                                                     KStandardGuiItem::cancel()) != KMessageBox::Continue) {
        return;
    }

    // purge the theme file
    QFile::remove(theme.filePath());

    // reset syntax manager repo to flush deleted theme
    KateHlManager::self()->reload();

    // fallback to Default schema + auto
    schemaCombo->setCurrentIndex(schemaCombo->findData(QVariant(KTextEditor::EditorPrivate::self()->hlManager()->repository().defaultTheme(KSyntaxHighlighting::Repository::LightTheme).name())));
    if (defaultSchemaCombo->currentIndex() == defaultSchemaCombo->findData(schemaNameToDelete)) {
        defaultSchemaCombo->setCurrentIndex(0);
    }

    // remove schema from combo box
    schemaCombo->removeItem(comboIndex);
    defaultSchemaCombo->removeItem(comboIndex);

    // Reload the color tab, since it uses cached schemas
    m_colorTab->reload();
}

bool KateSchemaConfigPage::newSchema()
{
    // location to write theme files to:
    const QString themesPath = QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation) + QStringLiteral("/org.kde.syntax-highlighting/themes");

    // get sane name
    QString schemaName;
    QString themeFileName;
    while (schemaName.isEmpty()) {
        bool ok = false;
        schemaName = QInputDialog::getText(this, i18n("Name for New Theme"), i18n("Name:"), QLineEdit::Normal, i18n("New Theme"), &ok);
        if (!ok) {
            return false;
        }

        // try if schema already around => if yes, retry name input
        // we try for duplicated file names, too
        themeFileName = themesPath + QStringLiteral("/") + schemaName + QStringLiteral(".theme");
        if (KateHlManager::self()->repository().theme(schemaName).isValid() || QFile::exists(themeFileName)) {
            KMessageBox::information(this, i18n("<p>The theme \"%1\" already exists.</p><p>Please choose a different theme name.</p>", schemaName), i18n("New Theme"));
            schemaName.clear();
        }
    }

    // get current theme data as template
    const QString currentThemeName = schemaCombo->itemData(schemaCombo->currentIndex()).toString();
    const auto currentTheme = KateHlManager::self()->repository().theme(currentThemeName);

    // load json content, shall work, as the theme as valid, but still abort on errors
    QFile loadFile(currentTheme.filePath());
    if (!loadFile.open(QIODevice::ReadOnly)) {
        return false;
    }
    const QByteArray jsonData = loadFile.readAll();
    QJsonParseError parseError;
    QJsonDocument jsonDoc = QJsonDocument::fromJson(jsonData, &parseError);
    if (parseError.error != QJsonParseError::NoError) {
        return false;
    }

    qDebug() << "lala";

    // patch new name into theme object
    QJsonObject newThemeObject = jsonDoc.object();
    QJsonObject metaData;
    metaData[QLatin1String("revision")] = 1;
    metaData[QLatin1String("name")] = schemaName;
    newThemeObject[QLatin1String("metadata")] = metaData;

    // write to new theme file, we might need to create the local dir first
    // keep saveFile in own scope, we need to ensure it is flushed to disk before the reload below happens
    {
        QDir().mkpath(themesPath);
        QFile saveFile(themeFileName);
        if (!saveFile.open(QIODevice::WriteOnly)) {
            return false;
        }
        saveFile.write(QJsonDocument(newThemeObject).toJson());
    }


    qDebug() << "lala" << themeFileName;

    // reset syntax manager repo to find new theme
    KateHlManager::self()->reload();

    // append items to combo boxes
    schemaCombo->addItem(schemaName, QVariant(schemaName));
    defaultSchemaCombo->addItem(schemaName, QVariant(schemaName));

    // finally, activate new schema (last item in the list)
    schemaCombo->setCurrentIndex(schemaCombo->count() - 1);
    return true;
}

void KateSchemaConfigPage::schemaChanged(const QString &schema)
{
    // we can't delete read-only themes, e.g. the stuff shipped inside Qt resources or system wide installed
    btndel->setEnabled(!KateHlManager::self()->repository().theme(schema).isReadOnly());

    // propagate changed schema to all tabs
    m_colorTab->schemaChanged(schema);
    m_defaultStylesTab->schemaChanged(schema);
    m_highlightTab->schemaChanged(schema);

    // save current schema index
    m_currentSchema = schema;
}

void KateSchemaConfigPage::comboBoxIndexChanged(int currentIndex)
{
    schemaChanged(schemaCombo->itemData(currentIndex).toString());
}

QString KateSchemaConfigPage::name() const
{
    return i18n("Color Themes");
}

QString KateSchemaConfigPage::fullName() const
{
    return i18n("Color Themes");
}

QIcon KateSchemaConfigPage::icon() const
{
    return QIcon::fromTheme(QStringLiteral("preferences-desktop-color"));
}

// END KateSchemaConfigPage
