/* This file is part of the KTextEditor project.
 *
 * Copyright (C) 2017 Grzegorz Szymaszek <gszymaszek@short.pl>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this library; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */

#ifndef _EDITORCONFIG_H_
#define _EDITORCONFIG_H_

#include <QLatin1String>

#include <editorconfig/editorconfig.h>
#include <editorconfig/editorconfig_handle.h>

#include "kateconfig.h"
#include "katedocument.h"
#include "kateglobal.h"
#include "katepartdebug.h"

class KateDocumentConfig;

namespace KTextEditor { class DocumentPrivate; }

class EditorConfig
{
public:
    explicit EditorConfig(KTextEditor::DocumentPrivate *document);
    ~EditorConfig();
    EditorConfig(const EditorConfig &) = delete;
    EditorConfig& operator=(const EditorConfig &) = delete;
    /**
     * Runs EditorConfig parser and sets proper parent DocumentPrivate
     * configuration. Implemented options: charset, end_of_line, indent_size,
     * indent_style, insert_final_newline, max_line_length, tab_width,
     * trim_trailing_whitespace.
     *
     * @see https://github.com/editorconfig/editorconfig/wiki/EditorConfig-Properties
     *
     * @return 0 if EditorConfig library reported successful file parsing,
     * positive integer if a parsing error occurred (the return value would be
     * the line number of parsing error), negative integer if another error
     * occurred. In other words, returns value returned by editorconfig_parse.
     */
    int parse();

private:
    KTextEditor::DocumentPrivate *m_document;
    editorconfig_handle m_handle;
};

#endif
