/*
    SPDX-FileCopyrightText: 2008 Paul Giannaros <paul@giannaros.org>
    SPDX-FileCopyrightText: 2009-2018 Dominik Haumann <dhaumann@kde.org>
    SPDX-FileCopyrightText: 2010 Joseph Wenninger <jowenn@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "katescript.h"
#include "katedocument.h"
#include "katepartdebug.h"
#include "katescriptdocument.h"
#include "katescripteditor.h"
#include "katescripthelpers.h"
#include "katescriptview.h"
#include "kateview.h"

#include <KLocalizedString>
#include <iostream>

#include <QFile>
#include <QFileInfo>
#include <QJSEngine>
#include <QQmlEngine>

KateScript::KateScript(const QString &urlOrScript, enum InputType inputType)
    : m_url(inputType == InputURL ? urlOrScript : QString())
    , m_inputType(inputType)
    , m_script(inputType == InputSCRIPT ? urlOrScript : QString())
{
}

KateScript::~KateScript()
{
    if (m_loadSuccessful) {
        // remove data...
        delete m_editor;
        delete m_document;
        delete m_view;
        delete m_engine;
    }
}

QString KateScript::backtrace(const QJSValue &error, const QString &header)
{
    QString bt;
    if (!header.isNull()) {
        bt += header + QLatin1String(":\n");
    }
    if (error.isError()) {
        bt += error.toString() + QLatin1Char('\n');
    }

    return bt;
}

void KateScript::displayBacktrace(const QJSValue &error, const QString &header)
{
    if (!m_engine) {
        std::cerr << "KateScript::displayBacktrace: no engine, cannot display error\n";
        return;
    }
    std::cerr << "\033[31m" << qPrintable(backtrace(error, header)) << "\033[0m" << '\n';
}

void KateScript::clearExceptions()
{
    if (!load()) {
        return;
    }
}

QJSValue KateScript::global(const QString &name)
{
    // load the script if necessary
    if (!load()) {
        return QJSValue::UndefinedValue;
    }
    return m_engine->globalObject().property(name);
}

QJSValue KateScript::function(const QString &name)
{
    QJSValue value = global(name);
    if (!value.isCallable()) {
        return QJSValue::UndefinedValue;
    }
    return value;
}

bool KateScript::load()
{
    if (m_loaded) {
        return m_loadSuccessful;
    }

    m_loaded = true;
    m_loadSuccessful = false; // here set to false, and at end of function to true

    // read the script file into memory
    QString source;
    if (m_inputType == InputURL) {
        if (!Kate::Script::readFile(m_url, source)) {
            return false;
        }
    } else {
        source = m_script;
    }

    // create script engine, register meta types
    m_engine = new QJSEngine();

    // export read & require function and add the require guard object
    QJSValue functions = m_engine->newQObject(new Kate::ScriptHelper(m_engine));
    m_engine->globalObject().setProperty(QStringLiteral("functions"), functions);
    m_engine->globalObject().setProperty(QStringLiteral("read"), functions.property(QStringLiteral("read")));
    m_engine->globalObject().setProperty(QStringLiteral("require"), functions.property(QStringLiteral("require")));
    m_engine->globalObject().setProperty(QStringLiteral("require_guard"), m_engine->newObject());

    // export debug function
    m_engine->globalObject().setProperty(QStringLiteral("debug"), functions.property(QStringLiteral("debug")));

    // export translation functions
    m_engine->globalObject().setProperty(QStringLiteral("i18n"), functions.property(QStringLiteral("_i18n")));
    m_engine->globalObject().setProperty(QStringLiteral("i18nc"), functions.property(QStringLiteral("_i18nc")));
    m_engine->globalObject().setProperty(QStringLiteral("i18np"), functions.property(QStringLiteral("_i18np")));
    m_engine->globalObject().setProperty(QStringLiteral("i18ncp"), functions.property(QStringLiteral("_i18ncp")));

    // register default styles as ds* global properties
    m_engine->globalObject().setProperty(QStringLiteral("dsNormal"), KTextEditor::dsNormal);
    m_engine->globalObject().setProperty(QStringLiteral("dsKeyword"), KTextEditor::dsKeyword);
    m_engine->globalObject().setProperty(QStringLiteral("dsFunction"), KTextEditor::dsFunction);
    m_engine->globalObject().setProperty(QStringLiteral("dsVariable"), KTextEditor::dsVariable);
    m_engine->globalObject().setProperty(QStringLiteral("dsControlFlow"), KTextEditor::dsControlFlow);
    m_engine->globalObject().setProperty(QStringLiteral("dsOperator"), KTextEditor::dsOperator);
    m_engine->globalObject().setProperty(QStringLiteral("dsBuiltIn"), KTextEditor::dsBuiltIn);
    m_engine->globalObject().setProperty(QStringLiteral("dsExtension"), KTextEditor::dsExtension);
    m_engine->globalObject().setProperty(QStringLiteral("dsPreprocessor"), KTextEditor::dsPreprocessor);
    m_engine->globalObject().setProperty(QStringLiteral("dsAttribute"), KTextEditor::dsAttribute);
    m_engine->globalObject().setProperty(QStringLiteral("dsChar"), KTextEditor::dsChar);
    m_engine->globalObject().setProperty(QStringLiteral("dsSpecialChar"), KTextEditor::dsSpecialChar);
    m_engine->globalObject().setProperty(QStringLiteral("dsString"), KTextEditor::dsString);
    m_engine->globalObject().setProperty(QStringLiteral("dsVerbatimString"), KTextEditor::dsVerbatimString);
    m_engine->globalObject().setProperty(QStringLiteral("dsSpecialString"), KTextEditor::dsSpecialString);
    m_engine->globalObject().setProperty(QStringLiteral("dsImport"), KTextEditor::dsImport);
    m_engine->globalObject().setProperty(QStringLiteral("dsDataType"), KTextEditor::dsDataType);
    m_engine->globalObject().setProperty(QStringLiteral("dsDecVal"), KTextEditor::dsDecVal);
    m_engine->globalObject().setProperty(QStringLiteral("dsBaseN"), KTextEditor::dsBaseN);
    m_engine->globalObject().setProperty(QStringLiteral("dsFloat"), KTextEditor::dsFloat);
    m_engine->globalObject().setProperty(QStringLiteral("dsConstant"), KTextEditor::dsConstant);
    m_engine->globalObject().setProperty(QStringLiteral("dsComment"), KTextEditor::dsComment);
    m_engine->globalObject().setProperty(QStringLiteral("dsDocumentation"), KTextEditor::dsDocumentation);
    m_engine->globalObject().setProperty(QStringLiteral("dsAnnotation"), KTextEditor::dsAnnotation);
    m_engine->globalObject().setProperty(QStringLiteral("dsCommentVar"), KTextEditor::dsCommentVar);
    m_engine->globalObject().setProperty(QStringLiteral("dsRegionMarker"), KTextEditor::dsRegionMarker);
    m_engine->globalObject().setProperty(QStringLiteral("dsInformation"), KTextEditor::dsInformation);
    m_engine->globalObject().setProperty(QStringLiteral("dsWarning"), KTextEditor::dsWarning);
    m_engine->globalObject().setProperty(QStringLiteral("dsAlert"), KTextEditor::dsAlert);
    m_engine->globalObject().setProperty(QStringLiteral("dsOthers"), KTextEditor::dsOthers);
    m_engine->globalObject().setProperty(QStringLiteral("dsError"), KTextEditor::dsError);

    // register scripts itself
    QJSValue result = m_engine->evaluate(source, m_url);
    if (hasException(result, m_url)) {
        return false;
    }

    // AFTER SCRIPT: set the view/document objects as necessary
    m_engine->globalObject().setProperty(QStringLiteral("editor"), m_engine->newQObject(m_editor = new KateScriptEditor(m_engine)));
    m_engine->globalObject().setProperty(QStringLiteral("document"), m_engine->newQObject(m_document = new KateScriptDocument(m_engine)));
    m_engine->globalObject().setProperty(QStringLiteral("view"), m_engine->newQObject(m_view = new KateScriptView(m_engine)));

    // yip yip!
    m_loadSuccessful = true;

    return true;
}

QJSValue KateScript::evaluate(const QString &program, const FieldMap &env)
{
    if (!load()) {
        qCWarning(LOG_KTE) << "load of script failed:" << program;
        return QJSValue();
    }

    // Wrap the arguments in a function to avoid polluting the global object
    QString programWithContext =
        QLatin1String("(function(") + QStringList(env.keys()).join(QLatin1Char(',')) + QLatin1String(") { return ") + program + QLatin1String("})");
    QJSValue programFunction = m_engine->evaluate(programWithContext);
    Q_ASSERT(programFunction.isCallable());

    QJSValueList args;
    args.reserve(env.size());
    for (auto it = env.begin(); it != env.end(); it++) {
        args << it.value();
    }

    QJSValue result = programFunction.call(args);
    if (result.isError()) {
        qCWarning(LOG_KTE) << "Error evaluating script: " << result.toString();
    }

    return result;
}

bool KateScript::hasException(const QJSValue &object, const QString &file)
{
    if (object.isError()) {
        displayBacktrace(object, i18n("Error loading script %1\n", file));
        m_errorMessage = i18n("Error loading script %1", file);
        delete m_engine;
        m_engine = nullptr;
        m_loadSuccessful = false;
        return true;
    }
    return false;
}

bool KateScript::setView(KTextEditor::ViewPrivate *view)
{
    if (!load()) {
        return false;
    }
    // setup the stuff
    m_document->setDocument(view->doc());
    m_view->setView(view);
    return true;
}

void KateScript::setGeneralHeader(const KateScriptHeader &generalHeader)
{
    m_generalHeader = generalHeader;
}

KateScriptHeader &KateScript::generalHeader()
{
    return m_generalHeader;
}
