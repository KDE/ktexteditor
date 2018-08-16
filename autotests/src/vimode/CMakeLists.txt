set(EXECUTABLE_OUTPUT_PATH ${CMAKE_CURRENT_BINARY_DIR})

include_directories(
  ${CMAKE_SOURCE_DIR}/src/vimode
)

set (VIMODE_TEST_LINK_LIBS KF5TextEditor
  KF5::I18n
  KF5::SyntaxHighlighting
  Qt5::Qml
  Qt5::Test
)

macro(vimode_unit_test)
  ecm_add_test(${ARGN} TEST_NAME "vimode_${ARGV0}"
               LINK_LIBRARIES ${VIMODE_TEST_LINK_LIBS})
endmacro()


vimode_unit_test(modes modes.cpp base.cpp)
vimode_unit_test(view view.cpp base.cpp)
vimode_unit_test(completion completion.cpp base.cpp fakecodecompletiontestmodel.cpp)
vimode_unit_test(keys keys.cpp base.cpp fakecodecompletiontestmodel.cpp emulatedcommandbarsetupandteardown.cpp)
vimode_unit_test(emulatedcommandbar emulatedcommandbar.cpp base.cpp fakecodecompletiontestmodel.cpp emulatedcommandbarsetupandteardown.cpp)

add_custom_target(vimode_tests ${CMAKE_CTEST_COMMAND} --force-new-ctest-process -R "^vimode_")
