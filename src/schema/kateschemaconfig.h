/*
    SPDX-FileCopyrightText: 2001-2003 Christoph Cullmann <cullmann@kde.org>
    SPDX-FileCopyrightText: 2002, 2003 Anders Lund <anders.lund@lund.tdcadsl.dk>
    SPDX-FileCopyrightText: 2012-2018 Dominik Haumann <dhaumann@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef KATE_SCHEMA_CONFIG_H
#define KATE_SCHEMA_CONFIG_H

#include "katecolortreewidget.h"
#include "katedialogs.h"
#include "kateextendedattribute.h"
#include "kateschema.h"

#include <QFont>
#include <QJsonObject>
#include <QMap>

class KateStyleTreeWidget;
class KComboBox;

class KateSchemaConfigColorTab : public QWidget
{
    Q_OBJECT

public:
    KateSchemaConfigColorTab();
    ~KateSchemaConfigColorTab();

    QColor backgroundColor() const;
    QColor selectionColor() const;

    QJsonObject exportJson() const;

public Q_SLOTS:
    void apply();
    void reload();
    void schemaChanged(const QString &newSchema);

    void importSchema(KConfigGroup &config);
    void exportSchema(KConfigGroup &config);

Q_SIGNALS:
    void changed();

private:
    QVector<KateColorItem> colorItemList(const KSyntaxHighlighting::Theme &theme) const;
    QVector<KateColorItem> readConfig(KConfigGroup &config, const KSyntaxHighlighting::Theme &theme);

private:
    // multiple shemas may be edited. Hence, we need one ColorList for each schema
    QMap<QString, QVector<KateColorItem>> m_schemas;
    QString m_currentSchema;

    KateColorTreeWidget *ui;
};

class KateSchemaConfigDefaultStylesTab : public QWidget
{
    Q_OBJECT

public:
    explicit KateSchemaConfigDefaultStylesTab(KateSchemaConfigColorTab *colorTab);
    ~KateSchemaConfigDefaultStylesTab() override;

Q_SIGNALS:
    void changed();

public:
    void schemaChanged(const QString &schema);
    void reload();
    void apply();

    KateAttributeList *attributeList(const QString &schema);
    void exportSchema(const QString &schema, KConfig *cfg);
    void importSchema(const QString &schemaName, const QString &schema, KConfig *cfg);

    QJsonObject exportJson(const QString &schema) const;

protected:
    void showEvent(QShowEvent *event) override;
    void updateColorPalette(const QColor &textColor);

private:
    KateStyleTreeWidget *m_defaultStyles;
    QHash<QString, KateAttributeList *> m_defaultStyleLists;
    KateSchemaConfigColorTab *m_colorTab;
    QString m_currentSchema;
};

class KateSchemaConfigHighlightTab : public QWidget
{
    Q_OBJECT

public:
    explicit KateSchemaConfigHighlightTab(KateSchemaConfigDefaultStylesTab *page, KateSchemaConfigColorTab *colorTab);
    ~KateSchemaConfigHighlightTab() override;

    void schemaChanged(const QString &schema);
    void reload();
    void apply();

Q_SIGNALS:
    void changed();

protected Q_SLOTS:
    void hlChanged(int z);

public Q_SLOTS:
    void exportHl(QString schema = QString(), int hl = -1, KConfig *cfg = nullptr);
    void importHl(const QString &fromSchemaName = QString(), QString schema = QString(), int hl = -1, KConfig *cfg = nullptr);

protected:
    void showEvent(QShowEvent *event) override;
    void updateColorPalette(const QColor &textColor);

private:
    KateSchemaConfigDefaultStylesTab *m_defaults;
    KateSchemaConfigColorTab *m_colorTab;

    KComboBox *hlCombo;
    KateStyleTreeWidget *m_styles;

    QString m_schema;
    int m_hl;

    QHash<QString, QHash<int, QVector<KTextEditor::Attribute::Ptr>>> m_hlDict;

public:
    QList<int> hlsForSchema(const QString &schema);
};

class KateSchemaConfigPage : public KateConfigPage
{
    Q_OBJECT

public:
    explicit KateSchemaConfigPage(QWidget *parent);
    ~KateSchemaConfigPage() override;
    QString name() const override;
    QString fullName() const override;
    QIcon icon() const override;

public Q_SLOTS:
    void apply() override;
    void reload() override;
    void reset() override;
    void defaults() override;
    void exportFullSchema();
    void importFullSchema();

private Q_SLOTS:
    void deleteSchema();
    bool newSchema(const QString &newName = QString());
    void schemaChanged(const QString &schema);
    void comboBoxIndexChanged(int currentIndex);

private:
    void refillCombos(const QString &schemaName, const QString &defaultSchemaName);
    QString requestSchemaName(const QString &suggestedName);

private:
    QString m_currentSchema;

    class QPushButton *btndel;
    class KComboBox *defaultSchemaCombo;
    class KComboBox *schemaCombo;
    KateSchemaConfigColorTab *m_colorTab;
    KateSchemaConfigDefaultStylesTab *m_defaultStylesTab;
    KateSchemaConfigHighlightTab *m_highlightTab;
};

#endif
