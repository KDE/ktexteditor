/*
    SPDX-FileCopyrightText: 2007, 2008 Matthew Woehlke <mw_triad@users.sourceforge.net>
    SPDX-FileCopyrightText: 2001-2003 Christoph Cullmann <cullmann@kde.org>
    SPDX-FileCopyrightText: 2002, 2003 Anders Lund <anders.lund@lund.tdcadsl.dk>
    SPDX-FileCopyrightText: 2012-2018 Dominik Haumann <dhaumann@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

// BEGIN Includes
#include "katethemeconfig.h"

#include "katecolortreewidget.h"
#include "kateconfig.h"
#include "katedocument.h"
#include "kateglobal.h"
#include "katehighlight.h"
#include "katepartdebug.h"
#include "katerenderer.h"
#include "katestyletreewidget.h"
#include "kateview.h"

#include <KComboBox>
#include <KConfigGroup>
#include <KLocalizedString>
#include <KMessageBox>
#include <KMessageWidget>

#include <QFileDialog>
#include <QInputDialog>
#include <QLabel>
#include <QMetaEnum>
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

/**
 * Helper to get json object for given valid theme.
 * @param theme theme to get json object for
 * @return json object of theme
 */
static QJsonObject jsonForTheme(const KSyntaxHighlighting::Theme &theme)
{
    // load json content, shall work, as the theme as valid, but still abort on errors
    QFile loadFile(theme.filePath());
    if (!loadFile.open(QIODevice::ReadOnly)) {
        return QJsonObject();
    }
    const QByteArray jsonData = loadFile.readAll();
    QJsonParseError parseError;
    QJsonDocument jsonDoc = QJsonDocument::fromJson(jsonData, &parseError);
    if (parseError.error != QJsonParseError::NoError) {
        return QJsonObject();
    }
    return jsonDoc.object();
}

/**
 * Helper to write json data to given file path.
 * After the function returns, stuff is flushed to disk for sure.
 * @param json json object with theme data
 * @param themeFileName file name to write to
 * @return did writing succeed?
 */
static bool writeJson(const QJsonObject &json, const QString &themeFileName)
{
    QFile saveFile(themeFileName);
    if (!saveFile.open(QIODevice::WriteOnly)) {
        return false;
    }
    saveFile.write(QJsonDocument(json).toJson());
    return true;
}

// BEGIN KateThemeConfigColorTab -- 'Colors' tab
KateThemeConfigColorTab::KateThemeConfigColorTab()
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

QVector<KateColorItem> KateThemeConfigColorTab::colorItemList(const KSyntaxHighlighting::Theme &theme) const
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

    const QString markerNames[KSyntaxHighlighting::Theme::MarkError - KSyntaxHighlighting::Theme::MarkBookmark + 1] = {
        i18n("Bookmark"), i18n("Active Breakpoint"), i18n("Reached Breakpoint"), i18n("Disabled Breakpoint"), i18n("Execution"), i18n("Warning"), i18n("Error")};

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

void KateThemeConfigColorTab::schemaChanged(const QString &newSchema)
{
    // ensure invalid or read-only stuff can't be changed
    const auto theme = KateHlManager::self()->repository().theme(newSchema);
    setDisabled(!theme.isValid() || theme.isReadOnly());

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
        QVector<KateColorItem> items = colorItemList(theme);
        for (int i = 0; i < items.count(); ++i) {
            KateColorItem &item(items[i]);
            item.color = theme.editorColor(item.role);
        }
        m_schemas[newSchema] = items;
    }

    // first block signals otherwise setColor emits changed
    const bool blocked = blockSignals(true);

    ui->clear();
    ui->addColorItems(m_schemas[m_currentSchema]);

    blockSignals(blocked);
}

void KateThemeConfigColorTab::apply()
{
    schemaChanged(m_currentSchema);

    // we use meta-data of enum for computing the json keys
    static const auto idx = KSyntaxHighlighting::Theme::staticMetaObject.indexOfEnumerator("EditorColorRole");
    Q_ASSERT(idx >= 0);
    const auto metaEnum = KSyntaxHighlighting::Theme::staticMetaObject.enumerator(idx);

    // export all themes we cached data for
    for (auto it = m_schemas.constBegin(); it != m_schemas.constEnd(); ++it) {
        // get theme for key, skip invalid or read-only themes for writing
        const auto theme = KateHlManager::self()->repository().theme(it.key());
        if (!theme.isValid() || theme.isReadOnly()) {
            continue;
        }

        // get current theme data from disk
        QJsonObject newThemeObject = jsonForTheme(theme);

        // patch the editor-colors part
        QJsonObject colors;
        for (const KateColorItem &item : it.value()) {
            QColor c = item.useDefault ? item.defaultColor : item.color;
            colors[QLatin1String(metaEnum.key(item.role))] = c.name();
        }
        newThemeObject[QLatin1String("editor-colors")] = colors;

        // write json back to file
        writeJson(newThemeObject, theme.filePath());
    }

    // all colors are written, so throw away all cached schemas
    m_schemas.clear();
}

void KateThemeConfigColorTab::reload()
{
    // drop all cached data
    m_schemas.clear();

    // trigger re-creation of ui from theme
    const auto backupName = m_currentSchema;
    m_currentSchema.clear();
    schemaChanged(backupName);
}

QColor KateThemeConfigColorTab::backgroundColor() const
{
    return ui->findColor(QStringLiteral("Color Background"));
}

QColor KateThemeConfigColorTab::selectionColor() const
{
    return ui->findColor(QStringLiteral("Color Selection"));
}
// END KateThemeConfigColorTab

// BEGIN FontColorConfig -- 'Normal Text Styles' tab
KateThemeConfigDefaultStylesTab::KateThemeConfigDefaultStylesTab(KateThemeConfigColorTab *colorTab)
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

KateThemeConfigDefaultStylesTab::~KateThemeConfigDefaultStylesTab()
{
    qDeleteAll(m_defaultStyleLists);
}

KateAttributeList *KateThemeConfigDefaultStylesTab::attributeList(const QString &schema)
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

void KateThemeConfigDefaultStylesTab::schemaChanged(const QString &schema)
{
    // ensure invalid or read-only stuff can't be changed
    const auto theme = KateHlManager::self()->repository().theme(schema);
    setDisabled(!theme.isValid() || theme.isReadOnly());

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

void KateThemeConfigDefaultStylesTab::updateColorPalette(const QColor &textColor)
{
    QPalette p(m_defaultStyles->palette());
    p.setColor(QPalette::Base, m_colorTab->backgroundColor());
    p.setColor(QPalette::Highlight, m_colorTab->selectionColor());
    p.setColor(QPalette::Text, textColor);
    m_defaultStyles->setPalette(p);
}

void KateThemeConfigDefaultStylesTab::reload()
{
    m_defaultStyles->clear();
    qDeleteAll(m_defaultStyleLists);
    m_defaultStyleLists.clear();

    schemaChanged(m_currentSchema);
}

void KateThemeConfigDefaultStylesTab::apply()
{
    // get enum meta data for json keys
    static const auto idx = KSyntaxHighlighting::Theme::staticMetaObject.indexOfEnumerator("TextStyle");
    Q_ASSERT(idx >= 0);
    const auto metaEnum = KSyntaxHighlighting::Theme::staticMetaObject.enumerator(idx);

    // export all configured styles of the cached themes
    QHashIterator<QString, KateAttributeList *> it(m_defaultStyleLists);
    while (it.hasNext()) {
        it.next();

        // get theme for key, skip invalid or read-only themes for writing
        const auto theme = KateHlManager::self()->repository().theme(it.key());
        if (!theme.isValid() || theme.isReadOnly()) {
            continue;
        }

        // get current theme data from disk
        QJsonObject newThemeObject = jsonForTheme(theme);

        // patch the text-styles part
        QJsonObject styles;
        for (int z = 0; z < defaultStyleCount(); z++) {
            QJsonObject style;
            KTextEditor::Attribute::Ptr p = it.value()->at(z);
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
        newThemeObject[QLatin1String("text-styles")] = styles;

        // write json back to file
        writeJson(newThemeObject, theme.filePath());
    }
}

void KateThemeConfigDefaultStylesTab::showEvent(QShowEvent *event)
{
    if (!event->spontaneous() && !m_currentSchema.isEmpty()) {
        KateAttributeList *l = attributeList(m_currentSchema);
        Q_ASSERT(l != nullptr);
        updateColorPalette(l->at(0)->foreground().color());
    }

    QWidget::showEvent(event);
}
// END FontColorConfig

// BEGIN KateThemeConfigHighlightTab -- 'Highlighting Text Styles' tab
KateThemeConfigHighlightTab::KateThemeConfigHighlightTab(KateThemeConfigDefaultStylesTab *page, KateThemeConfigColorTab *colorTab)
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

KateThemeConfigHighlightTab::~KateThemeConfigHighlightTab()
{
}

void KateThemeConfigHighlightTab::hlChanged(int z)
{
    m_hl = z;
    schemaChanged(m_schema);
}

/**
 * Helper to get the "default attributes" for the given schema + highlighting.
 * This means all stuff set without taking theme overrides for the highlighting into account.
 */
static KateAttributeList defaultsForHighlighting(const std::vector<KSyntaxHighlighting::Format> &formats, const KateAttributeList &defaultStyleAttributes)
{
    const KSyntaxHighlighting::Theme invalidTheme;
    KateAttributeList defaults;
    for (const auto &format : formats) {
        // create a KTextEditor attribute matching the default style for this format
        // use the default style attribute we got passed to have the one we currently have configured in the settings here
        KTextEditor::Attribute::Ptr newAttribute(new KTextEditor::Attribute(*defaultStyleAttributes.at(textStyleToDefaultStyle(format.textStyle()))));

        // check for override => if yes, set attribute as overridden, use invalid theme to avoid the usage of theme override!

        if (format.hasTextColorOverride()) {
            newAttribute->setForeground(format.textColor(invalidTheme));
        }
        if (format.hasBackgroundColorOverride()) {
            newAttribute->setBackground(format.backgroundColor(invalidTheme));
        }
        if (format.hasSelectedTextColorOverride()) {
            newAttribute->setSelectedForeground(format.selectedTextColor(invalidTheme));
        }
        if (format.hasSelectedBackgroundColorOverride()) {
            newAttribute->setSelectedBackground(format.selectedBackgroundColor(invalidTheme));
        }
        if (format.hasBoldOverride()) {
            newAttribute->setFontBold(format.isBold(invalidTheme));
        }
        if (format.hasItalicOverride()) {
            newAttribute->setFontItalic(format.isItalic(invalidTheme));
        }
        if (format.hasUnderlineOverride()) {
            newAttribute->setFontUnderline(format.isUnderline(invalidTheme));
        }
        if (format.hasStrikeThroughOverride()) {
            newAttribute->setFontStrikeOut(format.isStrikeThrough(invalidTheme));
        }

        // not really relevant, set it as configured
        newAttribute->setSkipSpellChecking(format.spellCheck());
        defaults.append(newAttribute);
    }
    return defaults;
}

void KateThemeConfigHighlightTab::schemaChanged(const QString &schema)
{
    // ensure invalid or read-only stuff can't be changed
    const auto theme = KateHlManager::self()->repository().theme(schema);
    setDisabled(!theme.isValid() || theme.isReadOnly());

    m_schema = schema;

    m_styles->clear();

    if (!m_hlDict.contains(m_schema)) {
        m_hlDict.insert(schema, QHash<int, QVector<KTextEditor::Attribute::Ptr>>());
    }

    // Set listview colors
    KateAttributeList *l = m_defaults->attributeList(schema);
    updateColorPalette(l->at(0)->foreground().color());

    // create unified stuff
    auto attributes = KateHlManager::self()->getHl(m_hl)->attributesForDefinition(m_schema);
    auto formats = KateHlManager::self()->getHl(m_hl)->formats();
    auto defaults = defaultsForHighlighting(formats, *l);

    for (int i = 0; i < attributes.size(); ++i) {
        // All stylenames have their language mode prefixed, e.g. HTML:Comment
        // split them and put them into nice substructures.
        int c = attributes[i]->name().indexOf(QLatin1Char(':'));
        if (c <= 0)
            continue;

        QString highlighting = attributes[i]->name().left(c);
        QString name = attributes[i]->name().mid(c + 1);
        auto &uniqueAttribute = m_uniqueAttributes[m_schema][highlighting][name].first;
        auto &uniqueAttributeDefault = m_uniqueAttributes[m_schema][highlighting][name].second;

        if (uniqueAttribute.data())
            attributes[i] = uniqueAttribute;
        else
            uniqueAttribute = attributes[i];

        if (uniqueAttributeDefault.data())
            defaults[i] = uniqueAttributeDefault;
        else
            uniqueAttributeDefault = defaults[i];
    }

    if (!m_hlDict[m_schema].contains(m_hl)) {
        m_hlDict[m_schema].insert(m_hl, attributes);
    }

    // None can't be changed with the current way
    // TODO: removed it from the list?
    m_styles->setDisabled(m_hl == 0);

    QHash<QString, QTreeWidgetItem *> prefixes;
    QVector<KTextEditor::Attribute::Ptr>::ConstIterator it = m_hlDict[m_schema][m_hl].constBegin();
    int i = 0;
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
            m_styles->addItem(parent, name, defaults.at(i), itemData);
        } else {
            m_styles->addItem(itemData->name(), defaults.at(i), itemData);
        }
        ++it;
        ++i;
    }

    m_styles->resizeColumns();
}

void KateThemeConfigHighlightTab::updateColorPalette(const QColor &textColor)
{
    QPalette p(m_styles->palette());
    p.setColor(QPalette::Base, m_colorTab->backgroundColor());
    p.setColor(QPalette::Highlight, m_colorTab->selectionColor());
    p.setColor(QPalette::Text, textColor);
    m_styles->setPalette(p);
}

void KateThemeConfigHighlightTab::reload()
{
    m_styles->clear();

    m_hlDict.clear();
    m_uniqueAttributes.clear();

    hlChanged(hlCombo->currentIndex());
}

void KateThemeConfigHighlightTab::apply()
{
    // handle all cached themes data
    for (const auto &themeIt : m_uniqueAttributes) {
        // get theme for key, skip invalid or read-only themes for writing
        const auto theme = KateHlManager::self()->repository().theme(themeIt.first);
        if (!theme.isValid() || theme.isReadOnly()) {
            continue;
        }

        // get current theme data from disk
        QJsonObject newThemeObject = jsonForTheme(theme);

        // look at all highlightings we have info stored
        QJsonObject overrides;
        for (const auto &highlightingIt : themeIt.second) {
            const QString definitionName = highlightingIt.first;
            QJsonObject styles;
            for (const auto &attributeIt : highlightingIt.second) {
                QJsonObject style;
                KTextEditor::Attribute::Ptr p = attributeIt.second.first;
                KTextEditor::Attribute::Ptr pDefault = attributeIt.second.second;
                if (p->hasProperty(QTextFormat::ForegroundBrush) && p->foreground().color() != pDefault->foreground().color()) {
                    style[QLatin1String("text-color")] = p->foreground().color().name();
                }
                if (p->hasProperty(QTextFormat::BackgroundBrush) && p->background().color() != pDefault->background().color()) {
                    style[QLatin1String("background-color")] = p->background().color().name();
                }
                if (p->hasProperty(SelectedForeground) && p->selectedForeground().color() != pDefault->selectedForeground().color()) {
                    style[QLatin1String("selected-text-color")] = p->selectedForeground().color().name();
                }
                if (p->hasProperty(SelectedBackground) && p->selectedBackground().color() != pDefault->selectedBackground().color()) {
                    style[QLatin1String("selected-background-color")] = p->selectedBackground().color().name();
                }
                if (p->hasProperty(QTextFormat::FontWeight) && p->fontBold() != pDefault->fontBold()) {
                    style[QLatin1String("bold")] = p->fontBold();
                }
                if (p->hasProperty(QTextFormat::FontItalic) && p->fontItalic() != pDefault->fontItalic()) {
                    style[QLatin1String("italic")] = p->fontItalic();
                }
                if (p->hasProperty(QTextFormat::TextUnderlineStyle) && p->fontUnderline() != pDefault->fontUnderline()) {
                    style[QLatin1String("underline")] = p->fontUnderline();
                }
                if (p->hasProperty(QTextFormat::FontStrikeOut) && p->fontStrikeOut() != pDefault->fontStrikeOut()) {
                    style[QLatin1String("strike-through")] = p->fontStrikeOut();
                }

                // ensure we skip empty overrides
                if (!style.isEmpty()) {
                    styles[attributeIt.first] = style;
                }
            }

            // ensure we skip empty overrides
            if (!styles.isEmpty()) {
                overrides[definitionName] = styles;
            }
        }

        // we set even empty overrides, to ensure we overwrite stuff!
        newThemeObject[QLatin1String("custom-styles")] = overrides;

        // write json back to file
        writeJson(newThemeObject, theme.filePath());
    }
}

QList<int> KateThemeConfigHighlightTab::hlsForSchema(const QString &schema)
{
    return m_hlDict[schema].keys();
}

void KateThemeConfigHighlightTab::showEvent(QShowEvent *event)
{
    if (!event->spontaneous()) {
        KateAttributeList *l = m_defaults->attributeList(m_schema);
        Q_ASSERT(l != nullptr);
        updateColorPalette(l->at(0)->foreground().color());
    }

    QWidget::showEvent(event);
}
// END KateThemeConfigHighlightTab

// BEGIN KateThemeConfigPage -- Main dialog page
KateThemeConfigPage::KateThemeConfigPage(QWidget *parent)
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

    QPushButton *copyButton = new QPushButton(i18n("&Copy..."), this);
    headerLayout->addWidget(copyButton);
    connect(copyButton, SIGNAL(clicked()), this, SLOT(copyTheme()));

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

    // label to inform about read-only state
    m_readOnlyThemeLabel = new KMessageWidget(i18n("Bundled read-only theme. To modify the theme, please copy it."), this);
    m_readOnlyThemeLabel->setCloseButtonVisible(false);
    m_readOnlyThemeLabel->setMessageType(KMessageWidget::Information);
    m_readOnlyThemeLabel->hide();
    layout->addWidget(m_readOnlyThemeLabel);

    // tabs
    QTabWidget *tabWidget = new QTabWidget(this);
    layout->addWidget(tabWidget);

    m_colorTab = new KateThemeConfigColorTab();
    tabWidget->addTab(m_colorTab, i18n("Colors"));
    connect(m_colorTab, SIGNAL(changed()), SLOT(slotChanged()));

    m_defaultStylesTab = new KateThemeConfigDefaultStylesTab(m_colorTab);
    tabWidget->addTab(m_defaultStylesTab, i18n("Default Text Styles"));
    connect(m_defaultStylesTab, SIGNAL(changed()), SLOT(slotChanged()));

    m_highlightTab = new KateThemeConfigHighlightTab(m_defaultStylesTab, m_colorTab);
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

void KateThemeConfigPage::exportFullSchema()
{
    // get save destination
    const QString currentSchemaName = m_currentSchema;
    const QString destName = QFileDialog::getSaveFileName(this, i18n("Exporting color theme: %1", currentSchemaName), currentSchemaName + QLatin1String(".theme"), QStringLiteral("%1 (*.theme)").arg(i18n("Color theme")));
    if (destName.isEmpty()) {
        return;
    }

    // get current theme
    const QString currentThemeName = schemaCombo->itemData(schemaCombo->currentIndex()).toString();
    const auto currentTheme = KateHlManager::self()->repository().theme(currentThemeName);

    // export is easy, just copy the file 1:1
    QFile::copy(currentTheme.filePath(), destName);
}

void KateThemeConfigPage::importFullSchema()
{
    const QString srcName = QFileDialog::getOpenFileName(this, i18n("Importing Color Theme"), QString(), QStringLiteral("%1 (*.theme)").arg(i18n("Color theme")));
    if (srcName.isEmpty()) {
        return;
    }

    // location to write theme files to
    const QString themesPath = QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation) + QStringLiteral("/org.kde.syntax-highlighting/themes");

    // construct file name for imported theme
    const QString themesFullFileName = themesPath + QStringLiteral("/") + QFileInfo(srcName).fileName();

    // if something might be overwritten, as the user
    if (QFile::exists(themesFullFileName)) {
        if (KMessageBox::warningContinueCancel(
                this, i18n("Importing will overwrite the existing theme file \"%1\"? This can not be undone.").arg(themesFullFileName), i18n("Possible Data Loss"), KGuiItem(i18n("Import Nevertheless")), KStandardGuiItem::cancel()) !=
            KMessageBox::Continue) {
            return;
        }
    }

    // copy theme file, we might need to create the local dir first
    QDir().mkpath(themesPath);
    QFile::copy(srcName, themesFullFileName);

    // reload themes DB & clear all attributes
    KateHlManager::self()->reload();
    for (int i = 0; i < KateHlManager::self()->modeList().size(); ++i) {
        KateHlManager::self()->getHl(i)->clearAttributeArrays();
    }

    // KateThemeManager::update() sorts the schema alphabetically, hence the
    // schema indexes change. Thus, repopulate the schema list...
    refillCombos(schemaCombo->itemData(schemaCombo->currentIndex()).toString(), defaultSchemaCombo->itemData(defaultSchemaCombo->currentIndex()).toString());
}

void KateThemeConfigPage::apply()
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

    // KateThemeManager::update() sorts the schema alphabetically, hence the
    // schema indexes change. Thus, repopulate the schema list...
    refillCombos(schemaCombo->itemData(schemaCombo->currentIndex()).toString(), defaultSchemaCombo->itemData(defaultSchemaCombo->currentIndex()).toString());
    schemaChanged(schemaName);
}

void KateThemeConfigPage::reload()
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

void KateThemeConfigPage::refillCombos(const QString &schemaName, const QString &defaultSchemaName)
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

void KateThemeConfigPage::reset()
{
    // reload themes DB & clear all attributes
    KateHlManager::self()->reload();
    for (int i = 0; i < KateHlManager::self()->modeList().size(); ++i) {
        KateHlManager::self()->getHl(i)->clearAttributeArrays();
    }

    // reload the view
    reload();
}

void KateThemeConfigPage::defaults()
{
    reset();
}

void KateThemeConfigPage::deleteSchema()
{
    const int comboIndex = schemaCombo->currentIndex();
    const QString schemaNameToDelete = schemaCombo->itemData(comboIndex).toString();

    // KSyntaxHighlighting themes can not be deleted, skip invalid themes, too
    const auto theme = KateHlManager::self()->repository().theme(schemaNameToDelete);
    if (!theme.isValid() || theme.isReadOnly()) {
        return;
    }

    // ask the user again, this can't be undone
    if (KMessageBox::warningContinueCancel(
            this, i18n("Do you really want to delete the theme \"%1\"? This can not be undone.").arg(schemaNameToDelete), i18n("Possible Data Loss"), KGuiItem(i18n("Delete Nevertheless")), KStandardGuiItem::cancel()) !=
        KMessageBox::Continue) {
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

bool KateThemeConfigPage::copyTheme()
{
    // get current theme data as template
    const QString currentThemeName = schemaCombo->itemData(schemaCombo->currentIndex()).toString();
    const auto currentTheme = KateHlManager::self()->repository().theme(currentThemeName);

    // location to write theme files to
    const QString themesPath = QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation) + QStringLiteral("/org.kde.syntax-highlighting/themes");

    // get sane name
    QString schemaName;
    QString themeFileName;
    while (schemaName.isEmpty()) {
        QInputDialog newNameDialog(this);
        newNameDialog.setInputMode(QInputDialog::TextInput);
        newNameDialog.setWindowTitle(i18n("Copy theme"));
        newNameDialog.setLabelText(i18n("Name for copy of color theme \"%1\":").arg(currentThemeName));
        if (newNameDialog.exec() == QDialog::Rejected) {
            return false;
        }
        schemaName = newNameDialog.textValue();

        // try if schema already around => if yes, retry name input
        // we try for duplicated file names, too
        themeFileName = themesPath + QStringLiteral("/") + schemaName + QStringLiteral(".theme");
        if (KateHlManager::self()->repository().theme(schemaName).isValid() || QFile::exists(themeFileName)) {
            KMessageBox::information(this, i18n("<p>The theme \"%1\" already exists.</p><p>Please choose a different theme name.</p>", schemaName), i18n("Copy Theme"));
            schemaName.clear();
        }
    }

    // get json for current theme
    QJsonObject newThemeObject = jsonForTheme(currentTheme);
    QJsonObject metaData;
    metaData[QLatin1String("revision")] = 1;
    metaData[QLatin1String("name")] = schemaName;
    newThemeObject[QLatin1String("metadata")] = metaData;

    // write to new theme file, we might need to create the local dir first
    QDir().mkpath(themesPath);
    if (!writeJson(newThemeObject, themeFileName)) {
        return false;
    }

    // reset syntax manager repo to find new theme
    KateHlManager::self()->reload();

    // append items to combo boxes
    schemaCombo->addItem(schemaName, QVariant(schemaName));
    defaultSchemaCombo->addItem(schemaName, QVariant(schemaName));

    // finally, activate new schema (last item in the list)
    schemaCombo->setCurrentIndex(schemaCombo->count() - 1);
    return true;
}

void KateThemeConfigPage::schemaChanged(const QString &schema)
{
    // we can't delete read-only themes, e.g. the stuff shipped inside Qt resources or system wide installed
    const auto theme = KateHlManager::self()->repository().theme(schema);
    btndel->setEnabled(!theme.isReadOnly());
    m_readOnlyThemeLabel->setVisible(theme.isReadOnly());

    // propagate changed schema to all tabs
    m_colorTab->schemaChanged(schema);
    m_defaultStylesTab->schemaChanged(schema);
    m_highlightTab->schemaChanged(schema);

    // save current schema index
    m_currentSchema = schema;
}

void KateThemeConfigPage::comboBoxIndexChanged(int currentIndex)
{
    schemaChanged(schemaCombo->itemData(currentIndex).toString());
}

QString KateThemeConfigPage::name() const
{
    return i18n("Color Themes");
}

QString KateThemeConfigPage::fullName() const
{
    return i18n("Color Themes");
}

QIcon KateThemeConfigPage::icon() const
{
    return QIcon::fromTheme(QStringLiteral("preferences-desktop-color"));
}

// END KateThemeConfigPage
