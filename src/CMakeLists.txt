# handle data files, .desktop & .cmake
add_subdirectory(data)

# syntax highlighting data files
add_subdirectory( syntax/data )

# jscripts for the part
add_subdirectory( script/data )

# set right defines for libgit2 usage
if(LIBGIT2_FOUND)
    add_definitions(-DLIBGIT2_FOUND)
    SET (CMAKE_REQUIRED_LIBRARIES LibGit2::LibGit2)
    set (KTEXTEDITOR_OPTIONAL_LIBS ${KTEXTEDITOR_OPTIONAL_LIBS} LibGit2::LibGit2)
endif()

# handle include files, both normal ones and generated ones
add_subdirectory(include)

# includes
include_directories(
  # for config.h
  ${CMAKE_BINARY_DIR}

  # for generated ktexteditor headers
  ${CMAKE_CURRENT_BINARY_DIR}/include

  # for normal sources
  ${CMAKE_CURRENT_SOURCE_DIR}
  ${CMAKE_CURRENT_SOURCE_DIR}/include
  ${CMAKE_CURRENT_SOURCE_DIR}/include/ktexteditor
  ${CMAKE_CURRENT_SOURCE_DIR}/buffer
  ${CMAKE_CURRENT_SOURCE_DIR}/completion
  ${CMAKE_CURRENT_SOURCE_DIR}/dialogs
  ${CMAKE_CURRENT_SOURCE_DIR}/document
  ${CMAKE_CURRENT_SOURCE_DIR}/script
  ${CMAKE_CURRENT_SOURCE_DIR}/mode
  ${CMAKE_CURRENT_SOURCE_DIR}/render
  ${CMAKE_CURRENT_SOURCE_DIR}/search
  ${CMAKE_CURRENT_SOURCE_DIR}/syntax
  ${CMAKE_CURRENT_SOURCE_DIR}/schema
  ${CMAKE_CURRENT_SOURCE_DIR}/undo
  ${CMAKE_CURRENT_SOURCE_DIR}/utils
  ${CMAKE_CURRENT_SOURCE_DIR}/inputmode
  ${CMAKE_CURRENT_SOURCE_DIR}/view
  ${CMAKE_CURRENT_SOURCE_DIR}/swapfile
  ${CMAKE_CURRENT_SOURCE_DIR}/variableeditor)

# KTextEditor interface sources
set(ktexteditor_LIB_SRCS
# text buffer & buffer helpers
buffer/katetextbuffer.cpp
buffer/katetextblock.cpp
buffer/katetextline.cpp
buffer/katetextcursor.cpp
buffer/katetextrange.cpp
buffer/katetexthistory.cpp
buffer/katetextfolding.cpp

# completion (widget, model, delegate, ...)
completion/katecompletionwidget.cpp
completion/katecompletionmodel.cpp
completion/katecompletiontree.cpp
completion/katecompletionconfig.cpp
completion/kateargumenthinttree.cpp
completion/kateargumenthintmodel.cpp
completion/katecompletiondelegate.cpp
completion/expandingtree/expandingwidgetmodel.cpp
completion/expandingtree/expandingdelegate.cpp
completion/expandingtree/expandingtree.cpp

# simple internal word completion
completion/katewordcompletion.cpp

# internal syntax-file based keyword completion
completion/katekeywordcompletion.cpp

# dialogs
dialogs/kateconfigpage.cpp
dialogs/katedialogs.cpp
dialogs/kateconfigpage.cpp

# document (THE document, buffer, lines/cursors/..., CORE STUFF)
document/katedocument.cpp
document/katebuffer.cpp

# undo
undo/kateundo.cpp
undo/katemodifiedundo.cpp
undo/kateundomanager.cpp

# scripting
script/katescript.cpp
script/kateindentscript.cpp
script/katecommandlinescript.cpp
script/katescriptmanager.cpp
script/katescriptaction.cpp

# scripting wrappers
script/katescriptdocument.cpp
script/katescriptview.cpp
script/katescripthelpers.cpp

# mode (modemanager and co)
mode/katemodemanager.cpp
mode/katemodeconfigpage.cpp
mode/katemodemenu.cpp
mode/katewildcardmatcher.cpp

# modeline variable editor
variableeditor/variablelineedit.cpp
variableeditor/variablelistview.cpp
variableeditor/variableeditor.cpp
variableeditor/variableitem.cpp
variableeditor/katehelpbutton.cpp

# printing classes
printing/kateprinter.cpp
printing/printpainter.cpp
printing/printconfigwidgets.cpp

# rendering stuff (katerenderer  and helpers)
render/katerenderer.cpp
render/katerenderrange.cpp
render/katelayoutcache.cpp
render/katetextlayout.cpp
render/katelinelayout.cpp

# search stuff
search/kateregexp.cpp
search/kateplaintextsearch.cpp
search/kateregexpsearch.cpp
search/katematch.cpp
search/katesearchbar.cpp

# syntax related stuff (highlighting, xml file parsing, ...)
syntax/katesyntaxmanager.cpp
syntax/katehighlight.cpp
syntax/katehighlighthelpers.cpp
syntax/katehighlightmenu.cpp
syntax/katesyntaxdocument.cpp
syntax/katehighlightingcmds.cpp

# view stuff (THE view and its helpers)
view/kateview.cpp
view/kateviewinternal.cpp
view/kateviewhelpers.cpp
view/katemessagewidget.cpp
view/katefadeeffect.cpp
view/kateanimation.cpp
view/katetextanimation.cpp
view/katestatusbar.cpp
view/wordcounter.cpp

# spell checking
spellcheck/prefixstore.h
spellcheck/prefixstore.cpp
spellcheck/ontheflycheck.h
spellcheck/ontheflycheck.cpp
spellcheck/spellcheck.h
spellcheck/spellcheck.cpp
spellcheck/spellcheckdialog.h
spellcheck/spellcheckdialog.cpp
spellcheck/spellcheckbar.cpp
spellcheck/spellingmenu.h
spellcheck/spellingmenu.cpp

# generic stuff, unsorted...
utils/katecmds.cpp
utils/kateconfig.cpp
utils/katebookmarks.cpp
utils/kateautoindent.cpp
utils/katetemplatehandler.cpp
utils/kateglobal.cpp
utils/katecmd.cpp
utils/ktexteditor.cpp
utils/document.cpp
utils/range.cpp
utils/documentcursor.cpp
utils/attribute.cpp
utils/codecompletioninterface.cpp
utils/codecompletionmodel.cpp
utils/codecompletionmodelcontrollerinterface.cpp
utils/configinterface.cpp
utils/movinginterface.cpp
utils/movingcursor.cpp
utils/movingrange.cpp
utils/movingrangefeedback.cpp
utils/messageinterface.cpp
utils/application.cpp
utils/mainwindow.cpp
utils/katedefaultcolors.cpp
utils/katecommandrangeexpressionparser.cpp
utils/katesedcmd.cpp

# schema
schema/kateschema.cpp
schema/kateschemaconfig.cpp
schema/katestyletreewidget.cpp
schema/katecolortreewidget.cpp
schema/katecategorydrawer.cpp

# swapfile
swapfile/kateswapdiffcreator.cpp
swapfile/kateswapfile.cpp

# export as HTML
export/exporter.cpp
export/htmlexporter.cpp

# input modes
inputmode/kateabstractinputmode.cpp
inputmode/kateabstractinputmodefactory.cpp
inputmode/katenormalinputmode.cpp
inputmode/katenormalinputmodefactory.cpp

# syntax hl files as resource file
"${CMAKE_CURRENT_BINARY_DIR}/syntax_resource.cpp"
)

# tool to create json index of highlightings, will validate all highlightings, too!
add_executable(katehighlightingindexer syntax/data/katehighlightingindexer.cpp)
target_link_libraries(katehighlightingindexer Qt5::XmlPatterns)

# generate the hl index resource
add_custom_command(OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/syntax_resource.cpp"
  COMMAND katehighlightingindexer "${CMAKE_CURRENT_BINARY_DIR}/syntax/data/index.json" "${CMAKE_CURRENT_SOURCE_DIR}/syntax/data/language.xsd" "${CMAKE_CURRENT_BINARY_DIR}/syntax/data/index.qrc"
  COMMAND ${Qt5Core_RCC_EXECUTABLE} -o "${CMAKE_CURRENT_BINARY_DIR}/syntax_resource.cpp" "${CMAKE_CURRENT_BINARY_DIR}/syntax/data/index.qrc"
  DEPENDS GeneratePhpXmlFiles
)

ki18n_wrap_ui(ktexteditor_LIB_SRCS
dialogs/textareaappearanceconfigwidget.ui
dialogs/bordersappearanceconfigwidget.ui
dialogs/commandmenuconfigwidget.ui
dialogs/commandmenueditwidget.ui
dialogs/completionconfigtab.ui
dialogs/navigationconfigwidget.ui
dialogs/editconfigwidget.ui
dialogs/filetypeconfigwidget.ui
dialogs/indentationconfigwidget.ui
dialogs/opensaveconfigwidget.ui
dialogs/opensaveconfigadvwidget.ui
dialogs/modonhdwidget.ui
dialogs/completionconfigwidget.ui
search/searchbarincremental.ui
search/searchbarpower.ui
spellcheck/spellcheckbar.ui
dialogs/spellcheckconfigwidget.ui
schema/howtoimportschema.ui )

# add the resource files, the one with mascot + ui file and the generated ones
qt5_add_resources( ktexteditor_LIB_SRCS data/ktexteditor.qrc "${CMAKE_CURRENT_BINARY_DIR}/script/data/script.qrc")

if (BUILD_VIMODE)
  ki18n_wrap_ui(ktexteditor_LIB_SRCS vimode/config/configwidget.ui)
  set(ktexteditor_LIB_SRCS ${ktexteditor_LIB_SRCS}
    inputmode/kateviinputmode.cpp
    inputmode/kateviinputmodefactory.cpp

    # vi input mode
    vimode/config/configtab.cpp

    vimode/modes/insertvimode.cpp
    vimode/modes/modebase.cpp
    vimode/modes/normalvimode.cpp
    vimode/modes/replacevimode.cpp
    vimode/modes/visualvimode.cpp

    vimode/appcommands.cpp
    vimode/cmds.cpp
    vimode/inputmodemanager.cpp
    vimode/command.cpp
    vimode/motion.cpp
    vimode/range.cpp
    vimode/keyparser.cpp
    vimode/globalstate.cpp
    vimode/emulatedcommandbar.cpp
    vimode/commandrangeexpressionparser.cpp
    vimode/keymapper.cpp
    vimode/marks.cpp
    vimode/jumps.cpp
    vimode/history.cpp
    vimode/macros.cpp
    vimode/mappings.cpp
    vimode/registers.cpp
    vimode/searcher.cpp
    vimode/completion.cpp
    vimode/completionrecorder.cpp
    vimode/completionreplayer.cpp
    vimode/macrorecorder.cpp
    vimode/lastchangerecorder.cpp
  )
endif()

add_library(KF5TextEditor ${ktexteditor_LIB_SRCS} ${KTEXTEDITOR_PUBLIC_HEADERS})
generate_export_header(KF5TextEditor BASE_NAME KTextEditor)
add_library(KF5::TextEditor ALIAS KF5TextEditor)

target_include_directories(KF5TextEditor INTERFACE "$<INSTALL_INTERFACE:${KDE_INSTALL_INCLUDEDIR_KF5}/KTextEditor>")

# API is more or less KParts++, other stuff is used only internally
target_link_libraries(KF5TextEditor
PUBLIC
  KF5::Parts
PRIVATE
  Qt5::Script
  Qt5::PrintSupport
  KF5::I18n
  KF5::Archive
  KF5::GuiAddons
  KF5::IconThemes
  KF5::ItemViews
  KF5::SonnetCore
  ${KTEXTEDITOR_OPTIONAL_LIBS}
)

set_target_properties(KF5TextEditor
   PROPERTIES VERSION ${KTEXTEDITOR_VERSION_STRING}
   SOVERSION ${KTEXTEDITOR_SOVERSION}
   EXPORT_NAME "TextEditor"
)

install(TARGETS KF5TextEditor EXPORT KF5TextEditorTargets ${KF5_INSTALL_TARGETS_DEFAULT_ARGS})

install(FILES
  ${CMAKE_CURRENT_BINARY_DIR}/ktexteditor_export.h
  DESTINATION ${KDE_INSTALL_INCLUDEDIR_KF5}/KTextEditor
  COMPONENT Devel
)

include(ECMGeneratePriFile)
ecm_generate_pri_file(BASE_NAME KTextEditor LIB_NAME KF5TextEditor DEPS "KParts" FILENAME_VAR PRI_FILENAME INCLUDE_INSTALL_DIR ${KDE_INSTALL_INCLUDEDIR_KF5}/KTextEditor)
install(FILES ${PRI_FILENAME} DESTINATION ${ECM_MKSPECS_INSTALL_DIR})

# add part
add_subdirectory(part)
