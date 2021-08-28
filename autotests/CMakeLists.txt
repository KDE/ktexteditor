find_package(Qt5 ${REQUIRED_QT_VERSION} CONFIG REQUIRED Test)

include(ECMMarkAsTest)
include(ECMAddTests)

remove_definitions(-DQT_NO_CAST_FROM_ASCII)

set( EXECUTABLE_OUTPUT_PATH ${CMAKE_CURRENT_BINARY_DIR} )

include_directories(
  # for config.h
  ${CMAKE_BINARY_DIR}

  # for generated ktexteditor headers
  ${CMAKE_BINARY_DIR}/src/include

  # for normal sources
  ${CMAKE_SOURCE_DIR}/src
  ${CMAKE_SOURCE_DIR}/src/include
  ${CMAKE_SOURCE_DIR}/src/buffer
  ${CMAKE_SOURCE_DIR}/src/completion
  ${CMAKE_SOURCE_DIR}/src/dialogs
  ${CMAKE_SOURCE_DIR}/src/document
  ${CMAKE_SOURCE_DIR}/src/script
  ${CMAKE_SOURCE_DIR}/src/mode
  ${CMAKE_SOURCE_DIR}/src/render
  ${CMAKE_SOURCE_DIR}/src/search
  ${CMAKE_SOURCE_DIR}/src/syntax
  ${CMAKE_SOURCE_DIR}/src/undo
  ${CMAKE_SOURCE_DIR}/src/utils
  ${CMAKE_SOURCE_DIR}/src/view
)

add_definitions(-DTEST_DATA_DIR=\"${CMAKE_SOURCE_DIR}/autotests/input/\")
add_definitions(-DJS_DATA_DIR=\"${CMAKE_SOURCE_DIR}/src/script/data/\")

set (KTEXTEDITOR_TEST_LINK_LIBS KF5TextEditor
  KF5::I18n
  KF5::GuiAddons
  KF5::SyntaxHighlighting
  Qt5::Qml
)

include(ECMMarkAsTest)

# test executable for encoding
add_executable(kateencodingtest src/kateencodingtest.cpp)
target_link_libraries(kateencodingtest ${KTEXTEDITOR_TEST_LINK_LIBS})
ecm_mark_as_test(kateencodingtest)

# test macro for encoding tests
MACRO(KTEXTEDITOR_ENCODING_TEST _encoding _testname)
  ADD_TEST (NAME encoding_${_testname}_create COMMAND kateencodingtest ${_encoding} ${CMAKE_SOURCE_DIR}/autotests/input/encoding/${_testname} ${CMAKE_CURRENT_BINARY_DIR}/${_testname} )
  ADD_TEST (NAME encoding_${_testname}_diff COMMAND ${CMAKE_COMMAND} -E compare_files ${CMAKE_SOURCE_DIR}/autotests/input/encoding/${_testname} ${CMAKE_CURRENT_BINARY_DIR}/${_testname} )
ENDMACRO(KTEXTEDITOR_ENCODING_TEST)

# add tests

# this file is utf-8, simple
KTEXTEDITOR_ENCODING_TEST ("utf-8" "utf8.txt")

# this file is latin15, but fallback should work!
KTEXTEDITOR_ENCODING_TEST ("utf-8" "latin15.txt")

# this file is utf32, little endian, but fallback should work!
KTEXTEDITOR_ENCODING_TEST ("utf-8" "utf32.txt")

# this file is utf16, little endian, but fallback should work!
KTEXTEDITOR_ENCODING_TEST ("utf-8" "utf16.txt")

# this file is utf32, big endian, but fallback should work!
KTEXTEDITOR_ENCODING_TEST ("utf-8" "utf32be.txt")

# this file is utf16, big endian, but fallback should work!
KTEXTEDITOR_ENCODING_TEST ("utf-8" "utf16be.txt")

# cyrillic utf-8
KTEXTEDITOR_ENCODING_TEST ("utf-8" "cyrillic_utf8.txt")

# cyrillic cp1251
KTEXTEDITOR_ENCODING_TEST ("utf-8" "cp1251.txt")

# cyrillic koi8-r
KTEXTEDITOR_ENCODING_TEST ("utf-8" "koi8-r.txt")

# one character latin-15 test, segfaulted
KTEXTEDITOR_ENCODING_TEST ("utf-8" "one-char-latin-15.txt")

# utf8 bom only, shouldn't produce error
#KTEXTEDITOR_ENCODING_TEST ("utf-8" "utf8-bom-only.txt") // TODO: make Kate::TextBuffer::saveBuffer save the BOM for empty files

# latin15, but coincidentally contains utf8 bom. The "bom" should be treated as normal text
KTEXTEDITOR_ENCODING_TEST ("ISO 8859-15" "latin15-with-utf8-bom.txt")

# test executable for indentation
add_executable(kateindenttest src/indenttest.cpp src/script_test_base.cpp src/testutils.cpp)
target_link_libraries(kateindenttest ${KTEXTEDITOR_TEST_LINK_LIBS}
    Qt5::Test)
ecm_mark_as_test(kateindenttest)

# test macro for indentation tests
MACRO(KTEXTEDITOR_INDENT_TEST _testname)
  ADD_TEST (NAME kateindenttest_${_testname} COMMAND kateindenttest ${_testname})
ENDMACRO(KTEXTEDITOR_INDENT_TEST)

# test different indenters sepearately to have smaller test chunks, that takes LONG
KTEXTEDITOR_INDENT_TEST ("testPython")
KTEXTEDITOR_INDENT_TEST ("testJulia")
KTEXTEDITOR_INDENT_TEST ("testCstyle")

# guard against execution with qt that has broken QJSEngine regex, see: https://bugreports.qt.io/browse/QTBUG-72879
if (NOT Qt5Widgets_VERSION VERSION_EQUAL 5.12.0)
    KTEXTEDITOR_INDENT_TEST ("testCppstyle")
endif()

KTEXTEDITOR_INDENT_TEST ("testCMake")
KTEXTEDITOR_INDENT_TEST ("testRuby")
KTEXTEDITOR_INDENT_TEST ("testHaskell")
KTEXTEDITOR_INDENT_TEST ("testLatex")
KTEXTEDITOR_INDENT_TEST ("testPascal")
KTEXTEDITOR_INDENT_TEST ("testAda")
KTEXTEDITOR_INDENT_TEST ("testXml")
KTEXTEDITOR_INDENT_TEST ("testNormal")
KTEXTEDITOR_INDENT_TEST ("testReplicode")

macro(ktexteditor_unit_test testname)
  ecm_add_test(src/${testname}.cpp ${ARGN}
               TEST_NAME ${testname}
	       LINK_LIBRARIES ${KTEXTEDITOR_TEST_LINK_LIBS} Qt5::Test)
endmacro()

ecm_add_tests(
  src/katetextbuffertest.cpp
  src/range_test.cpp
  src/undomanager_test.cpp
  src/plaintextsearch_test.cpp
  src/regexpsearch_test.cpp
  src/scriptdocument_test.cpp
  src/wordcompletiontest.cpp
  src/searchbar_test.cpp
  src/movingcursor_test.cpp
  src/configinterface_test.cpp
  src/messagetest.cpp
  src/kte_documentcursor.cpp
  src/bug313769.cpp
  src/katedocument_test.cpp
  src/movingrange_test.cpp
  src/kateview_test.cpp
  src/revision_test.cpp
  src/modificationsystem_test.cpp
  src/inlinenote_test.cpp
  src/variable_test.cpp
  src/templatehandler_test.cpp
  src/katefoldingtest.cpp
  src/bug286887.cpp
  src/katetextblocktest.cpp
  src/camelcursortest.cpp
  src/encodingtest.cpp
  LINK_LIBRARIES ${KTEXTEDITOR_TEST_LINK_LIBS} Qt5::Test
)

ktexteditor_unit_test(completion_test src/codecompletiontestmodel.cpp src/codecompletiontestmodels.cpp)
ktexteditor_unit_test(commands_test src/script_test_base.cpp src/testutils.cpp)
ktexteditor_unit_test(scripting_test src/script_test_base.cpp src/testutils.cpp)
ktexteditor_unit_test(katemodemanager_test src/katemodemanager_test_base.cpp)
ktexteditor_unit_test(bug313759 src/testutils.cpp)
ktexteditor_unit_test(bug317111 src/testutils.cpp)
ktexteditor_unit_test(bug205447 src/testutils.cpp)

add_subdirectory(src/vimode)

#
# benchmarks, don't execute during normal testing
# they don't test additional stuff but only hang the CI
#
add_executable(katemodemanager_benchmark src/katemodemanager_test_base.cpp src/katemodemanager_benchmark.cpp)
ecm_mark_nongui_executable(katemodemanager_benchmark)
add_test(NAME katemodemanager_benchmark COMMAND katemodemanager_benchmark CONFIGURATIONS BENCHMARK)
target_link_libraries(katemodemanager_benchmark ${KTEXTEDITOR_TEST_LINK_LIBS} Qt5::Test)

add_executable(bench_search src/benchmarks/bench_search.cpp)
target_link_libraries(bench_search PRIVATE ${KTEXTEDITOR_TEST_LINK_LIBS})
