# KTextEditor

Full text editor component

## Introduction

KTextEditor provides a powerful text editor component that you can embed in your
application, either as a KPart or using the KF5::TextEditor library (if you need
more control).

The text editor component contains many useful features, from syntax
highlighting and automatic indentation to advanced scripting support, making it
suitable for everything from a simple embedded text-file editor to an advanced
IDE.

## Usage

### KPart

As with other KParts, you should use KParts::MainWindow as your main window.
You can directly request "katepart", as in

    KService::Ptr service = KService::serviceByDesktopPath("katepart");
    if (service) {
        m_part = service->createInstance<KParts::ReadWritePart>(0);
    }

See the KParts documentation for more information on using KParts.

### Library

If you are using CMake, you need to have

    find_package(KF5TextEditor NO_MODULE)

(or similar) in your CMakeLists.txt file, and you need to link to
KF5::TextEditor.

After that, you can use KTextEditor::Editor to create an editor instance, and
use that to manage KTextEditor::Document instances.

    #include <KTextEditor/Document>
    #include <KTextEditor/Editor>
    #include <KTextEditor/View>

    KTextEditor::Editor *editor = KTextEditor::Editor::instance();
    // create a new document
    KTextEditor::Document *doc = editor->createDocument(this);
    // create a widget to display the document
    KTextEditor::View *view = doc->createView(containerWidget);

See the documentation for these classes for more information.

## Licensing

Contributions to KTextEditor shall be licensed under LGPLv2+.

All files shall contain a proper "SPDX-License-Identifier: LGPL-2.0-or-later" identifier inside a header like:

/*  SPDX-License-Identifier: LGPL-2.0-or-later

    Copyright (C) 2019 Christoph Cullmann <cullmann@kde.org>

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to
    the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
    Boston, MA 02110-1301, USA.
*/

## Further Documentation

- @ref kte_design
- @ref kte_port_to_5
- @ref kte_guidelines

