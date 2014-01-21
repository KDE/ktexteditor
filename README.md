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
    KTextEditor::View *view = editor->createView(containerWidget);

See the documentation for these classes for more information.

## Licensing

Contributions to KTextEditor shall be licensed under LGPLv2+.

## Further Documentation

- @ref kte_design
- @ref kte_howto
- @ref kte_port_to_5
- @ref kte_guidelines

## Links

- Home page: <https://projects.kde.org/projects/frameworks/ktexteditor>
- Mailing list: <https://mail.kde.org/mailman/listinfo/kde-frameworks-devel>
- IRC channel: #kde-devel on Freenode
- Git repository: <https://projects.kde.org/projects/frameworks/ktexteditor/repository>
